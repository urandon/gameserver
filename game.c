#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "server.h"
#include "game.h"
#include "sort.h"
#include "gameconstants.h"
#include "tokens.h"

typedef struct gamer_instance *p_gamer_instance;

/* contains pointers to active gamers */
enum BUFFERSIZE_T{
	BUFFERSIZE = 512
};
static struct global_t{
	p_gamer_instance * gamers;
	int total_players;
	int active_players;
	int finished_players;
	int trade_players;
	int month;
	int current_market_state;
	char message_buffer[BUFFERSIZE];
	int gc_counter;
} global;


enum auction_type {
	RESOURCE,
	PRODUCTION
};

/* In-game interface */
void game_request_get_playerinfo(int from_id, int about_id);
void game_request_get_marketinfo(int from_id);
void game_request_get_generalinfo(int from_id);
void game_request_build_factory(int id, int quantity);
void game_request_convert_resource(int id, int quantity);
void game_request_auction_bid(int id, enum auction_type type, int value, int price);
void game_request_finish_turn(int id);
void game_request_help(int id);

void game_player_pay(int id, int money);
int game_player_total_month_spent(int id);

void game_auction_totals();
void game_month_totals();
void reset_request(int id);


p_gamer_instance new_gamer_instance(int id)
{
	p_gamer_instance p;
	int i;

	p = (p_gamer_instance) malloc (sizeof(*p));

	p->id = id;
	p->res			= default_gamer.res;
	p->prod			= default_gamer.prod;
	p->factory[0]	= default_gamer.factory[0];
	p->cash			= default_gamer.cash;
	p->finished_turn	= 0;

	p->request.res2prod = 0;
	p->request.res_value = 0;
	p->request.res_price = 0;
	p->request.res_approved = 0;
	p->request.prod_value = 0;
	p->request.prod_price = 0;
	p->request.prod_approved = 0;


	for(i = 1; i < FACTORY_BUILD_TIME; i++){
		p->factory[i] = 0;
	}

	return p;
}

void game_new_month()
{
	int id, i, r;
	int acc;

	global.month++;
	global.finished_players = 0;
	global.trade_players = 0;

	for(id = 0; id < global.total_players; id++){
		if(global.gamers[id] != NULL){
			/* shift the factories to the month back */
			int * factories = (int *)(global.gamers[id]->factory);
			int k, curr_factories = factories[0];

			for(k = 0; k < FACTORY_BUILD_TIME-1; k++){
				factories[k] = factories[k+1];
			}
			factories[0] += curr_factories;
			factories[FACTORY_BUILD_TIME-1] = 0;

			sprintf(global.message_buffer,
					"Congrutilations! You have %d new factories\n",
					curr_factories);

			/* reset monthly activity */
			reset_request(id);
			global.gamers[id]->finished_turn = 0;
		}
	}


	/* Change market state according to the Markov process */

	r = 1 + (int)(12.0*rand()/(RAND_MAX+1.0)); /* r in  [1,...,12] */
	i = acc = 0;
	while(acc < r){
		acc += Markov_matrix[global.current_market_state][i];
		i++;
	}
	/* i refer to the_next_state */
	global.current_market_state = i - 1;

	sprintf(global.message_buffer,
			"Happy new %d month! The market state is %d\n",
			global.month, global.current_market_state+1);
	send_broadcast(global.message_buffer);
}


void game_start(int n)
{
	int id;

	srand(clock());
	global.gamers = (p_gamer_instance *) malloc (n * sizeof(*global.gamers));
	global.active_players = global.total_players = n;

	for(id = 0; id < global.total_players; id++){
		global.gamers[id] = new_gamer_instance(id);
	}

	global.month = 1;
	global.finished_players = 0;
	global.trade_players = 0;
	global.current_market_state = default_market_state - 1;

	global.gc_counter = 0;
}

void game_finish()
{
	int i;
	for(i = 0; i < global.total_players; i++){
		free(global.gamers[i]);
	}
	free(global.gamers);
	global.gamers = 0;
}

