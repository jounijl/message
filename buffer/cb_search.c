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
#include <unistd.h>
#include <string.h> // memset
#include <time.h>   // time (timestamp in cb_set_cursor)
#include "../include/cb_buffer.h"
//#include "../include/cb_compare.h"


int  cb_put_name(CBFILE **str, cb_name **cbn, int openpairs);
int  cb_put_leaf(CBFILE **str, cb_name **leaf, int openpairs); // 13.12.2013
int  cb_set_to_last_leaf(cb_name **tree, cb_name **lastleaf, int *openpairs); // sets openpairs, not yet tested 7.12.2013
int  cb_set_to_leaf(CBFILE **cbs, unsigned char **name, int namelen, int openpairs, cb_match *mctl); // 11.3.2014, 23.3.2014
int  cb_set_to_leaf_inner(CBFILE **cbs, unsigned char **name, int namelen, int openpairs, cb_match *mctl); // 16.3.2014, 23.3.2014
int  cb_set_to_name(CBFILE **str, unsigned char **name, int namelen, cb_match *mctl); // 11.3.2014, 23.3.2014
int  cb_set_search_method(CBFILE **buf, char method); // CBSEARCH*
int  cb_search_get_chr( CBFILE **cbs, unsigned long int *chr, long int *chroffset);
int  cb_save_name_from_charbuf(CBFILE **cbs, cb_name **fname, long int offset, unsigned char **charbuf, int index);
int  cb_automatic_encoding_detection(CBFILE **cbs);


/*
 * Returns a pointer to 'result' from 'leaf' tree matching the name and namelen with CBFILE cb_conf,
 * matchctl and openpairs count. Returns CBNOTFOUND if leaf was not found. 'result' may be NULL. */
int  cb_set_to_leaf(CBFILE **cbs, unsigned char **name, int namelen, int openpairs, cb_match *mctl){ // 23.3.2014
	if(cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || name==NULL || mctl==NULL){ 
	  fprintf(stderr,"\ncb_set_to_leaf: allocation error."); return CBERRALLOC; 
	}
	if( (*(**cbs).cb).list.current==NULL )
	  return CBEMPTY;
	(*(**cbs).cb).list.currentleaf = &(* (cb_name*) (*(*(**cbs).cb).list.current).leaf);
	return cb_set_to_leaf_inner( &(*cbs), &(*name), namelen, openpairs, &(*mctl)); // 11.3.2014
}
int  cb_set_to_leaf_inner(CBFILE **cbs, unsigned char **name, int namelen, int openpairs, cb_match *mctl){ // 11.4.2014, 23.3.2014
	int err = CBSUCCESS;
	cb_name *leafptr = NULL;
	if(cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || name==NULL || mctl==NULL){
	  fprintf(stderr,"\ncb_set_to_leaf_inner: allocation error."); return CBERRALLOC;
	}

	if( (*(**cbs).cb).list.currentleaf==NULL )
	  return CBEMPTY;
	// Node
	leafptr = &(*(*(**cbs).cb).list.currentleaf);

	err = cb_compare( &(*cbs), &(*name), namelen, &(*leafptr).namebuf, (*leafptr).namelen, &(*mctl)); // 11.4.2014, 23.3.2014
	if(err==CBMATCH){
	  if( openpairs < 0 ){ 
	    fprintf(stderr, "\ncb_set_to_leaf: error: openpairs was %i.", openpairs); // debug
	    return CBNOTFOUND;
	  }
	  if( openpairs == 1 ){ // 1 = first leaf (leaf or next)
            /*
             * 9.12.2013 (from set_to_name):
             * If searching of multiple same names is needed (in buffer also), do not return 
             * allready matched name. Instead, increase names matchcount (or set to 1 if it 
             * becomes 0) and search the next same name.
             */
            (*leafptr).matchcount++; if( (*leafptr).matchcount==0 ){ (*leafptr).matchcount+=2; }
            if( ( (**cbs).cf.searchmethod==CBSEARCHNEXTNAMES && (*leafptr).matchcount==1 ) || (**cbs).cf.searchmethod==CBSEARCHUNIQUENAMES ){ 
              if( (**cbs).cf.type!=CBCFGFILE ) // When used as only buffer, stream case does not apply
                if((*leafptr).offset>=( (*(**cbs).cb).buflen + 0 + 1 ) ){ // buflen + smallest possible name + endchar
                  /*
                   * Leafs content was out of memorybuffer, setting
                   * CBFILE:s index to the edge of the buffer.
                   */
                  (*(**cbs).cb).index = (*(**cbs).cb).buflen; // set as a stream, 1.12.2013
                  return CBNAMEOUTOFBUF;
                }
              (*(**cbs).cb).index=(*leafptr).offset; // 1.12.2013
            }
	    return CBSUCCESS; // CBMATCH
	  }
	}
	if( (*leafptr).leaf!=NULL && openpairs>=1 ){ // Left
	  (*(**cbs).cb).list.currentleaf = &(* (cb_name*) (*leafptr).leaf);
	  err = cb_set_to_leaf_inner( &(*cbs), &(*name), namelen, (openpairs-1), &(*mctl)); // 11.4.2014, 23.3.2014
	  if(err==CBSUCCESS)
	    return err;
	}
	if( (*leafptr).next!=NULL ){ // Right
	  (*(**cbs).cb).list.currentleaf = &(* (cb_name*) (*leafptr).next);
	  err = cb_set_to_leaf_inner( &(*cbs), &(*name), namelen, openpairs, &(*mctl)); // 11.4.2014, 23.3.2014
	  if(err==CBSUCCESS)
	    return err;
	}
	(*(**cbs).cb).list.currentleaf = NULL;
	return CBNOTFOUND;
}
int  cb_set_to_last_leaf(cb_name **tree, cb_name **lastleaf, int *openpairs){

	/*
	 * Finds the last put leaf whose openpairs -count is the same
	 * or less than is needed. Traversal is (node, left, right).
	 * To reach fastest the end, towards right leaf. Saves the leaves 
	 * openpairs count representing the count of "left" leaves
	 * representing it's closest as possible wanted 'topology level'
	 * of the last put node. The leafs are put in the order of
	 * appearance.
	 *
	 * n.l.r.                                 
	 *             1.      9.     10.         
	 * namelist: ->(1;1)->(1;2)->(1;3)        
	 * ---------  2. |    3. right  5.    8.  
	 * tree:       (2;1)->(2;2)->(2;3)->(2;4) 
	 *                   4. |left  | 6.   7.  
	 *                    (3;1)  (3;2)->(3;3) 
	 * to right                               
	 *              1.     6.     7.          
	 * namelist: ->(1;1)->(1;2)->(1;3)        
	 * ---------  2. |    3. right  4.    5.  
	 * tree:       (2;1)->(2;2)->(2;3)->(2;4) 
	 *                      |left  |          
	 *                    (3;1)  (3;2)->(3;3) 
	 */

	/* Finds the most rightmost leaf */
	cb_name *node = NULL;
	cb_name *iter = NULL;
	int leafcount = 0, nextcount=0; // count of leaves and overflow prevention
	if( tree==NULL || openpairs==NULL) return CBERRALLOC;
	if( *tree==NULL ){ *lastleaf=NULL; *openpairs=0; return CBEMPTY;}
	leafcount = 1; // tree was not null

	iter = &(* (cb_name*) (**tree).leaf);
	node = &(**tree); 
	*lastleaf = &(* (cb_name*) (**tree).leaf); // default
	if(*lastleaf==NULL){ // First leaf 
	  *lastleaf = &(**tree);
	  *openpairs = 1;    // To left
	  return CBSUCCESS;
	}

	while( iter!=NULL && nextcount<=CBMAXLEAVES && leafcount<=CBMAXLEAVES){
	  iter  = &(* (cb_name*) (*node).next); // The rightmost 
	  if(iter!=NULL){
	    ++nextcount;
	  }
	  //if( iter==NULL ){ // To the left if right was null
	  if( iter==NULL && leafcount<*openpairs ){ // To the left if right was null, 15.12.2013
	    iter = &(* (cb_name*) (*node).leaf);
	    if(iter!=NULL){
	      ++leafcount;
	    }
	  }
	  /*
	   * Leafcount has to be at the same level (or lower) to
	   * get the rightmost leaf of the same openpairs count.
	   */
	  //if(iter!=NULL && leafcount<=*openpairs){ 
	  if(iter!=NULL){  // 15.12.2013
	    node = &(*iter);
	    *lastleaf = &(*node);
	  }else{ // The rightmost leaf
	    iter = NULL; // stop the loop ( leafcount<=*openpairs )
	  }
	}
	*lastleaf = &(*node);
	*openpairs = leafcount;
	if(nextcount>=CBMAXLEAVES || leafcount>=CBMAXLEAVES){
	  fprintf(stderr,"\ncb_set_to_last_leaf: CBLEAFCOUNTERROR.");
	  return CBLEAFCOUNTERROR;
	}
	return CBSUCCESS; // returns allways a proper not null node or CBEMPTY, in error CBLEAFCOUNTERROR
}

