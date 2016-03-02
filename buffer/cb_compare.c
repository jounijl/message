/* 
 * Library to read and write streams. Valuepair list and search. Different character encodings.
 * 
 * Copyright (C) 2009, 2010, 2013, 2014, 2015 and 2016. Jouni Laakso
 *
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * Otherwice, this library is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser        
 * General Public License version 2.1 as published by the Free Software Foundation 6. of June year 2012;
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
 * more details. You should have received a copy of the GNU Lesser General Public License along with this library; if
 * not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * 
 * licence text is in file LIBRARY_LICENCE.TXT with a copyright notice of the licence text.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>    // memset
#include "../include/cb_buffer.h"     // main function definitions (the only one needed)
#include "../include/cb_compare.h"    // utilities

int  cb_compare_print_fullinfo(cb_match *mctl);
int  cb_compare_get_matchctl_host_byte_order(unsigned char **pattern, int patsize, unsigned int options, cb_match *ctl, int matchctl);

int  cb_compare_rfc2822(unsigned char **name1, int len1, unsigned char **name2, int len2, int from2){ // from2 23.11.2013
	unsigned long int chr1=0x65, chr2=0x65;
	int err1=CBSUCCESS, err2=CBSUCCESS;
	int indx1=0, indx2=0;
	if(name1==NULL || name2==NULL || *name1==NULL || *name2==NULL)
	  return CBERRALLOC;

	//fprintf(stderr,"\ncb_compare_rfc2822: [");
	//cb_print_ucs_chrbuf(&(*name1), len1, len1); fprintf(stderr,"] [");
	//cb_print_ucs_chrbuf(&(*name2), len2, len2); fprintf(stderr,"] len1: %i, len2: %i.", len1, len2);
	//fprintf(stderr,"\ncb_compare_rfc2822: [from2=%i]", from2);

	indx2 = from2;
	while( indx1<len1 && indx2<len2 && err1==CBSUCCESS && err2==CBSUCCESS ){ // 9.11.2013

	  err1 = cb_get_ucs_chr(&chr1, &(*name1), &indx1, len1);
	  err2 = cb_get_ucs_chr(&chr2, &(*name2), &indx2, len2);

	  chr1 = cb_from_ucs_to_host_byte_order( chr1 ); // 31.7.2014
	  chr2 = cb_from_ucs_to_host_byte_order( chr2 ); // 31.7.2014

	  if(err1>=CBNEGATION || err2>=CBNEGATION){
	    //return CBNOTFOUND;
	    return CBMISMATCH; // 20.10.2015
	  }
	  if( chr2 == chr1 ){
	    continue; // while
	  }else if( chr1 >= 65 && chr1 <= 90 ){ // large
	    if( chr2 >= 97 && chr2 <= 122 ) // small, difference is 32
	      if( chr2 == (chr1+32) ) // ASCII, 'A' + 32 = 'a'
	        continue;
	  }else if( chr2 >= 65 && chr2 <= 90 ){ // large
	    if( chr1 >= 97 && chr1 <= 122 ) // small
	      if( chr1 == (chr2+32) ) 
	        continue;
	  }
	  if( len1>(len2-from2) || ( len1==(len2-from2) && chr1<chr2 ) )
	    return CBGREATERTHAN; // 31.7.2014
	  else if( len1<(len2-from2) || ( len1==(len2-from2) && chr1>chr2 ) )
	    return CBLESSTHAN; // 31.7.2014
	  return CBMISMATCH; // 20.10.2015
	  //return CBNOTFOUND;
	}

	if( len1==(len2-from2) ) // 31.7.2014
	  return CBMATCH;
	else if( (len2-from2)>len1 ) // 9.11.2013, 23.11.2013, 31.7.2014 	// else if( len2>len1 ) // 9.11.2013, 23.11.2013
	  return CBMATCHLENGTH;
	else if( len1>(len2-from2) ) // 9.11.2013, 23.11.2013, 31.7.2014
	  return CBMATCHPART;

	return err1;
}
/*
 * Compares if name1 matches name2. CBMATCHPART returns if name1 is longer
 * than name2. CBMATCHLENGTH returns if name1 is shorter than name2.
 */
