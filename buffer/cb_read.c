/* 
 * Library to read and write streams. Searches valuepairs locations to a list. Uses different character encodings.
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
#include "../include/cb_buffer.h"

//int cb_clear_every_matchcount(CBFILE **cbs);
//int cb_clear_every_matchcount(CBFILE **cbs){
//	return CBSUCCESS;
//}

/*
 * 1 or 4 -byte functions.
 */
int  cb_find_every_name(CBFILE **cbs){
	int err = CBSUCCESS; 
	unsigned char *name = NULL;
	unsigned char  chrs[2]  = { 0x20, '\0' };
	int namelength = 0;
	char searchmethod=0;
	if( cbs==NULL || *cbs==NULL )
	  return CBERRALLOC;
	name = &chrs[0];
	searchmethod = (**cbs).cf.searchmethod;
	(**cbs).cf.searchmethod = CBSEARCHNEXTNAMES;
	err = cb_set_cursor_match_length_ucs( &(*cbs), &name, &namelength, -1 ); // no match
	(**cbs).cf.searchmethod = searchmethod;
	return err;
}
/*
 * 4-byte functions.
 */
int  cb_get_next_name_ucs(CBFILE **cbs, unsigned char **ucsname, int *namelength){
	int ret = CBSUCCESS, indx = 0;
	unsigned char *name = NULL;
	unsigned char  chrs[2] = { 0x20, '\0' };
	int namelen = 0;
	char searchmethod=0;
	name = &chrs[0];

	if( cbs==NULL || *cbs==NULL )
	  return CBERRALLOC;
	if( *ucsname!=NULL )
	  fprintf(stderr,"\ndebug, cb_get_next_name_ucs: *ucsname was not NULL.");

	searchmethod = (**cbs).cf.searchmethod;
	(**cbs).cf.searchmethod = CBSEARCHNEXTNAMES;
	ret = cb_set_cursor_match_length_ucs( &(*cbs), &name, &namelen, 0 ); // matches first (any)
	(**cbs).cf.searchmethod = searchmethod;

	free(*ucsname);
	if( ret==CBSUCCESS || ret==CBSTREAM ){ // returns only CBSUCCESS or CBSTREAM or error
	  /*
	   * Copy current name to new ucsname */
	  //fprintf(stderr,"\ndebug, cb_get_next_name_ucs: MERKKI.");
	  if( (**cbs).cb!=NULL && (*(**cbs).cb).current!=NULL ){
	    if(ucsname==NULL)
	      ucsname = (unsigned char**) malloc( sizeof( int ) ); // pointer size
	    *ucsname = (unsigned char*) malloc( sizeof(unsigned char)*( (*(*(**cbs).cb).current).namelen+1 ) );
	    (*ucsname)[(*(*(**cbs).cb).current).namelen] = '\0';
	    for( indx=0; indx<(*(*(**cbs).cb).current).namelen ; ++indx)
	      (*ucsname)[indx] = (*(*(**cbs).cb).current).namebuf[indx];
	    if( namelength==NULL )
	    if(namelength==NULL)
	      namelength = (int*) malloc( sizeof( int ) ); // int 
 	      *namelength = (*(*(**cbs).cb).current).namelen;
	  }else
	    ret = CBERRALLOC; 
          if( ucsname==NULL || *ucsname==NULL ){ ret = CBERRALLOC; }
	}

/*
 * TMP
 * Palauttaa:
 * joko CBSUCCESS tai CBSTREAM tai virhe:
 * CBERRALLOC,
 * cb_search_get_chr: CBSTREAMEND, CBNOENCODING, CBNOTUTF, CBUTFBOM
 */

	return ret;
}

