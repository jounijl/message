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
#include <stdlib.h>
#include <unistd.h>
#include "../../include/cb_buffer.h"

/*
 * Prints every name on by one with cb_get_next_name_ucs.
 *
 */

int main(int argc, char **argv);

int main(int argc, char **argv) {
	int err = 0, err2 = CBSUCCESS;
	unsigned long int chr = 0, chprev = 0;
	unsigned char *name = NULL;
	int namelen = 0;
	long int offset=0;
	CBFILE *in = NULL;

	err = cb_allocate_cbfile(&in, 0, 2048, 512);
        if(err!=CBSUCCESS){ fprintf(stderr,"\nError at cb_allocate_cbfile: %i.", err); return CBERRALLOC;}

	cb_set_encoding(&in, 1);

	do{
	  err = cb_get_next_name_ucs(&in, &name, &namelen);
	  if( err>=CBNEGATION && err!=CBNOTFOUND )
	    fprintf(stderr,"\ncb_get_next_name_ucs: err=%i.", err);
	  if( err==CBSTREAM || err==CBSUCCESS ){
            fprintf( stderr, "\n" );
	    cb_print_ucs_chrbuf(&name, namelen, namelen);	
            fprintf( stderr, "%lc", (*in).rstart );
	    err2==CBSUCCESS;
	    while( ( err2==CBSTREAM || err2==CBSUCCESS ) && ! ( chr==(*in).rend && chprev!=(*in).bypass ) ){
	      chprev = chr;
	      err2 = cb_search_get_chr(&in, &chr, &offset );
	      if( ( err2==CBSTREAM || err2==CBSUCCESS ) && ! ( chr==(*in).rend && chprev!=(*in).bypass ) )
                fprintf( stderr, "%lc", chr );
	      else if( chr==(*in).rend && chprev!=(*in).bypass )
	        fprintf( stderr, "%lc", (*in).rend );
	      chr=(*in).rend-1;
	    }
	    free(name); name=NULL; // name=NULL pois, virheviesti
	  }
	//}while( err!=CBSTREAMEND && err<CBERROR ); // looptest
	}while( err!=CBNOTFOUND && err!=CBSTREAMEND && err<CBERROR ); 

	cb_free_cbfile(&in);

	return err;
}
