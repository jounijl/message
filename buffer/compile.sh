#!/bin/sh

#
# Library archive
#

LIBSRCS=" cb_buffer.c cb_encoding.c cb_search.c cb_fifo.c "
LIBOBJS=" cb_buffer.o cb_encoding.o cb_search.o cb_search_topo.o cb_search_state.o cb_fifo.o "
LIBARCH="libcb.a"

rm $LIBARCH

for I in $LIBSRCS
 do
   gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c $I
done

gcc -g -Wall -DCBSTATETOPOLOGY -I.:/usr/include:../include -L/usr/lib -c cb_search.c -o cb_search_topo.o
gcc -g -Wall -DCBSTATEFUL      -I.:/usr/include:../include -L/usr/lib -c cb_search.c -o cb_search_state.o

ar -rcs $LIBARCH $LIBOBJS

if [ ! -f "./$LIBARCH" ]
 then
   echo "Could not make libraryfile."; exit 1;
fi

#
# Test programs
#
rm test_cb.o
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../buffer/test_cb.c
rm test_cb
# jarjestyksella on valia
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -L. test_cb.o $LIBARCH -o test_cb

#
# Programs
#
rm get_option.o
cd ../get_option
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../get_option/get_option.c &&
mv get_option.o ../buffer/
cd ../buffer
rm test_cbsearch.o
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../buffer/test_cbsearch.c 
rm test_cbconv.o
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../buffer/test_cbconv.c 
rm cbsearch
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib test_cbsearch.o get_option.o $LIBARCH -o cbsearch
rm cbconv
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib test_cbconv.o get_option.o $LIBARCH -o cbconv

#
# Utility and test programs
# 

cd tests
./compile.sh
cd ..


