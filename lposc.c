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
#include <lo/lo.h>

// globals
char* port = NULL;
lo_address destination;
lo_server_thread server;
struct launchpad* lp;

void lp2osc()
{
    int recv,i;
    
    while (1) {
	lp_receive(lp);
	printf("%d %d %d\n",
	       lp->event->row,
	       lp->event->col,
	       lp->event->press);
	
	lo_send(destination, "/lp/press", "iii",
		lp->event->row,
		lp->event->col,
		lp->event->press);
    }
}

void error_handler(int num, const char *msg, const char *where)
{
    fprintf(stderr,"OSC error #%d: %s @ %s\n", num, msg, where);
}

int press_handler(const char *path, const char *types, lo_arg **argv, int argc, lo_message  msg, void *user_data)
{
    int row,col,press;
    row = argv[0]->i;
    col = argv[1]->i;
    press = argv[2]->i;
    
    return 0;
}

void osc2lp()
{
    
}

int main(int argc, char* argv[])
{
    int o;
    char* dest = NULL;
    
    // parsing parameters
    while ((o = getopt(argc,argv,"p:d:")) != -1) {
	switch (o) {
	case 'p':
	    port = optarg;
	    break;
	case 'd':
	    dest = optarg;
	    break;
	case '?':
	    fprintf(stderr, "usage: lposc -p <listening port> -d <destination>\n");
	    return 0;
	    break;
	}
    }
    
    if (dest == NULL) {
	fprintf(stderr, "destination is mandatory\n");
    }
    
    // Launchpad initialization
    lp = lp_register();
    
    // OSC initialization
    destination = lo_address_new_from_url(dest);
    server_thread = lo_server_thread_new(port,error_handler);
    lo_server_add_method(server_thread, "/lp/press","iii",press_handler,NULL);
    
    lp2osc();
    
    lp_deregister(lp);
}
