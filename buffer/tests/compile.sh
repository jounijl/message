#!/bin/sh

#gcc -c toutf.c &&
#gcc toutf.o ../cb_fifo.o ../cb_search.o ../cb_buffer.o ../cb_encoding.o -o toutf8
#rm toutf.o

#gcc -c fromutf.c &&
#gcc fromutf.o ../cb_fifo.o ../cb_search.o ../cb_buffer.o ../cb_encoding.o -o fromutf8
#rm fromutf.o

#gcc -c test_bit_reversion.c &&
#gcc test_bit_reversion.o ../cb_fifo.o ../cb_search.o ../cb_buffer.o ../cb_encoding.o -o test_bit_reversion
#rm test_bit_reversion.o

#gcc -c test_byte_reversion.c &&
#gcc test_byte_reversion.o ../cb_fifo.o ../cb_search.o ../cb_buffer.o ../cb_encoding.o -o test_byte_reversion
#rm test_byte_reversion.o

#gcc -c loop.c &&
#gcc loop.o ../cb_fifo.o ../cb_search.o ../cb_buffer.o ../cb_encoding.o -o loop
#rm loop.o

gcc -c test_cbprint.c  &&
gcc test_cbprint.o ../cb_fifo.o ../cb_search_state.o ../cb_buffer.o ../cb_encoding.o -o cbprint
rm test_cbprint.o

gcc -c test_cbfindall.c &&
gcc test_cbfindall.o ../cb_fifo.o ../cb_search_topo.o ../cb_buffer.o ../cb_encoding.o -o cbfindall

#gcc -c test_cb_fifo.c &&
#gcc test_cb_fifo.o ../cb_fifo.o ../cb_search.o ../cb_buffer.o ../cb_encoding.o -o test_cbfifo
#rm test_cb_fifo.o
