# Author:	Yeon Kim
# NetID:	ykim22

# Configuration

CC = gcc
CFLAGS = -g -std=gnu99 -Wall -Iinclude -lz -lcrypto

# Variables

CLIENT_SOURCES = src/udpclient.c
SERVER_SOURCES = src/udpserver.c

CLIENT = bin/client
SERVER = bin/server

# Rules

all:	$(CLIENT) $(SERVER)

$(CLIENT):	$(CLIENT_SOURCES)
	@echo "Compiling $@"
	@$(CC) $(CFLAGS) -o $@ $<

$(SERVER): $(SERVER_SOURCES)
	@echo "Compiling $@"
	@$(CC) $(CFLAGS) -o $@ $<

clean:
	@echo "Removing objects"
	@rm -f $(CLIENT) $(SERVER)