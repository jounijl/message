#!/bin/sh

CC="/usr/bin/cc"
#LD="/usr/bin/ld"
LD="/usr/bin/clang"

#FLAGS=" -g -Wall -I/home/cerro/Documents/Source/message/pcre/include:.:/usr/include:../include"
#FLAGS=" -g -O0 -Weverything -I/usr/local/include -I. -I/usr/include -I../include "
FLAGS=" -g -O0 -I/usr/local/include -I. -I/usr/include -I../include "
LDFLAGS=" -I/usr/local/include -I. -I/usr/include -I../include -L/usr/lib -L/usr/local/lib -lc -lpcre32 "

echo "$CC $FLAGS -c test_regexp_search.c &&"
$CC $FLAGS -c test_regexp_search.c &&
echo "$LD $LDFLAGS test_regexp_search.o ../cb_fifo.o ../cb_search.o ../cb_buffer.o ../cb_encoding.o ../cb_compare.o -o test_regexp_search"
$LD $LDFLAGS test_regexp_search.o ../cb_fifo.o ../cb_search.o ../cb_buffer.o ../cb_encoding.o ../cb_compare.o -o test_regexp_search
rm test_regexp_search.o

$CC $FLAGS -c regexp_search.c &&
$LD $LDFLAGS regexp_search.o ../cb_fifo.o ../cb_search.o ../cb_buffer.o ../cb_encoding.o ../cb_compare.o -o regexp_search
rm regexp_search.o

$CC $FLAGS -c test2_regexp_search.c &&
$LD $LDFLAGS $FLAGS test2_regexp_search.o ../cb_fifo.o ../cb_search.o ../cb_buffer.o ../cb_encoding.o ../cb_compare.o -o test2_regexp_search
rm test2_regexp_search.o

#$CC $FLAGS -c toutf.c &&
#$LD $LDFLAGS toutf.o ../cb_fifo.o ../cb_search.o ../cb_buffer.o ../cb_encoding.o -o toutf8
#rm toutf.o

#$CC $FLAGS -c fromutf.c &&
#$LD $LDFLAGS fromutf.o ../cb_fifo.o ../cb_search.o ../cb_buffer.o ../cb_encoding.o -o fromutf8
#rm fromutf.o

#$CC $FLAGS -c test_bit_reversion.c &&
#$LD $LDFLAGS test_bit_reversion.o ../cb_fifo.o ../cb_search.o ../cb_buffer.o ../cb_encoding.o -o test_bit_reversion
#rm test_bit_reversion.o

#$CC $FLAGS -c test_byte_reversion.c &&
#$LD $LDFLAGS test_byte_reversion.o ../cb_fifo.o ../cb_search.o ../cb_buffer.o ../cb_encoding.o -o test_byte_reversion
#rm test_byte_reversion.o

#$CC $FLAGS -c loop.c &&
#$LD $LDFLAGS loop.o ../cb_fifo.o ../cb_search.o ../cb_buffer.o ../cb_encoding.o -o loop
#rm loop.o

#$CC $FLAGS -c test_cbprint.c  &&
#$LD $LDFLAGS test_cbprint.o ../cb_fifo.o ../cb_read.o ../cb_search_state.o ../cb_buffer.o ../cb_encoding.o ../get_option.o -o cbprint
#rm test_cbprint.o

#$CC $FLAGS -c test_cbfindall.c &&
#$LD $LDFLAGS test_cbfindall.o ../cb_fifo.o ../cb_read.o ../cb_search_topo.o ../cb_buffer.o ../cb_encoding.o -o cbfindall

#$CC $FLAGS -c test_cb_fifo.c &&
#$LD $LDFLAGS test_cb_fifo.o ../cb_fifo.o ../cb_search.o ../cb_buffer.o ../cb_encoding.o -o test_cbfifo
#rm test_cb_fifo.o
