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
#include <stdlib.h>
#include <unistd.h>
#include <lo/lo.h>

#include "liblaunchpad.h"

struct launchpad *lp;
int done = 0;
enum buffer displaying = buffer0;
enum buffer updating = buffer0;
enum bool flashing = false;

void error(int num, const char *m, const char *path);

int generic_handler(const char *path, const char *types, lo_arg **argv,
					int argc, void *data, void *user_data);

int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
				 void *data, void *user_data);

int reset_handler(const char *path, const char *types, lo_arg **argv, int argc,
				  void *data, void *user_data);

int led_handler(const char *path, const char *types, lo_arg **argv, int argc,
				void *data, void *user_data);

int mode_handler(const char *path, const char *types, lo_arg **argv, int argc,
				void *data, void *user_data);

int main()
{
    // Launchpad initialization
    lp = lp_register();
    
    /* start a new server on port 7770 */
    lo_server_thread st = lo_server_thread_new("7770", error);
    
    /* add method that will match any path and args */
    lo_server_thread_add_method(st, NULL, NULL, generic_handler, NULL);
    
    /* add method that will match the path /quit with no args */
    lo_server_thread_add_method(st, "/quit", "", quit_handler, NULL);
    lo_server_thread_add_method(st, "/reset", "", reset_handler, NULL);
    
    lo_server_thread_add_method(st, "/led", "ii", led_handler, NULL);	
    lo_server_thread_add_method(st, "/led", "iii", led_handler, NULL);	
    
    lo_server_thread_add_method(st, "/mode", "iiii", mode_handler, NULL);
    
    lo_server_thread_start(st);
    
    while (!done) {
	usleep(1000);
    }
    
    return 0;
}

void error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
    fflush(stdout);
}

/* catch any incoming messages and display them. returning 1 means that the
 * message has not been fully handled and the server should try other methods */
int generic_handler(const char *path, const char *types, lo_arg **argv,
					int argc, void *data, void *user_data)
{
    int i;

    printf("path: <%s>\n", path);
    for (i=0; i<argc; i++) {
	printf("arg %d '%c' ", i, types[i]);
	lo_arg_pp(types[i], argv[i]);
	printf("\n");
    }
    printf("\n");
    fflush(stdout);
	
    return 1;
}

int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
				 void *data, void *user_data)
{
    done = 1;
    printf("quiting\n\n");
    fflush(stdout);

    return 0;
}

int reset_handler(const char *path, const char *types, lo_arg **argv, int argc,
				 void *data, void *user_data)
{
	lp_reset(lp);
	return 0;
}

int led_handler(const char *path, const char *types, lo_arg **argv, int argc,
				void *data, void *user_data)
{
    int row, col,velocity;
    row			= argv[0]->i;
    col			= argv[1]->i;
	
	if (strcmp(types,"ii") == 0) {
		velocity = red_full + green_full + led_copy;
		lp_led(lp,row,col,velocity);
		return 0;
	} else if(strcmp(types,"iii") == 0) {
		velocity = argv[2]->i;
		lp_led(lp,row,col,velocity);
		return 0;
	}
}

int led_handler(const char *path, const char *types, lo_arg **argv, int argc,
				void *data, void *user_data)
{
    
    lp_setmode(lp, displaying, updating, flashing, copy);
    return 0;
}
