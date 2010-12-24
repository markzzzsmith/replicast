CC = clang
 
CFLAGS_DEBUG = -DLIBLOG_DEBUG_ALL
#CFLAGS_DEBUG =

CFLAGS = -Wall $(CFLAGS_DEBUG)

replicast : log inetaddr global.h replicast.c
	$(CC) $(CFLAGS) replicast.c -o replicast log.o inetaddr.o

log : log.h log.c
	$(CC) $(CFLAGS) -c log.c -o log.o

inetaddr : inetaddr.h inetaddr.c
	$(CC) $(CFLAGS) -c inetaddr.c -o inetaddr.o

clean :
	rm -f replicast log.o inetaddr.o
