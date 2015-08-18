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


int  cb_put_name(CBFILE **str, cb_name **cbn, int openpairs, int ocoffset);
int  cb_put_leaf(CBFILE **str, cb_name **leaf, int openpairs, int ocoffset);
int  cb_set_to_last_leaf(cb_name **tree, cb_name **lastleaf, int *openpairs); // sets openpairs, not yet tested 7.12.2013
int  cb_set_to_leaf(CBFILE **cbs, unsigned char **name, int namelen, int openpairs, cb_match *mctl); // 11.3.2014, 23.3.2014
int  cb_set_to_leaf_inner(CBFILE **cbs, unsigned char **name, int namelen, int openpairs, cb_match *mctl); // 16.3.2014, 23.3.2014
int  cb_set_to_name(CBFILE **str, unsigned char **name, int namelen, cb_match *mctl); // 11.3.2014, 23.3.2014
int  cb_search_get_chr( CBFILE **cbs, unsigned long int *chr, long int *chroffset);
int  cb_save_name_from_charbuf(CBFILE **cbs, cb_name **fname, long int offset, unsigned char **charbuf, int index, long int nameoffset);
int  cb_automatic_encoding_detection(CBFILE **cbs);

int  cb_check_json_name( unsigned char **ucsname, int *namelength ); // Test 19.2.2015


/*
 * Returns a pointer to 'result' from 'leaf' tree matching the name and namelen with CBFILE cb_conf,
 * matchctl and openpairs count. Returns CBNOTFOUND if leaf was not found. 'result' may be NULL. 
 *
 * Returns 
 *   on success: CBSUCCESS, CBSUCCESSLEAVESEXIST
 *   on negation: CBEMPTY, CBNOTFOUND, CBNOTFOUNDLEAVESEXIST, CBNAMEOUTOFBUF (if not file or seekable file)
 *   on error: CBERRALLOC.
 */
int  cb_set_to_leaf(CBFILE **cbs, unsigned char **name, int namelen, int openpairs, cb_match *mctl){ // 23.3.2014
	if(cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || name==NULL || mctl==NULL){ 
	  cb_log( &(*cbs), CBLOGALERT, "\ncb_set_to_leaf: allocation error."); return CBERRALLOC; 
	}
	if( (*(**cbs).cb).list.current==NULL )
	  return CBEMPTY;
	(*(**cbs).cb).list.currentleaf = &(* (cb_name*) (*(*(**cbs).cb).list.current).leaf);
	return cb_set_to_leaf_inner( &(*cbs), &(*name), namelen, openpairs, &(*mctl)); // 11.3.2014
}
// unsigned to 'int namelen' 6.12.2014
int  cb_set_to_leaf_inner(CBFILE **cbs, unsigned char **name, int namelen, int openpairs, cb_match *mctl){ // 11.4.2014, 23.3.2014
	int err = CBSUCCESS; char nextlevelempty=1;
	cb_name *leafptr = NULL;
	if(cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || name==NULL || mctl==NULL){
	  cb_log( &(*cbs), CBLOGALERT, "\ncb_set_to_leaf_inner: allocation error."); return CBERRALLOC;
	}

        cb_clog( CBLOGDEBUG, "\ncb_set_to_leaf_inner: debug:" );

	if( (*(**cbs).cb).list.currentleaf==NULL ){
          cb_clog( CBLOGDEBUG, " currentleaf was empty, returning." );
	  return CBEMPTY;
	}
	// Node
	leafptr = &(*(*(**cbs).cb).list.currentleaf);
    	/*
	 * Return value comparison. */
	if( leafptr!=NULL ) // 3.7.2015
	  if( (*leafptr).leaf!=NULL ) // 3.7.2015
	    nextlevelempty=0; // 3.7.2015

	cb_clog( CBLOGDEBUG, " comparing ["); 
	if(name!=NULL)
		cb_print_ucs_chrbuf( CBLOGDEBUG, &(*name), namelen, namelen );
	cb_clog( CBLOGDEBUG, "] to [");
	if(leafptr!=NULL)
		cb_print_ucs_chrbuf( CBLOGDEBUG, &(*leafptr).namebuf, (*leafptr).namelen, (*leafptr).buflen );
	cb_clog( CBLOGDEBUG, "] openpairs %i", openpairs);

	err = cb_compare( &(*cbs), &(*name), namelen, &(*leafptr).namebuf, (*leafptr).namelen, &(*mctl)); // 11.4.2014, 23.3.2014
	if(err==CBMATCH){
	  if( openpairs < 0 ){ 
	    cb_log( &(*cbs), CBLOGWARNING,  "\ncb_set_to_leaf: error: openpairs was %i.", openpairs); // debug
	    if( nextlevelempty==0 ) // 3.7.2015
	      return CBNOTFOUNDLEAVESEXIST; // 3.7.2015
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

// ONGELMA 1, vertaus samaan kohtaan set_to_name:ssa:
//1.7.2015 name: if( ( (**str).cf.searchmethod==CBSEARCHNEXTNAMES && (*iter).matchcount==1 ) || (**str).cf.searchmethod==CBSEARCHUNIQUENAMES ){

            if( ( (**cbs).cf.searchmethod==CBSEARCHNEXTLEAVES && (*leafptr).matchcount==1 ) || (**cbs).cf.searchmethod==CBSEARCHUNIQUELEAVES ){ 
              if( (**cbs).cf.type!=CBCFGFILE && (**cbs).cf.type!=CBCFGSEEKABLEFILE ){ // When used as only buffer, stream case does not apply
                if((*leafptr).offset>=( (*(**cbs).cb).buflen + 0 + 1 ) ){ // buflen + smallest possible name + endchar
                  /*
                   * Leafs content was out of memorybuffer, setting
                   * CBFILE:s index to the edge of the buffer.
                   */
                  (*(**cbs).cb).index = (*(**cbs).cb).buflen; // set as a stream, 1.12.2013
                  return CBNAMEOUTOFBUF;
                }
	      }
              (*(**cbs).cb).index = (long int) (*leafptr).offset; // 1.12.2013

// MUUTETTU RETURN CBSUCCESS LOHKON SISAAN 2.7.2015: 
	      if(nextlevelempty==0) // 3.7.2015
		return CBSUCCESSLEAVESEXIST; // 3.7.2015
	      cb_clog( CBLOGDEBUG, "\ncb_set_to_leaf_inner: debug: currentleaf was set and index %li is now set.", (*(**cbs).cb).index );
	      return CBSUCCESS; // CBMATCH
            }
	  }
	}
	if( (*leafptr).leaf!=NULL && openpairs>=1 ){ // Left
	  (*(**cbs).cb).list.currentleaf = &(* (cb_name*) (*leafptr).leaf);
	  err = cb_set_to_leaf_inner( &(*cbs), &(*name), namelen, (openpairs-1), &(*mctl)); // 11.4.2014, 23.3.2014
	  if(err==CBSUCCESS || err==CBSUCCESSLEAVESEXIST) // 3.7.2015
	    return err;
	}
	if( (*leafptr).next!=NULL ){ // Right
	  (*(**cbs).cb).list.currentleaf = &(* (cb_name*) (*leafptr).next);
	  err = cb_set_to_leaf_inner( &(*cbs), &(*name), namelen, openpairs, &(*mctl)); // 11.4.2014, 23.3.2014
	  if(err==CBSUCCESS || err==CBSUCCESSLEAVESEXIST ) // 3.7.2015
	    return err;
	}
	(*(**cbs).cb).list.currentleaf = NULL;
        if( nextlevelempty==0 ) // 3.7.2015
          return CBNOTFOUNDLEAVESEXIST; // 3.7.2015
	return CBNOTFOUND;
}
/*
 * Returns 
 *   on success: CBSUCCESS 
 *   on negation: CBEMPTY 
 *   on error: CBERRALLOC, CBLEAFCOUNTERROR
 */
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
	  cb_clog( CBLOGWARNING, "\ncb_set_to_last_leaf: CBLEAFCOUNTERROR.");
	  return CBLEAFCOUNTERROR;
	}
	return CBSUCCESS; // returns allways a proper not null node or CBEMPTY, in error CBLEAFCOUNTERROR
}

