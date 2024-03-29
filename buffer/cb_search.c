/*
 * Library to read and write streams. Searches valuepairs locations to a list. Uses different character encodings.
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
#include <unistd.h>
#include <string.h>  // memset
#include <time.h>    // time (timestamp in cb_set_cursor)
#include <stdbool.h> // C99 true/false
#include "../include/cb_buffer.h"
#include "../include/cb_json.h"


static signed int  cb_put_name(CBFILE **str, cb_name **cbn, signed int openpairs, signed int previousopenpairs);
static signed int  cb_put_leaf(CBFILE **str, cb_name **leaf, signed int openpairs, signed int previousopenpairs);
static signed int  cb_is_rstart(CBFILE **cbs, unsigned long int chr);
static signed int  cb_is_rend(CBFILE **cbs, unsigned long int chr, unsigned long int *matched_rend );
static signed int  cb_update_previous_length(CBFILE **str, signed long int nameoffset, signed int openpairs, signed int previousopenpairs); // 21.8.2015
static signed int  cb_set_to_last_leaf(cb_name **tree, cb_name **lastleaf, signed int *openpairs); // sets openpairs, not yet tested 7.12.2013
static signed int  cb_set_to_leaf(CBFILE **cbs, unsigned char **name, signed int namelen, signed int openpairs, signed int *level, cb_match *mctl); // 11.3.2014, 23.3.2014
static signed int  cb_set_to_leaf_inner(CBFILE **cbs, unsigned char **name, signed int namelen, signed int openpairs, signed int *level, cb_match *mctl); // 16.3.2014, 23.3.2014
static signed int  cb_set_to_leaf_inner_levels(CBFILE **cbs, unsigned char **name, signed int namelen, signed int openpairs, signed int *level, cb_match *mctl);
static signed int  cb_set_to_name(CBFILE **str, unsigned char **name, signed int namelen, cb_match *mctl); // 11.3.2014, 23.3.2014
static signed int  cb_search_get_chr( CBFILE **cbs, unsigned long int *chr, signed long int *chroffset);
static signed int  cb_save_name_from_charbuf(CBFILE **cbs, cb_name **fname, signed long int offset, unsigned char **charbuf, signed int index, signed long int nameoffset, unsigned long int matched_rend);
static signed int  cb_automatic_encoding_detection(CBFILE **cbs);

static signed int  cb_get_current_level_at_edge(CBFILE **cbs, signed int *level); // addition 29.9.2015
static signed int  cb_get_current_level(CBFILE **cbs, signed int *level); // to be used with cb_set_to_name -- addition 28.9.2015
static signed int  cb_get_current_level_sub(cb_name **cn, signed int *level); // addition 28.9.2015

static signed int  cb_init_terminalcount(CBFILE **cbs); // 29.9.2015, count of downwards flow control characters read at the at the last edge of the stream from the last leaf ( '}' '}' '}' = 3 from the last leaf) 
static signed int  cb_increase_terminalcount(CBFILE **cbs); // 29.9.2015

static signed int  cb_check_json_name( unsigned char **ucsname, signed int *namelength, char bypassremoved ); // 8.11.2016

//static signed int  cb_test_messageend( CBFILE *cbf );

signed int  cb_test_messageend( CBFILE *cbf ){ // 20.7.2021 to test if the message was already read
	if( cbf==NULL ) return CBERRALLOC;
	if( (*(*cbf).cb).messageoffset==-1 ) return CBNEGATION;
        if( (*cbf).cf.stopatmessageend==1 && \
	    (*(*cbf).cb).messageoffset>=0 && \
	    (*(*cbf).cb).contentlen >= (*(*cbf).cb).messageoffset ){
		cb_clog( CBLOGDEBUG, CBNEGATION, "\nCB:  The message offset length was already read (index %li, contentlen %li).", (*(*cbf).cb).index, (*(*cbf).cb).contentlen );
		cb_flush_log();
		return CBSUCCESS;
        }
	return CBNEGATION;
}


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
int  cb_set_to_leaf(CBFILE **cbs, unsigned char **name, signed int namelen, signed int openpairs, signed int *level, cb_match *mctl){ // 23.3.2014
	signed int err = CBSUCCESS;
	cb_name  copyname;
	cb_name *copyptr = NULL;
	//7.5.2017: if(cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || name==NULL || mctl==NULL){
	if(cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || name==NULL || *name==NULL || mctl==NULL || level==NULL ){ // 7.5.2017, 8.5.2017
	  cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_set_to_leaf: allocation error."); return CBERRALLOC;
	}
	if( (*(**cbs).cb).list.rd.current_root!=NULL && (*(*(**cbs).cb).list.rd.current_root ).leaf!=NULL ){ // Do not search to the right
	  if( (**cbs).cf.searchrightfromroot==1 ){
		/*
		 * Search to the right from root. */
	  	(*(**cbs).cb).list.currentleaf = &(* (*(**cbs).cb).list.rd.current_root ); // previous before 21.2.2017
	  	*level = (*(**cbs).cb).list.rd.current_root_level; // previous before 21.2.2017
	  }else{
		/*
		 * Search only the leaves of root. */
	  	(*(**cbs).cb).list.currentleaf = &(*(* (*(**cbs).cb).list.rd.current_root ).leaf);
	  	//*level = ( (*(**cbs).cb).list.rd.current_root_level + 1 );
	  	*level = (*(**cbs).cb).list.rd.current_root_level ; // to be checked again, 21.2.2017
	  	openpairs = 1;
	  }
	}else if( (*(**cbs).cb).list.rd.current_root!=NULL ){ // previous before 21.2.2017
	  if( (**cbs).cf.searchrightfromroot==1 ){
		/*
		 * Search to the right from root. */
	  	(*(**cbs).cb).list.currentleaf = &(* (*(**cbs).cb).list.rd.current_root ); // previous before 21.2.2017
	  	*level = (*(**cbs).cb).list.rd.current_root_level; // previous before 21.2.2017
	  }else{
		/*
		 * Search only to the left of root. 
		 * Pass a copy of current_root with next==NULL to compare only itself (reflexivity). 21.2.2017 */
		copyptr = &copyname;
		cb_init_name( &copyptr );
		if( cb_copy_name( &(*(**cbs).cb).list.rd.current_root, &copyptr ) < CBNEGATION ){
			//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_to_leaf: COMPARING CURRENT_ROOT ONLY" );
			(*copyptr).next = NULL; // Compare the current_root only and return, do not search to the right
		  	(*(**cbs).cb).list.currentleaf = &(*copyptr);
		}else{
			*level = 0;
			copyptr = NULL;
			return CBEMPTY;	
		}
	  }
	}else if( (*(**cbs).cb).list.current==NULL ){
	  *level = 0;
	  return CBEMPTY;
	}else{
	  *level = 1; // current was not null, at least a name is found at level one
	  if( (* (*(**cbs).cb).list.current ).leaf != NULL ){
	    	*level = 2;
	    	(*(**cbs).cb).list.currentleaf = &(* (cb_name*) (*(*(**cbs).cb).list.current).leaf );
	  }
	}