/*
 * Put leaf in 'current' name -tree and update 'currentleaf'. */
int  cb_put_leaf(CBFILE **str, cb_name **leaf, int openpairs){

	/*
	 *  openpairs=1  openpairs=2  openpairs=3  openpairs=2  openpairs=1  openpairs=0
	 *  atvalue=1    atvalue=1    atvalue=1    atvalue=1    atvalue=1    atvalue=0
	 *      |            |             |           |            |            |
	 * name=     subname=     subname2= value      &            &            & name2=
	 *      |--------- subsearch -------------------------------------------->
	 * 
	 * JSON:
	 *  openpairs=1 openpairs=2     openpairs=4              openpairs=2     openpairs=0
	 *      |           | openpairs=3   |       (openpairs=3 )   | (openpairs=1) |
	 *      |           |     |         |            |           |      |        |
	 *      {    subname:     { subname2: value     (,)          }     (,)       }, (name2): {
	 *      |---------------------------- subsearch ----------------------------->
	 * To json compatibility:
	 *   - Empty names should be allowed (object in value has 2 openpairs count with empty name)
	 *   - If previous openpairs was even, an even separator decreases openpairs count twice
	 *   - Valuestrings between ":s, arrays in between "[" and "]", strings false|true|null
	 *     and numbers are read elsewhere
	 */

	int leafcount=0, err=CBSUCCESS, ret=CBSUCCESS;
	cb_name *lastleaf  = NULL;
	cb_name *newleaf   = NULL;

	if( str==NULL || *str==NULL || (**str).cb==NULL || leaf==NULL || *leaf==NULL) return CBERRALLOC;

	/*
	 * Find last leaf */
	leafcount = openpairs;
	err = cb_set_to_last_leaf( &(*(**str).cb).list.current, &lastleaf, &leafcount);
	if(err>=CBERROR){ fprintf(stderr,"\ncb_put_leaf: cb_set_to_last_leaf returned error %i.", err); }
	(*(**str).cb).list.currentleaf = &(*lastleaf); // last
	if( leafcount==0 || err==CBEMPTY ){ 
	  return CBEMPTY;
	}else if(openpairs<=1){
	  return CBLEAFCOUNTERROR;
	}

	/*
	 * Allocate and copy leaf */
        err = cb_allocate_name( &newleaf, (**leaf).namelen ); 
	if(err!=CBSUCCESS){ fprintf(stderr,"\ncb_put_leaf: cb_allocate_name error %i.", err); return err; }
        err = cb_copy_name( &(*leaf), &newleaf );
	if(err!=CBSUCCESS){ fprintf(stderr,"\ncb_put_leaf: cb_copy_name error %i.", err); return err; }
	(*newleaf).firsttimefound = (signed long int) time(NULL);
	(*newleaf).next = NULL;
	(*newleaf).leaf = NULL;

	/*
	 * Add to leaf or to next name in list. */
	if( !(lastleaf==NULL && leafcount==1) ){ // First leaf
	  if( leafcount == openpairs ){ // Next name, to "right"
	    if( lastleaf==NULL ){
	      return CBERRALLOC;
	    }
	    (*(*(**str).cb).list.currentleaf).next = &(*newleaf); // 13.12.2013
	    lastleaf = &(* (cb_name*) (*(*(**str).cb).list.currentleaf).next);
	    ret = CBADDEDNEXTTOLEAF;
	  }else if( leafcount<openpairs ){ // Leaf, to "left"
	    if( lastleaf==NULL ){
	      return CBERRALLOC;
	    }
	    (*(*(**str).cb).list.currentleaf).leaf = &(*newleaf); // 13.12.2013
	    lastleaf = &(* (cb_name*) (*(*(**str).cb).list.currentleaf).leaf);
	    ret = CBADDEDLEAF;
	  }else if( leafcount>openpairs ){ // 
	    fprintf(stderr,"\ncb_put_leaf: error, leafcount>openpairs.");
	    return CBLEAFCOUNTERROR;
	  }
	}

	(*(**str).cb).list.currentleaf = &(*lastleaf);

	if(err>=CBNEGATION)
	  return err;
	return ret;
}

