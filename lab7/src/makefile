CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: tcpclient tcpserver udpclient udpserver

tcpclient: tcpclient.c
	$(CC) $(CFLAGS) -o tcpclient tcpclient.c

tcpserver: tcpserver.c
	$(CC) $(CFLAGS) -o tcpserver tcpserver.c

udpclient: udpclient.c
	$(CC) $(CFLAGS) -o udpclient udpclient.c

udpserver: udpserver.c
	$(CC) $(CFLAGS) -o udpserver udpserver.c

clean:
	rm -f tcpclient tcpserver udpclient udpserver

.PHONY: all clean