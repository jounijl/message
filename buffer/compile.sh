#!/bin/sh

rm cb_buffer.o cb_encoding.o
gcc -g -Wall -O1 -I.:/usr/include:../include -L/usr/lib -c ../buffer/cb_buffer.c
gcc -g -Wall -O1 -I.:/usr/include:../include -L/usr/lib -c ../buffer/cb_encoding.c 

rm main.o
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../buffer/main.c 

rm test_cb
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib cb_encoding.o cb_buffer.o main.o -o test_cb

cd tests
./compile.sh
cd ..

# ./test_cb 256 4096 ./tests/utf8_greek_2bytes.txt ./tests/utf8_greek_5bytes.txt