void game_kick(int id)
{
	if(global.gamers != NULL && id >= 0 && id < global.total_players){
		if(global.gamers[id] != NULL){
			if(global.gamers[id]->finished_turn){
				global.finished_players--;
			}
			free(global.gamers[id]);
			global.gamers[id] = NULL;
			global.active_players--;
		}
	}
}

void reset_request(int id){
	p_gamer_instance g = global.gamers[id];
	if(g != NULL){
		g->request.res2prod = 0;
		g->request.res_value = 0;
		g->request.res_price = 0;
		g->request.res_approved = 0;
		g->request.prod_value = 0;
		g->request.prod_price = 0;
		g->request.prod_approved = 0;
	}
}


void game_request_get_playerinfo(int from_id, int about_id)
{
	p_gamer_instance g;
	int id = about_id;

	if(!(id >= 0 && id < global.total_players)){
		sprintf(global.message_buffer,
				"Wrong player id[%d]. "
				"Try another value between 0 and %d\n",
				id, global.total_players);
	} else
	if( (g = global.gamers[id]) == NULL){
		sprintf(global.message_buffer,
				"The player[%d] went into a bankrupt\n", id);
	} else {
		int i, all_factories = 0;
		for(i = 0; i < FACTORY_BUILD_TIME; i++){
			all_factories += g->factory[i];
		}

		sprintf(global.message_buffer,
				"Player[%d] info:\n"
				"\tmoney\t= %ld\n"
				"\tfactories\t(active) = %d\t(in progress) = %d\n"
				"\tproduction\t= %d\n"
				"\tresource\t= %d\n",
				id, g->cash,
				g->factory[0], all_factories - g->factory[0],
				g->prod, g->res);
	}

	send_dirrect(from_id, global.message_buffer);
}

void game_request_get_generalinfo(int from_id)
{
	sprintf(global.message_buffer,
			"General info (game constants):\n"
			"\tFactiory build time\t= %d\n"
			"\tFactory price\t= %d\n"
			"\tConvertation (resource->production) price\t= %d\n"
			"\tMonthy expenditures (for each object):\n"
			"\t\tfactory\t= %d\n"
			"\t\tresource\t= %d\n"
			"\t\tproduction\t= %d\n"
			,FACTORY_BUILD_TIME,factory_price,
			convertation_price,
			monthy.factory, monthy.res, monthy.prod
			);
	send_dirrect(from_id, global.message_buffer);
}

void game_request_get_marketinfo(int from_id)
{
	struct market_state_t * ms =
			(struct market_state_t *)(market_state + global.current_market_state);

	sprintf(global.message_buffer,
			"Market info:\n"
			"\tMarket state#\t= %d\n"
			"\tMinimal resource price\t= %d\n"
			"\tMaximal production price\t= %d\n"
			"\tResource quantity\t= %d\n"
			"\tProduction needs\t= %d\n"
			"\tActive players\t= %d\n",
			global.current_market_state+1,
			ms->res_price, ms->prod_price,
			(int)(ms->res_coeff  * global.active_players),
			(int)(ms->prod_coeff * global.active_players),
			global.active_players
			);
	send_dirrect(from_id, global.message_buffer);
}

void game_request_help(int id)
{
	if(global.gamers[id] != NULL){
		send_dirrect(id, help_info);
	} else {
		send_dirrect(id, help_info_for_bankrupts);
	}
}

void game_player_pay(int id, int money)
{
	if(global.gamers[id] != NULL){
		global.gamers[id]->cash -= money;
		if(global.gamers[id]->cash < 0){
			sprintf(global.message_buffer,
					"Player [%d] goes bankrupt\n",
					id);
			send_broadcast(global.message_buffer);
			game_kick(id);
		}
	}
}

