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
#include <string.h> // strsep
#include "../include/cb_buffer.h"
#include "../include/cb_json.h"
#include "../include/cb_read.h"

int  cb_subfunction_get_currentleaf_content( CBFILE **cbf, unsigned char **ucscontent, int *clength, char allocate, int maxlen );
int  cb_subfunction_get_current_content( CBFILE **cbf, unsigned char **ucscontent, int *clength, int maxlen, char allocate );
int  cb_copy_content_internal( CBFILE **cbf, cb_name **cn, unsigned char **ucscontent, int *clength, int maxlen );
int  cb_get_current_name_subfunction(CBFILE **cbs, unsigned char **ucsname, int *namelength, char leaf );

/*
 * 1 or 4 -byte functions.
 */
int  cb_find_every_name(CBFILE **cbs){
	int err = CBSUCCESS; 
	unsigned char *name = NULL;
	unsigned char  chrs[2]  = { 0x20, '\0' };
	int namelength = 0;
	unsigned char searchmethod=0;
	if( cbs==NULL || *cbs==NULL )
	  return CBERRALLOC;
	name = &chrs[0];
	searchmethod = (**cbs).cf.searchmethod;
	(**cbs).cf.searchmethod = CBSEARCHNEXTNAMES;
	//err = cb_set_cursor_match_length_ucs( &(*cbs), &name, &namelength, 1, -1 ); // no match
	err = cb_set_cursor_match_length_ucs( &(*cbs), &name, &namelength, 0, -1 ); // no match, 24.10.2015
	// cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_find_every_name: cb_set_cursor_match_length_ucs returned %i.", err);
	(**cbs).cf.searchmethod = searchmethod;
	return err;
}

/*
 * 4-byte functions. Allocates ucsname.
 */

int  cb_get_currentleaf_name(CBFILE **cbs, unsigned char **ucsname, int *namelength ){
	int err = CBSUCCESS;
	unsigned char *ptr = NULL;
	if( cbs==NULL || *cbs==NULL || ucsname==NULL || namelength==NULL ){
		cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_get_current_name: parameter was null.");
		return CBERRALLOC;
	}
	err = cb_get_current_name_subfunction( &(*cbs), &ptr, &(*namelength), 1);
	*ucsname = &(*ptr); // 11.7.2016
	return err;
}
int  cb_get_current_name(CBFILE **cbs, unsigned char **ucsname, int *namelength ){
	int err = CBSUCCESS;
	unsigned char *ptr = NULL;
	if( cbs==NULL || *cbs==NULL || ucsname==NULL || namelength==NULL ){
		cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_get_current_name: parameter was null.");
		return CBERRALLOC;
	}
	err = cb_get_current_name_subfunction( &(*cbs), &ptr, &(*namelength), 0);
	*ucsname = &(*ptr); // 11.7.2016
	return err;
}
int  cb_get_current_name_subfunction(CBFILE **cbs, unsigned char **ucsname, int *namelength, char leaf ){
	/*
	 * Allocate and copy current name to new ucsname */
	int ret = CBSUCCESS, indx=0;
	if( cbs==NULL || *cbs==NULL || ucsname==NULL || namelength==NULL ){
		cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_get_current_name: parameter was null.");
		return CBERRALLOC;
	}

	if( (**cbs).cb!=NULL && ( ( (*(**cbs).cb).list.current!=NULL && leaf!=1 ) || ( (*(**cbs).cb).list.currentleaf!=NULL && leaf==1 ) ) ){
	  //if(ucsname==NULL)
	  //  ucsname = (unsigned char**) malloc( sizeof( int ) ); // pointer size
	  if( *ucsname!=NULL )
	  	cb_log( &(*cbs), CBLOGDEBUG, CBNEGATION, "\ncb_get_current_name: debug, cb_get_current_name_ucs: *ucsname was not NULL.");
	  if(leaf!=1){
	    *ucsname = (unsigned char*) malloc( sizeof(unsigned char)*( (unsigned int) (*(*(**cbs).cb).list.current).namelen+1 ) );
	  }else{
	    *ucsname = (unsigned char*) malloc( sizeof(unsigned char)*( (unsigned int) (*(*(**cbs).cb).list.currentleaf).namelen+1 ) );
	  }
	  if( ucsname==NULL ) { 
		cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_get_current_name: malloc returned null.");
		return CBERRALLOC; 
	  }
	  if( leaf!=1 ){
	    (*ucsname)[(*(*(**cbs).cb).list.current).namelen] = '\0';
	    for( indx=0; indx<(*(*(**cbs).cb).list.current).namelen ; ++indx)
	      (*ucsname)[indx] = (*(*(**cbs).cb).list.current).namebuf[indx];
	    (*ucsname)[ (*(*(**cbs).cb).list.current).namelen ] = '\0'; // 2.7.2016
	    if(namelength==NULL)
	      namelength = (int*) malloc( sizeof( int ) ); // int 
	    if(namelength==NULL) { return CBERRALLOC; }
 	    *namelength = (*(*(**cbs).cb).list.current).namelen;
	  }else{
	    (*ucsname)[(*(*(**cbs).cb).list.currentleaf).namelen] = '\0';
	    for( indx=0; indx<(*(*(**cbs).cb).list.currentleaf).namelen ; ++indx)
	      (*ucsname)[indx] = (*(*(**cbs).cb).list.currentleaf).namebuf[indx];
	    (*ucsname)[ (*(*(**cbs).cb).list.currentleaf).namelen ] = '\0'; // 2.7.2016
	    if(namelength==NULL)
	      namelength = (int*) malloc( sizeof( int ) ); // int 
	    if(namelength==NULL) { return CBERRALLOC; }
 	    *namelength = (*(*(**cbs).cb).list.currentleaf).namelen;
	  }
	}else{
	  ret = CBERRALLOC; 
	}
	if( (**cbs).cb!=NULL && (*(**cbs).cb).list.current==NULL && leaf!=1 )
	   return CBNOTFOUND;
	if( (**cbs).cb!=NULL && (*(**cbs).cb).list.currentleaf==NULL && leaf==1 )
	   return CBNOTFOUND;
	else if( (**cbs).cb==NULL )
	   cb_log( &(*cbs), CBLOGDEBUG, CBNEGATION, "\ncb_get_current_name: (**cbs).cb was null. ");
        if( ucsname==NULL || *ucsname==NULL ){ ret = CBERRALLOC; }
	return ret;
}

