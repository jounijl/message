/*
 * Program to generate name-value (or key-value) pairs. Generates output to test CBFILE.
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
#include <time.h>   // time
#include <string.h> // strcmp
#include <errno.h>  // int errno

#include "../include/cb_buffer.h"
#include "../get_option/get_option.h"

//#define NAMEBUFLEN    2048
#define BUFSIZE       1024
#define BLKSIZE       128

#define ERRUSAGE      -1

#define COUNT          1
#define VALUESIZE      10
#define NAMESIZE       10
#define VALUEINCREASE  0
#define NAMEINCREASE   0

#define DEBUG

int  main (int argc, char **argv);
void usage ( char *progname[] );
int write_utf8_2B( CBFILE **out, int count );
int write_utf8_5B( CBFILE **out, int count );

int main (int argc, char **argv) {
	int i=-1, u=0, atoms=0, fromend=0, err=CBSUCCESS, bcount=0, strdbytes=0;
	int co=0, vals=0, nams=0, valuesize=0, namesize=0, valueincrease=0, nameincrease=0;
	int bufsize=BUFSIZE, blksize=BLKSIZE, count=1;
	char outputenc=CBENC1BYTE;
	char *str_err, *value, flip2B5Bflop=0;
	CBFILE *out = NULL;
	long unsigned int chr = ' ';
	long unsigned int seed = (unsigned long) time(NULL);
	srandom(seed); 

/*	fprintf(stderr,"main: argc=%i", argc );
	for(u=0;u<argc;++u)
	  fprintf( stderr,", argv[%i]=%s", u, argv[u] );
	fprintf( stderr,".\n" ); 
*/

	// Last parameter was null (FreeBSD)
	atoms=argc;
	if(argv[(atoms-1)]==NULL && argc>0)
	  --atoms;

	// Parameters
	if ( atoms < 1 ){ // no fields
	  usage(&argv[0]);
	  exit(ERRUSAGE);
	}

	count = COUNT;
	valuesize = VALUESIZE;
	namesize = NAMESIZE;
	valueincrease = VALUEINCREASE;
	nameincrease = NAMEINCREASE;

	//
	// From end
	fromend = atoms-1;

        if ( atoms <= 1 ){
          usage(&argv[0]); // not enough parameters
          exit(ERRUSAGE);
        }

	// Allocate buffer
	err = cb_allocate_cbfile( &out, 1, bufsize, blksize);
	if(err>=CBERROR){ fprintf(stderr, "error at cb_allocate_cbfile: %i.", err); }
	cb_set_encoding(&out, CBENC1BYTE); // default

	/*
	 * Rest of the fields in between start ( ./progname ) and end.
         */
	for(i=1 ; i<fromend ; ++i){ 
	  u = get_option( argv[i], argv[i+1], 'c', &value); // count of pairs
	  if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
	    count = (int) strtol(value,&str_err,10);
            if(count==0 && errno==EINVAL)
              count = COUNT;
	    continue;
	  }
	  u = get_option( argv[i], argv[i+1], 'n', &value); // name size
	  if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
	    namesize = (int) strtol(value,&str_err,10);
            if(namesize==0 && errno==EINVAL)
              namesize = NAMESIZE;
	    continue;
	  }
	  u = get_option( argv[i], argv[i+1], 'N', &value); // name gradual increase, negative or positive
	  if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
	    nameincrease = (int) strtol(value,&str_err,10);
            if(nameincrease==0 && errno==EINVAL)
              nameincrease = NAMEINCREASE;
	    continue;
	  }
          u = get_option( argv[i], argv[i+1], 'e', &value); // output encoding number ( from cb_encoding.h )
          if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
            outputenc = (int) strtol(value,&str_err,10); 
            if(outputenc==0 && errno==EINVAL)
              outputenc = CBENC1BYTE;
            cb_set_encoding(&out, outputenc);
            continue;
          }
          u = get_option( argv[i], argv[i+1], 'v', &value); // value size
          if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
            valuesize = (int) strtol(value,&str_err,10); 
            if(valuesize==0 && errno==EINVAL)
              valuesize = VALUESIZE;
            continue;
          }
          u = get_option( argv[i], argv[i+1], 'V', &value); // value gradual increase
          if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
            valueincrease = (int) strtol(value,&str_err,10); 
            if(valueincrease==0 && errno==EINVAL)
              valueincrease = VALUEINCREASE;
            continue;
          }
	}

