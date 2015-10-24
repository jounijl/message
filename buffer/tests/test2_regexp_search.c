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


#ifdef PCRE2
#define PCRE2_CODE_UNIT_WIDTH 32
#include <pcre2.h>
#else
#include <pcre.h>
#endif

// multiples of 4-bytes
#define BLKSIZE		128
#define OVERLAPSIZE     64

#define ERRUSAGE -1

int  test_compare_regexp(unsigned char **name2, int len2, int startoffset, unsigned int options, cb_match *mctl);
int  test_compare_get_matchctl(unsigned char **pattern, int patsize, unsigned int options, cb_match *ctl, int matchctl);
int  test_get_matchctl(CBFILE **cbf, unsigned char **pattern, int patsize, unsigned int options, cb_match *ctl, int matchctl);
#ifndef PCRE2
int  test_compare_print_fullinfo(cb_match *mctl);
#endif

/*
 * cat <file> | ./test_regexp_search <regexp pattern>
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
        int bufindx=0, err=CBSUCCESS, parambufsize=0, indx=0, chrbufindx=0;
	unsigned int opt=0;
        unsigned char *ucsname = NULL; 
	unsigned long int chr = 0x20;
	unsigned char *chrbuf = NULL;
	cb_match mctl;
	int res=-1;
	//mctl.re = NULL; mctl.re_extra=NULL; mctl.matchctl=-7; mctl.resmcount=0; // these has to be initialized, otherwice free causes memory leak
	mctl.re = NULL; mctl.matchctl=-7; mctl.resmcount=0; // these has to be initialized, otherwice free causes memory leak, 21.10.2015

	/*
	 * Arguments. */
	if( argc<=1 || ( argc>1 && argv[1]==NULL ) ){
	  fprintf(stderr,"\nUsage:\n   %s <pattern>\n\n   Searches pattern from continuous input.\n\n", argv[0]);
	  exit(ERRUSAGE);
	}

	/*
	 * Allocation */

	/* Regular expression pattern */
        parambufsize = (int) strnlen( &(argv[1][0]), (size_t) (BLKSIZE*4) );
	ucsname = (unsigned char*) malloc( sizeof(char)* (unsigned int) ((parambufsize*4)+1 ) ); // '\0' + argumentsize*4 // bom is ignored
        if( ucsname==NULL ){ fprintf(stderr,"\nAllocation error, ucsname."); return CBERRALLOC; }
	memset( &(*ucsname), (int) 0x20, (size_t) (parambufsize*4) );
	ucsname[parambufsize*4]='\0';

	/* Buffer */
	chrbuf = (unsigned char*) malloc( sizeof(char)*( (BLKSIZE*4)+1 ) );
	memset( &(*chrbuf), (int) 0x20, (size_t) (BLKSIZE*4) );
	chrbuf[BLKSIZE*4]='\0';

	/* PCRE assumes host byte order and matches BOM:s if they are found, otherwice ignores them. */

	/* Copy */
	for( indx=0; indx<parambufsize && err==CBSUCCESS && indx<(int)strlen( argv[1] ); ++indx ){
	  // before 13.7.2014:
	  err = cb_put_ucs_chr( cb_from_ucs_to_host_byte_order( (unsigned int) argv[1][indx] ), &ucsname, &chrbufindx, (parambufsize*4)); // 30.6.2014
	  // after 13.7.2014 (..._host_byte_order moved to ..._get_matchctl ):
	  //err = cb_put_ucs_chr( (unsigned int) argv[1][indx], &ucsname, &chrbufindx, (parambufsize*4));
	}
	parambufsize = chrbufindx;
	ucsname[ parambufsize+1 ]='\0';


	fprintf(stderr,"\n%s parameter: [", argv[0]);
	cb_print_ucs_chrbuf( CBLOGDEBUG, &ucsname, chrbufindx, parambufsize);
	fprintf(stderr,"] length = %i , chrbufindx=%i.", strlen( argv[1] ), chrbufindx );


	/*
	 * Compiling regexp  */
	err = test_compare_get_matchctl( &ucsname, parambufsize, 0, &mctl, -7 ); // 12.4.2014
	if(err!=CBSUCCESS){ fprintf(stderr,"\nError in test_compare_get_matchctl, %i.", err); }

	//fprintf(stderr,"\n From test_compare_get_matchctl, err %i :", err);
	//fprintf(stderr,"\n mctl.matchctl=%i", mctl.matchctl);
	//if(mctl.re==NULL)
	//  fprintf(stderr,"\n mctl.re was null, err from test_compare_get_matchctl, %i.", err);
	//else
	//  fprintf(stderr,"\n mctl.re was not null.");

