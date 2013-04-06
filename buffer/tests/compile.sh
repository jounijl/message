#!/bin/sh

gcc -c toutf.c &&
gcc toutf.o ../cb_buffer.o ../cb_encoding.o -o toutf
rm toutf.o

gcc -c fromutf.c &&
gcc fromutf.o ../cb_buffer.o ../cb_encoding.o -o fromutf
rm fromutf.o
