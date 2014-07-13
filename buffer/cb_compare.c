/* 
 * Library to read and write streams. Valuepair list and search. Different character encodings.
 * 
 * Copyright (C) 2009, 2010 and 2013. Jouni Laakso
 * 
 * This library is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General
 * Public License version 2.1 as published by the Free Software Foundation 6. of June year 2012;
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
int  cb_compare_get_matchctl_host_byte_order(unsigned char **pattern, int patsize, int options, cb_match *ctl, int matchctl);

int  cb_compare_rfc2822(unsigned char **name1, int len1, unsigned char **name2, int len2, int from2){ // from2 23.11.2013
	unsigned long int chr1=0x65, chr2=0x65;
	int indx1=0, indx2=0, err1=CBSUCCESS, err2=CBSUCCESS;
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

	  if(err1>=CBNEGATION || err2>=CBNEGATION){
	    return CBNOTFOUND;
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
	  return CBNOTFOUND;
	}

	if( len1==len2 )
	  return CBMATCH;
	else if( len2>len1 ) // 9.11.2013, 23.11.2013
	  return CBMATCHLENGTH;
	else if( len2<len1 || len1>(len2-from2)) // 9.11.2013, 23.11.2013
	  return CBMATCHPART;

	return err1;
}
/*
 * Compares if name1 matches name2. CBMATCHPART returns if name1 is longer
 * than name2. CBMATCHLENGTH returns if name1 is shorter than name2.
 */
