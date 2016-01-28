all: runCommand shell shell2

runCommand: runCommand.o
	gcc -g runCommand.o --std=gnu89 -o runCommand

runCommand.o: runCommand.c
	gcc -g --std=gnu89 -c runCommand.c

shell: shell.o
	gcc -g shell.o -o shell

shell.o: shell.c
	gcc -g --std=gnu89 -c shell.c

shell2: shell2.o
	gcc -g shell2.o -o shell2

shell2.o: shell2.c
	gcc -g --std=gnu89 -c shell2.c

clean:
	rm -f runCommand.o runCommand
	rm -f shell.o shell
	rm -f shell2.o shell2
