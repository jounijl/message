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

int  cb_put_name(CBFILE **str, cb_name **cbn);
int  cb_set_to_name(CBFILE **str, unsigned char **name, int namelen, int matchlen);
int  cb_set_search_method(CBFILE **buf, char method); // CBSEARCH*
int  cb_search_get_chr( CBFILE **cbs, unsigned long int *chr, long int *chroffset);
int  cb_save_name_from_charbuf(CBFILE **cbs, cb_name *fname, long int offset, unsigned char **charbuf, int index);
int  cb_automatic_encoding_detection(CBFILE **cbs);
int  cb_set_cursor_match_length_ucs(CBFILE **cbs, unsigned char **ucsname, int *namelength, int matchlen);

int  cb_put_name(CBFILE **str, cb_name **cbn){ 
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

        if((*(**str).cb).name!=NULL){
          (*(**str).cb).current = &(*(*(**str).cb).last);
          if( (*(**str).cb).namecount>=1 )
            (*(*(**str).cb).current).length = ( (*(**str).cb).index - (**str).ahd.bytesahead ) - (*(*(**str).cb).current).offset;
          (*(**str).cb).last    = &(* (cb_name*) (*(*(**str).cb).last).next );
          err = cb_allocate_name( &(*(**str).cb).last ); if(err!=CBSUCCESS){ return err; }
          (*(*(**str).cb).current).next = &(*(*(**str).cb).last); // previous
          (*(**str).cb).current = &(*(*(**str).cb).last);
          ++(*(**str).cb).namecount;
          //if( (*(**str).cb).contentlen >= (*(**str).cb).buflen )
          if( ( (*(**str).cb).contentlen - (**str).ahd.bytesahead ) >= (*(**str).cb).buflen ) // 6.9.2013
            (*(*(**str).cb).current).length = (*(**str).cb).buflen; // Largest 'known' value
        }else{
          err = cb_allocate_name( &(*(**str).cb).name ); if(err!=CBSUCCESS){ return err; }
          (*(**str).cb).last    = &(* (cb_name*) (*(**str).cb).name); // last
          (*(**str).cb).current = &(* (cb_name*) (*(**str).cb).name); // current
          (*(*(**str).cb).current).next = NULL; // firsts next
          (*(**str).cb).namecount=1;
        }
        err = cb_copy_name(cbn, &(*(**str).cb).last); if(err!=CBSUCCESS){ return err; }
        (*(*(**str).cb).last).next = NULL;
        return CBSUCCESS;
}

