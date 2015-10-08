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


int  cb_put_name(CBFILE **str, cb_name **cbn, int openpairs, int previousopenpairs);
int  cb_put_leaf(CBFILE **str, cb_name **leaf, int openpairs, int previousopenpairs);
int  cb_update_previous_length(CBFILE **str, long int nameoffset, int openpairs, int previousopenpairs); // 21.8.2015
int  cb_set_to_last_leaf(cb_name **tree, cb_name **lastleaf, int *openpairs); // sets openpairs, not yet tested 7.12.2013
int  cb_set_to_leaf(CBFILE **cbs, unsigned char **name, int namelen, int openpairs, int *level, cb_match *mctl); // 11.3.2014, 23.3.2014
int  cb_set_to_leaf_inner(CBFILE **cbs, unsigned char **name, int namelen, int openpairs, int *level, cb_match *mctl); // 16.3.2014, 23.3.2014
int  cb_set_to_leaf_inner_levels(CBFILE **cbs, unsigned char **name, int namelen, int openpairs, int *level, cb_match *mctl);
int  cb_set_to_name(CBFILE **str, unsigned char **name, int namelen, cb_match *mctl); // 11.3.2014, 23.3.2014
int  cb_search_get_chr( CBFILE **cbs, unsigned long int *chr, long int *chroffset);
int  cb_save_name_from_charbuf(CBFILE **cbs, cb_name **fname, long int offset, unsigned char **charbuf, int index, long int nameoffset);
int  cb_automatic_encoding_detection(CBFILE **cbs);

int  cb_get_current_level_at_edge(CBFILE **cbs, int *level); // addition 29.9.2015
int  cb_get_current_level(CBFILE **cbs, int *level); // to be used with cb_set_to_name -- addition 28.9.2015
int  cb_get_current_level_sub(cb_name **cn, int *level); // addition 28.9.2015

int  cb_init_terminalcount(CBFILE **cbs); // 29.9.2015, count of downwards flow control characters read at the at the last edge of the stream from the last leaf ( '}' '}' '}' = 3 from the last leaf) 
int  cb_increase_terminalcount(CBFILE **cbs); // 29.9.2015
//int  cb_clear_read_state(CBFILE **cbs); // 29.9.2015, zeros the counters to read the next value with cb_get_chr

int  cb_check_json_name( unsigned char **ucsname, int *namelength ); // Test 19.2.2015


/**
int  cb_clear_read_state(CBFILE **cbs){
	if(cbs==NULL || *cbs==NULL || (**cbs).cb==NULL ) return CBERRALLOC;
	//(*(**cbs).cb).list.rd.injsonquotes = 0; (*(**cbs).cb).list.rd.injsonarray = 0;
	//(*(**cbs).cb).list.rd.wasjsonrend = 0; (*(**cbs).cb).list.rd.wasjsonsubrend = 0;
	//(*(**cbs).cb).list.rd.rstartflipflop = 1;
	(*(**cbs).cb).list.rd.lastreadchrendedtovalue=0; // lastreadchrwasfromoutside=0;
	return CBSUCCESS;
}
**/

/*
 * Returns a pointer to 'result' from 'leaf' tree matching the name and namelen with CBFILE cb_conf,
 * matchctl and openpairs count. Returns CBNOTFOUND if leaf was not found. 'result' may be NULL. 
 *
 * Returns
 *   on success: CBSUCCESS, CBSUCCESSLEAVESEXIST
 *   on negation: CBEMPTY, CBNOTFOUND, CBNOTFOUNDLEAVESEXIST, CBNAMEOUTOFBUF (if not file or seekable file)
 *   on error: CBERRALLOC.
 *
 * Level contains information on what level the previous search was left. If the next name has been
 * read, the length of the previous name should be >=0. If the next leaf is read, previous leafs length
 * should be >=0. If all of the leaves are read and names length has not been updated, cursor should be
 * at last rend, '&'. If last length was >=0, levels is set to 0 to read again the next name.
 */
