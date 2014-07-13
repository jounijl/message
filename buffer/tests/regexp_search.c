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
#include <string.h> // memset, strnlen
#include <stdlib.h> // malloc
#include "../../include/cb_buffer.h"  // main functions, struct cb_match
#include "../../include/cb_compare.h" // inner utilities, search function

// multiples of 4-bytes
#define BLKSIZE		128
#define OVERLAPSIZE     64

#define ERRUSAGE -1

/*
 * cat <file> | ./regexp_search <regexp pattern>
 */


/*
 * Pcre_exec returns 1 if it matched (once). If return value is more than one, internal groups were
 * found. (Groups are delimited in the re-pattern with parenthesis).
 *
 * pcre_get_substring gets allways the next groups text until it returns PCRE_ERROR_NOSUBSTRING. 
 * Third parameter of pcre_get_substring may be the return value of pcre_exec (the count of subtexts
 * to fetch).
 *
 * If it's wanted to get a new search from the same block, ovecs largest value has to be put to 
 * pcre_execs offset and fetch again.
 *
 * Same text has to be matched again and when it matches, resultvectors largest value, for example
 * ovec[pcrerc] where ovec is the resulting vector and pcrerc is pcre_execs return valua, is put to 
 * the next pcre_exec call and the next can be fetch.
 *
 *      The first pair of integers, ovector[0]  and  ovector[1],  identify  the
 *      portion  of  the subject string matched by the entire pattern. The next
 *      pair is used for the first capturing subpattern, and so on.  The  value
 *      returned by pcre_exec() is one more than the highest numbered pair that
 *      has been set. 
 */


int  main (int argc, char *argv[]);

