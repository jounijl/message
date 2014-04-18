/*
 * Program to test CBFILE library.
 *
 * Copyright (c) 2006, 2011, 2012 and 2013 Jouni Laakso
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 * Neither the name of the copyright owners nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>     // memset

#include "../../include/cb_buffer.h"

/* 4 time character count buffer */
#define OUTPUTSTEXTTIMES     40
#define TESTTEXTLENGTH       4 		// readaheadsize on 20 joka jaettuna neljalla on viisi (28->7)

int  main (int argc, char *argv[]);
int  main (int argc, char *argv[]){
        cb_ring cfi; int err=0, i=0, u=0;
        unsigned long int chr = ' ';
	int chrsize=1;

	memset(&(cfi.buf[0]), (int) 0x20, (size_t) CBREADAHEADSIZE);
	memset(&(cfi.storedsizes[0]), (int) 0x20, (size_t) CBREADAHEADSIZE);
        cb_fifo_init_counters(&cfi);

        for(u=0; u<OUTPUTSTEXTTIMES && chr!=(unsigned long int)EOF ; ++u){
          for(i=0; i<TESTTEXTLENGTH && chr!=(unsigned long int)EOF; ++i){
            //cb_fifo_print_counters(&cfi);
            chr = (unsigned long int) getc(stdin);
            if( chr!=(unsigned long int) EOF )
              err = cb_fifo_put_chr(&cfi, chr, chrsize);
	    //fprintf(stdout,"\n+ err=%i", err);
          }

	  //fprintf(stdout,"\n1. err=%i", err);
	  //err = cb_fifo_revert_chr(&cfi, &chr, &chrsize);
	  //fprintf(stdout,"\nReverted: %c , err=%i", chr, err);
          //cb_fifo_print_counters(&cfi);

          for(i=0; i<TESTTEXTLENGTH; ++i){
            //cb_fifo_print_counters(&cfi);
            err = cb_fifo_get_chr(&cfi, &chr, &chrsize);
            if(err==CBSUCCESS)
              fprintf(stdout,"%c", (int) chr);
	    //fprintf(stdout,"\n- err=%i", err);
          }
        }

        return 0;
}
