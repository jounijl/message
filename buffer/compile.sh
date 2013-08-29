#!/bin/sh

rm cb_buffer.o cb_encoding.o
#gcc -g -Wall -DTMP -I.:/usr/include:../include -L/usr/lib -c ../buffer/cb_buffer.c
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../buffer/cb_buffer.c
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../buffer/cb_encoding.c 

rm test_cb.o
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../buffer/test_cb.c 

rm test_cb
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib cb_encoding.o cb_buffer.o test_cb.o -o test_cb

rm get_option.o
cd ../get_option
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../get_option/get_option.c &&
mv get_option.o ../buffer/
cd ../buffer
rm cb_buffer.o
#gcc -g -Wall -DTMP -I.:/usr/include:../include -L/usr/lib -c ../buffer/cb_buffer.c
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../buffer/cb_buffer.c
rm cbsearch.o
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../buffer/cbsearch.c 

rm cbsearch
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib cb_encoding.o cb_buffer.o get_option.o cbsearch.o -o cbsearch


cd tests
./compile.sh
cd ..

# ./test_cb 256 4096 ./tests/utf8_greek_2bytes.txt ./tests/utf8_greek_5bytes.txt
