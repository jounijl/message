/* 
 * Library to read and write streams. Valuepair indexing with different character encodings.
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
#include "../include/cb_buffer.h"

/*
 Todo:
 x Add comment from # to '\n' . # may be first character of file or 
   in name or immediately after &[<sp>] but -> not in value <-
 x Not into this: 
   - Nesting =<text>& => <value1>=<text><value1.1>=<text2>&<text3>&
     ( cb_buffer only shows the start of <text> and has nothing to do with the
     rest of the <text>.)
     - Make nesting to a reader funktion and writer function.
       - Number showing nesting level and subfiles for example
         for subnests.
   x As in stanza or http, two '\n'-characters ends value (now default is '&')
     ( not into this, same explanation as before )
     - use lwspfolding and rend='\n'
   - Text to file functions (somewhere else than in this file):
     - Fixed length blocks:
       - put_next_pair
       - get_next_pair
 x Get and set = and & signs
 x Document on using the library in www
   - html meta tags
 - Testing plan 
 - Bug tracking
 x Change control characters to 32-bit size integer to allow different encodings
 x Change valuename and its comparing buffer to 32-bit
   x in set_cursor
 x cb_compare:en pituuksien vertailu myos, lisays 2009 poistaa nimista SP:t ja rivinvaihdot
   joten ehka pituuksia voi vertailla riittaa.
 x Ohjelma joka kirjoittaa UTF:aa luettuaan UCS:aa ja pain vastoin
 - cb_get_chr tunnistamaan useamman eri koodauksen virheellinen tavu, CBNOTUTF jne.
   erilaiset yhteen funktiossa cb_get_chr ja siita yleisemmin CBENOTINENCODING tms.
 x Testaukseen, kokeillaan muuttaa arvosta muuttujan arvoa. Laitetaan arvoksi nimiparin nakoinen yhdistelma. 
   (Tama tietenkin korvaantuu ohjelmoijan lisaamilla ohitusmerkeilla, http:ssa ei tarvitse.)
 x CBSTATETOPOLOGY
   - nice names
 x character encoding tests, multibyte and utf-32
 - boundary tests
 - stress test
 - cb_put_name cb_name malloc nimen koon perusteella ennen listaan tallettamista
 - Nimet sisaltavan valilyonteja ja tabeja. Ohjeen mukaan ne poistetaan ennen vertausta.
 */

int  cb_put_name(CBFILE **str, cb_name **cbn);
int  cb_set_to_name(CBFILE **str, unsigned char **name, int namelen);
int  cb_get_char_read_block(CBFILE **cbf, char *ch);
int  cb_set_type(CBFILE **buf, char type);
int  cb_set_search_method(CBFILE **buf, char method); // CBSEARCH*

int  cb_compare_chr(CBFILE **cbs, int index, unsigned long int chr); // not tested

#ifdef TMP
//unsigned long long int mallocs_fname=0, mallocs_charbuf=0, mallocs_buffer=0, mallocs_cbfile=0;
//unsigned long long int frees_fname=0, frees_charbuf=0, frees_buffer=0, frees_cbfile=0;
#endif

/*
 * Debug
 */
int  cb_print_names(CBFILE **str){ 
	cb_name *iter = NULL; int names=0;
	fprintf(stderr, "\n cb_print_names: \n");
	if(str!=NULL){
	  if( *str !=NULL){
            iter = &(*(*(**str).cb).name);
            if(iter!=NULL){
              do{
	        ++names;
	        fprintf(stderr, " [%i/%li] name [", names, (*(**str).cb).namecount );
                if(iter!=NULL){
	            cb_print_ucs_chrbuf(&(*iter).namebuf, (*iter).namelen, (*iter).buflen);
	        }
                fprintf(stderr, "] offset [%li] length [%i]", (*iter).offset, (*iter).length);
                fprintf(stderr, " matchcount [%li]\n", (*iter).matchcount);
                iter = &(* (cb_name *) (*iter).next );
              }while( iter != NULL );
              return CBSUCCESS;
	    }
	  }
	}else{
	  fprintf(stderr, "\n str was null "); 
	}
        return CBERRALLOC;
}
// Debug
void cb_print_counters(CBFILE **cbf){
        if(cbf!=NULL && *cbf!=NULL)
          fprintf(stderr, "\nnamecount:%li \t index:%li \t contentlen:%li \t  buflen:%li", (*(**cbf).cb).namecount, \
             (*(**cbf).cb).index, (*(**cbf).cb).contentlen, (*(**cbf).cb).buflen );
}