int  cb_put_name(CBFILE **str, cb_name **cbn, int openpairs){ 
        int err=0; 

        if(cbn==NULL || *cbn==NULL || str==NULL || *str==NULL || (**str).cb==NULL )
          return CBERRALLOC;

        /*
         * Do not save the name if it's out of buffer. cb_set_cursor finds
         * names from stream but can not use cb_names namelist if the names
         * are out of buffer. When used as file (CBCFGFILE), filesize limits
         * the list and this is not necessary.
         */
        if((**str).cf.type!=CBCFGFILE)
          if( (**cbn).offset >= ( (*(**str).cb).buflen-1 ) || ( (**cbn).offset + (**cbn).length ) >= ( (*(**str).cb).buflen-1 ) ) 
            return CBNAMEOUTOFBUF;

        if((*(**str).cb).list.name!=NULL){
          (*(**str).cb).list.current = &(*(*(**str).cb).list.last);
          (*(**str).cb).list.currentleaf = &(*(*(**str).cb).list.last); // 11.12.2013

	  
	  // Add leaf or name to a leaf, 2.12.2013
	  if(openpairs>1){ // first '=' is openpairs==1 (used only in CBSTATETREE)
	    //err = cb_put_leaf( &(*(**str).cb).list.current, &(*cbn), openpairs); // openpairs chooses between *next and *leaf
	    err = cb_put_leaf( &(*str), &(*cbn), openpairs); // openpairs chooses between *next and *leaf
	    return err;
	  }

	  // Previous names length
          if( (*(**str).cb).list.namecount>=1 )
            (*(*(**str).cb).list.current).length = ( (*(**str).cb).index - (**str).ahd.bytesahead ) - (*(*(**str).cb).list.current).offset;

	  // Add name 
          (*(**str).cb).list.last    = &(* (cb_name*) (*(*(**str).cb).list.last).next );
          err = cb_allocate_name( &(*(**str).cb).list.last, (**cbn).namelen ); if(err!=CBSUCCESS){ return err; } // 7.12.2013
          (*(*(**str).cb).list.current).next = &(*(*(**str).cb).list.last); // previous
          (*(**str).cb).list.current = &(*(*(**str).cb).list.last);
          (*(**str).cb).list.currentleaf = &(*(*(**str).cb).list.last);
          ++(*(**str).cb).list.namecount;
          //if( (*(**str).cb).contentlen >= (*(**str).cb).buflen )
          if( ( (*(**str).cb).contentlen - (**str).ahd.bytesahead ) >= (*(**str).cb).buflen ) // 6.9.2013
            (*(*(**str).cb).list.current).length = (*(**str).cb).buflen; // Largest 'known' value
        }else{
          err = cb_allocate_name( &(*(**str).cb).list.name, (**cbn).namelen ); if(err!=CBSUCCESS){ return err; } // 7.12.2013
          (*(**str).cb).list.last    = &(* (cb_name*) (*(**str).cb).list.name); // last
          (*(**str).cb).list.current = &(* (cb_name*) (*(**str).cb).list.name); // current
          (*(**str).cb).list.currentleaf = &(* (cb_name*) (*(**str).cb).list.name); // 11.12.2013
          (*(*(**str).cb).list.current).next = NULL; // firsts next
          (*(*(**str).cb).list.current).leaf = NULL;
          (*(**str).cb).list.namecount=1;
        }
        err = cb_copy_name( &(*cbn), &(*(**str).cb).list.last); if(err!=CBSUCCESS){ return err; } // 7.12.2013
        (*(*(**str).cb).list.last).next = NULL;
        (*(*(**str).cb).list.last).leaf = NULL;
	(*(*(**str).cb).list.last).firsttimefound = (signed long int) time(NULL); // 1.12.2013
        return CBADDEDNAME;
}

int  cb_set_to_name(CBFILE **str, unsigned char **name, int namelen, cb_match *mctl){ // cb_match 11.3.2014, 23.3.2014
	cb_name *iter = NULL; int err=CBSUCCESS;

	if(*str!=NULL && (**str).cb != NULL && mctl!=NULL ){
	  iter = &(*(*(**str).cb).list.name);
	  while(iter != NULL){
	    err = cb_compare( &(*str), &(*name), namelen, &(*iter).namebuf, (*iter).namelen, &(*mctl) ); // 23.11.2013, 11.4.2014
	    if( err == CBMATCH ){ // 9.11.2013
	      /*
	       * 20.8.2013:
	       * If searching of multiple same names is needed (in buffer also), do not return 
	       * allready matched name. Instead, increase names matchcount (or set to 1 if it 
	       * becomes 0) and search the next same name.
	       */
	      (*iter).matchcount++; if( (*iter).matchcount==0 ){ (*iter).matchcount+=2; }
	      /* First match on new name or if unique names are in use, the first match or the same match again, even if in stream. */
	      if( ( (**str).cf.searchmethod==CBSEARCHNEXTNAMES && (*iter).matchcount==1 ) || (**str).cf.searchmethod==CBSEARCHUNIQUENAMES ){ 
	        if( (**str).cf.type!=CBCFGFILE ) // When used as only buffer, stream case does not apply
	          if((*iter).offset>=( (*(**str).cb).buflen + 0 + 1 ) ){ // buflen + smallest possible name + endchar
		    /*
	             * Do not return names in namechain if it's content is out of memorybuffer
		     * to search multiple same names again from stream.
	 	     */
	            (*(**str).cb).index = (*(**str).cb).buflen; // set as a stream, 1.12.2013
	            (*(**str).cb).list.current = &(*iter); // 1.12.2013/2
	            (*(**str).cb).list.currentleaf = &(*iter); // 11.12.2013
	            return CBNAMEOUTOFBUF;
	          }
	        (*(**str).cb).index=(*iter).offset; // 1.12.2013
	        (*(**str).cb).list.current=&(*iter); // 1.12.2013
	        (*(**str).cb).list.currentleaf=&(*iter); // 11.12.2013
	        return CBSUCCESS;
	      }
	    }
	    iter = &(* (cb_name *) (*iter).next); // 9.12.2013
	  }
	  (*(**str).cb).index = (*(**str).cb).contentlen - (**str).ahd.bytesahead;  // 5.9.2013
	}else{
	  fprintf(stderr, "\ncb_set_to_name: allocation error.");
	  return CBERRALLOC; // 30.3.2013
	}
	return CBNOTFOUND;
}

