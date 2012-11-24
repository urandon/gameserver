#ifndef SERVER_H_
#define SERVER_H_

void send_dirrect(int id, const char * message);
int send_dirrect_buff(int descriptor, const char * buffer, int length);
void send_broadcast(const char * message);
void send_broadcast_buff(const char * buffer, int length);
void kick_player(int id, char * description);


#endif