/*
 * Searched next name in list. Not leafs. */
int  cb_get_next_name_ucs(CBFILE **cbs, unsigned char **ucsname, int *namelength){
	int ret = CBSUCCESS;
	unsigned char *ptr = NULL; // 11.7.2016
	unsigned char *name = NULL;
	unsigned char  chrs[5] = { 0x00, 0x00, 0x00, 0x20, '\0' };
	int namelen = 1;
	unsigned char searchmethod=0;
	name = &chrs[0];

	if( cbs==NULL || *cbs==NULL ){	  cb_log( &(*cbs), CBLOGALERT, CBERRALLOC, "\ncb_get_next_name_ucs: cbs was null."); return CBERRALLOC; }
	if( *ucsname!=NULL ){
	  cb_log( &(*cbs), CBLOGERR, CBERRALLOC, "\ncb_get_next_name_ucs: error, *ucsname was not NULL.");
	  return CBERRALLOC;
	}

	searchmethod = (**cbs).cf.searchmethod;
	(**cbs).cf.searchmethod = CBSEARCHNEXTNAMES;
	ret = cb_set_cursor_match_length_ucs( &(*cbs), &name, &namelen, 0, 0 ); // matches first (any)
	//cb_clog( CBLOGDEBUG, ret, "\ncb_get_next_name_ucs: cb_set_cursor_match_length_ucs returned %i (ocoffset 0, matchctl 0).", ret );
	//ret = cb_set_cursor_match_length_ucs( &(*cbs), &name, &namelen, ocoffset, 0 ); // matches first (any) (cbsearchnextleaves was added later, 1.7.2015)
	(**cbs).cf.searchmethod = searchmethod;

	free(*ucsname);
	*ucsname = NULL; // 11.12.2014
	if( ret==CBSUCCESS || ret==CBSTREAM || ret==CBFILESTREAM){ // returns only CBSUCCESS or CBSTREAM or error
	  //ret = cb_get_current_name( &(*cbs), &(*ucsname), &(*namelength) );
	  ret = cb_get_current_name( &(*cbs), &ptr, &(*namelength) ); // 11.7.2016
	  if( ptr!=NULL )
	    *ucsname = &(*ptr); // 11.7.2016
	  if(ret>=CBERROR){ cb_log( &(*cbs), CBLOGERR, ret, "\ncb_get_next_name_ucs: cb_get_current_name, error %i.", ret ); }
	  if(ret>=CBNEGATION){ cb_log( &(*cbs), CBLOGDEBUG, ret, "\ncb_get_next_name_ucs: cb_get_current_name returned %i.", ret ); }
	}

	/* May return CBMESSAGEHEADEREND if it was set */

	return ret;
}

/*
 * Not tested 1.7.2015. NEVER USED YET 6.7.2015 (under construction/will be removed)
 */
int  cb_find_leaf_from_current(CBFILE **cbs, unsigned char **ucsname, int *namelength, int *ocoffset, int matchctl ){
	cb_match mctl;
	mctl.re = NULL; mctl.matchctl = matchctl;
	return cb_find_leaf_from_current_matchctl( &(*cbs), &(*ucsname), &(*namelength), &(*ocoffset), &mctl);
}

