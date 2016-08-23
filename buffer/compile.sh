#!/bin/sh

CC="/usr/bin/cc -std=c11 "
LD="/usr/bin/clang"

# clang -std=?? values: (8.1.2016)
# https://github.com/llvm-mirror/clang/blob/master/include/clang/Frontend/LangStandards.def
# - Because goto has been used across blocks, it is wise to choose C only: c11
# - Goto problem may cause the stack to grow to too big and finally preventing the
#   program to run after some count of loops. The maximum settings are important to 
#   prevent this.


#
# Library archive
#

LIBSRCS=" cb_transfer.c cb_buffer.c cb_compare.c ../read/cb_read.c ../read/cb_json.c ../read/cb_urlencode.c cb_encoding.c cb_search.c cb_fifo.c cb_log.c "
LIBOBJS=" cb_transfer.o cb_buffer.o cb_compare.o cb_read.o cb_json.o cb_urlencode.o cb_encoding.o cb_search.o cb_fifo.o cb_log.o "
LIBARCH="libcb.a"
FLAGS=" -g -Weverything -I/usr/local/include -I. -I/usr/include -I../include -I../read "
LDFLAGS=" -g -lc -I/usr/local/include -I. -I/usr/include -I../include -L/usr/lib -L/usr/local/lib -lpcre2-32 "

rm $LIBARCH

for I in $LIBSRCS
 do
   $CC $FLAGS -c $I
done

#$CC $FLAGS -DCBSETSTATETOPOLOGY -c cb_search.c -o cb_search_topo.o
#$CC $FLAGS -DCBSETSTATEFUL      -c cb_search.c -o cb_search_state.o
#$CC $FLAGS -DCBSETSTATETREE     -c cb_search.c -o cb_search_tree.o

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

