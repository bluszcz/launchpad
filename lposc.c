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

#include "lposc.h"

struct launchpad *lp;
char *port = NULL;
lo_server osc;
lo_address dest;

void error_handler(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
    fflush(stdout);
}

int generic_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data)
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

int matrix_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data)
{
	int row = argv[0]->i;
	int col = argv[1]->i;
	int vel = argv[2]->i;
	return lp_matrix(lp,row,col,vel);
}

int scene_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data)
{
	int row = argv[0]->i;
	int vel = argv[1]->i;
	return lp_scene(lp,row,vel);	
}

int ctrl_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data)
{
	int col = argv[0]->i;
	int vel = argv[1]->i;
	return lp_ctrl(lp,col,vel);		
}

int reset_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data)
{
	lp_reset(lp);
	return 0;
}

int dest_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data)
{
	if (dest != NULL) free(dest);
	dest = lo_address_new_from_url((char*) argv[0]);
	return 0;
}

void* lp2osc()
{
	int row,col,press;
	
    while (1) {
		lp_receive(lp);
		
		if (dest != NULL) {
			if (lp->event[0] == NOTE) {
				// matrix or scene
				row = lp->event[1] / 16;
				col = lp->event[1] % 16;
				press = lp->event[2];
				
				if (col == 8) {
					// scene event
					lo_send(dest, "/lp/scene", "ii", row, press);
				} else {
					// matrix event
					lo_send(dest, "/lp/matrix", "iii", row, col, press);
				}
			} else {
				// ctrl event
				col = lp->event[1] - 104;
				press = lp->event[2];
				lo_send(dest, "/lp/ctrl", "ii", col, press);
			}
		}
	}
}

void* osc2lp()
{
	// register methods
	lo_server_add_method(osc, NULL, NULL, generic_handler, NULL);
	lo_server_add_method(osc, "/lp/matrix", "iii", matrix_handler, NULL);
	lo_server_add_method(osc, "/lp/scene", "ii", scene_handler, NULL);
	lo_server_add_method(osc, "/lp/ctrl", "ii", ctrl_handler, NULL);
	lo_server_add_method(osc, "/lp/reset", "", reset_handler, NULL);
	lo_server_add_method(osc, "/lp/dest", "s", dest_handler, NULL);
	
	while (true) {
		fflush(stdout);
		lo_server_recv(osc);
	}
}

int main(unsigned int argc, char* argv[])
{
	int err;
	pthread_t lp2osc_thread, osc2lp_thread;
	
    // Launchpad initialization
    lp = lp_register();
	
	// OSC initialization
	osc = lo_server_new(port, error_handler);
	printf("port: %d\n", lo_server_get_port(osc));
	fflush(stdout);
		
	err = pthread_create(&lp2osc_thread, NULL, lp2osc, NULL);
	if (err){
		fprintf(stderr, "failed to start lp2osc thread, with error %d", err);
		return 0;
	}

	err = pthread_create(&osc2lp_thread, NULL, osc2lp, NULL);
	if (err){
		fprintf(stderr, "failed to start osc2lp thread, with error %d", err);
		return 0;
	}
	
    pthread_join(lp2osc_thread, NULL);
    pthread_join(osc2lp_thread, NULL);
	
	lp_deregister(lp);
	lo_server_free(osc);
	
    return 0;
}
