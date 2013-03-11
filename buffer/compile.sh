#!/bin/sh

gcc -g -Wall -O1 -I.:/usr/include:../include -L/usr/lib -c ../buffer/cb_buffer.c
gcc -g -Wall -O1 -I.:/usr/include:../include -L/usr/lib -c ../buffer/cb_encoding.c

gcc -g -Wall -O1 -I.:/usr/include:../include -L/usr/lib -c ../buffer/main.c

gcc -g -Wall -O1 -I.:/usr/include:../include -L/usr/lib cb_encoding.o cb_buffer.o main.o -o test_cb
