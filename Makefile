#CFLAGS_DEBUG = -DLIBLOG_DEBUG_ALL
CFLAGS_DEBUG =

CFLAGS = -Wall $(CFLAGS_DEBUG)

replicast : log aip_ptoh global.h replicast.c
	clang $(CFLAGS) replicast.c -o replicast log.o aip_ptoh.o

log : log.h log.c
	clang $(CFLAGS) -c log.c -o log.o

aip_ptoh : aip_ptoh.h aip_ptoh.c
	clang $(CFLAGS) -c aip_ptoh.c -o aip_ptoh.o

clean :
	rm -f replicast log.o aip_ptoh.o