int  cb_set_to_polysemantic_names(CBFILE **cbf){
	return cb_set_search_method(&(*cbf), (char) CBSEARCHNEXTNAMES);
}

int  cb_set_to_unique_names(CBFILE **cbf){
	return cb_set_search_method(&(*cbf), (char) CBSEARCHUNIQUENAMES);
}

int  cb_set_search_method(CBFILE **cbf, char method){
	if(cbf!=NULL){
	  if((*cbf)!=NULL){
	    (**cbf).cf.searchmethod=method; 
	    return CBSUCCESS;
	  }
	}
	return CBERRALLOC;
}

//int  cb_remove_ahead_offset(CBFILE **cbf){ 
//	return cb_remove_ahead_offset( &(*cbf), &(**cbf).ahd );
//}
int  cb_remove_ahead_offset(CBFILE **cbf, cb_ring *cfi){ 
	int err=CBSUCCESS;
        if(cbf==NULL || *cbf==NULL || (**cbf).cb==NULL || cfi==NULL)
          return CBERRALLOC;

	/*
 	 * Index is allways in the right place. Contentlen is adjusted
	 * by bytes left in readahead to read it again. 
	 */
 
	if( (*(**cbf).cb).index <= ( (*(**cbf).cb).contentlen - (*cfi).bytesahead ) ){
	  (*(**cbf).cb).contentlen -= (*cfi).bytesahead;  // Do not read again except bytes ahead 
	}else if( (*(**cbf).cb).index < (*(**cbf).cb).contentlen ){
	  (*(**cbf).cb).contentlen = (*(**cbf).cb).index; // This is the right choice allmost every time
	}else{
	  (*(**cbf).cb).index = (*(**cbf).cb).contentlen; // Error
	  err = CBINDEXOUTOFBOUNDS; // Index was ahead of contentlen
	}

        if( (*(**cbf).cb).index < 0 ) // Just in case
          (*(**cbf).cb).index=0;
        if( (*(**cbf).cb).contentlen < 0 )
          (*(**cbf).cb).contentlen=0;

	if( (*(**cbf).cb).contentlen>=(*(**cbf).cb).buflen || (*(**cbf).cb).index>=(*(**cbf).cb).buflen )
	  err = CBSTREAM;

	// Empty readahead
        (*cfi).ahead=0;
        (*cfi).bytesahead=0;
        (*cfi).first=0;
        (*cfi).last=0;
        (*cfi).streamstart=-1; // readaheadbuffer is empty, cb_get_chr returns the right values
        (*cfi).streamstop=-1;
        return err;
}

int  cb_search_get_chr( CBFILE **cbs, unsigned long int *chr, long int *chroffset ){
	int err = CBSUCCESS, bytecount=0, storedbytes=0;
	if(cbs==NULL || *cbs==NULL || chroffset==NULL || chr==NULL)
	  return CBERRALLOC;
	if((**cbs).cf.unfold==1)
	  return cb_get_chr_unfold( &(*cbs), &(*chr), &(*chroffset) );
	else{
	  err = cb_get_chr( &(*cbs), &(*chr), &bytecount, &storedbytes );
	  *chroffset = (*(**cbs).cb).index - 1; // 2.12.2013
	  return err;
	}
	return CBERROR;
}

int  cb_get_chr_unfold(CBFILE **cbs, unsigned long int *chr, long int *chroffset){
	int bytecount=0, storedbytes=0, tmp=0; int err=CBSUCCESS;
	unsigned long int empty=0x61;
	if(cbs==NULL || *cbs==NULL || chroffset==NULL || chr==NULL)
	  return CBERRALLOC;

        //fprintf(stderr, "\ncb_get_chr_unfold: ahead=%i, bytesahead=%i. ", (int) (**cbs).ahd.ahead, (int) (**cbs).ahd.bytesahead );

	if( (**cbs).ahd.ahead == 0){
	  err = cb_get_chr( &(*cbs), &(*chr), &bytecount, &storedbytes );
	  if(err==CBSTREAM){  cb_fifo_set_stream( &(**cbs).ahd ); }
	  if(err==CBSTREAMEND){  cb_fifo_set_endchr( &(**cbs).ahd ); }

cb_get_chr_unfold_try_another:
	  if( WSP( *chr ) && err<CBNEGATION ){
	    cb_fifo_put_chr( &(**cbs).ahd, *chr, storedbytes);
	    err = cb_get_chr( &(*cbs), &(*chr), &bytecount, &storedbytes );
	    if(err==CBSTREAM){  cb_fifo_set_stream( &(**cbs).ahd ); }
	    if(err==CBSTREAMEND){  cb_fifo_set_endchr( &(**cbs).ahd ); }
	    if( WSP( *chr ) && err<CBNEGATION ){
	      cb_fifo_revert_chr( &(**cbs).ahd, &empty, &tmp );
	      goto cb_get_chr_unfold_try_another;
	    }else{
      	      cb_fifo_put_chr( &(**cbs).ahd, *chr, storedbytes); // save 'any', 1:st in store
	      cb_fifo_get_chr( &(**cbs).ahd, &(*chr), &storedbytes); // return first in fifo (WSP)
	    } 
	  }else if( CR( *chr ) && err<CBNEGATION ){
	    cb_fifo_put_chr( &(**cbs).ahd, *chr, storedbytes);
	    err = cb_get_chr( &(*cbs), &(*chr), &bytecount, &storedbytes );
	    if(err==CBSTREAM){  cb_fifo_set_stream( &(**cbs).ahd ); }
	    if(err==CBSTREAMEND){  cb_fifo_set_endchr( &(**cbs).ahd ); }

	    if( LF( *chr ) && err<CBNEGATION ){ 
	      cb_fifo_put_chr( &(**cbs).ahd, *chr, storedbytes);
	      err = cb_get_chr( &(*cbs), &(*chr), &bytecount, &storedbytes );
	      if(err==CBSTREAM){  cb_fifo_set_stream( &(**cbs).ahd ); }
	      if(err==CBSTREAMEND){  cb_fifo_set_endchr( &(**cbs).ahd ); }

	      if( WSP( *chr ) && err<CBNEGATION ){
	        cb_fifo_revert_chr( &(**cbs).ahd, &empty, &tmp ); // LF
	        cb_fifo_revert_chr( &(**cbs).ahd, &empty, &tmp ); // CR
	        goto cb_get_chr_unfold_try_another;
	      }else if( err<CBNEGATION ){ 
	        cb_fifo_put_chr( &(**cbs).ahd, *chr, storedbytes ); // save 'any', 3:rd in store
	        err = cb_fifo_get_chr( &(**cbs).ahd, &(*chr), &storedbytes); // return first in fifo (CR)
	        //fprintf(stderr, "\ncb_get_chr_unfold: returning after CRLF'any' chr=0x%.6lX.", *chr);
	      }
	    }else{
	      cb_fifo_put_chr( &(**cbs).ahd, *chr, storedbytes ); // save 'any', 2:nd in store
	      err = cb_fifo_get_chr( &(**cbs).ahd, &(*chr), &storedbytes); // return first in fifo (CR)
	    }
	  }
	  if(err>=CBNEGATION){
            //fprintf(stderr,"\ncb_get_chr_unfold: read error %i, chr:[%c].", err, (int) *chr); 
	    //cb_fifo_print_counters(&(**cbs).ahd);
	  }
	  *chroffset = (*(**cbs).cb).index - 1 - (**cbs).ahd.bytesahead; // Correct offset

	  //fprintf(stderr,"\nchr: [%c] chroffset:%i buffers offset:%i.", (int) *chr, (int) *chroffset, (int) ((**cbs).cb->index-1));
	  return err;
	}else if( (**cbs).ahd.ahead > 0){ // return previously read character
	  err = cb_fifo_get_chr( &(**cbs).ahd, &(*chr), &storedbytes );
	  *chroffset = (*(**cbs).cb).index - 1 - (**cbs).ahd.bytesahead; // 2.12.2013
	  //fprintf(stderr,"\nchr: [%c], from ring, offset:%i , offset:%i.", (int) *chr, (int) *chroffset, (int) ((**cbs).cb->index-1));
	  if(err>=CBNEGATION){
	    fprintf(stderr,"\ncb_get_chr_unfold: read error %i, ahead=%i, bytesahead:%i,\
	       storedbytes=%i, chr=[%c].", err, (**cbs).ahd.ahead, (**cbs).ahd.bytesahead, storedbytes, (int) *chr); 
	    cb_fifo_print_counters( &(**cbs).ahd );
	  }
	  //fprintf(stderr, "\ncb_get_chr_unfold: return [0x%.6lX]. ", *chr );
	  return err; // returns CBSTREAM
	}else
	  return CBARRAYOUTOFBOUNDS;
        return CBERROR;
}