int cb_print_ucs_chrbuf(unsigned char **chrbuf, int namelen, int buflen){
	int index=0, err=CBSUCCESS;
	unsigned long int chr=0;
	if(chrbuf==NULL && *chrbuf==NULL){ return CBERRALLOC; }
	for(index=0;index<namelen && index<buflen && err==CBSUCCESS;){
	   err = cb_get_ucs_chr(&chr, &(*chrbuf), &index, buflen);
	   fprintf(stderr, "%C", (unsigned int) chr );
	}
	return CBSUCCESS;
}

int  cb_put_ucs_chr(unsigned long int chr, unsigned char **chrbuf, int *bufindx, int bufsize){
        if( chrbuf==NULL || *chrbuf==NULL || bufindx==NULL){   return CBERRALLOC; }
        if( *bufindx>(bufsize-4) ){                            return CBBUFFULL; }
        (*chrbuf)[*bufindx]   = (unsigned char) (chr>>24);
        (*chrbuf)[*bufindx+1] = (unsigned char) (chr>>16);
        (*chrbuf)[*bufindx+2] = (unsigned char) (chr>>8);
        (*chrbuf)[*bufindx+3] = (unsigned char) chr;
        *bufindx+=4;
        //fprintf(stderr, "chrbuf put: [%lx]", chr);
        return CBSUCCESS;
}

int  cb_get_ucs_chr(unsigned long int *chr, unsigned char **chrbuf, int *bufindx, int bufsize){
        static unsigned long int N = 0xFFFFFF00; // 0xFFFFFFFF - 0xFF = 0xFFFFFF00
        if( chrbuf==NULL || *chrbuf==NULL ){     return CBERRALLOC; }
        if( *bufindx>(bufsize-4) ){              return CBNOTFOUND; }
        *chr = (unsigned long int) (*chrbuf)[*bufindx]; *chr = (*chr<<8) & N; *bufindx+=1; 
        *chr = *chr | (unsigned long int) (*chrbuf)[*bufindx]; *chr = (*chr<<8) & N; *bufindx+=1;
        *chr = *chr | (unsigned long int) (*chrbuf)[*bufindx]; *chr = (*chr<<8) & N; *bufindx+=1;
        *chr = *chr | (unsigned long int) (*chrbuf)[*bufindx]; *bufindx+=1;
        //fprintf(stderr, "[%lx]", *chr);
        return CBSUCCESS;
} 

int  cb_compare_chr(CBFILE **cbs, int index, unsigned long int chr){ // Never used or tested
	int indx=0, indx2=0;
	unsigned long int tmp=0;
	if(cbs==NULL || *cbs==NULL){ return CBERRALLOC; }
	// From left to right, lsb first
	// Get: msb first, Put: msb first, From left to right, lsb first
	tmp = chr;
	for(indx=0; indx<(**cbs).encodingbytes && indx<32; ++index){
	  for(indx2=((**cbs).encodingbytes-indx); indx2>1; --indx2){
	    tmp = tmp>>8;
	  }
	  if( (index+indx)<(*(**cbs).cb).buflen && (index+indx)<(*(**cbs).cb).contentlen ){ // index count: [LSB][MSB 0][LSB][MSB 1][][2]...
	    if( (*(**cbs).cb).buf[(index+indx)] != (unsigned char) tmp )
	      return CBNOTFOUND;
	  }else
	    return CBNOTFOUND;
	  tmp = chr;
	}
	return CBMATCH;
}

