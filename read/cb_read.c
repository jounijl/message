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

/*
 * To search a subtree, dot in name separates a subdomain. */
#define CBDOTSEPARATOR       '.'

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
	(**cbs).cf.searchmethod = searchmethod;
	return err;
}

/*
 * 4-byte functions. Allocates ucsname.
 */

int  cb_get_current_name(CBFILE **cbs, unsigned char **ucsname, int *namelength ){
	/*
	 * Allocate and copy current name to new ucsname */
	int ret = CBSUCCESS, indx=0;
	if( cbs==NULL || *cbs==NULL ) return CBERRALLOC;
	if( *ucsname!=NULL ) fprintf(stderr,"\ndebug: cb_get_current_name_ucs: *ucsname was not NULL.");

	if( (**cbs).cb!=NULL && (*(**cbs).cb).list.current!=NULL ){
	  if(ucsname==NULL)
	    ucsname = (unsigned char**) malloc( sizeof( int ) ); // pointer size
	  *ucsname = (unsigned char*) malloc( sizeof(unsigned char)*( (*(*(**cbs).cb).list.current).namelen+1 ) );
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

	if( cbs==NULL || *cbs==NULL )
	  return CBERRALLOC;
	//if( *ucsname!=NULL )
	//  fprintf(stderr,"\ndebug, cb_get_next_name_ucs: *ucsname was not NULL.");

	searchmethod = (**cbs).cf.searchmethod;
	(**cbs).cf.searchmethod = CBSEARCHNEXTNAMES;
	ret = cb_set_cursor_match_length_ucs( &(*cbs), &name, &namelen, 0, 0 ); // matches first (any)
	(**cbs).cf.searchmethod = searchmethod;

	free(*ucsname); ucsname = NULL;
	if( ret==CBSUCCESS || ret==CBSTREAM ){ // returns only CBSUCCESS or CBSTREAM or error
	  ret = cb_get_current_name( &(*cbs), &(*ucsname), &(*namelength) );
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


/*
 * Search name1.name2.name3  */
int  cb_tree_set_cursor_ucs(CBFILE **cbs, unsigned char **dotname, int namelen, int matchctl){

// Oli virhe:
// sz = read((**cbf).fd, (*blk).buf, (ssize_t)(*blk).buflen);
// =>
// CBSTREAMEND, dup ? kaksi samalla listalla

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

/*
 * DEBUG
 */

	/* Every name */
	undx = 0; indx=0;
	while( indx<namelen && err<=CBNEGATION && err2<=CBNEGATION ){
	  err2 = cb_get_ucs_chr( &chr, &(*dotname), &indx, namelen );
	  if( chr != (unsigned long int) CBDOTSEPARATOR)
	    err = cb_put_ucs_chr( chr, &ucsname, &undx, namelen );
	  if( chr == (unsigned long int) CBDOTSEPARATOR || indx>=namelen ){ // Name

	    // Debug:
	    fprintf(stderr, "\nsearchname=[");
	    cb_print_ucs_chrbuf( &ucsname, undx, CBNAMEBUFLEN );
	    fprintf(stderr, "] length %i, name [", undx);
	    cb_print_ucs_chrbuf( &(*dotname), namelen, namelen );
	    fprintf(stderr, "] length %i.", namelen);

	    err = cb_set_cursor_match_length_ucs( &(*cbs), &ucsname, &undx, namecount, matchctl );
	    if( err==CBSUCCESS || err==CBSTREAM ){ // Debug
	      firstname = &(*(*(**cbs).cb).list.current);
	      leaf = &(*(*(**cbs).cb).list.currentleaf);
	      if(namecount==0)
	        fprintf(stderr,"\nFound name: ["); // Debug:
	      else
	        fprintf(stderr,"\nFound leaf: ["); // Debug:
	      cb_print_ucs_chrbuf( &ucsname, undx, CBNAMEBUFLEN );
	      fprintf(stderr,"] leaves: (%i. name) [", (namecount+1) );
	      cb_print_leaves( &leaf );
	      fprintf(stderr,"]");
	    }

	    if(err!=CBSUCCESS && err!=CBSTREAM)
	      ret = CBNOTFOUND;

	    undx = 0;
	    ++namecount;
	  }
	}
	(**cbs).cf.searchstate = origsearchstate;
	free(ucsname);
	if(indx==namelen && ret==CBMATCH) // Match and dotted name was searched through (cursor is at name)
	  return CBSUCCESS;
	else // Name was not searched through or no match (cursor is somewhere in value)
	  return CBNOTFOUND;
}