int game_player_total_month_spent(int id)
{
	p_gamer_instance g = global.gamers[id];
	struct request_t * r;

	if(g == NULL){
		return 0;
	}

	r = &(g->request);

	return	/* factory building */
			/* the first factory building is payed immediately
			 * not in the end of the month */
			g->factory[1] * factory_price/2 +
			/* market trades */
			(- r->prod_approved * r->prod_price) +
			r->res_approved * r->res_price +
			/* monthly expenditures */
			g->factory[0] * monthy.factory +
			g->prod * monthy.prod +
			g->res * monthy.res;
}




int cmp_res(int id1, int id2){
	return	global.gamers[id1]->request.res_price >
			global.gamers[id2]->request.res_price;
}
int cmp_prod(int id1, int id2){
	return	global.gamers[id1]->request.prod_price <
			global.gamers[id2]->request.prod_price;
}
void game_auction_totals()
{
	int * buffer = (int *) malloc (global.trade_players * sizeof(int));
	int * sorted = (int *) malloc (global.trade_players * sizeof(int));
	int val_left, val_transfered, traders;
	int i, lo, hi, tmp, r;
	p_gamer_instance * g = global.gamers;

	/* resource trade */
	traders = 0;
	for(i = 0; i < global.total_players; i++){
		if(g[i] != NULL){
			if(g[i]->request.res_value != 0){
				sorted[traders++] = i;
			}
		}
	}
	sort(sorted, traders, cmp_res, buffer);
	/* take gifts */
	val_left = (int) (market_state[global.current_market_state].res_coeff *
				global.active_players);
	lo = hi = 0;
	while(val_left > 0 && lo < traders){
		if(lo > hi){
			hi = lo;
		}
		while(hi < traders && g[sorted[lo]]->request.res_price ==
				g[sorted[hi]]->request.res_price){
			hi++;
		}
		hi--;

		while(lo <= hi && val_left > 0){
			if(lo < hi){
				/* pick one of them by random */
				r = lo + (int)(((float)(hi-lo+1))*rand()/(RAND_MAX+1.0));
				/* swap sorted[r] and sorted[lo] */
				tmp = sorted[r];
				sorted[r] = sorted[lo];
				sorted[lo] = tmp;
			}
			if(g[sorted[lo]]->request.res_value < val_left){
				val_transfered = g[sorted[lo]]->request.res_value;
			} else {
				val_transfered = val_left;
			}
			g[sorted[lo]]->request.res_approved = val_transfered;
			val_left -= val_transfered;
			lo++;
		}
	}


	/* production trade */
	traders = 0;
	for(i = 0; i < global.total_players; i++){
		if(g[i] != NULL){
			if(g[i]->request.prod_value != 0){
				sorted[traders++] = i;
			}
		}
	}
	sort(sorted, traders, cmp_prod, buffer);
	/* take gifts */
	val_left = (int) (market_state[global.current_market_state].prod_coeff *
				global.active_players);
	lo = hi = 0;
	while(val_left > 0 && lo < traders){
		if(lo > hi){
			hi = lo;
		}
		while(hi < traders && g[sorted[lo]]->request.prod_price ==
				g[sorted[hi]]->request.prod_price )
		{
			hi++;
		}
		hi--;

		while(lo <= hi && val_left > 0){
			if(lo < hi){
				/* pick one of them by random */
				r = lo + (int)(((float)(hi-lo+1))*rand()/(RAND_MAX+1.0));
				/* swap sorted[r] and sorted[lo] */
				tmp = sorted[r];
				sorted[r] = sorted[lo];
				sorted[lo] = tmp;
			}
			if(g[sorted[lo]]->request.prod_value < val_left){
				val_transfered = g[sorted[lo]]->request.prod_value;
			} else {
				val_transfered = val_left;
			}
			g[sorted[lo]]->request.prod_approved = val_transfered;
			val_left -= val_transfered;
			lo++;
		}
	}

	free(buffer);
	free(sorted);
}

