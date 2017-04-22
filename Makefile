CC=g++
RM=rm -vf
CPPFLAGS=-std=c++11 -Wall -pedantic -Wextra

.PHONY: all clean

all: traceroute

client: traceroute.cpp
	${CC} ${CPPFLAGS} traceroute.cpp -o traceroute

clean:
	${RM} *.o
