
CC = g++
CFLAGS = -g -Wall -std=c++11


all: serverM serverS serverD serverU client

server%: server%.o server_utils.o
	$(CC) $(CFLAGS) -o $@ $^

server%.o: server%.cpp server_utils.h
	$(CC) $(CFLAGS) -c $<

server_utils.o: server_utils.cpp server_utils.h
	$(CC) $(CFLAGS) -c $<

client.o: client.cpp server_utils.h
	$(CC) $(CFLAGS) -c $<

client: client.o server_utils.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f serverM serverS serverD serverU client *.o