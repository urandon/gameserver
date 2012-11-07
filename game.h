#ifndef GAME_H_
#define GAME_H_

void game_start(int number_of_players);
void game_finish();
/*returns -1 when the game is over */
int game_state_update();
void game_kick(int id);
int game_command(int id, char * command);

#endif
