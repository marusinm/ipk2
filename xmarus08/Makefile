CC=g++
RM=rm -vf
CPPFLAGS=-std=c++11 -Wall -pedantic -Wextra

.PHONY: all clean

all: traceroute.cpp
	${CC} ${CPPFLAGS} traceroute.cpp -o trace

clean:
	${RM} *.o
