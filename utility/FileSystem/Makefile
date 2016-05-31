test: test.c FileSystem.o
	gcc -Wall -g -std=c99 test.c Queue.o FileSystem.o -o test

FileSystem.o: FileSystem.c FileSystem.h Queue.o
	cd ../FileSystem
	make FileSystem.o

Queue.o: ../Queue/Queue.c ../Queue/Queue.h
	gcc -Wall -g -std=c99 -c ../Queue/Queue.c ../Queue/Queue.o

clean:
	rm -rf test
	rm -rf FileSystem.o
	rm -rf Queue.o
	rm -rf *.dSYM