void game_month_totals()
{
	p_gamer_instance g;
	int id;
	for(id = 0; id < global.total_players; id++){
		if((g = global.gamers[id]) != NULL){
			if(g->request.res2prod != 0){
				g->prod += g->request.res2prod;
				sprintf(global.message_buffer,
						"You have produced %d production\n",
						g->request.res2prod);
				send_dirrect(id, global.message_buffer);
			}
			if(g->request.prod_approved != 0){
				g->prod -= g->request.prod_approved;
				sprintf(global.message_buffer,
						"Congrutilations! You have sold %d products\n",
						g->request.prod_approved);
				send_dirrect(id, global.message_buffer);
			}
			if(g->request.res_approved != 0){
				g->res += g->request.res_approved;
				sprintf(global.message_buffer,
						"Congrutilations! You have bought %d resources\n",
						g->request.res_approved);
				send_dirrect(id, global.message_buffer);
			}
			game_player_pay(id, game_player_total_month_spent(id));
		}
	}
}


/*============================= requests ================================*/

void game_request_auction_bid(int id, enum auction_type type, int value, int price)
{
	struct market_state_t * ms =
			(struct market_state_t * )(market_state + global.current_market_state);
	struct request_t * req = NULL;
	int was_requester = global.gamers[id]->request.prod_value &&
						global.gamers[id]->request.res_value;
	int still_requester;

	if(global.gamers[id] != NULL){
		req = &(global.gamers[id]->request);
	}

	if(type == RESOURCE){
		if(ms->res_price > price){
			sprintf(global.message_buffer,
					"You can't take such small bid. "
					"The minimal price for resource is %d\n",
					ms->res_price);
			send_dirrect(id, global.message_buffer);
		} else {
			req->res_value = value;
			req->res_price = price;
			sprintf(global.message_buffer,
					"Your current bid is %d resources for $%d per one\n"
					"You can change your request if you want\n",
					value, price);
			send_dirrect(id, global.message_buffer);
		}
	} else { /* type == PRODUCTION */
		if(ms->prod_price < price){
			sprintf(global.message_buffer,
					"You can't take such big bid. "
					"The maximal price for production is %d\n",
					ms->prod_price);
			send_dirrect(id, global.message_buffer);
		} else
		if(global.gamers[id]->prod < value){
			sprintf(global.message_buffer,
					"You can't sell production that do don't have. "
					"Your avaible production quantity is %d.\n",
					global.gamers[id]->prod);
			send_dirrect(id, global.message_buffer);

		} else {
			req->prod_value = value;
			req->prod_price = price;
			sprintf(global.message_buffer,
					"Your current bid is %d production for $%d per one\n"
					"You can change your request if you want\n",
					value, price);
			send_dirrect(id,  global.message_buffer);
		}
	}

	still_requester = global.gamers[id]->request.prod_value	&&
			global.gamers[id]->request.res_value;

	if(was_requester && !still_requester){
		global.trade_players--;
	} else
	if(!was_requester && still_requester){
		global.trade_players++;
	}


}

void game_request_build_factory(int id, int quantity)
{
	if(global.gamers[id] != NULL){
		global.gamers[id]->factory[FACTORY_BUILD_TIME-1] += quantity;
		sprintf(global.message_buffer,
				"You requested to build %d factories. "
				"It will be avaible in %d months.\n",
				quantity, FACTORY_BUILD_TIME);
		send_dirrect(id, global.message_buffer);
		game_player_pay(id, factory_price * quantity);
	}
}

void game_request_convert_resource(int id, int quantity)
{
	p_gamer_instance p = global.gamers[id];
	if(p != NULL){
		if(p->request.res2prod + quantity <= p->factory[0] && p->res >= quantity){
			p->request.res2prod += quantity;
			p->res -= quantity;
			sprintf(global.message_buffer,
					"You requested to produce %d production. "
					"It will be avaible on the next month.\n",
					quantity);
			send_dirrect(id, global.message_buffer);
			game_player_pay(id, convertation_price * quantity);
		} else
		if(p->res < quantity){
			sprintf(global.message_buffer,
					"Can't produce to much production. "
					"Not enough of resource (%d left)\n",
					p->res);
			send_dirrect(id, global.message_buffer);
		} else {
			sprintf(global.message_buffer,
					"Can't produce to much production. "
					"Not enough of active non-working factories (%d left)\n",
					p->factory[0] - p->request.res2prod);
			send_dirrect(id, global.message_buffer);
		}
	}
}