/*
 * Requires atomic operation, sets searchmethod to "nextnames" and back.
 *
 * Return values
 * Success: CBSUCCESS
 * Error: CBERRALLOC and all from cb_set_cursor_match_length_ucs_matchctl
 * May return: CBNOTFOUND
 */
int  cb_read_value_leaves( CBFILE **cbs ){
	int err=CBNOTFOUND;
	unsigned char smethod = CBSEARCHNEXTNAMES;
	unsigned char *testname = NULL;
	cb_name *oldcurrent = NULL;
	int testnamelen = 1;
	unsigned char testdata[2] = { 0x20, '\0' };
	testname = &testdata[0];
	if( cbs==NULL || *cbs==NULL ){
		cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_find_leaf_from_current: allocation error."); 
		return CBERRALLOC; 
	}

	/*
	 * Save previous current location. */
	if( (*(**cbs).cb).list.name!=NULL ){
		oldcurrent = &(*(*(**cbs).cb).list.name);
		if( (*(**cbs).cb).list.current!=NULL ){
			oldcurrent = &(*(*(**cbs).cb).list.current);
		}
	}

        //fprintf(stderr,"\ncb_read_value_leaves: previous current [");
	//if( oldcurrent!=NULL )
        //	cb_print_ucs_chrbuf( CBLOGDEBUG, &(*oldcurrent).namebuf, (*oldcurrent).namelen, (*oldcurrent).buflen );
	//else
	//	fprintf(stderr,"<empty>");
        //fprintf(stderr,"]");

	/*
	 * Find any next name (and read all leaves at the same time). */
	smethod = (**cbs).cf.searchmethod;
	(**cbs).cf.searchmethod=CBSEARCHNEXTNAMES;
	err = cb_set_cursor_match_length_ucs( &(*cbs), &testname, &testnamelen, 0, 0 );
	(**cbs).cf.searchmethod = smethod;
	if(err<CBNEGATION){	
		/* 
		 * Decrease matchcount of found testname to find it again. */
		if( (*(**cbs).cb).list.current!=NULL ){
			if( (*(*(**cbs).cb).list.current).matchcount>0 )
				(*(*(**cbs).cb).list.current).matchcount--;
			else if( (*(*(**cbs).cb).list.current).matchcount<0 ) // was matched many times, overflow
				(*(*(**cbs).cb).list.current).matchcount=1;
		}
	}

/**
        cb_log( &(*cbs), CBLOGDEBUG, CBNEGATION, "\ncb_read_value_leaves: err=%i moved current from [", err );
	if( (*(**cbs).cb).list.current!=NULL )
          cb_print_ucs_chrbuf( CBLOGDEBUG, &(*(*(**cbs).cb).list.current).namebuf, (*(*(**cbs).cb).list.current).namelen, (*(*(**cbs).cb).list.current).buflen );
	else
	  cb_log( &(*cbs), CBLOGDEBUG, CBNEGATION, "<empty>");
        cb_log( &(*cbs), CBLOGDEBUG, CBNEGATION, "]");

	cb_log( &(*cbs), CBLOGDEBUG, CBNEGATION, " to [" );
 	if( oldcurrent!=NULL )
          cb_print_ucs_chrbuf( CBLOGDEBUG, &(*oldcurrent).namebuf, (*oldcurrent).namelen, (*oldcurrent).buflen );
	else
	  cb_log( &(*cbs), CBLOGDEBUG, CBNEGATION, "<empty>");
        cb_log( &(*cbs), CBLOGDEBUG, CBNEGATION, "]");
**/
	/*
	 * Rewind to the previous name. */
	(*(**cbs).cb).list.current = &(*oldcurrent);
	(*(**cbs).cb).list.currentleaf = NULL;

	return err;
}
int  cb_find_leaf_from_current_matchctl(CBFILE **cbs, unsigned char **ucsname, int *namelength, int *ocoffset, cb_match *mctl ){
	int err=CBNOTFOUND, safecount=0;
	char atvalueend=0;
	if( ocoffset==NULL)
		cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_find_leaf_from_current: ocoffset was null."); 
	if( cbs==NULL || *cbs==NULL || ucsname==NULL || *ucsname==NULL || namelength==NULL || ocoffset==NULL || mctl==NULL ){
		cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_find_leaf_from_current: allocation error."); 
		return CBERRALLOC; 
	}
	safecount=0;

	//fprintf(stderr,"\ncb_find_leaf_from_current: namelength %i, name [", *namelength );
	//cb_print_ucs_chrbuf( CBLOGDEBUG, &(*ucsname), *namelength, CBNAMEBUFLEN);
	//fprintf(stderr,"]");

	/*
	 * Read to the next name and move the current pointer to the previous name. */
	err = cb_read_value_leaves( &(*cbs) );
	if(err>CBERROR){ cb_log( &(*cbs), CBLOGERR, err, "\ncb_find_leaf_from_current: cb_read_value_leaves, error %i.", err); return err; }
	if(err>CBNEGATION){ cb_log( &(*cbs), CBLOGDEBUG, err, "\ncb_find_leaf_from_current: cb_read_value_leaves returned %i.", err); }

	/*
	 * Search first matching leaf from any ocoffset. */
	*ocoffset = 1;
	err = CBNOTFOUNDLEAVESEXIST;
	while( ( err==CBNOTFOUNDLEAVESEXIST || err==CBVALUEEND || err==CBSTREAMEND ) && safecount<CBMAXLEAVES ){
		++safecount;

	   	err = cb_set_cursor_match_length_ucs_matchctl( &(*cbs), &(*ucsname), &(*namelength), *ocoffset, &(*mctl) ); 

		if(err==CBSUCCESS || err==CBSUCCESSLEAVESEXIST || err==CBSTREAM || err==CBFILESTREAM){
			return CBSUCCESS;
		}else if(err>=CBERROR){
		   cb_log( &(*cbs), CBLOGALERT, err, "\ncb_find_leaf_from_current: cb_set_cursor_match_length_ucs, error %i.", err);
		   return err;
		}else if(err==CBNOTFOUNDLEAVESEXIST || err==CBVALUEEND || err==CBSTREAMEND){
		   if( err==CBVALUEEND || err==CBSTREAMEND ){
			++atvalueend;
		   }
		   if( atvalueend>2 )
			return CBNOTFOUND;
		   /*
		    * Continue to search the next level leafs if some leafs existed. */
	            *ocoffset+=1;
		}else if(err>=CBNEGATION){
		   ; // cb_log( &(*cbs), CBLOGDEBUG, CBNEGATION, "\ncb_find_leaf_from_current: cb_set_cursor_match_length_ucs returned %i.", err);
		}
	}
	return CBNOTFOUND;
}

