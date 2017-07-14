/*
 * Program to convert encoding between input and output files.
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
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h> // strcmp
#include <errno.h>  // int errno

#include "../include/cb_buffer.h"
#include "../get_option/get_option.h"

#define BUFSIZE       1024
#define BLKSIZE       128

#define ERRUSAGE      -1

//#define DEBUG

int  main (int argc, char **argv);
void usage ( char *progname[] );

int main (int argc, char **argv) {
	int i=-1, u=0, y=0, atoms=0, fromend=0, err=CBSUCCESS;
	int bufsize=BUFSIZE, blksize=BLKSIZE, inputenc=CBENC4BYTE, outputenc=CBENCUTF8;
	char *str_err=NULL; // , *value;
	const char *value=NULL;
	CBFILE *in = NULL;
	CBFILE *out = NULL;
	unsigned long int chr = 0x61;

/*	fprintf(stderr,"main: argc=%i", argc );
	for(u=0;u<argc;++u)
	  fprintf( stderr,", argv[%i]=%s", u, argv[u] );
	fprintf( stderr,".\n" ); 
*/

	// Last parameter was null (FreeBSD)
	atoms=argc;
	if(argv[(atoms-1)]==NULL && argc>0)
	  --atoms;

	if ( atoms < 1 ){ // no fields
	  usage(&argv[0]);
	  exit(ERRUSAGE);
	}

	//
	// From end
	fromend = atoms;

#ifdef DEBUG
	fprintf(stderr,"\nDebug: name (namelen:%i, namebuflen:%i):", namelen, namebuflen);
	cb_print_ucs_chrbuf(&name, (namelen*4), namebuflen);
	//fprintf(stderr,"\nDebug: argv[fromend]: %s", argv[fromend]);
#endif 

	// Allocate buffers
	err = cb_allocate_cbfile( &in, 0, bufsize, blksize);
	if(err>=CBERROR){ fprintf(stderr, "error at cb_allocate_cbfile: %i.", err); }
	cb_set_encoding(&in, CBENC4BYTE);
	err = cb_allocate_cbfile( &out, 1, bufsize, blksize);
	if(err>=CBERROR){ fprintf(stderr, "error at cb_allocate_cbfile: %i.", err); }
	cb_set_encoding(&out, CBENCUTF8);

	// Parameters
	for(i=1 ; i<fromend ; ++i){ 
	  u = get_option( argv[i], argv[i+1], 'i', &value); // input encoding number ( from cb_encoding.h )
	  if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
	    inputenc = (int) strtol(value,&str_err,10); 
            if(inputenc==0 && errno==EINVAL)
              inputenc = CBENC4BYTE;
	    cb_set_encoding(&in, inputenc);
	    continue;
	  }
	  u = get_option( argv[i], argv[i+1], 'o', &value); // output encoding number ( from cb_encoding.h )
	  if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
	    outputenc = (int) strtol(value,&str_err,10);
            if(outputenc==0 && errno==EINVAL)
              outputenc = CBENCUTF8;
	    cb_set_encoding(&out, outputenc);
	    continue;
	  }
	  u = get_option( argv[i], argv[i+1], 'h', &value); // -h , usage message
	  if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
            usage(&argv[0]);
            exit(ERRUSAGE);
	  }
	} 

#ifdef DEBUG
	// Debug
	fprintf(stderr,"");
#endif

	// Program
	err = cb_get_chr(&in, &chr, &u, &y);
	if( err>=CBERROR ){ cb_clog( CBLOGERR, err, "\ncb_get_chr: error %i.", err  ); }
	else if( err>=CBNEGATION ){ cb_clog( CBLOGDEBUG, err, "\ncb_get_chr: %i.", err  ); }
	while( err!=CBSTREAMEND && err<CBERROR ){
	  //fprintf(stderr,"[%c]", (int) chr);
	  err = cb_put_chr(&out, chr, &u, &y);
	  if( err>=CBERROR ){ cb_clog( CBLOGERR, err, "\ncb_put_chr: error %i.", err  ); }
	  else if( err>=CBNEGATION ){ cb_clog( CBLOGDEBUG, err, "\ncb_put_chr: %i.", err  ); }
	  if( err==CBSUCCESS || err==CBSTREAM ){
	    err = cb_get_chr(&in, &chr, &u, &y);
	    if( err>=CBERROR ){ cb_clog( CBLOGERR, err, "\ncb_get_chr: error %i.", err  ); }
	    else if( err>=CBNEGATION ){ cb_clog( CBLOGDEBUG, err, "\ncb_get_chr: %i.", err  ); }
	  }
	}

	cb_flush(&out); // 30.6.2015

	cb_free_cbfile(&in);
	cb_free_cbfile(&out);

	exit( err );
}


void usage (char *progname[]){
	fprintf(stderr,"Usage:\n");
	fprintf(stderr,"\t%s [-i <encoding number> ] [ -o <encoding number> ] \\\n", progname[0]);
	fprintf(stderr,"\n\tConverts input from stdin in encoding -i to output\n");
	fprintf(stderr,"\tin encoding -o. Default input encoding is 4-byte UCS\n");
	fprintf(stderr,"\trepresentation of a character. Default output encoding\n");
	fprintf(stderr,"\tis UTF-8.\n");
}
