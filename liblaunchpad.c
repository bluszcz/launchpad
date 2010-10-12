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
    lp->rdata = malloc(sizeof(unsigned char)*MAX_PACKET_SIZE);
    
    if (lp->rdata == NULL) {
	fprintf(stderr,"could not allocate input buffer\n");
	return NULL;
    }
    
    //allocate output buffer
    lp->tdata = malloc(sizeof(unsigned char)*MAX_PACKET_SIZE);
  
    if (lp->tdata == NULL) {
	fprintf(stderr,"could not allocate output buffer\n");
	return NULL;
    }
        
    // initialize the protocol's state
    lp->event[0] = NOTE;
    lp->parse_at = 0;
    lp->received = 0;

    // reset launchpad
    lp_reset(lp);
    
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
				  MAX_PACKET_SIZE,
				  &lp->received,
				  0);
    }

    // check if the first byte is a prefix byte
    if (lp->rdata[lp->parse_at] == NOTE || lp->rdata[lp->parse_at] == CTRL) {
		lp->event[0] = lp->rdata[lp->parse_at++];
    }
	lp->event[1] = lp->rdata[lp->parse_at++];
	lp->event[2] = lp->rdata[lp->parse_at++];
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

int lp_check(struct launchpad* lp, int intensity)
{
    if (intensity > 2) {
	intensity = 2;
    }
    
    if (intensity < 1) {
	intensity = 1;
    }
    
    return lp_send3(lp,CTRL,0,124+intensity);
}

int lp_reset(struct launchpad* lp)
{
    return lp_send3(lp,CTRL,0,0);
}

int lp_setmode(struct launchpad* lp, enum buffer displaying, enum buffer updating, enum bool flashing, enum bool copy)
{
    int v = 32;
    // displaying buffer
    v += displaying;
    // updating buffer
    v += updating * 4;
    // flashing
    v += flashing * 8;
    // copying the new displaying buffer to the new updating buffer
    v += copy * 16;
    
    return lp_send3(lp,CTRL,0,v);
}

int lp_matrix(struct launchpad *lp, int row, int col, int velocity)
{
	if (row < 0 || row > 7 || col < 0 || col > 7)
		return 0;
	
	return lp_send3(lp, NOTE, row*16 + col, velocity);
}

int lp_scene(struct launchpad *lp, int row, int velocity)
{
	if (row < 0 || row > 7)
		return 0;
	
	return lp_send3(lp, NOTE, row*16 + 8, velocity);
}

int lp_ctrl(struct launchpad *lp, int col, int velocity)
{
	if (col < 0 || col > 7)
		return 0;
	
	return lp_send3(lp, CTRL, 104+col, velocity);
}