/*
 * Name has one byte for each character.
 */
int  cb_set_cursor_match_length(CBFILE **cbs, unsigned char **name, int *namelength, int ocoffset, int matchctl){
	int indx=0, chrbufindx=0, err=CBSUCCESS, bufsize=0;
	unsigned char *ucsname = NULL; 
	bufsize = *namelength; bufsize = bufsize*4;
	ucsname = (unsigned char*) malloc( sizeof(char)*( 1 + bufsize ) );
	if( ucsname==NULL ){ return CBERRALLOC; }
	ucsname[bufsize]='\0';
	for( indx=0; indx<*namelength && err==CBSUCCESS; ++indx ){
	  err = cb_put_ucs_chr( (unsigned long int)(*name)[indx], &ucsname, &chrbufindx, bufsize);
	}
	err = cb_set_cursor_match_length_ucs( &(*cbs), &ucsname, &chrbufindx, ocoffset, matchctl);
	free(ucsname);
	return err;
}
int  cb_set_cursor(CBFILE **cbs, unsigned char **name, int *namelength){
	return cb_set_cursor_match_length( &(*cbs), &(*name), &(*namelength), 0, 1 );
}

/*
 * 4-byte functions.
 */

/*
 * Sets cursor at name just after rstart, '=' or reads as long as 
 * the name is found. Returns CBERRBUFFULL if name is not found
 * until the buffer is full or if the name found and the buffer
 * is full, CBSTREAM. 
 *
 * Name is compared to a name in input. If the name occurs
 * in input as 31 bit characters in four bytes, the name given 
 * as parameter has to be given in the same format.
 *
 * Name has 4 bytes for one UCS-character.
 */