int  cb_set_to_leaf(CBFILE **cbs, unsigned char **name, int namelen, int openpairs, int *level, cb_match *mctl){ // 23.3.2014
	if(cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || name==NULL || mctl==NULL){ 
	  cb_log( &(*cbs), CBLOGALERT, "\ncb_set_to_leaf: allocation error."); return CBERRALLOC; 
	}

	if( (*(**cbs).cb).list.current==NULL ){
	  *level = 0;
	  return CBEMPTY;
	}
	*level = 1; // current was not null, at least a name is found at level one
	(*(**cbs).cb).list.currentleaf = &(* (cb_name*) (*(*(**cbs).cb).list.current).leaf);
	if( (*(**cbs).cb).list.currentleaf!=NULL ){ // addition 28.9.2015
	  *level = 2;
	}
	return cb_set_to_leaf_inner( &(*cbs), &(*name), namelen, openpairs, &(*level), &(*mctl)); // 11.3.2014
}
// unsigned to 'int namelen' 6.12.2014
int  cb_set_to_leaf_inner(CBFILE **cbs, unsigned char **name, int namelen, int openpairs, int *level, cb_match *mctl){ // 11.4.2014, 23.3.2014
	int err=CBSUCCESS, currentlevel=0; 
	if(cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || name==NULL || mctl==NULL){
	  cb_log( &(*cbs), CBLOGALERT, "\ncb_set_to_leaf_inner: allocation error."); return CBERRALLOC;
	}

	currentlevel = *level;

	err = cb_set_to_leaf_inner_levels( &(*cbs), &(*name), namelen, openpairs, &currentlevel, &(*mctl) );
	if(err>=CBERROR){ cb_clog( CBLOGERR, "\ncb_set_to_leaf_inner: cb_set_to_leaf_inner_levels, error %i.", err ); }
	if(err<CBNEGATION){ return err; } // level is correct if CBSUCCESS or CBSUCCESSLEAVESEXIST

	/*
	 * currentlevel is counted from the last level and it's open pair. Also the toterminal has to 
	 * include the first open pair - if *level is more than 0: toterminal+1 . */
	//if( currentlevel>1 )
	//  --currentlevel;
	*level = currentlevel - (*(**cbs).cb).list.toterminal; 
	if(*level<0){ // to be sure errors do not occur
	  cb_clog( CBLOGDEBUG, "\ncb_set_to_leaf_inner: error, levels was negative, %i.", *level );
	  *level=0; 
	}

	return err; // found or negation
}
// level 19.8.2015
int  cb_set_to_leaf_inner_levels(CBFILE **cbs, unsigned char **name, int namelen, int openpairs, int *level, cb_match *mctl){ // 11.4.2014, 23.3.2014
	int err = CBSUCCESS; char nextlevelempty=1;
	cb_name *leafptr = NULL;
	if(cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || name==NULL || mctl==NULL){
	  cb_log( &(*cbs), CBLOGALERT, "\ncb_set_to_leaf_inner_levels: allocation error."); return CBERRALLOC;
	}

	if( ( CBMAXLEAVES - 1 + openpairs ) < 0 || *level > CBMAXLEAVES ){ // just in case an erroneus loop hangs the application
	  cb_clog( CBLOGDEBUG, "\ncb_set_to_leaf_inner_levels: error, endless loop prevention, level exceeded %i.", *level);
	  return CBNOTFOUND;
	}

	if( (*(**cbs).cb).list.currentleaf==NULL ){
          //cb_clog( CBLOGDEBUG, "\ncb_set_to_leaf_inner_levels: currentleaf was empty, returning CBEMPTY with level %i.", *level );
	  return CBEMPTY;
	}
	// Node
	leafptr = &(*(*(**cbs).cb).list.currentleaf);
    	/*
	 * Return value comparison. */
	if( leafptr!=NULL ){ // 3.7.2015
	  if( (*leafptr).leaf!=NULL ) // 3.7.2015
	    nextlevelempty=0; // 3.7.2015
	}

	//cb_clog( CBLOGDEBUG, " comparing ["); 
	//cb_clog( CBLOGDEBUG, " ["); 
	//if(name!=NULL)
	//	cb_print_ucs_chrbuf( CBLOGDEBUG, &(*name), namelen, namelen );
	//cb_clog( CBLOGDEBUG, "] to [");
	//if(leafptr!=NULL)
	//	cb_print_ucs_chrbuf( CBLOGDEBUG, &(*leafptr).namebuf, (*leafptr).namelen, (*leafptr).buflen );
	//cb_clog( CBLOGDEBUG, "] " );

	if( openpairs == 1 ){ // 1 = first leaf (leaf or next)

	  err = cb_compare( &(*cbs), &(*name), namelen, &(*leafptr).namebuf, (*leafptr).namelen, &(*mctl)); // 11.4.2014, 23.3.2014, new location 19.8.2015

	  if(err==CBMATCH){ // 19.8.2015
            /*
             * 9.12.2013 (from set_to_name):
             * If searching of multiple same names is needed (in buffer also), do not return 
             * allready matched name. Instead, increase names matchcount (or set to 1 if it 
             * becomes 0) and search the next same name.
             */
            (*leafptr).matchcount++; if( (*leafptr).matchcount==0 ){ (*leafptr).matchcount+=2; }

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
	      } // this bracked added 17.8.2015 (the same as before)
              (*(**cbs).cb).index = (long int) (*leafptr).offset; // 1.12.2013

	      if(nextlevelempty==0){ // 3.7.2015
  	        return CBSUCCESSLEAVESEXIST; // 3.7.2015
	      }
	      return CBSUCCESS; // CBMATCH
            }//else
	      //cb_clog( CBLOGDEBUG, "\ncb_set_to_leaf_inner_levels: match, unknown searchmethod (%i,%i) or matchcount %li not 1.", (**cbs).cf.searchmethod, (**cbs).cf.leafsearchmethod, (*leafptr).matchcount );	
	  }//else
	    //cb_clog( CBLOGDEBUG, "\ncb_set_to_leaf_inner_levels: no match, openpairs==1 (effective with match)");
	}//else
	  //cb_clog( CBLOGDEBUG, "\ncb_set_to_leaf_inner_levels: openpairs was not 1.");

	if( (*leafptr).leaf!=NULL && openpairs>=1 ){ // Left
	  (*(**cbs).cb).list.currentleaf = &(* (cb_name*) (*leafptr).leaf);
	  *level = *level + 1; // level should return the last leafs openpairs count
	  //cb_clog( CBLOGDEBUG, "\ncb_set_to_leaf_inner_levels: TO LEFT." );
	  err = cb_set_to_leaf_inner_levels( &(*cbs), &(*name), namelen, (openpairs-1), &(*level), &(*mctl)); // 11.4.2014, 23.3.2014
	  if(err==CBSUCCESS || err==CBSUCCESSLEAVESEXIST){ // 3.7.2015
	    cb_clog( CBLOGDEBUG, "\ncb_set_to_leaf_inner_levels: returning CBSUCCESS, levels %i (2 - leaf).", *level);
	    return err;
	  }else
	    *level = *level - 1; // return to the previous level
// 4.10.2015: EHDOTUS: tahan kohtaan jos next==null, palauttaa *level ilman miinusta -> on jo lisatty lisayksena 28.9.2015

	}
	if( (*leafptr).next!=NULL ){ // Right (written after leaf if it is written)
	  (*(**cbs).cb).list.currentleaf = &(* (cb_name*) (*leafptr).next);
	  //cb_clog( CBLOGDEBUG, "\ncb_set_to_leaf_inner_levels: TO RIGHT." );
	  err = cb_set_to_leaf_inner_levels( &(*cbs), &(*name), namelen, openpairs, &(*level), &(*mctl)); // 11.4.2014, 23.3.2014
	  if(err==CBSUCCESS || err==CBSUCCESSLEAVESEXIST ){ // 3.7.2015
	    return err;
	  }
	}else if( (*leafptr).next==NULL && (*leafptr).leaf!=NULL && openpairs>=0 ){ // from this else starts addition made 28.9.2015 -- addition 28.9.2015, 2.10.2015
	  /*
	   * If the previous leaf compared previously was not null, add level by one (the compared leaf). */
	  *level = *level + 1; // 2.10.2015 explanations: -1, if next null, +1 if leaf was not null previously
	}
	// here ends an addition made 28.9.2015 -- /addition 28.9.2015

	// next was removed in version 22.9.2015: if null here, levels may be found but the currentleaf is null (needed in further tests)
	(*(**cbs).cb).list.currentleaf = NULL; // 30.9.2015 this is still here (and 8.10.2015)
        if( nextlevelempty==0 ){ // 3.7.2015
          return CBNOTFOUNDLEAVESEXIST; // 3.7.2015
	}
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

        //cb_clog( CBLOGDEBUG, "\ncb_set_to_last_leaf: " );

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
int  cb_put_leaf(CBFILE **str, cb_name **leaf, int openpairs, int previousopenpairs){

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

	//cb_log( &(*str), CBLOGDEBUG, "\ncb_put_leaf: openpairs=%i previousopenpairs=%i [", openpairs, previousopenpairs);
	//cb_print_ucs_chrbuf( CBLOGDEBUG, &(**leaf).namebuf, (**leaf).namelen, (**leaf).buflen );
	//cb_log( &(*str), CBLOGDEBUG, "]");

	cb_init_terminalcount( &(*str) );

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
        //++(*(**str).cb).list.nodecount;
	//if( newleaf!=NULL ) // 23.8.2015
	//  (*newleaf).serial = (*(**str).cb).list.nodecount; // 23.8.2015

	/*
	 * Update previous leafs length */
	//cb_log( &(*str), CBLOGDEBUG, "\ncb_put_leaf: Update previous leafs length: new leafs nameoffset %li openpairs %i previousopenpairs %i .", (**leaf).nameoffset, openpairs, previousopenpairs );
	err = cb_update_previous_length( &(*str), (**leaf).nameoffset, openpairs, previousopenpairs );
	if(err>=CBERROR){ cb_clog( CBLOGERR, "\ncb_put_name: cb_update_previous_length, error %i.", err );  return err; }

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

/*
 * Update previous names or leafs length just before setting a new name or leaf
 * as current or currentleaf.
 *
 * If openpairs is:
 *  increasing: can't update length
 *  decreasing: ok
 *  the same:   ok
 *
 * Name length is set if level is 0 (openpairs==0).
 *
 * To leave space to possible rewrites of names, keep name offsets in different
 * position than the length of the previous value. */
int  cb_update_previous_length(CBFILE **str, long int nameoffset, int openpairs, int previousopenpairs){
        if(str==NULL || *str==NULL || (**str).cb==NULL )
          return CBERRALLOC;
	// Previous names contents length
	//cb_log( &(*str), CBLOGDEBUG, "\ncb_update_previous_length: update previous names/leafs length: offset %li, openpairs %i, previousopenpairs %i.", (*(*(**str).cb).list.current).offset, openpairs, previousopenpairs );

        if( (*(**str).cb).list.last!=NULL && (*(**str).cb).list.namecount>=1 && \
			nameoffset>=( (*(*(**str).cb).list.last).offset + 2 ) && nameoffset<0x7FFFFFFF ){
	  /*
	   * Names
	   */
	  //cb_log( &(*str), CBLOGDEBUG, "\n previousopenpairs %i, openpairs %i", previousopenpairs, openpairs );
	  if( openpairs==0 ){ // to list (longest length)
	     // NAME
	     //cb_log( &(*str), CBLOGDEBUG, "\n1 names length to namelist" );
	     if( (*(**str).cb).list.last!=NULL ){
	       if( (*(**str).cb).list.last==NULL ) return CBERRALLOC; 
	         (*(*(**str).cb).list.last).length = nameoffset - (*(*(**str).cb).list.last).offset - 2; // last is allways a name? (2: '=' and '&')
	     }
	  }
	}
	if( (*(**str).cb).list.last!=NULL && (*(**str).cb).list.currentleaf!=NULL && \
			nameoffset>=(*(*(**str).cb).list.last).offset && nameoffset<0x7FFFFFFF ){ // Is in last name in list
	  /*
	   * Leaves
	   */
	  if( openpairs==previousopenpairs && ( openpairs>0 || previousopenpairs>0 ) ){ // from leaf to it's next leaf in list
	    // LEAF
	    //cb_log( &(*str), CBLOGDEBUG, "\n2 from leaf to leaf" );
	    if( (*(**str).cb).list.last==NULL ) return CBERRALLOC; 
	    if( (*(*(**str).cb).list.currentleaf).offset >= (*(*(**str).cb).list.last).offset ) // is lasts leaf
	      if( nameoffset >= ( (*(*(**str).cb).list.currentleaf).offset + 2 ) ) // leaf is after this leaf
	        (*(*(**str).cb).list.currentleaf).length = nameoffset - (*(*(**str).cb).list.currentleaf).offset - 2; // 2: '=' and '&'
	      // since read pointer is allways at read position, leaf can be added without knowing if one leaf is missing in between
          }else if( previousopenpairs>openpairs ){  // from leaf to it's name in list (second time, name previously) or from leaf to a lower leaf
	    // LEAF
            //cb_log( &(*str), CBLOGDEBUG, "\n3 from leaf downwards");
	    if( (*(**str).cb).list.currentleaf==NULL ) return CBERRALLOC; 
              (*(*(**str).cb).list.currentleaf).length = nameoffset - (*(*(**str).cb).list.currentleaf).offset - 2; // 2: '=' and '&'
	  }
	}
	return CBSUCCESS;
}

int  cb_put_name(CBFILE **str, cb_name **cbn, int openpairs, int previousopenpairs){ // ocoffset late, 12/2014
        int err=0; 

        if(cbn==NULL || *cbn==NULL || str==NULL || *str==NULL || (**str).cb==NULL )
          return CBERRALLOC;

	cb_init_terminalcount( &(*str) );

	//cb_log( &(*str), CBLOGDEBUG, "\ncb_put_name: openpairs=%i previousopenpairs=%i [", openpairs, previousopenpairs);
	//cb_print_ucs_chrbuf( CBLOGDEBUG, &(**cbn).namebuf, (**cbn).namelen, (**cbn).buflen );
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
	    err = cb_put_leaf( &(*str), &(*cbn), openpairs, previousopenpairs); // openpairs chooses between *next and *leaf
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
	  err = cb_update_previous_length( &(*str), (**cbn).nameoffset, openpairs, previousopenpairs );
	  if(err>=CBERROR){ cb_clog( CBLOGERR, "\ncb_put_name: cb_update_previous_length, error %i.", err );  return err; }

	  // Add name
          (*(**str).cb).list.last    = &(* (cb_name*) (*(*(**str).cb).list.last).next );

          err = cb_allocate_name( &(*(**str).cb).list.last, (**cbn).namelen ); if(err!=CBSUCCESS){ return err; } // 7.12.2013
          (*(*(**str).cb).list.current).next = &(*(*(**str).cb).list.last); // previous
          (*(**str).cb).list.current = &(*(*(**str).cb).list.last);
          (*(**str).cb).list.currentleaf = &(*(*(**str).cb).list.last);
          ++(*(**str).cb).list.namecount;
          //++(*(**str).cb).list.nodecount;
	  //if( (*(**str).cb).list.last!=NULL ) // 23.8.2015
	  //  (*(*(**str).cb).list.last).serial = (*(**str).cb).list.nodecount; // 23.8.2015
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
          //(*(**str).cb).list.nodecount=1;
	  //if( (*(**str).cb).list.last!=NULL ) // 23.8.2015
	  //  (*(*(**str).cb).list.last).serial = (*(**str).cb).list.nodecount; // 23.8.2015
        }
        err = cb_copy_name( &(*cbn), &(*(**str).cb).list.last); if(err!=CBSUCCESS){ return err; } // 7.12.2013
        (*(*(**str).cb).list.last).next = NULL;
        (*(*(**str).cb).list.last).leaf = NULL;
	(*(*(**str).cb).list.last).firsttimefound = (signed long int) time(NULL); // 1.12.2013
        return CBADDEDNAME;
}

