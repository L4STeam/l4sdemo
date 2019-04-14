# run this like this:
# CPATH=/path/to/aqmt/common make

SRC=analyzer.cpp
HEADERS=analyzer.h

CPP=g++
AR=ar

all: analyzer

libta: $(SRC) $(HEADERS) Makefile
	$(CPP) -c $(SRC) -std=c++11 -O3 -o libta.o
	$(AR) rcs libta.a libta.o

analyzer: main.cpp $(HEADERS) Makefile libta
	$(CPP) main.cpp -L. -lta -std=c++11 -lpcap -pthread -O3 -o $@

clean:
	rm -rf analyzer *.a *.o
