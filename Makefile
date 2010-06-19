

mcastrep : global.h mcastrep.c
	gcc -Wall -o mcastrep mcastrep.c

clean :
	rm -f mcastrep
