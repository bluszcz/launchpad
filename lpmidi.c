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
#include <alsa/asoundlib.h>
#include <pthread.h>

// globals
struct launchpad* lp;
snd_seq_t* midi_client;
int midi_in;
int midi_out;

void midi_register()
{
    //open a new alsa midi client
    if (snd_seq_open(&midi_client, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
	fprintf(stderr,"coud not open a new midi client\n");
	return;
    }
    
    //set the client's name
    snd_seq_set_client_name(midi_client, "Launchpad");
    
    //open a new midi output port
    midi_out = snd_seq_create_simple_port(midi_client, "Out",
					  SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
					  SND_SEQ_PORT_TYPE_PORT);
    if (midi_out < 0) {
	fprintf(stderr,"could not open the midi out port\n");
	return;
    }
  
    midi_in = snd_seq_create_simple_port(midi_client, "In",
					 SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
					 SND_SEQ_PORT_TYPE_PORT);
    if (midi_in < 0) {
	fprintf(stderr,"could not open the midi in port\n");
	return;
    }    
}

void midi_deregister()
{
    snd_seq_delete_simple_port(midi_client, midi_out);
    snd_seq_delete_simple_port(midi_client, midi_in);
    snd_seq_close(midi_client);
}

void* lp2midi(void* nothing)
{
    printf("waiting for launchpad events\n");
    
    int recv,i;
    snd_seq_event_t ev;
    
    while (1) {
	lp_receive(lp);	
	snd_seq_ev_clear(&ev);			// clean the event to send
	snd_seq_ev_set_source(&ev, midi_out);	// set the output port number
	snd_seq_ev_set_subs(&ev);		// broadcast to subscribers
	snd_seq_ev_set_direct(&ev);		// send now, don't put in queue
	
	if (lp->event->row < 0) {
	    // ctrl event
	    snd_seq_ev_set_controller(&ev, 1, lp->event->col +104, lp->event->press *127);
	} else {
	    // note event
	    if (lp->event->press) {
		snd_seq_ev_set_noteon(&ev, 1, lp->event->row *16 +lp->event->col, 127);
	    } else {
		snd_seq_ev_set_noteoff(&ev, 1, lp->event->row *16 +lp->event->col, 0);
	    }
	}
	
	// send the message
	snd_seq_event_output(midi_client, &ev);
	snd_seq_drain_output(midi_client);
    }
}

void* midi2lp(void* nothing)
{
    printf("waiting for midi events\n");
    snd_seq_event_t *ev;
    while (1) {
	
	// get a new event. if there aren't any, wait for one
	snd_seq_event_input(midi_client, &ev);
	
	// normal mode
	if(ev->data.note.channel == 1) {
	    
	    
	    // copy the data to send
	    switch (ev->type) {
		
	    case SND_SEQ_EVENT_NOTEON:
		lp_send3(lp, NOTE_ON, ev->data.note.note, ev->data.note.velocity);
		break;
		
	    case SND_SEQ_EVENT_NOTEOFF:
		lp_send3(lp, NOTE_OFF, ev->data.note.note, ev->data.note.velocity);
		break;
		
	    case SND_SEQ_EVENT_CONTROLLER:
		lp_send3(lp, CTRL, ev->data.control.param, ev->data.control.value);
		break;
	    }
	    
	    // free the midi event
	    snd_seq_free_event(ev);
	}
    }
}

int main(int argc, char* argv[])
{
    int err;
    pthread_t lp2midi_thread, midi2lp_thread;
    
    lp = lp_register();
    midi_register();
    
    // start listening to the launchpad
    err = pthread_create(&lp2midi_thread, NULL, lp2midi, NULL);
    if (err){
	fprintf(stderr, "failed to start usb thread with error %d", err);
	return 0;
    }
    
    // start listening to the midi port
    err = pthread_create(&midi2lp_thread, NULL, midi2lp, NULL);
    if (err){
	fprintf(stderr, "failed to start midi thread with error %d", err);
	return 0;
    }
    
    // wait for the threads to finish
    pthread_join(lp2midi_thread, NULL);
    pthread_join(midi2lp_thread, NULL);
    
    midi_deregister();
    lp_deregister(lp);
}
