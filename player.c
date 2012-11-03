#include <stdlib.h>
#include <string.h>
#include "player.h"
#include "settings.h"

#include <stdio.h>


#define DEFAULT_BUFFER 64

extern struct globalArgs_t globalArgs;

static int max_id = 0;
static int *descs = NULL;
static void init();
static char * default_name = "Anonymous";

/*===================== single player methods ========================*/

/*============ constuctor & desctructor */

/**
 * TODO: update constructor for messages
 */
p_player new_player(int descriptor)
{
	p_player p;
	
	p = (p_player) malloc (sizeof(*p));
	
	change_name(p, default_name);
	p->id = max_id++;
	p->descriptor = descriptor;
	
	p->messages = (struct message_queue *) malloc (sizeof(*(p->messages)));
	p->messages->lo = NULL;
	p->messages->hi = NULL;
	p->messages->count = 0;
	p->buffer_allocated = DEFAULT_BUFFER;
	p->buffer_size = 0;
	p->buffer = (char *) malloc (p->buffer_allocated * sizeof(char));
	
	p->data = new_game_instance(p->id);
	
	init();
	
	return p;
}

void change_name(p_player p, char * new_name)
{
	char * s = (char *) malloc ((strlen(new_name) + 1) * sizeof(char));
	strcpy(s, new_name);
	free(p->name);
	p->name = s;
}

static void destroy_messages(p_player p)
{
	struct message_list *m, *n;
	
	if(p->messages != NULL){
		m = p->messages->lo;
		while(m != NULL){
			n = m->next;
			free(m->message);
			free(m);
			m = n;
		}
	}
	
	free(p->messages);
	p->messages = NULL;
}

void destroy_player(p_player p)
{
	if(p != NULL){
		descs[p->id] = 0;
		free(p->name);
		free(p->messages);
		free(p->buffer);
		destroy_messages(p);
		free(p);
	}
}

/*============ search engine */

static p_players_storage look4player(p_players_storage storage, int id)
{
	p_players_storage s = storage;
	
	while(s != NULL){
		if(s->node->id == id){
			return s;
		}		
		s = s->next;
	}
	
	return NULL;
}

p_player get_player(p_players_storage storage, int id)
{
	p_players_storage p = look4player(storage, id);
	
	if(p != NULL){
		return p->node;
	}
	
	return NULL;
}

p_player get_player_by_desc(p_players_storage storage, int descriptor)
{
	return get_player(storage, desc2id(descriptor));
}

/*============ storage manipulation */

p_players_storage add_player(p_players_storage storage, p_player p)
{
	p_players_storage s;
	
	if(p ==  NULL){
		return storage;
	}
	
	s = (p_players_storage) malloc (sizeof(*s));
	s->next = storage;
	if(storage != NULL){
		storage->prev = s;
	}
	s->node = p;
	
	init();	
	descs[p->id] = p->descriptor;
	
	return s;
}

p_players_storage remove_player(p_players_storage storage, int id)
{
	p_players_storage p = look4player(storage, id), l, r;
	
	if(p == NULL){
		return storage;
	}
	
	l = p->prev;
	r = p->next;
	
	if(l != NULL){
		l->next = p;
	} else {
		storage = r;
	}
	
	if(r != NULL){
		r->prev = l;
	}
	
	destroy_player(p->node);
	free(p);
	
	return storage;
}


void destroy_storage(p_players_storage storage)
{
	p_players_storage l = storage->prev, r = storage->next;
	
	destroy_player(storage->node);
	free(storage);
	
	while(l != NULL){
		destroy_player(l->node);
		free(l);
		l = l->prev;
	}
	
	while(r != NULL){
		destroy_player(r->node);
		free(r);
		r = r->next;
	}
	
	free(descs);
	descs = NULL;
}

/*========================== id<->desc table =========================*/


/* returns -1 if not found */
int id2desc(int id){
	if(descs == NULL){
		return -1;
	}
	
	if(id >= 0 && id < globalArgs.players){
		return descs[id];
	}
	
	return -1;
}

/* returns -1 if not found */
int desc2id(int desc){
	int id;

	if(descs == NULL){
		return -1;
	}
	
	for(id = 0; id < globalArgs.players; id++){
		if(descs[id] == desc){
			return id;
		}
	}
	
	return -1;
}

static void init()
{
	if(descs == NULL){
		int i;
		descs = (int *) malloc(globalArgs.players * sizeof(*descs));
		for(i = 0; i < globalArgs.players; i++){
			descs[i] = 0;
		}
	}

}

/*=================== queue & buffer jobs ============================*/

static int parse_messages(p_player p);

static void push_queue(struct message_queue * queue, char * s)
{
	struct message_list * p;
	
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
	
	if(queue->hi == NULL){
		return NULL;
	} 
	
	s = queue->hi->message;
	p = queue->hi->next;
	if(p != NULL){
		p->prev = NULL;
	}
	queue->hi = p;
		
	queue->count--;
	
	return s;
}

int push_buffer(p_player p, char * buffer, int buffer_len)
{
	int size = p->buffer_size + buffer_len;
	
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
	
	while( (end = memchr(begin, '\n', buf_len)) != NULL ){
		/* (begin, end) => p->message */
		delta = end - begin + 1;
		
		s = (char *) malloc ( (delta + 1) * sizeof(*s) );
		strncpy(s, begin, delta);
		s[delta + 1] = '\0';
		push_queue(p->messages, s);
		
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

char * get_message(p_player p)
{
	return pop_queue(p->messages);
}
