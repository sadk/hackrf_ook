/*
 * Copyright (c) 2016, Denis Bodor
 *
 * Simple and ugly try to send ASK-OOK with hackrf
 * Derived from W.B.Hill ( M1BKF) <bugs@wbh.org> code hackrf_beep.c
 * (https://github.com/rufty/hackrf_beep)
 *
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <libhackrf/hackrf.h>

#define OOK_START		49248	// nbr samples for preamble
#define OOK_BIT			16832	// nbr samples for a bit
#define OOK_0			10944	// start position for a 0 (relative to OOK_BIT)
#define OOK_1			5536	// start position for a 1 (relative to OOK_BIT)
#define OOK_PAUSE		244384	// trailer
#define OOK_NBR_BITS	24		// nbr of bits in the message
#define OOK_MSG_SIZE	OOK_START+(OOK_BIT*OOK_NBR_BITS)+OOK_PAUSE
#define OOK_NEGATE		0
#define OOK_DEFAULT_MSG "010100100101011011110011"

// Transmit frequency
const uint64_t freq = 27195000L;
// Sample rate
const uint32_t samplerate = 8000000;
// Transmitter IF gain
const unsigned int gain = 47;

//int8_t txbuffer[OOK_MSG_SIZE] = { 0 };
int8_t *txbuffer;
int bufferOffset;

// bits to send
//char bits[] = "010100100101011011110011";
char *bits;

static hackrf_device* device = NULL;

volatile bool do_exit = false;

// Giving data to the HackRF
int tx_callback(hackrf_transfer* transfer)
{
	// How much to do?
	size_t count = transfer->valid_length;
	
	int i = 0;
	while (i < count) {
		// Not really sure what i'm doing here...
		(transfer->buffer)[i++] = txbuffer[bufferOffset];  // I
		//(transfer->buffer)[i++] = txbuffer[bufferOffset];  // Q
		(transfer->buffer)[i++] = 0;  // Q
		bufferOffset++;
		bufferOffset %= OOK_MSG_SIZE; // loop on the buffer
	}
	return 0 ;
}

// Deal with interruptions.
void sigint_callback_handler (int signum)
{
	fprintf(stderr, "Caught signal %d\n", signum);
	do_exit = true;
}

void printhelp(char *binname) {
	printf("Usage : %s [OPTIONS]\n", binname);
	printf(" -s us                preamble duration in microseconds (default -s %d)\n", OOK_START/8);
	printf(" -b us                overall bit duration in microseconds (default -b %d)\n", OOK_BIT/8);
	printf(" -0 us                width of gap for bit 0 in microseconds (default -0 %d)\n", OOK_0/8);
	printf(" -1 us                width of gap for bit 1 in microseconds (default -1 %d)\n", OOK_1/8);
	printf(" -p us                trailing duration after message in microseconds (default -p %d)\n", OOK_PAUSE/8);
	printf(" -m binary_message    send this bits  (default -m %s)\n", OOK_DEFAULT_MSG);
	printf(" -n                   bitwise NOT all bit\n");
	printf(" -h                   show this help\n");
}

int main (int argc, char** argv)
{
	int result;
	int retopt;
	int opt = 0;
	char *endptr;

	int ook_start = OOK_START;
	int ook_bit = OOK_BIT;
	int ook_0 = OOK_0;
	int ook_1 = OOK_1;
	int ook_pause = OOK_PAUSE;
	int ook_nbr_bits = OOK_NBR_BITS;
	int ook_msg_size = OOK_MSG_SIZE;
	int ook_negate = OOK_NEGATE;

	while ((retopt = getopt(argc, argv, "hs:b:0:1:p:m:n")) != -1) {
		switch (retopt) {
			case 'h':
				printhelp(argv[0]);
				return(EXIT_SUCCESS);
				opt++;
				break;
			case 's':
				ook_start = (int)strtol(optarg, &endptr, 10) * 8;
				if (endptr == optarg || ook_start == 0) {
					printf("You must specify a valid number\n");
					return(EXIT_SUCCESS);
				}
				opt++;
				break;
			case 'b':
				ook_bit = (int)strtol(optarg, &endptr, 10) * 8;
				if (endptr == optarg || ook_bit == 0) {
					printf("You must specify a valid number\n");
					return(EXIT_SUCCESS);
				}
				opt++;
				break;
			case '0':
				ook_0 = (int)strtol(optarg, &endptr, 10) * 8;
				if (endptr == optarg || ook_0 == 0) {
					printf("You must specify a valid number\n");
					return(EXIT_SUCCESS);
				}
				opt++;
				break;
			case '1':
				ook_1 = (int)strtol(optarg, &endptr, 10) * 8;
				if (endptr == optarg || ook_1 == 0) {
					printf("You must specify a valid number\n");
					return(EXIT_SUCCESS);
				}
				opt++;
				break;
			case 'p':
				ook_pause = (int)strtol(optarg, &endptr, 10) * 8;
				if (endptr == optarg || ook_pause == 0) {
					printf("You must specify a valid number\n");
					return(EXIT_SUCCESS);
				}
				opt++;
				break;
			case 'm':
				ook_nbr_bits = strlen(optarg);
				bits = strdup(optarg);
				opt++;
				break;
			case 'n':
				ook_negate = 1;
				opt++;
				break;
			default:
				printhelp(argv[0]);
				return(EXIT_FAILURE);
		}
	}

	if(bits == NULL) 
		bits = strdup(OOK_DEFAULT_MSG);

	ook_msg_size = ook_start+(ook_bit*ook_nbr_bits)+ook_pause;
	printf("Allocating %d samples (%d bytes)\n", ook_msg_size, ook_msg_size*sizeof(int8_t));
	txbuffer = malloc(ook_msg_size*sizeof(int8_t));
	if (txbuffer == NULL) {
		printf("Error allocating memory!\n");
		return(EXIT_FAILURE);
	}
	memset(txbuffer, 0, ook_msg_size*sizeof(int8_t));

	fprintf(stderr, "Precalculating lookup tables...\n");

	/*
	 * Precalc waveforms.
	 */
	// preamble
	int s = 0;
	while (s < ook_start) { txbuffer[s] = 127;	s++; }

	// bits
	for (int i = 0; i < ook_nbr_bits; i++) {
		if (bits[i] == '0') {
			s = ook_negate > 0 ? ook_start+(ook_bit*i)+ook_1 : ook_start+(ook_bit*i)+ook_0;
			printf("0");
		} else {
			s = ook_negate > 0 ? ook_start+(ook_bit*i)+ook_0 : ook_start+(ook_bit*i)+ook_1;
			printf("1");
		}
		while (s < ook_start+(ook_bit*(i+1))) { txbuffer[s] = 127; s++; }
	}
	printf("\n");

	// Catch signals that we want to handle gracefully.
	signal(SIGINT, &sigint_callback_handler);
	signal(SIGILL, &sigint_callback_handler);
	signal(SIGFPE, &sigint_callback_handler);
	signal(SIGSEGV, &sigint_callback_handler);
	signal(SIGTERM, &sigint_callback_handler);
	signal(SIGABRT, &sigint_callback_handler);

	/* Setup the HackRF for transmitting at full power, 8M samples/s, ~27MHz */
	fprintf(stderr, "Setting up the HackRF...\n");

	// Initialize the HackRF
	result = hackrf_init();
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_init() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Open the HackRF
	result = hackrf_open(&device);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_open() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Set the sample rate
	result = hackrf_set_sample_rate_manual(device, samplerate, 1);
	if(result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_sample_rate_set() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Set the filter bandwith to default
	result = hackrf_set_baseband_filter_bandwidth(device, hackrf_compute_baseband_filter_bw_round_down_lt(samplerate));
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_baseband_filter_bandwidth_set() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Set the gain
	result = hackrf_set_txvga_gain(device, gain);
	result |= hackrf_start_tx(device, tx_callback, NULL);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_start_tx() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Set the transmit frequency
	result = hackrf_set_freq(device, freq);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_set_freq() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Turn on the amp
	result = hackrf_set_amp_enable(device, (uint8_t)1) ;
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_set_amp_enable() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	/* Transmitting */
	fprintf(stderr, "Transmitting, stop with Ctrl-C\n");

	// Spin until done or killed.
	while ((hackrf_is_streaming(device) == HACKRF_TRUE) && (do_exit == false)) { 
		sleep (1);
	}

	/* Clean up and shut down */
	result = hackrf_is_streaming(device);
	if (do_exit) {
		printf("\nUser cancel, exiting...\n");
	} else {
		fprintf(stderr, "\nExiting... hackrf_is_streaming() result: %s (%d)\n", hackrf_error_name(result), result);
	}

	// Shut down the HackRF.
	if (device != NULL) {
		result = hackrf_stop_tx(device);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr, "hackrf_stop_tx() failed: %s (%d)\n", hackrf_error_name(result), result);
		}
		result = hackrf_close(device);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr, "hackrf_close() failed: %s (%d)\n", hackrf_error_name(result), result);
		}
		hackrf_exit();
	}

	free(txbuffer);

	return EXIT_SUCCESS;
}