int  cb_set_cursor_ucs(CBFILE **cbs, unsigned char **ucsname, int *namelength){
	return cb_set_cursor_match_length_ucs( &(*cbs), &(*ucsname), &(*namelength), 0, 1 );
}
int  cb_set_cursor_match_length_ucs(CBFILE **cbs, unsigned char **ucsname, int *namelength, int ocoffset, int matchctl){
	cb_match mctl;
	mctl.re = NULL; mctl.re_extra = NULL; mctl.matchctl = matchctl;
	return cb_set_cursor_match_length_ucs_matchctl(&(*cbs), &(*ucsname), &(*namelength), ocoffset, &mctl);
}
int  cb_set_cursor_match_length_ucs_matchctl(CBFILE **cbs, unsigned char **ucsname, int *namelength, int ocoffset, cb_match *mctl){ // 23.2.2014
	int  err=CBSUCCESS, cis=CBSUCCESS, ret=CBNOTFOUND;
	int  index=0, buferr=CBSUCCESS; 
	unsigned long int chr=0, chprev=CBRESULTEND;
	long int chroffset=0;
	char tovalue = 0; 
	unsigned char charbuf[CBNAMEBUFLEN+1]; // array 30.8.2013
	unsigned char *charbufptr = NULL;
	cb_name *fname = NULL;
	char atvalue=0; 
        unsigned long int ch3prev=CBRESULTEND+2, ch2prev=CBRESULTEND+1;
	int openpairs=0; // cbstatetopology, cbstatetree

	if( cbs==NULL || *cbs==NULL || mctl==NULL){
	  fprintf(stderr,"\ncb_set_cursor_match_length_ucs_matchctl: allocation error."); return CBERRALLOC;
	}
	chprev=(**cbs).cf.bypass+1; // 5.4.2013

	if( (**cbs).cf.searchstate==CBSTATETREE || (**cbs).cf.searchstate==CBSTATETOPOLOGY ) // Search next matching leaf
	  openpairs = ocoffset; // 12.12.2013

	// 1) Search list (or tree leaves) and set cbs.cb.list.index if name is found
	if( ocoffset==0 ){ // 12.12.2013
	  err = cb_set_to_name( &(*cbs), &(*ucsname), *namelength, &(*mctl) ); // 11.3.2014, 23.3.2014
	}else{
	  err = cb_set_to_leaf( &(*cbs), &(*ucsname), *namelength, ocoffset, &(*mctl) ); // mctl 11.3.2014, 23.3.2014
	}
	if(err==CBSUCCESS){
	  //fprintf(stderr,"\nName found from list.");
	  if( (*(**cbs).cb).buflen > ( (*(*(**cbs).cb).list.current).length+(*(*(**cbs).cb).list.current).offset ) ){
	    ret = CBSUCCESS;
	    goto cb_set_cursor_ucs_return;
	  }else
	    fprintf(stderr,"\ncb_set_cursor: Found name but it's length is over the buffer length.\n");
	}else if(err==CBNAMEOUTOFBUF){
	  //fprintf(stderr,"\ncb_set_cursor: Found old name out of cache and stream is allready passed by,\n");
	  //fprintf(stderr,"\n               set cursor as stream and beginning to search the same name again.\n");
	  // but search again the same name; return CBNAMEOUTOFBUF;
	  ;
	}

	// 2) Search stream, save name to buffer,
	//    write name, '=', data and '&' to buffer and update cb_name chain
	if(*ucsname==NULL)
	  return CBERRALLOC;

/*	if((**cbs).cf.searchstate==0)
	  fprintf(stderr, "\nCBSTATELESS");
	if((**cbs).cf.searchstate==1)
	  fprintf(stderr, "\nCBSTATEFUL");
	if((**cbs).cf.searchstate==2)
	  fprintf(stderr, "\nCBSTATETOPOLOGY");
	if((**cbs).cf.searchstate==3)
	  fprintf(stderr, "\nCBSTATETREE");
*/

	// Initialize memory characterbuffer and its counters
	memset( &(charbuf[0]), (int) 0x20, (size_t) CBNAMEBUFLEN);
	if(charbuf==NULL){  return CBERRALLOC; }
	charbuf[CBNAMEBUFLEN]='\0';
	charbufptr = &charbuf[0];

	// Initialize readahead counters and memory before use and before return 
	cb_remove_ahead_offset( &(*cbs), &(**cbs).ahd );
	cb_fifo_init_counters( &(**cbs).ahd );

	// Set cursor to the end to search next names
	(*(**cbs).cb).index = (*(**cbs).cb).contentlen; // -bytesahead ?

	//fprintf(stderr, "\nopenpairs %i ocoffset %i", openpairs, ocoffset );

	// Allocate new name
cb_set_cursor_reset_name_index:
	index=0;

	/*
	 * 6.12.2014:
	 * If name has to be replaced (not a stream function), information where
	 * the name starts is important. Since flow control characters must be
	 * bypassed with \ and only if the name is found, it's put to the index,
	 * name offset can be set here (to test after 6.12.2014). */
	//nameoffset = 

	// Search for new name
	// ...& - ignore and set to start
	// ...= - save any new name and compare it to 'name', if match, return
	err = cb_search_get_chr( &(*cbs), &chr, &chroffset); // 1.9.2013

	while( err<CBERROR && err!=CBSTREAMEND && index < (CBNAMEBUFLEN-3) && buferr <= CBSUCCESS){ 

	  //fprintf(stderr, "[%c]", (int) chr );

	  cb_automatic_encoding_detection( &(*cbs) ); // set encoding automatically if it's set

	  tovalue=0;

	  /*
	   * Read until rstart '='
	   */
	  // doubledelim is not tested 8.12.2013
	  if( (**cbs).cf.searchstate==CBSTATETOPOLOGY || (**cbs).cf.searchstate==CBSTATETREE ){ // '=', save name
	    if( ( chr==(**cbs).cf.rstart || ( chr==(**cbs).cf.subrstart && !intiseven( openpairs ) && (**cbs).cf.doubledelim==1 ) ) && \
	        chprev!=(**cbs).cf.bypass ){ // count of rstarts can be greater than the count of rends
	      //if(openpairs!=0){ // leaf
	      if(openpairs>0){ // leaf, 13.12.2013
	        ++openpairs;
	        tovalue=0;
	      }else if( openpairs==0 ){ // name
                ++openpairs;
	        tovalue=1;
	      }
	      if( (**cbs).cf.searchstate==CBSTATETREE ){ // saves leaves
	        tovalue=1;
	      }
	    }
	  }else if( (**cbs).cf.searchstate==CBSTATEFUL ){
	    if( ( chprev!=(**cbs).cf.bypass && chr==(**cbs).cf.rstart ) && atvalue!=1 ) // '=', save name, 13.4.2013, do not save when = is in value
	      tovalue=1;
	  }else{ // if( (**cbs).cf.searchstate==CBSTATELESS ){
	    if( chprev!=(**cbs).cf.bypass && chr==(**cbs).cf.rstart ) // '=', save name, 5.4.2013
	      tovalue=1;
	  }

	  if(tovalue==1){

	    atvalue=1;

	    if( buferr==CBSUCCESS ){
	      buferr = cb_save_name_from_charbuf( &(*cbs), &fname, chroffset, &charbufptr, index); // 7.12.2013
	      if(buferr==CBNAMEOUTOFBUF || buferr>=CBNEGATION){ fprintf(stderr, "\ncb_set_cursor_ucs: cb_save_name_from_ucs returned %i ", buferr); }
	      if(buferr!=CBNAMEOUTOFBUF){ // cb_save_name_from_charbuf returns CBNAMEOUTOFBUF if buffer is full
	        buferr = cb_put_name(&(*cbs), &fname, openpairs); // (last in list), jos nimi on verrattavissa, tallettaa nimen ja offsetin
	        if( buferr==CBADDEDNAME || buferr==CBADDEDLEAF || buferr==CBADDEDNEXTTOLEAF){
	          buferr = CBSUCCESS;
	        }
	      }
	      if( ! ((**cbs).cf.searchstate==CBSTATETREE || (**cbs).cf.searchstate==CBSTATETOPOLOGY) ){
	        if( (**cbs).cf.leadnames==1 )              // From '=' to '=' and from '&' to '=',
	          goto cb_set_cursor_reset_name_index;     // not just from '&' to '=', 8.12.2013.
	      }else{
	        if( openpairs>(ocoffset+1) && (**cbs).cf.searchstate==CBSTATETREE ) // reset charbuf index if second rstart or more from ocoffset
	          goto cb_set_cursor_reset_name_index;
	        if( openpairs>1 && (**cbs).cf.searchstate==CBSTATETOPOLOGY ) // reset charbuf index if second rstart or more
	          goto cb_set_cursor_reset_name_index; 
	        else if( ocoffset!=0 && openpairs!=(ocoffset+1) ) 
	          goto cb_set_cursor_reset_name_index;
	        else if( openpairs<(ocoffset+1) || openpairs<1 )
	          fprintf(stderr, "\ncb_set_cursor_ucs: error, openpairs<(ocoffset+1) || openpairs<1, %i<%i.", openpairs, (ocoffset+1));
	      }
	    }
	    if(buferr==CBNAMEOUTOFBUF || buferr>=CBNEGATION){
	      fprintf(stderr, "\ncb_set_cursor_ucs: cb_put_name returned %i ", buferr);
	    }

	    if(err==CBSTREAM){ // Set length of namepair to indicate out of buffer.
	      cb_remove_name_from_stream( &(*cbs) ); // also leaves
	      fprintf(stderr, "\ncb_set_cursor: name was out of memory buffer.\n");
	    }
	    /*
	     * Name (openpairs==1) 
	     */
	    if( openpairs<=1 && ocoffset==0 ){ // 2.12.2013, 13.12.2013, name1 from name1.name2.name3
	      //fprintf(stderr,"\nNAME COMPARE");
	      cis = cb_compare( &(*cbs), &(*ucsname), *namelength, &(*fname).namebuf, (*fname).namelen, &(*mctl) ); // 23.11.2013, 11.3.2014, 23.3.2014
	      if( cis == CBMATCH ){ // 9.11.2013
	        (*(**cbs).cb).index = (*(**cbs).cb).contentlen - (**cbs).ahd.bytesahead; // cursor at rstart, 6.9.2013 (this line can be removed)
	        if( (*(**cbs).cb).list.last != NULL ) // matchcount, this is first match, matchcount becomes 1, 25.8.2013
	          (*(*(**cbs).cb).list.last).matchcount++; 
	        if(err==CBSTREAM){
	          //fprintf(stderr,"\nName found from stream.");
	          ret = CBSTREAM; // cursor set, preferably the first time (remember to use cb_remove_name_from_stream)
	          goto cb_set_cursor_ucs_return;
	        }else{
	          //fprintf(stderr,"\nName found from buffer.");
	          ret = CBSUCCESS; // cursor set
	          goto cb_set_cursor_ucs_return;
	        }
	      }
	    }else if( ocoffset!=0 && openpairs==(ocoffset+1) ){ // 13.12.2013, order ocoffset name from name1.name2.name3
	    /*
	     * Leaf (openpairs==(ocoffset+1) if ocoffset > 0)
	     */
	      //fprintf(stderr,"\nLEAF COMPARE");
	      cis = cb_compare( &(*cbs), &(*ucsname), *namelength, &(*fname).namebuf, (*fname).namelen, &(*mctl) ); // 23.11.2013, 11.3.2014, 23.3.2014
	      if( cis == CBMATCH ){ // 9.11.2013
	        (*(**cbs).cb).index = (*(**cbs).cb).contentlen - (**cbs).ahd.bytesahead; // cursor at rstart, 6.9.2013 (this line can be removed)
	        if( (*(**cbs).cb).list.currentleaf != NULL ) // matchcount, this is first match, matchcount becomes 1, 25.8.2013
	          (*(*(**cbs).cb).list.currentleaf).matchcount++; 
	        if(err==CBSTREAM){
	          //fprintf(stderr,"\nLeaf found from stream.");
	          ret = CBSTREAM; // cursor set, preferably the first time (remember to use cb_remove_name_from_stream)
	          goto cb_set_cursor_ucs_return;
	        }else{
	          //fprintf(stderr,"\nLeaf found from buffer.");
	          ret = CBSUCCESS; // cursor set
	          goto cb_set_cursor_ucs_return;
	        }
	      }
	    }else{
	      fprintf(stderr,"\ncb_set_cursor_ucs: error: not leaf nor name, reseting charbuf.");
	    }
	    /*
	     * No match. Reset charbuf to search next name. 
	     */
	    if( (**cbs).cf.searchstate==CBSTATETREE || (**cbs).cf.leadnames==1 ){
	      //fprintf(stderr,"\nRESET (2)");
	      goto cb_set_cursor_reset_name_index; // 15.12.2013
	    }else
	      buferr = cb_put_ucs_chr(chr, &charbufptr, &index, CBNAMEBUFLEN); // write '='
	  }else if( chprev!=(**cbs).cf.bypass && ( chr==(**cbs).cf.rend || \
	            ( chr==(**cbs).cf.subrend && intiseven( openpairs ) && (**cbs).cf.doubledelim==1 ) ) ){
	      /*
	       * '&', start a new name 
	       */
	      if( ( (**cbs).cf.searchstate==CBSTATETOPOLOGY || (**cbs).cf.searchstate==CBSTATETREE ) && openpairs>=1){
	        --openpairs; // reader must read similarly, with openpairs count or otherwice
	        if( (**cbs).cf.json==1 && intiseven( openpairs ) && openpairs>0 && (**cbs).cf.doubledelim==1 ){ // JSON
 	          --openpairs; // If not ',' as should, remove last comma in list: a, b, c } , not yet tested 8.12.2013
	        }
	      }
	      atvalue=0; cb_free_name(&fname); fname=NULL; // allocate_name is at put_leaf and put_name and at cb_save_name_from_charbuf (first: more accurate name, second: shorter length in memory)
 	      goto cb_set_cursor_reset_name_index;
	  }else if(chprev==(**cbs).cf.bypass && chr==(**cbs).cf.bypass){
	      /*
	       * change \\ to one '\' 
	       */
	      chr=(**cbs).cf.bypass+1; // any char not '\'
	  }else if( ( openpairs<0 || openpairs<ocoffset ) && \
	            ((**cbs).cf.searchstate==CBSTATETREE || (**cbs).cf.searchstate==CBSTATETOPOLOGY) ){
	      /*
	       * When reading inner name-value -pairs, read of outer value ended.
	       */
              fprintf(stderr,"\nValue read, openpairs %i ocoffset %i.", openpairs, ocoffset); // debug
 	      return CBVALUEEND; // 12.12.2013
	  }else if( ( (**cbs).cf.searchstate==CBSTATEFUL || (**cbs).cf.searchstate==CBSTATETOPOLOGY ) && atvalue==1){ // Do not save data between '=' and '&' 
	      /*
	       * Indefinite length values. Index does not increase and unordered
	       * values length is not bound to CBNAMEBUFLEN. (index boundary check?)
	       */
	      ;
	  }else if((**cbs).cf.searchstate==CBSTATETREE ){ // save character to buffer, CBSTATETREE
	      buferr = cb_put_ucs_chr(chr, &charbufptr, &index, CBNAMEBUFLEN);
	  }else{ // save character to buffer CBSTATELESS
	      buferr = cb_put_ucs_chr(chr, &charbufptr, &index, CBNAMEBUFLEN);
	  }
          /* Automatic stop at header-end if it's set */
          if((**cbs).cf.rfc2822headerend==1)
            if( ch3prev==0x0D && ch2prev==0x0A && chprev==0x0D && chr==0x0A ){ // cr lf x 2
              if( (*(**cbs).cb).offsetrfc2822 == 0 )
                (*(**cbs).cb).offsetrfc2822 = chroffset; // 1.9.2013, offset set at last new line character
	      ret = CB2822HEADEREND;
	      goto cb_set_cursor_ucs_return;
            }

	  ch3prev = ch2prev; ch2prev = chprev; chprev = chr;
	  err = cb_search_get_chr( &(*cbs), &chr, &chroffset); // 1.9.2013
	    
	}

cb_set_cursor_ucs_return:
	cb_remove_ahead_offset( &(*cbs), &(**cbs).ahd ); // poistetaanko tassa kahteen kertaan 6.9.2013 ?
        cb_free_name(&fname); // fname on kaksi kertaa, put_name kopioi sen uudelleen
	fname = NULL; // lisays
	if( ret==CBSUCCESS || ret==CBSTREAM || ret==CBMATCH || ret==CBMATCHLENGTH) // match* on turha, aina stream tai success
	  if( (**cbs).cb!=NULL && (*(**cbs).cb).list.current!=NULL ){
	    if( (*(*(**cbs).cb).list.current).lasttimeused == -1 )
	      (*(*(**cbs).cb).list.current).lasttimeused = (*(*(**cbs).cb).list.current).firsttimefound; // first time found
	    else
	      (*(*(**cbs).cb).list.current).lasttimeused = (signed long int) time(NULL); // last time used in seconds
	  }
	return ret;

        return CBNOTFOUND;
}