#ifdef DEBUG
	// Debug
	fprintf(stderr,"\nDebug: Output ");
	if(out!=NULL){
	  fprintf(stderr," count [%i] encoding [%i] valuesize [%i] namesize [%i]", count, outputenc, valuesize, namesize );
	  fprintf(stderr," valueincrease [%i] nameincrease [%i]", valueincrease, nameincrease );
	}
	fprintf(stderr,"\n");
#endif

	// Program
	vals = valuesize;
	nams = namesize;
	co   = count;
	if( count == -1 )
	  co = 10;
	for(i=0; i<co && err<=CBERROR; ++i){
	  for(u=0; u<nams; ++u){
	    if(outputenc==CBENCUTF8){
	      if( flip2B5Bflop==0 ){
	        flip2B5Bflop=1;
/*
 * Koptic small alpha:
 * UCS: U+2C81
 * UTF-8: 0xE2 0xB2 0x81
 * UTF-16: 0x2C81
 */
	        err = cb_put_chr(&out, 0x0002C81, &bcount, &strdbytes ); // alpha 3 B
	      }else{
	        flip2B5Bflop=0;
/*
 * Small alpha:
 * UCS: U+03B1
 * UTF-8: 0xCE 0xB1
 * UTF-16: 0x03B1
 */
	        err = cb_put_chr(&out, 0x03B1, &bcount, &strdbytes ); // alpha 2 B
	      }
	    }else{
	      chr = (long unsigned int) 'A';
              err = cb_put_chr(&out, chr, &bcount, &strdbytes );
	    }
	  }
          err = cb_put_chr(&out, (*out).cf.rstart, &bcount, &strdbytes );
	  err = cb_flush(&out);
	  if(err!=CBSUCCESS){ fprintf(stderr,"\nError at cb_flush: %i.", err ); return err; }
	  for(u=0; u<vals; ++u){
	    if(outputenc==CBENCUTF8){
	    //  if( flip2B5Bflop==0 ){ // alpha = 03B1 / 2 B ; looks like omega (coptic): 10177
	    //    flip2B5Bflop=1;
	    //    err = cb_put_chr(&out, 0x10177, &bcount, &strdbytes );
	    //  }else{
	    //    flip2B5Bflop=0;
/*
 * Small beta: 
 * UCS: U+03B2
 * UTF-8: 0xCE 0xB2
 * UTF-16: 0x03B2
 */
	        err = cb_put_chr(&out, 0x03B2, &bcount, &strdbytes ); // beta 2 B
	    //  }
/* 
 * Small omega:
 * UCS: U+03C9
 * UTF-8: 0xCF 0x89
 * UTF-16: 0x03C9
 */

/*          
 * Koptic small vida:
 * UCS:  U+2C83
 * UTF-8: 0xE2 0xB2 0x83
 * UTF-16: 0x2C83
 */

	    }else{
	      chr = (long unsigned int) 'B';
              err = cb_put_chr(&out, chr, &bcount, &strdbytes );
	    }
	  }
          err = cb_put_chr(&out, (*out).cf.rend, &bcount, &strdbytes );
	  err = cb_flush(&out);
	  if(err!=CBSUCCESS){ fprintf(stderr,"\nError at cb_flush: %i.", err ); return err; }
	  nams += nameincrease;
	  vals += valueincrease;
	  if( count == -1 )
	    i = 0;
	}

	cb_free_cbfile(&out);

	return err ;
}


void usage (char *progname[]){
	fprintf(stderr,"Usage:\n");
	fprintf(stderr,"\t%s [-c <count of pairs> ] | [-e <encoding number> ] \\\n", progname[0]);
	fprintf(stderr,"\t     | [-v <value size> ] | [-V <value gradual increase> ] \\\n");
	fprintf(stderr,"\t     | [-n <name size> ] | [-N <name gradual increase> ] \n\n");
	fprintf(stderr,"\tOutputs name-value pairs to use in testing. Valuesize and namesize can\n");
	fprintf(stderr,"\tbe chosen and set to increase or decrease. Encoding is a number from\n");
	fprintf(stderr,"\tcb_encoding.h. Endless output if value of <count of pairs> is -1.\n\n");
}


/*
   Generate characters in Unicode 5 bytes
   Ancient greek numbers: http://www.unicode.org/charts/PDF/U10140.pdf
   10140 - 1018A 
*/

/*
   Generate characters in Unicode 2 byte
   Greek and coptic: http://www.unicode.org/charts/PDF/U0370.pdf
   0370 - 03FF 
  (Except:
   0380,   0381,   0382,   0383,   03A2,   
   0378,   0379,   038B,   038D,   037F )
*/