/*
 * Put leaf in 'current' name -tree and update 'currentleaf'. */
int  cb_put_leaf(CBFILE **str, cb_name **leaf, int openpairs, int ocoffset){

	/*
	 *  openpairs=1  openpairs=2  openpairs=3  openpairs=2    openpairs=1  openpairs=0
	 *  atvalue=1    atvalue=1    atvalue=1    atvalue=1      atvalue=1    atvalue=0
	 *      |            |             |           |              |            |
	 * name=     subname=     subname2= value      &              &            & name2=
	 *      |--------- subsearch ---------------------------------------------->
	 * 
	 *
	 * JSON pre 19.2.2015:
	 *  openpairs=1 openpairs=2     openpairs=4               openpairs=2     openpairs=0
	 *      |           | openpairs=3   |       (openpairs=3 )    | (openpairs=1) |
	 *      |           |     |         |            |            |      |        |
	 *      {    subname:     { subname2: value     (,)           }     (,)       }, (name2): {
	 *      |---------------------------- subsearch ------------------------------>
	 *
	 *
	 *   Valuestrings between ":s, arrays in between "[" and "]", strings false|true|null
	 *   and numbers are read elsewhere
	 *
	 *
	 * Comment to replace array:
	 *     {    subname:     { subname2: [ value, value2, value3 ] }     (,)       }, (name2): {
	 *
	 *
	 *     0         1  1         2       1         2         1 0         1 1         2        1 0         1          0 
	 *     {  subname:  { subname2: value , subname3: value3  } , subname4: { subname5: value5 } , subname6: value6   } 
	 *   
	 *
	 *   rstart    ':'
	 *   rend      ','                                     1) Colon problem:
	 *   subrstart '{'                                           "na:me"  "va:lue"
	 *   subrend   '}'                                     2) Comma problem:
	 *   cstart    '['                                           "va,lue"
	 *   cend      ']'
	 *                                                     Solution: char injsonquotes; // from " to "
	 *
	 * 	RFC-7159 , JSON
	 * 	RFC-4627 , JSON
	 */

	int leafcount=0, err=CBSUCCESS, ret=CBSUCCESS;
	cb_name *lastleaf  = NULL;
	cb_name *newleaf   = NULL;

	if( str==NULL || *str==NULL || (**str).cb==NULL || leaf==NULL || *leaf==NULL) return CBERRALLOC;

	cb_log( &(*str), CBLOGDEBUG, "\ncb_put_leaf: openpairs=%i ocoffset=%i [", openpairs, ocoffset);
	cb_print_ucs_chrbuf( CBLOGDEBUG, &(**leaf).namebuf, (**leaf).namelen, (**leaf).buflen );
	cb_log( &(*str), CBLOGDEBUG, "]");

	/*
	 * Find last leaf */
	leafcount = openpairs;
	err = cb_set_to_last_leaf( &(*(**str).cb).list.current, &lastleaf, &leafcount);
	if(err>=CBERROR){ cb_log( &(*str), CBLOGERR, "\ncb_put_leaf: cb_set_to_last_leaf returned error %i.", err); }
	(*(**str).cb).list.currentleaf = &(*lastleaf); // last
	if( leafcount==0 || err==CBEMPTY ){ 
	  return CBEMPTY;
	}else if(openpairs<=1){
	  return CBLEAFCOUNTERROR;
	}

	/*
	 * Allocate and copy leaf */
        err = cb_allocate_name( &newleaf, (**leaf).namelen ); 
	if(err!=CBSUCCESS){ cb_log( &(*str), CBLOGALERT, "\ncb_put_leaf: cb_allocate_name error %i.", err); return err; }
        err = cb_copy_name( &(*leaf), &newleaf );
	if(err!=CBSUCCESS){ cb_log( &(*str), CBLOGERR, "\ncb_put_leaf: cb_copy_name error %i.", err); return err; }
	(*newleaf).firsttimefound = (signed long int) time(NULL);
	(*newleaf).next = NULL;
	(*newleaf).leaf = NULL;

	/* 
	 * Update previous leafs length */
	//cb_log( &(*str), CBLOGDEBUG, "\ncb_put_leaf: Update previous leafs length: new leafs nameoffset %li openpairs %i ocoffset %i .", (**leaf).nameoffset, openpairs, ocoffset );
	if( (*(**str).cb).list.last!=NULL && (*(**str).cb).list.currentleaf!=NULL && (**leaf).nameoffset>=(*(*(**str).cb).list.last).offset){ // Is in last name in list
	  //cb_log( &(*str), CBLOGDEBUG, "\nUpdate previous leafs length: currentleafs offset %li ", (*(*(**str).cb).list.currentleaf).offset );
	  // Presuming the currentleaf is the last leaf (and certainly is after last name)
          if( (*(**str).cb).list.namecount>=1 && (**leaf).nameoffset>=(2+(*(*(**str).cb).list.currentleaf).offset) && (**leaf).nameoffset<0x7FFFFFFF ){
	    if( openpairs==1 && ocoffset==0 ){ // to list (longest length)
	    	//cb_log( &(*str), CBLOGDEBUG, "\ncb_put_leaf: 1");
		;
	    }else if( openpairs==1 && ocoffset>0 ){  // from leaf to it's name in list
	    	//cb_log( &(*str), CBLOGDEBUG, "\ncb_put_leaf: 2");
	        (*(*(**str).cb).list.currentleaf).length = (**leaf).nameoffset - (*(*(**str).cb).list.currentleaf).offset - 2; // 2: '=' and '&'
	    }else if( openpairs==2 && ocoffset==0 ){ // from name to it's leaf
	    	//cb_log( &(*str), CBLOGDEBUG, "\ncb_put_leaf: 3");
	    	;
	    }else if( openpairs>1 && ocoffset>0 ){ // from leaf to it's name or leaf
	    	//cb_log( &(*str), CBLOGDEBUG, "\ncb_put_leaf: 4");
		if( openpairs-ocoffset == 0 ) // to leafs list ( ! intiseven( openpairs ) )
	          (*(*(**str).cb).list.currentleaf).length = (**leaf).nameoffset - (*(*(**str).cb).list.currentleaf).offset - 2;
		else // to leafs leaf
		  ;
	    }
	  }
	}


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
	    cb_log( &(*str), CBLOGWARNING, "\ncb_put_leaf: error, leafcount>openpairs.");
	    return CBLEAFCOUNTERROR;
	  }
	}

	(*(**str).cb).list.currentleaf = &(*lastleaf);

	newleaf=NULL; // 27.2.2015
	lastleaf=NULL; // 27.2.2015

	if(err>=CBNEGATION)
	  return err;
	return ret;
}