void game_request_finish_turn(int id)
{
	if(global.gamers[id] != NULL){
		if(global.gamers[id]->finished_turn == 0){
			global.gamers[id]->finished_turn = 1;
			global.finished_players++;
			sprintf(global.message_buffer,
					"The player[%d] has finished the turn\n", id);
			send_broadcast(global.message_buffer);
		} else {
			sprintf(global.message_buffer,
					"Heh. You can't finish the turn twice.\n");
			send_dirrect(id, global.message_buffer);
		}
	}
	if(global.finished_players >= global.active_players){
		game_state_update();
	}
}

/*============================= game process ================================*/

int game_state_update()
{
	int id;
	if(global.active_players <= 1){
		for(id = 0; id < global.total_players; id++){
			if(global.gamers[id] != NULL){
				send_dirrect(id, "You WIN!\n");
			}
		}
		return -1;
	}
	if(global.active_players <= global.finished_players){
		send_broadcast(
				"All the turns finished. It's time to look at the stats.\n");
		game_auction_totals();
		game_month_totals();
		game_new_month();
	}

	return 0;
}

/* returns -1 in case of empty command */
int game_command(int id, char * command)
{
	p_tokens tokens = split(command);
	char * word;
	int val = 0, price = 0;
	int cnt = tokens->cnt;

	switch(tokens->cnt){
		case 3:
			price = atoi(tokens->token->next->next->word);
		case 2:
			val = atoi(tokens->token->next->word);
		case 1:
			word = tokens->token->word;
			break;
		default:
			send_dirrect(id, "Unknown command\n");
			game_request_help(id);
		case 0:
			destroy_tokens(tokens);
			return -1;
	};


	/* common tread both for gamers and zombies */

	if(!strcmp(word, "gcinc")){
		global.gc_counter++;
		sprintf(global.message_buffer,
				"Global counter increased. Current value is %d\n", global.gc_counter);
		send_broadcast(global.message_buffer);
	} else
	if(!strcmp(word, "gcdec")){
		global.gc_counter--;
		sprintf(global.message_buffer,
				"Global counter increased. Current value is %d\n", global.gc_counter);
		send_broadcast(global.message_buffer);
	} else

	if(!strcmp(word, "market")){
		game_request_get_marketinfo(id);
	} else
	if(!strcmp(word, "player") && cnt == 1){
		game_request_get_playerinfo(id, val);
	} else
	if(!strcmp(word, "stat")){
		game_request_get_playerinfo(id, id);
	} else
	if(!strcmp(word, "help")){
		game_request_help(id);
	} else
	if(!strcmp(word, "generalinfo")){
		game_request_get_generalinfo(id);
	} else

	if(global.gamers[id] != NULL){ /* if user is can play */
		if(!strcmp(word, "turn")){
			game_request_finish_turn(id);
		} else
		if(!strcmp(word, "build") && cnt == 2){
			game_request_build_factory(id, val);
		} else
		if(!strcmp(word, "prod") && cnt == 2){
			game_request_convert_resource(id, val);
		} else
		if(!strcmp(word, "buy") && cnt == 3){
			game_request_auction_bid(id, RESOURCE, val, price);
		} else
		if(!strcmp(word, "sell") && cnt == 3){
			game_request_auction_bid(id, PRODUCTION, val, price);
		} else {
			send_dirrect(id, "Unknown command\n");
			game_request_help(id);
		}
	} else { /* if user can't play */
		send_dirrect(id, "Unknown command\n");
		game_request_help(id);
	}

	send_dirrect(id, ">> ");


	destroy_tokens(tokens);

	return 0;
}
