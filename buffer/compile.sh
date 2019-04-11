#!/bin/sh

#
# Accepts flag "development" if -g is needed
#

OSSYSTEM="$( uname )"
ALPINE="0"
MUSLOBJS=""
if [ $OSSYSTEM = "FreeBSD" ]
 then
        echo "Compiling in $OSSYSTEM"
        CC="/usr/bin/clang -std=c11 -fPIC "
        LD="/usr/bin/clang -v -std=c11 "
 elif [ $OSSYSTEM = "SunOS" ]
  then
        echo "Compiling in $OSSYSTEM"
        CC="/usr/bin/clang -std=c11 -fPIC "
        LD="/usr/bin/clang -v -std=c11 -lsocket -lnsl "
 elif [ $OSSYSTEM = "Linux" ]
  then
        echo -n "Compiling in $OSSYSTEM"
        if [ "$( uname -a | grep Alpine | wc -l )" = "1" ]
         then
                echo " Alpine" ; ALPINE="1"
                LD="/usr/bin/clang "
                LIBOBJS=" /usr/lib/libpcre2-32.a /usr/lib/libc.a "
                CC="/usr/bin/clang "
                ##LD="/usr/bin/ld -lc "
		##LIBOBJS=" /usr/lib/libpcre2-32.a /usr/lib/libc.a "
		##MUSLOBJS=" /usr/lib/crti.o /usr/lib/crtn.o /usr/lib/rcrt1.o "
	        ##CC="/usr/bin/clang -std=c11 -fPIC "
         else
                echo "Other Linux"
                LD="/usr/bin/clang -v -std=gnu17 "
	        CC="/usr/bin/clang -std=gnu17 -fPIC "
        fi
 else
  echo "Unknown operating system, exit."
  exit 1
fi


# 64-bit compile fixed with -fPIC (to be checked later)

# clang -std=?? values: (8.1.2016)
# https://github.com/llvm-mirror/clang/blob/master/include/clang/Frontend/LangStandards.def
# - goto has been used across blocks, C only: c11

#
# Library archive
#

LIBSRCS=" cb_endian.c cb_transfer.c cb_buffer.c cb_compare.c ../read/cb_read.c ../read/cb_json.c ../read/cb_urlencode.c cb_encoding.c cb_search.c cb_fifo.c cb_log.c "
LIBOBJS=" ${LIBOBJS} cb_endian.o cb_transfer.o cb_buffer.o cb_compare.o cb_read.o cb_json.o cb_urlencode.o cb_encoding.o cb_search.o cb_fifo.o cb_log.o "
LIBARCH="libcb.a"
if [ "$1"="development" ]
 then
   # test 26.10.2018: FLAGS=" -g -Weverything -I/usr/local/include -I. -I/usr/include -I../include -I../read "
   FLAGS=" -g -Wall -I/usr/local/include -I. -I/usr/include -I../include -I../read "
   LDFLAGS=" -g -lc -I/usr/local/include -I. -I/usr/include -I../include -L/usr/lib -L/usr/local/lib -lpcre2-32 "
 else
   FLAGS=" -O1 -I/usr/local/include -I. -I/usr/include -I../include -I../read "
   LDFLAGS=" -O1 -lc -I/usr/local/include -I. -I/usr/include -I../include -L/usr/lib -L/usr/local/lib -lpcre2-32 "
fi

rm $LIBARCH

for I in $LIBSRCS
 do
   echo "$CC $FLAGS -c $I"
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
# Shared library
#
$LD -shared $LDFLAGS -o ./libcb.so $LIBOBJS &&
/usr/bin/strip ./libcb.so

#echo
#echo "LIBRARY READY"
#echo

#
# Test programs
#
rm test_cb.o
$CC $FLAGS -c ../buffer/test_cb.c


rm test_cb
# jarjestyksella on valia
if [ "$ALPINE" = 1 ]
 then
	echo "$LD -v -static -lcb $LDFLAGS -L. test_cb.o ${LIBOBJS} ${MUSLOBJS} /usr/lib/libc.a /usr/lib/libpcre2-32.a $LIBARCH -o test_cb"
	$LD -v -static -lcb $LDFLAGS -L. test_cb.o ${LIBOBJS} ${MUSLOBJS} /usr/lib/libc.a /usr/lib/libpcre2-32.a $LIBARCH -o test_cb
 else
	echo "$LD $LDFLAGS -lcb -L. test_cb.o $LIBARCH -o test_cb"
	$LD $LDFLAGS -lcb -L. test_cb.o $LIBARCH -o test_cb
fi
#$LD $LDFLAGS -L. test_cb.o -lcb -o test_cb


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


#for I in CBSETSTATEFUL CBSETSTATELESS CBSETSTATETOPOLOGY CBSETSTATETREE
#  do
#   rm test_cbsearch.o
#   $CC $FLAGS -D$I -c ../buffer/test_cbsearch.c 
#   rm cb_buffer_$I.o
#   $CC $FLAGS -D$I -c ../buffer/cb_buffer.c -o ../buffer/cb_buffer_$I.o
#   rm cbsearch.$I
#   $LD $LDFLAGS test_cbsearch.o cb_buffer_$I.o get_option.o $LIBARCH -o cbsearch.$I
# done
#rm cbsearch
# CBSETSTATETREE:
$LD $LDFLAGS test_cbsearch.o get_option.o ${MUSLOBJS} $LIBARCH -o cbsearch 
#rm cbconv
$LD $LDFLAGS test_cbconv.o get_option.o ${MUSLOBJS} $LIBARCH -o cbconv

#rm test_cbfile.o
#$CC $FLAGS -c ../buffer/test_cbfile.c
#rm test_cbfile
#$LD $LDFLAGS -L. get_option.o test_cbfile.o ${MUSLOBJS} $LIBARCH -o test_cbfile


#
# Utility and test programs
# 

echo "tests directory"

cd tests
./compile.sh
cd ..