/*
 * Returns the level of the current location where the cursor reads new characters
 * at the end of the stream.
 * */
int  cb_get_current_level_at_edge(CBFILE **cbs, int *level){ // addition 29.9.2015
	int currentlevel=0, err=CBSUCCESS;
	if( cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || level==NULL ) return CBERRALLOC;
	err = cb_get_current_level( &(*cbs),  &currentlevel);
	if(err>=CBERROR){ cb_clog( CBLOGERR, "\ncb_get_current_level_at_edge: cb_get_current_level, error %i.", err); }
	*level = currentlevel - (*(**cbs).cb).list.toterminal; 
	/*
	 * If something reads one rend or subrend away, the *level is incorrect.
	 *
	 * The value may not be read before the full name has been read. Otherwice
 	 * the level information is lost (unless rend or subrend '}' is inspected in
	 * the read function).
	 *
	 * Possibilities:
	 * a. If read was not from the library, flag is set. If flag is set, level is set to be a closed pair.
	 *    - Update of instructions: if value is read, it should be read complitely to last rend.
	 * b. Improvement of a. The last character is saved and if it is rend or subrend, toterminal is increased.
	 *    - Update of instructions: if value is read, it can be read until the first rend but not further.
	 * c. The state of reading, rstartflipflop, injsonarray, injsonquotes is saved in CBFILE and changed in
	 *    the cb_get_chr function. Toterminal is updated in cb_get_chr.
	 * d. The programmer must search the full next name with the tree in list before the value can be read as
	 *    is the case when starting. The tree is formed correctly if no value is read before the name in list 
	 *    is read.
	 * e. Like b. but the characters are folded, the last read character is saved in CBFILE if the new character
	 *    was not SP, tab, cr or lf or the previous was not a bypass (double delim). The toterminal is increased 
	 *    with one when library is reading it when the last character read was rend. 
	 * f. c. with unfolding as in e.
	 *
	 * Chosen method is f (2), cb_read structure is added to be used in cb_get_chr. For the mean time,
	 * b. is used (1). The variables and functions in c. can be cleaned to their own functions and the
	 * variables put into cb_read.
	 *
	 */

	//if( *level<0 )
	//  cb_clog( CBLOGDEBUG, "\ncb_get_current_level_at_edge: debug, error, level<0, *level %i (currentlevel %i, toterminal %i).", *level, currentlevel, (*(**cbs).cb).list.toterminal );

	return err;
}
/*
 * Returns the last read leafs level to be used
 * in reading the leaves of the names correctly. 
 *
 * Returns the level (0 if all are null).*/