// --

	/*
	 * Matching the input stream as overlapped blocks */
	chr = (unsigned long int) getc(stdin); 	// bom is not needed [pcre.txt, "CHARACTER CODES"], pcre ignores bom and assumes host byte order
	while( chr!=(unsigned long int)EOF && bufindx<BLKSIZE ){
	  cb_put_ucs_chr( cb_from_ucs_to_host_byte_order( (unsigned int) chr) , &chrbuf, &bufindx, BLKSIZE);
	  chr = (unsigned long int) getc(stdin);
	  if(bufindx>=(BLKSIZE-6) || chr==(unsigned long int)EOF){ // '\0' + last 4-bytes = 5 bytes, can be set to 5..7

            if( chr!=(unsigned long int)EOF ){ // Last one char has to be searched still
	      /*
	       * First block or in between blocks. */
#ifdef PCRE2
              opt = opt | PCRE2_NOTEOL; // Subject string is not the end of a line
#else
              opt = opt | PCRE_NOTEOL; // Subject string is not the end of a line
#endif
	    }else{
	      /*
	       * Last block. */
#ifdef PCRE2
	      opt = opt & ~PCRE2_NOTEOL;
#else
	      opt = opt & (unsigned int) ~PCRE_NOTEOL;
#endif
	    }
	    err = test_compare_regexp(&chrbuf, bufindx, 0, 0, &mctl);
	    fprintf(stderr,"\n After test_compare_regexp,");
	    if(err>=CBERROR )
	      fprintf(stderr," error %i.", err);
	    if(err>=CBNEGATION ){
	      fprintf(stderr," err %i.", err);
	    }else if(err==CBMATCH){
	      res=1;
	      fprintf(stderr," match, %i.", err);
	    }else if(err==CBMATCHGROUP){
	      res=1;
	      fprintf(stderr," match group, %i.", err);
	    }else if(err==CBMATCHMULTIPLE){
	      res=1;
	      fprintf(stderr," multiple matches, %i.", err);
	    }else
	      fprintf(stderr," %i.", err);
	    err = cb_copy_ucs_chrbuf_from_end(&chrbuf, &bufindx, BLKSIZE, OVERLAPSIZE ); // copies range: (bufindx-OVERLAPSIZE) ... bufindx
	    if(err!=CBSUCCESS){ fprintf(stderr,"\nError in cb_copy_from_end_to_start: %i.", err); }
	    /*
	     * Next block. */
#ifdef PCRE2
            opt = opt | PCRE2_NOTBOL; // Subject string is not the beginning of a line
#else
            opt = opt | PCRE_NOTBOL; // Subject string is not the beginning of a line
#endif
	  }
	}
	
	fprintf(stderr, "\n");
        return res;
}

int  test_get_matchctl(CBFILE **cbf, unsigned char **pattern, int patsize, unsigned int options, cb_match *ctl, int matchctl){
	// pcre.txt "CHARACTER CODES": options = options | PCRE_UTF32; 
	if(ctl==NULL){ fprintf(stderr,"\ntest_get_matchctl: allocation error."); return CBERRALLOC; }
	if(cbf!=NULL && *cbf!=NULL){
	  if( (**cbf).cf.asciicaseinsensitive==1){
#ifdef PCRE2
	    options = options | PCRE2_CASELESS; // can be set to another here
#else
	    options = options | PCRE_CASELESS; // can be set to another here
#endif
	  }
	}
#ifdef PCRE2
	options = options | PCRE2_UCP; // unicode properties
#else
	options = options | PCRE_UCP; // unicode properties
#endif

	return test_compare_get_matchctl(&(*pattern), patsize, options, &(*ctl), matchctl);
}

