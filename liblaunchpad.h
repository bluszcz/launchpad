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

// usb endpoints
#define EP_IN      ( LIBUSB_ENDPOINT_IN  | 1)
#define EP_OUT     ( LIBUSB_ENDPOINT_OUT | 2)

// midi channels used for communication
#define CTRL 0xB0 // control change, channel 1
#define NOTE 0x90 // note on, channel 1
#define NOTE_ON 0x90
#define NOTE_OFF 0x80

/**
 * a boolean type
 */
enum bool {
    false = 0,
    true = 1 };

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
	red_off		= 0x00,
	red_low		= 0x01,
	red_medium	= 0x02,
	red_full	= 0x03 };

/**
 * different intensities of green
 */
enum green {
    green_off		= 0x00,
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
    nothing	= 0x00,
    copy	= 0x12,
    clear	= 0x08 };

/**
 * a struct to describe a launchpad event - which can be either the pressing or
 * releasing of a key.
 */
struct event {
    int row; // from -1 to +7
    int col; // from 0 to +8
    int press; // 0 or 1
};

/**
 * the principal struct to handle the launchpad
 */
struct launchpad {
    struct libusb_device_handle* device;	//! usb device
    unsigned char* rdata;			//! buffer to store received data
    int rdata_max;				//! rdata's size
    unsigned char* tdata;			//! buffer to store data to transmit
    int tdata_max;				//! tdata's size
    
    unsigned int prefix;			//! current prefix byte
    int received;
    int parse_at;
    struct event* event;
    
    enum buffer displaying;			//! the buffer currently displayed
    enum buffer updating;			//! the buffer currently updated
    enum bool flashing;				//! whether we are flashing or not
};

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
 * launchpad. Received data is stored inside lp->rdata, and the amount of data
 * received is returned by the function.
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
 * encode a led state into a velocity value
 */
unsigned int lp_velocity(enum red red, enum green green, enum led_mode mode);

/**
 * turn on all the leds
 * \param intensity the leds' intensity, between 1 and 3
 */
int lp_allon(struct launchpad* lp, unsigned int intensity);

/**
 * reset the launchpad
 */
int lp_reset(struct launchpad* lp);

/**
 * set one led
 */
int lp_led(struct launchpad* lp, int row, int col, unsigned int velocity);

/** set the buffer mode
 *
 * \param displaying the buffer to display
 * \param updating the buffer to update
 * \param flashing whether the launchpad should display both buffers alternatively
 * \param copy whether we should copy the content of the newly displaying buffer into the newly updating buffer
 */
int lp_setmode(struct launchpad* lp, enum buffer displaying, enum buffer updating, enum bool flashing, enum bool copy);

/** standard mode
 *
 * displaying and writing on the same buffer
 * \param buf the buffer used
 */int lp_normal_mode(struct launchpad* lp, enum buffer buf);

/** 
 * displaying one buffer and writing on the other. if it is already the case,
 * the two buffers are swapped.
 * \param copy whether we should copy the content of the newly displaying buffer into the newly updating buffer
 */
int lp_doublebuffer_mode(struct launchpad* lp, enum bool copy);

/**
 * set or unset the flashing mode
 */
int lp_flashing_mode(struct launchpad* lp, enum bool flashing);
