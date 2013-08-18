#!/bin/sh

gcc -c toutf.c &&
gcc toutf.o ../cb_buffer.o ../cb_encoding.o -o toutf
rm toutf.o

gcc -c fromutf.c &&
gcc fromutf.o ../cb_buffer.o ../cb_encoding.o -o fromutf
rm fromutf.o

gcc -c test_bit_reversion.c &&
gcc test_bit_reversion.o ../cb_buffer.o ../cb_encoding.o -o test_bit_reversion
rm test_bit_reversion.o

gcc -c test_byte_reversion.c &&
gcc test_byte_reversion.o ../cb_buffer.o ../cb_encoding.o -o test_byte_reversion
rm test_byte_reversion.o

gcc -c loop.c &&
gcc loop.o ../cb_buffer.o ../cb_encoding.o -o loop
rm loop.o

