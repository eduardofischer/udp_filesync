CC=gcc
INC_DIR=./include
BIN_DIR=./bin
SRC_DIR=./src
CFLAGS=-Wall

SOURCES=$(SRC_DIR)/communication.c

all: client server
		
client: $(SRC_DIR)/client.c $(SOURCES)
	$(CC) $(CFLAGS) -o client $(SRC_DIR)/client.c $(SOURCES)

server: $(SRC_DIR)/server.c $(SOURCES)
	$(CC) $(CFLAGS) -o server $(SRC_DIR)/server.c $(SOURCES)

clean:
	rm -rf $(BIN_DIR)/*.o $(SRC_DIR)/*~ $(INC_DIR)/*~ *~ client server