//int  cb_set_to_name(CBFILE **str, unsigned char **name, int namelen){ 
int  cb_set_to_name(CBFILE **str, unsigned char **name, int namelen, int matchlen){ // 4.11.2013
	cb_name *iter = NULL; int err=CBSUCCESS;

	if(*str!=NULL && (**str).cb != NULL ){
	  iter = &(*(*(**str).cb).name);
	  while(iter != NULL){
	    if((**str).cf.caseinsensitive==1)
	      err = cb_compare_rfc2822( &(*iter).namebuf, (*iter).namelen, &(*name), namelen, matchlen );
	    else
	      err = cb_compare( &(*iter).namebuf, (*iter).namelen, &(*name), namelen, matchlen );
	    // if( err == CBMATCH ){ 
	    if( err == CBMATCH || err == CBMATCHLENGTH ){ // 4.11.2013
	      /*
	       * 20.8.2013:
	       * If searching of multiple same names is needed (in buffer also), do not return 
	       * allready matched name. Instead, increase names matchcount (or set to 1 if it 
	       * becomes 0) and search the next same name.
	       */
	      //if( (**str).cf.searchmethod==CBSEARCHNEXTNAMES ){ // 6.11.2013, matchcount kasvaa mutta ei merkitse jos CBSEARCHUNIQUENAMES
	      (*iter).matchcount++; if( (*iter).matchcount==0 ){ (*iter).matchcount+=2; }
	      //} // pois 6.11.2013.  Muutoin vaihdettaessa CBSEARCHNEXTNAMES:iin, matchcount tulee tayttaa ja haetaan aina uudelleen jo haettuun nimeen
	      /* First match on new name or if unique names are in use, the first match or the same match again, even if in stream. */
	      if( ( (**str).cf.searchmethod==CBSEARCHNEXTNAMES && (*iter).matchcount==1 ) || (**str).cf.searchmethod==CBSEARCHUNIQUENAMES ){ 
	        if( (**str).cf.type!=CBCFGFILE ) // When used as only buffer, stream case does not apply
	          if((*iter).offset>=( (*(**str).cb).buflen + 0 + 1 ) ){ // buflen + smallest possible name + endchar
		    /*
	             * Do not return names in namechain if it's content is out of memorybuffer
		     * to search multiple same names again from stream.
	 	     */
	            (**str).cb->index = (**str).cb->buflen; // set as a stream
	            (**str).cb->current = (cb_name*) iter;
	            return CBNAMEOUTOFBUF;
	          }
	        (**str).cb->index=(*iter).offset;
	        (**str).cb->current=(cb_name*) iter;
	        return CBSUCCESS;
	      }
	    }
	    iter = (cb_name *) (*iter).next;
	  }
	  //(**str).cb->index=(**str).cb->contentlen;
	  (**str).cb->index = (**str).cb->contentlen - (**str).ahd.bytesahead;  // 5.9.2013
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
	  *chroffset = (**cbs).cb->index - 1;
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

	  // jos tulokseksi tulee esim. kaksi space/tab:ia, ei poista enaa toista kertaa
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
	  *chroffset = (**cbs).cb->index - 1 - (**cbs).ahd.bytesahead; // Correct offset
	  //fprintf(stderr,"\nchr: [%c] chroffset:%i buffers offset:%i.", (int) *chr, (int) *chroffset, (int) ((**cbs).cb->index-1));
	  return err;
	}else if( (**cbs).ahd.ahead > 0){ // return previously read character
	  err = cb_fifo_get_chr( &(**cbs).ahd, &(*chr), &storedbytes );
	  *chroffset = (**cbs).cb->index - 1 - (**cbs).ahd.bytesahead;
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
int  cb_set_cursor_match_length(CBFILE **cbs, unsigned char **name, int *namelength, int matchlen){
	int indx=0, chrbufindx=0, err=CBSUCCESS, bufsize=0;
	unsigned char *ucsname = NULL; 
	bufsize = *namelength; bufsize = bufsize*4;
	ucsname = (unsigned char*) malloc( sizeof(char)*( 1 + bufsize ) );
	if( ucsname==NULL ){ return CBERRALLOC; }
	ucsname[bufsize]='\0';
	for( indx=0; indx<*namelength && err==CBSUCCESS; ++indx ){
	  err = cb_put_ucs_chr( (unsigned long int)(*name)[indx], &ucsname, &chrbufindx, bufsize);
	}
	err = cb_set_cursor_match_length_ucs( &(*cbs), &ucsname, &chrbufindx, matchlen);
	free(ucsname);
	return err;
}
int  cb_set_cursor(CBFILE **cbs, unsigned char **name, int *namelength){
	return cb_set_cursor_match_length( &(*cbs), &(*name), &(*namelength), *namelength );
}

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
	err = cb_set_cursor_match_length( &(*cbs), &name, &namelength, -1 ); // no match
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
	if( ret==CBSUCCESS || ret==CBSTREAM || ret==CBMATCH || ret==CBMATCHLENGTH){ // returns only CBSUCCESS or CBSTREAM
	  /*
	   * Copy current name to new ucsname */
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
	return cb_set_cursor_match_length_ucs( &(*cbs), &(*ucsname), &(*namelength), *namelength );
}
int  cb_set_cursor_match_length_ucs(CBFILE **cbs, unsigned char **ucsname, int *namelength, int matchlen){
	int  err=CBSUCCESS, cis=CBSUCCESS, ret=CBNOTFOUND;
	int  index=0, buferr=CBSUCCESS; 
	unsigned long int chr=0, chprev=CBRESULTEND;
	long int chroffset=0;
	unsigned char charbuf[CBNAMEBUFLEN+1]; // array 30.8.2013
	unsigned char *charbufptr = NULL;
	cb_name *fname = NULL;
	char atvalue=0; 
        unsigned long int ch3prev=CBRESULTEND+2, ch2prev=CBRESULTEND+1;

#ifdef CBSTATETOPOLOGY
	int openpairs=0;
#endif
	if( cbs==NULL || *cbs==NULL )
	  return CBERRALLOC;
	chprev=(**cbs).bypass+1; // 5.4.2013

	// 1) Search table and set cbs.cb.index if name is found
	//err = cb_set_to_name( &(*cbs), &(*ucsname), *namelength ); // 27.8.2013
	err = cb_set_to_name( &(*cbs), &(*ucsname), *namelength, matchlen ); // 4.11.2013
	if(err==CBSUCCESS){
	  //fprintf(stderr,"\nName found from list.");
	  if( (*(**cbs).cb).buflen > ( (*(*(**cbs).cb).current).length+(*(*(**cbs).cb).current).offset ) ){
	    ret = CBSUCCESS;
	    goto cb_set_cursor_ucs_return;
	  }else
	    fprintf(stderr,"\ncb_set_cursor: Found name but it's length is over the buffer length.\n");
	}else if(err==CBNAMEOUTOFBUF){
	  fprintf(stderr,"\ncb_set_cursor: Found old name out of cache and stream is allready passed by,\n");
	  fprintf(stderr,"\n               set cursor as stream and beginning to search the same name again.\n");
	  // but search again the same name; return CBNAMEOUTOFBUF;
	}

	// 2) Search stream, save name to buffer,
	//    write name, '=', data and '&' to buffer and update cb_name chain
	if(*ucsname==NULL)
	  return CBERRALLOC;

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

	// Allocate new name
cb_set_cursor_alloc_name:
	index=0;
	err = cb_allocate_name(&fname);
	if(err!=CBSUCCESS){  return err; } // 30.8.2013

	// Search for new name
	// ...& - ignore and set to start
	// ...= - save any new name and compare it to 'name', if match, return
	err = cb_search_get_chr( &(*cbs), &chr, &chroffset); // 1.9.2013
	while( err<CBERROR && err!=CBSTREAMEND && index < (CBNAMEBUFLEN-3) && buferr <= CBSUCCESS){ 

	  cb_automatic_encoding_detection( &(*cbs) ); // set encoding automatically if it's set

#ifdef CBSTATETOPOLOGY
	  if(chprev!=(**cbs).bypass && chr==(**cbs).rstart && openpairs!=0){ // count of rstarts can be greater than the count of rends
	     ++openpairs; 
	  }else if(chprev!=(**cbs).bypass && chr==(**cbs).rstart && openpairs==0){ // outermost pair
             ++openpairs;
#elif CBSTATEFUL
	  if(chprev!=(**cbs).bypass && chr==(**cbs).rstart && atvalue!=1){ // '=', save name, 13.4.2013, do not save when = is in value
#else
	  if(chprev!=(**cbs).bypass && chr==(**cbs).rstart){ // '=', save name, 5.4.2013
#endif
	    atvalue=1;

	    //fprintf(stderr, "\ncb_set_cursor_ucs: '=', index=%i, contentlen=%i", (int) (*(**cbs).cb).index, (int) (*(**cbs).cb).contentlen );
	    //fprintf(stderr, ", ahead=%i, bytesahead=%i. \n", (int) (**cbs).ahd.ahead, (int) (**cbs).ahd.bytesahead );

	    if( buferr==CBSUCCESS )
	      buferr = cb_save_name_from_charbuf( &(*cbs), &(*fname), chroffset, &charbufptr, index);
	    if(buferr==CBNAMEOUTOFBUF || buferr>=CBNEGATION){
	      fprintf(stderr, "\ncb_set_cursor_ucs: cb_save_name_from_ucs returned %i.", buferr);
	    }

	    if(err==CBSTREAM){ // Set length of namepair to indicate out of buffer.
	      cb_remove_name_from_stream( &(*cbs) );
	      fprintf(stderr, "\ncb_set_cursor: name was out of memory buffer.\n");
	    }

	    if((**cbs).cf.caseinsensitive==1) // 27.8.2013
	      cis = cb_compare_rfc2822( &(*ucsname), *namelength, &(*fname).namebuf, (*fname).namelen, matchlen ); // matchlen 4.11.2013
	    else
	      cis = cb_compare( &(*ucsname), *namelength, &(*fname).namebuf, (*fname).namelen, matchlen ); // matchlen 4.11.2013
	    //if( cis == CBMATCH ){ // 30.3.2013
	    if( cis == CBMATCH || cis == CBMATCHLENGTH ){ // 4.11.2013
	      //(**cbs).cb->index = chroffset; // cursor at rstart, 1.9.2013 (pois: 6.9.2013 tai *chroffset+1)
	      (**cbs).cb->index = (**cbs).cb->contentlen - (**cbs).ahd.bytesahead; // cursor at rstart, 6.9.2013 (this line can be removed)
	      //if( (**cbs).cf.searchmethod == CBSEARCHNEXTNAMES ) // pois 6.11.2013, sama kuin cb_set_name:ssa 
	      if( (*(**cbs).cb).last != NULL ) // matchcount, this is first match, matchcount becomes 1, 25.8.2013
	        (*(*(**cbs).cb).last).matchcount++; 
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
#ifdef CBSTATETOPOLOGY
          }else if(chprev!=(**cbs).bypass && chr==(**cbs).rend){ // '&', start new name
	    if(openpairs>=1) --openpairs; // (reader must read similarly, with openpairs count or otherwice)
#else
	  }else if( chprev!=(**cbs).bypass && chr==(**cbs).rend ){ // '&', start new name
#endif
	    atvalue=0; cb_free_fname(&fname); fname=NULL;
 	    goto cb_set_cursor_alloc_name;
	  }else if(chprev==(**cbs).bypass && chr==(**cbs).bypass){ // change \\ to one '\'
	    chr=(**cbs).bypass+1; // any char not '\'
#ifdef CBSTATEFUL
	  }else if(atvalue==1){ // Do not save data between '=' and '&' 
	    /* This state is to use indefinite length values. Index does not increase and
	     * unordered values length is not bound to length CBNAMEBUFLEN. 
             * ( name1=name2=value& becomes name1 once, otherwice (was) name1name2. )
	     */
	    ;
#endif
#ifdef CBSTATETOPOLOGY
	  }else if(atvalue==1){ // Do not save data between '=' and '&' 
	    /* The same as in CBSTATEFUL. Saves space. */
	    ;
#endif
	  }else{ // save character to buffer
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
        cb_free_fname(&fname);
	if( ret==CBSUCCESS || ret==CBSTREAM || ret==CBMATCH || ret==CBMATCHLENGTH) // match* on turha, aina stream tai success
	  if( (**cbs).cb!=NULL && (*(**cbs).cb).current!=NULL ){
	    (*(*(**cbs).cb).current).lasttimeused = (signed long int) time(NULL); // last time used in seconds
	    if( (*(*(**cbs).cb).current).firsttimefound == -1 )
	      (*(*(**cbs).cb).current).firsttimefound = (*(*(**cbs).cb).current).lasttimeused; // first time found
	  }
	return ret;

        return CBNOTFOUND;
}

int cb_save_name_from_charbuf(CBFILE **cbs, cb_name *fname, long int offset, unsigned char **charbuf, int index){
	      unsigned long int cmp=0x61;
	      int indx=0, buferr=CBSUCCESS, err=CBSUCCESS;

	      (*fname).namelen = index; // tuleeko olla vasta if:n jalkeen
	      (*fname).offset = offset;

	      //fprintf(stderr,"\n name:[");
	      //cb_print_ucs_chrbuf(&(*charbuf), index, CBNAMEBUFLEN);
	      //fprintf(stderr,"]");

              if(index<(*fname).buflen){ 
                (*fname).namelen=0; // 6.4.2013
	        while(indx<index && buferr==CBSUCCESS){ // 4 bytes (UCS word)
                  buferr = cb_get_ucs_chr( &cmp, &(*charbuf), &indx, CBNAMEBUFLEN); // 30.8.2013
                  if( buferr==CBSUCCESS ){
                    if( cmp==(**cbs).cstart ){ // Comment
                      while( indx<index && cmp!=(**cbs).cend && buferr==CBSUCCESS ){ // 2.9.2013
                        buferr = cb_get_ucs_chr( &cmp, &(*charbuf), &indx, CBNAMEBUFLEN); // 30.8.2013
                      }
                    }
cb_set_cursor_ucs_removals:
                    if( (**cbs).cf.removewsp==1 && WSP( cmp ) ){ // WSP:s between value and name
                      while( indx<index && WSP( cmp ) && buferr==CBSUCCESS ){
                        buferr = cb_get_ucs_chr( &cmp, &(*charbuf), &indx, CBNAMEBUFLEN); // 7.9.2013
                      }
	            }
	            if( (**cbs).cf.removecrlf==1 ){ // Removes every CR and LF characters between value and name
                      while( indx<index && ( CR( cmp ) || LF( cmp ) ) && buferr==CBSUCCESS ){
                        buferr = cb_get_ucs_chr( &cmp, &(*charbuf), &indx, CBNAMEBUFLEN); // 7.9.2013
                      }
	              if( (**cbs).cf.removewsp==1 && WSP( cmp ) )
	                goto cb_set_cursor_ucs_removals;
		    }

                    /* Write name */
                    if( cmp!=(**cbs).cend && buferr==CBSUCCESS){ // Name, 28.8.2013
                      if( ! NAMEXCL( cmp ) ) // bom should be replaced if it's not first in stream
                        buferr = cb_put_ucs_chr( cmp, &(*fname).namebuf, &(*fname).namelen, (*fname).buflen );
                    }
                  }
                }
              }else{
	        err = CBNAMEOUTOFBUF;
	      }
	      if(err!=CBNAMEOUTOFBUF)
	        err = cb_put_name(&(*cbs), &fname); // (last in list), jos nimi on verrattavissa, tallettaa nimen ja offsetin
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
	if(cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || (*(**cbs).cb).current==NULL )
	  return CBERRALLOC;
	(*(*(**cbs).cb).current).length =  (*(**cbs).cb).buflen;
	return CBSUCCESS;
}