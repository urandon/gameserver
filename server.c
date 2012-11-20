#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "player.h"
#include "settings.h"
#include "game.h"
#include "server.h"


extern struct globalArgs_t globalArgs;

static int listener;
static p_players_storage storage = NULL;

int build_active_set(fd_set * set)
{
	int max_d = STDIN_FILENO;
	int id, desc;

	VRB(3, printf("server::build_active_set()\n"));

	FD_ZERO(set);

	FD_SET(listener, set);
	if(listener > max_d){
		max_d = listener;
	}

	FD_SET(STDIN_FILENO, set);
	for(id = 0; id < storage->capacity; id++){
		desc = storage->descs[id];
		if(desc != -1){
			FD_SET(desc, set);
			if(desc > max_d){
				max_d = desc;
			}
		}
	}

	return max_d;
}

/* returns -1 on error */
int send_dirrect_buff(int descriptor, const char * buffer, int length)
{
	int cnt = 0;
	int res;

	VRB(4, printf("server::send_dirrect_buff(desc = %d)\n", descriptor));

	while(cnt < length){
		res = write(descriptor, buffer+cnt, length-cnt);
		if(res == -1){
			return -1;
		}
		cnt += res;
	}
	return 0;
}

void send_dirrect(int id, const char * message)
{
	VRB(3, printf("server::send_dirrect(id = %d, %s)\n", id, message));
	if( send_dirrect_buff(id2desc(storage, id), message, strlen(message)) == -1){
		/* kick him */
	}
}

void send_broadcast(const char * message)
{
	int id, desc;
	int length = strlen(message)+1;

	VRB(3, printf("server::send_broadcast(%s)\n", message));
	printf("%s", message);

	for(id = 0; id < storage->capacity; id++){
		if( (desc = id2desc(storage, id)) != -1){
			VRB(3, printf("server::send_broadcast() -> [%d]\n", id));
			if(send_dirrect_buff(desc, message, length) == -1){
				/* Exception */
				/* kill the player ha-ha-ha!!! */
				/* heh, it was just a joke */
			}
		}
	}
}

void kick_player(int id, char * description)
{
	char * s;
	int desc;
	p_player p;

	VRB(3, printf("server::kick_player(id = %d, %s)\n", id, description));

	desc = id2desc(storage, id);
	p = get_player(storage, id);

	if(p != NULL){
		s = (char *) malloc ((strlen(description) + strlen(p->name) + 64) * sizeof(*s));
		sprintf(s, "Player %s[%d] has been kicked because of %s\n", \
			p->name, id, description);
		game_kick(id);
		remove_player(storage, id);
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

	VRB(3, printf("server::start_listen()\n"));

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

	printf("The Game has been stated at port %d\n", globalArgs.listener_port);
}


void wait4connections()
{
	p_player p;
	fd_set rdfs;
	const int buffer_size = 64;
	char * buffer, message[64], *command;
	int n = globalArgs.players, max_d;
	int length, message_len = 0;
	int id, desc;
	int connected = 0;

	VRB(3, printf("server::wait4connections()\n"));

	buffer = (char *) malloc (buffer_size * sizeof(char));
	strcpy(message, "Admin, stop playing with console. Don't be like a schoolar.\n");

	while(globalArgs.connected_players < n){
		max_d = build_active_set(&rdfs);
		if(connected != globalArgs.connected_players){
			message_len = sprintf(message, \
				"Waiting for users: %d of %d connected\n", \
				globalArgs.connected_players, n);
			connected = globalArgs.connected_players;
		}

		select(max_d+1, &rdfs, NULL, NULL, NULL);

		if(FD_ISSET(STDIN_FILENO, &rdfs)){
			if(read(STDIN_FILENO, buffer, buffer_size) == 0){
				printf("Use SIGINT to kill me, luke\n");
			}
			printf("%s", message);
			printf("Connected %d users\n", globalArgs.connected_players);
		}
		if(FD_ISSET(listener, &rdfs)){
			desc = accept(listener, NULL, NULL);
			if(desc == -1){
				perror("accept(listener)");
			} else {
				new_player(storage, desc);
				p = get_player_by_desc(storage, desc);
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
		for(id = 0; id < storage->capacity; id++){
			desc = id2desc(storage, id);
			if(desc != -1 && FD_ISSET(desc, &rdfs)){
				/* may be listen to the player */
				length = read(desc, buffer, buffer_size);
				if(length == 0){
					kick_player(id, \
						"connection error (or user logged out)");
				} else
				if(length == -1){
					perror("read");
				} else
				if(push_buffer(storage, id, buffer, length) == -1){
					kick_player(id, "we don't like flooders");
					send_broadcast(message);
				} else {
					/* say him that the game hasn't been started */
					while((command = get_message(storage, id)) != NULL){
						send_dirrect_buff(desc, message, message_len);
						free(command);
					}
				}
			}
		}

	}
	free(buffer);
}


void game_process()
{
	fd_set rdfs;
	const int buffer_size = 64;
	char * buffer, message[64], *command;
	int n = globalArgs.players, max_d;
	int length, message_len = 0;
	int id, desc;
	int connected = 0;

	VRB(3, printf("server::game_process()\n"));

	buffer = (char *) malloc (buffer_size * sizeof(char));
	strcpy(message, "Admin, stop playing with console. Don't be like a schoolar.\n");

	do{
		max_d = build_active_set(&rdfs);
		if(connected != globalArgs.connected_players){
			message_len = sprintf(message, \
				"Connected users: %d of %d connected\n", \
				globalArgs.connected_players, n);
			connected = globalArgs.connected_players;
		}

		select(max_d+1, &rdfs, NULL, NULL, NULL);

		if(FD_ISSET(STDIN_FILENO, &rdfs)){
			if(read(STDIN_FILENO, buffer, buffer_size) == 0){
				printf("Use SIGINT to kill me, luke\n");
			}
			printf("%s", message);
			printf("Connected %d users\n", globalArgs.connected_players);
		}
		if(FD_ISSET(listener, &rdfs)){
			desc = accept(listener, NULL, NULL);
			if(desc == -1){
				perror("accept(listener)");
			} else {
				send_dirrect(desc, "Connection rejected. The game have been stated\n");
				shutdown(desc, SHUT_RDWR);
				close(desc);
			}
		}
		for(id = 0; id < storage->capacity; id++){
			desc = id2desc(storage, id);
			if(desc != -1 && FD_ISSET(desc, &rdfs)){
				/* may be listen to the player */
				length = read(desc, buffer, buffer_size);
				if(length == 0){
					kick_player(id, \
						"connection error (or user logged out)");
				} else
				if(length == -1){
					perror("read");
				} else
				if(push_buffer(storage, id, buffer, length) == -1){
					kick_player(id, "we don't like flooders");
					send_broadcast(message);
				} else {
					while((command = get_message(storage, id)) != NULL){
						game_command(id, command);
						free(command);
						if(id2desc(storage, id) != -1){
							send_dirrect(id, ">> ");
						}
					}
				}
			}
		}

	} while (game_state_update() != -1);
	free(buffer);
}


/* void game_process() */


int main(int argc, char ** argv)
{
	parse_args(argc, argv);
	VRB(1, printf("Verbosiblity level = %d\n", globalArgs.verbose));
	start_listen(argv[0]);
	storage = new_storage(globalArgs.players);
	wait4connections();
	send_broadcast("The Game is started!\n>>\n");
	game_start(globalArgs.players);
	game_process();
	send_broadcast("The Game is over!\n");
	game_finish();
	destroy_storage(storage);

	return 0;
}
