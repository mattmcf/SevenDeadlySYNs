test: test.c FileTable.o FileSystem.o Queue.o HashTable.o LinkedList.o ChunkyFile.o ColoredPrint.o
	gcc -std=c99 -g test.c FileTable.o HashTable.o FileSystem.o Queue.o ChunkyFile.o LinkedList.o ColoredPrint.o -o test

ColoredPrint.o: ../ColoredPrint/ColoredPrint.c ../ColoredPrint/ColoredPrint.h Queue.o
	gcc -Wall -std=c99 -g -c ../ColoredPrint/ColoredPrint.c -o ColoredPrint.o

ChunkyFile.o: ../ChunkyFile/ChunkyFile.c ../ChunkyFile/ChunkyFile.h
	gcc -Wall -std=c99 -g -c ../ChunkyFile/ChunkyFile.c -o ChunkyFile.o

FileSystem.o: ../FileSystem/FileSystem.c ../FileSystem/FileSystem.h
	gcc -Wall -std=c99 -g -c ../FileSystem/FileSystem.c -o FileSystem.o

HashTable.o: ../HashTable/HashTable.c ../HashTable/HashTable.h
	gcc -Wall -std=c99 -g -c ../HashTable/HashTable.c -o HashTable.o

Queue.o: ../Queue/Queue.c ../Queue/Queue.h
	gcc -Wall -std=c99 -g -std=c99 -c ../Queue/Queue.c -o Queue.o

LinkedList.o: ../LinkedList/LinkedList.c ../LinkedList/LinkedList.h
	gcc -Wall -std=c99 -g -std=c99 -c ../LinkedList/LinkedList.c -o LinkedList.o

clean:
	rm -rf test
	rm -rf FileTable.o
	rm -rf FileSystem.o
	rm -rf Queue.o
	rm -rf HashTable.o
	rm -rf LinkedList.o
	rm -rf ChunkyFile.o
	rm -rf ColoredPrint.o
	rm -rf *.dSYM
