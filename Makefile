CC = gcc
CFLAGS = -g

.PHONY: clean 

.SUFFIXES: .c

CLIENT_DIR = client/ 
COMMON_DIR = common/ 
TRACKER_DIR = tracker/ 

UTILITY_DIR = utility/
UTILITY_FILES = $(UTILITY_DIR)/AsyncQueue/AsyncQueue.h $(UTILITY_DIR)/AsyncQueue/AsyncQueue.c \
	$(UTILITY_DIR)/FileSystem/FileSystem.h $(UTILITY_DIR)/FileSystem/FileSystem.c \
	$(UTILITY_DIR)/HashTable/HashTable.h $(UTILITY_DIR)/HashTable/HashTable.c \
	$(UTILITY_DIR)/LinkedList/LinkedList.h $(UTILITY_DIR)/LinkedList/LinkedList.c \
	$(UTILITY_DIR)/Queue/Queue.h $(UTILITY_DIR)/Queue/Queue.c \
	$(UTILITY_DIR)/SDSet/SDSet.h $(UTILITY_DIR)/SDSet/SDSet.c 

UTILITY_OBJS = $(UTILITY_FILES:.c=.0)

HEADER_FILES = $(COMMON_DIR)constant.h $(COMMON_DIR)packets.h $(COMMON_DIR)peer_table.h 
SRC_FILES = 

OBJ_FILES = $(SRC_FILES:.c=.o)

EXECUTABLES = $(TRACKER_DIR)tracker $(CLIENT_DIR)client

all: EXECUTABLES

$(TRACKER_DIR)tracker: UTILITY_OBJS

$(CLIENT_DIR)client: UTILITY_OBJS

UTILITY_OBJS

