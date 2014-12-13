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
#include <string.h> // strsep
#include "../include/cb_buffer.h"
#include "../read/cb_read.h"

/*
 * To search a subtree, dot in name separates a subdomain. */

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
	err = cb_set_cursor_match_length_ucs( &(*cbs), &name, &namelength, 1, -1 ); // no match
	// int  cb_set_cursor_match_length_ucs(CBFILE **cbs, unsigned char **ucsname, int *namelength, int ocoffset, int matchctl)
	(**cbs).cf.searchmethod = searchmethod;
	return err;
}

/*
 * 4-byte functions. Allocates ucsname.
 */

int  cb_get_current_name(CBFILE **cbs, unsigned char **ucsname, int *namelength ){
	/*
	 * Allocate and copy current name to new ucsname */
	int ret = CBSUCCESS; int indx=0;
	if( cbs==NULL || *cbs==NULL ) return CBERRALLOC;

	if( (**cbs).cb!=NULL && (*(**cbs).cb).list.current!=NULL ){
	  if(ucsname==NULL)
	    ucsname = (unsigned char**) malloc( sizeof( int ) ); // pointer size
	  else if( *ucsname!=NULL ) 
	    fprintf(stderr,"\ndebug: cb_get_current_name_ucs: *ucsname was not NULL.");
	  *ucsname = (unsigned char*) malloc( sizeof(unsigned char)*( (unsigned int) (*(*(**cbs).cb).list.current).namelen+1 ) );
	  if( ucsname==NULL ) { return CBERRALLOC; }
	  (*ucsname)[(*(*(**cbs).cb).list.current).namelen] = '\0';
	  for( indx=0; indx<(*(*(**cbs).cb).list.current).namelen ; ++indx)
	    (*ucsname)[indx] = (*(*(**cbs).cb).list.current).namebuf[indx];
	  if(namelength==NULL)
	    namelength = (int*) malloc( sizeof( int ) ); // int 
	  if(namelength==NULL) { return CBERRALLOC; }
 	  *namelength = (*(*(**cbs).cb).list.current).namelen;
	}else
	  ret = CBERRALLOC; 
        if( ucsname==NULL || *ucsname==NULL ){ ret = CBERRALLOC; }
	return ret;
}

int  cb_get_next_name_ucs(CBFILE **cbs, unsigned char **ucsname, int *namelength){
	int ret = CBSUCCESS;
	unsigned char *name = NULL;
	unsigned char  chrs[2] = { 0x20, '\0' };
	int namelen = 0;
	char searchmethod=0;
	name = &chrs[0];

	if( cbs==NULL || *cbs==NULL ){	  fprintf(stderr,"\ncb_get_next_name_ucs: cbs was null."); return CBERRALLOC;	}
	//if( *ucsname!=NULL )
	//  fprintf(stderr,"\ndebug, cb_get_next_name_ucs: *ucsname was not NULL.");

	searchmethod = (**cbs).cf.searchmethod;
	(**cbs).cf.searchmethod = CBSEARCHNEXTNAMES;
	ret = cb_set_cursor_match_length_ucs( &(*cbs), &name, &namelen, 0, 0 ); // matches first (any)
	(**cbs).cf.searchmethod = searchmethod;

	free(*ucsname); // ucsname = NULL;
	*ucsname = NULL; // 11.12.2014
	if( ret==CBSUCCESS || ret==CBSTREAM ){ // returns only CBSUCCESS or CBSTREAM or error
	  ret = cb_get_current_name( &(*cbs), &(*ucsname), &(*namelength) );
	  fprintf(stderr,"\ncb_get_next_name_ucs: cb_get_current_name returned %i.", ret);
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


//int  cb_tree_set_cursor_ucs(CBFILE **cbs, unsigned char **dotname, int namelen, int matchctl){
// Oli virhe:
// sz = read((**cbf).fd, (*blk).buf, (ssize_t)(*blk).buflen);
// =>
// CBSTREAMEND, dup ? kaksi samalla listalla

