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
#include "../include/cb_read.h"

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
	    cb_log( &(*cbs), CBLOGDEBUG, "\ncb_get_current_name: debug, cb_get_current_name_ucs: *ucsname was not NULL.");
	  *ucsname = (unsigned char*) malloc( sizeof(unsigned char)*( (unsigned int) (*(*(**cbs).cb).list.current).namelen+1 ) );
	  if( ucsname==NULL ) { return CBERRALLOC; }
	  (*ucsname)[(*(*(**cbs).cb).list.current).namelen] = '\0';
	  for( indx=0; indx<(*(*(**cbs).cb).list.current).namelen ; ++indx)
	    (*ucsname)[indx] = (*(*(**cbs).cb).list.current).namebuf[indx];
	  if(namelength==NULL)
	    namelength = (int*) malloc( sizeof( int ) ); // int 
	  if(namelength==NULL) { return CBERRALLOC; }
 	  *namelength = (*(*(**cbs).cb).list.current).namelen;
	}else{
	  ret = CBERRALLOC; 
	}
	if( (**cbs).cb!=NULL && (*(**cbs).cb).list.current==NULL )
	   return CBNOTFOUND;
	else if( (**cbs).cb==NULL )
	   cb_log( &(*cbs), CBLOGDEBUG, "\ncb_get_current_name: (**cbs).cb was null. ");
        if( ucsname==NULL || *ucsname==NULL ){ ret = CBERRALLOC; }
	return ret;
}

/*
 * Searched next name in list. Not leafs. */
int  cb_get_next_name_ucs(CBFILE **cbs, unsigned char **ucsname, int *namelength){
	int ret = CBSUCCESS;
	unsigned char *name = NULL;
	unsigned char  chrs[2] = { 0x20, '\0' };
	int namelen = 0;
	unsigned char searchmethod=0;
	name = &chrs[0];

	if( cbs==NULL || *cbs==NULL ){	  cb_log( &(*cbs), CBLOGALERT, "\ncb_get_next_name_ucs: cbs was null."); return CBERRALLOC; }
	if( *ucsname!=NULL ){
	  cb_log( &(*cbs), CBLOGERR, "\ncb_get_next_name_ucs: error, *ucsname was not NULL.");
	  return CBERRALLOC;
	}

	searchmethod = (**cbs).cf.searchmethod;
	(**cbs).cf.searchmethod = CBSEARCHNEXTNAMES;
	ret = cb_set_cursor_match_length_ucs( &(*cbs), &name, &namelen, 0, 0 ); // matches first (any)
	//ret = cb_set_cursor_match_length_ucs( &(*cbs), &name, &namelen, ocoffset, 0 ); // matches first (any) (cbsearchnextleaves was added later, 1.7.2015)
	(**cbs).cf.searchmethod = searchmethod;

	free(*ucsname);
	*ucsname = NULL; // 11.12.2014
	if( ret==CBSUCCESS || ret==CBSTREAM || ret==CBFILESTREAM){ // returns only CBSUCCESS or CBSTREAM or error
	  ret = cb_get_current_name( &(*cbs), &(*ucsname), &(*namelength) );
	  if(ret>=CBERROR){ cb_log( &(*cbs), CBLOGERR, "\ncb_get_next_name_ucs: cb_get_current_name, error %i.", ret ); }
	  if(ret>=CBNEGATION){ cb_log( &(*cbs), CBLOGDEBUG, "\ncb_get_next_name_ucs: cb_get_current_name returned %i.", ret ); }
	}

	/* May return CB2822HEADEREND if it was set */

	return ret;
}

/*
 * Not tested 1.7.2015. NEVER USED YET 3.7.2015 (under construction/will be removed)
 *
 * Returns CBSUCCESS, CBNOTFOUND or an error. */