int  cb_put_name(CBFILE **str, cb_name **cbn, int openpairs, int ocoffset){ // ocoffset late, 12/2014
        int err=0; 

        if(cbn==NULL || *cbn==NULL || str==NULL || *str==NULL || (**str).cb==NULL )
          return CBERRALLOC;

	//cb_log( &(*str), CBLOGDEBUG, "\ncb_put_name: openpairs=%i ocoffset=%i [", openpairs, ocoffset);
	//cb_print_ucs_chrbuf( &(**cbn).namebuf, (**cbn).namelen, (**cbn).buflen );
	//cb_log( &(*str), CBLOGDEBUG, "]");

        /*
         * Do not save the name if it's out of buffer. cb_set_cursor finds
         * names from stream but can not use cb_names namelist if the names
         * are out of buffer. When used as file (CBCFGFILE or CBCFGSEEKABLEFILE),
         * filesize limits the list and this is not necessary.
         */
        if((**str).cf.type!=CBCFGFILE && (**str).cf.type!=CBCFGSEEKABLEFILE)
          if( (**cbn).offset >= ( (*(**str).cb).buflen-1 ) || ( (**cbn).offset + (**cbn).length ) >= ( (*(**str).cb).buflen-1 ) ) 
            return CBNAMEOUTOFBUF;

        if((*(**str).cb).list.name!=NULL){
          (*(**str).cb).list.current = &(*(*(**str).cb).list.last);
          (*(**str).cb).list.currentleaf = &(*(*(**str).cb).list.last); // 11.12.2013

	  
	  // Add leaf or name to a leaf, 2.12.2013
	  if(openpairs>1){ // first '=' is openpairs==1 (used only in CBSTATETREE)
	    //err = cb_put_leaf( &(*(**str).cb).list.current, &(*cbn), openpairs); // openpairs chooses between *next and *leaf
	    err = cb_put_leaf( &(*str), &(*cbn), openpairs, ocoffset); // openpairs chooses between *next and *leaf
	    return err;
	  }

	  /*                                  +--- length ------+
	   *                                  |                 |
	   *         &    # comment \n   name1=                 &     name2=
	   *         |                        |                 |     |    |
	   *    1nameoffset                1offset          2nameoffset   2offset
	   *         +---- 1namearealength ---+--- 1contentlen -+     +----+ 2namelen
	   *  
 	   *            namearealength = 1offset - 1nameoffset;
 	   *            1contentlength = 2nameoffset - 1offset = length
	   * 14.12.2014
	   */
	  // Previous names contents length
	  if( (*(**str).cb).list.last!=NULL ){
            if( (*(**str).cb).list.namecount>=1 && (**cbn).nameoffset>=( (*(*(**str).cb).list.last).offset + 2 ) && (**cbn).nameoffset<0x7FFFFFFF ){
	      if( openpairs==0 && ocoffset==0 ){ // to list (longest length)
	         (*(*(**str).cb).list.last).length = (**cbn).nameoffset - (*(*(**str).cb).list.last).offset - 2;
	      }else if( openpairs==0 && ocoffset==1 && (*(**str).cb).list.currentleaf!=NULL ){ // from leaf to it's next leaf in list
		  if( (*(*(**str).cb).list.currentleaf).offset >= (*(*(**str).cb).list.last).offset ) // is lasts leaf
		    if( (**cbn).nameoffset >= ( (*(*(**str).cb).list.currentleaf).offset + 2 ) ) // leaf is after this leaf
	              (*(*(**str).cb).list.currentleaf).length = (**cbn).nameoffset - (*(*(**str).cb).list.currentleaf).offset - 2; // 2: '=' and '&'
	            // since read pointer is allways at read position, leaf can be added without knowing if one leaf is missing in between
	      }else if( openpairs==1 && ocoffset==0 ){ // from name to it's leaf
		  ;
	      }else if( openpairs>1 && ocoffset>=0 ){ // from leaf to it's leaf
	          ;
	      }
	    }
	  }

	  // Add name
          (*(**str).cb).list.last    = &(* (cb_name*) (*(*(**str).cb).list.last).next );

          err = cb_allocate_name( &(*(**str).cb).list.last, (**cbn).namelen ); if(err!=CBSUCCESS){ return err; } // 7.12.2013
          (*(*(**str).cb).list.current).next = &(*(*(**str).cb).list.last); // previous
          (*(**str).cb).list.current = &(*(*(**str).cb).list.last);
          (*(**str).cb).list.currentleaf = &(*(*(**str).cb).list.last);
          ++(*(**str).cb).list.namecount;
	  if( (**str).cf.type!=CBCFGFILE && (**str).cf.type!=CBCFGSEEKABLEFILE) // 20.12.2014
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

/*
 * Returns 
 *   on success: CBSUCCESS, CBSUCCESSLEAVESEXIST
 *   on negation: CBEMPTY, CBNOTFOUND, CBNOTFOUNDLEAVESEXIST, CBNAMEOUTOFBUF (if not file or seekable file)
 *   on error: CBERRALLOC.
 */
int  cb_set_to_name(CBFILE **str, unsigned char **name, int namelen, cb_match *mctl){ // cb_match 11.3.2014, 23.3.2014
	cb_name *iter = NULL; int err=CBSUCCESS; char nextlevelempty=1;

	if(*str!=NULL && (**str).cb != NULL && mctl!=NULL ){
	  iter = &(*(*(**str).cb).list.name);
	  while(iter != NULL){
	    /*
	     * Helpful return value comparison. */
	    if( (*iter).leaf!=NULL ) // 3.7.2015
	      nextlevelempty=0; // 3.7.2015
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
	        if( (**str).cf.type!=CBCFGFILE && (**str).cf.type!=CBCFGSEEKABLEFILE) // When used as only buffer, stream case does not apply
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
		if(nextlevelempty==0) // 3.7.2015
		  return CBSUCCESSLEAVESEXIST; // 3.7.2015
		cb_clog( CBLOGDEBUG, "\ncb_set_to_name: debug: current, currentleaf and index %li is set.", (*(**str).cb).index );
	        return CBSUCCESS;
	      }
	    }
	    iter = &(* (cb_name *) (*iter).next); // 9.12.2013
	  }
	  (*(**str).cb).index = (*(**str).cb).contentlen - (**str).ahd.bytesahead;  // 5.9.2013
	}else{
	  cb_log( &(*str), CBLOGALERT, "\ncb_set_to_name: allocation error.");
	  return CBERRALLOC; // 30.3.2013
	}
	if(nextlevelempty==0)
	  return CBNOTFOUNDLEAVESEXIST;
	return CBNOTFOUND;
}

