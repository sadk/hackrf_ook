/****************************************************************************************************************************************************/
/*  Purpose:    Simple demo of using libhackrf to transmit a two-tone beep.                                                                         */
/*  Author:     Copyright (c) 2015, W.B.Hill ( M1BKF) <bugs@wbh.org> All rights reserved.                                                           */
/*  License:    Derived from hackrf_transfer, any parts written by my released into the public domain.                                              */
/****************************************************************************************************************************************************/

/*
 * This transmits at the transmit frequency, freq, +800KHz (the HamRadio APRS frequency here.)
 * At a sample rate of 8M samples/s, for 800KHz there are 10 samples per carrier wave.
 *
 * The Mark and Space frequcies are 1200Hz and 2200Hz respectively, so 6666 and 3636 samples per wave.
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

// Transmit frequency.
const uint64_t freq = 27195000L;
// Sample rate.
const uint32_t samplerate = 8000000;
// Transmitter IF gain.
const unsigned int gain = 47;

int8_t txbuffer[OOK_MSG_SIZE] = { 0 };
int bufferOffset;

// Handle for the HackRF
static hackrf_device* device = NULL;

// Time to give up?
volatile bool do_exit = false;


// Dump more data to the HackRF
int tx_callback(hackrf_transfer* transfer)
{
	// How much to do?
	size_t count = transfer->valid_length;
	// Copy it over.
	int i = 0;

	while (i < count) {
		(transfer->buffer)[i++] = txbuffer[bufferOffset];  // I
		(transfer->buffer)[i++] = txbuffer[bufferOffset];  // Q
		bufferOffset++;
		bufferOffset %= OOK_MSG_SIZE;
	}
	return 0 ;
}


// Deal with interruptions.
void sigint_callback_handler (int signum)
{
	fprintf(stderr, "Caught signal %d\n", signum);
	do_exit = true;
}


int main (int argc, char** argv)
{
	int result;

	// bits to send
	char bits[] = "01010010 01010110 11110011";

	// Catch signals that we want to handle gracefully.
	signal(SIGINT, &sigint_callback_handler);
	signal(SIGILL, &sigint_callback_handler);
	signal(SIGFPE, &sigint_callback_handler);
	signal(SIGSEGV, &sigint_callback_handler);
	signal(SIGTERM, &sigint_callback_handler);
	signal(SIGABRT, &sigint_callback_handler);

	fprintf(stderr, "Precalculating lookup tables...\n");

	/*
	 * Precalc waveforms.
	 */
	// preamble
	int s = 0;
	while (s < OOK_START) { txbuffer[s] = 127;	s++; }

	// bits
	for (int i = 0; i < OOK_NBR_BITS; i++) {
		if (bits[i] == '0') {
			s = OOK_START+(OOK_BIT*i)+OOK_0;
			printf("0");
		} else {
			s = OOK_START+(OOK_BIT*i)+OOK_1;
			printf("1");
		}
		while (s < OOK_START+(OOK_BIT*(i+1))) { txbuffer[s] = 127; s++; }
	}
	printf("\n");

	/*
	// bit 0
	s = OOK_START+(OOK_BIT*0)+OOK_0;
    while (s < OOK_START+(OOK_BIT*1)) { txbuffer[s] = 127; s++; }
	// bit 1
	s = OOK_START+(OOK_BIT*1)+OOK_1;
	while (s < OOK_START+(OOK_BIT*2)) { txbuffer[s] = 127; s++; }
	// bit 2
	s = OOK_START+(OOK_BIT*2)+OOK_0;
	while (s < OOK_START+(OOK_BIT*3)) { txbuffer[s] = 127; s++; }
	// bit 3
	s = OOK_START+(OOK_BIT*3)+OOK_1;
	while (s < OOK_START+(OOK_BIT*4)) { txbuffer[s] = 127; s++; }
	// bit 4
	s = OOK_START+(OOK_BIT*4)+OOK_0;
	while (s < OOK_START+(OOK_BIT*5)) { txbuffer[s] = 127; s++; }
	// bit 5
	s = OOK_START+(OOK_BIT*5)+OOK_0;
	while (s < OOK_START+(OOK_BIT*6)) { txbuffer[s] = 127; s++; }
	// bit 6
	s = OOK_START+(OOK_BIT*6)+OOK_1;
	while (s < OOK_START+(OOK_BIT*7)) { txbuffer[s] = 127; s++; }
	// bit 7
	s = OOK_START+(OOK_BIT*7)+OOK_0;
	while (s < OOK_START+(OOK_BIT*8)) { txbuffer[s] = 127; s++; }
	// bit 8
	s = OOK_START+(OOK_BIT*8)+OOK_0;
	while (s < OOK_START+(OOK_BIT*9)) { txbuffer[s] = 127; s++; }
	// bit 9
	s = OOK_START+(OOK_BIT*9)+OOK_1;
	while (s < OOK_START+(OOK_BIT*10)) { txbuffer[s] = 127; s++; }
	// bit 10
	s = OOK_START+(OOK_BIT*10)+OOK_0;
	while (s < OOK_START+(OOK_BIT*11)) { txbuffer[s] = 127; s++; }
	// bit 11
	s = OOK_START+(OOK_BIT*11)+OOK_1;
	while (s < OOK_START+(OOK_BIT*12)) { txbuffer[s] = 127; s++; }
	// bit 12
	s = OOK_START+(OOK_BIT*12)+OOK_0;
	while (s < OOK_START+(OOK_BIT*13)) { txbuffer[s] = 127; s++; }
	// bit 13
	s = OOK_START+(OOK_BIT*13)+OOK_1;
	while (s < OOK_START+(OOK_BIT*14)) { txbuffer[s] = 127; s++; }
	// bit 14
	s = OOK_START+(OOK_BIT*14)+OOK_1;
	while (s < OOK_START+(OOK_BIT*15)) { txbuffer[s] = 127; s++; }
	// bit 15
	s = OOK_START+(OOK_BIT*15)+OOK_0;
	while (s < OOK_START+(OOK_BIT*16)) { txbuffer[s] = 127; s++; }
	// bit 16
	s = OOK_START+(OOK_BIT*16)+OOK_1;
	while (s < OOK_START+(OOK_BIT*17)) { txbuffer[s] = 127; s++; }
	// bit 17
	s = OOK_START+(OOK_BIT*17)+OOK_1;
	while (s < OOK_START+(OOK_BIT*18)) { txbuffer[s] = 127; s++; }
	// bit 18
	s = OOK_START+(OOK_BIT*18)+OOK_1;
	while (s < OOK_START+(OOK_BIT*19)) { txbuffer[s] = 127; s++; }
	// bit 19
	s = OOK_START+(OOK_BIT*19)+OOK_1;
	while (s < OOK_START+(OOK_BIT*20)) { txbuffer[s] = 127; s++; }
	// bit 20
	s = OOK_START+(OOK_BIT*20)+OOK_0;
	while (s < OOK_START+(OOK_BIT*21)) { txbuffer[s] = 127; s++; }
	// bit 21
	s = OOK_START+(OOK_BIT*21)+OOK_0;
	while (s < OOK_START+(OOK_BIT*22)) { txbuffer[s] = 127; s++; }
	// bit 22
	s = OOK_START+(OOK_BIT*22)+OOK_1;
	while (s < OOK_START+(OOK_BIT*23)) { txbuffer[s] = 127; s++; }
	// bit 23
	s = OOK_START+(OOK_BIT*23)+OOK_1;
	while (s < OOK_START+(OOK_BIT*24)) { txbuffer[s] = 127; s++; }
	*/

	/* Setup the HackRF for transmitting at full power, 8M samples/s, 144MHz */
	fprintf(stderr, "Setting up the HackRF...\n");

	// Initialize the HackRF.
	result = hackrf_init();
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_init() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Open the HackRF.
	result = hackrf_open(&device);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_open() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Set the sample rate.
	result = hackrf_set_sample_rate_manual(device, samplerate, 1);
	if(result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_sample_rate_set() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Set the filter bandwith to default.
	result = hackrf_set_baseband_filter_bandwidth(device, hackrf_compute_baseband_filter_bw_round_down_lt(samplerate));
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_baseband_filter_bandwidth_set() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Set the gain.
	result = hackrf_set_txvga_gain(device, gain);
	result |= hackrf_start_tx(device, tx_callback, NULL);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_start_tx() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Set the transmit frequency.
	result = hackrf_set_freq(device, freq);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_set_freq() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Turn on the amp.
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
	// That's all, folks!!!
	return EXIT_SUCCESS;
}