int  test_compare_get_matchctl(unsigned char **pattern, int patsize, unsigned int options, cb_match *ctl, int matchctl){
	//const pcre32 *re = NULL;
	//const pcre32_extra *re_extra = NULL;
	//const int  *ptr = NULL;
	const char *errptr = NULL; int errcode=0; 
	const char  errmsg[6] = {'e','r','r','o','r','\0'};	
#ifdef PCRE2
	PCRE2_SIZE erroffset = 0;
	PCRE2_SPTR32 sptr = NULL;
#else
	int erroffset=0;
#endif
	//int err=CBSUCCESS;
	//PCRE_SPTR32 sptr = NULL; // PCRE_SPTR32 vastaa unsigned int*
	errptr = &errmsg[0];

	if(ctl==NULL){
	  fprintf(stderr, "\ntest_compare_get_matchctl: allocation error, ctl was null.");
          return CBERRALLOC;
	}
	(*ctl).matchctl = matchctl;
	if(pattern==NULL || *pattern==NULL){ // return empty cb_match if pattern was null
	  fprintf(stderr, "\ntest_compare_get_matchctl: pattern was null.");
	  return CBREPATTERNNULL;
	}
	//pcre32_free_study( (pcre32_extra*) (*ctl).re_extra );
	//free( (*ctl).re );
	//(*ctl).re = NULL; (*ctl).re_extra = NULL;
	(*ctl).re = NULL; // 21.10.2015

	//fprintf(stderr,"\ntest_compare_get_matchctl: pattern [");
	//err = cb_print_ucs_chrbuf( &(*pattern), patsize, patsize );
	//fprintf(stderr,"] err %i, patsize %i.", err, patsize );

/*
       Kaytetaan 32-bittisia funktioita mutta ei aseteta UTF-32:ta. Luotetaan
       siihen etta ohjelma muuttaa merkit 32-bittiseen UCS-muotoon.

       pcre.txt:
       In 32-bit mode, when  PCRE_UTF32  is  not  set,  character  values  are
       treated in the same way as in 8-bit, non UTF-8 mode, except, of course,
       that they can range from 0 to 0x7fffffff instead of 0 to 0xff. 
 */

	/*
	 * Compile pattern to re and re_extra. Default character tables. */
	//options = options | PCRE_UTF32; // 32-bit functions without UTF-32 (without this line is the correct choice)

/*
	Funktio allokoi errptr:n muistiin:	http://www.pcre.org/pcre.txt
	If errptr is NULL, pcre_compile() returns NULL immediately.  Otherwise,
	if compilation of a pattern fails,  pcre_compile()  returns  NULL,  and
	sets the variable pointed to by errptr to point to a textual error mes-
	sage.
*/

	////ptr  = &(* (int*) *pattern);
	////sptr = &(* (PCRE_SPTR32)  ptr); // vastaa sptr = &(**pattern)
	//sptr = &(* (PCRE_SPTR32)  *pattern); // vastaa sptr = &(**pattern)

	//fprintf(stderr,"\ntest_compare_get_matchctl: pattern before compile: [");
	//cb_print_ucs_chrbuf( CBLOGDEBUG, &(*pattern), patsize, patsize );
	//fprintf(stderr,"] patsize %i.\n", patsize );

	//re = pcre32_compile2( &(*sptr), options, &errcode, &errptr, &erroffset, NULL ); // PCRE_SPTR32 vastaa unsigned int*
	//(*ctl).re = &(* (int*) re); // cast to pointer size
	//(*ctl).re = &(* (int*) pcre32_compile2( &(*sptr), options, &errcode, &errptr, &erroffset, NULL ) ); // PCRE_SPTR32 vastaa unsigned int*, 29.5.2014
#ifndef PCRE2
	(*ctl).re = &(* (int*) pcre32_compile2( (* (PCRE_SPTR32*) pattern) , (int) options, &errcode, &errptr, &erroffset, NULL ) ); // PCRE_SPTR32 vastaa unsigned int*, 29.5.2014, 20.10.2015
#else
	sptr = &(** (PCRE2_SPTR32*)  pattern);
	(*ctl).re = &(* (PSIZE) pcre2_compile_32( sptr, (PCRE2_SIZE) (patsize/4), (uint32_t) options, &errcode, &erroffset, NULL ) );
#endif
	fprintf(stderr,"\ntest_compare_get_matchctl: pcre32_compile2, errcode %i.", errcode); // man pcreapi # /COMPILATION ERROR CODES
	fprintf(stderr,"\ntest_compare_get_matchctl: same pattern after compile: [");
	cb_print_ucs_chrbuf( CBLOGDEBUG, &(*pattern), patsize, patsize);
	fprintf(stderr,"], errcode %i.", errcode);

	//if( re==NULL ){
	if( (*ctl).re==NULL ){
          fprintf(stderr,"\ntest_compare_get_matchctl: error compiling re: \"%s\" offset %i, at re offset %i.", errptr, erroffset, errcode);
          return CBERRREGEXCOMP;
        }else{
	  //re_extra = pcre32_study( re, 0, &errptr ); // constant pointers can not change
	  // 21.10.2015 (*ctl).re_extra = &(* (int*)  pcre32_study( (*ctl).re, 0, &errptr ) ); // constant pointers can not change, 29.5.2014
	  //if( (*ctl).re_extra==NULL ){
          //  fprintf(stderr,"\ntest_compare_get_matchctl: studied re_extra was null: \"%s\" .", errptr );
	  //}
	  if(errptr!=NULL){
            fprintf(stderr,"\ntest_compare_get_matchctl: at study, errptr was \"%s\" .", errptr );
	  }
	// else{
	//    (*ctl).re_extra = &(* (int*) re_extra); // cast to pointer size
        //    //fprintf(stderr,"\ntest_compare_get_matchctl, debug, returning: re_extra was not null, errptr: \"%s\".", errptr);
	//  }
	  fprintf(stderr,"\ntest_compare_get_matchctl: studied pattern. ");
	}

	return CBSUCCESS;
}
/*
 * Compares compiled regexp to 4-byte string name2 using 32-bit functions. */
