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
//#include <unistd.h>
#include "../include/cb_buffer.h"

int  cb_compare_strict(unsigned char **name1, int len1, unsigned char **name2, int len2, int from2);
int  cb_compare_rfc2822(unsigned char **name1, int len1, unsigned char **name2, int len2, int from2);


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
	  //fprintf(stderr,".");
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
int  cb_compare(CBFILE **cbs, unsigned char **name1, int len1, unsigned char **name2, int len2, int matchctl){
	int err = CBSUCCESS, dfr = 0, indx = 0;
	unsigned char stb[3] = { 0x20, 0x20, '\0' };
	unsigned char *stbp = NULL;
	stbp = &stb[0]; 

	//fprintf(stderr,"\ncb_compare: [");
	//cb_print_ucs_chrbuf(&(*name1), len1, len1); fprintf(stderr,"] [");
	//cb_print_ucs_chrbuf(&(*name2), len2, len2); fprintf(stderr,"] len1: %i, len2: %i.", len1, len2);

	if(matchctl==-1)
	  return CBNOTFOUND; // no match
	if( cbs==NULL || *cbs==NULL )
	  return CBERRALLOC;
	if( matchctl==-6 ){ // %am%
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
	}else if( matchctl==-5 ){ // %ame
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
	}else if( matchctl==0 ){ // all
	  stbp = &stb[0]; 
	  err = cb_compare_strict( &stbp, 0, &(*name2), len2, 0 );
	}else if((**cbs).cf.asciicaseinsensitive==1){ // name or nam%
	  err = cb_compare_rfc2822( &(*name1), len1, &(*name2), len2, 0 );
	}else{ // name or nam%
	  err = cb_compare_strict( &(*name1), len1, &(*name2), len2, 0 );
	}

	//fprintf(stderr,"\ncb_compare: returning %i.", err);

	switch (matchctl) {
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
	    //fprintf(stderr,"\ncb_compare -5 err=%i", err);
	    if( err==CBMATCH || err==CBMATCHLENGTH )
	      return CBMATCH; // ( cut from end ame1 matches name1 )
	    break;
	  case -6:
	    //fprintf(stderr,"\ncb_compare -6 err=%i", err);
	    if( err==CBMATCH || err==CBMATCHLENGTH )
	      return CBMATCH; // in the middle
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

