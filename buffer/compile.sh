#!/bin/sh


rm cb_buffer.o cb_encoding.o cb_search.o
#gcc -g -Wall -DTMP -I.:/usr/include:../include -L/usr/lib -c ../buffer/cb_buffer.c
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../buffer/cb_buffer.c
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../buffer/cb_encoding.c 
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../buffer/cb_search.c
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../buffer/cb_fifo.c

rm test_cb.o
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../buffer/test_cb.c 

rm test_cb
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib cb_search.o cb_encoding.o cb_buffer.o test_cb.o cb_fifo.o -o test_cb

rm get_option.o
cd ../get_option
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../get_option/get_option.c &&
mv get_option.o ../buffer/
cd ../buffer
rm cb_buffer.o
#gcc -g -Wall -DTMP -I.:/usr/include:../include -L/usr/lib -c ../buffer/cb_buffer.c
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../buffer/cb_buffer.c
rm test_cbsearch.o
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../buffer/test_cbsearch.c 

rm cbsearch
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib \
       cb_encoding.o cb_buffer.o get_option.o cb_search.o cb_fifo.o test_cbsearch.o -o cbsearch



cd tests
./compile.sh
cd ..

# ./test_cb 256 4096 ./tests/utf8_greek_2bytes.txt ./tests/utf8_greek_5bytes.txt
