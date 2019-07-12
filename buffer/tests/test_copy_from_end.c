/*
 * Program to test a function in cb_fifo.c .
 * 
 * Copyright (c) 2009, 2010 and 2013, Jouni Laakso
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
#include <string.h> // memset
#include <stdlib.h> // malloc
#include "../../include/cb_buffer.h"

// multiples of 4-bytes
#define BLKSIZE		128
#define OVERLAPSIZE     64

/*
 * cat <file> | ./test_copy_from_end
 */

int  main (int argc, char *argv[]);

int main (int argc, char *argv[]) {
        int bufindx=0, err=CBSUCCESS;
        char *str_err = NULL;
	unsigned long int chr = 0x20;
	//unsigned char chrbuf[BLKSIZE+1];
	unsigned char *chrbuf = NULL;

	chrbuf = (unsigned char*) malloc( sizeof(char)*(BLKSIZE+1) );
	memset( &(*chrbuf), (int) 0x20, (size_t) BLKSIZE);
	chrbuf[BLKSIZE]='\0';

	chr = (unsigned long int) getc(stdin);
	while( chr!=EOF && bufindx<BLKSIZE ){
	  cb_put_ucs_chr(chr, &chrbuf, &bufindx, BLKSIZE);
	  chr = (unsigned long int) getc(stdin);
	  if(bufindx>=BLKSIZE || chr==EOF){
	    cprint( STDERR_FILENO, "block:[");
	    cb_print_ucs_chrbuf(&chrbuf, bufindx, BLKSIZE);
	    cprint( STDERR_FILENO, "]\n");
	    err = cb_copy_ucs_chrbuf_from_end(&chrbuf, &bufindx, BLKSIZE, OVERLAPSIZE );
	    if(err!=CBSUCCESS){ cprint( STDERR_FILENO, "\nError in cb_copy_from_end_to_start: %i.", err); }
	  }
	}
	
        return CBSUCCESS;
}
