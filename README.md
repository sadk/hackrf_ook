# hackrf_ook
Great Scott Gadgets HackRF OOK transmit poc.

This is an attempt to make [HackRF One](https://greatscottgadgets.com/hackrf) emit ASK-OOK signal. The primary test setup is to emulate a 27 Mhz Cardin S46 TX2 garage door remote control.
`hackrf_ook` compute samples and use libhackrf to transmit the signal with default parameters or values specified on the command line.

Current available options are :

- `-s µs` : preamble duration in microseconds (default -s 6156)
- `-b µs` : overall bit duration in microseconds (default -b 2104)
- `-0 µs` : width of gap for bit 0 in microseconds (default -0 1368)
- `-1 µs` : width of gap for bit 1 in microseconds (default -1 692)
- `-p µs` : trailing duration after message in microseconds (default -p 30548)
- `-m binary_message` : send this bits  (default -m 010100100101011011110011)
- `-n` : bitwise NOT all bit
- `-h` : show this help

```
                                                                                                       /  /
 preamble | bit | bit | bit | bit | bit | bit | bit | bit | bit | bit | bit | bit | bit | bit |       /  / bit | bit |     pause
          |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  10 |  11 |  12 |  13 |      /  /   22 |  23 |
          |     |     |     |     |     |     |     |     |     |     |     |     |     |     |     /  /       |     |
__________| ____|   __| ____|   __| ____|   __| ____|   __| ____|   __| ____|   __| ____|   __|    /  /    ____|   __|
|         | |   |   | | |   |   | | |   |   | | |   |   | | |   |   | | |   |   | | |   |   | |   /  /     |   |   | |
|         |_|   |___| |_|   |___| |_|   |___| |_|   |___| |_|   |___| |_|   |___| |_|   |___| |  /  /     _|   |___| |_________________
          |     |     |     |     |     |     |     |     |     |     |     |     |     |     | /  /           |     |
          |     |     |     |     |     |     |     |     |     |     |     |     |     |     |/  /            |     |
	                                                                                      /  /


Bits configuration :

|       |_____________|
|       |             |
|       |             |
|       |             |
|       |             |
|       |             |
|_______|             |
|       |             |
|       |             |
<- gap ->                   (-0 or -1 option)
<----- bit width ----->     (-b option)
```
