#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "player.h"
#include "settings.h"


#define DEFAULT_BUFFER 64

extern struct globalArgs_t globalArgs;


static char * default_name = "Anonymous";

/*===================== single player methods ========================*/

/*============ Constructor & desctructor */
p_players_storage new_storage(int capacity)
{
	p_players_storage storage;
	int i;

	storage = (p_players_storage) malloc (sizeof(*storage));
	storage->current_id = 0;
	storage->connected = 0;
	storage->capacity = capacity;
	storage->descs = (int *) malloc (capacity * sizeof(int));
	storage->players = (typeof(storage->players)) \
			malloc (capacity * sizeof(* storage->players));
	for(i = 0; i < capacity; i++){
		storage->descs[i] = -1;
		storage->players[i] = NULL;
	}

	return storage;
}

static int next_id(p_players_storage storage)
{
	if(storage->connected >= storage->capacity){
		return -1;
	}
	while(storage->descs[storage->current_id] != -1){
		if(storage->current_id >= globalArgs.players){
			storage->current_id = 0;
		} else {
			storage->current_id++;
		}
	}
	return storage->current_id;
}


/* returns -1 in case of error */
int new_player(p_players_storage storage, int descriptor)
{
	p_player p;

	VRB(4, printf("player::new_player(desc = %d)\n", descriptor));

	p = (p_player) malloc (sizeof(*p));

	if((p->id = next_id(storage)) == -1){
		free(p);
		return -1;
	}
	p->descriptor = descriptor;
	storage->descs[p->id] = p->descriptor;
	p->name = NULL;
	change_name(p, default_name);

	p->messages = (struct message_queue *) malloc (sizeof(*(p->messages)));
	p->messages->lo = NULL;
	p->messages->hi = NULL;
	p->messages->count = 0;
	p->buffer_allocated = DEFAULT_BUFFER;
	p->buffer_size = 0;
	p->buffer = (char *) malloc (p->buffer_allocated * sizeof(char));


	/* adding to storage */
	storage->connected++;
	storage->players[p->id] = p;
	storage->descs[p->id] = p->descriptor;

	return p->id;
}

void change_name(p_player p, char * new_name)
{
	char * s;

	VRB(4, printf("player::change_name(id = %d, %s)\n", p->id, new_name));

	s = (char *) malloc ((strlen(new_name) + 1) * sizeof(char));
	strcpy(s, new_name);
	free(p->name);
	p->name = s;
}


static void destroy_messages(p_player p)
{
	struct message_list *m, *n;

	VRB(4, printf("player::destroy_messages(id = %d)\n", p->id));

	if(p->messages != NULL){
		m = p->messages->hi;
		while(m != NULL){
			n = m->next;
			free(m->message);
			free(m);
			m = n;
		}
	}
}

static void destroy_player(p_player p)
{
	if(p != NULL){
		VRB(4, printf("player::destroy_player(id = %d)\n", p->id));

		free(p->name);
		free(p->buffer);
		destroy_messages(p);
		free(p->messages);
		free(p);
	}
}

/*============ search engine */

/* returns -1 in case if there are no player with this id */
static int look4player(p_players_storage storage, int id)
{
	VRB(4, printf("player::look4player(id = %d)\n", id));

	if(id >= storage->capacity || id < 0){
		return -1;
	}

	if(storage->players[id] == NULL){
		return -1;
	}

	return id;
}

p_player get_player(p_players_storage storage, int id)
{
	int pos;

	VRB(4, printf("player::get_player(id = %d)\n", id));
	if( (pos = look4player(storage, id)) != -1 ){
		return storage->players[pos];
	}

	return NULL;
}

p_player get_player_by_desc(p_players_storage storage, int descriptor)
{
	VRB(4, printf("player::get_player_by_desc(desc = %d)\n", descriptor));

	return get_player(storage, desc2id(storage, descriptor));
}

/*============ storage manipulation */

void remove_player(p_players_storage storage, int id)
{
	VRB(4, printf("player::remove_player(id = %d)\n", id));
	if(id >= storage->capacity){
		VRB(1,fprintf(stderr, "The id=%d more than storage capasity =%d",
				id, storage->capacity));
		exit(-1);
	}
	destroy_player(storage->players[id]);
	storage->connected--;
	storage->descs[id] = -1;
}

void destroy_storage(p_players_storage storage)
{
	int i;

	VRB(4, printf("player::destroy_storage\n"));
	for(i = 0; i < storage->capacity; i++){
		destroy_player(storage->players[i]);
	}
	free(storage->descs);
	free(storage->players);
	free(storage);
}

