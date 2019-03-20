CC = gcc
LDFLAGS = -lm -lpthread

all: http_clients_itime

http_clients_itime: http_clients_itime.c Makefile
	$(CC)  http_clients_itime.c $(LDFLAGS) -o $@

clean:
	rm -rf http_clients_itime *.o
