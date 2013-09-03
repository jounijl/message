/*
 * Program to search name from cbfput name-value pairs.
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

#define NAMEBUFLEN    128
#define BUFSIZE       1024
#define BLKSIZE       128
#define ENDCHR        0x0A

#define ERRUSAGE      -1

#define DEBUG
#undef  TMP

int  main (int argc, char **argv);
int  search_and_print_name(CBFILE **in, unsigned char **name, int namelength);
int  print_current_name(CBFILE **cbf);
void usage ( char *progname[] );

int main (int argc, char **argv) {
	int i=-1,u=0,y=0,namearraylen=0, atoms=0,fromend=0, err=CBSUCCESS;
	int bufsize=BUFSIZE, blksize=BLKSIZE, namelen=0, namebuflen=0, count=1;
	char list=0;
	char *str_err, *value, *namearray = NULL;
	CBFILE *in = NULL;
	unsigned char *name = NULL;
	unsigned char chr = ' ', chprev = ' ';
	unsigned long int endchr = ENDCHR;

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
	if ( atoms < 2 ){ // no fields
	  usage(&argv[0]);
	  exit(ERRUSAGE);
	}

	//
	// From end
	fromend = atoms-1;

	/*
	 * Name */
        if ( atoms >= 2 ){
	  namelen = (int) strlen( argv[fromend] );
	  namebuflen = NAMEBUFLEN; // 4 * namelen;
	  name = (unsigned char *) malloc( sizeof(unsigned char)*( namebuflen + 1 ) );
	  if(name==NULL){ fprintf(stderr,"\nerror in malloc, name was null"); exit(CBERRALLOC); }
	  // name[ namelen*4 ] = '\0';
	  u = 0;
	  for(i=0; i<namelen && u<namebuflen; ++i){
	    chr = (unsigned long int) argv[fromend][i]; chr = chr & 0x000000FF;
	    err = cb_put_ucs_chr( chr, &name, &u, namebuflen);
	  }
	  if( namebuflen>(i*4) )
	    name[ i*4 ] = '\0';
	  if(err!=CBSUCCESS){ fprintf(stderr,"\ncbsearch: cb_put_ucs_chr, err=%i.", err); }
	  if( namebuflen>(namelen*4) )
            name[ namelen*4 ] = '\0';

        }else{
          usage(&argv[0]); // not enough parameters
          exit(ERRUSAGE);
        }

#ifdef DEBUG
	fprintf(stderr,"\nDebug: name (namelen:%i, namebuflen:%i):", namelen, namebuflen);
	cb_print_ucs_chrbuf(&name, (namelen*4), namebuflen);
	//fprintf(stderr,"\nDebug: argv[fromend]: %s", argv[fromend]);
