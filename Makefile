

replicast : global.h replicast.c
	gcc -Wall -o replicast replicast.c

clean :
	rm -f replicast
