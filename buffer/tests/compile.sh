#!/bin/sh

CC="/usr/bin/cc"
LD="/usr/bin/clang"

FOBJS="../cb_fifo.o ../cb_search.o ../cb_buffer.o ../cb_encoding.o ../cb_compare.o ../cb_log.o ../cb_read.o "

FLAGS=" -Weverything -g -O0 -I/usr/local/include -I. -I/usr/include -I../../include -I../../read "
LDFLAGS=" -Weverything -I/usr/local/include -I. -I/usr/include -I../../include -I../../read -L/usr/lib -L/usr/local/lib -lc -lpcre2-32 "

#echo "$CC $FLAGS -c test_regexp_search.c &&"
$CC $FLAGS -c test_regexp_search.c &&
$LD $LDFLAGS test_regexp_search.o $FOBJS -o test_regexp_search
rm test_regexp_search.o

$CC $FLAGS -c regexp_search.c &&
$LD $LDFLAGS regexp_search.o $FOBJS -o regexp_search
rm regexp_search.o

$CC $FLAGS -c test2_regexp_search.c &&
$LD -lpcre32 $LDFLAGS $FLAGS test2_regexp_search.o $FOBJS -o test2_regexp_search
rm test2_regexp_search.o

$CC -DPCRE2 $FLAGS -c test2_regexp_search.c &&
$LD -lpcre32 $LDFLAGS $FLAGS test2_regexp_search.o $FOBJS -o test2_regexp_search_pcre2
rm test2_regexp_search.o

#$CC $FLAGS -c toutf.c &&
#$LD $LDFLAGS toutf.o $FOBJS -o toutf8
#rm toutf.o

#$CC $FLAGS -c fromutf.c &&
#$LD $LDFLAGS fromutf.o $FOBJS -o fromutf8
#rm fromutf.o

#$CC $FLAGS -c test_bit_reversion.c &&
#$LD $LDFLAGS test_bit_reversion.o $FOBJS -o test_bit_reversion
#rm test_bit_reversion.o

#$CC $FLAGS -c test_byte_reversion.c &&
#$LD $LDFLAGS test_byte_reversion.o $FOBJS -o test_byte_reversion
#rm test_byte_reversion.o

$CC $FLAGS -c loop.c &&
$LD $LDFLAGS loop.o $FOBJS -o loop
rm loop.o

#$CC $FLAGS -c test_cbprint.c  &&
#$LD $LDFLAGS test_cbprint.o $FOBJS ../get_option.o -o cbprint
#rm test_cbprint.o

$CC $FLAGS -c test_cbfindall.c &&
$LD $LDFLAGS test_cbfindall.o $FOBJS -o cbfindall
rm test_cbfindall.o

#$CC $FLAGS -c test_cb_fifo.c &&
#$LD $LDFLAGS test_cb_fifo.o $FOBJS -o test_cbfifo
#rm test_cb_fifo.o