int  cb_compare(unsigned char **name1, int len1, unsigned char **name2, int len2){
	int index=0, err=0, num=0;
	if( *name1==NULL || *name2==NULL )
	  return CBERRALLOC;
 	if( len1==0 || len2==0 )
	  return CBNOTFOUND;

//	fprintf(stderr,"cb_compare: name1=%s name2=%s\n", *name1, *name2);

	num=len1;
	if(len1>len2)
	  num=len2;
	for(index=0;index<num;++index)
	  if((*name1)[index]!=(*name2)[index]){
	    index=num+7; err=CBNOTFOUND;
	  }else{
	    if(len1==len2)
	      err=CBMATCH;
	    else
	      err=CBMATCHPART; // 30.3.2013, was: match, not notfound
	  }
	return err;
}

int  cb_set_to_name(CBFILE **str, unsigned char **name, int namelen){ 
	cb_name *iter = NULL; 

	if(*str!=NULL && (**str).cb != NULL ){
	  iter = &(*(*(**str).cb).name);
	  while(iter != NULL){
	    if( cb_compare( &(*iter).namebuf, (*iter).namelen, &(*name), namelen ) == CBMATCH ){ 
	      /*
	       * 20.8.2013:
	       * If searching of multiple same names is needed (in buffer also), do not return 
	       * allready matched name. Instead, increase names matchcount (or set to 1 if it 
	       * becomes 0) and search the next same name.
	       */
#ifdef TMP
              fprintf(stderr,"\nMATCH, matchcount was %li.", (*iter).matchcount );
#endif
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
#ifdef TMP
                fprintf(stderr,"\nMATCH, new matchcount=%li", (*iter).matchcount );
#endif
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

int  cb_copy_name( cb_name **from, cb_name **to ){
	int index=0;
	if( from!=NULL && *from!=NULL && to!=NULL && *to!=NULL ){
	  for(index=0; index<(**from).namelen && index<(**to).buflen ; ++index)
	    (**to).namebuf[index] = (**from).namebuf[index];
	  (**to).namelen = index;
	  (**to).offset  = (**from).offset;
          (**to).length  = (**from).length; // 20.8.2013
          (**to).matchcount = (**from).matchcount; // 20.8.2013
	  (**to).next   = (**from).next; // This knows where this points to, so it only can be copied but references to this can not
	  return CBSUCCESS;
	}
	return CBERRALLOC;
}

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

int  cb_allocate_name(cb_name **cbn){ 
	int err=0;
	*cbn = (cb_name*) malloc(sizeof(cb_name));
	if(*cbn==NULL)
	  return CBERRALLOC;
	(**cbn).namebuf = (unsigned char*) malloc(sizeof(char)*(CBNAMEBUFLEN+1));
	if((**cbn).namebuf==NULL)
	  return CBERRALLOC;
	for(err=0;err<CBNAMEBUFLEN;++err)
	  (**cbn).namebuf[err]=' ';
	(**cbn).namebuf[CBNAMEBUFLEN]='\0';
	(**cbn).buflen=CBNAMEBUFLEN;
	(**cbn).namelen=0;
	(**cbn).offset=0; 
	(**cbn).length=0;
	(**cbn).matchcount=0;
	(**cbn).next=NULL;
	return CBSUCCESS;
}

int  cb_set_cstart(CBFILE **str, unsigned long int cstart){ // comment start
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).cstart=cstart;
	return CBSUCCESS;
}
int  cb_set_cend(CBFILE **str, unsigned long int cend){ // comment end
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).cend=cend;
	return CBSUCCESS;
}
int  cb_set_rstart(CBFILE **str, unsigned long int rstart){ // value start
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).rstart=rstart;
	return CBSUCCESS;
}
int  cb_set_rend(CBFILE **str, unsigned long int rend){ // value end
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).rend=rend;
	return CBSUCCESS;
}
int  cb_set_bypass(CBFILE **str, unsigned long int bypass){ // bypass , new - added 19.12.2009
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).bypass=bypass;
	return CBSUCCESS;
}

int  cb_allocate_cbfile(CBFILE **str, int fd, int bufsize, int blocksize){
	unsigned char *blk = NULL; 
	return cb_allocate_cbfile_from_blk(str, fd, bufsize, &blk, blocksize);
}