int  cb_search_get_chr( CBFILE **cbs, unsigned long int *chr, long int *chroffset ){
	int err = CBSUCCESS, bytecount=0, storedbytes=0;
	if(cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || chroffset==NULL || chr==NULL){
	  cb_clog( CBLOGDEBUG,"\ncb_search_get_chr: null parameter was given, error %i.", CBERRALLOC);
	  return CBERRALLOC;
	}
	if( (**cbs).cf.unfold==1 && (*(**cbs).cb).contentlen==( (**cbs).ahd.currentindex+(**cbs).ahd.bytesahead ) ){
	  ; // cb_clog( CBLOGDEBUG,"\ncb_search_get_chr: unfolding and at the end of buffer, 1.");
	}else if( (**cbs).cf.unfold==1 ){
	  /* 
	   * Not at the end of buffer.
	   * Reading of a value should stop at the rend. After rend, the readahead should be empty
	   * because LWSP characters are not read after the character. Emptying the read ahead buffer. */
	  //cb_clog( CBLOGDEBUG,"\ncb_search_get_chr: Unfold read was not at the end of buffer. Emptying the read ahead buffer.");
	  err = cb_fifo_init_counters( &(**cbs).ahd );
	  if( err>=CBERROR ){ cb_clog( CBLOGERR, "\ncb_search_get_chr: cb_fifo_init_counters, error %i.", err ); }
	}
	if((**cbs).cf.unfold==1){
	  return cb_get_chr_unfold( &(*cbs), &(**cbs).ahd, &(*chr), &(*chroffset) );
	}else{
	  err = cb_get_chr( &(*cbs), &(*chr), &bytecount, &storedbytes );
	  // Stream, buffer, file
	  *chroffset = (*(**cbs).cb).index - 1; // 2.12.2013
	  // Seekable file
	  // was 2.8.2015: if( (**cbs).cf.type==CBCFGSEEKABLEFILE )
	  // was 2.8.2015:   if( (*(**cbs).cb).readlength>=0 ) // overflow
	  if( (**cbs).cf.type==CBCFGSEEKABLEFILE )
	    if( (*(**cbs).cb).readlength>0 ) // no overflow, 2.8.2015
	      *chroffset = (*(**cbs).cb).readlength - 1;
	  return err;
	}
	return CBERROR;
}

/*
 * Name has one byte for each character.
 */
