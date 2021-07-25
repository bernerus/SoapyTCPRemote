# SoapyTCPRemote
TCP based SoapySDR remoting, no clever protocols, just a TCP/IP byte stream.

Note: currently developed and tested on Debian stable only.. ports welcome (see below!)

## Why?
Because I've tried SoapyRemote, and it always has streaming errors due to network underrun/audio overrun,
which is daft when I have a solid 80Mbits/sec (10Mbytes/sec) from the source device to my laptop (measured
using a simple `scp` command).

I've also tried USB/IP, which is a total loss for anything faster than keyboards.

I've tried a network audio solution [TRX](http://www.pogo.org.uk/%7Emark/trx/streaming-desktop-audio.html),
which looks good with RTP for networking, but sadly due to having to switch between user and kernel
mode for every UDP packet at 500Hz+ thrashes my wee source device to kernel panic &amp; death =(

## How?
By using the same approach as simple TCP/IP network protocols, use the capabilities of the underlying
TCP/IP stack which is reliable, efficiently and implemented in the kernel so I don't have problems
due to process/thread thrashing. Atop this, use `stdio` to manage buffering and data segmentation
(less code for me to write), and finally, don't aim for stupid low latency, choose buffer sies that
provide ~250msecs delay, this again reduces thrashing on the source device (which is tiny =)).

## Building
You will need `libsoapysdr-dev`, `cmake` and `build-essentials`

Then it's the usual CMake incantations (on both source and target devices!):
 * `cmake -B build`
 * `cmake --build build`
 * `su` or `sudo -s`
 * `cd build; make install`
 
After which you should be able to check the driver is installed with `SoapySDRUtil --info` and run
the server (on the device where your SDR is attached) `SoapyTCPServer`.

## Usage
 * Run the server on the target device: `SoapyTCPServer`
 * Connect from the client: `SoapySDRUtil --probe=driver=tcpremote,tcpremote:address=<serverIP>,tcpremote:driver=<serverSDR>`
 * Once you have a working conneciton string, use in your favourite SDR package such as gqrx.
 
## Debugging
So it's not working first time? You can get significant details by setting the SoapySDR log level in the environment:
 * `SOAPY_SDR_LOG_LEVEL=<VALUE>` where `<VALUE>` is one of: `ERROR, WARNING, NOTICE, INFO (def), DEBUG, TRACE`
 
## Feedback
Feel free to raise issues for discussion or problems or features, better yet: submit PRs to fix my poor code please!
