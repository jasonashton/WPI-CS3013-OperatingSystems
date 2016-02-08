all: readexample test_syscall1

readexample: readexample.o
	gcc -o readexample readexample.o

readexample.o: readexample.c
	gcc -c readexample.c

test_syscall1: test_syscall1.o
	gcc -o test_syscall1 test_syscall1.o

test_syscall1.o: test_syscall1.c
	gcc -c test_syscall1.c

clean:
	rm -rf readexample readexample.o
	rm -rf test_syscall1 test_syscall1.o