int  cb_compare(CBFILE **cbs, unsigned char **name1, int len1, unsigned char **name2, int len2, cb_match *mctl){
	int err = CBSUCCESS, mcount = 1;
	int indx = 0;
	int dfr = 0;
	unsigned char stb[3] = { 0x20, 0x20, '\0' };
	unsigned char *stbp = NULL;
	cb_match newmctl;
	stbp = &stb[0];
	newmctl.re=NULL; newmctl.matchctl=1;

	if(mctl==NULL){ cb_log( &(*cbs), CBLOGALERT, CBERRALLOC, "\ncb_compare: allocation error, cb_match."); return CBERRALLOC; }
	if( cbs==NULL || *cbs==NULL )
	  return CBERRALLOC;

	if( name1==NULL || name2==NULL || *name1==NULL || *name2==NULL ){ 
		cb_log( &(*cbs), CBLOGALERT, CBERRALLOC, "\ncb_compare: name allocation error."); 
		return CBERRALLOC;
 	} // 24.10.2015

	//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_compare: address of name1: %lx, address of name2: %lx.", (long int) *name1, (long int) *name2 );

/**
	//if( (*mctl).matchctl<=-7 && (*mctl).matchctl>=-10){
	  cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_compare, name1: [");
	  cb_print_ucs_chrbuf( CBLOGDEBUG, &(*name1), len1, len1); cb_clog( CBLOGDEBUG, CBNEGATION, "], name2: [");
	  cb_print_ucs_chrbuf( CBLOGDEBUG, &(*name2), len2, len2); cb_clog( CBLOGDEBUG, CBNEGATION, "] len1: %i, len2: %i, matchctl %i", len1, len2, (*mctl).matchctl );
	  cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_compare: index %li, maxlength %li", (*(**cbs).cb).index, (*(**cbs).cb).maxlength );
	  cb_clog( CBLOGDEBUG, CBNEGATION, " readlength %li, buflen %li, contentlen %li.", (*(**cbs).cb).readlength, (*(**cbs).cb).buflen, (*(**cbs).cb).contentlen );
	//}
 **/

	if((*mctl).matchctl==-1)
	  return CBMISMATCH; // no match, 20.10.2015
	  //return CBNOTFOUND; // no match
	if( (*mctl).matchctl==-8 || (*mctl).matchctl==-10 ){ // new 18.3.2014, not yet tested 18.3.2014
	  /*
	   * Compiles regexp from pattern just before search.
	   * name1 has to be NULL -terminated string.
	   */
	  if(name1==NULL){	cb_log( &(*cbs), CBLOGALERT, CBERRALLOC, "\nerror in cb_compare, -8: name1 was null.");  return CBERRALLOC; }

	  err = cb_get_matchctl( &(*cbs), &(*name1), len1, 0, &newmctl, (*mctl).matchctl ); // 13.4.2014
	  if(err!=CBSUCCESS){ cb_log( &(*cbs), CBLOGERR, err, "\ncb_compare, -8: error in cb_get_matchctl, %i.", err); }
	  err = cb_compare_regexp( &(*name2), len2, &newmctl, &mcount);
	  if(err>=CBERROR){ cb_log( &(*cbs), CBLOGERR, err, "\ncb_compare, -8: error in cb_compare_regexp, %i.", err); }
	  (*mctl).resmcount = mcount; // 9.8.2015
	  pcre2_code_free_32( (pcre2_code_32*) newmctl.re ); // 15.11.2015, free just compiled re
	}else if( (*mctl).matchctl==-7 || (*mctl).matchctl==-9 ){ // new 18.3.2014, not yet tested 18.3.2014
	  /*
	   * Uses only compiled regexp inside mctl.
	   * name1 may be null and len1 is ignored. 
	   * Pattern has to be compiled and attached
	   * to mctl before and elsewhere. name2 has
	   * to be NULL terminated.
	   */
	  if( (*mctl).re==NULL){	cb_log( &(*cbs), CBLOGERR, CBERRALLOC, "\nerror in cb_compare, -7: re was null.");  return CBREWASNULL; }
	  err = cb_compare_regexp( &(*name2), len2, &(*mctl), &mcount); 
	  if(err>=CBERROR){ cb_log( &(*cbs), CBLOGERR, err, "\ncb_compare, -7: error in cb_compare_regexp, %i.", err); }
	  (*mctl).resmcount = mcount; // 9.8.2015
	  // 15.11.2015, do not free parameter re.
	}else if( (*mctl).matchctl==-6 ){ // %am%
	  if( len2 < len1 ){
	    dfr = len1-len2; // if len2-len1 is negative
	    err = CBMISMATCH; // 20.10.2015
	    //err = CBNOTFOUND;
	  }else{
	    dfr = len2-len1;
	    for(indx=0; ( indx<=dfr && err!=CBMATCH && err!=CBMATCHLENGTH ); indx+=4){ // %am%, greediness: compares dfr*len1 times per name
	      if( (**cbs).cf.asciicaseinsensitive==1 && (len2-indx) > 0 ){
	        err = cb_compare_rfc2822( &(*name1), len1, &(*name2), len2, indx );
	      }else if( (len2-indx) > 0 ){
	        err = cb_compare_strict( &(*name1), len1, &(*name2), len2, indx );
	      }else{
	        err = CBMISMATCH; // 20.10.2015
	        //err = CBNOTFOUND;
	      }
	    }
	  }
	}else if( (*mctl).matchctl==-5 ){ // %ame
	  if( len2 < len1 ){
	    dfr = len1-len2; // as if len2-len1 is negative
	    err = CBMISMATCH; // 20.10.2015
	    //err = CBNOTFOUND;
	  }else{
	    dfr = len2-len1;
	    if( (**cbs).cf.asciicaseinsensitive==1 && (len2-dfr) > 0 ){
	       err = cb_compare_rfc2822( &(*name1), len1, &(*name2), len2, dfr );
	    }else if( (len2-dfr) > 0 ){
	      err = cb_compare_strict( &(*name1), len1, &(*name2), len2, dfr );
	    }else{
	      err = CBMISMATCH; // 20.10.2015
	      //err = CBNOTFOUND;
	    }
	  }
	}else if( (*mctl).matchctl==0 ){ // all
	  stbp = &stb[0]; 
	  err = cb_compare_strict( &stbp, 0, &(*name2), len2, 0 );
	}else if((**cbs).cf.asciicaseinsensitive==1){ // name or nam%
	  err = cb_compare_rfc2822( &(*name1), len1, &(*name2), len2, 0 );
	}else{ // name or nam% , (or lexical compare if matchctl is -11, -12, -13 or -14)
	  err = cb_compare_strict( &(*name1), len1, &(*name2), len2, 0 );
	}

	switch ( (*mctl).matchctl) {
	  case  1:
	    if( err==CBMATCH )
	      return CBMATCH; // returns complitely the same or an error
	    break;
	  case  0:
	    if( err==CBMATCHLENGTH )
	      return CBMATCH; // any
	    break;
	  case -2:
	    if( err==CBMATCH || err==CBMATCHLENGTH )
	      return CBMATCH; // length (name can be cut for example nam matches name1)
	    break;
	  case -3:
	    if( err==CBMATCH || err==CBMATCHPART )
	      return CBMATCH; // part
	    break;
	  case -4:
	    if( err==CBMATCH || err==CBMATCHPART || err==CBMATCHLENGTH )
	      return CBMATCH; // part or length
	    break;
	  case -5:
	    if( err==CBMATCH || err==CBMATCHLENGTH )
	      return CBMATCH; // ( cut from end ame1 matches name1 )
	    break; // match from end
	  case -6:
	    if( err==CBMATCH || err==CBMATCHLENGTH )
	      return CBMATCH; // in the middle
	    break;
	  case -7: // new 18.3.2014, not yet tested, "strict re match"
	    if( err==CBMATCH || err==CBMATCHGROUP ) // do not match multiple 
	      return CBMATCH;
	    break;
	  case -8: // new 18.3.2014, not yet tested, "strict re match"
	    if( err==CBMATCH || err==CBMATCHGROUP ) // do not match multiple 
	      return CBMATCH;
	    break;
	  case -9: // new 18.4.2014, not yet tested
	    if( err==CBMATCH || err==CBMATCHGROUP || err==CBMATCHMULTIPLE )
	      return CBMATCH;
	    break;
	  case -10: // new 13.7.2014, not fully tested
	    if( err==CBMATCH || err==CBMATCHGROUP || err==CBMATCHMULTIPLE )
	      return CBMATCH;
	    break;
	  case -11: // new 31.7.2014, not tested, match lexically equal or less than
	    if( err==CBMATCH || err==CBMATCHPART || err==CBLESSTHAN )
	      return CBMATCH; // <=
	    break;
	  case -12: // new 31.7.2014, not tested, match lexically equal or greater than
	    if( err==CBMATCH || err==CBMATCHLENGTH || err==CBGREATERTHAN )
	      return CBMATCH; // =>
	    break;
	  case -13: // new 31.7.2014, not tested, match lexically less than
	    if( err==CBMATCHPART || err==CBLESSTHAN )
	      return CBMATCH; // <
	    break;
	  case -14: // new 31.7.2014, not tested, match lexically greater than
	    if( err==CBMATCHLENGTH || err==CBGREATERTHAN )
	      return CBMATCH; // >
	    break;
	  default:
	    return err; 
	}
	return err; // other information
}
int  cb_compare_strict(unsigned char **name1, int len1, unsigned char **name2, int len2, int from2){ // from2 23.11.2013
	signed err=CBSUCCESS, cmp=0;
	int indx1=0, indx2=0, num=0;
	char stp=0;
	if( name1==NULL || name2==NULL || *name1==NULL || *name2==NULL )
	  return CBERRALLOC;

	//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_compare_strict: name1=[");
	//cb_print_ucs_chrbuf( CBLOGDEBUG, &(*name1), len1, len1); cb_clog( CBLOGDEBUG, CBNEGATION, "] name2=[");
	//cb_print_ucs_chrbuf( CBLOGDEBUG, &(*name2), len2, len2); cb_clog( CBLOGDEBUG, CBNEGATION, "] from %i ", from2);

	num=len1;
	if(len1>len2)
	  num=len2;
	indx1=0;
	for(indx2=from2; stp!=1 && indx1<num && indx2<num && err<CBNEGATION; ++indx2){
	  if( (*name1)[indx1]!=(*name2)[indx2] ){
	    // indx2=num+7; err=CBNOTFOUND;
	    //err=CBNOTFOUND; // 17.8.2015
	    err=CBMISMATCH; // 17.8.2015, 20.10.2015

            cmp = (signed int) cb_from_ucs_to_host_byte_order( (unsigned long int) (*name1)[indx1] );  // 31.7.2014
            cmp -= (signed int) cb_from_ucs_to_host_byte_order( (unsigned long int) (*name2)[indx2] ); // 31.7.2014

	    if( len1 > (len2-from2) || ( len1==(len2-from2) && cmp>0 ) )
	      err = CBGREATERTHAN; 	// 31.7.2014
	    else if( len1 < (len2-from2) || ( len1==(len2-from2) && cmp<0 ) )
	      err = CBLESSTHAN;    	// 31.7.2014
	    stp=1; // 17.8.2015
	  }else{
	    if( err < CBNEGATION ){
	      if( len1 > (len2-from2) )
	        err = CBMATCHPART; // 30.3.2013, was: match, not notfound
	      else if( len1 < (len2-from2) )
	        err = CBMATCHLENGTH; // 30.3.2013, was: match, not notfound, 31.7.2014
	    }
	  }
	  ++indx1;
	}
	if( err < CBNEGATION ){
	  if( len1==(len2-from2) )
	    err=CBMATCH;
	  else if( len2 > (len1+from2) ) // 9.11.2013
	    err=CBMATCHLENGTH;
	  else if( len2 < (len1+from2) ) // 9.11.2013
	    err=CBMATCHPART;
	}
	return err;
}

