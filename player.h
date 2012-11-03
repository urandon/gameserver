#ifndef PLAYER_H
#define PLAYER_H

/**
 * В данном модуле описывается игрок как сетевая сущность,
 * которой не чуждо иметь своё имя, сокет, буфер, игровую сущность.
 * Игровая сущность отделена от сетевой для обеспечения заменяемости
 * игрового движка.
 */


#include "game.h"

/*=================== single player manipulation =====================*/

struct message_list{
	char * message;
	struct message_list *next, *prev;
};

struct message_queue{
	struct message_list *lo, *hi;
	int count;
};

struct player{
	int id;
	int descriptor;
	
	char * name;
	
	struct message_queue * messages;
	char * buffer;
	int buffer_allocated, buffer_size;
	
	p_game_instance data;
};

typedef struct player  *p_player;

struct players_storage{
	p_player node;
	struct players_storage * next, * prev;
};

typedef struct players_storage  *p_players_storage;


p_player new_player(int descriptor);
void destroy_player(p_player p);

void change_name(p_player p, char * new_name);

p_player get_player(p_players_storage storage, int id);
p_player get_player_by_desc(p_players_storage storage, int descriptor);
p_players_storage add_player(p_players_storage storage, p_player p);
p_players_storage remove_player(p_players_storage storage, int descriptor);

/*==================== buffer manipulation ===========================*/

/* returns the number of new incomming messages
 * or -1 if the player is flooder */
int push_buffer(p_player p, char * buffer, int buffer_len);
char * get_message(p_player p);

/*==================== multi player manipulation =====================*/

void set_num_of_players(int n);
int id2desc(int id);
int desc2id(int descriptor);

void destroy_storage(p_players_storage storage);

#endif