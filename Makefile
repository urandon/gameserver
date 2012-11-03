#
# Makefile
#
SOURCES.c= server.c settings.c player.c game.c
INCLUDES=  server.h settings.h player.h game.h
CLAGS=
SLIBS=
PROGRAM= server

OBJECTS= $(SOURCES.c:.c=.o)

.KEEP_STATE:

debug := CFLAGS= -g

all debug: $(PROGRAM)

server: $(INCLUDES) $(OBJECTS)
	gcc $(OBJECTS) $(SLIBS) -o server

server.o: server.c server.h settings.h game.h
	gcc -Wall server.c -c

settings.o: settings.c settings.h
	gcc -Wall settings.c -c
	
player.o: player.c player.h game.h settings.h
	gcc -Wall player.c -c
	
game.o: game.c game.h settings.h server.h
	gcc -Wall game.c -c

clean:
	rm -rf $(PROGRAM) $(OBJECTS)
