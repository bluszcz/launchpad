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

#include "liblaunchpad.h"
#include <unistd.h>

struct launchpad* lp_register()
{
    struct launchpad *lp;
    int bufferlen;
    
    //build the struct
    lp = malloc(sizeof(struct launchpad));
    if (lp == NULL) {
	fprintf(stderr,"Unable to allocate memory\n");
	return NULL;
    }
    
    //initialize usb
    if(libusb_init(NULL)!=0){
	fprintf(stderr,"Unable to initialize usb\n");
	return NULL;
    } else {
        printf("usb initialized\n");
    }
    
    //find the device
    lp->device = libusb_open_device_with_vid_pid(NULL, ID_VENDOR, ID_PRODUCT);
    if (lp->device == NULL) {
	fprintf(stderr,"Unable to find the launchpad\n");
 	return NULL;
    } else {
        printf("launchpad found\n");
    }
    
    //claim the device
    if(libusb_claim_interface(lp->device, 0) != 0) {
	fprintf(stderr,"Unable to claim the launchpad\n");
	return NULL;
    } else {
	printf("launchpad claimed\n");
    }
  
    //allocate input buffer
    bufferlen = libusb_get_max_packet_size(libusb_get_device(lp->device), EP_IN);
    lp->rdata = malloc(sizeof(unsigned char)*bufferlen);
    lp->rdata_max = bufferlen;
    
    if (lp->rdata == NULL) {
	fprintf(stderr,"could not allocate input buffer\n");
	return NULL;
    }
    
    //allocate output buffer
    bufferlen = libusb_get_max_packet_size(libusb_get_device(lp->device), EP_OUT);
    lp->tdata = malloc(sizeof(unsigned char)*bufferlen);
    lp->tdata_max = bufferlen;
  
    if (lp->tdata == NULL) {
	fprintf(stderr,"could not allocate output buffer\n");
	return NULL;
    }

    lp->event = malloc(sizeof(struct event));
    if (lp->event == NULL) {
	fprintf(stderr,"could not allocate event\n");
	return NULL;
    }
    
    printf("buffers allocated\n");
    
    lp_reset(lp);
    lp_normal_mode(lp,buffer0);
    lp->prefix = NOTE;
    return lp;
}

void lp_deregister(struct launchpad *lp)
{
    //declaim the device
    libusb_release_interface(lp->device,0);
    
    //close the device
    libusb_close(lp->device);
    
    //free allocated memory
    free(lp->rdata);
    free(lp->tdata);
    free(lp->event);
    
    //close usb
    libusb_exit(NULL);    
}

void lp_receive(struct launchpad* lp)
{
    if (lp->parse_at == lp->received) {
	// there is no data to parse. wait for some
	lp->parse_at = 0;
	lp->received = 0;
	libusb_interrupt_transfer(lp->device,
				  EP_IN,
				  lp->rdata,
				  lp->rdata_max,
				  &lp->received,
				  0);
    }
    
    // check if the first byte is a prefix byte
    if (lp->rdata[lp->parse_at] == NOTE || lp->rdata[lp->parse_at] == CTRL) {
	lp->prefix = lp->rdata[lp->parse_at];
	lp->parse_at++;
    }
    
    // first byte: which key is pressed
    switch(lp->prefix) {
    case NOTE:
	lp->event->row = lp->rdata[lp->parse_at] / 16;
	lp->event->col = lp->rdata[lp->parse_at] % 16;
	break;
    case CTRL:
	lp->event->row = -1;
	lp->event->col = lp->rdata[lp->parse_at] -104;
	break;
    }
    lp->parse_at++;
    
    // second byte: press or release
    lp->event->press = (lp->rdata[lp->parse_at] != 0);
    lp->parse_at++;
}

int lp_send(struct launchpad* lp, int size)
{
    int transmitted;
    
    // send the data
    libusb_interrupt_transfer(lp->device,
			      EP_OUT,
			      lp->tdata,
			      size,
			      &transmitted,
			      0);
    
    return transmitted;
}

int lp_send3(struct launchpad* lp, unsigned int data0, unsigned int data1, unsigned int data2)
{
    lp->tdata[0] = data0;
    lp->tdata[1] = data1;
    lp->tdata[2] = data2;
    return lp_send(lp,3);
}

unsigned int lp_velocity(enum red red, enum green green, enum led_mode mode)
{
    return red + green + mode;
}

int lp_reset(struct launchpad* lp)
{
    return lp_send3(lp,CTRL,0,0);
}

int lp_allon(struct launchpad* lp, unsigned int intensity)
{
    if (intensity > 2) {
	intensity = 2;
    }
    return lp_send3(lp,CTRL,0,125+intensity);
}

int lp_led(struct launchpad* lp, int row, int col, unsigned int velocity)
{
    if (row == -1) {
	// automap button
	return lp_send3(lp, CTRL, 104+col, velocity);
    }
    
    // grid or scene button
    return lp_send3(lp, NOTE, row * 16 + col, velocity);
}

int lp_setmode(struct launchpad* lp, enum buffer displaying, enum buffer updating, enum bool flashing, enum bool copy)
{
    lp->displaying = displaying;
    lp->updating = updating;
    lp->flashing = flashing;
    
    int v = 32;
    // displaying buffer
    v += lp->displaying;
    // updating buffer
    v += lp->updating * 4;
    // flashing
    v += lp->flashing * 8;
    // copying the new displaying buffer to the new updating buffer
    v += copy * 16;
    
    return lp_send3(lp,CTRL,0,v);
}

int lp_normal_mode(struct launchpad* lp, enum buffer buf)
{
    return lp_setmode(lp, buf, buf, false, false);
}

int lp_doublebuffer_mode(struct launchpad* lp, enum bool copy)
{
    if (lp->displaying == lp->updating) {
	// entering the doublebuffer mode
	switch (lp->displaying) {
	case buffer0:
	    return lp_setmode(lp, buffer0, buffer1, false, copy);
	case buffer1:
	    return lp_setmode(lp, buffer1, buffer0, false, copy);
	}
    } else {
	return lp_setmode(lp, lp->updating, lp->displaying, false, copy);
    }
}

int lp_flashing_mode(struct launchpad* lp, enum bool flashing)
{
    return lp_setmode(lp, lp->displaying, lp->updating, flashing, false);
}