int  cb_compare(CBFILE **cbs, unsigned char **name1, int len1, unsigned char **name2, int len2, cb_match *mctl){
	int err = CBSUCCESS, dfr = 0, indx = 0, mcount = 0;
	unsigned char stb[3] = { 0x20, 0x20, '\0' };
	unsigned char *stbp = NULL;
	cb_match newmctl;
	stbp = &stb[0];
	newmctl.re=NULL; newmctl.re_extra=NULL; newmctl.matchctl=1;

	if(mctl==NULL){ fprintf(stderr,"\ncb_compare: allocation error, cb_match."); return CBERRALLOC; }

	//fprintf(stderr,"\ncb_compare, name1: [");
	//cb_print_ucs_chrbuf(&(*name1), len1, len1); fprintf(stderr,"] name2: [");
	//cb_print_ucs_chrbuf(&(*name2), len2, len2); fprintf(stderr,"] len1: %i, len2: %i, matchctl %i.", len1, len2, (*mctl).matchctl );

	if((*mctl).matchctl==-1)
	  return CBNOTFOUND; // no match
	if( cbs==NULL || *cbs==NULL )
	  return CBERRALLOC;
	if( (*mctl).matchctl==-8 || (*mctl).matchctl==-10 ){ // new 18.3.2014, not yet tested 18.3.2014
	  /*
	   * Compiles regexp from pattern just before search.
	   * name1 has to be NULL -terminated string.
	   */
	  if(name1==NULL){	fprintf(stderr,"\nerror in cb_compare, -8: name1 was null.");  return CBERRALLOC; }

	  err = cb_get_matchctl( &(*cbs), &(*name1), len1, 0, &newmctl, (*mctl).matchctl ); // 13.4.2014
	  if(err!=CBSUCCESS){ fprintf(stderr,"\ncb_compare, -8: error in cb_get_matchctl, %i.", err); }
	  err = cb_compare_regexp( &(*name2), len2, 0, 0, &newmctl, &mcount);

	  if(err>=CBERROR){ fprintf(stderr,"\ncb_compare, -8: error in cb_compare_regexp, %i.", err); }
	}else if( (*mctl).matchctl==-7 || (*mctl).matchctl==-9 ){ // new 18.3.2014, not yet tested 18.3.2014
	  /*
	   * Uses only compiled regexp inside mctl.
	   * name1 may be null and len1 is ignored. 
	   * Pattern has to be compiled and attached
	   * to mctl before and elsewhere. name2 has
	   * to be NULL terminated.
	   */
	  if( (*mctl).re==NULL){	fprintf(stderr,"\nerror in cb_compare, -7: re was null.");  return CBERRALLOC; }
	  err = cb_compare_regexp( &(*name2), len2, 0, 0, &(*mctl), &mcount);
	  if(err>=CBERROR){ fprintf(stderr,"\ncb_compare, -7: error in cb_compare_regexp, %i.", err); }
	}else if( (*mctl).matchctl==-6 ){ // %am%
	  dfr = len2 - len1; // index of name2 to start searching
	  if(dfr<0){
	    err = CBNOTFOUND;
	  }else{
	    for(indx=0; ( indx<=dfr && err!=CBMATCH && err!=CBMATCHLENGTH ); indx+=4){ // %am%, greediness: compares dfr*len1 times per name
	      if( (**cbs).cf.asciicaseinsensitive==1 && (len2-indx) > 0 ){
	        err = cb_compare_rfc2822( &(*name1), len1, &(*name2), len2, indx );
	      }else if( (len2-indx) > 0 ){
	        err = cb_compare_strict( &(*name1), len1, &(*name2), len2, indx );
	      }else{
	        err = CBNOTFOUND;
	      }
	    }
	  }
	}else if( (*mctl).matchctl==-5 ){ // %ame
	  dfr = len2 - len1;
	  if(dfr<0){
	    err = CBNOTFOUND;
	  }else if( (**cbs).cf.asciicaseinsensitive==1 && (len2-dfr) > 0 ){
	    err = cb_compare_rfc2822( &(*name1), len1, &(*name2), len2, dfr );
	  }else if( (len2-dfr) > 0 ){
	    err = cb_compare_strict( &(*name1), len1, &(*name2), len2, dfr );
	  }else{
	    err = CBNOTFOUND;
	  }
	}else if( (*mctl).matchctl==0 ){ // all
	  stbp = &stb[0]; 
	  err = cb_compare_strict( &stbp, 0, &(*name2), len2, 0 );
	}else if((**cbs).cf.asciicaseinsensitive==1){ // name or nam%
	  err = cb_compare_rfc2822( &(*name1), len1, &(*name2), len2, 0 );
	}else{ // name or nam%
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
	    break;
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
	  default:
	    return err; 
	}
	return err; // other information
}
int  cb_compare_strict(unsigned char **name1, int len1, unsigned char **name2, int len2, int from2){ // from2 23.11.2013
	int indx1=0, indx2=0, err=CBMATCHLENGTH, num=0;
	if( name1==NULL || name2==NULL || *name1==NULL || *name2==NULL )
	  return CBERRALLOC;

	//fprintf(stderr,"cb_compare: name1=[");
	//cb_print_ucs_chrbuf(&(*name1), len1, len1); fprintf(stderr,"] name2=[");
	//cb_print_ucs_chrbuf(&(*name2), len2, len2); fprintf(stderr,"]\n");

	num=len1;
	if(len1>len2)
	  num=len2;
	indx1=0;
	for(indx2=from2; indx1<num && indx2<num ;++indx2){
	  if((*name1)[indx1]!=(*name2)[indx2]){
	    indx2=num+7; err=CBNOTFOUND;
	  }else{
	    if( len1==len2 || len1==(len2-from2) )
	      err=CBMATCH;
	    else
	      err=CBMATCHPART; // 30.3.2013, was: match, not notfound
	  }
	  ++indx1;
	}
	if( len1==len2 )
	  err=CBMATCH;
	else if( len2>len1 ) // 9.11.2013
	  err=CBMATCHLENGTH;
	else if( len2<len1 ) // 9.11.2013
	  err=CBMATCHPART;

	return err;
}

/*
 * To use compare -7 and -8 . 4-byte text with PCRE UTF-32 (in place of UCS). */
int  cb_get_matchctl(CBFILE **cbf, unsigned char **pattern, int patsize, int options, cb_match *ctl, int matchctl){
	if(ctl==NULL){ fprintf(stderr,"\ncb_get_matchctl: allocation error."); return CBERRALLOC; }
	if(cbf!=NULL && *cbf!=NULL)
	  if( (**cbf).cf.asciicaseinsensitive==1)
	    options = options | PCRE_CASELESS; // can be set to another here
	options = options | PCRE_UCP; // unicode properties

	return cb_compare_get_matchctl(&(*pattern), patsize, options, &(*ctl), matchctl);
}

