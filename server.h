#ifndef SERVER_H
#define SERVER_H

void send_dirrect(int descriptor, const char * message);
void send_broadcast(const char * message);
void kick_player(int id, char * description);


#endif