int  cb_find_leaf_from_current(CBFILE **cbs, unsigned char **ucsname, int *namelength, int *ocoffset, int matchctl ){
	cb_match mctl;
	mctl.re = NULL; mctl.re_extra = NULL; mctl.matchctl = matchctl;
	return cb_find_leaf_from_current_matchctl( &(*cbs), &(*ucsname), &(*namelength), &(*ocoffset), &mctl);
}
int  cb_find_leaf_from_current_matchctl(CBFILE **cbs, unsigned char **ucsname, int *namelength, int *ocoffset, cb_match *mctl ){
	int err=CBNOTFOUND, err2=CBNOTFOUND, safecount=0;
	cb_name *cnpointer = NULL;
	cb_name *nextcn = NULL;
	if( ocoffset==NULL)
		cb_clog( CBLOGALERT, "\ncb_find_leaf_from_current: ocoffset was null."); 
	if( cbs==NULL || *cbs==NULL || ucsname==NULL || *ucsname==NULL || namelength==NULL || ocoffset==NULL || mctl==NULL ){
		cb_clog( CBLOGALERT, "\ncb_find_leaf_from_current: allocation error."); 
		return CBERRALLOC; 
	}
	safecount=0;

	fprintf(stderr,"\ncb_find_leaf_from_current: namelength %i, name [", *namelength );
	cb_print_ucs_chrbuf( CBLOGDEBUG, &(*ucsname), *namelength, CBNAMEBUFLEN);
	fprintf(stderr,"]");

	cnpointer = (*(**cbs).cb).list.name;
	if( (*(**cbs).cb).list.current!=NULL )
	  cnpointer = (*(**cbs).cb).list.current;
	if( (*(**cbs).cb).list.currentleaf!=NULL )
	  cnpointer = (*(**cbs).cb).list.currentleaf;

	/*
	 * Continue until value end or no more leafs. */
	while( err==CBNOTFOUND && safecount<CBMAXLEAVES && cnpointer!=NULL ){
		++safecount;

		//fprintf(stderr,"\ncb_find_leaf_from_current: ocoffset %i, err %i, safecount %i, matchctl %i.", *ocoffset, err, safecount, (*mctl).matchctl );
	   	err = cb_set_cursor_match_length_ucs( &(*cbs), &(*ucsname), &(*namelength), *ocoffset, 0 ); // any next (compared later)
		//fprintf(stderr,"\ncb_find_leaf_from_current: cb_set_cursor_match_length_ucs, err %i.", err );

		cnpointer = (*(**cbs).cb).list.currentleaf;
		/*
		 * Check if next level exists from this leaf. */
		if( cnpointer!=NULL && nextcn==NULL )
		  nextcn = (*cnpointer).leaf;

		if(err==CBSUCCESS || err==CBSTREAM || err==CBFILESTREAM){
		   if( cnpointer!=NULL ){
		     err2 = cb_compare( &(*cbs), &(*ucsname), *namelength, &(*cnpointer).namebuf, (*cnpointer).namelen , &(*mctl) );
		     if(err2==CBMATCH){
		     	/*
		      	 * Currentleaf is at needed leaf (matchctl). */
		     	return CBSUCCESS;
		     }else{
			/*
			 * Continue to match the next name. */
			continue;
		     }
		   }
		}else if(err>=CBERROR){
		   cb_log( &(*cbs), CBLOGALERT, "\ncb_find_leaf_from_current: cb_set_cursor_match_length_ucs, error %i.", err);
		   return err;
		}else if(err==CBVALUEEND || err==CBSTREAMEND){
		   /*
		    * Continue to search the next level leafs if some leafs existed. */
		   if( nextcn!=NULL ){
		      *ocoffset=*ocoffset+1;
		      nextcn=NULL;
		      continue;
		   }else{
		      return CBNOTFOUND;
		   }
		}else if(err>=CBNEGATION){
		   cb_log( &(*cbs), CBLOGDEBUG, "\ncb_find_leaf_from_current: cb_set_cursor_match_length_ucs returned %i.", err);
		}
	}
	return CBNOTFOUND;
}

/*
 * Content.
 */

int  cb_get_current_content( CBFILE **cbf, unsigned char **ucscontent, int *clength ){
        int len = MAXCONTENTLEN;
        if( cbf==NULL || *cbf==NULL || (**cbf).cb==NULL || (*(**cbf).cb).list.current==NULL || clength==NULL ){ return CBERRALLOC; }
        if( (*(*(**cbf).cb).list.current).length >= 0 )
                len = (*(*(**cbf).cb).list.current).length;
        return cb_get_content( &(*cbf), &(*(**cbf).cb).list.current, &(*ucscontent), &(*clength), len );
}
int  cb_get_currentleaf_content( CBFILE **cbf, unsigned char **ucscontent, int *clength ){
        int len = MAXCONTENTLEN;
        if( cbf==NULL || *cbf==NULL || (**cbf).cb==NULL || (*(**cbf).cb).list.currentleaf==NULL || clength==NULL ){ return CBERRALLOC; }
        if( (*(*(**cbf).cb).list.currentleaf).length >= 0 )
                len = (*(*(**cbf).cb).list.currentleaf).length;
        return cb_get_content( &(*cbf), &(*(**cbf).cb).list.currentleaf, &(*ucscontent), &(*clength), len );
}

/*
 * ucscontent is allocated and copied from current index from rstart '=' to rend '&'
 * cb_name:s length to maximum of maxlength.
 * 
 * If cn_name:s length is not set, allocates maxlength size buffer with *clength content.
 *
 * Returns: CBSUCCESS
 * On error: CBERRALLOC
 */
