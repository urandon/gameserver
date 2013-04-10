#ifndef SERVER_H_
#define SERVER_H_

enum message_type {
	U_BROADCAST, U_PRIVATE, // user's messages
	S_BROADCAST, S_HUMAN, S_MACHINE, // server's messages
	NO_TYPE // when type is not defined
};

void send_dirrect(int id, const char * message, enum message_type type);
int send_dirrect_buff(int descriptor, const char * buffer, int length, enum message_type type);
void send_broadcast(const char * message, enum message_type type);
// void send_broadcast_raw(const char * message, enum message_type type); // is deprecated
void send_broadcast_buff(const char * buffer, int length, enum message_type type);
void kick_player(int id, char * description);

#endif