int main (int argc, char *argv[]) {
        int bufindx=0, err=CBSUCCESS, opt=0, parambufsize=0, indx=0, chrbufindx=0, err2=CBSUCCESS;
        unsigned char *ucsname = NULL; 
        char *str_err = NULL;
	unsigned long int chr = 0x20;
	unsigned char *chrbuf = NULL;
	cb_match mctl; int mcount=0;
	int res=-1;
	mctl.re = NULL; mctl.re_extra=NULL; mctl.matchctl=-7; // these has to be initialized, otherwice free causes memory leak

	/*
	 * Arguments. */
	if( argc<=1 || ( argc>1 && argv[1]==NULL ) ){
	  fprintf(stderr,"\nUsage:\n   %s <pattern>\n\n   Searches pattern from continuous input.\n\n", argv[0]);
	  exit(ERRUSAGE);
	}

	/*
	 * Allocation */

	/* Buffer */
	chrbuf = (unsigned char*) malloc( sizeof(char)*( (BLKSIZE*4)+1 ) );
	memset( &(*chrbuf), (int) 0x20, (size_t) (BLKSIZE*4) );
	chrbuf[BLKSIZE*4]='\0';

	/* Regular expression pattern */
        parambufsize = strnlen( &(argv[1][0]), (size_t) (BLKSIZE*4) );
	ucsname = (unsigned char*) malloc( sizeof(char)*( (parambufsize*4)+1 ) ); // '\0' + argumentsize*4 // bom is ignored
        if( ucsname==NULL ){ fprintf(stderr,"\nAllocation error, ucsname."); return CBERRALLOC; }
	memset( &(*ucsname), (int) 0x20, (size_t) (parambufsize*4));
	ucsname[parambufsize*4]='\0';

	/* Copy */
	for( indx=0; indx<parambufsize && err==CBSUCCESS && indx<strlen( argv[1] ); ++indx ){
	  err = cb_put_ucs_chr( (unsigned long int) argv[1][indx], &ucsname, &chrbufindx, (parambufsize*4));
	  //err = cb_put_ucs_chr( cb_from_ucs_to_host_byte_order( (unsigned long int) argv[1][indx] ), &ucsname, &chrbufindx, (parambufsize*4)); // 13.7.2014
	}
	parambufsize = chrbufindx;
	ucsname[ parambufsize+1 ]='\0';

	fprintf(stderr,"\n%s parameter: [", argv[0]);
	cb_print_ucs_chrbuf(&ucsname, chrbufindx, parambufsize);
	fprintf(stderr,"] length = %i , chrbufindx=%i.", strlen( argv[1] ), chrbufindx );


	/*
	 * Compiling regexp  */
	fprintf(stderr,"\n cb_compare_get_matchctl( &ucsname, %i, 0, &mctl, -7 );", parambufsize);
	err = cb_compare_get_matchctl( &ucsname, parambufsize, 0, &mctl, -7 ); // 12.4.2014
	//err = cb_compare_get_matchctl_host_byte_order( &ucsname, parambufsize, 0, &mctl, -7 ); // testi 13.7.2014
	if(err!=CBSUCCESS){ fprintf(stderr,"\nError in cb_compare_get_matchctl, %i.", err); }

	fprintf(stderr,"\n From cb_compare_get_matchctl, err %i :", err);
	fprintf(stderr,"\n mctl.matchctl=%i", mctl.matchctl);
	if(mctl.re==NULL)
	  fprintf(stderr,"\n mctl.re was null, err from cb_compare_get_matchctl, %i.", err);
	else
	  fprintf(stderr,"\n mctl.re was not null.");
	if(mctl.re_extra==NULL)
	  fprintf(stderr,"\n mctl.re_extra was null, err from cb_compare_get_matchctl, %i.", err);
	else
	  fprintf(stderr,"\n mctl.re_extra was not null.");

	/*
	 * Matching the input stream as overlapped blocks */
	chr = (unsigned long int) getc(stdin); 	// bom is not needed [pcre.txt, "CHARACTER CODES"], pcre ignores bom and assumes host byte order
	//fprintf(stderr,"\nregexp_search: chr=%X bufindx=%d BLKSIZE=%d", chr, bufindx, BLKSIZE);
	while( chr!=EOF && bufindx<BLKSIZE ){
	  //cb_put_ucs_chr(chr, &chrbuf, &bufindx, BLKSIZE);
	  cb_put_ucs_chr( cb_from_ucs_to_host_byte_order( chr ), &chrbuf, &bufindx, BLKSIZE); // 13.7.2014
	  chr = (unsigned long int) getc(stdin);
	  //fprintf(stderr,"\nregexp_search: chr=%X bufindx=%d BLKSIZE=%d", chr, bufindx, BLKSIZE);
	  if(bufindx>=(BLKSIZE-6) || chr==EOF){ // '\0' + last 4-bytes = 5 bytes, can be set to 5..7

            if( chr!=(unsigned char)EOF ){ // Last one char has to be searched still
	      /*
	       * First block or in between blocks. */
              opt = opt | PCRE_NOTEOL; // Subject string is not the end of a line	    
	    }else{
	      /*
	       * Last block. */
	      opt = opt & ~PCRE_NOTEOL;
	    }
	    err = cb_compare_regexp_one_block(&chrbuf, bufindx, 0, 0, &mctl, &mcount);
	    //fprintf(stderr,"\n regexp_search main: after cb_compare_regexp,");
	    if(err>=CBERROR )
	      fprintf(stderr," error %i.", err);
	    //if(err==CBNOTFOUND){
	    //  fprintf(stderr," not found.");
	    //}else 
	    //if(err>=CBNEGATION ){
	    //  fprintf(stderr," negation %i.", err);
	    //}else 
	    if(err==CBMATCH){
	      res=1;
	      fprintf(stderr,"\nMatch found, %i.", err);
	    }else if(err==CBMATCHGROUP){
	      res=1;
	      fprintf(stderr,"\nMatch group found, %i.", err);
	    }else if(err==CBMATCHMULTIPLE){
	      res=1;
	      fprintf(stderr,"\nMultiple matches, %i.", err);
	    }//else
	    //  fprintf(stderr," %i.", err);
	    err = cb_copy_ucs_chrbuf_from_end(&chrbuf, &bufindx, BLKSIZE, OVERLAPSIZE ); // copies range: (bufindx-OVERLAPSIZE) ... bufindx
	    if(err!=CBSUCCESS){ fprintf(stderr,"\nError in cb_copy_from_end_to_start: %i.", err); }
	    /*
	     * Next block. */
            opt = opt | PCRE_NOTBOL; // Subject string is not the beginning of a line
 
	  }
	}

	if(res==-1)
	  fprintf(stderr,"\nPattern was not found.");
	
	fprintf(stderr, "\n");
        return res;
}
