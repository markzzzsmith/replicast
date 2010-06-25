CFLAGS_DEBUG = -DLIBLOG_DEBUG_ALL
#CFLAGS_DEBUG =

CFLAGS = -Wall $(CFLAGS_DEBUG)

replicast : log global.h replicast.c
	gcc $(CFLAGS) replicast.c -o replicast log.o

log : log.h log.c
	gcc $(CFLAGS) -c log.c -o log.o

clean :
	rm -f replicast log.o
