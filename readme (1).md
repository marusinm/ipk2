Traceroute
===========================================================
Autor: Marušin Marek, xmarus08
Datum: 23.04.2017

## SYNOPSIS

trace [-f first_ttl] [-m max_ttl] <ip-address>

## DESCRIPTION

Traceroute is a utility that records the route (the specific gateway computers at each hop) through the Internet between your computer and a specified destination computer. It also calculates and displays the amount of time each hop took. Traceroute is a handy tool both for understanding where problems are in the Internet network and for getting a detailed sense of the Internet itself. Another utility, PING, is often used prior to using traceroute to see whether a host is present on the network.

## Makefile:
  $ make           - compile the program
  $ make clean     - remove compiled program

## FILES    

  Makefile         
  readme.md - documentation
  traceroute.cpp - source code

## BUGS AND PRESONAL VIEW

..zatial nic mozno doplnit

## SOURCES

References from which I came:
- https://www.tutorialspoint.com/cplusplus/
- http://www.haifux.org/lectures/217/netLec5.pdf
- http://stackoverflow.com/

- https://tools.ietf.org/html/rfc7231
- http://liw.fi/manpages/
- https://guides.github.com/features/mastering-markdown/
- http://www.tldp.org/HOWTO/Man-Page/q3.html
- IPK WIKI