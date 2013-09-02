/* 
 * Library to read and write streams. Searches valuepairs locations to list. Uses different character encodings.
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
#include "../include/cb_buffer.h"

int  cb_put_name(CBFILE **str, cb_name **cbn);
int  cb_set_to_name(CBFILE **str, unsigned char **name, int namelen);
int  cb_set_search_method(CBFILE **buf, char method); // CBSEARCH*
int  cb_search_get_chr( CBFILE **cbs, cb_ring *readahead, unsigned long int *chr, long int *chroffset);
int  cb_unfolding_get_chr(CBFILE **cbs, cb_ring *readahead, unsigned long int *chr, long int *chroffset);
int  cb_save_name_from_charbuf(CBFILE **cbs, cb_name *fname, long int offset, unsigned char **charbuf, int index);
int  cb_automatic_encoding_detection(CBFILE **cbs);

#define SPTABCR( x )        ( (x)==0x0D || (x)==0x20 || (x)==0x09 )

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
            (*(*(**str).cb).current).length = (*(**str).cb).index - (*(*(**str).cb).current).offset;
          (*(**str).cb).last    = &(* (cb_name*) (*(*(**str).cb).last).next );
          err = cb_allocate_name( &(*(**str).cb).last ); if(err!=CBSUCCESS){ return err; }
          (*(*(**str).cb).current).next = &(*(*(**str).cb).last); // previous
          (*(**str).cb).current = &(*(*(**str).cb).last);
          ++(*(**str).cb).namecount;
          if( (*(**str).cb).contentlen >= (*(**str).cb).buflen )
            (*(*(**str).cb).current).length = (*(**str).cb).buflen; // Largest 'known' value
        }else{
          // err = cb_allocate_name( & (cb_name*) (*(**str).cb).name ); if(err!=CBSUCCESS){ return err; }
          err = cb_allocate_name( &(*(**str).cb).name ); if(err!=CBSUCCESS){ return err; } // 17.3.2013
          (*(**str).cb).last    = &(* (cb_name*) (*(**str).cb).name); // last
          (*(**str).cb).current = &(* (cb_name*) (*(**str).cb).name); // current
          (*(*(**str).cb).current).next = NULL; // firsts next
          (*(**str).cb).namecount=1;
        }
        err = cb_copy_name(cbn, &(*(**str).cb).last); if(err!=CBSUCCESS){ return err; }
        (*(*(**str).cb).last).next = NULL;
        return CBSUCCESS;
}

int  cb_set_to_name(CBFILE **str, unsigned char **name, int namelen){ 
	cb_name *iter = NULL; int err=CBSUCCESS;

	if(*str!=NULL && (**str).cb != NULL ){
	  iter = &(*(*(**str).cb).name);
	  while(iter != NULL){
	    if((**str).cf.caseinsensitive==1)
	      err = cb_compare_rfc2822( &(*iter).namebuf, (*iter).namelen, &(*name), namelen );
	    else
	      err = cb_compare( &(*iter).namebuf, (*iter).namelen, &(*name), namelen );
	    if( err == CBMATCH ){ 
	      /*
	       * 20.8.2013:
	       * If searching of multiple same names is needed (in buffer also), do not return 
	       * allready matched name. Instead, increase names matchcount (or set to 1 if it 
	       * becomes 0) and search the next same name.
	       */
	      if( (**str).cf.searchmethod==CBSEARCHNEXTNAMES ){
	        (*iter).matchcount++; if( (*iter).matchcount==0 ){ (*iter).matchcount+=2; }
	      }
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
	  (**str).cb->index=(**str).cb->contentlen;
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

int  cb_search_get_chr( CBFILE **cbs, cb_ring *readahead, unsigned long int *chr, long int *chroffset ){
	int err = CBSUCCESS, bytecount=0, storedbytes=0;
	if(cbs==NULL || *cbs==NULL || readahead==NULL || chroffset==NULL || chr==NULL)
	  return CBERRALLOC;
	if((**cbs).cf.unfold==1)
	  return cb_unfolding_get_chr( &(*cbs), &(*readahead), &(*chr), &(*chroffset) );
	else{
	  err = cb_get_chr( &(*cbs), &(*chr), &bytecount, &storedbytes );
	  *chroffset = (**cbs).cb->index - 1;
	  return err;
	}
	return CBERROR;
}

