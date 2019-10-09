CC=gcc
INC_DIR=./include
BIN_DIR=./bin
SRC_DIR=./src
CFLAGS=-Wall -lm -lreadline -pthread

SOURCES=$(SRC_DIR)/communication.c $(SRC_DIR)/filesystem.c $(SRC_DIR)/aux.c

all: client server
		
client: $(SRC_DIR)/client.c $(SOURCES)
	$(CC) -o client $(SRC_DIR)/client.c $(CFLAGS) $(SOURCES)

server: $(SRC_DIR)/server.c $(SOURCES)
	$(CC) -o server $(SRC_DIR)/server.c $(CFLAGS) $(SOURCES)

clean:
	rm -rf $(BIN_DIR)/*.o $(SRC_DIR)/*~ $(INC_DIR)/*~ *~ client server
