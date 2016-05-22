test: test.c FileSystem.o
	gcc -Wall -g -std=c99 test.c Queue.o FileSystem.o -o test

FileSystem.o: FileSystem.c FileSystem.h Queue.o
	gcc -Wall -g -std=c99 -c FileSystem.c Queue.o

Queue.o: ../Queue/Queue.c ../Queue/Queue.h
	gcc -Wall -g -std=c99 -c ../Queue/Queue.c

clean:
	rm -rf test
	rm -rf FileSystem.o
	rm -rf Queue.o
	rm -rf *.dSYM