int  test_compare_regexp(unsigned char **name2, int len2, int startoffset, unsigned int options, cb_match *mctl){
	int err=CBSUCCESS, pcrerc=0;
#ifndef PCRE2
	pcre32_extra *re_extra = NULL;
	int ovecsize=OVECSIZE;
#else
	int oveccount=-1;
        pcre2_match_data_32 *match_data=NULL;
        const pcre2_code_32 *pcrecode = NULL;
#endif
	int ovec[OVECSIZE+1];
	unsigned int opt=0;
	int matchcount=0, groupcount=0;
	if( name2==NULL || *name2==NULL || mctl==NULL || (*mctl).re==NULL ){ 
	  fprintf(stderr,"\ntest_compare_regexp: allocation error."); 
	  return CBERRALLOC;
	}
	opt = opt | options;

	memset(&(ovec), (int) 0x20, (size_t) OVECSIZE); // zero
	ovec[OVECSIZE] = '\0';

#ifndef PCRE2
	test_compare_print_fullinfo( &(*mctl) );
#endif

#ifndef PCRE2
pcre_match_another:
#endif
	// 3. const PCRE_SPTR32 subject
	//pcrerc = pcre32_exec( (pcre32*) (*mctl).re, (pcre32_extra*) (*mctl).re_extra, (* (PCRE_SPTR32*) name2), len2, startoffset, opt, &(* (int*) ovec), ovecsize );
#ifndef PCRE2
	pcrerc = pcre32_exec( (pcre32*) (*mctl).re, re_extra, (* (PCRE_SPTR32*) name2), len2, startoffset, (int) opt, &(* (int*) ovec), ovecsize );
#else
	pcrecode = &(* (const pcre2_code_32*) (*mctl).re );
	match_data = &(* (pcre2_match_data_32*) pcre2_match_data_create_from_pattern_32( pcrecode, NULL ) );
        pcrerc = pcre2_match_32( pcrecode, (* (PCRE2_SPTR32*) name2 ), (PCRE2_SIZE) (len2/4), \
                (PCRE2_SIZE) startoffset, (uint32_t) opt, &(*match_data), NULL );
        oveccount = (int) pcre2_get_ovector_count( &(*match_data) );
#endif
	fprintf(stderr,"\ntest_compare_regexp: result: PCRERC %i.", pcrerc);
	//fprintf(stderr,"\ntest_compare_regexp: block:[");
	//cb_print_ucs_chrbuf( CBLOGDEBUG, &(*name2), len2, len2);
	//fprintf(stderr,"], length %i.", len2);

	if( pcrerc >= 0 ){ // one|many or groupmatch
	  ++matchcount;
	  if(err==CBMATCH){ // many
	    err = CBMATCHMULTIPLE; 
	  }else if(err!=CBMATCHMULTIPLE){ // one
	    err = CBMATCH;
	  }
	  if( pcrerc > 1 ){ // many groups
	    ++groupcount;
	    if( err!=CBMATCHMULTIPLE )
	      err = CBMATCHGROUP;
	    //pcre_get_substring would search the next substring(s) before returning PCRE_ERROR_NOSUBSTRING
	  }
/*
       pcre.txt:
       The first pair of integers, ovector[0]  and  ovector[1],  identify  the
       portion  of  the subject string matched by the entire pattern. The next
       pair is used for the first capturing subpattern, and so on.  The  value
       returned by pcre_exec() is one more than the highest numbered pair that
       has been set. 
 */
#ifdef PCRE2
	}else if( pcrerc < 0){
	  err = CBNOTFOUND;
	}
#else
	  startoffset = ovec[1];
	  fprintf(stderr, "\ntest_compare_regexp: next, startoffset=%i.", startoffset);
	  goto pcre_match_another;
	}else if( pcrerc == PCRE_ERROR_NOMATCH && err==CBSUCCESS){
	  err = CBNOTFOUND;
	}else if( pcrerc!=PCRE_ERROR_NOMATCH && pcrerc!=PCRE_ERROR_PARTIAL){ // error codes: man pcreapi
	  fprintf(stderr, "\ntest_compare_regexp: pcre32_exec returned error %i.", err); // PCRE_ERROR_BADUTF8 = -10
	  if(err==PCRE_ERROR_BADUTF8)
	    fprintf(stderr, "\ntest_compare_regexp: PCRE_ERROR_BADUTF8.");
	  if(err==CBSUCCESS)
	    err = CBERRREGEXEC;
	}
