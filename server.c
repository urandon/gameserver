#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "settings.h"
#include "player.h"
#include "game.h"
#include "server.h"

#define VRB(V,A) if(globalArgs.verbose >= (V)){ A; }
#define DBG ;fprintf(stderr, "google.com\n");

extern struct globalArgs_t globalArgs;

static int listener;
static p_players_storage storage = NULL;

int build_active_set(fd_set * set)
{
	p_players_storage p;
	int max_d = STDIN_FILENO;
	int desc;
	
	FD_ZERO(set);
	
	FD_SET(listener, set);
	if(listener > max_d){
		max_d = listener;
	}
	
	FD_SET(STDIN_FILENO, set);
	for(p = storage; p != NULL; p = p->next){
		desc = p->node->descriptor;
		FD_SET(desc, set);
		if(desc > max_d){
			max_d = desc;
		}
	}
	
	return max_d;
}

/* returns -1 on error */
int send_dirrect_buff(int descriptor, const char * buffer, int length)
{
	int cnt = 0;
	int res;
	
	while(cnt < length){
		res = write(descriptor, buffer+cnt, length-cnt);
		if(res == -1){
			return -1;
		}
		cnt += res;
	}
	return 0;
}

void send_dirrect(int descriptor, const char * message)
{
	if( send_dirrect_buff(descriptor, message, strlen(message)) == -1){
		/* kick him */
	}
}

void send_broadcast(const char * message)
{	
	p_players_storage p = storage;
	int length = strlen(message)+1;
	
	printf("%s", message);
	for(p = storage; p != NULL; p = p->next){
		if(send_dirrect_buff(p->node->descriptor, message, length) == -1){
			/* exeption */
			/* kill the player ha-ha-ha!!! */
			/* heh, it was just a joke */
		}
	}
}

void kick_player(int id, char * description)
{
	char * s;
	int desc = id2desc(id);
	p_player p = get_player(storage, id);
	
	if(p != NULL){
		s = (char *) malloc ((strlen(description) + strlen(p->name) + 64) * sizeof(*s));
		sprintf(s, "Player %s[%d] has been kicked because of %s\n", \
			p->name, id, description);
		game_kick(id);
		storage = remove_player(storage, id);
		shutdown(desc, SHUT_RDWR);
		close(desc);
		/* tell everybody about that looser */
		send_broadcast(s);
		free(s); /* check for this place later */
		globalArgs.connected_players--;
	}
}

void start_listen(const char* argv0)
{
	const int max_listen_queue = 5;
	struct sockaddr_in addr;
	int optval = 1;
	
	if( (listener = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
		perror(argv0);
		exit(EXIT_FAILURE);
	}
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(globalArgs.listener_port);
	addr.sin_addr.s_addr = INADDR_ANY;
	
	setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	
	if(bind(listener,(struct sockaddr *) &addr, sizeof(addr)) == -1){
		perror(argv0);
		exit(EXIT_FAILURE);
	}
	
	if(listen(listener, max_listen_queue) == -1){
		perror(argv0);
		exit(EXIT_FAILURE);
	}
	
	fprintf(stderr, "The Game has been stated at port %d\n", \
			globalArgs.listener_port);
}


void wait4connections()
{
	p_players_storage s;
	p_player p;
	fd_set rdfs;
	const int buffer_size = 64;
	char * buffer, message[64];
	int n = globalArgs.players, max_d;
	int length, message_len = 0;
	int desc;
	
	buffer = (char *) malloc (buffer_size * sizeof(char));
	strcpy(message, "Admin, stop playing with console. Don't be like a schoolar.\n");
	
	while(globalArgs.connected_players < n){
		max_d = build_active_set(&rdfs);
		select(max_d+1, &rdfs, NULL, NULL, NULL);
		
		VRB(2, send_broadcast("Incomming data\n"));
		
		if(FD_ISSET(STDIN_FILENO, &rdfs)){
			read(STDIN_FILENO, buffer, buffer_size);
			printf("%s", message);
			printf("Connected %d users\n", globalArgs.connected_players);
		}
		if(FD_ISSET(listener, &rdfs)){
			desc = accept(listener, NULL, NULL);
			if(desc == -1){
				perror("accept(listener)");
			} else {
				storage = add_player(storage, p = new_player(desc));
				message_len = sprintf(message, \
					"New user %s[%d] has been connected\n", \
					p->name, p->id);
				send_broadcast(message);
				message_len = sprintf(message, \
					"Waiting for users: %d of %d connected\n", \
					++globalArgs.connected_players, n);
				send_broadcast(message);
			}
		}
		for(s = storage; s != NULL; s = s->next){
			desc = s->node->descriptor;
			if(FD_ISSET(s->node->descriptor, &rdfs)){
				/* may be listen to the player */
				length = read(desc, buffer, buffer_size);
				if(length == 0){
					kick_player(s->node->id, \
						"connection error (or user logged out)");
				} else
				if(length == -1){
					perror("read");
				} else
				if(push_buffer(s->node, buffer, length) == -1){
					kick_player(s->node->id, "we don't like flooders");
				} else {
					/* say him that the game hasn't been started */
					while(get_message(s->node) != NULL){
						send_dirrect_buff(desc, message, message_len);
					}
				}
			}
		}

	}
}



int main(int argc, char ** argv)
{	
	parse_args(argc, argv);
	VRB(1, printf("Verbosiblity level = %d\n", globalArgs.verbose));
	start_listen(argv[0]);
	wait4connections();
	send_broadcast("The Game is started");
	// main cycle
	
	
	for(;;){}
	
	return 0;
}