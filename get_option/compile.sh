#!/sbin/sh

PROG="getoption"
OPROG="main.o"

clear

rm ./$PROG ./$OPROG get_option.o

/usr/bin/gcc -Wall -c get_option.c  &&
/usr/bin/gcc -Wall -c main.c &&
/usr/bin/gcc -Wall main.o get_option.o -o $PROG &&
chmod 700 ./$PROG 