// 7.4.2017 NULL CHECK MISSING
	err = cb_set_to_leaf_inner( &(*cbs), &(*name), namelen, openpairs, &(*level), &(*mctl)); // 11.3.2014
	if( copyptr!=NULL ){
		/*
		 * Do not leave the copy to the list or tree. */
	 	if( (*(**cbs).cb).list.rd.current_root!=NULL )
		  (*(**cbs).cb).list.currentleaf = &(* (*(**cbs).cb).list.rd.current_root );
		else
		  (*(**cbs).cb).list.currentleaf = NULL;
	}
	copyptr = NULL;
	return err;
}
int  cb_restore_root( CBFILE **cbs ){
	if( cbs==NULL || *cbs==NULL || (**cbs).cb==NULL ) return CBERRALLOC;
	if( (*(**cbs).cb).list.rd.previous_root != NULL ){
		(*(**cbs).cb).list.rd.current_root = &(* (*(**cbs).cb).list.rd.previous_root );
		(*(**cbs).cb).list.rd.current_root_level = (*(**cbs).cb).list.rd.previous_root_level;
	}else{
		(*(**cbs).cb).list.rd.current_root = NULL;
		(*(**cbs).cb).list.rd.current_root_level = -1;
	}
	return CBSUCCESS;
}
void cb_print_current_root( CBFILE **cbs, int priority){
	cb_clog( priority, CBSUCCESS, "Current root '" );
	if( (*(**cbs).cb).list.rd.current_root==NULL ){
		cb_clog( priority, CBSUCCESS, "null'" );
	}else{
		cb_print_name( &(*(**cbs).cb).list.rd.current_root, priority );
		cb_clog( priority, CBSUCCESS, "' level %i", (*(**cbs).cb).list.rd.current_root_level  );
	}
}
int  cb_set_root( CBFILE **cbs ){
	if( cbs==NULL || *cbs==NULL || (**cbs).cb==NULL ) return CBERRALLOC;
	// 4.3.2023
	if( (*(**cbs).cb).list.rd.current_root != NULL ){
		(*(**cbs).cb).list.rd.previous_root = &(* (*(**cbs).cb).list.rd.current_root );
		(*(**cbs).cb).list.rd.previous_root_level = (*(**cbs).cb).list.rd.current_root_level;
	}else{
		(*(**cbs).cb).list.rd.previous_root = NULL;
		(*(**cbs).cb).list.rd.previous_root_level = -1;
	}
	// /4.3.2023
	if( (*(**cbs).cb).list.rd.last_name!=NULL ) 
		(*(**cbs).cb).list.rd.current_root = &(* (*(**cbs).cb).list.rd.last_name );
	else
		(*(**cbs).cb).list.rd.current_root = NULL; // 8.5.2017
	if( (*(**cbs).cb).list.rd.last_level>=0) (*(**cbs).cb).list.rd.current_root_level = (*(**cbs).cb).list.rd.last_level;
	//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_root: setting current_root_level %i, current_root [", (*(**cbs).cb).list.rd.current_root_level );
	//if( (*(**cbs).cb).list.rd.current_root!=NULL && (*(*(**cbs).cb).list.rd.current_root).namebuf!=NULL )
	//cb_print_ucs_chrbuf( CBLOGDEBUG, &(* (*(**cbs).cb).list.rd.current_root ).namebuf, (*(*(**cbs).cb).list.rd.current_root).namelen, (*(*(**cbs).cb).list.rd.current_root).buflen );
	//cb_clog( CBLOGDEBUG, CBNEGATION, "]" );
	return CBSUCCESS;
}
int  cb_release_root( CBFILE **cbs ){
	if( cbs==NULL || *cbs==NULL || (**cbs).cb==NULL ) return CBERRALLOC;
	(*(**cbs).cb).list.rd.current_root = NULL;
	(*(**cbs).cb).list.rd.current_root_level = -1;
	return CBSUCCESS;
}
int  cb_set_to_leaf_inner(CBFILE **cbs, unsigned char **name, signed int namelen, signed int openpairs, signed int *level, cb_match *mctl){ // 11.4.2014, 23.3.2014
	signed int err=CBSUCCESS, currentlevel=0; 
	if(cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || name==NULL || mctl==NULL){
	  cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_set_to_leaf_inner: allocation error."); return CBERRALLOC;
	}

	currentlevel = *level;

	err = cb_set_to_leaf_inner_levels( &(*cbs), &(*name), namelen, openpairs, &currentlevel, &(*mctl) );
	if(err>=CBERROR){ cb_clog( CBLOGERR, err, "\ncb_set_to_leaf_inner: cb_set_to_leaf_inner_levels, error %i.", err ); }

	//if( currentlevel>=(*level) )
	//	cb_clog( CBLOGDEBUG, CBNEGATION, "\nSET_TO_LEAF_INNER: CURRENTLEVEL %i, LEVEL %i (start, err %i).", currentlevel, *level, err );

	/*
	 * 1.1.2017. */
	if( err<CBNEGATION ){
		/*
		 * Match. */
		(*(**cbs).cb).list.rd.last_name  = &(* (*(**cbs).cb).list.currentleaf );
		(*(**cbs).cb).list.rd.last_level = *level;

		return err; // pre 1.1.2017: level is correct if CBSUCCESS or CBSUCCESSLEAVESEXIST
	}

	/*
	 * currentlevel is counted from the last level and it's open pair. Also the toterminal has to
	 * include the first open pair - if *level is more than 0: toterminal+1 . */
        //if( currentlevel>1 )
        //  --currentlevel;	
	// 1.1.2017: toterminal has nothing to do with this function (used in determining the level of the last added leaf)
	*level = currentlevel - (*(**cbs).cb).list.toterminal;

	/*
	 * Real bug here. The reader should keep track of the open pairs.
	 * If the reading has stopped to a leaf of the tree, the levels is not necessarily
	 * correct anymore when hitting the zero level or the the original list (or hitting the ground).
	 * Why was this again? 17.3.2016.
	 * (Uncomment the error log and test.)
	 */
	if(*level<0){ // to be sure errors do not occur
	  //cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_to_leaf_inner: error, levels was negative, %i (currentlevel %i, toterminal %i). Set to 0.", *level, currentlevel, (*(**cbs).cb).list.toterminal );
	  *level=0;
	}

	return err; // found or negation
}
// level 19.8.2015
int  cb_set_to_leaf_inner_levels(CBFILE **cbs, unsigned char **name, signed int namelen, signed int openpairs, signed int *level, cb_match *mctl){ // 11.4.2014, 23.3.2014
	signed int err = CBSUCCESS; signed char nextlevelempty=1;
	signed int startlevel=0; // tmp debug, 3.1.2017
	cb_name *leafptr = NULL;
	if(cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || name==NULL || mctl==NULL){
	  cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_set_to_leaf_inner_levels: allocation error."); return CBERRALLOC;
	}
	startlevel = *level; // tmp debug, 3.1.2017

	if( ( CBMAXLEAVES - 1 + openpairs ) < 0 || *level > CBMAXLEAVES ){ // just in case an erroneus loop hangs the application
	  cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_to_leaf_inner_levels: error, endless loop prevention, level exceeded %i.", *level);
	  return CBNOTFOUND;
	}

	if( (*(**cbs).cb).list.currentleaf==NULL ){
          //cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_to_leaf_inner_levels: currentleaf was empty, returning CBEMPTY with level %i.", *level );
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

	/***
	cb_clog( CBLOGDEBUG, CBNEGATION, " comparing ["); 
	if(name!=NULL)
		cb_print_ucs_chrbuf( CBLOGDEBUG, &(*name), namelen, namelen );
	cb_clog( CBLOGDEBUG, CBNEGATION, "] to [");
	if(leafptr!=NULL)
		cb_print_ucs_chrbuf( CBLOGDEBUG, &(*leafptr).namebuf, (*leafptr).namelen, (*leafptr).buflen );
	cb_clog( CBLOGDEBUG, CBNEGATION, "] " );
	 ***/

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

	    // CBSEARCHNEXTGROUPLEAVES 11.11.2016
            if( ( (**cbs).cf.leafsearchmethod==CBSEARCHNEXTLEAVES && (*leafptr).matchcount==1 ) || \
		( (**cbs).cf.leafsearchmethod==CBSEARCHNEXTGROUPLEAVES && \
			( (*leafptr).matchcount==1 || (*leafptr).group==(*(**cbs).cb).list.currentgroup ) ) || \
		( (**cbs).cf.leafsearchmethod==CBSEARCHNEXTLASTGROUPLEAVES && \
			( (*leafptr).matchcount==1 || (*leafptr).group<(*(**cbs).cb).list.currentgroup ) ) || \
		(**cbs).cf.leafsearchmethod==CBSEARCHUNIQUELEAVES ){ 
	      if( (*leafptr).matchcount==1 ) // test 19.11.2016
	        (*leafptr).group = (*(**cbs).cb).list.currentgroup; // 11.11.2016

	      //if( (*leafptr).group==(*(**cbs).cb).list.currentgroup && (**cbs).cf.leafsearchmethod==CBSEARCHNEXTGROUPLEAVES ){
		//cb_clog( CBLOGDEBUG, CBNEGATION, "\nSET_TO_LEAF (CBSEARCHNEXTGROUPLEAVES) GROUP %i, CURRENTGROUP %i, MATCHCOUNT %li [", (*leafptr).group, (*(**cbs).cb).list.currentgroup, (*leafptr).matchcount );
		//cb_print_ucs_chrbuf( CBLOGDEBUG, &(*leafptr).namebuf, (*leafptr).namelen, (*leafptr).buflen );
		//cb_clog( CBLOGDEBUG, CBNEGATION, "]");
	      //}

              if( (**cbs).cf.type!=CBCFGFILE && (**cbs).cf.type!=CBCFGSEEKABLEFILE && (**cbs).cf.type!=CBCFGBOUNDLESSBUFFER){ // When used as only buffer, stream case does not apply, 28.2.2016
                if((*leafptr).offset>=( (*(**cbs).cb).buflen + 0 + 1 ) ){ // buflen + smallest possible name + endchar
                  /*
                   * Leafs content was out of memorybuffer, setting
                   * CBFILE:s index to the edge of the buffer.
                   */
                  (*(**cbs).cb).index = (*(**cbs).cb).buflen; // set as a stream, 1.12.2013
                  return CBNAMEOUTOFBUF;
                }
	      } // this curly brace added 17.8.2015 (the same as before)
	      // index set here withoud boundary check if: CBCFGFILE, CBCFGSEEKABLEFILE, CBCFGBOUNDLESSBUFFER
              (*(**cbs).cb).index = (signed long int) (*leafptr).offset; // 1.12.2013

	      if(nextlevelempty==0){ // 3.7.2015
  	        return CBSUCCESSLEAVESEXIST; // 3.7.2015
	      }
	      return CBSUCCESS; // CBMATCH
            }//else
	      //cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_to_leaf_inner_levels: match, unknown searchmethod (%i,%i) or matchcount %li not 1.", (**cbs).cf.searchmethod, (**cbs).cf.leafsearchmethod, (*leafptr).matchcount );	
	  }//else
	    //cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_to_leaf_inner_levels: no match, openpairs==1 (effective with match)");
	}//else
	  //cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_to_leaf_inner_levels: openpairs was not 1.");

	if( (*leafptr).leaf!=NULL && openpairs>=1 ){ // Left
	  (*(**cbs).cb).list.currentleaf = &(* (cb_name*) (*leafptr).leaf);
	  *level = *level + 1; // level should return the last leafs openpairs count
	  //cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_to_leaf_inner_levels: TO LEFT." );
	  err = cb_set_to_leaf_inner_levels( &(*cbs), &(*name), namelen, (openpairs-1), &(*level), &(*mctl)); // 11.4.2014, 23.3.2014

	  if(err==CBSUCCESS || err==CBSUCCESSLEAVESEXIST){ // 3.7.2015
	    //if( startlevel>=(*level) )  // TMP DEBUG 3.1.2017
	    //	cb_clog( CBLOGDEBUG, CBNEGATION, "\nSET_TO_LEAF_INNER_LEVELS: STARTLEVEL %i, LEVEL %i (left, err %i).", startlevel, *level, err );
	    //cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_to_leaf_inner_levels: returning CBSUCCESS, levels %i (2 - leaf).", *level);
	    return err;
	  }else{
	    *level = *level - 1; // return to the previous level
	  }
	}
	if( (*leafptr).next!=NULL ){ // Right (written after leaf if it is written)
	  (*(**cbs).cb).list.currentleaf = &(* (cb_name*) (*leafptr).next);
	  //cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_to_leaf_inner_levels: TO RIGHT." );
	  err = cb_set_to_leaf_inner_levels( &(*cbs), &(*name), namelen, openpairs, &(*level), &(*mctl)); // 11.4.2014, 23.3.2014

	  if(err==CBSUCCESS || err==CBSUCCESSLEAVESEXIST ){ // 3.7.2015
	    //if( startlevel>=(*level) )  // TMP DEBUG 3.1.2017
	    //	cb_clog( CBLOGDEBUG, CBNEGATION, "\nSET_TO_LEAF_INNER_LEVELS: STARTLEVEL %i, LEVEL %i (right, err %i).", startlevel, *level, err );
	    return err;
	  }
	}else if( (*leafptr).next==NULL && (*leafptr).leaf!=NULL && openpairs>=0 ){ // from this else starts addition made 28.9.2015 -- addition 28.9.2015, 2.10.2015
	  /*
	   * If the previous leaf compared previously was not null, add level by one (the compared leaf). */
	  *level = *level + 1; // 2.10.2015 explanations: -1, if next null, +1 if leaf was not null previously
	}
	// here ends an addition made 28.9.2015 -- /addition 28.9.2015


	//if( startlevel>=(*level) )
	//  cb_clog( CBLOGDEBUG, CBNEGATION, "\nSET_TO_LEAF_INNER_LEVELS: STARTLEVEL %i, LEVEL %i (not found, err %i).", startlevel, *level, err );


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
int  cb_set_to_last_leaf(cb_name **tree, cb_name **lastleaf, signed int *openpairs){

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
	signed int leafcount = 0, nextcount=0; // count of leaves and overflow prevention
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
//cb_clog( CBLOGDEBUG, CBNEGATION, ".");
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
	  cb_clog( CBLOGWARNING, CBNEGATION, "\ncb_set_to_last_leaf: CBOVERMAXLEAVES (%i).", nextcount);
	  //return CBLEAFCOUNTERROR;
	  return CBOVERMAXLEAVES; // 19.10.2015
	}
	return CBSUCCESS; // returns allways a proper not null node or CBEMPTY, in error (CBLEAFCOUNTERROR), CBOVERMAXLEAVES; // 19.10.2015
}

/*
 * Put leaf in 'current' name -tree and update 'currentleaf'. */
