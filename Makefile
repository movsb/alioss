include crypto/Makefile

# flags
CC = gcc
CFLAGS = -std=c99 -c

CPP = g++
CXXFLAGS = -std=c++11 -c -g

# all

.PHONY: all
all: main
	@echo Done!

# make

http.o: http.cpp http.h
	$(CPP) $(CXXFLAGS) -o $@ $<

main.o: main.cpp http.h
	$(CPP) $(CXXFLAGS) -o $@ $<

time.o: time.cpp time.h
	$(CPP) $(CXXFLAGS) -o $@ $<

OBJS = http.o main.o time.o
LIBS = crypto/crypto.a

main: $(OBJS)
	$(CPP) -o $@ $(OBJS) $(LIBS)



# clean

.PHONY:clean clean_objs clean_main
clean: clean_objs clean_main

clean_objs:
	rm -f $(OBJS)

clean_main:
	rm -f main

