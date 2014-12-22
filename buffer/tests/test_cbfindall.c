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
#include "../../include/cb_read.h"

/*
 * Finds all names once with cb_find_every_name and prints them.
 *
 */


int main(void);

int main(void) {
	int err = CBSUCCESS;
	CBFILE *in = NULL;

	err = cb_allocate_cbfile(&in, 0, 2048, 512);
        if(err!=CBSUCCESS){ fprintf(stderr,"\nError at cb_allocate_cbfile: %i.", err); return CBERRALLOC;}
	// Tests: cat testi.txt | $0
	// cat testi.txt | tr -c -d "&" | wc -c # 53
	// cat testi.txt | tr -c -d "=" | wc -c # 48
	//cb_set_search_state( &in, CBSTATETREE ); // lists, something missing
	//cb_set_search_state( &in, CBSTATELESS ); // lists 48
	//cb_set_search_state( &in, CBSTATEFUL ); // lists 40 (something is missing)

	cb_set_search_state( &in, CBSTATETOPOLOGY ); // error

	// Returns:
	// cb_search.c: "Value read, openpairs 0 ocoffset 1"
	// return CBVALUEEND;

	cb_set_encoding(&in, 1);

	err = cb_find_every_name(&in);
	if( err==CBVALUEEND ){
		fprintf(stderr,"\n Returning VALUEEND (method %i)", (*in).cf.searchmethod );
		if( (*in).cf.searchmethod==CBSTATETOPOLOGY || (*in).cf.searchmethod==CBSTATETREE ){
			fprintf(stderr,", possible start and endchar mismatch.");
		}
	}
	if(err>=CBNEGATION && err!=CBNOTFOUND)
	  fprintf(stderr,"\ncb_find_every_name: err=%i.", err);

	cb_print_names(&in);

	cb_free_cbfile(&in);

	return err;
}