int  cb_unfolding_get_chr(CBFILE **cbs, cb_ring *readahead, unsigned long int *chr, long int *chroffset){
	int bytecount=0, storedbytes=0, tmp=0; int err=CBSUCCESS;
	unsigned long int empty=0x61;
	if(cbs==NULL || *cbs==NULL || readahead==NULL || chroffset==NULL || chr==NULL)
	  return CBERRALLOC;

	if((*readahead).ahead == 0){
	  err = cb_get_chr( &(*cbs), &(*chr), &bytecount, &storedbytes );
	  if(err==CBSTREAM){  cb_fifo_set_stream( &(*readahead) ); }
	  if(err==CBSTREAMEND){  return CBSTREAMEND; }

	  // jos tulokseksi tulee esim. kaksi space/tab:ia, ei poista enaa toista kertaa
cb_unfolding_try_another:
	  if( SP( *chr ) && err<CBNEGATION ){
	    cb_fifo_put_chr( &(*readahead), *chr, storedbytes);
	    err = cb_get_chr( &(*cbs), &(*chr), &bytecount, &storedbytes );
	    if(err==CBSTREAM){  cb_fifo_set_stream( &(*readahead) ); }
	    if( SP( *chr) ){
	      cb_fifo_revert_chr( &(*readahead), &empty, &tmp );
	      goto cb_unfolding_try_another;
	    }else{
      	      cb_fifo_put_chr( &(*readahead), *chr, storedbytes);
	      cb_fifo_get_chr( &(*readahead), &(*chr), &storedbytes);
	    } 
	  }else if( CR( *chr ) && err<CBNEGATION ){
	    cb_fifo_put_chr( &(*readahead), *chr, storedbytes);
	    err = cb_get_chr( &(*cbs), &(*chr), &bytecount, &storedbytes );
	    if(err==CBSTREAM){  cb_fifo_set_stream( &(*readahead) ); }

	    if( LF( *chr ) && err<CBNEGATION ){ 
	      cb_fifo_put_chr( &(*readahead), *chr, storedbytes);
	      err = cb_get_chr( &(*cbs), &(*chr), &bytecount, &storedbytes );
	      if(err==CBSTREAM){  cb_fifo_set_stream( &(*readahead) ); }

	      if( SP( *chr ) && err<CBNEGATION ){
	        cb_fifo_revert_chr( &(*readahead), &empty, &tmp ); // CR
	        cb_fifo_revert_chr( &(*readahead), &empty, &tmp ); // LF
	        goto cb_unfolding_try_another;
	      }else if( err<CBNEGATION ){ 
	        cb_fifo_put_chr( &(*readahead), *chr, storedbytes ); // save 'any', 3:rd in store
	        cb_fifo_get_chr( &(*readahead), &(*chr), &storedbytes); // return first in fifo (CR)
	      }
	    }else{
	      cb_fifo_put_chr( &(*readahead), *chr, storedbytes ); // save 'any', 2:nd in store
	      cb_fifo_get_chr( &(*readahead), &(*chr), &storedbytes); // return first in fifo (CR)
	    }
	  }else{
	    cb_fifo_put_chr( &(*readahead), *chr, storedbytes ); // save 'any', 1:nd in store
	    cb_fifo_get_chr( &(*readahead), &(*chr), &storedbytes); // return first in fifo (CR)
	  }
	  if(err>=CBNEGATION){
            fprintf(stderr,"\ncb_unfolding_get_chr: read error %i, chr:[%c].", err, (int) *chr); 
	    cb_fifo_print_counters(&(*readahead));
	  }
	  *chroffset = (**cbs).cb->index - 1 - (*readahead).bytesahead; // Correct offset
	  fprintf(stderr,"\nchr: [%c] offset:%i.", (int) *chr, (int) *chroffset);
	  return err;
	}else if((*readahead).ahead > 0){ // return previously read character
	  err = cb_fifo_get_chr( &(*readahead), &(*chr), &storedbytes );
	  *chroffset = (**cbs).cb->index - 1 - (*readahead).bytesahead;
	  fprintf(stderr,"\nchr: [%c], from ring, offset:%i.", (int) *chr, (int) *chroffset );
	  if(err>=CBNEGATION){
	    fprintf(stderr,"\ncb_unfolding_get_chr: read error %i, ahead=%i, bytesahead:%i,\
	       storedbytes=%i, chr=[%c].", err, (*readahead).ahead, (*readahead).bytesahead, storedbytes, (int) *chr); 
	    cb_fifo_print_counters(&(*readahead));
	  }
	  return err; // returns CBSTREAM
	}else
	  return CBARRAYOUTOFBOUND;
        return CBERROR;
}

/*
 * Name has one byte for each character.
 */
