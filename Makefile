CC=gcc
LIB_DIR=./lib
INC_DIR=./include
BIN_DIR=./bin
SRC_DIR=./src

all: fsync_client
	$(CC) -o client $(BIN_DIR)/fsync_client.o

fsync_client:
	$(CC) -c $(SRC_DIR)/fsync_client.c -o $(BIN_DIR)/fsync_client.o -Wall

clean:
	rm -rf $(LIB_DIR)/*.a $(BIN_DIR)/*.o $(SRC_DIR)/*~ $(INC_DIR)/*~ *~ client
