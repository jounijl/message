/*
 * Program to search a name from name-value pairs.
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

#define LIKECHR       '%'

#define NAMEBUFLEN    2048
#define BUFSIZE       1024
#define BLKSIZE       128
#define ENDCHR        0x0A

#define ERRUSAGE      -1

#define DEBUG

int  main (int argc, char **argv);
int  search_and_print_name(CBFILE **in, unsigned char **name, int namelength, char tree);
int  print_name(CBFILE **cbf, cb_name **nm);
int  search_and_print_tree(CBFILE **cbs, unsigned char **dotname, int namelen, int matchctl);
void usage ( char *progname[] );

int main (int argc, char **argv) {
	int i=-1,u=0,y=0,namearraylen=0, atoms=0,fromend=0, err=CBSUCCESS;
	int bufsize=BUFSIZE, blksize=BLKSIZE, namelen=0, namebuflen=0, count=1, co=0;
	char list=0, inputenc=CBENC1BYTE, tree=0;
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
	  name[ namelen*4 ] = '\0';
	  u = 0;
	  for(i=0; i<namelen && u<namebuflen; ++i){
	    chr = (unsigned long int) argv[fromend][i]; chr = chr & 0x000000FF;
	    err = cb_put_ucs_chr( chr, &name, &u, namebuflen);
	  }
	  if( namebuflen > u )
	    name[ u ] = '\0';
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
	cb_use_as_stream(&in);
	cb_set_encoding(&in, CBENC1BYTE);
	(*in).cf.rfc2822headerend=0;

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
	  u = get_option( argv[i], argv[i+1], 'x', &value); // regular expression search
	  if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE || u == GETOPTSUCCESSNOVALUE){
	    tree = -10;
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
          u = get_option( argv[i], argv[i+1], 'i', &value); // input encoding number ( from cb_encoding.h )
          if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
            inputenc = (int) strtol(value,&str_err,10); 
            if(inputenc==0 && errno==EINVAL)
              inputenc = CBENC1BYTE;
            cb_set_encoding(&in, inputenc);
            continue;
          }
          u = get_option( argv[i], argv[i+1], 't', &value); // traverse the dotted (or primary key-) tree with cb_tree_set_cursor_ucs
          if( u == GETOPTSUCCESS || u == GETOPTSUCCESSATTACHED || u == GETOPTSUCCESSPOSSIBLEVALUE ){
	    tree = 1;
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
	fprintf(stderr,"] count [%i] buffer size [%i] block size [%i] encoding [%i]", count, bufsize, blksize, inputenc );
	if(in!=NULL)
	  fprintf(stderr," value end [0x%.6lX]", (*in).cf.rend);
	fprintf(stderr,"\n");
#endif

	// Program
	co = count;
	if(count==-1)
	  co = 10;
	for(i=0; i<co && err<=CBERROR; ++i){
	  if(count==-1)
	    i = 0;
	  fprintf(stderr,"\n%i.", (i+1) );
	  if( list==0 ) // one name
	    err = search_and_print_name(&in, &name, (namelen*4), tree );
	  else{ // list of names
	    if(namearray!=NULL){
	      memset( &(*name), (int) 0x20, namebuflen );
	      namearraylen = strnlen( &(*namearray), namebuflen );
	      u = 0; chprev = (unsigned long int) 0x0A; namelen=0;
	      for(y=0; y<namearraylen && y<10000; ++y ){ // (if first char in name is sp, possibly prints "null name")
	        chprev = chr;
	        chr = (unsigned long int) namearray[y]; chr = chr & 0x000000FF;
	        if( ! WSP( chr ) ){
	          err = cb_put_ucs_chr( chr, &name, &u, namebuflen);
	       	  namelen = u;
	        } 
	        if( ( WSP( chr ) && ! WSP( chprev ) && namelen>0 ) || y==(namearraylen-1) || chr=='\0' ){
	          name[ namelen*4 ] = '\0';
	          err = search_and_print_name(&in, &name, namelen, tree );
	          namelen = 0; u = 0;
	          memset( &(*name), (int) 0x20, namebuflen );
	        }
	      }
	    }
	  }
	}

	// Debug:
	//cb_print_names(&in);

	memset( &(*name), (int) 0x20, namebuflen );
	name[namebuflen] = '\0';
	cb_free_cbfile(&in);
	free( name );

	exit( err );
}


void usage (char *progname[]){
	fprintf(stderr,"Usage:\n");
	fprintf(stderr,"\t%s [-c <count> ] [-b <buffer size> ] [-l <block size> ] [-x]\\\n", progname[0]);
	fprintf(stderr,"\t     [-i <encoding number> ] [-e <char in hex> ] [-t ] <name> \n\n");
	fprintf(stderr,"\t%s [-c <count> ] [-b <buffer size> ] [-l <block size> ] \\\n", progname[0]);
	fprintf(stderr,"\t     [-i <encoding number> ] [-e <char in hex> ] [-t ] [-x]\\\n");
	fprintf(stderr,"\t         -s \"<name1> [ <name2> [?<name3> [...] ] ]\"\n");
	fprintf(stderr,"\n\tSearches name from input once or <count> times. Buffer\n");
	fprintf(stderr,"\tand block sizes can be set. End character can be changed\n");
	fprintf(stderr,"\tfrom LF (0x0A) with value in hexadesimal. Many names can be\n");
	fprintf(stderr,"\tdefined with flag -s. Names are searched in order. Search\n");
	fprintf(stderr,"\tsearches next same names. %s only unfolds the text,\n", progname[0]);
	fprintf(stderr,"\tit does not remove cr, lf or wsp characters between values\n");
	fprintf(stderr,"\tand names. Name can be matched from beginning, in the middle\n");
        fprintf(stderr,"\tor from the end using character \'%c\' to represent \'any\'. If\n", '%');
        fprintf(stderr,"\t<count> is -1, search is endless. With -t the search include\n");
        fprintf(stderr,"\tinner subtrees. Name is separated with dots representing\n");
	fprintf(stderr,"\tthe sublevels. Regular expression in name, flag \'x\'.\n\n");
        fprintf(stderr,"\tExample 1:\n");
        fprintf(stderr,"\t   cat testfile.txt | ./cbsearch -c 4 -b 2048 -l 512 unknown\n\n");
        fprintf(stderr,"\tExample 2:\n");
        fprintf(stderr,"\t   cat testfile.txt | ./cbsearch -c 4 -b 2048 -l 512 %cknow%c\n\n", '%', '%');
        fprintf(stderr,"\tExample 3:\n");
        fprintf(stderr,"\t   cat testfile.txt | ./cbsearch -c 2 -b 1024 -l 512 -s \"name1 name4 %cknown unkno%c\"\n\n", '%', '%');
        fprintf(stderr,"\tExample 4:\n");
        fprintf(stderr,"\t   cat testfile.txt | ./cbsearch -c -1 -b 1024 -l 512 un_known\n\n");
}

/*
 * Search name and print it.
 */