/*
 * Allocates fname */
int cb_save_name_from_charbuf(CBFILE **cbs, cb_name **fname, long int offset, unsigned char **charbuf, int index){
	unsigned long int cmp=0x61;
	int indx=0, buferr=CBSUCCESS, err=CBSUCCESS;
	char atname=0;

	if( cbs==NULL || *cbs==NULL || fname==NULL || charbuf==NULL )
	  return CBERRALLOC;

	err = cb_allocate_name(&(*fname), (index+1) ); // moved here 7.12.2013 ( +1 is one over the needed size )
	if(err!=CBSUCCESS){  return err; } // 30.8.2013

	(**fname).namelen = index; // tuleeko olla vasta if:n jalkeen
	(**fname).offset = offset;

	//fprintf(stderr,"\n cb_save_name_from_charbuf: name [");
	//cb_print_ucs_chrbuf(&(*charbuf), index, CBNAMEBUFLEN);
	//fprintf(stderr,"] index=%i, (**fname).buflen=%i cstart:[%c] cend:[%c] ", index, (**fname).buflen, (char) (**cbs).cf.cstart, (char) (**cbs).cf.cend);

        if(index<(**fname).buflen){ 
          (**fname).namelen=0; // 6.4.2013
	  while(indx<index && buferr==CBSUCCESS){ // 4 bytes (UCS word)
cb_save_name_ucs_removals: 
            buferr = cb_get_ucs_chr( &cmp, &(*charbuf), &indx, CBNAMEBUFLEN); // 30.8.2013
            if( buferr==CBSUCCESS ){
              if( cmp==(**cbs).cf.cstart ){ // Comment
                while( indx<index && cmp!=(**cbs).cf.cend && buferr==CBSUCCESS ){ // 2.9.2013
                  buferr = cb_get_ucs_chr( &cmp, &(*charbuf), &indx, CBNAMEBUFLEN); // 30.8.2013
                }
              }
              if( (**cbs).cf.removewsp==1 && WSP( cmp ) && ( (**cbs).cf.removenamewsp==1 || atname==0 ) ){ // WSP:s between value and name
	        if( indx<index && buferr==CBSUCCESS ){
	          goto cb_save_name_ucs_removals; // 13.12.2013
	        }
	      }
	      if( (**cbs).cf.removecrlf==1 ){ // Removes every CR and LF characters between value and name, and in name
                if( indx<index && ( CR( cmp ) || LF( cmp ) ) && buferr==CBSUCCESS ){
	          goto cb_save_name_ucs_removals; 
	        }                
	      }

              /* Write name */
              if( cmp!=(**cbs).cf.cend && buferr==CBSUCCESS){ // Name, 28.8.2013
                if( ! NAMEXCL( cmp ) ){ // bom should be replaced if it's not first in stream
	          if( ! ( indx==index && (**cbs).cf.removewsp==1 && WSP( cmp ) ) ){ // special last white space, 13.12.2013
                    buferr = cb_put_ucs_chr( cmp, &(**fname).namebuf, &(**fname).namelen, (**fname).buflen );
	            //fprintf(stderr,"[%c]", (char) cmp);
	            atname=1;
	          }
	        }
              }
            }
          }
        }else{
	  err = CBNAMEOUTOFBUF;
	}

	return err;
}

