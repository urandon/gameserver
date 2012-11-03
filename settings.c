#include <unistd.h>
#include <sys/types.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "settings.h"

#define MAX_PLAYERS 50

struct globalArgs_t globalArgs;

static const char * optString = "b:n:p:h?v";

static const struct option longOpts[] = {
	{ "buffer-limit", required_argument, NULL, 'b' },
	{ "players", required_argument, NULL, 'n' },
	{ "port", required_argument, NULL, 'p' },
	{ "help", no_argument, NULL, 'h' },
	{ "verbose", no_argument, NULL, 'v' },
	{NULL, no_argument, NULL, 0}
};

void display_usage(char * argv0)
{
	const char * descript[] = {
		"Set the string buffer limit to prevent flood atack (default 1Mb)",
		"Set the number of players",
		"Set the game port",
		"Display this message",
		"Show more information during the work"
	};
	const char * params[] = {
		"LIMIT_IN_BYTES",
		"NUM_OF_PLAYERS",
		"PORT",
		NULL,
		NULL,
	};
	struct option *opt;
	int i, n = sizeof(longOpts) / sizeof(struct option) - 1;
		
	printf("Usage: %s [options]\n", argv0);
	printf("Options:\n");
	for(i = 0; i < n; i++){
		opt = (struct option *)longOpts + i;
		switch(opt->has_arg){
			case required_argument :
				printf("  -%c, --%s=%s\t%s\n", \
					opt->val, opt->name, params[i], descript[i]);
				break;
			case optional_argument :
				printf("  -%c, --%s[=%s]\t%s\n", \
					opt->val, opt->name, params[i], descript[i]);
				break;
			case no_argument :
				printf("  -%c, --%s\t%s\n", \
					opt->val, opt->name, descript[i]);
				break;
			default:
				break;
		}
	}
}

void parse_args(int argc, char ** argv)
{
	int longIndex, opt;
	int tmp;
	
	globalArgs.kick_buffer_limit = 1048576;
	globalArgs.listener_port = 1984;
	globalArgs.players = 5;
	globalArgs.connected_players = 0;
	globalArgs.verbose = 0;
	
	while( (opt = getopt_long( argc, argv, optString, longOpts, &longIndex )) != -1){
		switch(opt){
			case 'n':
				tmp = atoi(optarg);
				if(tmp <= 0 || tmp > MAX_PLAYERS){
					fprintf(stderr, "Number of players must be between 0 and %d", MAX_PLAYERS);
					exit(EXIT_FAILURE);
				}
				globalArgs.players = tmp;
				break;
			case 'p':
				globalArgs.listener_port = atoi(optarg);
				break;
			case 'h': case '?':
				display_usage(argv[0]);
				exit(EXIT_SUCCESS);
				break;
			case 'v':
				globalArgs.verbose++;
				break;
			default:
				break;
		}
	}
}

