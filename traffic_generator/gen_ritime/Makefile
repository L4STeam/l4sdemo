CC = gcc
LDFLAGS = -lm -lpthread

all: gen_ritime

gen_ritime: gen_ritime.c Makefile
	$(CC)  gen_ritime.c $(LDFLAGS) -o $@

clean:
	rm -rf gen_ritime *.o
