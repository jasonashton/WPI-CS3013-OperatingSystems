all: memhandler-random memhandler-fifo memhandler-lru

memhandler-random: memhandler-random.c
	gcc -Wall -g -o memhandler-random memhandler-random.c -pthread

memhandler-fifo: memhandler-fifo.c
	gcc -Wall -g -o memhandler-fifo memhandler-fifo.c -pthread

memhandler-lru: memhandler-lru.c
	gcc -Wall -g -o memhandler-lru memhandler-lru.c -pthread

clean:
	rm -rf memhandler-random.o memhandler-random
	rm -rf memhandler-fifo.o memhandler-fifo
	rm -rf memhandler-lru.o memhandler-lru
