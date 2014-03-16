#!/bin/sh

GCC="/usr/local/bin/gcc"

#
# Library archive
#

LIBSRCS=" cb_buffer.c cb_compare.c ../read/cb_read.c cb_encoding.c cb_search.c cb_fifo.c"
LIBOBJS=" cb_buffer.o cb_compare.o cb_read.o cb_encoding.o cb_search_topo.o cb_search.o cb_search_state.o cb_search_tree.o cb_fifo.o "
LIBARCH="libcb.a"
FLAGS=" -g -Wall -I.:/usr/include:../include:/usr/local/include -L/usr/lib:/usr/local/lib -lpcre -lpcre16 -lpcre32 "

rm $LIBARCH

for I in $LIBSRCS
 do
   $GCC $FLAGS -c $I
done

$GCC $FLAGS -DCBSETSTATETOPOLOGY -c cb_search.c -o cb_search_topo.o
$GCC $FLAGS -DCBSETSTATEFUL      -c cb_search.c -o cb_search_state.o
$GCC $FLAGS -DCBSETSTATETREE     -c cb_search.c -o cb_search_tree.o

ar -rcs $LIBARCH $LIBOBJS

if [ ! -f "./$LIBARCH" ]
 then
   echo "Could not make libraryfile."; exit 1;
fi

#
# Test programs
#
rm test_cb.o
$GCC $FLAGS -c ../buffer/test_cb.c
rm test_cb
# jarjestyksella on valia
$GCC $FLAGS -L. test_cb.o $LIBARCH -o test_cb


#
# Programs
#
rm get_option.o
cd ../get_option
$GCC $FLAGS -c ../get_option/get_option.c &&
mv get_option.o ../buffer/
cd ../buffer
rm test_cbconv.o
$GCC $FLAGS -c ../buffer/test_cbconv.c 

for I in CBSETSTATEFUL CBSETSTATELESS CBSETSTATETOPOLOGY CBSETSTATETREE
  do
   rm test_cbsearch.o
   $GCC $FLAGS -D$I -c ../buffer/test_cbsearch.c 
   rm cb_buffer_$I.o
   $GCC $FLAGS -D$I -c ../buffer/cb_buffer.c -o ../buffer/cb_buffer_$I.o
   rm cbsearch.$I
   $GCC $FLAGS -D$I test_cbsearch.o cb_buffer_$I.o get_option.o $LIBARCH -o cbsearch.$I
 done
rm cbsearch
$GCC $FLAGS -DCBSETSTATETREE test_cbsearch.o get_option.o $LIBARCH -o cbsearch
rm cbconv
$GCC $FLAGS test_cbconv.o get_option.o $LIBARCH -o cbconv
rm test_cbfile.o
$GCC $FLAGS -c ../buffer/test_cbfile.c
rm test_cbfile
$GCC $FLAGS -L. get_option.o test_cbfile.o $LIBARCH -o test_cbfile

#
# Utility and test programs
# 

cd tests
./compile.sh
cd ..

