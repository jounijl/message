/*
 * Program to test CBFILE library.
 *
 * Copyright (c) 2006, 2011, 2012, 2013 and 2023 Jouni Laakso
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
#include <string.h>
#include "../../include/cb_buffer.h"
#include "../../include/cb_read.h"

/*
 * Finds parameter attribute increasing group number in every search, 16.11.2023.
 */

signed int  main (signed int argc, char **argv);
signed int  main (signed int argc, char **argv){
	signed int err = CBSUCCESS, u = 0;
	unsigned char *prm = NULL;
	signed int prmlen = 0, group = 0;
	CBFILE *in = NULL;
	if( argc<1 || argv==NULL || *argv==NULL ) exit( -1 );
	prm = &( (unsigned char**) argv)[1][0];
	prmlen = (signed int) strlen( &argv[1][0] );

	cprint( STDERR_FILENO, "main: argc=%i prm [", argc );
        for(u=0;u<prmlen;++u)
          cprint( STDERR_FILENO,"%c", prm[u] );
        cprint( STDERR_FILENO,"].\n" );

	err = cb_allocate_cbfile(&in, 0, 2048, 512);
        if(err!=CBSUCCESS){ cprint( STDERR_FILENO, "\nError at cb_allocate_cbfile: %i.", err); return CBERRALLOC;}

	cb_set_encoding(&in, CBENCUTF8);
	(*in).cf.searchmethod = CBSEARCHNEXTNAMES;
	//(*in).cf.searchmethod = CBSEARCHNEXTGROUPNAMES;
	//(*in).cf.searchmethod = CBSEARCHNEXTLASTGROUPNAMES;

	err = CBSUCCESS;
	for( group=0; group<100 && ( err<CBNEGATION && err!=CBMESSAGEHEADEREND && err!=CBMESSAGEEND ); ++group ){
		cb_increase_group_number( &in );
		for( u=0; u<3; ++u ){
			err = cb_set_cursor( &in, &prm, &prmlen );
			cb_clog( CBLOGERR, CBSUCCESS, "\nerr %i", err );
			if( err<CBNEGATION && err!=CBMESSAGEHEADEREND && err!=CBMESSAGEEND ){
				cb_clog( CBLOGERR, CBSUCCESS, "\nFound %s, group %i", prm, group );
				if( (*(*in).cb).list.current!=NULL ){
					cb_clog( CBLOGERR, CBSUCCESS, ", cursor at %li [", (*(*(*in).cb).list.current).offset );
					cb_print_name( &(*(*in).cb).list.current, CBLOGERR );
					cb_clog( CBLOGERR, CBSUCCESS, "]." );
				}else{
					cb_clog( CBLOGERR, CBSUCCESS, ", error." );
				}
			}
		}
		//err = CBSUCCESS;
	}

	if(err>=CBNEGATION && err!=CBNOTFOUND){
	  cprint( STDERR_FILENO, "\ntest_groups: err=%i.", err);
	}

	//cb_print_names(&in, CBLOGDEBUG);

	cb_free_cbfile(&in);

	return err;
}
