#include <stdlib.h>
#include <string.h>
#include "player.h"
#include "settings.h"


#define DEFAULT_BUFFER 64

extern struct globalArgs_t globalArgs;

/* int next_id()'s variables */
static const int tollerance_coeff = 3;
static int current_id = 0;


static int *descs = NULL;
static void init();
static char * default_name = "Anonymous";

/*===================== single player methods ========================*/

/*============ constuctor & desctructor */

int next_id()
{
	while(descs[current_id] != 0){
		if(current_id >= globalArgs.players * tollerance_coeff){
			current_id = 0;
		} else {
			current_id++;
		}
	}
	return current_id;
}

/**
 * TODO: max_id => int next_id();
 */
p_player new_player(int descriptor)
{
	p_player p;

	init();
	VRB(4, printf("player::new_player(desc = %d)\n", descriptor));

	p = (p_player) malloc (sizeof(*p));

	p->id = next_id();
	p->descriptor = descriptor;
	descs[p->id] = p->descriptor;
	p->name = NULL;
	change_name(p, default_name);

	p->messages = (struct message_queue *) malloc (sizeof(*(p->messages)));
	p->messages->lo = NULL;
	p->messages->hi = NULL;
	p->messages->count = 0;
	p->buffer_allocated = DEFAULT_BUFFER;
	p->buffer_size = 0;
	p->buffer = (char *) malloc (p->buffer_allocated * sizeof(char));

	p->data = new_game_instance(p->id);

	return p;
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

void destroy_player(p_player p)
{
	if(p != NULL){
		VRB(4, printf("player::destroy_player(id = %d)\n", p->id));

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
	VRB(4, printf("player::look4player(id = %d)\n", id));

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
	p_players_storage p;

	VRB(4, printf("player::get_player(id = %d)\n", id));
	p = look4player(storage, id);

	if(p != NULL){
		return p->node;
	}

	return NULL;
}

p_player get_player_by_desc(p_players_storage storage, int descriptor)
{
	VRB(4, printf("player::get_player_by_desc(desc = %d)\n", descriptor));

	return get_player(storage, desc2id(descriptor));
}

/*============ storage manipulation */

p_players_storage add_player(p_players_storage storage, p_player p)
{
	p_players_storage s;

	if(p ==  NULL){
		VRB(4, printf("player::add_player(NULL)\n"));
		return storage;
	}

	VRB(4, printf("player::add_player(id = %d)\n", p->id));

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
	p_players_storage p, l, r;

	VRB(4, printf("player::remove_player(id = %d)\n", id));

	p = look4player(storage, id);
	if(p == NULL){
		return storage;
	}

	l = p->prev;
	r = p->next;

	if(l != NULL){
		l->next = r;
	} else {
		storage = r;
	}

	if(r != NULL){
		r->prev = l;
	}

	destroy_player(p->node);
	free(p);
	descs[id] = 0;

	return storage;
}


void destroy_storage(p_players_storage storage)
{
	p_players_storage l = storage->prev, r = storage->next;

	VRB(4, printf("player::destroy_storage\n"));

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
	VRB(4, printf("player::id2desc(id = %d)\n", id));
	if(descs == NULL){
		return -1;
	}

	if(id >= 0 && id < globalArgs.players * tollerance_coeff){
		return descs[id];
	}

	return -1;
}

/* returns -1 if not found */
int desc2id(int desc){
	int id;

	VRB(4, printf("player::desc2id(desc = %d)\n", desc));

	if(descs == NULL){
		return -1;
	}

	for(id = 0; id < globalArgs.players * tollerance_coeff; id++){
		if(descs[id] == desc){
			return id;
		}
	}

	return -1;
}

/**
 * TODO:!!!
 * fix for max_id > globalArgs.players;
 */
static void init()
{
	if(descs == NULL){
		int i;
		VRB(4, printf("player::init()\n"));
		descs = (int *) malloc(globalArgs.players * tollerance_coeff * sizeof(*descs));
		for(i = 0; i < globalArgs.players * tollerance_coeff; i++){
			descs[i] = 0;
		}
	}

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

int push_buffer(p_player p, char * buffer, int buffer_len)
{
	int size = p->buffer_size + buffer_len;

	VRB(4, printf("player::push_buffer(id = %d, buffer)\n", p->id));

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

	VRB(4, printf("player::parse_messages(id = %d)\n", p->id));

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
	VRB(4, printf("player::get_message(id = %d)\n", p->id));

	return pop_queue(p->messages);
}