int  cb_get_current_level(CBFILE **cbs, int *level){ // addition 28.9.2015
	int err = CBSUCCESS;
	cb_name *iter = NULL;
	if( level==NULL ) return CBERRALLOC; 
	*level = 0;
	if( cbs==NULL || *cbs==NULL || (**cbs).cb==NULL ) return CBERRALLOC;

	//iter = &(*(*(**cbs).cb).list.name); // 2.10.2015, VIRHE TASSA, LASKEMINEN PITAA ALOITTAA AINA VIIMEISESTA LISTAN ALKIOSTA
	iter = &(*(*(**cbs).cb).list.last); // 2.10.2015, VIRHE TASSA, LASKEMINEN PITAA ALOITTAA AINA VIIMEISESTA LISTAN ALKIOSTA
					    // TARKISTETTAVA VIELA ONKO MUITA KORJAUKSIA JOTKA LIITTYVAT TAHAN.

	if( iter!=NULL ){
	  *level = 1;
	}else{
	  return CBSUCCESS; // ..._set_leaf returns here, CBEMPTY
	}
	//if( (*iter).next!=NULL )
	//  cb_clog( CBLOGDEBUG, "\ncb_get_current_level: error, name after last was not null.");

	if( (*iter).leaf==NULL ){
	  return CBSUCCESS;
	}else{
	  *level = 2; // 6.10.2015, first leaf, from open pairs
	}
	iter = &(* (cb_name*) (*iter).leaf ); // XV , here, level is from closed pair
	err = cb_get_current_level_sub( &iter , &(*level) );
	return err;
}
int  cb_get_current_level_sub(cb_name **cn, int *level){ // addition 28.9.2015
	int err = CBSUCCESS;
	cb_name *iter = NULL;
	if( cn==NULL || *cn==NULL || level==NULL ) return CBERRALLOC;
	/*
	 * Search of last leaf and counting the level. 
	 *
	 * Towards right (faster, name is not compared). */
	if( (**cn).next!=NULL ){
		iter = &(* (cb_name*) (**cn).next );
		err = cb_get_current_level_sub( &iter, &(*level) );
	}else if( (**cn).leaf!=NULL ){ // XV, 3.10.2015, 6.10.2015
	        *level += 1;
		iter = &(* (cb_name*) (**cn).leaf );
		err = cb_get_current_level_sub( &iter, &(*level) );
	}else{ 
	 	; //cb_clog( CBLOGDEBUG, "\ncb_get_current_level_sub: next and leaf was null, level %i.", *level);
	}

	return CBSUCCESS;
}

/*
 * Returns 
 *   on success: CBSUCCESS, CBSUCCESSLEAVESEXIST
 *   on negation: CBEMPTY, CBNOTFOUND, CBNOTFOUNDLEAVESEXIST, CBNAMEOUTOFBUF (if not file or seekable file)
 *   on error: CBERRALLOC.
 */
