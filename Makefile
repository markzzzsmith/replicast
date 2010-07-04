#CFLAGS_DEBUG = -DLIBLOG_DEBUG_ALL
CFLAGS_DEBUG =

CFLAGS = -Wall $(CFLAGS_DEBUG)

replicast : log aip_pton global.h replicast.c
	clang $(CFLAGS) replicast.c -o replicast log.o aip_pton.o

log : log.h log.c
	clang $(CFLAGS) -c log.c -o log.o

aip_pton : aip_pton.h aip_pton.c
	clang $(CFLAGS) -c aip_pton.c -o aip_pton.o

clean :
	rm -f replicast log.o
