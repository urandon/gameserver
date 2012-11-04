#
# Makefile
#
SOURCES.c = server.c settings.c player.c game.c
INCLUDES  =  server.h settings.h player.h game.h
CFLAGS = -Wall
C_DEBUG_FLAGS = -g3 -DDEBUG_ALL
C_RELEASE_FLAGS = -O3
CC = gcc
PROGRAM = server

OBJECTS = $(SOURCES.c:.c=.o)

.PHONY = all clean install
.KEEP_STATE:

.PHONY: debug  
debug: CFLAGS+=$(C_DEBUG_FLAGS)  
debug: server

.PHONY: release  
release: CFLAGS+=$(C_RELEASE_FLAGS)
release: server

all: $(PROGRAM)

server: $(INCLUDES) $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(SLIBS) -o server

server.o: server.c server.h settings.h game.h
	$(CC) $(CFLAGS) server.c -c

settings.o: settings.c settings.h
	$(CC) $(CFLAGS) settings.c -c
	
player.o: player.c player.h game.h settings.h
	$(CC) $(CFLAGS) player.c -c
	
game.o: game.c game.h settings.h server.h
	$(CC) $(CFLAGS) game.c -c

clean:
	rm -rf $(PROGRAM) $(OBJECTS)