int  cb_set_to_name(CBFILE **str, unsigned char **name, int namelen, cb_match *mctl){ // cb_match 11.3.2014, 23.3.2014
	cb_name *iter = NULL; int err=CBSUCCESS; char nextlevelempty=1;

        //cb_clog( CBLOGDEBUG, "\ncb_set_to_name: " );

	//cb_clog( CBLOGDEBUG, " comparing ["); 
	//if(name!=NULL)
	//	cb_print_ucs_chrbuf( CBLOGDEBUG, &(*name), namelen, namelen );
	//cb_clog( CBLOGDEBUG, "] ");

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
	  //cb_clog( CBLOGWARN, "\ncb_search_get_chr: null parameter was given, error CBERRALLOC.", CBERRALLOC);
	  return CBERRALLOC;
	}

	if( (**cbs).cf.unfold==1 && (*(**cbs).cb).contentlen==( (**cbs).ahd.currentindex+(**cbs).ahd.bytesahead ) ){
	  ; 
	}else if( (**cbs).cf.unfold==1 ){
	  /* 
	   * Not at the end of buffer.
	   * Reading of a value should stop at the rend. After rend, the readahead should be empty
	   * because LWSP characters are not read after the character. Emptying the read ahead buffer. */
	  err = cb_fifo_init_counters( &(**cbs).ahd );
	  if( err>=CBERROR ){ cb_clog( CBLOGERR, "\ncb_search_get_chr: cb_fifo_init_counters, error %i.", err ); }
	}
	if((**cbs).cf.unfold==1){
	  err = cb_get_chr_unfold( &(*cbs), &(**cbs).ahd, &(*chr), &(*chroffset) );
	  (*(**cbs).cb).list.rd.lastreadchrendedtovalue=0; // 7.10.2015, lastreadchrwasfromoutside=0; // 29.9.2015
	  return err; 
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
	  (*(**cbs).cb).list.rd.lastreadchrendedtovalue=0; // 7.10.2015, lastreadchrwasfromoutside=0; // 29.9.2015
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

/*
 * Number of closing flow control characters from the level of the last leaf.
 *
 * Every saved name or leaf zeros the terminaloffset. Every subrend and rend 
 * increases it.
 *
 * For example: 
 *        name },    the terminal offset is 2 or
 *
 *        name2 } } }, the terminal offset is 4
 *
 *       name3: {     the terminal offset is 0
 *
 */
int  cb_init_terminalcount(CBFILE **cbs){
	if( cbs==NULL || *cbs==NULL || (**cbs).cb==NULL) return CBERRALLOC;
	(*(**cbs).cb).list.toterminal=0;
	//cb_clog( CBLOGDEBUG, "\ncb_init_terminalcount: toterminal %i.", (*(**cbs).cb).list.toterminal );
	return CBSUCCESS;
}
int  cb_increase_terminalcount(CBFILE **cbs){
	if( cbs==NULL || *cbs==NULL || (**cbs).cb==NULL) return CBERRALLOC;
	++(*(**cbs).cb).list.toterminal;
	if((*(**cbs).cb).list.toterminal<0)
	  (*(**cbs).cb).list.toterminal=0;
	//cb_clog( CBLOGDEBUG, "\ncb_increase_terminalcount: toterminal %i.", (*(**cbs).cb).list.toterminal );
	return CBSUCCESS;
}

int  cb_set_cursor_match_length_ucs_matchctl(CBFILE **cbs, unsigned char **ucsname, int *namelength, int ocoffset, cb_match *mctl){ // 23.2.2014
	int  err=CBSUCCESS, cis=CBSUCCESS, ret=CBNOTFOUND;
	int  buferr=CBSUCCESS; 
	int  index=0;
	unsigned long int chr=0, chprev=CBRESULTEND; 
	long int chroffset=0;
	long int nameoffset=0; // 6.12.2014
	char tovalue = 0;
	char injsonquotes = 0, injsonarray = 0, wasjsonrend=0, wasjsonsubrend=0, jsonemptybracket=0;
	unsigned char charbuf[CBNAMEBUFLEN+1]; // array 30.8.2013
	unsigned char *charbufptr = NULL;
	cb_name *fname = NULL;
	char atvalue=0; 
	char rstartflipflop=0; // After subrstart '{', rend ';' has the same effect as concecutive rstart '=' and rend ';' (empty value).
			       // After rstart '=', subrstart '{' or subrend '}' has no effect until rend ';'.  (3.10.2015: and reader should read until rend)
			       // For example: name { name2=text2; name3 { name4=text4; name5=text5; } }
			       // (JSON can be replaced by setting rstart '"', rend '"' and by removing ':' from name)
        unsigned long int ch3prev=CBRESULTEND+2, ch2prev=CBRESULTEND+1;
	int openpairs=0, previousopenpairs=0; // cbstatetopology, cbstatetree (+ cb_name length information)
	int levels=0; // try to find the last read openpairs with this variable

	if( cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || mctl==NULL){
	  cb_log( &(*cbs), CBLOGALERT, "\ncb_set_cursor_match_length_ucs_matchctl: allocation error."); return CBERRALLOC;
	}
	chprev=(**cbs).cf.bypass+1; // 5.4.2013


/**
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
		cb_clog( CBLOGDEBUG, ", cbstatetree, [");
	else
		cb_clog( CBLOGDEBUG, ", not cbstatetree, [");
 **/
	//cb_clog( CBLOGDEBUG, "\ncb_set_cursor_match_length_ucs_matchctl: ocoffset %i, matchctl %i, NAME [", ocoffset, (*mctl).matchctl );
	//cb_print_ucs_chrbuf( CBLOGDEBUG, &(*ucsname), *namelength, CBNAMEBUFLEN );
	//cb_clog( CBLOGDEBUG, "].");

	//if(*ucsname==NULL || namelength==NULL ){ // 13.12.2014
	if( ucsname==NULL || *ucsname==NULL || namelength==NULL ){ // 7.7.2015
	  cb_log( &(*cbs), CBLOGALERT, "\ncb_set_cursor_match_length_ucs_matchctl: allocation error, ucsname or length was null.");
	  return CBERRALLOC;
	}

	/*
	 * Previous toterminal if previous read was from outside of the library
	 * (last rend or subrend).
	 */
	if( (**cbs).cf.searchstate==CBSTATETREE && (*(**cbs).cb).list.rd.lastreadchrendedtovalue==1 ){ // 7.10.2015 lastreadchrwasfromoutside==1 ){
		/*
	 	 * Last character from reads outside of the library. Most likely the
	         * last rend or subrend after reading the value.
		 *
		 * If reading is stopped one character after the bypass character,
		 * the tree is not formed correctly. The value should be read to the
		 * rend or subrend character (until the reading of rends and subrends
		 * is put to some of the get_chr functions and it's state is saved
		 * in cb_read or similar solution).
		 *
		 */
		//if( (*(**cbs).cb).list.rd.lastchr==(**cbs).cf.rend || (*(**cbs).cb).list.rd.lastchr==(**cbs).cf.subrend ){
		if( (*(**cbs).cb).list.rd.lastchr==(**cbs).cf.rend || ( (*(**cbs).cb).list.rd.lastchr==(**cbs).cf.subrend && \
			( (**cbs).cf.doubledelim==1 || (**cbs).cf.json==1 ) ) ){ // 8.10.2015 (this can be done in any case and this line was not needed)
		  previousopenpairs=CBMAXLEAVES; // 6.10.2015, from up to down
		  cb_increase_terminalcount( &(*cbs) );
		  if( (*(**cbs).cb).list.rd.lastchr==(**cbs).cf.rend ){ // wasjsonsubrend is needed here because of '}' ','
		    wasjsonrend=1; wasjsonsubrend=0; rstartflipflop=0;
		  }else if( (*(**cbs).cb).list.rd.lastchr==(**cbs).cf.subrend ){
		    wasjsonrend=0; wasjsonsubrend=1;
		  }
		}
		(*(**cbs).cb).list.rd.lastreadchrendedtovalue=0; // 7.10.2015 lastreadchrwasfromoutside=0; // zero the flag after addition (not more additions are needed)
	}

	if( (**cbs).cf.searchstate==CBSTATETREE || (**cbs).cf.searchstate==CBSTATETOPOLOGY ){ // Search next matching leaf (assuming the ocoffset was remembered correctly from the previous search)
	  openpairs = ocoffset; // 12.12.2013
	}

	// 1) Search list (or tree leaves) and set cbs.cb.list.index if name is found
	if( ocoffset==0 ){ // 12.12.2013
	  err = cb_set_to_name( &(*cbs), &(*ucsname), *namelength, &(*mctl) ); // 11.3.2014, 23.3.2014
	  if(err>=CBNEGATION){
	    if( cb_get_current_level_at_edge( &(*cbs), &levels ) >= CBERROR ){ // count the levels (separately from other functions)
	      cb_clog( CBLOGERR, "\ncb_set_cursor_match_length_ucs_matchctl: cb_get_current_level, allocation error. ");
	      return CBERRALLOC;
	    }else{
	      openpairs = levels;
	    }
	  }
	}else{
	  err = cb_set_to_leaf( &(*cbs), &(*ucsname), *namelength, ocoffset, &levels, &(*mctl) ); // mctl 11.3.2014, 23.3.2014
	  if( levels<0 ){
	    levels = 0; // 30.9.2015 temporary until current is set to null and currentleaf after coming back to the namelist
	    openpairs = levels; // 30.9.2015
	  }
	  openpairs = levels;
	  if( (*(**cbs).cb).list.current!=NULL && (*(**cbs).cb).list.last!=NULL ){
	    if( (*(*(**cbs).cb).list.current).offset != (*(*(**cbs).cb).list.last).offset && openpairs>0 && (**cbs).cf.findleaffromallnames!=1 ){ // 3.10.2015
	      /*
	       * Searched leaf inside value of a name and the name is not last in the list.
	       * Returning the search results if the value was read (openpairs==0). */
	      ret = err;
	      goto cb_set_cursor_ucs_return;
	    }
	  }
	}

        // 1.10.2015
        // VIII still too many set_length -functions. Excess has been marked with text "28.9.2015". Part of them can be removed.
        //      (7.10.2015: not fixed, 8.10.2015: not fixed)


	if(previousopenpairs==CBMAXLEAVES) // 7.10.2015, from up to down (toterminal was increased since the last read)
	  previousopenpairs=openpairs+1;
	else // unknown, cursor id between = and & 
	  previousopenpairs=openpairs; // 7.10.2015
	
	//cb_clog( CBLOGDEBUG, "\ncb_set_cursor_match_length_ucs_matchctl: openpairs at start %i (levels %i, ocoffset %i).", openpairs, levels, ocoffset);

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
	injsonarray=0;

	// Search for new name
	// ...& - ignore and set to start
	// ...= - save any new name and compare it to 'name', if match, return

	err = cb_search_get_chr( &(*cbs), &chr, &chroffset); // 1.9.2013

	//cb_clog( CBLOGDEBUG, "|%c,0x%X|", (unsigned char) chr, (unsigned char) chr );
	//cb_clog( CBLOGDEBUG, "|%c|", (unsigned char) chr ); // BEST
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

	while( err<CBERROR && err!=CBSTREAMEND && index < (CBNAMEBUFLEN-3) && buferr <= CBSUCCESS){ 

	  if( (**cbs).encoding==CBENCAUTO )
	  	cb_automatic_encoding_detection( &(*cbs) ); // set encoding automatically if it's set

	  tovalue=0;


	  /*
	   * Read until rstart '='
	   */
	  if( (**cbs).cf.searchstate==CBSTATETOPOLOGY || (**cbs).cf.searchstate==CBSTATETREE ){ // '=', save name

	    /*
	     * JSON arrays (a comma between an array may disturb reading of the values)
	     * similarly as quotes. 
	     */
	    if( (**cbs).cf.json==1 ){
	      if( chprev!=(**cbs).cf.bypass && chr==(unsigned long int)'[' && injsonquotes!=1 )
		injsonarray++;
	      if( chprev!=(**cbs).cf.bypass && chr==(unsigned long int)']' && injsonquotes!=1 )
		injsonarray--;
	      if( injsonarray<0 )
		injsonarray=0;

	      /*
	       * JSON quotes (is never reset by anything else). 
	       */
	      if( chprev!=(**cbs).cf.bypass && chr==(unsigned long int)'\"' ){
	        if( injsonquotes==0 ) // first quote
	          ++injsonquotes;
	        else if( injsonquotes==1 ) // second quote
	          injsonquotes=2; // ready to save the name (or after value)
	      }
	      if( injsonquotes==2 && chr!=(unsigned long int)'\"' ){
	        injsonquotes=3; // do not save characters after second '"' to name, 27.8.2015
	      }
	    }

	    // Second is JSON comma problem ("na:me") prevention in name 21.2.2015
	    if(       (    ( chr==(**cbs).cf.rstart && (**cbs).cf.json!=1 ) \
	                || ( chr==(**cbs).cf.rstart && (**cbs).cf.json==1 && injsonquotes!=1 && injsonarray<=0 ) \
	                || ( chr==(**cbs).cf.subrstart && rstartflipflop!=1 && (**cbs).cf.doubledelim==1 && (**cbs).cf.json!=1 ) \
	                || ( chr==(**cbs).cf.subrstart && (**cbs).cf.json==1 && injsonquotes!=1 && injsonarray<=0 ) \
	              ) && chprev!=(**cbs).cf.bypass ){ // count of rstarts can be greater than the count of rends
	      wasjsonrend=0;
	      wasjsonsubrend=0;

	      if( chr==(**cbs).cf.rstart){ // 17.8.2015
		rstartflipflop=1;
	        if( (**cbs).cf.json==1 )
		  jsonemptybracket=0; // 3.10.2015, from '{' to '}' without a name or '{' '{' '}' '}' without a name
	      }

	      if( chr==(**cbs).cf.subrstart && (**cbs).cf.json==1 ) { // 21.2.2015 JSON
		injsonquotes=0;
		injsonarray=0;
		++jsonemptybracket; // 3.10.2015, from '{' to '}' without a name or '{' '{' '}' '}' without a name
	        tovalue=0;
	        goto cb_set_cursor_reset_name_index; // update openpairs only from ':' (JSON rstart is ':')
	      } // /21.2.2015
	      if(openpairs>0){ // leaf, 13.12.2013
	        previousopenpairs=openpairs;
	        ++openpairs;
	        tovalue=0;
	      }else if( openpairs==0 ){ // name
	        previousopenpairs=openpairs;
                ++openpairs;
	        tovalue=1;
	      }
	      if( (**cbs).cf.searchstate==CBSTATETREE ){ // saves leaves
	        tovalue=1;
	      }
	      //cb_clog( CBLOGDEBUG, "\nopenpairs=%i", openpairs );
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
	    //if( charbufptr==NULL )
	    //  cb_clog( CBLOGDEBUG, "\n charbufptr is null." );

	    if( buferr==CBSUCCESS ){ 

	      buferr = cb_save_name_from_charbuf( &(*cbs), &fname, chroffset, &charbufptr, index, nameoffset); // 7.12.2013, 6.12.2014
	      if(buferr==CBNAMEOUTOFBUF || buferr>=CBNEGATION){ cb_log( &(*cbs), CBLOGNOTICE, "\ncb_set_cursor_ucs: cb_save_name_from_ucs returned %i ", buferr); }

	      //cb_clog( CBLOGDEBUG, "\ncb_set_cursor_match_length_ucs_matchctl: cb_save_name_from_charbuf returned %i .", buferr );

	      if( fname==NULL )
	        cb_clog( CBLOGDEBUG, "\n fname is null." );

	      if(buferr!=CBNAMEOUTOFBUF && fname!=NULL ){ // cb_save_name_from_charbuf returns CBNAMEOUTOFBUF if buffer is full

		//cb_clog( CBLOGDEBUG, "\n Openpairs=%i, ocoffset=%i, injsonquotes=%i, injsonarray=%i,index=%li, name [", openpairs, ocoffset, injsonquotes, injsonarray, (*(**cbs).cb).index );
		//cb_print_ucs_chrbuf( CBLOGDEBUG, &(*fname).namebuf, (*fname).namelen, (*fname).namelen);
		//cb_clog( CBLOGDEBUG, "] ");
		//if( (**cbs).cf.json==1 )
		//  cb_clog( CBLOGDEBUG, ", going to cb_check_json_name.");

		if( (**cbs).cf.json!=1 || ( (**cbs).cf.json==1 && ( ( injsonquotes>=2 && (**cbs).cf.jsonnamecheck!=1 ) || \
				( injsonquotes>=2 && (**cbs).cf.jsonnamecheck==1 && \
				cb_check_json_name( &charbufptr, &index )!=CBNOTJSON ) ) ) ){ // 19.2.2015, check json name with quotes 27.8.2015
				// cb_check_json_name( &(*fname).namebuf, &(*fname).namelen )!=CBNOTJSON ) ) ) ){ // 19.2.2015
	          //buferr = cb_put_name(&(*cbs), &fname, openpairs, ocoffset); // (last in list), if name is comparable, saves name and offset
	          buferr = cb_put_name(&(*cbs), &fname, openpairs, previousopenpairs); // (last in list), if name is comparable, saves name and offset

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
	      cb_log( &(*cbs), CBLOGNOTICE, "\ncb_set_cursor_ucs: cb_put_name returned %i. ", buferr);
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
	                           ( chr==(**cbs).cf.rend && (**cbs).cf.json==1 && injsonquotes!=1 && injsonarray<=0 ) || \
	                           ( (**cbs).cf.json!=1 && chr==(**cbs).cf.subrend && rstartflipflop!=1 && (**cbs).cf.doubledelim==1 ) || \
	                           ( (**cbs).cf.json==1 && injsonquotes!=1 && injsonarray<=0 && chr==(**cbs).cf.subrend ) ) ){

	      if( (**cbs).cf.json!=1 || ( ( jsonemptybracket<=0 && chr==(**cbs).cf.subrend ) || chr==(**cbs).cf.rend ) ) // '}' ',' json-condition added 3.10.2015, kolmas
	        cb_increase_terminalcount( &(*cbs) );

	      if( chr==(**cbs).cf.rend ){
		rstartflipflop=0;
		if( (**cbs).cf.json==1 )
	          jsonemptybracket=0;
	      }else if( (**cbs).cf.json==1 ){
	        --jsonemptybracket;
	        if(jsonemptybracket<0)
		   jsonemptybracket=0; // 3.10.2015, last
	      }

	      /*
	       * '&', start a new name, 27.2.2015: solution to '}' + ',' -> --openpairs once
	       */
	      if( ( (**cbs).cf.searchstate==CBSTATETOPOLOGY || (**cbs).cf.searchstate==CBSTATETREE ) && openpairs>=1){
	        if( (**cbs).cf.json==1 && chr==(**cbs).cf.subrend && openpairs>0 ){ // JSON
	          // 1. (JSON rend is ',' subrend is '}' ) . If not ',' as should, remove last comma in list: a, b, c } 21.2.2015
		  // 2. If '}' after '}', the same.
		  wasjsonsubrend=1;
		  if( wasjsonrend==0 && injsonarray<=0 ){ // array: [ <value>[<,><value> [...] ] ], no curly braces - injsonarray is not necessary here
			previousopenpairs = openpairs;
			--openpairs;
			cb_update_previous_length( &(*cbs), chroffset, openpairs, previousopenpairs); // 28.9.2015
			// subrend was in place of rend, special condition:
			// 1. reduce with '}' '}' but not with ',' in ',' '}' '}'
			// 3. reduce with '}' '}' but not with ',' in '}' '}' ',' ; 3.10.2015: in namelist, not in leaves
			//wasjsonsubrend=0; // was as if ',' , not '}'
		  }
		  wasjsonrend=0;
	          injsonquotes=0;
	        }else{ 

		  if( (**cbs).cf.json==1 && wasjsonrend==0 && injsonarray<=0 )  // injsonarray is needed here
		    wasjsonrend=1; 
		  // 3. If previous '}' reduced the openpairs count, do not reduce again with ','
		  if( (**cbs).cf.json!=1 || ( injsonarray<=0 && (**cbs).cf.json==1 ) ){	// 28.9.2015 1
		  	previousopenpairs = openpairs;
	          	--openpairs; // reader must read similarly, with openpairs count or otherwice
			cb_update_previous_length( &(*cbs), chroffset, openpairs, previousopenpairs); // 28.9.2015
		  }
                  wasjsonsubrend=0;
		}
	        //cb_clog( CBLOGDEBUG, "\nopenpairs=%i", openpairs );
	      }
	      atvalue=0;
	      cb_free_name(&fname);
	      fname=NULL; // allocate_name is at put_leaf and put_name and at cb_save_name_from_charbuf (first: more accurate name, second: shorter length in memory)
// JSON: XX
// conf:issa on sama seuraavasti: (**cbs).cf.json==1 && (openpairs<0 || openpairs<(ocoffset-1) 
	      if( ocoffset>0 && (**cbs).cf.json==1 && injsonquotes!=1 && openpairs<previousopenpairs && openpairs==0 ){ // TEST, 23.8.2015, 3.10.2015, toinen yritys. 8.10.2015
		/*
		 * JSON special: concecutive '}' and ',' */
		//cb_log( &(*cbs), CBLOGDEBUG, "\nocoffset %i, openpairs<previousopenpairs && openpairs==%i, %i<%i. Returning VALUEEND.", ocoffset, openpairs, openpairs, previousopenpairs ); // Test
		//cb_clog( CBLOGDEBUG, " PREVCHR %c.", (char) (*(**cbs).cb).list.rd.lastchr );
	        injsonquotes=0;
		/*
		 * In test 22.8.2015, update length here and second time at cb_put_name or 
	 	 * cb_put_leaf. Second time the value is updated to the final value (with writable 
		 * block sizes). Writable block sizes are not done well yet here (at 22.8.2015). */
		cb_update_previous_length( &(*cbs), chroffset, openpairs, previousopenpairs); // 22.8.2015
		/*
		 * These have to be set to null to know the right level the next time, 1.10.2015. */
		(*(**cbs).cb).list.currentleaf=NULL; // 1.10.2015, VI
		if( (**cbs).cf.findleaffromallnames!=1 ){
	          ret = CBVALUEEND; // 22.8.2015
	          //cb_clog( CBLOGDEBUG, "\ncb_set_cursor_...: set currentleaf to null, openpairs=%i", openpairs );
	          goto cb_set_cursor_ucs_return;
		}
	      }
 	      goto cb_set_cursor_reset_name_index;
	  }else if(chprev==(**cbs).cf.bypass && chr==(**cbs).cf.bypass){
	      /*
	       * change \\ to one '\' 
	       */

// pre TEST 7.10.2015			  ( (**cbs).cf.json==0 && (openpairs<0 || openpairs<ocoffset) ) && 
// pre TEST 8.10.2015			  ( (**cbs).cf.json==0 && (openpairs<0 || openpairs<ocoffset) ) && 
// pre TEST 8.10.2015	 	          ( (**cbs).cf.json==1 && (openpairs<0 || openpairs<(ocoffset-1) ) ) && 

	      chr=(**cbs).cf.bypass+1; // any char not '\'
	  }else if(     ( \
			  ( (**cbs).cf.json==0 && (openpairs<0 || openpairs<ocoffset ) && openpairs<previousopenpairs ) && \
		          ( (**cbs).cf.searchstate==CBSTATETREE || (**cbs).cf.searchstate==CBSTATETOPOLOGY ) \
		        ) || ( \
		          ( (**cbs).cf.json==1 && (openpairs<0 || openpairs<(ocoffset-1) ) && openpairs<previousopenpairs ) && \
		          ( (**cbs).cf.searchstate==CBSTATETREE || (**cbs).cf.searchstate==CBSTATETOPOLOGY) \
			) ){ // 27.2.2015: test JSON: TEST WAS: FAIL ( '}'+',' should be -1, not -2 ) , STILL HERE, AN ERROR

// CONF: XX

// 8.10.2015: added: openpairs<previousopenpairs , openpairs  must go up until it can go down.

	      /*
	       * When reading inner name-value -pairs, read of outer value ended.
	       */
              //cb_log( &(*cbs), CBLOGDEBUG, "\nValue read, openpairs %i ocoffset %i.", openpairs, ocoffset); // debug
	      /*
	       * In test 22.8.2015, update length here and second time at cb_put_name or 
	       * cb_put_leaf. Second time the value is updated to the final value (with writable 
	       * block sizes in mind). Writable block sizes are not done well yet here (at 22.8.2015). */
	      cb_update_previous_length( &(*cbs), chroffset, openpairs, previousopenpairs); // 22.8.2015
	      if( ocoffset>0 && (**cbs).cf.findleaffromallnames!=1 ){ // if conditions added 3.10.2015 (search was a leaf search and search is not from all names)
	        ret = CBVALUEEND; // 22.8.2015
	        // testattu, ei toimi oikein, toterminal > level: (*(**cbs).cb).list.rd.lastreadchrendedtovalue=1; // 7.10.2015, kohta XX (VIELA TESTATTAVANA KLO 11:50)
	        //cb_clog( CBLOGDEBUG, "\nPLACE, json=%i, PREVCHR %c.", (**cbs).cf.json, (char) (*(**cbs).cb).list.rd.lastchr );
	        goto cb_set_cursor_ucs_return;
 	        // return CBVALUEEND; // 12.12.2013
	      }
	  }else if( ( (**cbs).cf.searchstate==CBSTATEFUL || (**cbs).cf.searchstate==CBSTATETOPOLOGY ) && atvalue==1){ // Do not save data between '=' and '&' 
	      /*
	       * Indefinite length values. Index does not increase and unordered
	       * values length is not bound to CBNAMEBUFLEN. (index boundary check?)
	       */
	      ;
	  }else if((**cbs).cf.searchstate==CBSTATETREE ){ // save character to buffer, CBSTATETREE
	      // Next json condition has no effect, 11.7.2015
	      if( (**cbs).cf.json!=1 || ( (**cbs).cf.json==1 && ( injsonquotes==1 || injsonquotes==2 ) ) ){ // 11.7.2015, if json, write name only if inside quotes (json name is always "string")
	        buferr = cb_put_ucs_chr(chr, &charbufptr, &index, CBNAMEBUFLEN);
	      }
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

	  //cb_clog( CBLOGDEBUG, "%c|", (unsigned char) chr ); // BEST
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
	//cb_clog( CBLOGDEBUG, "\ncb_set_cursor_match_length_ucs_matchctl: returning %i.", ret);
	return ret;

        return CBNOTFOUND;
}

/*
 * Allocates fname */
int cb_save_name_from_charbuf(CBFILE **cbs, cb_name **fname, long int offset, unsigned char **charbuf, int index, long int nameoffset){
	unsigned long int cmp=0x61;
	int buferr=CBSUCCESS, err=CBSUCCESS;
	int indx = 0;
	char atname=0, injsonquotes=0;

	if( cbs==NULL || *cbs==NULL || fname==NULL || charbuf==NULL || *charbuf==NULL ){
	  //cb_clog( CBLOGDEBUG, "\ncb_save_name_from_charbuf: parameter was NULL, CBERRALLOC.");
	  return CBERRALLOC;
	}

	err = cb_allocate_name( &(*fname), (index+1) ); // moved here 7.12.2013 ( +1 is one over the needed size )
	if(err!=CBSUCCESS){  return err; } // 30.8.2013

	(**fname).namelen = index;
	(**fname).offset = offset;
	(**fname).nameoffset = nameoffset; // 6.12.2014

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
	          if( cmp == (unsigned long int) 0x22 && injsonquotes==1 )
	            ++injsonquotes;
	          if( (**cbs).cf.json!=1 || ( (**cbs).cf.json==1 && injsonquotes==1 ) ){ // 27.8.2015, do not save quotes in json name "name" -> name (after checking the name format)
	            if( ! ( indx==index && (**cbs).cf.removewsp==1 && WSP( cmp ) ) ){ // special last white space, 13.12.2013
                      buferr = cb_put_ucs_chr( cmp, &(**fname).namebuf, &(**fname).namelen, (**fname).buflen );
	              atname=1;
	            }
		  }else if( cmp == (unsigned long int) 0x22 ){ // '"'
	            ++injsonquotes; if(injsonquotes<0) injsonquotes=3;
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
	if( chprev!= (unsigned long int) '\\' && chr == (unsigned long int) '"' ){ // json bypass character 
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
