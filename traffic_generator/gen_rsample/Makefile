CC = gcc
LDFLAGS = -lpthread

all: gen_rsamples

gen_rsamples: gen_rsamples.c Makefile
	$(CC) gen_rsamples.c $(LDFLAGS) -O3 -o $@

clean:
	rm -rf gen_rsamples *.o