/*
 * To use compare -7 and -8 . 4-byte text with PCRE UTF-32 (in place of UCS). */
int  cb_get_matchctl(CBFILE **cbf, unsigned char **pattern, int patsize, unsigned int options, cb_match *ctl, int matchctl){
	if(ctl==NULL){ cb_log( &(*cbf), CBLOGALERT, CBERRALLOC, "\ncb_get_matchctl: allocation error."); return CBERRALLOC; }
	if(cbf!=NULL && *cbf!=NULL)
	  if( (**cbf).cf.asciicaseinsensitive==1)
	    options = options | (unsigned int) PCRE2_CASELESS; // can be set to another here
	options = options | (unsigned int) PCRE2_UCP; // unicode properties
        /** You should include PCRE2_UTF for proper UTF-8, UTF-16, or UTF-32 support. If you omit 
            it, you get pure 8-bit, or UCS-2, or UCS-4 character handling. [http://www.regular-expressions.info/pcre2.html] **/
	// UCS-4, options =  options | (unsigned int) PCRE2_UTF; // pcre2_utf is different in pcre2 (?) 21.10.2015
	return cb_compare_get_matchctl(&(*pattern), patsize, options, &(*ctl), matchctl);
}

int  cb_compare_get_matchctl(unsigned char **pattern, int patsize, unsigned int options, cb_match *ctl, int matchctl){
        unsigned char *hbpat = NULL; int indx=0, bufindx=0; unsigned long int chr = 0;
        int err=CBSUCCESS, err2=CBSUCCESS;

	if( pattern==NULL || *pattern==NULL || ctl==NULL ){
          cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_compare_get_matchctl: allocation error, %i.", CBERRALLOC);
	  return CBERRALLOC;
	}

        /*
         * Pattern in host byte order */
        hbpat = (unsigned char*) malloc( sizeof(char)*( (unsigned int) patsize+1 ) ); // '\0' + argumentsize
        //hbpat = (unsigned char*) malloc( sizeof(char)*( (unsigned int) patsize+5 ) ); // '\0' + argumentsize, 21.10.2015, test UTF BOM
        if( hbpat==NULL ){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_compare_get_matchctl: allocation error, host byte pattern."); return CBERRALLOC; }
        memset( &(*hbpat), (int) 0x20, (size_t) patsize );
        //memset( &(*hbpat), (int) 0x20, (size_t) (patsize+4) ); // test BOM
        hbpat[patsize]='\0';
        //hbpat[patsize+4]='\0'; // test BOM

	/*
	 * 21.10.2015: TEST UTF 32-bit BOM. */
	//chr = 0xFEFF;
	//cb_clog( CBLOGDEBUG, CBNEGATION, "[0x%.4x]", (unsigned char) chr );
	//chr = cb_from_ucs_to_host_byte_order( chr );
	//cb_clog( CBLOGDEBUG, CBNEGATION, "[0x%.4x]", (unsigned char) chr );
	//cb_put_ucs_chr( chr, &hbpat, &bufindx, patsize);

        /*
         * Copy to host byte order */
        for( indx=0; indx<patsize && bufindx<patsize && err==CBSUCCESS && err2==CBSUCCESS; ){
	  if(err>=CBERROR){ cb_clog( CBLOGDEBUG, err, "\ncb_compare_get_matchctl: cb_get_ucs_chr, error %i.", err); }
          err = cb_get_ucs_chr( &chr, &(*pattern), &indx, patsize );
	  //cb_clog( CBLOGDEBUG, CBNEGATION, "[%c]", (unsigned char) chr );
          chr = cb_from_ucs_to_host_byte_order( chr );
	  //cb_clog( CBLOGDEBUG, CBNEGATION, "[0x%.4lx]", chr );
          if( chr != 0xFEFF && chr != 0xFFFE ) // bom
            err2 = cb_put_ucs_chr( chr, &hbpat, &bufindx, patsize);
	  if(err2>=CBERROR){ cb_clog( CBLOGDEBUG, err, "\ncb_compare_get_matchctl: cb_put_ucs_chr, error %i.", err); }
        } 
        hbpat[bufindx] = '\0';
        err=CBSUCCESS; err2=CBSUCCESS;

        //fprintf(stderr,"\ncb_compare_get_matchctl: pattern in host byte order (hbpat) : [");
        //cb_print_ucs_chrbuf( CBLOGDEBUG, &hbpat, bufindx, patsize );
        //fprintf(stderr,"] err %i, patsize %i.", err, patsize );

        /* hbpat has to be null terminated */
        err2 = cb_compare_get_matchctl_host_byte_order( &hbpat, bufindx, options, &(*ctl), matchctl);
        if(err2>=CBERROR){ cb_clog( CBLOGDEBUG, err, "\ncb_compare_get_matchctl: cb_compare_get_matchctl_host_byte_order, error %i.", err2 );}
        free(hbpat);
        return err2;
}


int  cb_compare_get_matchctl_host_byte_order(unsigned char **pattern, int patsize, unsigned int options, cb_match *ctl, int matchctl){
	int errcode=0; // , err=CBSUCCESS;
	PCRE2_SIZE erroffset=0;
	PCRE2_SPTR32 sptr = NULL; // PCRE_SPTR32 vastaa const unsigned int*

	if(ctl==NULL){
	  cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_compare_get_matchctl: allocation error, ctl was null.");
          return CBERRALLOC;
	}
	(*ctl).matchctl = matchctl;
	//if(pattern==NULL || *pattern==NULL){ // return empty cb_match if pattern was null, 21.10.2015, pattern character may be null if length is 0
	if( pattern==NULL ){ // return empty cb_match if pattern was null
	  cb_clog( CBLOGNOTICE, CBNEGATION, "\ncb_compare_get_matchctl: pattern was null.");
	  return CBREPATTERNNULL;
	}
	pcre2_code_free_32( (pcre2_code_32*) (*ctl).re );
	(*ctl).re = NULL;


/**

[http://www.regular-expressions.info/pcre2.html]

Before you can use a regular expression, it needs to be converted into a binary format for improved efficiency. To do this, simply call pcre2_compile() 
passing your regular expression as a string. If the string is null-terminated, you can pass PCRE2_ZERO_TERMINATED as the second parameter. Otherwise, 
pass the length in code units as the second parameter. For UTF-8 this is the length in bytes, while for UTF-16 or UTF-32 this is the length in bytes 
divided by 2 or 4. 

 **/

	/*
	 * Compile pattern to re. Default character tables. */
	sptr = &(** (PCRE2_SPTR32*)  pattern);
	(*ctl).re = &(* (PSIZE) pcre2_compile_32( sptr, (PCRE2_SIZE) (patsize/4), (uint32_t) options, &errcode, &erroffset, NULL ) );

	if( (*ctl).re==NULL ){
          cb_clog( CBLOGERR, CBERRREGEXCOMP, "\ncb_compare_get_matchctl: error compiling re, re is null, offset %i, at re offset %i.", erroffset, errcode);
          return CBERRREGEXCOMP;
        }

	return CBSUCCESS;
}
/*
 * Copies name2 to subject string block in host byte order (pcre32) and compares block by block
 * from start of name2 or from overlapsize to the end of block. Compares compiled regexp to 4-byte 
 * string name2 in blocks using pcre32. */
int  cb_compare_regexp(unsigned char **name2, int len2, cb_match *mctl, int *matchcount){
	unsigned char *ucsdata = NULL;
	int nameindx=0, bufindx=0;
	unsigned long int chr = 0x20;
	unsigned int opt=0;
	int err=CBSUCCESS, err2=CBSUCCESS, terr=CBMISMATCH;

	if( name2==NULL || *name2==NULL || mctl==NULL || matchcount==NULL){ 
	  cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_compare_regexp: allocation error."); 
	  return CBERRALLOC;
	}
	if( (*mctl).re==NULL ){
	  cb_clog( CBLOGINFO, CBREWASNULL, "\ncb_compare_regexp: re was null (possibly because of an error in the regular expression text)."); 
	  return CBREWASNULL;
	}

	//fprintf(stderr,"\ncb_compare_regexp: [");

	/* Allocate subject block to match in parts. */	
        ucsdata = (unsigned char*) malloc( sizeof(char)*( CBREGSUBJBLOCK+1 ) ); // + '\0', bom is ignored
        if( ucsdata==NULL ){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_compare_regexp: ucsdata, allocation error %i.", CBERRALLOC ); return CBERRALLOC; }
        memset( &(*ucsdata), (int) 0x20, (size_t) CBREGSUBJBLOCK );
        ucsdata[CBREGSUBJBLOCK]='\0';

	/*
	 * Block by block. (Strings searched may not overlap.) */
	err = cb_get_ucs_chr(&chr, &(*name2), &nameindx, CBREGSUBJBLOCK );
	while( chr!= (unsigned long int) EOF && nameindx<=len2 && bufindx<=len2 && err==CBSUCCESS ){
	  if(chr!=0xFEFF)
	    err = cb_put_ucs_chr( cb_from_ucs_to_host_byte_order( (unsigned int) chr), &ucsdata, &bufindx, CBREGSUBJBLOCK);
	  if(bufindx>=(CBREGSUBJBLOCK-5) || chr== (unsigned long int) EOF || err==CBARRAYOUTOFBOUNDS || bufindx==len2 ){ // next block
	    if( chr!=(unsigned char)EOF ){
	      opt = opt | (unsigned int) PCRE2_NOTEOL; // Subject string is not the end of a line
	    }else{
	      opt = opt & ~PCRE2_NOTEOL; // Last block
	    }
	    terr = cb_compare_regexp_one_block(&ucsdata, bufindx, 0, &(*mctl), &(*matchcount) );

            //fprintf(stderr,"\ncb_compare_regexp: block:[");
            //cb_print_ucs_chrbuf( CBLOGDEBUG, &(*name2), len2, len2);
            //fprintf(stderr,"] , returns %i. (bufindx=%d, nameindx=%d, len2=%d)", terr, bufindx, nameindx, len2);

            if(terr>=CBERROR )
              cb_clog( CBLOGERR, terr, "\ncb_compare_regexp: cb_compare_regexp_one_block error %i.", terr);
            if(terr>=CBNEGATION && terr<CBERROR){
              ; // fprintf(stderr," err %i.", terr);
            }else if(terr==CBMATCH){
              ; // fprintf(stderr," match, %i.", terr);
            }else if(terr==CBMATCHGROUP){
              ; // fprintf(stderr," match group, %i.", terr);
            }else if(terr==CBMATCHMULTIPLE){
              ; // fprintf(stderr," multiple matches, %i.", terr);
            }else
              ; // fprintf(stderr," %i.", terr);
	    if(terr==CBMATCH || terr==CBMATCHMULTIPLE || terr==CBMATCHGROUP){
	      free(ucsdata);
	      return terr; // at least one match was found, return
	    }
            err2 = cb_copy_ucs_chrbuf_from_end(&ucsdata, &bufindx, CBREGSUBJBLOCK, CBREGOVERLAPSIZE ); // copies range: (bufindx-OVERLAPSIZE) ... bufindx
            if(err2!=CBSUCCESS){ cb_clog( CBLOGERR, err2, "\ncb_compare_regexp: Error in cb_copy_from_end: %i.", err2); }
	    opt = opt | PCRE2_NOTBOL; // Subject string is not the beginning of a line
	  }
	  err = cb_get_ucs_chr(&chr, &(*name2), &nameindx, CBREGSUBJBLOCK );
	}

	//fprintf(stderr,"]");

	free(ucsdata);
	return terr;
}
/*
 * Compares compiled regexp to 4-byte string name2 using 32-bit functions. */
int  cb_compare_regexp_one_block(unsigned char **name2, int len2, int startoffset, cb_match *mctl, int *matchcount){
	int opt=0, err=-1, pcrerc=0, oveccount=-1;
        int groupcount=0;
	PCRE2_SIZE *ovector=NULL; 
	pcre2_match_data_32 *match_data=NULL;
	const pcre2_code_32 *pcrecode = NULL; 
        if( name2==NULL || *name2==NULL || mctl==NULL ){ 
          cb_clog( CBLOGERR, CBERRALLOC, "\ncb_compare_regexp: allocation error."); 
          return CBERRALLOC;
        }
	if( (*mctl).re==NULL ){
	  cb_clog( CBLOGINFO, CBREWASNULL, "\ncb_compare_regexp: re was null (possibly because of an error in the regular expression text).");
	  return CBREWASNULL;
	}

        //cb_compare_print_fullinfo( &(*mctl) );

// pcre2_match_data_32 'struct pcre2_real_match_data_32'	'typedef struct pcre2_real_match_data { ... } pcre2_real_match_data;' 	src/pcre2_intmodedep.h
// pcre2_code_32       'const struct pcre2_real_code_32'	'typedef struct pcre2_real_code { ... } pcre2_real_code.' 	src/pcre2_intmodedep.h
// PCRE2_UCHAR32       'const unsigned int'
// PCRE2_SPTR32	       'const unsigned int *'
// PCRE2_SIZE          'size_t'

	pcrecode = &(* (const pcre2_code_32*) (*mctl).re );
	match_data = &(* (pcre2_match_data_32*) pcre2_match_data_create_from_pattern_32( pcrecode, NULL ) );
	if(match_data==NULL)
	  cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_compare_regexp_one_block: match_data was null.");

pcre_match_another:

	/** pcre2: The length and starting offset are in code units (not characters).
	    The length of the entire string. [http://www.regular-expressions.info/pcre2.html] **/
        pcrerc = pcre2_match_32( pcrecode, (* (PCRE2_SPTR32*) name2 ), (PCRE2_SIZE) (len2/4), \
		(PCRE2_SIZE) startoffset, (uint32_t) opt, &(*match_data), NULL );

	//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_compare_regexp_one_block: pcre2_exec_32 PCRERC %i.", pcrerc );
	if( pcrerc<0 ){
		cb_clog( CBLOGDEBUG, CBNEGATION, "" );
	}

	/** startoffset: The length and starting offset are in code units, not characters. [http://www.regular-expressions.info/pcre2.html] **/

	oveccount = (int) pcre2_get_ovector_count( &(*match_data) );

        if( pcrerc >= 0 ){ // one|many or groupmatch
          *matchcount+=1;
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
	  ovector = pcre2_get_ovector_pointer_32( &(*match_data) );
          startoffset = (int) ovector[0];
	  if( startoffset>0 )
            goto pcre_match_another;
        }else if( pcrerc==PCRE2_ERROR_NOMATCH && err==-1){
          err = CBMISMATCH; // 20.10.2015
        }else if( pcrerc!=PCRE2_ERROR_NOMATCH && pcrerc!=PCRE2_ERROR_PARTIAL){ // error codes: man pcreapi
          if( pcrerc>=PCRE2_ERROR_UTF32_ERR2 && pcrerc<=PCRE2_ERROR_UTF8_ERR1 ){ // 20.10.2015
            cb_clog( CBLOGWARNING, CBNEGATION, "\ncb_compare_regexp_one_block: invalid UTF.");
	  }else{
            cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_compare_regexp_one_block: pcre2_match_32, %i.", pcrerc );
	  }
          if(err==-1)
            err = CBERRREGEXEC;
        }else if( pcrerc==PCRE2_ERROR_NOMATCH || pcrerc==PCRE2_ERROR_PARTIAL ){
          err = CBMISMATCH; // 20.10.2015
	}
	pcre2_match_data_free_32(match_data);
        return err;
}

/* /compare -7 and -8 */

/*
 * pcre2 info. */
int  cb_compare_print_fullinfo(cb_match *mctl){
	int res=0;
	pcre2_code_32 *re = NULL;
	unsigned char *cres = NULL;
	//unsigned char  errmsg[6] = {'e','r','r','o','r','\0'};
	unsigned char  errmsg[1024];
        memset( &(errmsg[0]), (int) 0x20, (size_t) 1024 );
	cres = &(*errmsg);

	if(mctl==NULL){  return CBERRALLOC; }

	if((*mctl).re!=NULL){ re = &(* (pcre2_code_32*) (*mctl).re); } // 20.10.2015

	cb_clog( CBLOGINFO, CBNEGATION, "\ncb_match:");	
	cb_clog( CBLOGINFO, CBNEGATION, "\n\tmatchctl=%i", (*mctl).matchctl );	
	if( re != NULL ){ // const
	  res = pcre2_pattern_info_32( re, PCRE2_INFO_SIZE, &(* (PSIZE) cres) );
	  if(res==0){ 	cb_clog( CBLOGINFO, CBNEGATION, "\n\tsize=%i (, %i, %i)", cres[0], cres[1], cres[2] ); }
	  res = pcre2_pattern_info_32( re, PCRE2_INFO_NAMECOUNT, &(* (PSIZE) cres) );
	  if(res==0){ 	cb_clog( CBLOGINFO, CBNEGATION, "\n\tsubpatterns=%i", (unsigned int) ( (0|cres[0])<<8 | cres[1] ) ); }
	  res = pcre2_pattern_info_32( re, PCRE2_INFO_ALLOPTIONS, &(* (PSIZE) cres) );
	  if(res==0){ 	cb_clog( CBLOGINFO, CBNEGATION, "\n\toptions=%i", (unsigned int) ( (0|cres[0])<<8 | cres[1] ) ); }
	}
	return CBSUCCESS;
}