#endif
	fprintf(stderr, "\ntest_compare_regexp: returning %i, matchcount=%i, groupcount=%i.", err, matchcount, groupcount);
	return err;
}
/*
 * pcre info. */  
#ifndef PCRE2
int  test_compare_print_fullinfo(cb_match *mctl){
        int res=0;
        const pcre32_extra *rextra = NULL;
        const pcre32       *re = NULL;
        unsigned char *cres = NULL;   
        unsigned char  errmsg[6] = {'e','r','r','o','r','\0'};
        cres = &(*errmsg);
        
        if(mctl==NULL){  return CBERRALLOC; }
 
        if((*mctl).re!=NULL){ re = &(* (const pcre32*) (*mctl).re); } // 20.10.2015

        cb_clog( CBLOGINFO, "\ncb_match:");
        cb_clog( CBLOGINFO, "\n\tmatchctl=%i", (*mctl).matchctl );
        if( re != NULL ){ // const
          res = pcre32_fullinfo( re, rextra, PCRE_INFO_SIZE, &(* (PSIZE) cres) );
          if(res==0){   cb_clog( CBLOGINFO, "\n\tsize=%i (, %i, %i)", cres[0], cres[1], cres[2] ); }
          res = pcre32_fullinfo( re, rextra, PCRE_INFO_NAMECOUNT, &(* (PSIZE) cres) );
          if(res==0){   cb_clog( CBLOGINFO, "\n\tsubpatterns=%i", (unsigned int) ( (0|cres[0])<<8 | cres[1] ) ); }
          res = pcre32_fullinfo( re, rextra, PCRE_INFO_OPTIONS, &(* (PSIZE) cres) );
          if(res==0){   cb_clog( CBLOGINFO, "\n\toptions=%i", (unsigned int) ( (0|cres[0])<<8 | cres[1] ) ); }
        }
        return CBSUCCESS;
        
}       
#endif