int  cb_get_content( CBFILE **cbf, cb_name **cn, unsigned char **ucscontent, int *clength, int maxlength ){
        int err=CBSUCCESS, bsize=0, ssize=0, ucsbufindx=0;
        unsigned long int chr = 0x20, chprev = 0x20;
        int openpairs=1; char found=0;
        int maxlen = maxlength, lindx=0; // lindx 14.2.2015
        if( cbf==NULL || *cbf==NULL || clength==NULL ){ cb_log( &(*cbf), CBLOGALERT, "\ncb_get_content: cbf or clength was null."); return CBERRALLOC; }
        if( cn==NULL || *cn==NULL ){ cb_log( &(*cbf), CBLOGALERT, "\ncb_get_content: cn was null."); return CBERRALLOC; }
        
        if(ucscontent==NULL)
                ucscontent = (unsigned char**) malloc( sizeof( PSIZE ) ); // pointer size
        if(ucscontent==NULL){ cb_clog( CBLOGALERT, "\ncb_get_content: malloc returned null."); return CBERRALLOC; }                
        /*
         * Count length */
        if( (**cn).length > 0 ) // Length from previous count
                maxlen = (**cn).length;
        // Otherwice allocates maxlength buffer
        
        /*
         * Allocate buffer */
        *ucscontent = (unsigned char*) malloc( sizeof(unsigned char)*( (unsigned int) maxlen+2 ) );
        if( ucscontent==NULL ) {
                cb_log( &(*cbf), CBLOGALERT, "\ncb_get_content: malloc returned null.");
                return CBERRALLOC;
        }
        memset( &(**ucscontent), 0x20, (unsigned int) maxlen+1 );
        (*ucscontent)[ maxlen+1 ] = '\0';
        *clength = maxlen; 

        /*
         * Copy contents and update length. */
        ucsbufindx=0;
        chprev = (**cbf).cf.bypass-1; chr = (**cbf).cf.bypass+1;
        //for(maxlen=0 ; maxlen<maxlength && err<CBERROR; ++maxlen ){
        for(lindx=0 ; lindx<maxlen && lindx<maxlength && err<CBERROR; ++lindx ){
                chprev = chr;
                err = cb_get_chr( &(*cbf), &chr, &bsize, &ssize);

                // if( chprev!=(**cbf).cf.bypass && chr==(**cbf).cf.rstart ) // pre 1.7.2015
                // if( chprev!=(**cbf).cf.bypass && chr==(**cbf).cf.rend ) // pre 1.7.2015
                if( chprev!=(**cbf).cf.bypass && ( chr==(**cbf).cf.rstart || chr==(**cbf).cf.subrstart ) ) // 1.7.2015
                        ++openpairs;
                if( chprev!=(**cbf).cf.bypass && ( chr==(**cbf).cf.rend || chr==(**cbf).cf.subrend ) ) // 1.7.2015
                        --openpairs;
                if( openpairs<=0 && ( (**cbf).cf.searchstate==CBSTATETREE || (**cbf).cf.searchstate==CBSTATETOPOLOGY ) ){
                        //maxlen=maxlength; // stop
                        lindx=maxlength; // stop
                        found=1;
                        continue;
                }else if( ( (**cbf).cf.searchstate==CBSTATELESS || (**cbf).cf.searchstate==CBSTATEFUL ) &&
                            ( chr==(**cbf).cf.rend && chprev!=(**cbf).cf.bypass ) ){
                        //maxlen=maxlength; // stop
                        lindx=maxlength; // stop
                        found=1;
                        continue;
                }
                if( err<CBERROR )
                        err = cb_put_ucs_chr( chr, &(*ucscontent), &ucsbufindx, *clength);
                if( chprev==(**cbf).cf.bypass && chr==(**cbf).cf.bypass )
                        chr = (**cbf).cf.rstart; // any chr not bypass
        }
        *clength = ucsbufindx;

	if(ucsbufindx<maxlen) // 14.2.2015, terminating '\0'
	  (*ucscontent)[ucsbufindx+1]='\0';

        if( found==1 && (**cn).length<0 && err<CBNEGATION){
                (**cn).length = ucsbufindx/4;
        }

        return CBSUCCESS;
}

/*
 * Ucsname is allocated, ucsnamelen is not allocated. ucsname has to be NULL or CBERRALLOC will result. */
int  cb_allocate_ucsname_from_onebyte( unsigned char **ucsname, int *ucsnamelen, unsigned char **onebytename, int *onebytenamelen ){
	int err=CBSUCCESS, indx=0, onebindx=0 ;
	unsigned long int chr = 0x20;
	if( *ucsname!=NULL || onebytename==NULL || *onebytename==NULL || onebytenamelen==NULL || ucsnamelen==NULL ) return CBERRALLOC;
	*ucsname = (unsigned char*) malloc( sizeof(unsigned char) * ( 1 + ( 4*( (unsigned int) *onebytenamelen) ) ) );
	if(*ucsname==NULL){
		cb_clog( CBLOGERR, "\ncb_allocate_ucsname_from_onebyte: malloc returned null.");
		return CBERRALLOC;
	}
	*ucsnamelen = 4*(*onebytenamelen);
	memset( &(**ucsname), (int) 0x20, (size_t) *ucsnamelen );
	(*ucsname)[*ucsnamelen] = '\0';
	onebindx=0;
	for( indx=0; indx<*ucsnamelen && onebindx<*onebytenamelen && err<CBNEGATION; ){
		chr = (*onebytename)[onebindx];
		err = cb_put_ucs_chr( chr, &(*ucsname), &indx, *ucsnamelen );
		++onebindx;
	}
	return err;
}