int  search_and_print_name(CBFILE **in, unsigned char **name, int namelength, char tree){
	int err = 0, indx = 0; unsigned long int stchr = 0x20, endchr = 0x20;
	unsigned char tmp[CBNAMEBUFLEN+1];
	unsigned char *ptr = NULL;
	cb_name   *nameptr = NULL;
	tmp[CBNAMEBUFLEN]  = '\0';
	ptr = &(tmp[0]);

	if( in==NULL || *in==NULL )
	  return CBERRALLOC;
	if( name!=NULL && *name!=NULL && namelength>0 ){ // %ame nam% %am%
	  if(tree<=-8){ // regular expression search
	    err = cb_set_cursor_match_length_ucs( &(*in), &(*name), &namelength, 0, tree );
	  }else{ // Test LIKECHR
	    cb_get_ucs_chr(&stchr, &(*name), &indx, namelength); indx = (namelength - 4);
	    cb_get_ucs_chr(&endchr, &(*name), &indx, namelength);
	    if( stchr == (unsigned long int) LIKECHR ){
	      for(indx=0; indx<namelength-4 && indx<CBNAMEBUFLEN-4; ++indx)
	        tmp[indx] = (*name)[indx+4] ;
	      if( endchr == (unsigned long int) LIKECHR ){
	        indx -= 4; // %am%
	        if(tree==0){
	          err = cb_set_cursor_match_length_ucs( &(*in), &ptr, &indx, 0, -6 );
	        }else
	          err = search_and_print_tree( &(*in), &ptr, indx, -6 );
	      }else{
	        if(tree==0){
	          err = cb_set_cursor_match_length_ucs( &(*in), &ptr, &indx, 0, -5 ); // %ame
	        }else
	          err = search_and_print_tree( &(*in), &ptr, indx, -5 ); 
	      }
	    }else if( endchr == (unsigned long int) LIKECHR ){
	      namelength -= 4; // %
	      if(tree==0){
	        err = cb_set_cursor_match_length_ucs( &(*in), &(*name), &namelength, 0, -2 ); // nam%
	      }else{
	        err = search_and_print_tree( &(*in), &(*name), namelength, -2 ); 
	      }
	    }else{
	      if(tree==0){
	        err = cb_set_cursor_ucs( &(*in), &(*name), &namelength ); // matchctl=1
	      }else
	        err = search_and_print_tree( &(*in), &(*name), namelength, 1 ); 
	    }
	  }
	}else{
	  fprintf(stderr, "\n Name was null.\n");
	}
	if(err>=CBERROR){ fprintf(stderr, "error at cb_set_cursor: %i.", err); }
	if(tree==0)
	  nameptr = &(*(*(**in).cb).list.current);
	else
	  nameptr = &(*(*(**in).cb).list.currentleaf);
	if( err==CBSUCCESS || err==CBSTREAM ){
	  //nameptr = &(*(*(**in).cb).list.current);
	  //fprintf(stderr, "\n cbsearch, printing name:");
	  print_name(&(*in), &nameptr );
	  fprintf(stderr, " cbsearch, printing leaves:");
	  cb_print_leaves( &nameptr );
	}
	if(err==CB2822HEADEREND ){
	  fprintf(stderr, "\n Header end. \n");
	}
	if( tree==0 && ( err==CBNOTFOUND || err==CB2822HEADEREND ) ){
	  fprintf(stderr, "\n Name \"");
	  cb_print_ucs_chrbuf( &(*name), namelength, namelength );
	  fprintf(stderr, "\" not found.\n");
	  return err;
	}
	if(err==CBSTREAM){
	  fprintf(stderr, "\n Stream.\n");
	}else if(err==CBSTREAMEND){
	  fprintf(stderr, "\n Stream end.\n");
	}
	return err;
}
int  print_name(CBFILE **cbf, cb_name **nm){
	int err = 0;
	signed long int tmp=0;

	int opennamepairs=1; // cbstatetopology

	unsigned long int chr = (unsigned long int) ' ', chrprev = (unsigned long int) ' ';

	if(cbf==NULL || *cbf==NULL || (**cbf).cb == NULL || nm == NULL || *nm ==NULL )
	  return CBERRALLOC;

	chrprev=(**cbf).cf.bypass+4; chr=(**cbf).cf.rend+4;

	fprintf(stderr, "\n Name:         \t[");
	cb_print_ucs_chrbuf( &(**nm).namebuf, (**nm).namelen, (**nm).buflen);
	fprintf(stderr, "]\n Name length:  \t%i", (**nm).namelen);
	fprintf(stderr, "\n Offset:       \t%lli", (**nm).offset);
	fprintf(stderr, "\n Length set to:\t%i", (**nm).length);
	fprintf(stderr, "\n Ahead:        \t%i", (**cbf).ahd.ahead );
	fprintf(stderr, "\n Bytes ahead:  \t%i", (**cbf).ahd.bytesahead );
	fprintf(stderr, "\n Matchcount:   \t%li", (**nm).matchcount );
	fprintf(stderr, "\n Content:      \t\"");

	chrprev = chr;
	//err = cb_get_chr( &(*cbf), &chr, &bcount, &strbcount );
	err = cb_get_chr_unfold( &(*cbf), &chr, &tmp );

	while( ( ( chr!=(**cbf).cf.rend || ( chrprev==(**cbf).cf.bypass && chr==(**cbf).cf.rend ) ) && \
	   ( (**cbf).cf.searchstate!=CBSTATETOPOLOGY || ( opennamepairs!=0 && (**cbf).cf.searchstate==CBSTATETOPOLOGY ) ) ) && \
	     err<=CBNEGATION ){

	  if(err==CBSTREAM)
	    cb_remove_name_from_stream(&(*cbf)); // used as a stream and removing a name whose value is across the buffer boundary
	  fprintf(stderr,"%c", (int) chr);
	  if( (**cbf).cf.searchstate==CBSTATETOPOLOGY ){
	    if( chr==(**cbf).cf.rend )
	      --opennamepairs;
	    if( chr==(**cbf).cf.rstart )
	      ++opennamepairs;
	    if( opennamepairs<0 )
	      opennamepairs = 0;
	  }
	  chrprev = chr;
	  //err = cb_get_chr( &(*cbf), &chr, &bcount, &strbcount );
	  err = cb_get_chr_unfold( &(*cbf), &chr, &tmp );
	}
	if(err>=CBERROR){ fprintf(stderr, "error at cb_get_chr_unfold: %i.", err); }
	fprintf(stderr,"\"\n");
	return err;
}

