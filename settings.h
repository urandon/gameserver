#ifndef SETTINGS_H_
#define SETTINGS_H_

#ifdef DEBUG
#include <stdio.h>
#define VRB(V,A) if(globalArgs.verbose >= (V)){ A; }
#define DBG ;fprintf(stderr, "google.com\n");
#else
#define VRB(V,A)
#define DBG
#endif


struct globalArgs_t {
	int players;
	int connected_players;
	int listener_port;
	long kick_buffer_limit;
	int verbose;
};

void parse_args(int argc, char ** argv);

#endif
