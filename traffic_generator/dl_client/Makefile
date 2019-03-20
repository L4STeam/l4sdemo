CC = gcc
LDFLAGS = -lm -lpthread

all: dl_client

dl_client: dl_client.c Makefile
	$(CC)  dl_client.c $(LDFLAGS) -o $@

clean:
	rm -rf dl_client *.o