#endif 

	// Allocate buffer
	err = cb_allocate_cbfile( &in, 0, bufsize, blksize);
	if(err>=CBERROR){ fprintf(stderr, "error at cb_allocate_cbfile: %i.", err); }
	cb_set_to_polysemantic_names(&in);
	cb_set_encoding(&in, CBENC1BYTE);

	/*
	 * Rest of the fields in between start ( ./progname ) and end ( <name> )
         */
	for(i=1 ; i<fromend ; ++i){ 
	  u = get_option( argv[i], argv[i+1], 'c', &value); // count
	  if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
	    count = (int) strtol(value,&str_err,10);
            if(count==0 && errno==EINVAL)
              count = 1;
	    continue;
	  }
	  u = get_option( argv[i], argv[i+1], 'b', &value); // buffer size
	  if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
	    bufsize = (int) strtol(value,&str_err,10);
            if(bufsize==0 && errno==EINVAL)
              bufsize = BUFSIZE;
	    continue;
	  }
	  u = get_option( argv[i], argv[i+1], 'l', &value); // block size
	  if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
	    blksize = (int) strtol(value,&str_err,10);
            if(blksize==0 && errno==EINVAL)
              blksize = BLKSIZE;
	    continue;
	  }
	  u = get_option( argv[i], argv[i+1], 'e', &value); // end character
	  if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
	    if(value!=NULL){
	      endchr = (unsigned long int) strtol(value,&str_err,16);
	      fprintf(stderr, "\nAfter get_option, value=%s endchr=0x%.6lX.", value, endchr);
	      err = CBSUCCESS;
	      if( ! ( endchr==0 && errno==EINVAL ) )
	        err = cb_set_rend( &in, endchr);
	      if( err>=CBERROR ){ fprintf(stderr,"\nError ar cb_set_rend, %i.", err); }
            }else{
	      err = cb_set_rend( &in, endchr); // set to new line, 0x0A
              if( err>=CBERROR ){ fprintf(stderr,"\nError ar cb_set_rend, %i.", err); }
	    }
	    continue;
	  }
	  u = get_option( argv[i], argv[i+1], 's', &namearray); // list of names
	  if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
	    list=1;
	    if(namearray!=NULL)
	      err = CBSUCCESS;
            else
	      err = CBERRALLOC;
	    continue;
	  }
	} 

#ifdef DEBUG
	// Debug
	if( name!=NULL )
	  fprintf(stderr,"\nDebug: Searching [");
	cb_print_ucs_chrbuf(&name, (namelen*4), namebuflen);
	if(in!=NULL)
	fprintf(stderr,"] count [%i] buffer size [%i] block size [%i]", count, bufsize, blksize );
	if(in!=NULL)
	  fprintf(stderr," value end [0x%.6lX]", (*in).rend);
	fprintf(stderr,"\n");
#endif

	// Program
	for(i=0; i<count && err<=CBERROR; ++i){
	  fprintf(stderr,"\n%i.", (i+1) );
	  if( list==0 ) // one name
	    err = search_and_print_name(&in, &name, (namelen*4) );
	  else{ // list of names
	    if(namearray!=NULL){
	      memset( &(*name), (int) 0x20, namebuflen );
	      namearraylen = strnlen( &(*namearray), namebuflen );
	      u = 0; chprev = (unsigned long int) 0x0A; namelen=0;
	      for(y=0; y<namearraylen && y<10000; ++y ){
	        chprev = chr;
	        chr = (unsigned long int) namearray[y]; chr = chr & 0x000000FF;
	        if( ! WSP( chr ) ){
	          err = cb_put_ucs_chr( chr, &name, &u, namebuflen);
	       	  namelen = u;
	          //fprintf(stderr,"%C", chr );
	        } 
	        if( ( WSP( chr ) && ! WSP( chprev ) ) || y==(namearraylen-1) || chr=='\0' ){
	          fprintf(stderr,"\n Search name: ");
	          cb_print_ucs_chrbuf(&name, namelen, namebuflen);
	          fprintf(stderr,".\n");
	          name[ namelen*4 ] = '\0';
	          err = search_and_print_name(&in, &name, namelen );
	          namelen = 0; u = 0;
	          memset( &(*name), (int) 0x20, namebuflen );
	        }
	      }
	    }
	  }
	}

	memset( &(*name), (int) 0x20, namebuflen );
	name[namebuflen] = '\0';
	cb_free_cbfile(&in);
	free( name );

	exit( err );
}


void usage (char *progname[]){
	fprintf(stderr,"Usage:\n");
	fprintf(stderr,"\t%s [-c <count> ] [ -b <buffer size> ] \\\n", progname[0]);
	fprintf(stderr,"\t      [ -l <block size> ] [-e <char in hex> ] <name> \n\n");
	fprintf(stderr,"\t%s [-c <count> ] [ -b <buffer size> ] [ -l <block size> ] \\\n", progname[0]);
	fprintf(stderr,"\t	[ -e <char in hex> ] -s \"name1 name2 name3 ...\"\n");
	fprintf(stderr,"\n\tSearches name from input once or <count> times. Buffer\n");
	fprintf(stderr,"\tand block sizes can be set. End character can be changed\n");
	fprintf(stderr,"\tfrom LF (0x0A) with value in hexadesimal. Many names can be\n");
	fprintf(stderr,"\tdefined with flag -s. Names are searched in order. Search\n");
	fprintf(stderr,"\tsearches next same names.\n\n");
}

