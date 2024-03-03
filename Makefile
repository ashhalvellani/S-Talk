all: main

main:
	gcc -Wall -Werror main.c list.c -o s-talk -lpthread
	
clean:
	rm -f s-talk