int  cb_set_cursor_match_length(CBFILE **cbs, unsigned char **name, int *namelength, int ocoffset, int matchctl){
	int indx=0, err=CBSUCCESS; int bufsize=0;
	int chrbufindx=0;
	unsigned char *ucsname = NULL; 
	bufsize = *namelength; bufsize = bufsize*4;
	ucsname = (unsigned char*) malloc( sizeof(char)*( (unsigned int) bufsize + 1 ) );
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
	mctl.re = NULL; mctl.re_extra = NULL; mctl.matchctl = matchctl; mctl.resmcount = 0;
	return cb_set_cursor_match_length_ucs_matchctl(&(*cbs), &(*ucsname), &(*namelength), ocoffset, &mctl);
}
int  cb_set_cursor_match_length_ucs_matchctl(CBFILE **cbs, unsigned char **ucsname, int *namelength, int ocoffset, cb_match *mctl){ // 23.2.2014
	int  err=CBSUCCESS, cis=CBSUCCESS, ret=CBNOTFOUND; // , errcontlen=0;
	int  buferr=CBSUCCESS; 
	int  index=0;
	unsigned long int chr=0, chprev=CBRESULTEND;
	long int chroffset=0;
	long int nameoffset=0; // 6.12.2014
	char tovalue = 0;
	char injsonquotes = 0, wasjsonrend=0; // TEST 10.7.2015, was: , wasjsonsubrend=0;
	unsigned char charbuf[CBNAMEBUFLEN+1]; // array 30.8.2013
	unsigned char *charbufptr = NULL;
	cb_name *fname = NULL;
	char atvalue=0; 
	char rstartflipflop=0; // After subrstart '{', rend ';' has the same effect as concecutive rstart '=' and rend ';' (empty value).
			       // After rstart '=', subrstart '{' or subrend '}' has no effect until rend ';'. 
			       // For example: name { name2=text2; name3 { name4=text4; name5=text5; } }
			       // (JSON can be replaced by setting rstart '"', rend '"' and by removing ':' from name)
        unsigned long int ch3prev=CBRESULTEND+2, ch2prev=CBRESULTEND+1;
	int openpairs=0; // cbstatetopology, cbstatetree

	if( cbs==NULL || *cbs==NULL || mctl==NULL){
	  cb_log( &(*cbs), CBLOGALERT, "\ncb_set_cursor_match_length_ucs_matchctl: allocation error."); return CBERRALLOC;
	}
	chprev=(**cbs).cf.bypass+1; // 5.4.2013

	cb_clog( CBLOGDEBUG, "\ncb_set_cursor_match_length_ucs_matchctl: ocoffset %i, matchctl %i", ocoffset, (*mctl).matchctl );
	if( (**cbs).cf.searchmethod==CBSEARCHUNIQUENAMES )
		cb_clog( CBLOGDEBUG, ", unique names");
	else if( (**cbs).cf.searchmethod==CBSEARCHNEXTNAMES )
		cb_clog( CBLOGDEBUG, ", next names");
	if( (**cbs).cf.leafsearchmethod==CBSEARCHUNIQUENAMES )
		cb_clog( CBLOGDEBUG, ", unique leaves");
	else if( (**cbs).cf.searchmethod==CBSEARCHNEXTNAMES )
		cb_clog( CBLOGDEBUG, ", next leaves");
	if( (**cbs).cf.searchstate==CBSTATETREE )
		cb_clog( CBLOGDEBUG, ", cbstatetree.");
	else
		cb_clog( CBLOGDEBUG, ", not cbstatetree.");

	//if(*ucsname==NULL || namelength==NULL ){ // 13.12.2014
	if( ucsname==NULL || *ucsname==NULL || namelength==NULL ){ // 7.7.2015
	  cb_log( &(*cbs), CBLOGALERT, "\ncb_set_cursor_match_length_ucs_matchctl: allocation error, ucsname or length was null.");
	  return CBERRALLOC;
	}

	if( (**cbs).cf.searchstate==CBSTATETREE || (**cbs).cf.searchstate==CBSTATETOPOLOGY ) // Search next matching leaf
	  openpairs = ocoffset; // 12.12.2013

	// 1) Search list (or tree leaves) and set cbs.cb.list.index if name is found
	if( ocoffset==0 ){ // 12.12.2013
	  err = cb_set_to_name( &(*cbs), &(*ucsname), *namelength, &(*mctl) ); // 11.3.2014, 23.3.2014
	}else{
	  err = cb_set_to_leaf( &(*cbs), &(*ucsname), *namelength, ocoffset, &(*mctl) ); // mctl 11.3.2014, 23.3.2014
	}
	// if(err==CBSUCCESS){
	if(err==CBSUCCESS || err==CBSUCCESSLEAVESEXIST){ // 3.7.2015
	  //cb_log( &(*cbs), CBLOGDEBUG, "\nName found from list.");
	  if( (*(**cbs).cb).buflen > ( (*(*(**cbs).cb).list.current).length+(*(*(**cbs).cb).list.current).offset ) ){
	    ret = CBSUCCESS;
	    goto cb_set_cursor_ucs_return;
	  }else{
	    /* Found name but it's length is over the buffer length. */
	    /*
	     * Stream case CBCFGSTREAM. Used also in CBCFGFILE (unseekable). CBCFGBUFFER just in case. */
	    cb_log( &(*cbs), CBLOGDEBUG, "\ncb_set_cursor: Found name but it's length is over the buffer length.\n");
	    /*
	     * 15.12.2014: matchcount prevents matching again. If seekable file is in use,
	     * file can be seeked to old position. In this case, found names offset is returned
	     * even if it's out of buffers boundary. 
	     */
	    if( (**cbs).cf.type==CBCFGSEEKABLEFILE ){
	       ret = CBSUCCESS;
	       goto cb_set_cursor_ucs_return; // palautuva offset oli kuitenkin viela puskurin kokoinen
	    }                                 // returning offset was still the size of a buffer
	  }
	}else if(err==CBNAMEOUTOFBUF){
	  //cb_log( &(*cbs), CBLOGDEBUG, "\ncb_set_cursor: Found old name out of cache and stream is allready passed by,\n");
	  //cb_log( &(*cbs), CBLOGDEBUG, "\n               set cursor as stream and beginning to search the same name again.\n");
	  // but search again the same name; return CBNAMEOUTOFBUF;
	  /* 15.12.2014: cb_set_to_name return CBSUCCESS if CBCFGFILE or CBCFGSEEKABLEFILE */
	  ;
	}

	// 2) Search stream, save name to buffer,
	//    write name, '=', data and '&' to buffer and update cb_name chain
	if(*ucsname==NULL)
	  return CBERRALLOC;

/*
	if((**cbs).cf.searchstate==0)
	  cb_log( &(*cbs), CBLOGDEBUG, "\nCBSTATELESS");
	if((**cbs).cf.searchstate==1)
	  cb_log( &(*cbs), CBLOGDEBUG, "\nCBSTATEFUL");
	if((**cbs).cf.searchstate==2)
	  cb_log( &(*cbs), CBLOGDEBUG, "\nCBSTATETOPOLOGY");
	if((**cbs).cf.searchstate==3)
	  cb_log( &(*cbs), CBLOGDEBUG, "\nCBSTATETREE");
*/

	// Initialize memory characterbuffer and its counters
	memset( &(charbuf[0]), (int) 0x20, (size_t) CBNAMEBUFLEN);
	if(charbuf==NULL){  return CBERRALLOC; }
	charbuf[CBNAMEBUFLEN]='\0';
	charbufptr = &charbuf[0];

	// Set cursor to the end to search next names
	(*(**cbs).cb).index = (*(**cbs).cb).contentlen; // -bytesahead ?

	//cb_log( &(*cbs), CBLOGDEBUG, "\nopenpairs %i ocoffset %i", openpairs, ocoffset );


	//cb_log( &(*cbs), CBLOGDEBUG, "\nring buffer data first %i last %i {", (**cbs).ahd.first, (**cbs).ahd.last );
	//cb_fifo_print_buffer( &(**cbs).ahd, CBLOGDEBUG );
	//cb_log( &(*cbs), CBLOGDEBUG, "} ");


	// Allocate new name
cb_set_cursor_reset_name_index:
	index=0;
	injsonquotes=0;


	// Search for new name
	// ...& - ignore and set to start
	// ...= - save any new name and compare it to 'name', if match, return
	err = cb_search_get_chr( &(*cbs), &chr, &chroffset); // 1.9.2013

	//cb_clog( CBLOGDEBUG, "|%c,0x%X|", (unsigned char) chr, (unsigned char) chr );
	cb_clog( CBLOGDEBUG, "|%c|", (unsigned char) chr );
	//fprintf(stderr,"[%c (err %i)]", (unsigned char) chr, err);
	//cb_log( &(*cbs), CBLOGDEBUG, "\nset_cursor: new name, index (chr offset %li).", chroffset );
        //cb_clog( CBLOGDEBUG, "\nring buffer counters:");
        //cb_fifo_print_counters( &(**cbs).ahd, CBLOGDEBUG );
        //cb_clog( CBLOGDEBUG, "\ndata [");
	//cb_fifo_print_buffer( &(**cbs).ahd, CBLOGDEBUG );
        //cb_clog( CBLOGDEBUG, "]");

	/*
	 * 6.12.2014:
	 * If name has to be replaced (not a stream function), information where
	 * the name starts is important. Since flow control characters must be
	 * bypassed with '\' and only if the name is found, it's put to the index,
	 * name offset can be set here (to test after 6.12.2014). Programmer 
	 * should leave the cursor where the data ends, at '&'. */
	nameoffset = chroffset;
	//cb_log( &(*cbs), CBLOGDEBUG, "\nset_cursor: nameoffset %li", nameoffset);

	while( err<CBERROR && err!=CBSTREAMEND && index < (CBNAMEBUFLEN-3) && buferr <= CBSUCCESS){ 

	  //cb_log( &(*cbs), CBLOGDEBUG, "\n[%c]", (int) chr );
	  //cb_log( &(*cbs), CBLOGDEBUG, ", atvalue=%i, tovalue=%i, openpairs=%i, injsonquotes=%i", atvalue, tovalue, openpairs, injsonquotes );

	  cb_automatic_encoding_detection( &(*cbs) ); // set encoding automatically if it's set

	  tovalue=0;


	  /*
	   * Read until rstart '='
	   */
	  // doubledelim is not tested 8.12.2013 (17.8.2015: every other subrstart subrstop pair does not work well)
	  if( (**cbs).cf.searchstate==CBSTATETOPOLOGY || (**cbs).cf.searchstate==CBSTATETREE ){ // '=', save name

	    /*
	     * JSON quotes (is never reset by anything else). */
	    if( chprev!=(**cbs).cf.bypass && chr==(unsigned long int)'\"' ){
	      if( injsonquotes==0 ) // first quote
	        ++injsonquotes;
	      else if( injsonquotes==1 ) // second quote
	        injsonquotes=2; // ready to save the name (or after value)
	      else if( injsonquotes==2 ) // after second quote
	        injsonquotes=0; // after the name (or value ended with '"')
	    }

	    // Second is JSON comma problem ("na:me") prevention in name 21.2.2015
	    if(       (    ( chr==(**cbs).cf.rstart && (**cbs).cf.json!=1 ) \
	                || ( chr==(**cbs).cf.rstart && (**cbs).cf.json==1 && injsonquotes!=1 ) \
	                || ( chr==(**cbs).cf.subrstart && rstartflipflop!=1 && (**cbs).cf.doubledelim==1 && (**cbs).cf.json!=1 ) \
	                || ( chr==(**cbs).cf.subrstart && (**cbs).cf.json==1 && injsonquotes!=1 ) \
	              ) && chprev!=(**cbs).cf.bypass ){ // count of rstarts can be greater than the count of rends
	      wasjsonrend=0;

	      if( chr==(**cbs).cf.rstart) // 17.8.2015
		rstartflipflop=1;

	      if( chr==(**cbs).cf.subrstart && (**cbs).cf.json==1 ) { // 21.2.2015 JSON
		injsonquotes=0;
	        tovalue=0;
	        goto cb_set_cursor_reset_name_index; // update openpairs only from ':' (JSON rstart is ':')
	      } // /21.2.2015
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

	    //cb_clog( CBLOGDEBUG, "\n Going to save name from charbuf to fname." );
	    //if( fname==NULL )
	    //  cb_clog( CBLOGDEBUG, "\n fname is null." );
	    //if( charbufptr==NULL )
	    //  cb_clog( CBLOGDEBUG, "\n charbufptr is null." );

	    if( buferr==CBSUCCESS ){ 

	      buferr = cb_save_name_from_charbuf( &(*cbs), &fname, chroffset, &charbufptr, index, nameoffset); // 7.12.2013, 6.12.2014
	      if(buferr==CBNAMEOUTOFBUF || buferr>=CBNEGATION){ cb_log( &(*cbs), CBLOGNOTICE, "\ncb_set_cursor_ucs: cb_save_name_from_ucs returned %i ", buferr); }

	      //cb_clog( CBLOGDEBUG, "\ncb_save_name_from_charbuf: %i .", buferr );

	      //if(buferr!=CBNAMEOUTOFBUF ){ // cb_save_name_from_charbuf returns CBNAMEOUTOFBUF if buffer is full
	      if(buferr!=CBNAMEOUTOFBUF && fname!=NULL ){ // cb_save_name_from_charbuf returns CBNAMEOUTOFBUF if buffer is full

		//cb_clog( CBLOGDEBUG, "\n Openpairs=%i, ocoffset=%i, injsonquotes=%i, index=%li going to cb_check_json_name |", openpairs, ocoffset, injsonquotes, (*(**cbs).cb).index );
		//cb_print_ucs_chrbuf( CBLOGDEBUG, &(*fname).namebuf, (*fname).namelen, (*fname).namelen);
		//cb_clog( CBLOGDEBUG, "| ");

		if( (**cbs).cf.json!=1 || ( (**cbs).cf.json==1 && ( ( injsonquotes==2 && (**cbs).cf.jsonnamecheck!=1 ) || \
				( injsonquotes==2 && (**cbs).cf.jsonnamecheck==1 && \
				cb_check_json_name( &(*fname).namebuf, &(*fname).namelen )!=CBNOTJSON ) ) ) ){ // 19.2.2015
	          buferr = cb_put_name(&(*cbs), &fname, openpairs, ocoffset); // (last in list), jos nimi on verrattavissa, tallettaa nimen ja offsetin


		  //cb_clog( CBLOGDEBUG, "\n Put name |");
		  //cb_print_ucs_chrbuf( CBLOGDEBUG, &(*fname).namebuf, (*fname).namelen, (*fname).namelen);
		  //cb_clog( CBLOGDEBUG, "|, openpairs=%i, injsonquotes=%i ", openpairs, injsonquotes );
		  injsonquotes=0;

	          if( buferr==CBADDEDNAME || buferr==CBADDEDLEAF || buferr==CBADDEDNEXTTOLEAF){
	            buferr = CBSUCCESS;
	          }
	        }
	      }
	      if( (**cbs).cf.searchstate!=CBSTATETREE && (**cbs).cf.searchstate!=CBSTATETOPOLOGY ){
	        if( (**cbs).cf.leadnames==1 ){              // From '=' to '=' and from '&' to '=',
	          goto cb_set_cursor_reset_name_index;     // not just from '&' to '=', 8.12.2013.
		}
	      }else{
	        if( openpairs>(ocoffset+1) && (**cbs).cf.searchstate==CBSTATETREE ) // reset charbuf index if second rstart or more from ocoffset
	          goto cb_set_cursor_reset_name_index;
	        if( openpairs>1 && (**cbs).cf.searchstate==CBSTATETOPOLOGY ) // reset charbuf index if second rstart or more
	          goto cb_set_cursor_reset_name_index;
	        else if( ocoffset!=0 && openpairs!=(ocoffset+1) )
	          goto cb_set_cursor_reset_name_index;
	        else if( openpairs<(ocoffset+1) || openpairs<1 )
	          cb_log( &(*cbs), CBLOGDEBUG, "\ncb_set_cursor_ucs: error, openpairs<(ocoffset+1) || openpairs<1, %i<%i.", openpairs, (ocoffset+1));
	      }
	    }
	    if(buferr==CBNAMEOUTOFBUF || buferr>=CBNEGATION){
	      cb_log( &(*cbs), CBLOGNOTICE, "\ncb_set_cursor_ucs: cb_put_name returned %i ", buferr);
	    }

	    if(err==CBSTREAM){ // Set length of namepair to indicate out of buffer.
	      cb_remove_name_from_stream( &(*cbs) ); // also leaves
	      cb_log( &(*cbs), CBLOGNOTICE, "\ncb_set_cursor: name was out of memory buffer.\n");
	    }
	    /*
	     * Name (openpairs==1)
	     */

           // If unfolding is used and last read resulted an already read character (one character maximum) 
           // and index is zero, do not save the empty name.

	   if( openpairs<=1 && ocoffset==0 ){ // 2.12.2013, 13.12.2013, name1 from name1.name2.name3
	      //cb_log( &(*cbs), CBLOGDEBUG, "\nNAME COMPARE");
	      cis = cb_compare( &(*cbs), &(*ucsname), *namelength, &(*fname).namebuf, (*fname).namelen, &(*mctl) ); // 23.11.2013, 11.3.2014, 23.3.2014
	      if( cis == CBMATCH ){ // 9.11.2013
	        (*(**cbs).cb).index = (*(**cbs).cb).contentlen - (**cbs).ahd.bytesahead; // cursor at rstart, 6.9.2013 (this line can be removed)
	        if( (*(**cbs).cb).list.last != NULL ) // matchcount, this is first match, matchcount becomes 1, 25.8.2013
	          (*(*(**cbs).cb).list.last).matchcount++; 
	        if(err==CBSTREAM){
	          //cb_log( &(*cbs), CBLOGDEBUG, "\nName found from stream.");
	          ret = CBSTREAM; // cursor set, preferably the first time (remember to use cb_remove_name_from_stream)
	          goto cb_set_cursor_ucs_return;
	        }else if(err==CBFILESTREAM){
	          //cb_log( &(*cbs), CBLOGDEBUG, "\nName found from file stream (seekable).");
	          ret = CBFILESTREAM; // cursor set, preferably the first time
	          goto cb_set_cursor_ucs_return;
	        }else{
	          //cb_log( &(*cbs), CBLOGDEBUG, "\nName found from buffer.");
	          ret = CBSUCCESS; // cursor set
	          goto cb_set_cursor_ucs_return;
	        }
	      }
	    }else if( ocoffset!=0 && openpairs==(ocoffset+1) ){ // 13.12.2013, order ocoffset name from name1.name2.name3
	      /*
	       * Leaf (openpairs==(ocoffset+1) if ocoffset > 0)
	       */
	      //cb_log( &(*cbs), CBLOGDEBUG, "\nLEAF COMPARE");
	      cis = cb_compare( &(*cbs), &(*ucsname), *namelength, &(*fname).namebuf, (*fname).namelen, &(*mctl) ); // 23.11.2013, 11.3.2014, 23.3.2014
	      if( cis == CBMATCH ){ // 9.11.2013
	        (*(**cbs).cb).index = (*(**cbs).cb).contentlen - (**cbs).ahd.bytesahead; // cursor at rstart, 6.9.2013 (this line can be removed)
	        if( (*(**cbs).cb).list.currentleaf != NULL ) // matchcount, this is first match, matchcount becomes 1, 25.8.2013
	          (*(*(**cbs).cb).list.currentleaf).matchcount++; 
	        if(err==CBSTREAM){
	          //cb_log( &(*cbs), CBLOGDEBUG, "\nLeaf found from stream.");
	          ret = CBSTREAM; // cursor set, preferably the first time (remember to use cb_remove_name_from_stream)
	          goto cb_set_cursor_ucs_return;
	        }else if(err==CBFILESTREAM){
	          //cb_log( &(*cbs), CBLOGDEBUG, "\nLeaf found from stream.");
	          ret = CBFILESTREAM; // cursor set, preferably the first time (remember to use cb_remove_name_from_stream)
	          goto cb_set_cursor_ucs_return;
	        }else{
	          //cb_log( &(*cbs), CBLOGDEBUG, "\nLeaf found from buffer.");
	          ret = CBSUCCESS; // cursor set
	          goto cb_set_cursor_ucs_return;
	        }
	      }
	    }else{
	      /*
	       * Mismatch */
	      // Debug (or verbose error messages)
	      //cb_log( &(*cbs), CBLOGDEBUG, "\ncb_set_cursor_ucs: error: not leaf nor name, reseting charbuf, namecount %ld.", (*(**cbs).cb).list.namecount );
	    }
	    /*
	     * No match. Reset charbuf to search next name. 
	     */
	    if( (**cbs).cf.searchstate==CBSTATETREE || (**cbs).cf.leadnames==1 ){
	      goto cb_set_cursor_reset_name_index; // 15.12.2013
	    }else
	      buferr = cb_put_ucs_chr(chr, &charbufptr, &index, CBNAMEBUFLEN); // write '='

	  }else if( chprev!=(**cbs).cf.bypass && ( ( chr==(**cbs).cf.rend && (**cbs).cf.json!=1 ) || \
	                           ( chr==(**cbs).cf.rend && (**cbs).cf.json==1 && injsonquotes!=1 ) || \
	                           ( (**cbs).cf.json!=1 && chr==(**cbs).cf.subrend && rstartflipflop!=1 && (**cbs).cf.doubledelim==1 ) || \
	                           ( (**cbs).cf.json==1 && injsonquotes!=1 && chr==(**cbs).cf.subrend ) ) ){

	      if( chr==(**cbs).cf.rend )
		rstartflipflop=0;

	      /*
	       * '&', start a new name, 27.2.2015: solution to '}' + ',' -> --openpairs once
	       */
	      if( ( (**cbs).cf.searchstate==CBSTATETOPOLOGY || (**cbs).cf.searchstate==CBSTATETREE ) && openpairs>=1){
	        if( (**cbs).cf.json==1 && chr==(**cbs).cf.subrend && openpairs>0 ){ // JSON
	          // (JSON rend is ',' subrend is '}' ) . If not ',' as should, remove last comma in list: a, b, c } 21.2.2015
		  if( wasjsonrend==0 )
			--openpairs;
		  wasjsonrend=0;
	          injsonquotes=0;
	        }else{ 

		  if( (**cbs).cf.json==1 && wasjsonrend==0 )  
		    wasjsonrend=1; 
	          --openpairs; // reader must read similarly, with openpairs count or otherwice
		}
	      }
	      atvalue=0; 
	      cb_free_name(&fname);
	      fname=NULL; // allocate_name is at put_leaf and put_name and at cb_save_name_from_charbuf (first: more accurate name, second: shorter length in memory)
// 11.7.2015: from rstart to rend, value contains WSP characters outside jsonquotes
	      // TEST 10.7.2015
	      if( (**cbs).cf.json==1 && injsonquotes!=1 && openpairs<ocoffset ){
		/*
		 * JSON special: concecutive '}' and ',' */
		cb_log( &(*cbs), CBLOGDEBUG, "\nopenpairs<ocoffset, %i. ocoffset %i. TEST 10.7.2015, returning VALUEEND", openpairs, ocoffset );
	        injsonquotes=0;
		return CBVALUEEND;
	      }
 	      goto cb_set_cursor_reset_name_index;
	  }else if(chprev==(**cbs).cf.bypass && chr==(**cbs).cf.bypass){
	      /*
	       * change \\ to one '\' 
	       */
	      chr=(**cbs).cf.bypass+1; // any char not '\'
	  }else if(     ( \
			  ( (**cbs).cf.json==0 && (openpairs<0 || openpairs<ocoffset) ) && \
		          ( (**cbs).cf.searchstate==CBSTATETREE || (**cbs).cf.searchstate==CBSTATETOPOLOGY ) \
		        ) || ( \
		          ( (**cbs).cf.json==1 && (openpairs<0 || openpairs<(ocoffset-1) ) ) && \
		          ( (**cbs).cf.searchstate==CBSTATETREE || (**cbs).cf.searchstate==CBSTATETOPOLOGY) \
			) ){ // 27.2.2015: test JSON: TEST WAS: FAIL ( '}'+',' should be -1, not -2 ) , STILL HERE, AN ERROR
	      /*
	       * When reading inner name-value -pairs, read of outer value ended.
	       */
              //cb_log( &(*cbs), CBLOGDEBUG, "\nValue read, openpairs %i ocoffset %i.", openpairs, ocoffset); // debug
 	      return CBVALUEEND; // 12.12.2013
	  }else if( ( (**cbs).cf.searchstate==CBSTATEFUL || (**cbs).cf.searchstate==CBSTATETOPOLOGY ) && atvalue==1){ // Do not save data between '=' and '&' 
	      /*
	       * Indefinite length values. Index does not increase and unordered
	       * values length is not bound to CBNAMEBUFLEN. (index boundary check?)
	       */
	      ;
	  }else if((**cbs).cf.searchstate==CBSTATETREE ){ // save character to buffer, CBSTATETREE
	      // Next json condition has no effect, 11.7.2015
	      if( (**cbs).cf.json!=1 || ( (**cbs).cf.json==1 && ( injsonquotes==1 || injsonquotes==2 ) ) ) // 11.7.2015, if json, write name only if inside quotes (json name is always "string")
	      	buferr = cb_put_ucs_chr(chr, &charbufptr, &index, CBNAMEBUFLEN);
	  }else{ // save character to buffer CBSTATELESS
	      buferr = cb_put_ucs_chr(chr, &charbufptr, &index, CBNAMEBUFLEN);
	  }
          /* Automatic stop at header-end if it's set */
          if((**cbs).cf.rfc2822headerend==1)
            if( ch3prev==0x0D && ch2prev==0x0A && chprev==0x0D && chr==0x0A ){ // cr lf x 2
              if( (*(**cbs).cb).offsetrfc2822 == 0 )
	        if( chroffset < 0x7FFFFFFF) // integer size - 1 
                  (*(**cbs).cb).offsetrfc2822 = chroffset; // 1.9.2013, offset set at last new line character
	      ret = CB2822HEADEREND;
	      goto cb_set_cursor_ucs_return;
            }

	  ch3prev = ch2prev; ch2prev = chprev; chprev = chr;
	  /*
	   * cb_search_get_chr returns maximum bufsize offset */
	  err = cb_search_get_chr( &(*cbs), &chr, &chroffset); // 1.9.2013

	  cb_clog( CBLOGDEBUG, "%c|", (unsigned char) chr );
	  //fprintf(stderr,"[%c (err %i)]", (unsigned char) chr, err);
	  //cb_clog( CBLOGDEBUG, "\nring buffer counters:");
	  //cb_fifo_print_counters( &(**cbs).ahd, CBLOGDEBUG );
          //cb_clog( CBLOGDEBUG, "\ndata [");
	  //cb_fifo_print_buffer( &(**cbs).ahd, CBLOGDEBUG );
          //cb_clog( CBLOGDEBUG, "]");

	}

	//if(err==CBSTREAMEND) // 27.12.2014
	//  ret = CBSTREAMEND;

cb_set_cursor_ucs_return:

	//cb_log( &(*cbs), CBLOGDEBUG, "\nring buffer data first %i last %i {", (**cbs).ahd.first, (**cbs).ahd.last );
	//cb_fifo_print_buffer( &(**cbs).ahd, CBLOGDEBUG );
	//cb_log( &(*cbs), CBLOGDEBUG, "} ");

        cb_free_name(&fname); // fname on kaksi kertaa, put_name kopioi sen uudelleen
	fname = NULL; // lisays
	if( ret==CBSUCCESS || ret==CBSTREAM || ret==CBFILESTREAM || ret==CBMATCH || ret==CBMATCHLENGTH ) // match* on turha, aina stream tai success
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
int cb_save_name_from_charbuf(CBFILE **cbs, cb_name **fname, long int offset, unsigned char **charbuf, int index, long int nameoffset){
	unsigned long int cmp=0x61;
	int buferr=CBSUCCESS, err=CBSUCCESS;
	int indx = 0;
	char atname=0;

	if( cbs==NULL || *cbs==NULL || fname==NULL || charbuf==NULL || *charbuf==NULL ){
	  //cb_clog( CBLOGDEBUG, "\ncb_save_name_from_charbuf: parameter was NULL, CBERRALLOC.");
	  return CBERRALLOC;
	}

	//cb_log( &(*cbs), CBLOGDEBUG, "\ncb_save_name_from_charbuf: before malloc size %i", (index+1) );
	//if( fname==NULL )
	//   cb_log( &(*cbs), CBLOGDEBUG, " fname was NULL");
	//if( *fname==NULL )
	//   cb_log( &(*cbs), CBLOGDEBUG, " *fname was NULL");
	err = cb_allocate_name( &(*fname), (index+1) ); // moved here 7.12.2013 ( +1 is one over the needed size )
	//cb_log( &(*cbs), CBLOGDEBUG, ".\ncb_save_name_from_charbuf: after malloc, %i .", err);
	if(err!=CBSUCCESS){  return err; } // 30.8.2013

	(**fname).namelen = index;
	(**fname).offset = offset;
	(**fname).nameoffset = nameoffset; // 6.12.2014
	//cb_log( &(*cbs), CBLOGDEBUG, "\ncb_save_name_from_charbuf: setting nameoffset %ld .", nameoffset);

	//cb_log( &(*cbs), CBLOGDEBUG, "\n cb_save_name_from_charbuf: name [");
	//cb_print_ucs_chrbuf( CBLOGDEBUG, &(*charbuf), index, CBNAMEBUFLEN);
	//cb_log( &(*cbs), CBLOGDEBUG, "] index=%i, (**fname).buflen=%i cstart:[%c] cend:[%c] ", index, (**fname).buflen, (char) (**cbs).cf.cstart, (char) (**cbs).cf.cend);

        if(index<(**fname).buflen){ 
          (**fname).namelen=0; // 6.4.2013
	  while(indx<index && buferr==CBSUCCESS){ // 4 bytes (UCS word)
cb_save_name_ucs_removals: 
            buferr = cb_get_ucs_chr( &cmp, &(*charbuf), &indx, CBNAMEBUFLEN); // 30.8.2013
            if( buferr==CBSUCCESS ){
              if( cmp==(**cbs).cf.cstart ){ // Comment (+ in case of JSON, cstart and cend is set to '[' and ']'. Reading of array to avoid extra commas.)
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
	            //cb_log( &(*cbs), CBLOGDEBUG, "[%c]", (char) cmp);
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

/*
 * Colons may appear in valuestrings or names. If name starts with " and ends
 * with " no colons can be in between. Names with colons are not found !!
 * against the principle of JSON. http://www.json.org 
 *
 * This function checks that name has '"' at start and end. This way it was
 * not cut in between name. This check is put before cb_put_name in
 * cb_set_cursor.
 *
 * Colons should be programmed to be bypassed in searching the names in 
 * namelist.
 */
int  cb_check_json_name( unsigned char **ucsname, int *namelength ){
	int err=CBSUCCESS, indx=0;
	unsigned long int chr=0x20, chprev=0x20;
	if( ucsname==NULL || *ucsname==NULL || namelength==NULL ){ cb_clog( CBLOGDEBUG, "\ncb_check_json_name: name was null."); return CBERRALLOC; }
                
	/*
	 * "String can have any UNICODE character except '"' or '\' or control characters", [json.org] 
	 */

	if(*namelength<2){
	        cb_clog( CBLOGDEBUG, "\ncb_check_json_name: returning CBNOTJSON: ");
		cb_print_ucs_chrbuf( CBLOGDEBUG, &(*ucsname), *namelength, *namelength);
	        cb_clog( CBLOGDEBUG, " name length was %i (double quotes do not fit in), returning.", *namelength );
	        return CBNOTJSON;
	}        
	err = cb_get_ucs_chr( &chr, &(*ucsname), &indx, *namelength);
	if( chr != (unsigned long int) '"' ){ // first quotation
	        cb_clog( CBLOGDEBUG, "\ncb_check_json_name: returning CBNOTJSON: ");
		cb_print_ucs_chrbuf( CBLOGDEBUG, &(*ucsname), *namelength, *namelength);
		cb_clog( CBLOGDEBUG, " first character was not quotation (length %d).", *namelength );
	        return CBNOTJSON;
	}

	// Linear white spaces, CR and LF character were removed from name.
	// Control characters are not checked
	indx = *namelength-8;
	err  = cb_get_ucs_chr( &chprev, &(*ucsname), &indx, *namelength );
	indx = *namelength-4;
	err = cb_get_ucs_chr( &chr, &(*ucsname), &indx, *namelength );
	if( chprev!= (unsigned long int) '\\' && chr == (unsigned long int) '"' ){
          //cb_clog( CBLOGDEBUG, "\ncb_check_json_name: |");
	  //cb_print_ucs_chrbuf( CBLOGDEBUG, &(*ucsname), *namelength, *namelength);
          //cb_clog( CBLOGDEBUG, "| returning success.");
	  return CBSUCCESS; // second quotation
	}
	cb_clog( CBLOGDEBUG, "\ncb_check_json_name: returning CBNOTJSON: ");
	cb_print_ucs_chrbuf( CBLOGDEBUG, &(*ucsname), *namelength, *namelength);
	cb_clog( CBLOGDEBUG, " last character was not quotation (length %d).", *namelength );
	return CBNOTJSON;
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
	if( (**cbs).cf.type!=CBCFGSEEKABLEFILE && (**cbs).cf.type!=CBCFGFILE )
		(*(*(**cbs).cb).list.current).length =  (*(**cbs).cb).buflen;
	return CBSUCCESS;
}