/*
 * Search name and print it.
 */
int  search_and_print_name(CBFILE **in, unsigned char **name, int namelength){
	int err = 0; 

	if( in==NULL || *in==NULL )
	  return CBERRALLOC;
	err = cb_set_cursor_ucs( &(*in), &(*name), &namelength );
	if(err>=CBERROR){ fprintf(stderr, "error at cb_set_cursor: %i.", err); }
	if( err==CBSUCCESS || err==CBSTREAM ){
	  err = print_current_name(&(*in));
	  if(err!=CBSUCCESS){ fprintf(stderr, "\nName not found.\n"); }
	}
	if(err==CBNOTFOUND){
	  fprintf(stderr, "\nName \"");
	  cb_print_ucs_chrbuf( &(*name), namelength, namelength );
	  fprintf(stderr, "\" not found.\n");
	  return err;
	}
	if(err==CBSTREAM){
	  fprintf(stderr, "\nStream start.\n");
	}else if(err==CBSTREAMEND){
	  fprintf(stderr, "\nStream end.\n");
	}
	return err;
}
int  print_current_name(CBFILE **cbf){
	int err = 0, bcount=0, strbcount=0;
#ifdef CBSTATETOPOLOGY
	int opennamepairs=1;
#endif
	unsigned long int chr = (unsigned long int) ' ', chrprev = (unsigned long int) ' ';

	if(cbf==NULL || *cbf==NULL || (**cbf).cb == NULL || (*(**cbf).cb).current == NULL)
	  return CBERRALLOC;

	fprintf(stderr, "\n Name:         \t[");
	cb_print_ucs_chrbuf( &(*(*(**cbf).cb).current).namebuf, (*(*(**cbf).cb).current).namelen, (*(*(**cbf).cb).current).buflen);
	fprintf(stderr, "]\n Name length:  \t%i", (*(*(**cbf).cb).current).namelen);
	fprintf(stderr, "\n Offset:       \t%li", (*(*(**cbf).cb).current).offset);
	fprintf(stderr, "\n Length set to:\t%i", (*(*(**cbf).cb).current).length);
	fprintf(stderr, "\n Matchcount:   \t%li", (*(*(**cbf).cb).current).matchcount);
	fprintf(stderr, "\n Content:      \t\"");

#ifdef CBSTATETOPOLOGY
	while( ( chrprev!=(**cbf).bypass && chr!=(**cbf).rend && opennamepairs==0 ) && err<=CBNEGATION ){
#else
	while( ( chr!=(**cbf).rend && chrprev!=(**cbf).bypass ) && err<=CBNEGATION ){
#endif
	  chrprev = chr;
	  err = cb_get_chr( &(*cbf), &chr, &bcount, &strbcount );
	  if(err==CBSTREAM)
	    cb_remove_name_from_stream(&(*cbf)); // used as stream and removing name whose value is across buffer boundary
	  if( chr!=(**cbf).rend )
	    fprintf(stderr,"%c", (int) chr);
#ifdef CBSTATETOPOLOGY
	  if( chr==(**cbf).rend )
	    --opennamepairs;
	  if( chr==(**cbf).rstart )
	    ++opennamepairs;
	  if( opennamepairs<0 )
	    opennamepairs = 0;
#endif
	}
	if(err>=CBERROR){ fprintf(stderr, "error at cb_get_chr: %i.", err); }
	fprintf(stderr,"\"\n");
	return err;
}