int  cb_compare_get_matchctl(unsigned char **pattern, int patsize, int options, cb_match *ctl, int matchctl){
	unsigned char *hbpat = NULL; int indx=0, bufindx=0; unsigned long int chr = 0;
	int err=CBSUCCESS, err2=CBSUCCESS;

        /*
	 * Pattern in host byte order */
        hbpat = (unsigned char*) malloc( sizeof(char)*( patsize+1 ) ); // '\0' + argumentsize
        if( hbpat==NULL ){ fprintf(stderr,"\ncb_compare_get_matchctl: allocation error, host byte pattern."); return CBERRALLOC; }
        memset( &(*hbpat), (int) 0x20, (size_t) patsize );
        hbpat[patsize]='\0';

	/*
	 * Copy to host byte order */
        for( indx=0; indx<patsize && bufindx<patsize && err==CBSUCCESS && err2==CBSUCCESS; ){
	  err = cb_get_ucs_chr( &chr, &(*pattern), &indx, patsize );
	  chr = cb_from_ucs_to_host_byte_order( chr );
	  if( chr != 0xFEFF ) // bom
            err2 = cb_put_ucs_chr( chr, &hbpat, &bufindx, patsize);
        } 
  	hbpat[bufindx] = '\0';
	err=CBSUCCESS; err2=CBSUCCESS;

	//fprintf(stderr,"\ncb_compare_get_matchctl: pattern in host byte order [");
	//err = cb_print_ucs_chrbuf( &(*pattern), patsize, patsize );
	//fprintf(stderr,"] err %i, patsize %i.", err, patsize );

	err2 = cb_compare_get_matchctl_host_byte_order( &hbpat, patsize, options, &(*ctl), matchctl);
	free(hbpat);
	return err2;
}
int  cb_compare_get_matchctl_host_byte_order(unsigned char **pattern, int patsize, int options, cb_match *ctl, int matchctl){
	const char *errptr = NULL; int errcode=0, erroffset=0; // , err=CBSUCCESS; 
	const char  errmsg[6] = {'e','r','r','o','r','\0'};	
	PCRE_SPTR32 sptr = NULL; // PCRE_SPTR32 vastaa const unsigned int*
	errptr = &errmsg[0];

	if(ctl==NULL){
	  fprintf(stderr, "\ncb_compare_get_matchctl: allocation error, ctl was null.");
          return CBERRALLOC;
	}
	(*ctl).matchctl = matchctl;
	if(pattern==NULL || *pattern==NULL){ // return empty cb_match if pattern was null
	  fprintf(stderr, "\ncb_compare_get_matchctl: pattern was null.");
	  return CBREPATTERNNULL;
	}
	(*ctl).re = NULL; (*ctl).re_extra = NULL;

	//fprintf(stderr,"\ncb_compare_get_matchctl_host_byte_order: pattern [");
	//err = cb_print_ucs_chrbuf( &(*pattern), patsize, patsize );
	//fprintf(stderr,"] err %i, patsize %i.", err, patsize );

/*
       Kaytetaan 32-bittisia funktioita mutta ei aseteta UTF-32:ta. Tekstin 
       on oltava muodossa "host byte order".

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

	sptr = (PCRE_SPTR32)  *pattern; // const

	(*ctl).re = &(* (PSIZE) pcre32_compile2( sptr, options, &errcode, &errptr, &erroffset, NULL ) ); // PCRE_SPTR32 vastaa const unsigned int*, 29.5.2014, 16.6.2014

	//fprintf(stderr,"\ncb_compare_get_matchctl: compiled pattern: [");
	//cb_print_ucs_chrbuf(&(*pattern), patsize, patsize);
	//fprintf(stderr,"] size %d, errcode %i.", patsize, errcode); // man pcreapi # /COMPILATION ERROR CODES

	if( (*ctl).re==NULL ){
          fprintf(stderr,"\ncb_compare_get_matchctl: error compiling re: \"%s\" offset %i, at re offset %i.", errptr, erroffset, errcode);
          return CBERRREGEXCOMP;
        }else{
	  (*ctl).re_extra = &(* (PSIZE)  pcre32_study( (const pcre32*) (*ctl).re, 0, &errptr ) ); // constant pointers can not change, 29.5.2014, 16.6.2014
	  if( (*ctl).re_extra==NULL ){
            fprintf(stderr,"\ncb_compare_get_matchctl: studied re_extra was null: \"%s\" .", errptr );
	  }
	  if(errptr!=NULL){
            fprintf(stderr,"\ncb_compare_get_matchctl: at study, errptr was \"%s\" .", errptr );
	  }
	}

	return CBSUCCESS;
}
/*
 * Copies name2 to subject string block in host byte order (pcre32) and compares block by block
 * from start of name2 or from overlapsize to the end of block. Compares compiled regexp to 4-byte 
 * string name2 in blocks using pcre32. */
int  cb_compare_regexp(unsigned char **name2, int len2, int startoffset, int options, cb_match *mctl, int *matchcount){
	unsigned char *ucsdata = NULL;
	int nameindx=0, bufindx=0;
	unsigned long int chr = 0x20;
	int err=CBSUCCESS, err2=CBSUCCESS, terr=CBNOTFOUND, opt=0;

	if( name2==NULL || *name2==NULL || mctl==NULL || (*mctl).re==NULL || matchcount==NULL){ 
	  fprintf(stderr,"\ncb_compare_regexp: allocation error."); 
	  return CBERRALLOC;
	}

	//fprintf(stderr,"\ncb_compare_regexp: [");

	/* Allocate subject block to match in parts. */	
        ucsdata = (unsigned char*) malloc( sizeof(char)*( CBREGSUBJBLOCK+1 ) ); // + '\0', bom is ignored
        if( ucsdata==NULL ){ fprintf(stderr,"\nAllocation error, ucsdata."); return CBERRALLOC; }
        memset( &(*ucsdata), (int) 0x20, (size_t) CBREGSUBJBLOCK );
        ucsdata[CBREGSUBJBLOCK]='\0';

	/*
	 * Block by block. (Strings searched may not overlap.) */
	err = cb_get_ucs_chr(&chr, &(*name2), &nameindx, CBREGSUBJBLOCK );
	while( chr!=EOF && nameindx<=len2 && bufindx<=len2 && err==CBSUCCESS){
	  //fprintf(stderr,"%c", (unsigned char) chr);
	  if(chr!=0xFEFF)
	    err = cb_put_ucs_chr( cb_from_ucs_to_host_byte_order( (unsigned int) chr), &ucsdata, &bufindx, CBREGSUBJBLOCK);
	  if(bufindx>=(CBREGSUBJBLOCK-5) || chr==EOF || err==CBARRAYOUTOFBOUNDS || bufindx==len2 ){ // next block
	    if( chr!=(unsigned char)EOF ){
	      opt = opt | PCRE_NOTEOL; // Subject string is not the end of a line
	    }else{
	      opt = opt & ~PCRE_NOTEOL; // Last block
	    }
	    terr = cb_compare_regexp_one_block(&ucsdata, bufindx, 0, 0, &(*mctl), &(*matchcount) );

            //fprintf(stderr,"\ncb_compare_regexp: block:[");
            //cb_print_ucs_chrbuf(&(*name2), len2, len2);
            //fprintf(stderr,"] , returns %i. (bufindx=%d, nameindx=%d, len2=%d)", err, bufindx, nameindx, len2);

            if(terr>=CBERROR )
              fprintf(stderr," error %i.", terr);
            if(terr>=CBNEGATION && terr<CBERROR){
              fprintf(stderr," err %i.", terr);
            }else if(terr==CBMATCH){
              fprintf(stderr," match, %i.", terr);
            }else if(terr==CBMATCHGROUP){
              fprintf(stderr," match group, %i.", terr);
            }else if(terr==CBMATCHMULTIPLE){
              fprintf(stderr," multiple matches, %i.", terr);
            }else
              fprintf(stderr," %i.", terr);
	    if(terr==CBMATCH || terr==CBMATCHMULTIPLE || terr==CBMATCHGROUP){
	      free(ucsdata);
	      return terr; // at least one match was found, return
	    }
            err2 = cb_copy_ucs_chrbuf_from_end(&ucsdata, &bufindx, CBREGSUBJBLOCK, CBREGOVERLAPSIZE ); // copies range: (bufindx-OVERLAPSIZE) ... bufindx
            if(err2!=CBSUCCESS){ fprintf(stderr,"\nError in cb_copy_from_end_to_start: %i.", err); }
	    opt = opt | PCRE_NOTBOL; // Subject string is not the beginning of a line
	  }
	  err = cb_get_ucs_chr(&chr, &(*name2), &nameindx, CBREGSUBJBLOCK );
	}

	//fprintf(stderr,"]");

	free(ucsdata);
	return terr;
}
/*
 * Compares compiled regexp to 4-byte string name2 using 32-bit functions. */
int  cb_compare_regexp_one_block(unsigned char **name2, int len2, int startoffset, int options, cb_match *mctl, int *matchcount){
	int opt=0, err=CBSUCCESS, pcrerc=0;
        int groupcount=0;
        int ovec[OVECSIZE+1]; int ovecsize=OVECSIZE;
        if( name2==NULL || *name2==NULL || mctl==NULL || (*mctl).re==NULL ){ 
          fprintf(stderr,"\ncb_compare_regexp: allocation error."); 
          return CBERRALLOC;
        }

        memset(&(ovec), (int) 0x20, (size_t) OVECSIZE); // zero
        ovec[OVECSIZE] = '\0';

        //cb_compare_print_fullinfo( &(*mctl) );

pcre_match_another:
        // 3. const PCRE_SPTR32 subject
        pcrerc = pcre32_exec( (pcre32*) (*mctl).re, (pcre32_extra*) (*mctl).re_extra, ( (PCRE_SPTR32) *name2), len2, startoffset, opt, &(* (int*) ovec), ovecsize );

        //fprintf(stderr,"\ncb_compare_regexp_one_block: block:[");
        //cb_print_ucs_chrbuf(&(*name2), len2, len2);
        //fprintf(stderr,"]");

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
/*
       pcre.txt:
       The first pair of integers, ovector[0]  and  ovector[1],  identify  the
       portion  of  the subject string matched by the entire pattern. The next
       pair is used for the first capturing subpattern, and so on.  The  value
       returned by pcre_exec() is one more than the highest numbered pair that
       has been set. 
 */
          startoffset = ovec[1];
          //fprintf(stderr, "\ncb_compare_regexp: next, startoffset=%i.", startoffset);
          goto pcre_match_another;
        }else if( pcrerc == PCRE_ERROR_NOMATCH && err==CBSUCCESS){
          err = CBNOTFOUND;
        }else if( pcrerc!=PCRE_ERROR_NOMATCH && pcrerc!=PCRE_ERROR_PARTIAL){ // error codes: man pcreapi
          fprintf(stderr, "\ncb_compare_regexp: pcre32_exec returned error %i.", err); // PCRE_ERROR_BADUTF8 = -10
          if(err==PCRE_ERROR_BADUTF8)
            fprintf(stderr, "\ncb_compare_regexp: PCRE_ERROR_BADUTF8.");
          if(err==CBSUCCESS)
            err = CBERRREGEXEC;
        }
        //fprintf(stderr, "\ncb_compare_regexp: returning %i, matchcount=%i, groupcount=%i.", err, *matchcount, groupcount);
        return err;
}

