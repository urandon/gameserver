#ifndef SETTINGS_H
#define SETTINGS_H

struct globalArgs_t {
	int players;
	int connected_players;
	int listener_port;
	long kick_buffer_limit;
	int verbose;
};

void parse_args(int argc, char ** argv);

#endif
