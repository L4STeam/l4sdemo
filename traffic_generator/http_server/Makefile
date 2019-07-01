CC = gcc
LDFLAGS = -lpthread

all: run_httpserver

run_httpserver: run_httpserver.c Makefile
	$(CC) run_httpserver.c -g $(LDFLAGS) -O3 -o $@

clean:
	rm -rf run_httpserver *.o