int  cb_set_cursor(CBFILE **cbs, unsigned char **name, int *namelength){
	int indx=0, chrbufindx=0, err=CBSUCCESS, bufsize=0;
	unsigned char *ucsname = NULL; 
	bufsize = *namelength; bufsize = bufsize*4;
	ucsname = (unsigned char*) malloc( sizeof(char)*( 1 + bufsize ) );
	if( ucsname==NULL ){ return CBERRALLOC; }
	ucsname[bufsize]='\0';
	for( indx=0; indx<*namelength && err==CBSUCCESS; ++indx ){
	  err = cb_put_ucs_chr( (unsigned long int)(*name)[indx], &ucsname, &chrbufindx, bufsize);
	}
	err = cb_set_cursor_ucs( &(*cbs), &ucsname, &chrbufindx);
	free(ucsname);
	return err;
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
	int  err=CBSUCCESS, cis=CBSUCCESS;
	int index=0, buferr=CBSUCCESS; 
	unsigned long int chr=0, chprev=CBRESULTEND;
	long int chroffset=0;
	unsigned char charbuf[CBNAMEBUFLEN+1]; // array 30.8.2013
	unsigned char *charbufptr = NULL;
	cb_name *fname = NULL;
	char atvalue=0;
	cb_ring readahead;
        unsigned long int ch3prev=CBRESULTEND+2, ch2prev=CBRESULTEND+1;

#ifdef CBSTATETOPOLOGY
	int openpairs=0;
#endif
	if( cbs==NULL || *cbs==NULL )
	  return CBERRALLOC;
	chprev=(**cbs).bypass+1; // 5.4.2013

	// 1) Search table and set cbs.cb.index if name is found
	err = cb_set_to_name( &(*cbs), &(*ucsname), *namelength ); // 27.8.2013
	if(err==CBSUCCESS){
	  if( (*(**cbs).cb).buflen > ( (*(*(**cbs).cb).current).length+(*(*(**cbs).cb).current).offset ) )
	    return CBSUCCESS;
	  else
	    fprintf(stderr,"\ncb_set_cursor: Found name but it's length is over the buffer length.\n");
	}else if(err==CBNAMEOUTOFBUF){
	  fprintf(stderr,"\ncb_set_cursor: Found old name out of cache and stream is allready passed by,\n");
	  fprintf(stderr,"\n               set cursor as stream and beginning to search the same name again.\n");
	  // but search again the same name; return CBNAMEOUTOFBUF;
	}
	// Set cursor to the end to search next names
	(*(**cbs).cb).index = (*(**cbs).cb).contentlen;

	// 2) Search stream, save name to buffer,
	//    write name, '=', data and '&' to buffer and update cb_name chain
	if(*ucsname==NULL)
	  return CBERRALLOC;

	// Initialize memory characterbuffer and its counters
	memset( &(charbuf[0]), (int) 0x20, (size_t) CBNAMEBUFLEN);
	if(charbuf==NULL){  return CBERRALLOC; }
	charbuf[CBNAMEBUFLEN]='\0';
	charbufptr = &charbuf[0];

	// Initialize readahead counters and memory (used in unfolding)
	cb_fifo_init_counters(&readahead);

	// Allocate new name
cb_set_cursor_alloc_name:
	index=0; 
	err = cb_allocate_name(&fname);
	if(err!=CBSUCCESS){  return err; } // 30.8.2013

#ifdef TMP        
        cb_print_counters(&(*cbs));
        cb_print_names(&(*cbs));
#endif

	// Search for new name
	// ...& - ignore and set to start
	// ...= - save any new name and compare it to 'name', if match, return
	err = cb_search_get_chr( &(*cbs), &readahead, &chr, &chroffset); // 1.9.2013
	while( err<CBERROR && err!=CBSTREAMEND && index < (CBNAMEBUFLEN-3) && buferr <= CBSUCCESS){ 

	  // Set encoding automatically if it's set
	  if((**cbs).encoding==CBENCAUTO || ( (**cbs).encoding==CBENCPOSSIBLEUTF16LE && (*(**cbs).cb).contentlen == 4 ) )
	    cb_automatic_encoding_detection( &(*cbs) );

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

	      // tuleeko seuraavan ehtona olla JOS(buferr==0)
	      buferr = cb_save_name_from_charbuf( &(*cbs), &(*fname), chroffset, &charbufptr, index);
	      if(buferr==CBNAMEOUTOFBUF || buferr>=CBNEGATION){
	        fprintf(stderr, "\ncb_set_cursor_ucs: cb_save_name_from_ucs returned %i.", buferr);
	      }

	      if(err==CBSTREAM){ // Set length of namepair to indicate out of buffer.
		cb_remove_name_from_stream( &(*cbs) );
	        fprintf(stderr, "\ncb_set_cursor: name was out of memory buffer.\n");
	      }

	      if((**cbs).cf.caseinsensitive==1) // 27.8.2013
	        cis = cb_compare_rfc2822( &(*ucsname), *namelength, &(*fname).namebuf, (*fname).namelen );
	      else
	        cis = cb_compare( &(*ucsname), *namelength, &(*fname).namebuf, (*fname).namelen ); 
  	      if( cis == CBMATCH ){ // 30.3.2013
	        // (**cbs).cb->index = (**cbs).cb->contentlen; // cursor
	        // (**cbs).cb->index = (**cbs).cb->contentlen - readahead.bytesahead; // cursor at rstart
	        (**cbs).cb->index = chroffset; // cursor at rstart, 1.9.2013
	        if( (**cbs).cf.searchmethod == CBSEARCHNEXTNAMES ) // matchcount, this is first match, matchcount becomes 1, 25.8.2013
	          if( (*(**cbs).cb).last != NULL )
	            (*(*(**cbs).cb).last).matchcount++; 
                cb_free_fname(&fname); // free everything and return, 30.8.2013
	        if(err==CBSTREAM)
	          return CBSTREAM;  // cursor set, preferably the first time (remember to use cb_remove_name_from_stream)
		else
		  return CBSUCCESS; // cursor set
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
	      chr=' '; // any char not '\'
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
          if((**cbs).cf.rfc2822==1)
            if( ch3prev==0x0D && ch2prev==0x0A && chprev==0x0D && chr==0x0A ){ // cr lf x 2
              if( (*(**cbs).cb).offsetrfc2822 == 0 )
                (*(**cbs).cb).offsetrfc2822 = chroffset; // 1.9.2013, offset set at last new line character
                //(*(**cbs).cb).offsetrfc2822 = (*(**cbs).cb).contentlen; // offset set at last new line character
              cb_free_fname(&fname); // 30.8.2013
              return CB2822HEADEREND;
            }

	  ch3prev = ch2prev; ch2prev = chprev; chprev = chr;
	  err = cb_search_get_chr( &(*cbs), &readahead, &chr, &chroffset); // 1.9.2013
	    
	}
        cb_free_fname(&fname);

        return CBNOTFOUND;
}

int cb_save_name_from_charbuf(CBFILE **cbs, cb_name *fname, long int offset, unsigned char **charbuf, int index){
	      unsigned long int cmp=0x61;
	      int indx=0, buferr=CBSUCCESS, err=CBSUCCESS;

	      (*fname).namelen = index; // tuleeko olla vasta if:n jalkeen
	      (*fname).offset = offset;

	      fprintf(stderr,"\n name:[");
	      cb_print_ucs_chrbuf(&(*charbuf), index, CBNAMEBUFLEN);
	      fprintf(stderr,"]");

              if(index<(*fname).buflen){ 
                (*fname).namelen=0; // 6.4.2013
	        while(indx<index && buferr==CBSUCCESS){ // 4 bytes (UCS word)
                  buferr = cb_get_ucs_chr( &cmp, &(*charbuf), &indx, CBNAMEBUFLEN); // 30.8.2013
                  if( buferr==CBSUCCESS ){
                    if( cmp==(**cbs).cstart ){ // Comment
                      //while( indx<index && cmp!=(**cbs).cend && cmp!=(**cbs).rend && buferr==CBSUCCESS ){
                      while( indx<index && cmp!=(**cbs).cend && buferr==CBSUCCESS ){ // 2.9.2013
                        buferr = cb_get_ucs_chr( &cmp, &(*charbuf), &indx, CBNAMEBUFLEN); // 30.8.2013
                      }
                    }
                    /* Write name */
                    if( cmp!=(**cbs).cend && buferr==CBSUCCESS){ // Name, 28.8.2013
	              if( ! ( (**cbs).cf.rfc2822 && CTL( cmp ) ) )
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
	  //fprintf(stderr,"\n at automatic encoding detection");
	  if( (*(**cbs).cb).contentlen == 4 || (*(**cbs).cb).contentlen == 2 || (*(**cbs).cb).contentlen == 3 ){ // UTF-32, 4; UTF-16, 2; UTF-8, 3;
	    // 32 is multiple of 16. Testing it again results correct encoding without loss of bytes or errors.
	    enctest = cb_bom_encoding(cbs);
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