/*
 * Content.
 */

int  cb_copy_current_content( CBFILE **cbf, unsigned char **ucscontent, int *clength, int maxlen ){
        if( cbf==NULL || *cbf==NULL || (**cbf).cb==NULL || ucscontent==NULL || *ucscontent==NULL || clength==NULL ){
		cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_copy_current_content: parameter was null.");
		return CBERRALLOC;
	}
	return cb_subfunction_get_current_content( &(*cbf), &(*ucscontent), &(*clength), maxlen, 0);
}
int  cb_get_current_content( CBFILE **cbf, unsigned char **ucscontent, int *clength, int maxlen ){
	unsigned char *ptr = NULL;
	int err = CBSUCCESS;
        if( cbf==NULL || *cbf==NULL || (**cbf).cb==NULL || ucscontent==NULL || clength==NULL ){
		cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_get_current_content: parameter was null.");
		return CBERRALLOC;
	}
	err = cb_subfunction_get_current_content( &(*cbf), &ptr, &(*clength), maxlen, 1);
	if( ptr!=NULL )
	  *ucscontent = &(*ptr); // 11.7.2016
	return err;
}
int  cb_subfunction_get_current_content( CBFILE **cbf, unsigned char **ucscontent, int *clength, int maxlen, char allocate ){
        int len = MAXCONTENTLEN;
	unsigned char *ptr = NULL;
        int err = CBSUCCESS;
        if( cbf==NULL || *cbf==NULL || (**cbf).cb==NULL || (*(**cbf).cb).list.current==NULL || clength==NULL || ucscontent==NULL ){ 
		cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_subfunction_get_current_content: parameter was null." );
		return CBERRALLOC; 
	}
        if( (*(*(**cbf).cb).list.current).length >= 0 )
                len = (*(*(**cbf).cb).list.current).length;
	if(maxlen>0 && len>maxlen) // 16.4.2016, 27.5.2016
		len=maxlen;
	if(allocate==0)
 	       err = cb_copy_content( &(*cbf), &(*(**cbf).cb).list.current, &(*ucscontent), &(*clength), len );
	else
 	       err = cb_get_content( &(*cbf), &(*(**cbf).cb).list.current, &(*ucscontent), &(*clength), len );
	if( ptr!=NULL )
		*ucscontent = &(*ptr); // 11.7.2016
	return err;
}
int  cb_copy_currentleaf_content( CBFILE **cbf, unsigned char **ucscontent, int *clength, int maxlen ){
        if( cbf==NULL || *cbf==NULL || (**cbf).cb==NULL || ucscontent==NULL || *ucscontent==NULL || clength==NULL ){ 
		cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_copy_currentleaf_content: parameter was null.");
		return CBERRALLOC; 
	}
	return cb_subfunction_get_currentleaf_content( &(*cbf), &(*ucscontent), &(*clength), 0, maxlen );
}
int  cb_get_currentleaf_content( CBFILE **cbf, unsigned char **ucscontent, int *clength, int maxlen ){
	unsigned char *ptr = NULL;
	int err = CBSUCCESS;
        if( cbf==NULL || *cbf==NULL || (**cbf).cb==NULL || ucscontent==NULL || clength==NULL ){
		cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_get_currentleaf_content: parameter was null.");
		return CBERRALLOC;
	}
	err = cb_subfunction_get_currentleaf_content( &(*cbf), &ptr, &(*clength), 1, maxlen );
	if( ptr!=NULL )
		*ucscontent = &(*ptr); // 11.7.2016
	return err;
}
int  cb_subfunction_get_currentleaf_content( CBFILE **cbf, unsigned char **ucscontent, int *clength, char allocate, int maxlen ){
        int len = MAXCONTENTLEN;
	unsigned char *ptr = NULL;
	int err = CBSUCCESS;
        if( cbf==NULL || *cbf==NULL || (**cbf).cb==NULL || (*(**cbf).cb).list.currentleaf==NULL || clength==NULL || ucscontent==NULL ){ 
		cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_get_currentleaf_content: parameter was null.");
		return CBERRALLOC; 
	}
	if( maxlen>0 && len>maxlen) // 27.5.2016
		len=maxlen;
        if( (*(*(**cbf).cb).list.currentleaf).length >= 0 ){
                len = (*(*(**cbf).cb).list.currentleaf).length * 4; // 15.9.2015, character count times four bytes per character
	}
	//cb_log( &(*cbf), CBLOGDEBUG, CBNEGATION, "\ncb_get_currentleaf_content: maximum content length was %i.", len);
	if(allocate==0){
		if( *ucscontent==NULL ){ cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_get_currentleaf_content: parameter was null."); return CBERRALLOC; }
        	return cb_copy_content( &(*cbf), &(*(**cbf).cb).list.currentleaf, &(*ucscontent), &(*clength), len ); 
	}else{
        	err = cb_get_content( &(*cbf), &(*(**cbf).cb).list.currentleaf, &ptr, &(*clength), len ); 
		if( ptr!=NULL )
			*ucscontent = &(*ptr); // 11.7.2016
		return err;
	}
}

/*
 * ucscontent is allocated and copied from current index from rstart '=' to rend '&'
 * cb_name:s length to maximum of maxlength.
 * 
 * If cn_name:s length is not set, allocates maxlength size buffer with *clength content.
 *
 * Returns: CBSUCCESS, CBSUCCESSJSONQUOTES, CBSUCCESSJSONARRAY
 * On error: CBERRALLOC
 */
int  cb_get_content( CBFILE **cbf, cb_name **cn, unsigned char **ucscontent, int *clength, int maxlength ){
        int maxlen = maxlength;
        if( cbf==NULL || *cbf==NULL || clength==NULL ){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_get_content: cbf or clength was null."); return CBERRALLOC; }
        if( cn==NULL || *cn==NULL ){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_get_content: cn was null."); return CBERRALLOC; }
        if( ucscontent==NULL ){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_get_content: ucscontent was null."); return CBERRALLOC; }

        /*
         * Count length */
	maxlen = maxlength; // 19.7.2016
        if( (**cn).length > 0 ) // Length from previous count
                maxlen = (**cn).length * 4; // 15.9.2015, character count times four bytes per character
        // Otherwice allocates maxlength buffer
        
        /*
         * Allocate buffer */
        *ucscontent = (unsigned char*) malloc( sizeof(unsigned char)*( (unsigned int) maxlen+2 ) );
        if( ucscontent==NULL ) {
                cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_get_content: malloc returned null.");
                return CBERRALLOC;
        }
        memset( &(**ucscontent), 0x20, (unsigned int) maxlen+1 );
        (*ucscontent)[ maxlen+1 ] = '\0';
        *clength = maxlen; 

	//return cb_copy_content( &(*cbf), &(*cn), &(*ucscontent), &(*clength), maxlength );
	return cb_copy_content( &(*cbf), &(*cn), &(*ucscontent), &(*clength), maxlen ); // 19.7.2016
}

/*
 * The same as cb_copy_content with JSON value form check if cb_conf checkjson is set. */
int  cb_copy_content( CBFILE **cbf, cb_name **cn, unsigned char **ucscontent, int *clength, int maxlength ){
	int err = CBSUCCESS, from = 0;
        if( cbf==NULL || *cbf==NULL || clength==NULL ){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_copy_content: cbf or clength was null."); return CBERRALLOC; }
        if( cn==NULL || *cn==NULL ){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_copy_content: cn was null."); return CBERRALLOC; }
	if( ucscontent==NULL || *ucscontent==NULL ){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_copy_content: ucscontent was null."); return CBERRALLOC; }
	if( (**cn).namebuf==NULL ){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_copy_content: (**cn).namebuf was null."); return CBERRALLOC; }

// 19.7.2016 (similar with cb_get_content)
        if( (**cn).length > 0 )
                *clength = (**cn).length * 4; // 15.9.2015, character count times four bytes per character
        else
                *clength = maxlength;
// /19.7.2016

	err = cb_copy_content_internal( &(*cbf), &(*cn), &(*ucscontent), &(*clength), maxlength );
	if( err>=CBERROR ){ cb_clog( CBLOGERR, err, "\ncb_copy_content: cb_copy_content_internal, error %i.", err ); return err; }
	if( (**cbf).cf.urldecodevalue==1 ){
		/*
		 * JSON format can't be used with this. Never use URL-encoding, 25.5.2016. */
		err = cb_decode_url_encoded_bytes( &(*ucscontent), *clength, &(*ucscontent), &(*clength), maxlength );
		if( err>=CBERROR ){ cb_clog( CBLOGERR, err, "\ncb_copy_content: cb_decode_url_encoded_bytes, error %i.", err ); }
	}
	if( err>=CBNEGATION || (**cbf).cf.jsonvaluecheck!=1 )
		return err;
	/*
	 * Optional JSON value integrity check. */
	if( *clength>maxlength )
		return CBNOTJSON; // too long
	if(err==CBSUCCESSJSONQUOTES){
		return cb_check_json_string_content( &(*ucscontent), *clength , &from, (**cbf).cf.jsonremovebypassfromcontent );
	}
	return cb_check_json_value( &(*cbf), &(*ucscontent), *clength, &from );
}
/*
 * If (**cbf).cf.json==1, removes quotes around the content.
 *
 * Returns:  CBSUCCESSJSONQUOTES	- Was a JSON string, quotes removed.
 *           CBSUCCESSJSONARRAY 	- Includes JSON array brackets.
 *           CBSUCCESS          	- May have been a JSON value.
 */
int  cb_copy_content_internal( CBFILE **cbf, cb_name **cn, unsigned char **ucscontent, int *clength, int maxlength ){
        int err=CBSUCCESS, bsize=0, ssize=0, ucsbufindx=0;
        unsigned long int chr = 0x20, chprev = 0x20;
	char injsonquotes=0, atstart=1, jsonstring=1, injsonarray=0;
        int openpairs=1; char found=0, firstjsonbracket=1;
        int maxlen = maxlength, lindx=0;
        if( cbf==NULL || *cbf==NULL || clength==NULL ){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_copy_content_internal: cbf or clength was null."); return CBERRALLOC; }
        if( cn==NULL || *cn==NULL ){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_copy_content_internal: cn was null."); return CBERRALLOC; }
	if( ucscontent==NULL || *ucscontent==NULL ){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_copy_content_internal: ucscontent was null."); return CBERRALLOC; }
	if( (**cn).namebuf==NULL ){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_copy_content_internal: (**cn).namebuf was null."); return CBERRALLOC; }

        /*
         * Copy contents and update length. */
        ucsbufindx=0;
        chprev = (**cbf).cf.bypass-1; chr = (**cbf).cf.bypass+1;
        for(lindx=0 ; lindx<*clength && lindx<maxlen && lindx<maxlength && err<CBNEGATION && ucsbufindx<maxlength && \
		 ( (**cbf).cf.stopatmessageend==0 || err!=CBMESSAGEEND ) && ( (**cbf).cf.stopatheaderend==0 || err!=CBMESSAGEHEADEREND ); ++lindx ){
                chprev = chr;

		if( (**cbf).cf.json==1 ){
		   if( chr=='}' && firstjsonbracket==1 && injsonquotes!=1 )
		   	firstjsonbracket=0;
		   else if( injsonquotes!=1 && chr!='}' && ! ( WSP( chr ) || CR( chr ) || LF( chr ) ) )
		   	firstjsonbracket=1;
		}

                err = cb_get_chr( &(*cbf), &chr, &bsize, &ssize); // returns CBSTREAMEND if EOF
		//cb_clog( CBLOGDEBUG, CBNEGATION, "[%c]", (char) chr );
		if( err>CBERROR ){ cb_clog( CBLOGERR, err, "\ncb_copy_content: cb_get_chr, error %i.", err); return err; }
	        if( (**cbf).cf.stopatmessageend==1 && err==CBMESSAGEEND ) continue; // actually stop, cf flag 8.4.2016
	        if( (**cbf).cf.stopatheaderend==1 && err==CBMESSAGEHEADEREND ) continue; // actually stop, cf flag 8.4.2016

		if( (**cbf).cf.json==1 ){
		   if( chr=='[' && injsonquotes==0 ) // 9.11.2015, 10.11.2015
			injsonarray=1;
		   else if( chr==']' && injsonarray==1 && ( injsonquotes==0 || injsonquotes>=2 ) ) // 10.11.2015
			injsonarray=2;
		}

                if( injsonquotes!=1 && chprev!=(**cbf).cf.bypass && ( chr==(**cbf).cf.rstart || chr==(**cbf).cf.subrstart ) ){ // 1.7.2015, 12.11.2015
                        ++openpairs;
			//cb_clog( CBLOGDEBUG, CBNEGATION, " ++openpairs=%i ", openpairs );
		}
                if( injsonquotes!=1 && chprev!=(**cbf).cf.bypass && ( ( chr==(**cbf).cf.rend && injsonarray!=1 ) || chr==(**cbf).cf.subrend ) ){ // 1.7.2015, 9.11.2015
                        --openpairs;
			if( chr==(**cbf).cf.subrend && firstjsonbracket==1 ){ --openpairs; firstjsonbracket=0; }
			//cb_clog( CBLOGDEBUG, CBNEGATION, " --openpairs=%i ", openpairs );
		}
                if( openpairs<=0 && ( (**cbf).cf.searchstate==CBSTATETREE || (**cbf).cf.searchstate==CBSTATETOPOLOGY ) ){
			//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_copy_content: openpairs==0, stop.");
                        lindx=maxlength; // stop
                        found=1;
                        continue;
                }else if( ( (**cbf).cf.searchstate==CBSTATELESS || (**cbf).cf.searchstate==CBSTATEFUL ) &&
                            ( chr==(**cbf).cf.rend && chprev!=(**cbf).cf.bypass ) ){
                        lindx=maxlength; // stop
                        found=1;
                        continue;
                }
                if( err<CBERROR ){
			/* If JSON, Removes white space characters before content. */
			if( (**cbf).cf.json==1 && atstart==1 && ( WSP( chr ) || CR( chr ) || LF( chr ) ) )
				continue; // 8.11.2015
			else
				atstart=0;
			/*
			 * If JSON and not quotes, jsonstring==0. */
			if( (**cbf).cf.json==1 ){
			  if( chprev!=(**cbf).cf.bypass && chr==(unsigned long int)'\"' && jsonstring>=1 ){ // value without quotes 1
              		    if( injsonquotes==1 ) // second quote
                	  	injsonquotes=2; // ready to save the name (or after value)
			  }else if(jsonstring==1)
				jsonstring=0;
			}
			if( (**cbf).cf.json!=1 || ( (**cbf).cf.json==1 && ( ( injsonquotes==1 || injsonarray==1 || jsonstring==0 ) ) ) ) { // 10.11.2015
			 	//cb_clog( CBLOGDEBUG, CBNEGATION, "[0x%.2X]", (char) chr, '\"' ); // BEST
				/*
				 * Removes bypass characters from content. */
				if( (**cbf).cf.json!=1 || ( (**cbf).cf.jsonremovebypassfromcontent==1 && \
				      (**cbf).cf.json==1 && ! ( ( chprev=='\\' && chr=='"' ) || ( chprev=='\\' && chr=='\\' ) ) ) ){ // 2.3.2016
                        	   err = cb_put_ucs_chr( chr, &(*ucscontent), &ucsbufindx, *clength);
				}
			}
			if(err>CBERROR){ cb_clog( CBLOGERR, err, "\ncb_copy_content: cb_put_ucs_chr, error %i.", err); return err; }
			if( (**cbf).cf.json==1 ){
			  if( chprev!=(**cbf).cf.bypass && chr==(unsigned long int)'\"' && jsonstring==1 ){ // value without quotes 2
			    jsonstring=2;
             		    if( injsonquotes==0 || ( injsonquotes==2 && injsonarray==1 ) ) // first quote, or second quote if an array, 10.11.2015
 		        	injsonquotes=1; // 1
  	          	  }
			}
		}
                if( chprev==(**cbf).cf.bypass && chr==(**cbf).cf.bypass )
                        chr = (**cbf).cf.rstart; // any chr not bypass
        }
        *clength = ucsbufindx;

	if(ucsbufindx<maxlen) // 14.2.2015, terminating '\0'
	  (*ucscontent)[ucsbufindx+1]='\0';

        if( found==1 && (**cn).length<0 && err<CBNEGATION){
                (**cn).length = ucsbufindx/4;
        }

	//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_copy_content: content: [" );
	//if( ucscontent!=NULL && *ucscontent!=NULL && clength!=NULL && (*clength)>0 )
	//  cb_print_ucs_chrbuf( CBLOGDEBUG, &(*ucscontent), (*clength), maxlength);
	//cb_clog( CBLOGDEBUG, CBNEGATION, "] injsonquotes: %i, injsonarray: %i.", injsonquotes, injsonarray );

	/*
	 * If typecheck is needed without quotes. */
	if(injsonquotes>=2)
		return CBSUCCESSJSONQUOTES;
	if(injsonarray>=2)
		return CBSUCCESSJSONARRAY;
	if(err<CBNEGATION) // 30.3.2016
          return CBSUCCESS;
	return err;
}

/*
 * Copies 4-byte characters to one byte characters and cut's the buffer with '\0'. 
 * Does not free. */
int  cb_convert_from_ucs_to_onebyte( unsigned char **name, int *namelen ){ 
	int indx = 0, onebindx = 0, err = CBSUCCESS;
	unsigned long int chr = 0x20;
	if( name==NULL || *name==NULL || namelen==NULL ){
		cb_clog( CBLOGERR, CBERRALLOC, "\ncb_convert_from_ucs_to_onebyte: parameter was null, error %i.", CBERRALLOC );
		return CBERRALLOC;
	}
	for( indx=0; indx<*namelen && indx<CBNAMEBUFLEN && err<CBERROR && (onebindx+1)<*namelen; ++onebindx ){
		err = cb_get_ucs_chr( &chr, &(*name), &indx, *namelen);
		(*name)[ onebindx ] = (unsigned char) chr ;
	}
	(*name)[ onebindx ] = '\0';
	*namelen = onebindx;
	return CBSUCCESS;
}
/*
 * Ucsname is allocated, ucsnamelen is not allocated. ucsname has to be NULL or CBERRALLOC will result. */ 
int  cb_allocate_ucsname_from_onebyte( unsigned char **ucsname, int *ucsnamelen, unsigned char **onebytename, int *onebytenamelen ){ 
	if( ucsname==NULL || *ucsname!=NULL || onebytename==NULL || *onebytename==NULL || onebytenamelen==NULL || ucsnamelen==NULL ){
		cb_clog( CBLOGERR, CBERRALLOC, "\ncb_allocate_ucsname_from_onebyte: parameter was null or ucsname was not empty." );
		return CBERRALLOC;
	}
	*ucsname = (unsigned char*) malloc( sizeof(unsigned char) * ( 1 + ( 4*( (unsigned int) *onebytenamelen) ) ) );
	if(*ucsname==NULL){
		cb_clog( CBLOGERR, CBERRALLOC, "\ncb_allocate_ucsname_from_onebyte: malloc returned null.");
		return CBERRALLOC;
	}
	*ucsnamelen = 4*(*onebytenamelen);
	memset( &(**ucsname), (int) 0x20, (size_t) *ucsnamelen );
	(*ucsname)[*ucsnamelen] = '\0';
	return cb_copy_ucsname_from_onebyte( &(*ucsname), &(*ucsnamelen), &(*onebytename), &(*onebytenamelen) );
} 
int  cb_copy_ucsname_from_onebyte( unsigned char **ucsname, int *ucsnamelen, unsigned char **onebytename, int *onebytenamelen ){
	int err=CBSUCCESS, indx=0, onebindx=0 ;
	unsigned long int chr = 0x20;
	if( ucsname==NULL || *ucsname==NULL || onebytename==NULL || *onebytename==NULL || onebytenamelen==NULL || ucsnamelen==NULL ){
		cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_copy_ucsname_from_onebyte: parameter was null.");
		return CBERRALLOC;
	}
	onebindx=0;
	for( indx=0; indx<*ucsnamelen && onebindx<*onebytenamelen && err<CBNEGATION; ){
		chr = (*onebytename)[onebindx];
		err = cb_put_ucs_chr( chr, &(*ucsname), &indx, *ucsnamelen );
		++onebindx;
	}
	return err;
}

/*
 * Hierarchical searches. These are at the moment all usable search methods.
 * If names length is still -1, searches all leafs of the name (until the next name)
 * and searches the wanted named leaf by searching from the resulted tree.
 *
 * If search is not done name by name (the names being in the list), the tree structure
 * will be broken. It is possible to find just one name like this. During other 
 * searches, the tree would be broken. 19.8.2015
 *
 * May be removed if not used (27.8.2015).
 */
// int cb_search_leaf_from_currentname(CBFILE **cbf, unsigned char **ucsparameter, int ucsparameterlen ){

int  cb_get_long_int( unsigned char **ucsnumber, int ucsnumlen, signed long int *nmbr){
	int indx=0, err=CBSUCCESS, maxcounter=0; unsigned long int nm=0;
	char first=1, minus=0;
	if( ucsnumber==NULL || *ucsnumber==NULL || nmbr==NULL ){
		cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_get_long_int: parameter was null.");
		return CBERRALLOC;
	}
	*nmbr=0;

	for(indx=0; indx<ucsnumlen && indx>=0 && err<CBNEGATION && maxcounter<11; ){
		++maxcounter; // minus sign plus characters 4294967295, together 11, without minus, 10
		err = cb_get_ucs_chr( &nm, &(*ucsnumber), &indx, ucsnumlen);
		if(err<CBNEGATION){ // 16 bits largest number 655536
			if(first==0){
				*nmbr = (*nmbr)*10;
			}else{
				if( nm == (signed long int) '-' ){ // negative
					++maxcounter;
					minus = 1;
				}else
					first = 0;
			}
			if( nm != (signed long int) '-' )
				(*nmbr) += (nm-0x30);
		}
	}
	if( minus==1 )
		(*nmbr) = 0 - (*nmbr);
	return CBSUCCESS;
}

