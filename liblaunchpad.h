/*
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of California, Berkeley nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <signal.h>
#include <alsa/asoundlib.h>

// launchpad identifiers
#define ID_VENDOR  0x1235
#define ID_PRODUCT 0x000E

// size to use for buffers
#define MAX_PACKET_SIZE 8

// usb endpoints
#define EP_IN      ( LIBUSB_ENDPOINT_IN  | 1)
#define EP_OUT     ( LIBUSB_ENDPOINT_OUT | 2)

// midi channels used for communication
#define CTRL 0xB0 // control change, channel 1
#define NOTE 0x90 // note on, channel 1
#define NOTE_ON 0x90 // note on, channel 1
#define NOTE_OFF 0x80 // note off, channel 1

/**
 * the principal struct to handle the launchpad
 */
struct launchpad {
    struct libusb_device_handle* device;	//! usb device
    unsigned char* rdata;			//! buffer to store incoming data
    unsigned char* tdata;			//! buffer to store outgoing data
    
    // handling the protocol's state
    unsigned int prefix;			//! current prefix byte (CTRL or NOTE)
    int received;				//! amount of data currently stored in rdata
    int parse_at;				//! where to read in rdata to get the next event
    snd_seq_event_t event;		       	//! store the parsed midi event
};

/**
 * a boolean type
 */
enum bool {
    false = 0,
    true  = 1 };

/**
 * buffer identifiers. the launchpad has two buffers
 */
enum buffer {
    buffer0 = 0,
    buffer1 = 1 };

/**
 * different intensities of red
 */
enum red {
	red_low		= 0x01,
	red_medium	= 0x02,
	red_full	= 0x03 };

/**
 * different intensities of green
 */
enum green {
    green_low		= 0x10,
    green_medium	= 0x20,
    green_full		= 0x30 };

/**
 * when setting a led, you can either :
 * - set only one buffer
 * - set both buffers
 * - set one buffer and clear the other
 */
enum led_mode {
    led_copy	= 0x0C,
    led_clear	= 0x08 };

/** 
 * register the launchpad 
 */
struct launchpad *lp_register();

/**
 * deregister the launchpad
 */
void lp_deregister(struct launchpad* lp);

/** receive data from the launchpad 
 * 
 * this is a blocking function waiting for some data to arrive to the
 * launchpad. Received data is stored inside launchpad->rdata, then parsed as a
 * midi event in launchpad->event. Sometimes, several events are received at
 * once. In such a case, only the first event is parsed. Next time the function
 * is called, the next event in the buffer is parsed until the buffer is empty.
 */
void lp_receive(struct launchpad* lp);

/** send data to the launchpad
 *
 * the data has to be written to lp->tdata before calling this function.
 * \param size the amount of data to send
 */
int lp_send(struct launchpad* lp, int size);

/** send a standard three bytes message
 * 
 * all messages sent to the launchpad are three bytes long.
 * \param data0 first byte
 * \param data1 second byte
 * \param data2 third byte
 */
int lp_send3(struct launchpad* lp, unsigned int data0, unsigned int data1, unsigned int data2);

/**
 * turn on all the leds
 * \param intensity the leds' intensity, between 1 and 3
 */
int lp_check(struct launchpad* lp, int intensity);

/**
 * reset the launchpad
 */
int lp_reset(struct launchpad* lp);

/** set the launchpad's mode
 *
 * \param displaying the buffer to display
 * \param updating the buffer to update
 * \param flashing whether the launchpad should display both buffers alternatively
 * \param copy whether we should copy the content of the newly displaying buffer into the newly updating buffer
 */
int lp_setmode(struct launchpad* lp, enum buffer displaying, enum buffer updating, enum bool flashing, enum bool copy);