int  cb_allocate_cbfile_from_blk(CBFILE **str, int fd, int bufsize, unsigned char **blk, int blklen){ 
	int err=CBSUCCESS;
	*str = (CBFILE*) malloc(sizeof(CBFILE));
	if(*str==NULL)
	  return CBERRALLOC;
	(**str).cb = NULL; (**str).blk = NULL;
        (**str).cf.type=CBCFGSTREAM; // default
        //(**str).cf.type=CBCFGFILE; // first test was ok
        //(**str).cf.searchmethod=CBSEARCHNEXTNAMES; // default, finds first, third, second, .. not ready
        (**str).cf.searchmethod=CBSEARCHUNIQUENAMES;
	(**str).rstart=CBRESULTSTART;
	(**str).rend=CBRESULTEND;
	(**str).bypass=CBBYPASS;
	(**str).cstart=CBCOMMENTSTART;
	(**str).cend=CBCOMMENTEND;
	(**str).encodingbytes=CBDEFAULTENCODINGBYTES;
	(**str).encoding=CBDEFAULTENCODING;
	(**str).fd = dup(fd);
	if((**str).fd == -1){ err = CBERRFILEOP; (**str).cf.type=CBCFGBUFFER; } // 20.8.2013

	err = cb_allocate_buffer(&(**str).cb, bufsize);
	if(err==-1){ return CBERRALLOC;}
	if(*blk==NULL){
	  err = cb_allocate_buffer(&(**str).blk, blklen);
	}else{
	  err = cb_allocate_buffer(&(**str).blk, 0); // blk
	  if(err==-1){ return CBERRALLOC;}
	  free( (*(**str).blk).buf );
	  (*(**str).blk).buf = &(**blk);
	  (*(**str).blk).buflen = blklen;
	  (*(**str).blk).contentlen = blklen;
	}
	if(err==-1){ return CBERRALLOC;}
	return CBSUCCESS;
}

int  cb_allocate_buffer(cbuf **cbf, int bufsize){ 
	int err=0;
	*cbf = (cbuf *) malloc(sizeof(cbuf));
	if(cbf==NULL)
	  return CBERRALLOC;
	(**cbf).buf = (unsigned char *) malloc(sizeof(char)*(bufsize+1));
	if( (**cbf).buf == NULL )
	  return CBERRALLOC;

	for(err=0;err<bufsize;++err)
	  (**cbf).buf[err]=' ';
	(**cbf).buf[bufsize]='\0';
	(**cbf).buflen=bufsize;
	(**cbf).contentlen=0;
	(**cbf).namecount=0;
	(**cbf).index=0;
#ifdef CB2822MESSAGE
        (**cbf).offset2822=0;
#endif
	(**cbf).name=NULL;
	(**cbf).current=NULL;
	(**cbf).last=NULL;
	return CBSUCCESS;
}

int  cb_reinit_cbfile(CBFILE **buf){
	int err=CBSUCCESS;
	if( buf==NULL || *buf==NULL ){ return CBERRALLOC; }
	err = cb_reinit_buffer(&(**buf).cb);
	err = cb_reinit_buffer(&(**buf).blk);
	return err;
}

int  cb_free_cbfile(CBFILE **buf){ 
	int err=CBSUCCESS; 
	cb_reinit_buffer(&(**buf).cb); // free names
	if((*(**buf).cb).buf!=NULL)
	  free((**buf).cb->buf); // free buffer data
	free((**buf).cb); // free buffer
	if((*(**buf).blk).buf!=NULL)
          free((**buf).blk->buf); // free block data
	free((**buf).blk); // free block
	if((**buf).cf.type!=CBCFGBUFFER){ // 20.8.2013
	  err = close((**buf).fd); // close stream
	  if(err==-1){ err=CBERRFILEOP;}
	}
	free(*buf); // free buf
	return err;
}
int  cb_free_buffer(cbuf **buf){
        int err=CBSUCCESS;
        err = cb_reinit_buffer( &(*buf) );
        free( (**buf).buf );
        free( *buf );
        return err;
}
int  cb_free_fname(cb_name **name){
        int err=CBSUCCESS;
        if(name!=NULL && *name!=NULL)
          free((**name).namebuf);
        else
          err=CBERRALLOC; 
        free(*name);
        return err;
}
int  cb_reinit_buffer(cbuf **buf){ // free names and init
	cb_name *name = NULL;
	cb_name *nextname = NULL;
	if(buf!=NULL && *buf!=NULL){
	  (**buf).index=0;
	  (**buf).contentlen=0;
	  name = (**buf).name;
	  while(name != NULL){
	    nextname = &(* (cb_name*) (*name).next);
            cb_free_fname(&name);
	    name = &(* nextname);
	  }
	  (**buf).namecount=0;
	}
	free(name); free(nextname);
	(**buf).name=NULL; // 1.6.2013
	(**buf).current=NULL; // 1.6.2013
	(**buf).last=NULL; // 1.6.2013
	return CBSUCCESS;
}

