#ifndef GAMECONSTANTS_H_
#define GAMECONSTANTS_H_

#define FACTORY_BUILD_TIME 5

const int convertation_price = 2000;
const int factory_price = 5000;

const struct monty_expenditures_t{
	int factory;
	int res;
	int prod;
} monthy = {
		1000, 500, 300
};

struct request_t {
	int res2prod;
	int res_value;
	int res_price;
	int res_approved;
	int prod_value;
	int prod_price;
	int prod_approved;
};

struct gamer_instance{
	int id;

	/* factory[0] == active_factories
	 * factory[k] == will_be_active_after_k_mounths */
	int factory[FACTORY_BUILD_TIME];
	int res;
	int prod;
	long int cash;
	int finished_turn;
	struct request_t request;
} default_gamer = {
		0, {2}, 4, 2, 1000, 0,
		{0, 0, 0, 0, 0, 0, 0}
};


const struct market_state_t {
		float res_coeff;
		int res_price;
		float prod_coeff;
		int prod_price;
} market_state[5] = {
		{1.0, 800, 3.0, 6500},
		{1.5, 650, 2.5, 6000},
		{2.0, 500, 2.0, 5500},
		{2.5, 400, 1.5, 5000},
		{3.0, 300, 1.0, 4000}
};
const int default_market_state = 3;

const float Markov_matrix[5][5] = {
		{4, 4, 2, 1, 1},
		{3, 4, 3, 1, 1},
		{1, 3, 4, 3, 1},
		{1, 1, 3, 4, 3},
		{1, 1, 2, 4, 4},
};

char help_info[] = \
		"Possible commands:\n"
		"\tmarket\tshows the market state\n"
		"\tplayer id\tshows player info with taken id\n"
		"\tprod val\tproduce 'val' production\n"
		"\tbuy val price\tsend a request to buy 'val' resource for 'price'\n"
		"\tsell val price\tsend a request to sell 'val' production for 'price'\n"
		"\tbuild val\tbuilt 'val' factories (you can use it in 5 month)\n"
		"\tturn\tfinish this turn\n"
		"\thelp\tdisplay this message\n"
		;


#endif
