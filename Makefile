CC = clang
 
#CFLAGS_DEBUG = -DLIBLOG_DEBUG_ALL
CFLAGS_DEBUG =

CFLAGS = -Wall $(CFLAGS_DEBUG)

replicast : log aip_ptox global.h replicast.c
	$(CC) $(CFLAGS) replicast.c -o replicast log.o aip_ptox.o

log : log.h log.c
	$(CC) $(CFLAGS) -c log.c -o log.o

aip_ptox : aip_ptox.h aip_ptox.c
	$(CC) $(CFLAGS) -c aip_ptox.c -o aip_ptox.o

clean :
	rm -f replicast log.o aip_ptox.o
