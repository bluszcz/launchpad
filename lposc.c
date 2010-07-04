#include "lposc.h"

// globals
enum prefix prefix;
int port;
lo_address destination;
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

int main(int argc, char* argv[])
{
    int o;
    int port = 0;
    char* dest = NULL;
    
    // parsing parameters
    while ((o = getopt(argc,argv,"p:d:")) != -1) {
	switch (o) {
	case 'p':
	    port = atoi(optarg);
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
    
    destination = lo_address_new_from_url(dest);
    prefix = note;
    
    lp = lp_register();
    
    lp2osc();
    
    lp_deregister(lp);
}
