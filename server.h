#ifndef SERVER_H_
#define SERVER_H_

void send_dirrect(int descriptor, const char * message);
void send_broadcast(const char * message);
void kick_player(int id, char * description);


#endif
