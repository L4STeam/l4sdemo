CC = gcc
LDFLAGS = -lpthread

all: dl_server

dl_server: dl_server.c Makefile
	$(CC) dl_server.c $(LDFLAGS) -O3 -o $@

clean:
	rm -rf dl_server *.o