/*========================== id<->desc table =========================*/
/* returns -1 if not found */
int id2desc(p_players_storage storage, int id)
{
	VRB(4, printf("player::id2desc(id = %d)\n", id));
	if(storage->descs == NULL){
		return -1;
	}

	if(id >= 0 && id < storage->capacity){
		return storage->descs[id];
	}

	return -1;
}

/* returns -1 if not found */
int desc2id(p_players_storage storage,  int desc)
{
	int id;

	VRB(4, printf("player::desc2id(desc = %d)\n", desc));

	if(storage->descs == NULL){
		return -1;
	}

	for(id = 0; id < storage->capacity; id++){
		if(storage->descs[id] == desc){
			return id;
		}
	}

	return -1;
}

/*=================== queue & buffer jobs ============================*/

static int parse_messages(p_player p);

static void push_queue(struct message_queue * queue, char * s)
{
	struct message_list * p;

	VRB(4, printf("player::push_queue(%s)\n", s));

	p = (struct message_list *) malloc (sizeof(*p));
	p->prev = queue->lo;
	p->next = NULL;
	p->message = s;
	if(queue->lo != NULL){
		queue->lo->next = p;
		queue->lo = p;
	} else {
		queue->lo = queue->hi = p;
	}

	queue->count++;
}

static char * pop_queue(struct message_queue * queue)
{
	struct message_list *p;
	char * s;

	VRB(4, printf("player::pop_queue()\n"));

	if(queue->hi == NULL){
		return NULL;
	}

	s = queue->hi->message;
	p = queue->hi->next;
	free(queue->hi);
	if(p != NULL){
		p->prev = NULL;
		queue->hi = p;
	} else {
		queue->hi = queue->lo = NULL;
	}

	queue->count--;

	return s;
}

int push_buffer(p_players_storage storage, int id, char * buffer, int buffer_len)
{
	p_player p = p = get_player(storage, id);
	int size;

	if(p == NULL){
		return -1;
	}
	size = p->buffer_size + buffer_len;

	/* copy to player's buffer */
	if(size > globalArgs.kick_buffer_limit){
		/* kick the player, please */
		return -1;
	}
	if(size > p->buffer_allocated){
		p->buffer = (char *) realloc(p->buffer, size);
		p->buffer_allocated = size;
	}

	memcpy(p->buffer + p->buffer_size, buffer, buffer_len * sizeof(char));
	p->buffer_size = size;

	return parse_messages(p);
}

/**
 * TODO: buffer reallocation
 * 		 and recounting
 */
/* returns the #of new messages */
static int parse_messages(p_player p)
{
	char * end, * begin = p->buffer;
	char * new_buffer;
	char * s;
	int buf_len = p->buffer_size, delta;
	int cnt = 0;
	int slash_r_found = 0;

	VRB(4, printf("player::parse_messages(id = %d)\n", p->id));

	while( (end = memchr(begin, '\n', buf_len)) != NULL ){
		/* (begin, end) => p->message */
		slash_r_found = 0;
		if(end - begin > 0){
			if(*(end-1) == '\r'){
				end--;
				slash_r_found = 1;
			}
		}
		delta = end - begin + 1;

		s = (char *) malloc ( (delta + 1) * sizeof(*s) );
		strncpy(s, begin, delta);
		s[delta] = '\0';
		if(slash_r_found){
			s[delta-1] = '\n';
		}
		push_queue(p->messages, s);

		if(slash_r_found){
			end++;
		}

		begin = end+1;
		buf_len -= delta;
		cnt++;
	}

	/* delete unneed from the buffer */
	delta = begin - p->buffer; /* dumped difference */
	if(delta == 0){
		return 0;
	}

	if(delta >= p->buffer_size){
		p->buffer_size = 0;
		if(p->buffer_allocated != DEFAULT_BUFFER){
			free(p->buffer);
			p->buffer = (char *) malloc (DEFAULT_BUFFER * sizeof(char));
		}
		p->buffer_allocated = DEFAULT_BUFFER;
	} else {
		new_buffer = (char *) malloc ((p->buffer_size - delta) * sizeof(char));
		memcpy(new_buffer, p->buffer + delta, p->buffer_size - delta);
		free(p->buffer);
		p->buffer = new_buffer;
		p->buffer_size -= delta;
		p->buffer_allocated = p->buffer_size;
	}

	return cnt;
}

char * get_message(p_players_storage storage, int id)
{
	p_player p;
	VRB(4, printf("player::get_message(id = %d)\n", p->id));
	if( (p = get_player(storage, id)) == NULL){
		return NULL;
	}

	return pop_queue(p->messages);
}
