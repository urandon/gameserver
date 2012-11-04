#ifndef GAME_H_
#define GAME_H_

struct game_instance{
	int id;

	int resource;
	int production;
	int factory;
	long int cash;
};

typedef struct game_instance *p_game_instance;

p_game_instance new_game_instance(int id);
int game_kick(int id);
/**
 * TODO:: interface:
 * void destruct_game_instance(int id);
 * int game_start(...);
 * int game_state_update();
 * int game_command(int id, char * command);
 */

#endif