int  cb_automatic_encoding_detection(CBFILE **cbs){
	int enctest=CBDEFAULTENCODING;
	if(cbs==NULL || *cbs==NULL )
	  return CBERRALLOC;
	if((**cbs).encoding==CBENCAUTO || ( (**cbs).encoding==CBENCPOSSIBLEUTF16LE && (*(**cbs).cb).contentlen == 4 ) ){
	  if( (*(**cbs).cb).contentlen == 4 || (*(**cbs).cb).contentlen == 2 || (*(**cbs).cb).contentlen == 3 ){ // UTF-32, 4; UTF-16, 2; UTF-8, 3;
	    // 32 is multiple of 16. Testing it again results correct encoding without loss of bytes or errors.
	    enctest = cb_bom_encoding( &(*cbs) );
	    if( enctest==CBENCUTF8 || enctest==CBENCUTF16BE || enctest==CBENCUTF16LE || enctest==CBENCPOSSIBLEUTF16LE || enctest==CBENCUTF32LE || enctest==CBENCUTF32BE ){
	      cb_set_encoding( &(*cbs), enctest);
	      return CBSUCCESS;
	    }else{
	      cb_set_encoding( &(*cbs), CBDEFAULTENCODING );
	      return CBAUTOENCFAIL;
	    }
	  }else
	    return CBEMPTY;
	}
	return CBNOTSET;
}

int  cb_remove_name_from_stream(CBFILE **cbs){
	if(cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || (*(**cbs).cb).list.current==NULL )
	  return CBERRALLOC;
	(*(*(**cbs).cb).list.current).length =  (*(**cbs).cb).buflen;
	return CBSUCCESS;
}
