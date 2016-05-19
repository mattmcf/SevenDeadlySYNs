# UTILITY_DIR = utility/
# UTILITY_FILES = $(UTILITY_DIR)/AsyncQueue/AsyncQueue.h $(UTILITY_DIR)/AsyncQueue/AsyncQueue.c \
# 	$(UTILITY_DIR)/FileSystem/FileSystem.h $(UTILITY_DIR)/FileSystem/FileSystem.c \
# 	$(UTILITY_DIR)/HashTable/HashTable.h $(UTILITY_DIR)/HashTable/HashTable.c \
# 	$(UTILITY_DIR)/LinkedList/LinkedList.h $(UTILITY_DIR)/LinkedList/LinkedList.c \
# 	$(UTILITY_DIR)/Queue/Queue.h $(UTILITY_DIR)/Queue/Queue.c \
# 	$(UTILITY_DIR)/SDSet/SDSet.h $(UTILITY_DIR)/SDSet/SDSet.c 
# UTILITY_OBJS = $(UTILITY_FILES:.c=.0)

HEADER_FILES = common/constant.h 

OBJ_FILES = utility/HashTable/HashTable.o utility/LinkedList/LinkedList.o utility/AsyncQueue/asyncqueue.o \
	utility/Queue/queue.o utility/ChunkyFile/ChunkyFile.o utility/FileSystem/FileSystem.o utility/SDSet/SDSet.o \
	common/peer_table.o

all: tracker/tracker_app client/client_app


tracker/tracker_app: tracker/tracker.c tracker/tracker.h tracker/network_tracker.o $(OBJ_FILES) $(HEADER_FILES)
	gcc -Wall -pedantic -std=c99 -g tracker/tracker.c tracker/network_tracker.o $(OBJ_FILES) -o tracker/tracker_app

client/client_app: client/client.c client/client.h client/network_client.o $(OBJ_FILES) $(HEADER_FILES)
	gcc -Wall -pedantic -std=c99 -g client/client.c client/network_client.o $(OBJ_FILES) -o client/client_app

tracker/network_tracker.o: tracker/network_tracker.c tracker/network_tracker.h $(OBJ_FILES) $(HEADER_FILES)
	gcc -pthread -Wall -pedantic -std=c99 -g -c tracker/network_tracker.c utility/FileSystem/FileSystem.o -o tracker/network_tracker.o

client/network_client.o: client/network_client.c client/network_client.h $(OBJ_FILES) $(HEADER_FILES)
	gcc -pthread -Wall -pedantic -std=c99 -g -c client/network_client.c utility/FileSystem/FileSystem.o utility/AsyncQueue/asyncqueue.o -o client/network_client.o

common/peer_table.o: common/peer_table.h common/peer_table.c $(HEADER_FILES)
	gcc -Wall -pedantic -std=c99 -g -c common/peer_table.c -o common/peer_table.o

utility/ChunkyFile/ChunkyFile.o: utility/ChunkyFile/ChunkyFile.c utility/ChunkyFile/ChunkyFile.h
	gcc -Wall -pedantic -std=c99 -g -c utility/ChunkyFile/ChunkyFile.c -o utility/ChunkyFile/ChunkyFile.o

utility/FileSystem/FileSystem.o: utility/FileSystem/FileSystem.c utility/FileSystem/FileSystem.h
	gcc -Wall -pedantic -std=c99 -g -c utility/FileSystem/FileSystem.c -o utility/FileSystem/FileSystem.o

utility/HashTable/HashTable.o: utility/HashTable/HashTable.c utility/HashTable/HashTable.h utility/LinkedList/LinkedList.o
	gcc -Wall -pedantic -std=c99 -g -c utility/HashTable/HashTable.c -o utility/HashTable/HashTable.o 

utility/LinkedList/LinkedList.o: utility/LinkedList/LinkedList.c utility/LinkedList/LinkedList.h
	gcc -Wall -pedantic -std=c99 -g -c utility/LinkedList/LinkedList.c -o utility/LinkedList/LinkedList.o 

utility/AsyncQueue/asyncqueue.o: utility/AsyncQueue/AsyncQueue.c utility/AsyncQueue/AsyncQueue.h utility/Queue/queue.o
	gcc -pthread -g -c utility/AsyncQueue/AsyncQueue.c -o utility/AsyncQueue/asyncqueue.o

utility/Queue/queue.o: utility/Queue/Queue.c utility/Queue/Queue.h
	gcc -Wall -pedantic -std=c99 -g -c utility/Queue/Queue.c -o utility/Queue/queue.o

utility/SDSet/SDSet.o: utility/SDSet/SDSet.c utility/SDSet/SDSet.h
	gcc -Wall -pedantic -std=c99 -g -c utility/SDSet/SDSet.c -o utility/SDSet/SDSet.o

clean:
	rm -rf $(OBJ_FILES)
	rm -rf tracker/network_tracker.o
	rm -rf client/network_client.o
	rm -rf client/client_app
	rm -rf tracker/tracker_app

