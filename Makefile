CC = clang
#CC =gcc
 
#CFLAGS_DEBUG = -DLIBLOG_DEBUG_ALL
CFLAGS_DEBUG =

CFLAGS = -Wall $(CFLAGS_DEBUG)

replicast : log inetaddr stringnz replicast.c
	$(CC) $(CFLAGS) replicast.c -o replicast log.o inetaddr.o stringnz.o

log : log.h log.c
	$(CC) $(CFLAGS) -c log.c -o log.o

inetaddr : inetaddr.h inetaddr.c
	$(CC) $(CFLAGS) -c inetaddr.c -o inetaddr.o

stringnz : stringnz.h stringnz.c
	$(CC) $(CFLAGS) -c stringnz.c -o stringnz.o

clean :
	rm -f replicast log.o inetaddr.o stringnz.o
