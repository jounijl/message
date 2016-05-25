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
#include <errno.h>  // int errno
#include "../../include/cb_buffer.h"
#include "../../include/cb_read.h"
#include "../../get_option/get_option.h"

#define ERRUSAGE -1

int main(int argc, char **argv);
void usage (char *progname[]);

#define INBLOCKSIZE 2048

int main(int argc, char **argv) {
	int err = CBSUCCESS, atoms=0, fromend=0, i=0, u=0, bc=0, sb=0, indx=0;
	int inblklen = INBLOCKSIZE;
	unsigned char  ucstextdata[INBLOCKSIZE+1];
	unsigned char *ucstext = NULL;
	int            ucstextlen = 0;
	unsigned long int chr = 0;
	char to_url_encoded = 0;
	char *value = NULL;
	CBFILE *in  = NULL;
	CBFILE *out = NULL;
	ucstextdata[INBLOCKSIZE] = '\0';
	ucstext = &ucstextdata[0];

	err = cb_allocate_cbfile(&in, 0, 4, inblklen );
        if(err!=CBSUCCESS){ cb_clog( CBLOGERR, err, "\nError at cb_allocate_cbfile: %i (in).", err); return CBERRALLOC; }
	err = cb_allocate_cbfile(&out, 1, 4, 512 );
        if(err!=CBSUCCESS){ cb_clog( CBLOGERR, err, "\nError at cb_allocate_cbfile: %i (out).", err); return CBERRALLOC; }

	cb_set_encoding(&in, CBENC1BYTE);
	cb_set_encoding(&out, CBENC1BYTE);

        // Last parameter was null (FreeBSD)
        atoms=argc;
        if(argv[(atoms-1)]==NULL && argc>0)
          --atoms;

        // Parameters
        if ( atoms < 1 ){ // no fields
          usage(&argv[0]);
          exit(ERRUSAGE);
        }

        //
        // From end
        fromend = atoms-1;

        if ( atoms < 1 || atoms > 2 ){
          usage(&argv[0]); // not enough parameters or too many
          exit(ERRUSAGE);
        }
        /*
         * Rest of the fields in between start ( ./progname ) and end.
         */
        for(i=1 ; i<=fromend ; ++i){
          u = get_option( argv[i], argv[i+1], 'i', &value);
          if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
		to_url_encoded = 0;
          }
          u = get_option( argv[i], argv[i+1], 'o', &value);
          if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
		to_url_encoded = 1;
          }
	}

	if( to_url_encoded==0 ){
		/*
		 * From URL-encoded to one-byte text. */
		ucstextlen=0;
		while( err<CBNEGATION && chr!=(unsigned long int)EOF && ucstextlen<INBLOCKSIZE ){
			err = cb_get_chr( &in, &chr, &bc, &sb );
			if(err>=CBERROR){ cb_clog( CBLOGERR, err, "\ncb_get_chr, error %i.", err); }
			if( err<CBNEGATION ){
				err = cb_put_ucs_chr( chr, &ucstext, &ucstextlen, INBLOCKSIZE );
				//cb_clog( CBLOGDEBUG, CBNEGATION, "[%c, %i]", (unsigned char) chr, err );
			}
		}

		//cb_clog( CBLOGERR, err, "\nDebug, ucstext: [");
		//cb_print_ucs_chrbuf( CBLOGDEBUG, &ucstext, ucstextlen, INBLOCKSIZE );
		//cb_clog( CBLOGERR, err, "]");

		err = cb_decode_url_encoded_bytes( &ucstext, ucstextlen, &ucstext, &ucstextlen, INBLOCKSIZE );
		if(err>=CBERROR){ cb_clog( CBLOGERR, err, "\ncb_decode_url_encoded_bytes, error %i.", err ); }

		err = CBSUCCESS; indx=0;
		//cb_clog( CBLOGDEBUG, CBNEGATION, "ucstextlen %i |", ucstextlen );
		while( err<CBNEGATION && chr!=(unsigned long int)EOF && indx<INBLOCKSIZE && indx<ucstextlen ){
			err = cb_get_ucs_chr( &chr, &ucstext, &indx, ucstextlen);
			if(err<CBNEGATION){
				err = cb_put_chr( &out, chr, &bc, &sb );
				//cb_clog( CBLOGDEBUG, CBNEGATION, "%c, 0x%.2x | err %i |", (unsigned char) chr, (unsigned char) chr, err );
				if(err>=CBERROR){ cb_clog( CBLOGERR, err, "\ncb_put_chr, error %i.", err); }
			}else{
				cb_clog( CBLOGERR, err, "\ncb_get_ucs_chr returned %i.", err);
			}
		}
	}else if( to_url_encoded==1 ){
		/*
		 * From one-byte text to URL-encoded text. */
		while( err<CBNEGATION && chr!=(unsigned long int)EOF && ucstextlen<INBLOCKSIZE ){   
                        err = cb_get_chr( &in, &chr, &bc, &sb );
			if(err>=CBERROR){ cb_clog( CBLOGERR, err, "\ncb_get_chr, error %i.", err); }
                        if( err<CBNEGATION ){
				err = cb_put_url_encode( &out, chr, &bc, &sb );
				if(err>=CBERROR){ cb_clog( CBLOGERR, err, "\ncb_put_url_encode, error %i.", err); }
			}
		}
	}

	//cb_clog( CBLOGERR, err, "\nDebug, ucstext: [");
	//cb_print_ucs_chrbuf( CBLOGDEBUG, &ucstext, ucstextlen, INBLOCKSIZE );
	//cb_clog( CBLOGERR, err, "]");

	cb_flush( &out );
	cb_free_cbfile(&in);
	cb_free_cbfile(&out);

	return err;
}

void usage (char *progname[]){
        fprintf(stderr,"Usage:\n");
        fprintf(stderr,"\t <input> | %s [ -i  | -o ] \n\n", progname[0]);
        fprintf(stderr,"\tOutputs one byte presentation from url percent encoded input or\n");
        fprintf(stderr,"\twith flag -o, url percent encoded output from one byte input.\n");
}