int  cb_put_leaf(CBFILE **str, cb_name **leaf, signed int openpairs, signed int previousopenpairs){

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

	signed int leafcount=0, err=CBSUCCESS, ret=CBSUCCESS;
	cb_name *lastleaf  = NULL;
	cb_name *newleaf   = NULL;

	if( str==NULL || *str==NULL || (**str).cb==NULL || leaf==NULL || *leaf==NULL) return CBERRALLOC;

	//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_put_leaf: openpairs=%i previousopenpairs=%i [", openpairs, previousopenpairs);
	//cb_print_ucs_chrbuf( CBLOGDEBUG, CBNEGATION, &(**leaf).namebuf, (**leaf).namelen, (**leaf).buflen );
	//cb_clog( CBLOGDEBUG, CBNEGATION, "]");

	cb_init_terminalcount( &(*str) );

	/*
	 * Find last leaf */
	leafcount = openpairs;
	err = cb_set_to_last_leaf( &(*(**str).cb).list.current, &lastleaf, &leafcount);
	if(err>=CBERROR){ cb_clog( CBLOGERR, err, "\ncb_put_leaf: cb_set_to_last_leaf returned error %i.", err); }
	(*(**str).cb).list.currentleaf = &(*lastleaf); // last
	if( leafcount==0 || err==CBEMPTY ){ 
	  return CBEMPTY;
	}else if(openpairs<=1){
	  return CBLEAFCOUNTERROR;
	}

	/*
	 * Allocate and copy leaf */
        err = cb_allocate_name( &newleaf, (**leaf).namelen ); 

	if(err!=CBSUCCESS){ cb_clog( CBLOGALERT, err, "\ncb_put_leaf: cb_allocate_name error %i.", err); return err; }
        err = cb_copy_name( &(*leaf), &newleaf );
	if(err!=CBSUCCESS){ cb_clog( CBLOGERR, err, "\ncb_put_leaf: cb_copy_name error %i.", err); return err; }
	(*newleaf).firsttimefound = (signed long int) time(NULL);
	(*newleaf).next = NULL;
	(*newleaf).leaf = NULL;
        //++(*(**str).cb).list.nodecount;
	//if( newleaf!=NULL ) // 23.8.2015
	//  (*newleaf).serial = (*(**str).cb).list.nodecount; // 23.8.2015

	/*
	 * Update previous leafs length */
	err = cb_update_previous_length( &(*str), (**leaf).nameoffset, openpairs, previousopenpairs );
	if(err>=CBERROR){ cb_clog( CBLOGERR, err, "\ncb_put_leaf: cb_update_previous_length, error %i.", err );  return err; }

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
	    cb_clog( CBLOGWARNING, CBNEGATION, "\ncb_put_leaf: error, leafcount>openpairs.");
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
int  cb_update_previous_length(CBFILE **str, signed long int nameoffset, signed int openpairs, signed int previousopenpairs){
        if(str==NULL || *str==NULL || (**str).cb==NULL )
          return CBERRALLOC;
	// Previous names contents length

        if( (*(**str).cb).list.last!=NULL && (*(**str).cb).list.namecount>=1 && \
			nameoffset>=( (*(*(**str).cb).list.last).offset + 2 ) && nameoffset<0x7FFFFFFF ){
	  /*
	   * Names
	   */
	  if( openpairs==0 ){ // to list (longest length)
	     // NAME
	     if( (*(**str).cb).list.last!=NULL ){
	       if( (*(**str).cb).list.last==NULL ) return CBERRALLOC; 
	       //24.10.2018: (*(*(**str).cb).list.last).length = nameoffset - (*(*(**str).cb).list.last).offset - 2; // last is allways a name? (2: '=' and '&')
	       (*(*(**str).cb).list.last).length = (int) ( nameoffset - (*(*(**str).cb).list.last).offset - 2 ); // last is allways a name? (2: '=' and '&'), 24.10.2018
	       if( (*(*(**str).cb).list.last).length<0 ) // 24.10.2018
			(*(*(**str).cb).list.last).length = 0; // 2147483647; // 24.10.2018
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
	    if( (*(**str).cb).list.last==NULL ) return CBERRALLOC; 
	    if( (*(*(**str).cb).list.currentleaf).offset >= (*(*(**str).cb).list.last).offset ) // is lasts leaf
	      if( nameoffset >= ( (*(*(**str).cb).list.currentleaf).offset + 2 ) ) // leaf is after this leaf
	        (*(*(**str).cb).list.currentleaf).length = (int) ( nameoffset - (*(*(**str).cb).list.currentleaf).offset - 2 ); // 2: '=' and '&'
	      // since read pointer is allways at read position, leaf can be added without knowing if one leaf is missing in between
          }else if( previousopenpairs>openpairs ){  // from leaf to it's name in list (second time, name previously) or from leaf to a lower leaf
	    // LEAF
	    if( (*(**str).cb).list.currentleaf==NULL ) return CBERRALLOC; 
              (*(*(**str).cb).list.currentleaf).length = (int) ( nameoffset - (*(*(**str).cb).list.currentleaf).offset - 2 ); // 2: '=' and '&'
	  }
	}
	return CBSUCCESS;
}

int  cb_put_name(CBFILE **str, cb_name **cbn, signed int openpairs, signed int previousopenpairs){ // ocoffset late, 12/2014
        signed int err=0;
	cb_name *ptr = NULL;

        if(cbn==NULL || *cbn==NULL || str==NULL || *str==NULL || (**str).cb==NULL )
	  return CBERRALLOC;

	if( (**cbn).namebuf==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_put_name: name was null, error %i.", CBERRALLOC );   return CBERRALLOC; } // 23.10.2015

	cb_init_terminalcount( &(*str) );

	//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_put_name: openpairs=%i previousopenpairs=%i [", openpairs, previousopenpairs);
	//cb_print_ucs_chrbuf( CBLOGDEBUG, CBNEGATION, &(**cbn).namebuf, (**cbn).namelen, (**cbn).buflen );
	//cb_clog( CBLOGDEBUG, CBNEGATION, "]");


        /*
         * Do not save the name if it's out of buffer. cb_set_cursor finds
         * names from stream but can not use cb_names namelist if the names
         * are out of buffer. When used as file (CBCFGFILE or CBCFGSEEKABLEFILE),
         * filesize limits the list and this is not necessary.
         */
        //if((**str).cf.type!=CBCFGFILE && (**str).cf.type!=CBCFGSEEKABLEFILE)
        if((**str).cf.type!=CBCFGFILE && (**str).cf.type!=CBCFGSEEKABLEFILE && (**str).cf.type!=CBCFGBOUNDLESSBUFFER) // 28.2.2016
          if( (**cbn).offset >= ( (*(**str).cb).buflen-1 ) || ( (**cbn).offset + (**cbn).length ) >= ( (*(**str).cb).buflen-1 ) )
            return CBNAMEOUTOFBUF;

        if((*(**str).cb).list.name!=NULL){
          (*(**str).cb).list.current = &(*(*(**str).cb).list.last);
          (*(**str).cb).list.currentleaf = &(*(*(**str).cb).list.last); // 11.12.2013

	  /*
	   * Tested maximum limits. With max 4 leafs, limit 10872. 19.10.2015
	   */
	  if( (*(**str).cb).list.namecount>=CBMAXNAMES ){
	    cb_clog( CBLOGWARNING, CBNEGATION, "\ncb_put_name: CBOVERMAXNAMES (%lli).", (*(**str).cb).list.namecount );
	    return CBOVERMAXNAMES;
	  }

          ++(*(**str).cb).list.nodecount; // 28.10.2015
	  
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
	  if(err>=CBERROR){ cb_clog( CBLOGERR, err, "\ncb_put_name: cb_update_previous_length, error %i.", err );  return err; }

	  // Add name
          //(*(**str).cb).list.last    = &(* (cb_name*) (*(*(**str).cb).list.last).next );
// 7.10.2021, note:
          //7.10.2021err = cb_allocate_name( &(*(**str).cb).list.last, (**cbn).namelen ); if(err!=CBSUCCESS){ return err; } // 7.12.2013
          err = cb_allocate_name( &ptr, (**cbn).namelen ); if(err!=CBSUCCESS){ return err; } // 7.12.2013, 7.10.2021

	  (*(*(**str).cb).list.last).next = &(*ptr); // 7.10.2021
	  (*(**str).cb).list.last         = &(* (cb_name*) (*(*(**str).cb).list.last).next ); // 7.10.2021

          (*(*(**str).cb).list.current).next = &(*(*(**str).cb).list.last); // previous
          (*(**str).cb).list.current = &(*(*(**str).cb).list.last);
          (*(**str).cb).list.currentleaf = &(*(*(**str).cb).list.last);
          ++(*(**str).cb).list.namecount;
          //++(*(**str).cb).list.nodecount;
	  //if( (*(**str).cb).list.last!=NULL ) // 23.8.2015
	  //  (*(*(**str).cb).list.last).serial = (*(**str).cb).list.nodecount; // 23.8.2015
	  //if( (**str).cf.type!=CBCFGFILE && (**str).cf.type!=CBCFGSEEKABLEFILE) // 20.12.2014
	  if( (**str).cf.type!=CBCFGFILE && (**str).cf.type!=CBCFGSEEKABLEFILE){ // 20.12.2014
            if( ( (*(**str).cb).contentlen - (**str).ahd.bytesahead ) >= (*(**str).cb).buflen ){ // 6.9.2013
              //TEST: (*(*(**str).cb).list.current).length = (*(**str).cb).buflen; // Largest 'known' value
              (*(*(**str).cb).list.current).length = -1; // TEST 15.8.2017, -1 'unknown', result: no effect (+ test execution time is the same )
	      //cb_clog( CBLOGDEBUG, CBNEGATION, "\nSETTING ATTRIBUTE LENGTH TO BUFFER LENGTH, contentlen %li, ahd %i, buflen %li", 
		//(*(**str).cb).contentlen, (**str).ahd.bytesahead, (*(**str).cb).buflen ); // TMP, 14.8.2017
	    }
	  }
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
int  cb_get_current_level_at_edge(CBFILE **cbs, signed int *level){ // addition 29.9.2015
	signed int currentlevel=0, err=CBSUCCESS;
	if( cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || level==NULL ) return CBERRALLOC;
	err = cb_get_current_level( &(*cbs),  &currentlevel);
	if(err>=CBERROR){ cb_clog( CBLOGERR, err, "\ncb_get_current_level_at_edge: cb_get_current_level, error %i.", err); }
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

	// 10.3.2017, just in case
	if( *level < 0 )
		*level = 0;

	return err;
}
/*
 * Returns the last read leafs level to be used
 * in reading the leaves of the names correctly. 
 *
 * Returns the level (0 if all are null).*/
int  cb_get_current_level(CBFILE **cbs, signed int *level){ // addition 28.9.2015
	signed int err = CBSUCCESS;
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

	if( (*iter).leaf==NULL ){
	  return CBSUCCESS;
	}else{
	  *level = 2; // 6.10.2015, first leaf, from open pairs
	}
	iter = &(* (cb_name*) (*iter).leaf ); // XV , here, level is from closed pair
	err = cb_get_current_level_sub( &iter , &(*level) );
	return err;
}
int  cb_get_current_level_sub(cb_name **cn, signed int *level){ // addition 28.9.2015
	signed int err = CBSUCCESS;
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
	        //*level += 1;
	        ++(*level);
		iter = &(* (cb_name*) (**cn).leaf );
		err = cb_get_current_level_sub( &iter, &(*level) );
	}else{ 
	 	; 
	}

	return CBSUCCESS;
}

/*
 * Returns 
 *   on success: CBSUCCESS, CBSUCCESSLEAVESEXIST
 *   on negation: CBEMPTY, CBNOTFOUND, CBNOTFOUNDLEAVESEXIST, CBNAMEOUTOFBUF (if not file or seekable file)
 *   on error: CBERRALLOC.
 */
int  cb_set_to_name(CBFILE **str, unsigned char **name, signed int namelen, cb_match *mctl){ // cb_match 11.3.2014, 23.3.2014
	cb_name *iter = NULL; signed int err=CBSUCCESS; signed char nextlevelempty=1;
	cb_name *tmp  = NULL;

	if(str==NULL || *str==NULL || name==NULL || *name==NULL || mctl==NULL)  // 22.10.2015
		return CBERRALLOC;

	/** Debug print 25.12.2021
        cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_to_name: comparing ["); 
	if( name!=NULL && *name!=NULL )
		cb_print_ucs_chrbuf( CBLOGDEBUG, &(*name), namelen, namelen );
	cb_clog( CBLOGDEBUG, CBNEGATION, "] ");
	 **/

	if(*str!=NULL && (**str).cb != NULL && mctl!=NULL ){
	  if( (*(**str).cb).list.name!=NULL ){ // 12.12.2021
		iter = &(*(*(**str).cb).list.name);
	  }else{
		iter = NULL;
	  }
	  while(iter != NULL){
//cb_clog( CBLOGDEBUG, CBNEGATION, ",");
	    /*
	     * Helpful return value comparison. */
	    if( (*iter).leaf!=NULL ) // 3.7.2015
	      nextlevelempty=0; // 3.7.2015
	    err = cb_compare( &(*str), &(*name), namelen, &(*iter).namebuf, (*iter).namelen, &(*mctl) ); // 23.11.2013, 11.4.2014
	    if( err == CBMATCH ){ // 9.11.2013
/*** 11.12.2021, debug print 25.12.2021
cb_clog( CBLOGDEBUG, CBNEGATION, "[MATCH, '");
if( iter!=NULL && (*iter).namebuf!=NULL )
  cb_print_ucs_chrbuf( CBLOGDEBUG, &(*iter).namebuf, (*iter).namelen, (*iter).namelen );
cb_clog( CBLOGDEBUG, CBNEGATION, "']");
 ***/
	      /*
	       * 20.8.2013:
	       * If searching of multiple same names is needed (in buffer also), do not return
	       * allready matched name. Instead, increase names matchcount (or set to 1 if it
	       * becomes 0) and search the next same name.
	       */
	      (*iter).matchcount++; if( (*iter).matchcount==0 ){ (*iter).matchcount+=2; }
	      /* First match on new name or if unique names are in use, the first match or the same match again, even if in stream. */
	      //11.11.2016: if( ( (**str).cf.searchmethod==CBSEARCHNEXTNAMES && (*iter).matchcount==1 ) || (**str).cf.searchmethod==CBSEARCHUNIQUENAMES ){
	      //12.12.2021: if( ( (**str).cf.searchmethod==CBSEARCHNEXTNAMES && (*iter).matchcount==1 ) ||
	      /* 12.12.2021: First one is the former 'groupless' search, now a form of a unique search
               *             across groups (otherwice 'cb_match' is used with 'cb_compare'). */
	      if( (*mctl).unique_namelist_search==1 || \
	          ( (**str).cf.searchmethod==CBSEARCHNEXTNAMES && (*iter).matchcount==1 ) || \
		  ( (**str).cf.searchmethod==CBSEARCHNEXTGROUPNAMES && \
			( (*iter).matchcount==1 || (*iter).group==(*(**str).cb).list.currentgroup ) ) || \
		  ( (**str).cf.searchmethod==CBSEARCHNEXTLASTGROUPNAMES && \
			( (*iter).matchcount==1 || (*iter).group==(*(**str).cb).list.currentgroup ) ) || \
		  (**str).cf.searchmethod==CBSEARCHUNIQUENAMES ){
		if( (*iter).matchcount==1 ) // test 19.11.2016
	          (*iter).group = (*(**str).cb).list.currentgroup; // 11.11.2016
	        if( (**str).cf.type!=CBCFGFILE && (**str).cf.type!=CBCFGSEEKABLEFILE && (**str).cf.type!=CBCFGBOUNDLESSBUFFER) // When used as only buffer, stream case does not apply, 28.2.2016
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
		// 28.2.2106: adds here without boundary check if CBCFGFILE, CBCFGSEEKABLEFILE, CBCFGBOUNDLESSBUFFER
	        (*(**str).cb).index=(*iter).offset; // 1.12.2013
	        (*(**str).cb).list.current=&(*iter); // 1.12.2013
	        (*(**str).cb).list.currentleaf=&(*iter); // 11.12.2013

		/*
		 * Reset or update the 'last_name' before returning, 2.1.2017.. */
		(*(**str).cb).list.rd.last_name  = &(* (*(**str).cb).list.current );
		(*(**str).cb).list.rd.last_level = 0;

		//cb_clog( CBLOGDEBUG, CBNEGATION, "\nCB_SET_TO_NAME" );

		if(nextlevelempty==0) // 3.7.2015
		  return CBSUCCESSLEAVESEXIST; // 3.7.2015
	        return CBSUCCESS;
	      }
	    }
	    if( (*iter).next!=NULL ){
		tmp = &(* (cb_name *) (*iter).next); // 12.12.2021
		iter = &(* (cb_name *) tmp ); // 12.12.2021
	    	//12.12.2021: iter = &(* (cb_name *) (*iter).next); // 9.12.2013
	    }else{
		iter = NULL; // 12.12.2021
	    }
	  }
	  (*(**str).cb).index = (*(**str).cb).contentlen - (**str).ahd.bytesahead;  // 5.9.2013
	}else{
	  cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_set_to_name: allocation error.");
	  return CBERRALLOC; // 30.3.2013
	}
	if(nextlevelempty==0)
	  return CBNOTFOUNDLEAVESEXIST;
	return CBNOTFOUND;
}

int  cb_search_get_chr( CBFILE **cbs, unsigned long int *chr, long int *chroffset ){
	signed int err = CBERROR, bytecount=0, storedbytes=0;
	if(cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || chroffset==NULL || chr==NULL){
	  //cb_clog( CBLOGWARN, CBERRALLOC, "\ncb_search_get_chr: null parameter was given, error CBERRALLOC.", CBERRALLOC);
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
	  if( err>=CBERROR ){ cb_clog( CBLOGERR, err, "\ncb_search_get_chr: cb_fifo_init_counters, error %i.", err ); }
	}
	if((**cbs).cf.unfold==1){
	  err = cb_get_chr_unfold( &(*cbs), &(**cbs).ahd, &(*chr), &(*chroffset) );
	  (*(**cbs).cb).list.rd.lastreadchrendedtovalue=0; // 7.10.2015, lastreadchrwasfromoutside=0; // 29.9.2015
	  if( err==CBERRTIMEOUT ){
		cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_search_get_chr: cb_get_chr_unfold CBERRTIMEOUT.");
		cb_flush_log();
	  }
	/***
	 ***/
	}else{
	  err = cb_get_chr( &(*cbs), &(*chr), &bytecount, &storedbytes );
	  if( err==CBERRTIMEOUT ){
		cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_search_get_chr: cb_get_chr CBERRTIMEOUT.");
		cb_flush_log();
	  }
/***
else if( err!=CBSUCCESS ){
	cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_search_get_chr: cb_get_chr %i.", err );
	cb_flush_log();
}
 ***/
	  // Stream, buffer, file
	  *chroffset = (*(**cbs).cb).index - 1; // 2.12.2013
	  // Seekable file
	  // was 2.8.2015: if( (**cbs).cf.type==CBCFGSEEKABLEFILE )
	  // was 2.8.2015:   if( (*(**cbs).cb).readlength>=0 ) // overflow
	  if( (**cbs).cf.type==CBCFGSEEKABLEFILE ){
	    if( (*(**cbs).cb).readlength>0 ){ // no overflow, 2.8.2015
	      *chroffset = (*(**cbs).cb).readlength - 1;
	    }
	  }

	  (*(**cbs).cb).list.rd.lastreadchrendedtovalue=0; // 7.10.2015, lastreadchrwasfromoutside=0; // 29.9.2015
	}

	/* 28.9.2017. */
	if( err>=CBNEGATION && err<CBERROR && (*(**cbs).cb).list.rd.encodingerroroccurred<CBNEGATION )
		(*(**cbs).cb).list.rd.encodingerroroccurred = err;

	if( (**cbs).cf.stopateof==1 && *chr==(unsigned long int)0x00FF ){ // Unicode EOF 0x00FF chart U0080, 31.10.2016
		if( err<CBNEGATION && err!=CBMESSAGEEND && err!=CBMESSAGEHEADEREND ){
			//cb_clog( 22, CBSUCCESS, "\nEOF CBENDOFFILE" );
			(*(**cbs).cb).list.rd.stopped = 1;
			return CBENDOFFILE;
		}
	}

	if( err==CBSTREAMEND ) (*(**cbs).cb).list.rd.stopped = 1; // 15.8.2017

/***
if( err==CBERRTIMEOUT ){
  cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_search_get_chr return CBERRTIMEOUT" );
  cb_flush_log();
}else if(err!=CBSUCCESS){
  cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_search_get_chr: return %i (3).", err );
  cb_flush_log();
}
 ***/
	return err;
}

inline signed int  cb_is_rstart(CBFILE **cbs, unsigned long int chr){
	if(cbs==NULL || *cbs==NULL){ return CBERRALLOC; }
	if( chr==(**cbs).cf.rstart )
	  return true;
	/*
	 * BUG with unfolding: 20.3.2016. Removes first character of the next word if unfolding is set. 
	 */
	if( (**cbs).cf.findwords==1 ){ // name end, record start
	  if( WSP( chr ) || CR( chr ) || LF( chr ) )
	    return true;
	}
	if( (**cbs).cf.findwordssql==1 && sqlrstartchr( chr ) && chr!=(**cbs).cf.rend && chr!=(**cbs).cf.subrend ) // 4.12.2016, 9.11.2017
	  return true; // 4.12.2016, 9.11.2017
	if( (**cbs).cf.findwordssqlplusbypass==1 && ( chr==(**cbs).cf.bypass || sqlrstartchr( chr ) ) && chr!=(**cbs).cf.rend && chr!=(**cbs).cf.subrend ) // 9.11.2017
	  return true; // 9.11.2017
	return false;
}

inline signed int  cb_is_rend(CBFILE **cbs, unsigned long int chr, unsigned long int *matched_rend ){
	if(cbs==NULL || *cbs==NULL){ return CBERRALLOC; }
	if( chr==(**cbs).cf.rend ){ // '$', record end, name start
	  if( matched_rend!=NULL ) *matched_rend = chr;
//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_is_rend %li, rend (cbfile id %i).", chr, (**cbs).id );
	  return true;
	}
	if( (**cbs).cf.findwordstworends==1 && chr==(**cbs).cf.subrend ){
	  if( matched_rend!=NULL ) *matched_rend = chr;
//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_is_rend %li, subrend (cbfile id %i).", chr, (**cbs).id );
	  return true; // 3.11.2016
	}
	if( (**cbs).cf.thirdrend!=-1 && chr==(**cbs).cf.thirdrend ){
	  if( matched_rend!=NULL ) *matched_rend = chr; // 1.10.2021
//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_is_rend %li, thirdrend (cbfile id %i).", chr, (**cbs).id );
	  return true; // 25.9.2021
	}
	return false;
}

/*
 * Name has one byte for each character.
 */
int  cb_set_cursor_match_length(CBFILE **cbs, unsigned char **name, signed int *namelength, signed int ocoffset, signed int matchctl){
	signed int indx=0, err=CBSUCCESS; signed int bufsize=0;
	signed int chrbufindx=0;
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
int  cb_set_cursor(CBFILE **cbs, unsigned char **name, signed int *namelength){
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
int  cb_set_cursor_ucs(CBFILE **cbs, unsigned char **ucsname, signed int *namelength){
	return cb_set_cursor_match_length_ucs( &(*cbs), &(*ucsname), &(*namelength), 0, 1 );
}
int  cb_set_cursor_match_length_ucs(CBFILE **cbs, unsigned char **ucsname, signed int *namelength, signed int ocoffset, signed int matchctl){
	cb_match mctl;
	cb_fill_empty_matchctl( &mctl ); // 12.12.2021
	mctl.re = NULL; mctl.matchctl = matchctl; mctl.resmcount = 0;
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
	return CBSUCCESS;
}
int  cb_increase_terminalcount(CBFILE **cbs){
	if( cbs==NULL || *cbs==NULL || (**cbs).cb==NULL) return CBERRALLOC;
	++(*(**cbs).cb).list.toterminal;
	if((*(**cbs).cb).list.toterminal<0)
	  (*(**cbs).cb).list.toterminal=0;
	return CBSUCCESS;
}

/* 30.10.2016. */
int  cb_test_eof_read( CBFILE **cbs ){
	if( cbs==NULL || *cbs==NULL || (**cbs).cb==NULL ) return CBERRALLOC;
	if( (*(**cbs).cb).eofoffset>0 && (*(**cbs).cb).eofoffset<=(*(**cbs).cb).index )
		return CBSUCCESS;
	return CBNEGATION;
}
/*
 * 8.5.2016. */
int  cb_test_message_end( CBFILE **cbs ){
	signed int ret = CBSUCCESS;
	if( cbs==NULL || *cbs==NULL ) return CBERRALLOC;
	if( (**cbs).cb==NULL ){ // 30.6.2016
		cb_clog( CBLOGERR, CBERRALLOC, "\ncb_test_message_end: cb was null, error %i.", CBERRALLOC ); return CBERRALLOC;
	}
        /*
         * Stop at the message payload end set outside of the library with cb_set_message_end . */
        if( (**cbs).cf.stopatmessageend==1 && (*(**cbs).cb).messageoffset>0 ){
		if( ( (*(**cbs).cb).index+1 )+(**cbs).ahd.bytesahead >= (*(**cbs).cb).messageoffset || \
		   (**cbs).ahd.currentindex >= (*(**cbs).cb).messageoffset ){
			cb_fifo_set_endchr( &(**cbs).ahd );
		}
                if( (*(**cbs).cb).index+1 >= (*(**cbs).cb).messageoffset || (*(**cbs).cb).messageoffset <= (*(**cbs).cb).contentlen ){
                   ret = CBMESSAGEEND; // disregard negations and errors if header end
		}
        }
	return ret;
}

/*
 * 8.5.2016. */
int  cb_test_header_end( CBFILE **cbs ){
	signed int ret = CBSUCCESS;
	if( cbs==NULL || *cbs==NULL ) return CBERRALLOC;
        if( (**cbs).cf.stopatheaderend==1 ){
	  if( (*(**cbs).cb).headeroffset>0 && (*(**cbs).cb).headeroffset <= (*(**cbs).cb).contentlen )
	    ret = CBMESSAGEHEADEREND;
	}
	return ret;
}

#define json_value_first_letter( x )   ( WSP( x ) || x=='{' || x=='[' || x=='\"' || x=='t' || x=='f' || x=='n' || ( x>=0x30 && x<=0x39 ) )

int  cb_set_cursor_match_length_ucs_matchctl(CBFILE **cbs, unsigned char **ucsname, signed int *namelength, signed int ocoffset, cb_match *mctl){ // 23.2.2014
	signed int  err=CBSUCCESS, cis=CBSUCCESS, ret=CBNOTFOUND; // return values
	signed int  buferr=CBSUCCESS, savenameerr=CBSUCCESS;
	signed int  index=0, freecount=0;
	unsigned long int chr=0, chprev=CBRESULTEND; 
	signed long int chroffset=0;
	signed long int nameoffset=0; // 6.12.2014
	signed char tovalue = 0;
	signed char injsonquotes = 0, injsonarray = 0, wasjsonrend=0, wasjsonsubrend=0, jsonemptybracket=0; // json values
	unsigned char  charbuf[CBNAMEBUFLEN+1]; // array 30.8.2013
	unsigned char *charbufptr = NULL;
	cb_name *fname = NULL; // Allocates two times, reads first to charbur, then allocates clean version to fname and finally in cb_put_name writing third time allocating the second time
	unsigned char atvalue=0; 
	unsigned char rstartflipflop=0; // After subrstart '{', rend ';' has the same effect as concecutive rstart '=' and rend ';' (empty value).
			       // After rstart '=', subrstart '{' or subrend '}' has no effect until rend ';'.  (3.10.2015: and reader should read until rend)
			       // For example: name { name2=text2; name3 { name4=text4; name5=text5; } }
			       // (JSON can be replaced by setting rstart '"', rend '"' and by removing ':' from name)
	signed char lastinnamelist = 0;
        unsigned long int ch3prev=CBRESULTEND+2, ch2prev=CBRESULTEND+1;
	signed int openpairs=0, previousopenpairs=0; // cbstatetopology, cbstatetree (+ cb_name length information)
	signed int levels=0; // try to find the last read openpairs with this variable
	unsigned long int matched_rend = 0x20;

	if( cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || mctl==NULL){
	  cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_set_cursor_match_length_ucs_matchctl: allocation error."); return CBERRALLOC;
	}
	chprev=(**cbs).cf.bypass+1; // 5.4.2013

	/*
	 * 20.3.2016, if wordlist or onename search, the rstart and rend are backwards.
 	 * Searching should start from rend, not from rstart. CBSTATEFUL.
	 */
	if( (**cbs).cf.searchstate==CBSTATEFUL && ( (**cbs).cf.findwords==1 || (**cbs).cf.searchnameonly==1 ) ){
	  atvalue=1; // TEST 20.3.2016
	}

	/* 28.9.2017. */
	(*(**cbs).cb).list.rd.encodingerroroccurred = CBSUCCESS;

/**
	cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_cursor_match_length_ucs_matchctl: ocoffset %i, matchctl %i", ocoffset, (*mctl).matchctl );
	if( (**cbs).cf.searchmethod==CBSEARCHUNIQUENAMES )
		cb_clog( CBLOGDEBUG, CBNEGATION, ", unique names");
	else if( (**cbs).cf.searchmethod==CBSEARCHNEXTNAMES )
		cb_clog( CBLOGDEBUG, CBNEGATION, ", next names");
	if( (**cbs).cf.leafsearchmethod==CBSEARCHUNIQUENAMES )
		cb_clog( CBLOGDEBUG, CBNEGATION, ", unique leaves");
	else if( (**cbs).cf.searchmethod==CBSEARCHNEXTNAMES )
		cb_clog( CBLOGDEBUG, CBNEGATION, ", next leaves");
	if( (**cbs).cf.searchstate==CBSTATETREE )
		cb_clog( CBLOGDEBUG, CBNEGATION, ", cbstatetree, [");
	else
		cb_clog( CBLOGDEBUG, CBNEGATION, ", not cbstatetree, [");
 **/
	//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_cursor_match_length_ucs_matchctl: ocoffset %i, matchctl %i, NAME [", ocoffset, (*mctl).matchctl );
	//cb_print_ucs_chrbuf( CBLOGDEBUG, &(*ucsname), *namelength, CBNAMEBUFLEN );
	//cb_clog( CBLOGDEBUG, CBNEGATION, "].");

	//if(*ucsname==NULL || namelength==NULL ){ // 13.12.2014
	if( ucsname==NULL || *ucsname==NULL || namelength==NULL ){ // 7.7.2015
	  cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_set_cursor_match_length_ucs_matchctl: allocation error, ucsname or length was null.");
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
		if( cb_is_rend( &(*cbs), (*(**cbs).cb).list.rd.lastchr, &matched_rend ) || ( (*(**cbs).cb).list.rd.lastchr==(**cbs).cf.subrend && \
			( (**cbs).cf.doubledelim==1 || (**cbs).cf.json==1 ) ) ){ // 8.10.2015 (this can be done in any case and this line was not needed)
		  previousopenpairs=CBMAXLEAVES; // 6.10.2015, from up to down
		  cb_increase_terminalcount( &(*cbs) );
		  if( cb_is_rend( &(*cbs), (*(**cbs).cb).list.rd.lastchr, &matched_rend ) ){ // wasjsonsubrend is needed here because of '}' ','
		    wasjsonrend=1; wasjsonsubrend=0; rstartflipflop=0;
		  }else if( (*(**cbs).cb).list.rd.lastchr==(**cbs).cf.subrend ){
		    wasjsonrend=0; wasjsonsubrend=1;
		  }
		  (*(**cbs).cb).list.rd.lastchr=(**cbs).cf.rend+1; // just in case removes the character (not needed, 6.11.2015)
		  if( (*(**cbs).cb).list.rd.lastchr==(**cbs).cf.subrend )
		     (*(**cbs).cb).list.rd.lastchr=(**cbs).cf.rend+2;
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
	      cb_clog( CBLOGERR, CBERRALLOC, "\ncb_set_cursor_match_length_ucs_matchctl: cb_get_current_level, allocation error. ");
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

/**** TEST 20.7.2021. copied into cb_transfer.c
      RETEST 20.7.2021
 ****/
/**** removed 21.7.2021, cursor may be in any location, this is a wrong location to test
	if( err==CBNOTFOUND && cb_test_messageend( &(**cbs) )==CBSUCCESS ){
		ret = err;
		goto cb_set_cursor_ucs_return;
	}
 ****/

        // 1.10.2015
        // VIII still too many set_length -functions. Excess has been marked with text "28.9.2015". Part of them can be removed.
        //      (7.10.2015: not fixed, 8.10.2015: not fixed)


	if(previousopenpairs==CBMAXLEAVES) // 7.10.2015, from up to down (toterminal was increased since the last read)
	  previousopenpairs=openpairs+1;
	else // unknown, cursor id between = and & 
	  previousopenpairs=openpairs; // 7.10.2015
	

	if(err==CBSUCCESS || err==CBSUCCESSLEAVESEXIST){ // 3.7.2015
	  //cb_clog( CBLOGDEBUG, CBNEGATION, "\nName found from list.");
	  if( (*(**cbs).cb).buflen > ( (*(*(**cbs).cb).list.current).length+(*(*(**cbs).cb).list.current).offset ) ){
	    ret = CBSUCCESS;
	    goto cb_set_cursor_ucs_return;
	  }else{
	    /* Found name but it's length is over the buffer length. */
	    /*
	     * Stream case CBCFGSTREAM. Used also in CBCFGFILE (unseekable). CBCFGBUFFER just in case. */
	    cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_cursor: Found name but it's length is over the buffer length."); // \n");
 /***
	    cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_cursor: type %i, buflen %li, current length %i, current offset %li, name [", (**cbs).cf.type, \
		(*(**cbs).cb).buflen, (*(*(**cbs).cb).list.current).length, (*(*(**cbs).cb).list.current).offset );
	    cb_print_ucs_chrbuf( CBLOGDEBUG, &(*(*(**cbs).cb).list.current).namebuf, (*(*(**cbs).cb).list.current).buflen, CBNAMEBUFLEN );
	    cb_clog( CBLOGDEBUG, CBNEGATION, "], names:");
	    cb_print_names( &(*cbs), CBLOGDEBUG );
  ***/
	    /*
	     * 15.12.2014: matchcount prevents matching again. If seekable file is in use,
	     * file can be seeked to old position. In this case, found names offset is returned
	     * even if it's out of buffers boundary. 
	     */
	    if( (**cbs).cf.type==CBCFGSEEKABLEFILE ){
	       ret = CBSUCCESS;
	       goto cb_set_cursor_ucs_return; // palautuva offset oli kuitenkin viela puskurin kokoinen
	                                      // returning offset was still the size of a buffer
	    }else if( (**cbs).cf.type==CBCFGFILE ){ // && (*(*(**cbs).cb).list.current).offset <= (*(**cbs).cb).buflen ){

//cb_clog( CBLOGDEBUG, CBNEGATION, "\n\n\tCBCFGFILE\n");

	       /*
	        * 14.8.2017, if name was found from buffer, return normally. Previous comments were:
	        * "CBCFGBUFFER just in case.". These lines added 15.8.2017. */
		ret = CBSUCCESS;
		goto cb_set_cursor_ucs_return;
		// Return name position even if the content may be out of buffer, possibly cb_put_name error here, 14.8.2017
	    }
	  }
	}else if(err==CBNAMEOUTOFBUF){
	  //cb_clog( CBLOGDEBUG, CBNAMEOUTOFBUF, "\ncb_set_cursor: Found old name out of cache and stream is allready passed by,\n");
	  //cb_clog( CBLOGDEBUG, CBNNAMEOUTOFBUF, "\n               set cursor as stream and beginning to search the same name again.\n");
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
	  cb_clog( CBLOGDEBUG, CBNEGATION, "\nCBSTATELESS");
	if((**cbs).cf.searchstate==1)
	  cb_clog( CBLOGDEBUG, CBNEGATION, "\nCBSTATEFUL");
	if((**cbs).cf.searchstate==2)
	  cb_clog( CBLOGDEBUG, CBNEGATION, "\nCBSTATETOPOLOGY");
	if((**cbs).cf.searchstate==3)
	  cb_clog( CBLOGDEBUG, CBNEGATION, "\nCBSTATETREE");
*/

	// Initialize memory characterbuffer and its counters
	memset( &(charbuf[0]), (int) 0x20, (size_t) CBNAMEBUFLEN);
	//if(charbuf==NULL){  return CBERRALLOC; }
	charbuf[CBNAMEBUFLEN]='\0';
	charbufptr = &charbuf[0];


	// Set cursor to the end to search next names
	(*(**cbs).cb).index = (*(**cbs).cb).contentlen; // -bytesahead ?


	// 30.10.2016: test if EOF was already read if stopateof
	if( (**cbs).cf.stopateof==1 ){
		if( (*(**cbs).cb).eofoffset>0 && (*(**cbs).cb).eofoffset<=(*(**cbs).cb).index ){ 
			if( (*(**cbs).cb).eofoffset<(*(**cbs).cb).index ) // 9.3.2017
				cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_cursor_ucs: EOF offset %li was over the index %li, returning CBENDOFFILE.", (*(**cbs).cb).eofoffset, (*(**cbs).cb).index );
			//cb_clog( 22, CBSUCCESS, "\nEOF CBENDOFFILE" );
			ret = CBENDOFFILE;
			(*(**cbs).cb).list.rd.stopped = 1;
               		goto cb_set_cursor_ucs_return;
			//19.12.2016: return CBENDOFFILE;
		}
	}

	//cb_clog( CBLOGDEBUG, CBNEGATION, "\nopenpairs %i ocoffset %i", openpairs, ocoffset );


	//cb_clog( CBLOGDEBUG, CBNEGATION, "\nring buffer data first %i last %i {", (**cbs).ahd.first, (**cbs).ahd.last );
	//cb_fifo_print_buffer( &(**cbs).ahd, CBLOGDEBUG );
	//cb_clog( CBLOGDEBUG, CBNEGATION, "} ");


	// Allocate new name
cb_set_cursor_reset_name_index:
	index=0;
	injsonquotes=0;
	injsonarray=0;


        /*
         * 'fname' was copied to a new allocated name in the list and used the last time. Time to free fname. 18.11.2015 / 2. */
	if(fname!=NULL){
            (*fname).next=NULL; (*fname).leaf=NULL; cb_free_name( &fname, &freecount ); fname=NULL;
	}


	// Search for new name
	// ...& - ignore and set to start
	// ...= - save any new name and compare it to 'name', if match, return

	if( cb_test_header_end( &(*cbs) ) == CBMESSAGEHEADEREND ){
		ret = CBMESSAGEHEADEREND;
		cb_update_previous_length( &(*cbs), chroffset, openpairs, previousopenpairs); // 19.7.2016 STILL IN TEST (branch 'transfer')
		goto cb_set_cursor_ucs_return;
	}

	// ADDED HERE 25.3.2016. THIS SHOULD BE CHECKED.
        if( (**cbs).cf.stopatheaderend==1 ){ // 26.3.2016
          // Automatic stop at header-end if it's set
	  //cb_clog( CBLOGDEBUG, CBNEGATION, "\n[0x%.2x,0x%.2x,0x%.2x,0x%.2x]", (unsigned int) ch3prev, (unsigned int) ch2prev, (unsigned int) chprev, (unsigned int) chr );

          if( ch3prev==0x0D && ch2prev==0x0A && chprev==0x0D && chr==0x0A ){ // cr lf x 2
            if( (*(**cbs).cb).headeroffset < 0 ){
	      if( chroffset < 0x7FFFFFFF){ // integer size - 1 
                (*(**cbs).cb).headeroffset = (int) chroffset; // 1.9.2013, offset set at last new line character, 26.3.2016, 24.10.2018
                //24.10.2018: (*(**cbs).cb).headeroffset = chroffset; // 1.9.2013, offset set at last new line character, 26.3.2016
	      }
	    }
	    ret = CBMESSAGEHEADEREND;
	    cb_update_previous_length( &(*cbs), chroffset, openpairs, previousopenpairs); // 19.7.2016 STILL IN TEST (branch 'transfer')
	    goto cb_set_cursor_ucs_return;
          }
	}
	ch3prev = ch2prev; ch2prev = chprev; chprev = chr; // 25.3.2016
	// /ADDED HERE 25.3.2016

	if( ret!=CBMESSAGEEND && ret!=CBMESSAGEHEADEREND ){ // 27.3.2016
// 1
	  err = cb_search_get_chr( &(*cbs), &chr, &chroffset); // 1.9.2013
	}else{
	  err = ret;
	}

	if( err==CBERRTIMEOUT ){ // 25.12.2021
	  cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_cursor: cb_search_get_chr CBERRTIMEOUT (1)" );
	  cb_flush_log();
	}else if( err!=CBSUCCESS ){
	  //cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_cursor: cb_search_get_chr %i (1)", err );
	  //cb_flush_log();
	}
	if( err==CBERRTIMEOUT ){ 
	    ret = CBERRTIMEOUT; // 18.7.2021 TEST
	    goto cb_set_cursor_ucs_return;
	}

	//cb_clog( CBLOGDEBUG, CBNEGATION, "|%c,0x%X|", (unsigned char) chr, (unsigned char) chr );
	//cb_clog( CBLOGDEBUG, CBNEGATION, "|%c|", (unsigned char) chr ); // BEST
	//cb_clog( CBLOGDEBUG, CBNEGATION, "\nset_cursor: new name, index (chr offset %li).", chroffset );
        //cb_clog( CBLOGDEBUG, CBNEGATION, "\nring buffer counters:");
        //cb_fifo_print_counters( &(**cbs).ahd, CBLOGDEBUG );
        //cb_clog( CBLOGDEBUG, CBNEGATION, "\ndata [");
	//cb_fifo_print_buffer( &(**cbs).ahd, CBLOGDEBUG );
        //cb_clog( CBLOGDEBUG, CBNEGATION, "]");

	/*
	 * 6.12.2014:
	 * If name has to be replaced (not a stream function), information where
	 * the name starts is important. Since flow control characters must be
	 * bypassed with '\' and only if the name is found, it's put to the index,
	 * name offset can be set here (to test after 6.12.2014). Programmer 
	 * should leave the cursor where the data ends, at '&'. */
	nameoffset = chroffset;

	//while( err<CBERROR && err!=CBSTREAMEND && index < (CBNAMEBUFLEN-3) && buferr <= CBSUCCESS ){ 
	//while( err!=CBMESSAGEEND && err!=CBMESSAGEHEADEREND && err<CBERROR && err!=CBSTREAMEND && index < (CBNAMEBUFLEN-3) && buferr <= CBSUCCESS ){ // 27.3.2016
	//while( err!=CBMESSAGEEND && err!=CBMESSAGEHEADEREND && err<CBERROR && err!=CBSTREAMEND && err!=CBSTREAMEAGAIN && index < (CBNAMEBUFLEN-3) && buferr <= CBSUCCESS ){ // 27.3.2016, 20.5.2016
	while( err!=CBMESSAGEEND && err!=CBMESSAGEHEADEREND && err<CBERROR && err!=CBSTREAMEND && err!=CBSTREAMEAGAIN && err!=CBENDOFFILE && \
		index < (CBNAMEBUFLEN-3) && buferr <= CBSUCCESS ){ // 27.3.2016, 20.5.2016, 31.10.2016

	  //cb_clog( CBLOGDEBUG, CBNEGATION, "+");

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
	      // if( chprev!=(**cbs).cf.bypass && chr==(unsigned long int)'[' && injsonquotes!=1 )
	      if( chr==(unsigned long int)'[' && injsonquotes!=1 ) // 9.11.2015
		injsonarray++;
	      //if( chprev!=(**cbs).cf.bypass && chr==(unsigned long int)']' && injsonquotes!=1 )
	      if( chr==(unsigned long int)']' && injsonquotes!=1 ) // 9.11.2015
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
	    if(       (    ( cb_is_rstart( &(*cbs), chr ) && (**cbs).cf.json!=1 ) \
	                || ( cb_is_rstart( &(*cbs), chr ) && (**cbs).cf.json==1 && injsonquotes!=1 && injsonarray<=0 ) \
	                || ( chr==(**cbs).cf.subrstart && rstartflipflop!=1 && (**cbs).cf.doubledelim==1 && (**cbs).cf.json!=1 ) \
	                || ( chr==(**cbs).cf.subrstart && (**cbs).cf.json==1 && injsonquotes!=1 && injsonarray<=0 ) \
	              ) && chprev!=(**cbs).cf.bypass ){ // count of rstarts can be greater than the count of rends
	      wasjsonrend=0;
	      wasjsonsubrend=0;

	      if( cb_is_rstart( &(*cbs), chr ) ){ // 17.8.2015
		rstartflipflop=1;
	        if( (**cbs).cf.json==1 )
		  jsonemptybracket=0; // 3.10.2015, from '{' to '}' without a name or '{' '{' '}' '}' without a name
	      }

	      if( chr==(**cbs).cf.subrstart && (**cbs).cf.json==1 ) { // 21.2.2015 JSON
		injsonquotes=0;
		injsonarray=0;
		++jsonemptybracket; // 3.10.2015, from '{' to '}' without a name or '{' '{' '}' '}' without a name
		//cb_clog( CBLOGDEBUG, CBNEGATION, "\n subrstart, ++jsonemptybracket (%i) ", jsonemptybracket );
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
	      //cb_clog( CBLOGDEBUG, CBNEGATION, "\nopenpairs=%i (rstart, subrstart)", openpairs );
	    }else if( (**cbs).cf.json==1 && (**cbs).cf.jsonsyntaxcheck==1 && \
		chr!='\"' && chr!=':' && atvalue==0 && chr!=(**cbs).cf.rend && chr!=(**cbs).cf.subrend ){
	    	  /*
	    	   * Syntax error 14.7.2017. <rend> WSP, CR, LF "name": WSP, CR, LF, json_value_first_letter
	    	   *
	    	   * Part I and II, between rend and '"' and between rstart ':' and <value>.
	     	   */
	       	  if( WSP( chr ) || CR( chr ) || LF( chr ) ){
		     ; // before value
		  }else if( json_value_first_letter( chr ) && injsonquotes>=2 ){
		     // ':' missing, start of value
		     (*(**cbs).cb).list.rd.syntaxerrorreason = CBSYNTAXERRMISSINGCOLON;
		     (*(**cbs).cb).list.rd.syntaxerrorindx = (*(**cbs).cb).index;
		     //cb_clog( CBLOGDEBUG, CBNEGATION, "\nCBSYNTAXERRMISSINGCOLON [%c]", (unsigned char) chr );
		     if( (**cbs).cf.stopatjsonsyntaxerr==1 ){
		     	ret = CBSYNTAXERROR;
		     	goto cb_set_cursor_ucs_return; 
		     }
	       	  }else if( injsonquotes>=2 || injsonquotes==0 ){
		     if( chr==(**cbs).cf.bypass ){
		 	(*(**cbs).cb).list.rd.syntaxerrorreason = CBSYNTAXERRBYPASSNOTALLOWEDHERE;
		     	(*(**cbs).cb).list.rd.syntaxerrorindx = (*(**cbs).cb).index;
			//cb_clog( CBLOGDEBUG, CBNEGATION, "\nCBSYNTAXERRBYPASSNOTALLOWEDHERE [%c]", (unsigned char) chr );
		     	if( (**cbs).cf.stopatjsonsyntaxerr==1 ){
			  ret = CBSYNTAXERROR;
		          goto cb_set_cursor_ucs_return;
			}
		     }else if( chr=='#' || chr=='/'  ){
		 	(*(**cbs).cb).list.rd.syntaxerrorreason = CBSYNTAXERRCOMMENTNOTALLOWEDHERE;
		     	(*(**cbs).cb).list.rd.syntaxerrorindx = (*(**cbs).cb).index;
			//cb_clog( CBLOGDEBUG, CBNEGATION, "\nCBSYNTAXERRCOMMENTNOTALLOWEDHERE [%c]", (unsigned char) chr );
		     	if( (**cbs).cf.stopatjsonsyntaxerr==1 ){
			  ret = CBSYNTAXERROR;
		          goto cb_set_cursor_ucs_return;
			}
		     }else{ // any chr not quote or colon
		 	(*(**cbs).cb).list.rd.syntaxerrorreason = CBSYNTAXERRCHARNOTALLOWEDHERE;
		     	(*(**cbs).cb).list.rd.syntaxerrorindx = (*(**cbs).cb).index;
			//cb_clog( CBLOGDEBUG, CBNEGATION, "\nCBSYNTAXERRCHARNOTALLOWEDHERE [%c]", (unsigned char) chr );
		     	if( (**cbs).cf.stopatjsonsyntaxerr==1 ){
			  ret = CBSYNTAXERROR;
		          goto cb_set_cursor_ucs_return;
			}
		     }
	       	  } // array should be checked at the value or in part rend -> name
	    }
 /***
 ***/
	  }else if( (**cbs).cf.searchstate==CBSTATEFUL ){
	    if( ( chprev!=(**cbs).cf.bypass && cb_is_rstart( &(*cbs), chr ) ) && atvalue!=1 ) // '=', save name, 13.4.2013, do not save when = is in value
	      tovalue=1;
	  }else{ // if( (**cbs).cf.searchstate==CBSTATELESS ){
	    if( chprev!=(**cbs).cf.bypass && cb_is_rstart( &(*cbs), chr ) ) // '=', save name, 5.4.2013
	      tovalue=1;
	  }

	  if(tovalue==1){

	  // 23.8.2016, namelist:
cb_set_cursor_match_length_ucs_matchctl_save_name:

	    atvalue=1;

	    //cb_clog( CBLOGDEBUG, CBNEGATION, "\n Going to save name from charbuf to fname." );
	    //if( charbufptr==NULL )
	    //  cb_clog( CBLOGDEBUG, CBNEGATION, "\n charbufptr is null." );

	    if( buferr==CBSUCCESS ){ 

// savenameerr

	      //buferr = cb_save_name_from_charbuf( &(*cbs), &fname, chroffset, &charbufptr, index, nameoffset); // 7.12.2013, 6.12.2014
//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_cursor_match_length_ucs_matchctl: cb_save_name_from_charbuf, chr %li.", matched_rend );
	      savenameerr = cb_save_name_from_charbuf( &(*cbs), &fname, chroffset, &charbufptr, index, nameoffset, matched_rend ); // 7.12.2013, 6.12.2014, 23.10.2015, 1.10.2021
	      //if(buferr==CBNAMEOUTOFBUF || buferr>=CBNEGATION){ cb_clog( CBLOGNOTICE, buferr, "\ncb_set_cursor_ucs: cb_save_name_from_ucs returned %i ", buferr); }
	      if(savenameerr==CBNAMEOUTOFBUF || savenameerr>=CBNEGATION){ cb_clog( CBLOGNOTICE, buferr, "\ncb_set_cursor_ucs: cb_save_name_from_ucs returned %i ", savenameerr ); }

	      //cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_cursor_match_length_ucs_matchctl: cb_save_name_from_charbuf returned %i .", savenameerr );

	      if( fname==NULL )
	        cb_clog( CBLOGDEBUG, CBNEGATION, "\n fname is null." );

	      //if(buferr!=CBNAMEOUTOFBUF && fname!=NULL ){ // cb_save_name_from_charbuf returns CBNAMEOUTOFBUF if buffer is full
	      //if(buferr!=CBNAMEOUTOFBUF && fname!=NULL && buferr<CBERROR ){ // cb_save_name_from_charbuf returns CBNAMEOUTOFBUF if buffer is full, 23.10.2015
	      if(savenameerr!=CBNAMEOUTOFBUF && fname!=NULL && savenameerr<CBERROR ){ // cb_save_name_from_charbuf returns CBNAMEOUTOFBUF if buffer is full, 23.10.2015

		//cb_clog( CBLOGDEBUG, CBNEGATION, "\n Openpairs=%i, ocoffset=%i, injsonquotes=%i, injsonarray=%i,index=%li, name [", openpairs, ocoffset, injsonquotes, injsonarray, (*(**cbs).cb).index );
		//cb_print_ucs_chrbuf( CBLOGDEBUG, &(*fname).namebuf, (*fname).namelen, (*fname).namelen);
		//cb_clog( CBLOGDEBUG, CBNEGATION, "] ");
		//if( (**cbs).cf.json==1 )
		//  cb_clog( CBLOGDEBUG, CBNEGATION, ", going to cb_check_json_name.");

		if( (**cbs).cf.json!=1 || ( (**cbs).cf.json==1 && ( ( injsonquotes>=2 && (**cbs).cf.jsonnamecheck!=1 ) || \
				( injsonquotes>=2 && (**cbs).cf.jsonnamecheck==1 && \
				cb_check_json_name( &charbufptr, &index, (**cbs).cf.jsonremovebypassfromcontent )<CBNOTJSON ) ) ) ){ // 19.2.2015, check json name with quotes 27.8.2015
	          //buferr = cb_put_name(&(*cbs), &fname, openpairs, ocoffset); // (last in list), if name is comparable, saves name and offset
		  if( (**cbs).cf.searchnameonly!=1 )
	            savenameerr = cb_put_name( &(*cbs), &fname, openpairs, previousopenpairs ); // (last in list), if name is comparable, saves name and offset
// CB_PUT_NAME
		  //cb_clog( CBLOGDEBUG, CBNEGATION, "\n Put name |");
		  //cb_print_ucs_chrbuf( CBLOGDEBUG, &(*fname).namebuf, (*fname).namelen, (*fname).namelen);
		  //cb_clog( CBLOGDEBUG, CBNEGATION, "|, openpairs=%i, injsonquotes=%i ", openpairs, injsonquotes );
	
		  injsonquotes=0;

	          if( savenameerr==CBADDEDNAME || savenameerr==CBADDEDLEAF || savenameerr==CBADDEDNEXTTOLEAF){
	            buferr = CBSUCCESS;
	          }else{
		    buferr = savenameerr; // 23.10.2015
		  }
		  if( (**cbs).cf.namelist==1 && lastinnamelist==1 ){ // 23.8.2016, namelist
		  	goto cb_set_cursor_ucs_return;
		  }
	        }
	      }
	      if( (**cbs).cf.searchstate!=CBSTATETREE && (**cbs).cf.searchstate!=CBSTATETOPOLOGY ){
	        if( (**cbs).cf.leadnames==1 ){             // From '=' to '=' and from '&' to '=',
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
	          cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_cursor_ucs: error, openpairs<(ocoffset+1) || openpairs<1, %i<%i.", openpairs, (ocoffset+1));
	      }
	    }
// savenameerr
	    //if(buferr==CBNAMEOUTOFBUF || buferr>=CBNEGATION){
	    if(savenameerr==CBNAMEOUTOFBUF || savenameerr>=CBNEGATION){
	      if(savenameerr>=CBERROR)
	        cb_clog( CBLOGERR, savenameerr, "\ncb_set_cursor_ucs: cb_put_name, error %i. ", savenameerr);
	      else
	        cb_clog( CBLOGNOTICE, savenameerr, "\ncb_set_cursor_ucs: cb_put_name returned %i. ", savenameerr);
	    }

	    if(err==CBSTREAM){ // Set length of namepair to indicate out of buffer.
	      cb_remove_name_from_stream( &(*cbs) ); // also leaves
	      cb_clog( CBLOGNOTICE, err, "\ncb_set_cursor: name was out of memory buffer.\n");
	    }
	    /*
	     * Name (openpairs==1)
	     */

            // If unfolding is used and last read resulted an already read character (one character maximum) 
            // and index is zero, do not save the empty name.

// savenameerr
// buferr1 ja buferr2 ?? , && buferr<CBERROR
	    if( fname!=NULL ){ // 22.10.2015
	      if( openpairs<=1 && ocoffset==0 ){ // 2.12.2013, 13.12.2013, name1 from name1.name2.name3
	        //cb_clog( CBLOGDEBUG, CBNEGATION, "\nNAME COMPARE");
		if( (*fname).namebuf!=NULL ) // 18.11.2015
	          cis = cb_compare( &(*cbs), &(*ucsname), *namelength, &(*fname).namebuf, (*fname).namelen, &(*mctl) ); // 23.11.2013, 11.3.2014, 23.3.2014
// CB_COMPARE 1.
	        if( cis == CBMATCH ){ // 9.11.2013
	          (*(**cbs).cb).index = (*(**cbs).cb).contentlen - (**cbs).ahd.bytesahead; // cursor at rstart, 6.9.2013 (this line can be removed)
	          if( (*(**cbs).cb).list.last != NULL ){ // matchcount, this is first match, matchcount becomes 1, 25.8.2013
	            (*(*(**cbs).cb).list.last).matchcount++;
		    (*(*(**cbs).cb).list.last).group = (*(**cbs).cb).list.currentgroup; // 11.11.2016
		  }
	          if(err==CBSTREAM){
	            //cb_clog( CBLOGDEBUG, CBNEGATION, "\nName found from stream.");
	            ret = CBSTREAM; // cursor set, preferably the first time (remember to use cb_remove_name_from_stream)
	            goto cb_set_cursor_ucs_return;
	          }else if(err==CBFILESTREAM){
	            //cb_clog( CBLOGDEBUG, CBNEGATION, "\nName found from file stream (seekable).");
	            ret = CBFILESTREAM; // cursor set, preferably the first time
	            goto cb_set_cursor_ucs_return;
	          }else{
	            //cb_clog( CBLOGDEBUG, CBNEGATION, "\nName found from buffer.");
	            ret = CBSUCCESS; // cursor set
	            goto cb_set_cursor_ucs_return;
	          }
	        }
	      }else if( ocoffset!=0 && openpairs==(ocoffset+1) ){ // 13.12.2013, order ocoffset name from name1.name2.name3
	        /*
	         * Leaf (openpairs==(ocoffset+1) if ocoffset > 0)
	         */
	        //cb_clog( CBLOGDEBUG, CBNEGATION, "\nLEAF COMPARE");
	        cis = cb_compare( &(*cbs), &(*ucsname), *namelength, &(*fname).namebuf, (*fname).namelen, &(*mctl) ); // 23.11.2013, 11.3.2014, 23.3.2014
// CB_COMPARE 2.
	        if( cis == CBMATCH ){ // 9.11.2013
	          (*(**cbs).cb).index = (*(**cbs).cb).contentlen - (**cbs).ahd.bytesahead; // cursor at rstart, 6.9.2013 (this line can be removed)
	          if( (*(**cbs).cb).list.currentleaf != NULL ){ // matchcount, this is first match, matchcount becomes 1, 25.8.2013
	            (*(*(**cbs).cb).list.currentleaf).matchcount++;
		    (*(*(**cbs).cb).list.currentleaf).group = (*(**cbs).cb).list.currentgroup; // 11.11.2016
		  }
	          if(err==CBSTREAM){
	            //cb_clog( CBLOGDEBUG, CBNEGATION, "\nLeaf found from stream.");
	            ret = CBSTREAM; // cursor set, preferably the first time (remember to use cb_remove_name_from_stream)
	            goto cb_set_cursor_ucs_return;
	          }else if(err==CBFILESTREAM){
	            //cb_clog( CBLOGDEBUG, CBNEGATION, "\nLeaf found from stream.");
	            ret = CBFILESTREAM; // cursor set, preferably the first time (remember to use cb_remove_name_from_stream)
	            goto cb_set_cursor_ucs_return;
	          }else{
	            //cb_clog( CBLOGDEBUG, CBNEGATION, "\nLeaf found from buffer.");
	            ret = CBSUCCESS; // cursor set
	            goto cb_set_cursor_ucs_return;
	          }
	        }
	      }else{
	        /*
	         * Mismatch */
	        // Debug (or verbose error messages)
	        //cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_cursor_ucs: error: not leaf nor name, reseting charbuf, namecount %ld.", (*(**cbs).cb).list.namecount );
	      }
	      /*
	       * 'fname' was copied to a new allocated name in the list and used the last time. Time to free fname. 18.11.2015 / 2. */
	       (*fname).next=NULL; (*fname).leaf=NULL; cb_free_name( &fname, &freecount ); fname=NULL;
	    }
	    /*
	     * No match. Reset charbuf to search next name.
	     */
	    if( (**cbs).cf.searchstate==CBSTATETREE || (**cbs).cf.leadnames==1 ){
	      goto cb_set_cursor_reset_name_index; // 15.12.2013
	    }else{
	      buferr = cb_put_ucs_chr(chr, &charbufptr, &index, CBNAMEBUFLEN); // write '='
	    }

	  //}else if( chprev!=(**cbs).cf.bypass && ( ( chr==(**cbs).cf.rend && (**cbs).cf.json!=1 ) || 
	  //                         ( chr==(**cbs).cf.rend && (**cbs).cf.json==1 && injsonquotes!=1 && injsonarray<=0 ) || 
	  //                         ( (**cbs).cf.json!=1 && chr==(**cbs).cf.subrend && rstartflipflop!=1 && (**cbs).cf.doubledelim==1 ) || 
	  //                         ( (**cbs).cf.json==1 && injsonquotes!=1 && injsonarray<=0 && chr==(**cbs).cf.subrend ) ) ){
	  }else if( chprev!=(**cbs).cf.bypass && ( ( cb_is_rend( &(*cbs), chr, &matched_rend ) && (**cbs).cf.json!=1 ) || \
	                           ( cb_is_rend( &(*cbs), chr, &matched_rend ) && (**cbs).cf.json==1 && injsonquotes!=1 && injsonarray<=0 ) || \
	                           ( (**cbs).cf.json!=1 && chr==(**cbs).cf.subrend && rstartflipflop!=1 && (**cbs).cf.doubledelim==1 ) || \
	                           ( (**cbs).cf.json==1 && injsonquotes!=1 && injsonarray<=0 && chr==(**cbs).cf.subrend ) ) ){

	      if( (**cbs).cf.json!=1 || ( ( jsonemptybracket<=0 && chr==(**cbs).cf.subrend ) || cb_is_rend( &(*cbs), chr, &matched_rend ) ) ) // '}' ',' json-condition added 3.10.2015, kolmas
	        cb_increase_terminalcount( &(*cbs) );

	      if( cb_is_rend( &(*cbs), chr, &matched_rend ) ){
		rstartflipflop=0;
		if( (**cbs).cf.json==1 )
	          jsonemptybracket=0;
	      }

// Moved to the end of this block 6.11.2015
/**
	      if( chr==(**cbs).cf.subrend && (**cbs).cf.json==1 ){
	        --jsonemptybracket;
	        if(jsonemptybracket<0)
		   jsonemptybracket=0; // 3.10.2015, last
	      }
 **/

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
// XVIX
			if(jsonemptybracket<=0){ // 6.11.2015
			  //cb_clog( CBLOGDEBUG, CBNEGATION, "\n jsonemptybracket!=1 (%i), --openpairs", jsonemptybracket );
			  --openpairs;
			}//else
			//  cb_clog( CBLOGDEBUG, CBNEGATION, "\n jsonemptybracket>=1 (%i)", jsonemptybracket );
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
// XVIX
	          	--openpairs; // reader must read similarly, with openpairs count or otherwice
			cb_update_previous_length( &(*cbs), chroffset, openpairs, previousopenpairs); // 28.9.2015
		  }
                  wasjsonsubrend=0;
		}
	        //cb_clog( CBLOGDEBUG, CBNEGATION, "\nopenpairs=%i (rend, subrend)", openpairs );
	      }
// Move 6.11.2015
	      if( chr==(**cbs).cf.subrend && (**cbs).cf.json==1 ){
	        --jsonemptybracket;
	        if(jsonemptybracket<0)
		   jsonemptybracket=0; // 3.10.2015, last
	      }
// /Move 6.11.2015
	      atvalue=0;
	      if(fname!=NULL){ 
		(*fname).next=NULL; (*fname).leaf=NULL; // 15.11.2015
	        cb_free_name(&fname, &freecount);
	      }
	      fname=NULL; // allocate_name is at put_leaf and put_name and at cb_save_name_from_charbuf (first: more accurate name, second: shorter length in memory)
// JSON: XX
// conf has the same like: (**cbs).cf.json==1 && (openpairs<0 || openpairs<(ocoffset-1) 
	      if( ocoffset>0 && (**cbs).cf.json==1 && injsonquotes!=1 && openpairs<previousopenpairs && openpairs==0 ){ // TEST, 23.8.2015, 3.10.2015, toinen yritys. 8.10.2015
		/*
		 * JSON special: concecutive '}' and ',' */
		//cb_clog( CBLOGDEBUG, CBNEGATION, "\nocoffset %i, openpairs<previousopenpairs && openpairs==%i, %i<%i. Returning VALUEEND.", ocoffset, openpairs, openpairs, previousopenpairs ); // Test
		//cb_clog( CBLOGDEBUG, CBNEGATION, " PREVCHR %c.", (char) (*(**cbs).cb).list.rd.lastchr );
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
	          //cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_cursor_...: set currentleaf to null, openpairs=%i", openpairs );
	          goto cb_set_cursor_ucs_return;
		}
	      }
 	      goto cb_set_cursor_reset_name_index;
	  }else if(chprev==(**cbs).cf.bypass && chr==(**cbs).cf.bypass){
	      /*
	       * change \\ to one '\' 
	       */

	      chr=(**cbs).cf.bypass+1; // any char not '\'

	  }else if(     ( \
			  ( (**cbs).cf.json==0 && (openpairs<0 || openpairs<ocoffset ) && openpairs<previousopenpairs ) && \
		          ( (**cbs).cf.searchstate==CBSTATETREE || (**cbs).cf.searchstate==CBSTATETOPOLOGY ) \
		        ) || ( \
		          ( (**cbs).cf.json==1 && (openpairs<0 || openpairs<(ocoffset-1) ) && openpairs<previousopenpairs ) && \
		          ( (**cbs).cf.searchstate==CBSTATETREE || (**cbs).cf.searchstate==CBSTATETOPOLOGY) \
			) ){ // 27.2.2015: test JSON: TEST WAS: FAIL ( '}'+',' should be -1, not -2 ) , STILL HERE, AN ERROR

// CONF: XX

// 8.10.2015: added: openpairs<previousopenpairs , openpairs must go up until it can go down.

	      /*
	       * When reading inner name-value -pairs, read of outer value ended.
	       */
              //cb_clog( CBLOGDEBUG, CBNEGATION, "\nValue read, openpairs %i ocoffset %i.", openpairs, ocoffset); // debug
	      /*
	       * In test 22.8.2015, update length here and second time at cb_put_name or 
	       * cb_put_leaf. Second time the value is updated to the final value (with writable 
	       * block sizes in mind). Writable block sizes are not done well yet here (at 22.8.2015). */
	      cb_update_previous_length( &(*cbs), chroffset, openpairs, previousopenpairs); // 22.8.2015
	      if( ocoffset>0 && (**cbs).cf.findleaffromallnames!=1 ){ // if conditions added 3.10.2015 (search was a leaf search and search is not from all names)
	        ret = CBVALUEEND; // 22.8.2015
	        // testattu, ei toimi oikein, toterminal > level: (*(**cbs).cb).list.rd.lastreadchrendedtovalue=1; // 7.10.2015, kohta XX (VIELA TESTATTAVANA KLO 11:50)
	        //cb_clog( CBLOGDEBUG, CBNEGATION, "\nPLACE, json=%i, PREVCHR %c.", (**cbs).cf.json, (char) (*(**cbs).cb).list.rd.lastchr );
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
          if( (**cbs).cf.stopatheaderend==1 ){
	    //cb_clog( CBLOGDEBUG, CBNEGATION, "\n[0x%.2x,0x%.2x,0x%.2x,0x%.2x]", (unsigned int) ch3prev, (unsigned int) ch2prev, (unsigned int) chprev, (unsigned int) chr );
            if( ch3prev==0x0D && ch2prev==0x0A && chprev==0x0D && chr==0x0A ){ // cr lf x 2
	      //cb_clog( CBLOGDEBUG, CBNEGATION, "\n[ ***\n  *** HTTP HEADERS END ***\n*** ]"); // 0x%.2x]", (unsigned int) ch3prev, (unsigned int) ch2prev, (unsigned int) chprev, (unsigned int) chr );
              if( (*(**cbs).cb).headeroffset < 0 ) // 26.3.2016
	        if( chroffset < 0x7FFFFFFF) // integer size - 1 
                  (*(**cbs).cb).headeroffset = (int) chroffset; // 1.9.2013, offset set at last new line character, 24.10.2018
	      ret = CBMESSAGEHEADEREND;
	      cb_update_previous_length( &(*cbs), chroffset, openpairs, previousopenpairs); // 19.7.2016 STILL IN TEST (branch 'transfer')
	      goto cb_set_cursor_ucs_return;
            }
	  }

          /*
           * Stop at the message payload end set outside of the library with cb_set_message_end . */
	  if( cb_test_message_end( &(*cbs) ) == CBMESSAGEEND ){ // 26.3.2016, 28.3.2016, 8.5.2016
		ret = CBMESSAGEEND; // disregard negations and errors if header end
		goto cb_set_cursor_ucs_return;
	  }


	  ch3prev = ch2prev; ch2prev = chprev; chprev = chr;
	  /*
	   * cb_search_get_chr returns maximum bufsize offset */
// 2
	  err = cb_search_get_chr( &(*cbs), &chr, &chroffset); // 1.9.2013

	  if( err==CBERRTIMEOUT ){
	  	cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_cursor: cb_search_get_chr, CBERRTIMEOUT (2)" );
	  	cb_flush_log();
	  }else if( err!=CBSUCCESS ){
	  	//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_cursor: cb_search_get_chr %i (2)", err );
	  	//cb_flush_log();
	  }
	  if( err==CBERRTIMEOUT ){
	    ret = CBERRTIMEOUT; // 18.7.2021 TEST
	    goto cb_set_cursor_ucs_return;
	  }

	  //cb_clog( CBLOGDEBUG, CBNEGATION, "%c|", (unsigned char) chr ); // BEST
	  //cb_clog( CBLOGDEBUG, CBNEGATION, "\nring buffer counters:");
	  //cb_fifo_print_counters( &(**cbs).ahd, CBLOGDEBUG );
          //cb_clog( CBLOGDEBUG, CBNEGATION, "\ndata [");
	  //cb_fifo_print_buffer( &(**cbs).ahd, CBLOGDEBUG );
          //cb_clog( CBLOGDEBUG, CBNEGATION, "]");

	}

	/**
	 **/
	if( index >= (CBNAMEBUFLEN-3) && (**cbs).cf.searchstate!=CBSTATELESS ){
		cb_clog( CBLOGERR, CBBUFFULL, "\ncb_set_cursor_match_length_ucs_matchctl: namebuffer is full, error %i.", CBBUFFULL );
		err = CBBUFFULL;
		ret = CBBUFFULL; // 11.12.2016
	}

	//if(err==CBSTREAMEND) // 27.12.2014
	//  ret = CBSTREAMEND;

cb_set_cursor_ucs_return:

//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_cursor_ucs_return");

	//cb_clog( CBLOGDEBUG, CBNEGATION, "\nring buffer data first %i last %i {", (**cbs).ahd.first, (**cbs).ahd.last );
	//cb_fifo_print_buffer( &(**cbs).ahd, CBLOGDEBUG );
	//cb_clog( CBLOGDEBUG, CBNEGATION, "} ");

	if( fname!=NULL ){
		(*fname).next=NULL; (*fname).leaf=NULL; // 15.11.2015
        	cb_free_name(&fname, &freecount); // fname on kaksi kertaa, put_name kopioi sen uudelleen
	}
	fname = NULL;
	if( ret==CBSUCCESS || ret==CBSTREAM || ret==CBFILESTREAM || ret==CBMATCH || ret==CBMATCHLENGTH ){ // match* is useless, returns always stream or success
	  if( (**cbs).cb!=NULL && (*(**cbs).cb).list.current!=NULL ){
	    if( (*(*(**cbs).cb).list.current).lasttimeused == -1 )
	      (*(*(**cbs).cb).list.current).lasttimeused = (*(*(**cbs).cb).list.current).firsttimefound; // first time found
	    else
	      (*(*(**cbs).cb).list.current).lasttimeused = (signed long int) time(NULL); // last time used in seconds
	  }
	}

	// cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_cursor_match_length_ucs_matchctl: returning %i (err %i).", ret, err );

//if( (**cbs).cf.namelist==1 && ( err==CBSTREAMEND || err==CBMESSAGEEND || err==CBMESSAGEHEADEREND ) )
// cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_cursor_match_length_ucs_matchctl: CALLING NAMELIST %i ATVALUE %i ERR %i LASTINNAMELIST %i.", (**cbs).cf.namelist, atvalue, err, lastinnamelist );

	//TEST 5.10.2017: if( (**cbs).cf.namelist==1 && ( err==CBSTREAMEND || err==CBMESSAGEEND || err==CBMESSAGEHEADEREND ) && lastinnamelist==0 ){
	// Result: solved the case, caused other problems, must be fixed elsewhere ("name1, name2, name3" <- problem at end, name3 is not saved in the list)
	if( (**cbs).cf.namelist==1 && lastinnamelist==0 && \
		( atvalue==0 || ( atvalue==1 && (**cbs).cf.wordlist==0 ) ) && \
			( err==CBSTREAMEND || err==CBMESSAGEEND || err==CBMESSAGEHEADEREND ) ){

		/* 7.10.2017: cf.wordlist was added by testing: "name1, name2, name3" | tests/test_cblist
		 *            and: "\$name1, \$name2, \$name3" | tests/test_cbwords  */

		/*
		 * As mentioned before, wordlist (rstart and rend are backwards) works only with CBSTATEFUL.
		 * This includes additionally the variable atvalue (in test 29.9.2016). 
		 */
		lastinnamelist = 1;
		if( index > 0 )
			goto cb_set_cursor_match_length_ucs_matchctl_save_name;
	}

	if( (**cbs).cf.stopateof==1 ){ // 31.10.2016
		if( ( ret>=CBNEGATION && ret!=CBVALUEEND && ret!=CBMESSAGEEND && ret!=CBMESSAGEHEADEREND ) && err==CBENDOFFILE ){
			//cb_clog( 22, CBSUCCESS, "\nEOF CBENDOFFILE" );
			(*(**cbs).cb).list.rd.stopped = 1;
			ret = CBENDOFFILE;
		}
	}

	return ret;

        return CBNOTFOUND;
}

/*
 * Allocates fname */
int cb_save_name_from_charbuf(CBFILE **cbs, cb_name **fname, signed long int offset, unsigned char **charbuf, signed int index, signed long int nameoffset, unsigned long int matched_rend ){
	unsigned long int cmp=0x61;
	int buferr=CBSUCCESS, err=CBSUCCESS;
	int indx = 0;
	char atname=0, injsonquotes=0;

	if( cbs==NULL || *cbs==NULL || fname==NULL || charbuf==NULL || *charbuf==NULL ){
	  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_save_name_from_charbuf: parameter was NULL, error %i.", CBERRALLOC );
	  return CBERRALLOC;
	}

	if(*fname!=NULL){ // 18.11.2015
		free( *fname ); *fname=NULL;
	}
	err = cb_allocate_name( &(*fname), (index+1) ); // moved here 7.12.2013 ( +1 is one over the needed size )

	if(err!=CBSUCCESS){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_save_name_from_charbuf: name allocation, error %i.", err);  return err; } // 30.8.2013

	(**fname).namelen = index;
	(**fname).offset = offset;
	(**fname).nameoffset = nameoffset; // 6.12.2014
//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_save_name_from_charbuf: setting matched_rend chr %li.", matched_rend );
	(**fname).matched_rend = matched_rend; // 1.10.2021
//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_save_name_from_charbuf: set matched_rend %li.", (**fname).matched_rend );

/***
	cb_clog( CBLOGDEBUG, CBNEGATION, "\n cb_save_name_from_charbuf: name [");
	cb_print_ucs_chrbuf( CBLOGDEBUG, &(*charbuf), index, CBNAMEBUFLEN);
	cb_clog( CBLOGDEBUG, CBNEGATION, "] matched_rend %li, cbfile id %i", matched_rend, (**cbs).id );
 ***/
	//cb_clog( CBLOGDEBUG, CBNEGATION, "] index=%i, (**fname).buflen=%i, removeeof=%i, cstart:[%c] cend:[%c] ", index, (**fname).buflen, (**cbs).cf.removeeof, (char) (**cbs).cf.cstart, (char) (**cbs).cf.cend);

        if(index<(**fname).buflen){
          (**fname).namelen=0; // 6.4.2013
	  while(indx<index && buferr==CBSUCCESS){ // 4 bytes (UCS word)
cb_save_name_ucs_removals: 
//cb_clog( CBLOGDEBUG, CBNEGATION, "-");
            buferr = cb_get_ucs_chr( &cmp, &(*charbuf), &indx, CBNAMEBUFLEN); // 30.8.2013
            if( buferr==CBSUCCESS ){
              if( (**cbs).cf.removecommentsinname==1 && cmp==(**cbs).cf.cstart ){   // Comment (+ in case of JSON, cstart and cend is set to '[' and ']'. Reading of array to avoid extra commas.)
                while( indx<index && cmp!=(**cbs).cf.cend && buferr==CBSUCCESS ){   // 2.9.2013
//cb_clog( CBLOGDEBUG, CBNEGATION, "*");
                  buferr = cb_get_ucs_chr( &cmp, &(*charbuf), &indx, CBNAMEBUFLEN); // 30.8.2013
                } // cf.removecommentsinname, 8.1.2015
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
	      if( (**cbs).cf.removeeof==1 ){ // Removes EOF from the start of the name, 26.8.2016
                if( indx<index && cmp==( unsigned long int )0x00FF && buferr==CBSUCCESS){ // EOF (-1) in a character is 0x00FF
	          goto cb_save_name_ucs_removals;
	        }
	      }
	      if( (**cbs).cf.removesemicolon==1 ){ // 20.3.2016, not yet tested
                if( indx<index && ( cmp == (unsigned long int) 0x003B ) && buferr==CBSUCCESS ){ // ';'
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
		      if( ! ( indx==index && (**cbs).cf.removesemicolon==1 && ( cmp==(unsigned long int) 0x003B ) ) ){ // special last semicolon, 20.3.2016
                        buferr = cb_put_ucs_chr( cmp, &(**fname).namebuf, &(**fname).namelen, (**fname).buflen );
	                atname=1;
		      }
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

	//cb_clog( CBLOGDEBUG, CBNEGATION, "\n cb_save_name_from_charbuf: WROTE name [");
	//cb_print_ucs_chrbuf( CBLOGDEBUG, &(**fname).namebuf, (**fname).namelen, (**fname).buflen );
	//cb_clog( CBLOGDEBUG, CBNEGATION, "] index=%i, (**fname).buflen=%i cstart:[%c] cend:[%c] ", index, (**fname).buflen, (char) (**cbs).cf.cstart, (char) (**cbs).cf.cend);

	if(err>=CBNEGATION && err!=CBNAMEOUTOFBUF){ // 23.10.2015
	  cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_save_name_from_charbuf: name allocation, returning %i.", err);
	  if(err>=CBERROR)
	    cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_save_name_from_charbuf: name allocation, error %i.", err);
	}
	return err;
}

int  cb_check_json_name( unsigned char **ucsname, signed int *namelength, char bypassremoved ){
	signed int from=0;
	if( ucsname==NULL || *ucsname==NULL || namelength==NULL ){ cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_check_json_name: name was null."); return CBERRALLOC; }
	return cb_check_json_string( &(*ucsname), *namelength, &from, bypassremoved );
}

int  cb_automatic_encoding_detection(CBFILE **cbs){
	signed int enctest=CBDEFAULTENCODING;
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
//cb_clog( CBLOGDEBUG, CBNEGATION, "\nCALLING REMOVE_NAME_FROM_STREAM" );
	if( (**cbs).cf.type!=CBCFGSEEKABLEFILE && (**cbs).cf.type!=CBCFGFILE ){
		if( (*(**cbs).cb).buflen<2147483647 ) // 24.10.2018
			(*(*(**cbs).cb).list.current).length = (int) (*(**cbs).cb).buflen;
		else
			(*(*(**cbs).cb).list.current).length = 2147483647; // 24.10.2018, integer maximum size
		//24.10.2018: (*(*(**cbs).cb).list.current).length = (*(**cbs).cb).buflen;
	}
	return CBSUCCESS;
}

/*
 * Sets name lists names group as -1 using namelist search
 * matchctl 1, 25.9.2021 . Does not search leaves. */
signed int  cb_set_groupless( CBFILE *cbf, unsigned char *name, signed int namelen ){
        signed int err = CBSUCCESS;
        cb_match m;
	cb_fill_empty_matchctl( &m );
	m.unique_namelist_search = 1;
        if( cbf==NULL || (*cbf).cb==NULL || name==NULL ) return CBERRALLOC;

        /*
         * Search the namelist. */
        err = cb_set_to_name( &cbf, &name, namelen, &m );
/*** Debug 25.12.2021
cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_groupless: cb_set_to_name '" );
if( name!=NULL && namelen>0 )
  cb_print_ucs_chrbuf( CBLOGDEBUG, &name, namelen, namelen );
cb_clog( CBLOGDEBUG, CBNEGATION, "', %i", err );
if( cbf!=NULL )
  cb_print_names( &cbf, CBLOGDEBUG );
 ***/
        if( err>=CBERROR || err==CBNOTFOUNDLEAVESEXIST || err==CBNOTFOUND ){
/*** Debug 25.12.2021
cb_clog( CBLOGDEBUG, CBNEGATION, ", names:" );
if( cbf!=NULL )
  cb_print_names( &cbf, CBLOGDEBUG );
 ***/
		return err;
	}
        if( (*(*cbf).cb).list.current==NULL ){
//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_groupless: cb_set_to_name, CBEMPTY." );
		return CBEMPTY;
	}

        /*
         * Set as groupless. */
        (*(*(*cbf).cb).list.current).group = -1;

	/*
	 * Set as not found, 12.12.2021 16:18 this one actually works here
	 * and is really needed. */
	(*(*(*cbf).cb).list.current).matchcount = 0;
//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_set_groupless: cb_set_to_name, set group as -1." );
        return CBSUCCESS;
}
/*
 * Sets current name as not found using namelist search
 * matchctl 1, 25.9.2021 . Does not search leaves. */
signed int  cb_set_unread( CBFILE *cbf, unsigned char *name, signed int namelen ){
        int err = CBSUCCESS;
        cb_match m; m.matchctl = 1; m.resmcount = 0; m.errcode = 0; m.erroffset = 0;
        if( cbf==NULL || (*cbf).cb==NULL || name==NULL ) return CBERRALLOC;
        /*
         * Search the namelist. */
        err = cb_set_to_name( &cbf, &name, namelen, &m );
        if( err>=CBNEGATION ) return err;
        if( (*(*cbf).cb).list.current==NULL ) return CBEMPTY;
        /*
         * Set as unread. */
        (*(*(*cbf).cb).list.current).matchcount = 0;
        (*(*(*cbf).cb).list.current).lasttimeused = -1;
        return CBSUCCESS;
}