int  cb_use_as_buffer(CBFILE **buf){
        return cb_set_type(&(*buf), (char) CBCFGBUFFER);
}
int  cb_use_as_file(CBFILE **buf){
        return cb_set_type(&(*buf), (char) CBCFGFILE);
}
int  cb_use_as_stream(CBFILE **buf){
        return cb_set_type(&(*buf), (char) CBCFGSTREAM);
}
int  cb_set_type(CBFILE **buf, char type){ // 20.8.2013
	if(buf!=NULL){
	  if((*buf)!=NULL){
	    (**buf).cf.type=type; // 20.8.2013
	    return CBSUCCESS;
	  }
	}
	return CBERRALLOC;
}
int  cb_set_to_multiple_same_names(CBFILE **cbf){
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
int  cb_get_char_read_block(CBFILE **cbf, char *ch){
	ssize_t sz=0; // int err=0;
	cblk *blk = NULL; 
	blk = (**cbf).blk;

	if(blk!=NULL){
	  if( ( (*blk).index < (*blk).contentlen ) && ( (*blk).contentlen <= (*blk).buflen ) ){
	    // return char
	    *ch = (*blk).buf[(*blk).index];
	    ++(*blk).index;
	  }else if((**cbf).cf.type!=CBCFGBUFFER){ // 20.8.2013
	    // read a block and return char
	    sz = read((**cbf).fd, (*blk).buf, (ssize_t)(*blk).buflen);
	    (*blk).contentlen = (int) sz;
	    if( 0 < (int) sz ){ // read more than zero bytes
	      (*blk).contentlen = (int) sz;
	      (*blk).index = 0;
	      *ch = (*blk).buf[(*blk).index];
	      ++(*blk).index;
	    }else{ // error or end of file
//	      (*blk).contentlen = 0; (*blk).index = 0; *ch = ' ';
	      (*blk).index = 0; *ch = ' ';
	      return CBSTREAMEND;
	    }
	  }else{ // use as block
	    return CBSTREAMEND;
	  }
	  return CBSUCCESS;
	}else
	  return CBERRALLOC;
}

int  cb_flush(CBFILE **cbs){
	int err=CBSUCCESS; // indx=0;
	if(*cbs!=NULL){
 	  if((**cbs).cf.type!=CBCFGBUFFER){
	    if((**cbs).blk!=NULL){
	      if((*(**cbs).blk).contentlen <= (*(**cbs).blk).buflen )
	        err = (int) write((**cbs).fd, (*(**cbs).blk).buf, (size_t) (*(**cbs).blk).contentlen);
	      else
	        err = (int) write((**cbs).fd, (*(**cbs).blk).buf, (size_t) (*(**cbs).blk).buflen); // indeksi nollasta ?
	      if(err<0){
	        err = CBERRFILEWRITE ; 
	      }else{
	        err = CBSUCCESS;
	        (*(**cbs).blk).contentlen=0;
	      }
	      return err;
	    }
	  }else
	    return CBUSEDASBUFFER;
	}
	return CBERRALLOC;
}

/* added 15.12.2009, not yet tested */
// Write to block to read again (in regular expression matches for example).
int  cb_write_to_block(CBFILE **cbs, unsigned char *buf, int size){ 
	int indx=0;
	if(*cbs!=NULL && buf!=NULL && size!=0)
	  if((**cbs).blk!=NULL){
	    for(indx=0; indx<size; ++indx){
	      if((*(**cbs).blk).contentlen < (*(**cbs).blk).buflen ){
	        (*(**cbs).blk).buf[(*(**cbs).blk).contentlen] = buf[indx];
	        ++(*(**cbs).blk).contentlen;
	      }else{
	        return CBBUFFULL;
	      }
	    }
	    return CBSUCCESS;
	  }
	return CBERRALLOC;
}

int  cb_write(CBFILE **cbs, unsigned char *buf, int size){ 
	int err=0;
	if(*cbs!=NULL && buf!=NULL){
	  if((**cbs).blk!=NULL){
	    for(err=0; err<size; ++err){
	      //cb_put_ch(cbs,&buf[err]);
	      cb_put_ch(cbs,buf[err]); // 12.8.2013
	    }
	    err = cb_flush(cbs);
	    return err;
	  }
	}
	return CBERRALLOC;
}

int  cb_write_cbuf(CBFILE **cbs, cbuf *cbf){
	if( cbs==NULL || *cbs==NULL || cbf==NULL || (*cbf).buf==NULL ){ return CBERRALLOC; }
	return cb_write(cbs, &(*(*cbf).buf), (*cbf).contentlen);
}

int  cb_put_ch(CBFILE **cbs, unsigned char ch){ // 12.8.2013
	int err=CBSUCCESS;
	if(*cbs!=NULL)
	  if((**cbs).blk!=NULL){
cb_put_ch_put:
	    if((*(**cbs).blk).contentlen < (*(**cbs).blk).buflen ){
              (*(**cbs).blk).buf[(*(**cbs).blk).contentlen] = ch; // 12.8.2013
	      ++(*(**cbs).blk).contentlen;
	    }else if((**cbs).cf.type!=CBCFGBUFFER){ // 20.8.2013
	      err = cb_flush(cbs); // new block
	      goto cb_put_ch_put;
	    }else if((**cbs).cf.type==CBCFGBUFFER){ // 20.8.2013
	      return CBBUFFULL;
	    }
	    return err;
	  }
	return CBERRALLOC;
}

int  cb_get_ch(CBFILE **cbs, unsigned char *ch){ // Copy ch to buffer and return it until end of buffer
	char chr=' '; int err=0;
	if( cbs!=NULL && *cbs!=NULL ){
	// cb_get_ch_read_buffer
	  if( (*(**cbs).cb).index < (*(**cbs).cb).contentlen){
	     ++(*(**cbs).cb).index;
	     *ch = (*(**cbs).cb).buf[ (*(**cbs).cb).index ];
	     return CBSUCCESS;
	  }
	  // cb_get_ch_read_stream_to_buffer
	  *ch=' ';
	  // get char
	  err = cb_get_char_read_block(cbs, &chr);
	  if( err == CBSTREAMEND || err >= CBERROR ){ return err; }
	  // copy char if name-value -buffer is not full
	  if((**cbs).cb->contentlen < (**cbs).cb->buflen ){
	    if( chr != EOF ){
	      (**cbs).cb->buf[(**cbs).cb->contentlen] = chr;
	      ++(**cbs).cb->contentlen;
	      (**cbs).cb->index = (**cbs).cb->contentlen;
	      *ch = chr;
	      return CBSUCCESS;
	    }else{
	      *ch=chr;
	      (**cbs).cb->index = (**cbs).cb->contentlen;
	      return CBSTREAMEND;
	    }
	  }else{
	    *ch=chr;
	    if( chr == EOF )
	      return CBSTREAMEND;
	    if((**cbs).cb->contentlen > (**cbs).cb->buflen )
	      return CBSTREAM;
	    return err; // at edge of buffer and stream
	  }
	}
	return CBERRALLOC;
}

int  cb_get_buffer(cbuf *cbs, unsigned char **buf, int *size){ 
	int from=0, to=0;
	to = *size;
	return cb_get_buffer_range(cbs,buf,size,&from,&to);
}

// Allocates new buffer
int  cb_get_buffer_range(cbuf *cbs, unsigned char **buf, int *size, int *from, int *to){ 
	int index=0;
	if( cbs==NULL || (*cbs).buf==NULL ){ return CBERRALLOC;}
	*buf = (unsigned char *) malloc( sizeof(char)*( *size+1 ) );
	if(*buf==NULL){ fprintf(stderr, "\ncb_get_sub_buffer: malloc returned null."); return CBERRALLOC; }
	(*buf)[(*size)] = '\0';
	for(index=0;(index<(*to-*from) && index<(*size) && (index+*from)<(*cbs).contentlen); ++index){
	  (*buf)[index] = (*cbs).buf[index+*from];
	}
	*size = index;
	return CBSUCCESS;
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
	int  err=CBSUCCESS, enctest=CBDEFAULTENCODING;
	int index=0, indx=0, indx2=0, bytecount=0, storedbytes=0, buferr=CBSUCCESS;
	unsigned long int chr=0, cmp=0, chprev=CBRESULTEND, ch2prev=CBRESULTEND+1;
	unsigned char *charbuf   = NULL;
	cb_name *fname  = NULL;
	char atvalue=0;
#ifdef CB2822MESSAGE
        unsigned long int ch3prev=CBRESULTEND+2;
#endif
#ifdef CBSTATETOPOLOGY
	int openpairs=0;
#endif
	if( cbs==NULL || *cbs==NULL )
	  return CBERRALLOC;
	chprev=(**cbs).bypass+1; // 5.4.2013

#ifdef TMP        
        if(cbs!=NULL && *cbs!=NULL && (**cbs).cb !=NULL)
          fprintf(stderr, "\nnamecount: %li", (*(**cbs).cb).namecount);
#endif

	// 1) Search table and set cbs.cb.index if name is found
	err = cb_set_to_name(cbs,(unsigned char **)ucsname,*namelength);
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

	// Allocate buffer for characters
	charbuf = (unsigned char *)malloc(sizeof(unsigned char)*(CBNAMEBUFLEN+1));
	if(charbuf==NULL){  return CBERRALLOC; }
	charbuf[CBNAMEBUFLEN]='\0';

	// Allocate new name
cb_set_cursor_alloc_name:
	index=0; 
	err = cb_allocate_name(&fname);
	if(err!=CBSUCCESS){  free(charbuf); return err; }

#ifdef TMP        
        cb_print_counters(&(*cbs));
        cb_print_names(&(*cbs));
#endif

	// Search for new name
	// ...& - ignore and set to start
	// ...= - save any new name and compare it to 'name', if match, return
	err = cb_get_chr(cbs,&chr,&bytecount,&storedbytes); // storedbytes uusi
	while( err<CBERROR && err!=CBSTREAMEND && index < (CBNAMEBUFLEN-3) && buferr == CBSUCCESS){ 

	  //
	  // Automatic encoding detection if it's set
	  if((**cbs).encoding==CBENCAUTO || ( (**cbs).encoding==CBENCPOSSIBLEUTF16LE && (*(**cbs).cb).contentlen == 4 ) ){
	    fprintf(stderr,"\n at automatic encoding detection");
	    if( (*(**cbs).cb).contentlen == 4 || (*(**cbs).cb).contentlen == 2 || (*(**cbs).cb).contentlen == 3 ){ // UTF-32, 4; UTF-16, 2; UTF-8, 3;
	      // 32 is multiple of 16. Testing it again results correct encoding without loss of bytes or errors.
	      enctest = cb_bom_encoding(cbs);
	      if( enctest==CBENCUTF8 || enctest==CBENCUTF16BE || enctest==CBENCUTF16LE || enctest==CBENCPOSSIBLEUTF16LE || enctest==CBENCUTF32LE || enctest==CBENCUTF32BE ){
	        cb_set_encoding( &(*cbs), enctest);
	      }else{
	        cb_set_encoding( &(*cbs), CBDEFAULTENCODING );
	      }
	    }
	  }
	  // End of automatic encoding detection


#ifdef CBSTATETOPOLOGY
	  if(chprev!=(**cbs).bypass && chr==(**cbs).rstart && openpairs!=0) // count of rstarts can be greater than the count of rends
	     ++openpairs; 
	  else if(chprev!=(**cbs).bypass && chr==(**cbs).rstart && openpairs==0){ // outermost pair
             ++openpairs;
#elif CBSTATEFUL
	  if(chprev!=(**cbs).bypass && chr==(**cbs).rstart && atvalue!=1){ // '=', save name, 13.4.2013, do not save when = is in value
#else
	  if(chprev!=(**cbs).bypass && chr==(**cbs).rstart){ // '=', save name, 5.4.2013
#endif
	      atvalue=1;
	      (*fname).namelen = index; indx2=0;
              if(index<(*fname).buflen){ // Remove SP:s and tabs in name
	        indx=0;
                (*fname).namelen=0; // 6.4.2013
	        while(indx<index && buferr==CBSUCCESS){ // 4 bytes (UCS word)
                  buferr = cb_get_ucs_chr( &cmp, &charbuf, &indx, CBNAMEBUFLEN);
                  if( buferr==CBSUCCESS ){
                    if( cmp==(**cbs).cstart ){ // Comment
                      while( indx<index && cmp!=(**cbs).cend && cmp!=(**cbs).rend && buferr==CBSUCCESS ){
                        buferr = cb_get_ucs_chr( &cmp, &charbuf, &indx, CBNAMEBUFLEN);
                      }
                    }
                    if( ! NAMEXCL( cmp ) && cmp!=(**cbs).cend && buferr==CBSUCCESS){ // Name
                      buferr = cb_put_ucs_chr( cmp, &(*fname).namebuf, &(*fname).namelen, (*fname).buflen );
                    }
                  }
                }
	      }
	      (*fname).offset= ( (**cbs).cb->contentlen - 1 );
	      cb_put_name(cbs, &fname); // (last in list)
//
	      if(err==CBSTREAM){ // Set length of namepair to indicate out of buffer.
		cb_remove_name_from_stream(cbs);
	        fprintf(stderr, "\ncb_set_cursor: name was out of memory buffer.\n");
	      }

  	      if(cb_compare((unsigned char **)ucsname, *namelength, &(*fname).namebuf, (*fname).namelen) == CBMATCH){ // 30.3.2013
	        (**cbs).cb->index = (**cbs).cb->contentlen; // cursor
	        if( (**cbs).cf.searchmethod == CBSEARCHNEXTNAMES ) // matchcount, this is first match, matchcount becomes 1, 25.8.2013
	          if( (*(**cbs).cb).last != NULL )
	            (*(*(**cbs).cb).last).matchcount++; 
                free(charbuf); charbuf=NULL; cb_free_fname(&fname); // free everything and return
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
	      buferr = cb_put_ucs_chr(chr, &charbuf, &index, CBNAMEBUFLEN); // 5.4.2013
	  }

          /* Automatic stop at "Internet Message Format" header if it's set */
#ifdef CB2822MESSAGE
          ch3prev=ch2prev; ch2prev=chprev;
          if( ch3prev==0x0D && ch2prev==0x0A && chprev==0x0D && chr==0x0A ){ // cr lf x 2
            if( (*(**cbs).cb).offset2822 == 0 )
              (*(**cbs).cb).offset2822 = (*(**cbs).cb).contentlen; // offset set at last new line character
            free(charbuf); cb_free_fname(&fname);
            return CB2822HEADEREND;
          }
#endif
	  chprev = chr;
	  err = cb_get_chr(cbs,&chr,&bytecount,&storedbytes);
	}
        free(charbuf); cb_free_fname(&fname);
#ifdef TMP        
        if(cbs!=NULL && *cbs!=NULL && (**cbs).cb !=NULL)
          fprintf(stderr, "\nnamecount: %li", (*(**cbs).cb).namecount);
#endif
        return CBNOTFOUND;
}

int  cb_remove_name_from_stream(CBFILE **cbs){
	if(cbs==NULL || *cbs==NULL || (**cbs).cb==NULL || (*(**cbs).cb).current==NULL )
	  return CBERRALLOC;
	(*(*(**cbs).cb).current).length =  (*(**cbs).cb).buflen;
	return CBSUCCESS;
}
