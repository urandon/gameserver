#include <stdlib.h>
#include "game.h"
#include "settings.h"

extern struct globalArgs_t globalArgs;

/* static int * gamers = NULL; */

p_game_instance new_game_instance(int id)
{
	p_game_instance p;

	p = (p_game_instance) malloc (sizeof(*p));

	p->id = id;
	p->resource = 4;
	p->production = 2;
	p->factory = 2;
	p->cash = 10000;

	return p;
}

int game_kick(int id)
{
	return id;
}
