CC=gcc
INC_DIR=./include
BIN_DIR=./bin
SRC_DIR=./src
CFLAGS=-Wall -DPORT=3000

SOURCES=$(SRC_DIR)/socket.c

all: fsync_client fsync_server
		
fsync_client:
	$(CC) $(CFLAGS) -o client $(SRC_DIR)/fsync_client.c $(SOURCES)

fsync_server:
	$(CC) $(CFLAGS) -o server $(SRC_DIR)/fsync_server.c $(SOURCES)

clean:
	rm -rf $(BIN_DIR)/*.o $(SRC_DIR)/*~ $(INC_DIR)/*~ *~ client server
