#!/bin/sh

#
# Library archive
#

LIBSRCS=" cb_buffer.c cb_compare.c ../read/cb_read.c cb_encoding.c cb_search.c cb_fifo.c"
LIBOBJS=" cb_buffer.o cb_compare.o cb_read.o cb_encoding.o cb_search_topo.o cb_search.o cb_search_state.o cb_search_tree.o cb_fifo.o "
LIBARCH="libcb.a"

rm $LIBARCH

for I in $LIBSRCS
 do
   gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c $I
done

gcc -g -Wall -DCBSETSTATETOPOLOGY -I.:/usr/include:../include -L/usr/lib -c cb_search.c -o cb_search_topo.o
gcc -g -Wall -DCBSETSTATEFUL      -I.:/usr/include:../include -L/usr/lib -c cb_search.c -o cb_search_state.o
gcc -g -Wall -DCBSETSTATETREE     -I.:/usr/include:../include -L/usr/lib -c cb_search.c -o cb_search_tree.o

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
rm test_cbconv.o
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../buffer/test_cbconv.c 

for I in CBSETSTATEFUL CBSETSTATELESS CBSETSTATETOPOLOGY CBSETSTATETREE
  do
   rm test_cbsearch.o
   gcc -g -Wall -D$I -I.:/usr/include:../include -L/usr/lib -c ../buffer/test_cbsearch.c 
   rm cb_buffer_$I.o
   gcc -g -Wall -D$I -I.:/usr/include:../include -L/usr/lib -c ../buffer/cb_buffer.c -o ../buffer/cb_buffer_$I.o
   rm cbsearch.$I
   gcc -g -Wall -D$I -I.:/usr/include:../include -L/usr/lib test_cbsearch.o cb_buffer_$I.o get_option.o $LIBARCH -o cbsearch.$I
 done
rm cbsearch
#gcc -g -Wall -DCBSETSTATETOPOLOGY -I.:/usr/include:../include -L/usr/lib test_cbsearch.o get_option.o $LIBARCH -o cbsearch
#gcc -g -Wall -DCBSETSTATELESS -I.:/usr/include:../include -L/usr/lib test_cbsearch.o get_option.o $LIBARCH -o cbsearch
gcc -g -Wall -DCBSETSTATETREE -I.:/usr/include:../include -L/usr/lib test_cbsearch.o get_option.o $LIBARCH -o cbsearch
rm cbconv
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib test_cbconv.o get_option.o $LIBARCH -o cbconv
rm test_cbfile.o
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -c ../buffer/test_cbfile.c
rm test_cbfile
gcc -g -Wall -I.:/usr/include:../include -L/usr/lib -L. get_option.o test_cbfile.o $LIBARCH -o test_cbfile

#
# Utility and test programs
# 

cd tests
./compile.sh
cd ..