/* /compare -7 and -8 */

int  cb_compare_print_fullinfo(cb_match *mctl){
	int res=0;
	const pcre32_extra *rextra = NULL;
	const pcre32       *re = NULL;
	unsigned char *cres = NULL;
	unsigned char  errmsg[6] = {'e','r','r','o','r','\0'};
	cres = &(*errmsg);

	if(mctl==NULL){  return CBERRALLOC; }

	if((*mctl).re!=NULL){ re = (const pcre32*) (*mctl).re; }
	if((*mctl).re_extra!=NULL){ rextra = (const pcre32_extra*) (*mctl).re_extra; }

	fprintf(stderr, "\ncb_match:");	
	fprintf(stderr, "\n\tmatchctl=%i", (*mctl).matchctl );	
	if( re != NULL ){ // const
	  res = pcre32_fullinfo( re, rextra, PCRE_INFO_SIZE, &(* (PSIZE) cres) );
	  if(res==0){ 	fprintf(stderr, "\n\tsize=%i (, %i, %i)", cres[0], cres[1], cres[2] ); }
	  res = pcre32_fullinfo( re, rextra, PCRE_INFO_NAMECOUNT, &(* (PSIZE) cres) );
	  if(res==0){ 	fprintf(stderr, "\n\tsubpatterns=%i", (unsigned int) ( (0|cres[0])<<8 | cres[1] ) ); }
	  res = pcre32_fullinfo( re, rextra, PCRE_INFO_OPTIONS, &(* (PSIZE) cres) );
	  if(res==0){ 	fprintf(stderr, "\n\toptions=%i", (unsigned int) ( (0|cres[0])<<8 | cres[1] ) ); }
	}
	return CBSUCCESS;
}