/*
 * Search name1.name2.name3  */
int  search_and_print_tree(CBFILE **cbs, unsigned char **dotname, int namelen, int matchctl){

        int err=CBSUCCESS, err2=CBSUCCESS, indx=0, undx=0;
        int                  ret = CBNEGATION;
        char           namecount = 0;
        char     origsearchstate = 1;
        unsigned   char *ucsname = NULL;
        cb_name       *firstname = NULL;
        unsigned long int    chr = 0x20;
        cb_name            *leaf = NULL;

        if( cbs==NULL || *cbs==NULL || (**cbs).cb==NULL ) return CBERRALLOC;
        if( dotname==NULL || *dotname==NULL ) return CBERRALLOC;

        /* Allocate new name to hold the name */
        ucsname = (unsigned char *) malloc( sizeof(char)*( namelen+1 ) );       
        if( ucsname == NULL ) { return CBERRALLOC; }
        ucsname[namelen] = '\0';

        origsearchstate = (**cbs).cf.searchstate;
        (**cbs).cf.searchstate = CBSTATETREE;

        /* Every name */
        undx = 0; indx=0;
        while( indx<namelen && err<=CBNEGATION && err2<=CBNEGATION ){
          err2 = cb_get_ucs_chr( &chr, &(*dotname), &indx, namelen );
          if( chr != (unsigned long int) '.')
            err = cb_put_ucs_chr( chr, &ucsname, &undx, namelen );
          if( chr == (unsigned long int) '.' || indx>=namelen ){ // Name

            err = cb_set_cursor_match_length_ucs( &(*cbs), &ucsname, &undx, namecount, matchctl );
            if( err==CBSUCCESS || err==CBSTREAM ){ // Debug
              firstname = &(*(*(**cbs).cb).list.current);
              leaf = &(*(*(**cbs).cb).list.currentleaf);
              if(namecount==0){
                fprintf(stderr,"\nFound \"");
	        cb_print_ucs_chrbuf( &(*firstname).namebuf, (*firstname).namelen, (*firstname).buflen );
	        fprintf(stderr,"\", (from list)");
                fprintf(stderr," leaves from currentleaf: ");
                leaf = &(* (cb_name*) (*(*(**cbs).cb).list.currentleaf).leaf);
                cb_print_leaves( &leaf );
              }else{
                fprintf(stderr,"\nFound leaf (currentleaf) \"");
	        cb_print_ucs_chrbuf( &(*leaf).namebuf, (*leaf).namelen, (*leaf).buflen );
	        fprintf(stderr,"\" (from tree).");
                fprintf(stderr," leaves from currentleaf: ");
                cb_print_leaves( &leaf );
	      }
            }else{
	      fprintf(stderr,"\n\"");
	      cb_print_ucs_chrbuf( &ucsname, undx, undx );
	      fprintf(stderr,"\" not found.");
	    }

            if(err!=CBSUCCESS && err!=CBSTREAM)
              ret = CBNOTFOUND;
            else
              ret = err;

            undx = 0;
            ++namecount;
          }
        }
        (**cbs).cf.searchstate = origsearchstate;
        free(ucsname);
        if(indx==namelen && (ret==CBSUCCESS || ret==CBSTREAM) ) // Match and dotted name was searched through (cursor is at name), 13.12.2013
          return ret;
        else // Name was not searched through or no match (cursor is somewhere in value)
          return CBNOTFOUND;
}

