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
#include "../../get_option/get_option.h"

/*
 * To use text editors to analyze the output, it's useful to cut the value
 * in lines 
 */
#define LINEWIDTH   423

#define VALUEINCREASE  0
#define NAMEINCREASE   0

#define ERRUSAGE      -1

/*
 * Prints every name one by one with cb_get_next_name_ucs.
 *
 */



int main(int argc, char **argv);
void usage (char *progname[]);

int main(int argc, char **argv) {
	int err = 0, err2 = CBSUCCESS, linecounter=0, atoms=0, fromend=0;
	unsigned long int chr = 0, chprev = 0;
	unsigned char *name = NULL;
	char *str_err, *value;
	int namelen = 0, valueincrease=0, nameincrease=0, ninc=0, vinc=0, indx=0, i=0, u=0;
	long int offset=0;
	CBFILE *in = NULL;

	err = cb_allocate_cbfile(&in, 0, 2048, 512);
        if(err!=CBSUCCESS){ fprintf(stderr,"\nError at cb_allocate_cbfile: %i.", err); return CBERRALLOC;}
	//cb_set_search_state( &in, CBSTATELESS );
	cb_set_search_state( &in, CBSTATEFUL );

	cb_set_encoding(&in, 1);

        // Last parameter was null (FreeBSD)
        atoms=argc;
        if(argv[(atoms-1)]==NULL && argc>0)
          --atoms;

        // Parameters
        if ( atoms < 1 ){ // no fields
          usage(&argv[0]);
          exit(ERRUSAGE);
        }

        valueincrease = VALUEINCREASE;
        nameincrease = NAMEINCREASE;

        //
        // From end
        fromend = atoms-1;

        if ( atoms <= 1 ){
          usage(&argv[0]); // not enough parameters
          exit(ERRUSAGE);
        }
        /*
         * Rest of the fields in between start ( ./progname ) and end.
         */
        for(i=1 ; i<fromend ; ++i){ 
          u = get_option( argv[i], argv[i+1], 'V', &value); // name increase
          if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
            valueincrease = (int) strtol(value,&str_err,10);
            if(valueincrease==0 && errno==EINVAL)
              valueincrease = VALUEINCREASE;
            continue;
          }
          u = get_option( argv[i], argv[i+1], 'N', &value); // value increase
          if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
            nameincrease = (int) strtol(value,&str_err,10);
            if(nameincrease==0 && errno==EINVAL)
              nameincrease = NAMEINCREASE;
            continue;
          }
	}
	if(valueincrease<0 || nameincrease<0 ){
	    fprintf(stderr,"\nValueincrease and nameincrease have to be positive values.");
	    usage(&(argv[0]));
	    exit(ERRUSAGE);
	}

	vinc = 1;
	ninc = 0;

	do{
	  err = cb_get_next_name_ucs(&in, &name, &namelen);
	  if( err>=CBNEGATION && err!=CBNOTFOUND )
	    fprintf(stderr,"\ncb_get_next_name_ucs: err=%i.", err);
	  if( err==CBSTREAM || err==CBSUCCESS ){
            fprintf( stderr, "\n" );
	    //
	    // Print name
/*	    for(indx=0; indx<ninc; ++indx){ // Length increase
	      if( name != NULL && namelen > 0 ){
	         i = 0; chr = (long unsigned int) 'A';
	         u = cb_get_ucs_chr( &chr, &name, &i, namelen);
	         if(u>=CBNEGATION)
	           fprintf( stderr, "\ncb_get_ucs_chr: err %i.", u );
	         fprintf( stderr, "%lc", chr );
	      }
	    }
*/
	    ninc += nameincrease;
            //fprintf( stderr, "[0]", chr );
	    cb_print_ucs_chrbuf(&name, namelen, namelen); // Name
            fprintf( stderr, "%lc", (*in).rstart );
	    err2==CBSUCCESS; linecounter=0;
            //fprintf( stderr, "[1]", chr );
	    while( ( err2==CBSTREAM || err2==CBSUCCESS ) && ! ( chr==(*in).rend && chprev!=(*in).bypass ) ){
              //fprintf( stderr, "[loop 2]", chr );
	      chprev = chr;
	      err2 = cb_search_get_chr(&in, &chr, &offset );
	      if(err2==CBSTREAM) // This is required here, 10.11.2013
	        cb_remove_name_from_stream( &in );
              //fprintf( stderr, "[loop 3]", chr );
	      //
	      // Print value

	      if( ( err2==CBSTREAM || err2==CBSUCCESS ) && ! ( chr==(*in).rend && chprev!=(*in).bypass ) ){
//	        for(indx=0; indx<vinc; ++indx) // Length increase
                  fprintf( stderr, "%lc", chr );
	      }else if( chr==(*in).rend && chprev!=(*in).bypass ){
	        fprintf( stderr, "%lc", (*in).rend );
	        vinc += valueincrease;
	      }	      
	      chr=(*in).rend-1;
              //fprintf( stderr, "[loop 4]", chr );
	      //
	      // Decrease line length to read the result in text editors
	      ++linecounter;
	      if(linecounter==LINEWIDTH){
	        fprintf( stderr, "\n");
	        linecounter=0;
	      }
	    }
	    free(name); name=NULL; // name=NULL pois, virheviesti
            //fprintf( stderr, "[5]", chr );
	  }
	//}while( err!=CBSTREAMEND && err<CBERROR ); // looptest
	}while( err!=CBNOTFOUND && err!=CBSTREAMEND && err<CBERROR ); 

	cb_free_cbfile(&in);

	return err;
}

void usage (char *progname[]){
        fprintf(stderr,"Usage:\n");
        fprintf(stderr,"\t%s [-V <value gradual increase> ] [-N <name gradual increase> ] \n\n", progname[0]);
        fprintf(stderr,"\tOutputs inputs name-value pairs. Every character is printed\n");
        fprintf(stderr,"\t<value gradual increase> times in value and <name gradual increase> .\n");
        fprintf(stderr,"\tin name.\n");
}

