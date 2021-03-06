#ifndef PLAYER_H_
#define PLAYER_H_

/**
 * В данном модуле описывается игрок как сетевая сущность,
 * которой не чуждо иметь своё имя, сокет, буфер, игровую сущность.
 * Игровая сущность отделена от сетевой для обеспечения заменяемости
 * игрового движка.
 */


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
};

typedef struct player  *p_player;


struct players_storage{
	p_player * players;
	int * descs;
	int capacity;
	int connected;
	int current_id;
};

typedef struct players_storage  *p_players_storage;


void change_name(p_player p, char * new_name);
p_players_storage new_storage(int capacity);
int new_player(p_players_storage storage, int descriptor);
p_player get_player(p_players_storage storage, int id);
p_player get_player_by_desc(p_players_storage storage, int descriptor);
void remove_player(p_players_storage storage, int id);

/*==================== buffer manipulation ===========================*/

/* returns the number of new incoming messages
 * or -1 if the player is flooder */
int push_buffer(p_players_storage storage, int id, char * buffer, int buffer_len);
char * get_message(p_players_storage storage, int id);

/*==================== multiplayer manipulation =====================*/

int id2desc(p_players_storage storage, int id);
int desc2id(p_players_storage storage,  int descriptor);

void destroy_storage(p_players_storage storage);

#endif
