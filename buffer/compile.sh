#!/bin/sh

CC="/usr/bin/cc"
#LD="/usr/bin/ld"
LD="/usr/bin/clang"

#
# Library archive
#

LIBSRCS=" cb_buffer.c cb_compare.c ../read/cb_read.c cb_encoding.c cb_search.c cb_fifo.c"
LIBOBJS=" cb_buffer.o cb_compare.o cb_read.o cb_encoding.o cb_search_topo.o cb_search.o cb_search_state.o cb_search_tree.o cb_fifo.o "
LIBARCH="libcb.a"
#FLAGS=" -g -Wall -I.:/usr/include:../include:/usr/local/include -L/usr/lib:/usr/local/lib -lpcre -lpcre16 -lpcre32 "
# oli ennen 27.11.2014:
#FLAGS=" -g -Wall -I~/Documents/Source/message/pcre/include -I.:/usr/include:../include -L~/Documents/Source/message/pcre -L/usr/lib -lpcre32 "
# mika PCRE:ssa oli erikoista, saako UTF:n paalle ohjelmallisesti muutoinkin, ts. kirjastot loytyvat ilman asetuksia?
FLAGS=" -g -Weverything -I/usr/local/include -I. -I/usr/include -I../include "
LDFLAGS=" -lc -I/usr/local/include -I. -I/usr/include -I../include -L/usr/lib -L/usr/local/lib -lpcre32 "

rm $LIBARCH

for I in $LIBSRCS
 do
   $CC $FLAGS -c $I
done

$CC $FLAGS -DCBSETSTATETOPOLOGY -c cb_search.c -o cb_search_topo.o
$CC $FLAGS -DCBSETSTATEFUL      -c cb_search.c -o cb_search_state.o
$CC $FLAGS -DCBSETSTATETREE     -c cb_search.c -o cb_search_tree.o

ar -rcs $LIBARCH $LIBOBJS

if [ ! -f "./$LIBARCH" ]
 then
   echo "Could not make libraryfile."; exit 1;
fi

#
# Test programs
#
rm test_cb.o
$CC $FLAGS -c ../buffer/test_cb.c
rm test_cb
# jarjestyksella on valia
$LD $LDFLAGS -L. test_cb.o $LIBARCH -o test_cb


#
# Programs
#
rm get_option.o
cd ../get_option
$CC $FLAGS -c ../get_option/get_option.c &&
mv get_option.o ../buffer/
cd ../buffer
rm test_cbconv.o
$CC $FLAGS -c ../buffer/test_cbconv.c 

for I in CBSETSTATEFUL CBSETSTATELESS CBSETSTATETOPOLOGY CBSETSTATETREE
  do
   rm test_cbsearch.o
   $CC $FLAGS -D$I -c ../buffer/test_cbsearch.c 
   rm cb_buffer_$I.o
   $CC $FLAGS -D$I -c ../buffer/cb_buffer.c -o ../buffer/cb_buffer_$I.o
   rm cbsearch.$I
   $LD $LDFLAGS test_cbsearch.o cb_buffer_$I.o get_option.o $LIBARCH -o cbsearch.$I
 done
rm cbsearch
# CBSETSTATETREE:
$LD $LDFLAGS test_cbsearch.o get_option.o $LIBARCH -o cbsearch 
rm cbconv
$LD $LDFLAGS test_cbconv.o get_option.o $LIBARCH -o cbconv
rm test_cbfile.o
$CC $FLAGS -c ../buffer/test_cbfile.c
rm test_cbfile
$LD $LDFLAGS -L. get_option.o test_cbfile.o $LIBARCH -o test_cbfile

#
# Utility and test programs
# 

cd tests
./compile.sh
cd ..

