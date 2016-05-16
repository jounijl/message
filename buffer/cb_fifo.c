/* 
 * Library to read and write streams. 
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
#include <string.h> // memset

#include "../include/cb_buffer.h"

// Debug
int  cb_fifo_print_buffer(cb_ring *cfi, char priority){
        int i=0, err=0, chrsize=0; unsigned long int chr = ' ';
        if(cfi==NULL || (*cfi).buf==NULL)
          return CBERRALLOC;
        for( i = (*cfi).ahead; i > 0; i-=4 ){
          err = cb_fifo_get_chr(&(*cfi), &chr, &chrsize);
          cb_clog( priority, CBSUCCESS, "%c", (int) chr);
          err = cb_fifo_put_chr(&(*cfi), chr, chrsize);
        }
	return err;
}
// Debug
int  cb_fifo_print_counters(cb_ring *cfi, char priority){
        if(cfi==NULL || (*cfi).buf==NULL)
          return CBERRALLOC;
        cb_clog( priority, CBSUCCESS, "\nahead:            %i", (*cfi).ahead );
        cb_clog( priority, CBSUCCESS, "\nbytesahead:       %i", (*cfi).bytesahead );
        cb_clog( priority, CBSUCCESS, "\nbuflen:           %i", (*cfi).buflen );
        cb_clog( priority, CBSUCCESS, "\nsizeslen:         %i", (*cfi).sizeslen );
        cb_clog( priority, CBSUCCESS, "\nfirst:            %i", (*cfi).first );
        cb_clog( priority, CBSUCCESS, "\nlast:             %i", (*cfi).last );
        cb_clog( priority, CBSUCCESS, "\nstreamstart:      %i", (*cfi).streamstart );
        cb_clog( priority, CBSUCCESS, "\nstreamstop:       %i", (*cfi).streamstop );
        return CBSUCCESS;
}

int  cb_fifo_init_counters(cb_ring *cfi){
	int err = CBSUCCESS;
        if( cfi==NULL )
          return CBERRALLOC;
        (*cfi).ahead=0;
        (*cfi).bytesahead=0;
        (*cfi).first=0;
        (*cfi).last=0;
        (*cfi).streamstart=-1;
        (*cfi).streamstop=-1;
        if((*cfi).buf==NULL){
	  (*cfi).buflen=0;
          err = CBERRALLOC;
	}else{
	  memset( &((*cfi).buf[0]), (int) 0x20, (size_t) CBREADAHEADSIZE);
	  (*cfi).buf[CBREADAHEADSIZE]='\0';
	  (*cfi).buflen=CBREADAHEADSIZE;
	}
        if((*cfi).storedsizes==NULL){
	  (*cfi).sizeslen=0;
          err = CBERRALLOC;
	}else{
	  memset( &((*cfi).storedsizes[0]), (int) 0x20, (size_t) CBREADAHEADSIZE);
	  (*cfi).storedsizes[CBREADAHEADSIZE]='\0';
	  (*cfi).sizeslen=CBREADAHEADSIZE;
	}
        return err;
}
int  cb_fifo_set_stream(cb_ring *cfi){
        if(cfi==NULL || (*cfi).buf==NULL)
          return CBERRALLOC;
        if((*cfi).streamstart==-1)
	  (*cfi).streamstart=(*cfi).ahead;
	return CBSUCCESS;	
}
int  cb_fifo_set_endchr(cb_ring *cfi){
        if(cfi==NULL || (*cfi).buf==NULL)
          return CBERRALLOC;
        if((*cfi).streamstop==-1)
	  (*cfi).streamstop=(*cfi).ahead;
	return CBSUCCESS;
}
int  cb_fifo_revert_chr(cb_ring *cfi, unsigned long int *chr, int *chrsize){ 
        int err=CBSUCCESS; unsigned long int chrs=0;
	int tmp1=0, tmp2=0;
	unsigned char *ptr = NULL;
        if(cfi==NULL || (*cfi).buf==NULL)
          return CBERRALLOC;
        if((*cfi).ahead>0){
	  if( (*cfi).last<=3 && (*cfi).first>3)
	    tmp1 = (*cfi).buflen - 4 + (*cfi).last ;
	  else if( (*cfi).last>3 && (*cfi).ahead!=0)
	    tmp1 = (*cfi).last-4;
	  tmp2 = tmp1;

	  ptr = &((*cfi).storedsizes[0]);
          err = cb_get_ucs_chr( &chrs, &ptr, &tmp1, (*cfi).sizeslen );
	  *chrsize = (int) chrs;

	  ptr = &((*cfi).buf[0]);
          err = cb_get_ucs_chr( &(*chr), &ptr, &tmp2, (*cfi).buflen );
          if( err == CBSUCCESS ){
            (*cfi).ahead-=4;
	    (*cfi).bytesahead -= (int) *chrsize;
	    (*cfi).last-=4;
	    if( (*cfi).last<0 )
	      (*cfi).last = (*cfi).buflen + (*cfi).last;
	  }
	  if(err==CBSUCCESS && (*cfi).streamstart==0)
	    return CBSTREAM;
	  if(err==CBSUCCESS && (*cfi).streamstop==0)
	    return CBSTREAMEND;
          return err;
        }else
          return CBEMPTY;
}
int  cb_fifo_get_chr(cb_ring *cfi, unsigned long int *chr, int *chrsize){
        int err=CBSUCCESS, tmp=0; unsigned long int chrs=0;
	unsigned char *ptr = NULL;
        if(cfi==NULL || (*cfi).buf==NULL)
          return CBERRALLOC;
        if((*cfi).ahead>0){
	  tmp = (*cfi).first;
	  ptr = &((*cfi).buf[0]);
          err = cb_get_ucs_chr( &(*chr), &ptr, &(*cfi).first, (*cfi).buflen );
          if( (*cfi).first >= (*cfi).buflen-3 ) (*cfi).first=0;
          (*cfi).ahead-=4;
	  if((*cfi).streamstart!=-1 && (*cfi).streamstart!=0)
	    (*cfi).streamstart-=4;
	  if((*cfi).streamstop!=-1 && (*cfi).streamstop!=0)
	    (*cfi).streamstop-=4;

	  ptr = &((*cfi).storedsizes[0]);
          err = cb_get_ucs_chr( &chrs, &ptr, &tmp, (*cfi).sizeslen );
	  *chrsize = (int) chrs;
	  (*cfi).bytesahead -= (int) *chrsize; // alustettu nolla tulee takaisin
	  if(err==CBSUCCESS && (*cfi).streamstart==0)
	    return CBSTREAM;
	  if(err==CBSUCCESS && (*cfi).streamstop==0)
	    return CBSTREAMEND;
          return err;
        }else
          return CBEMPTY;
}
int  cb_fifo_put_chr(cb_ring *cfi, unsigned long int chr, int chrsize){
        int err=CBSUCCESS, tmp=0;
	unsigned char *ptr = NULL;
        if(cfi==NULL || (*cfi).buf==NULL)
          return CBERRALLOC;
        if((*cfi).ahead>=(*cfi).buflen-4)
          return CBBUFFULL;
        if((*cfi).last>=(*cfi).buflen-3)
          (*cfi).last = 0;
	tmp = (*cfi).last;
	ptr = &((*cfi).buf[0]);
        err = cb_put_ucs_chr( chr, &ptr, &(*cfi).last, (*cfi).buflen);

        (*cfi).ahead+=4;

	ptr = &((*cfi).storedsizes[0]);
        err = cb_put_ucs_chr( (unsigned long int) chrsize, &ptr, &tmp, (*cfi).sizeslen );
        (*cfi).bytesahead += (int) chrsize;

        return err;
}

/*
 * 4-byte characterbuffer. 
 */
int cb_print_ucs_chrbuf(char priority, unsigned char **chrbuf, int namelen, int buflen){
        int index=0, err=CBSUCCESS;
        unsigned long int chr=0x20; // 11.12.2014
        if(chrbuf==NULL && *chrbuf==NULL){ return CBERRALLOC; }
	if(namelen<4){ // 4.7.2015, 4 bytes minimum
	   cb_clog( priority, CBSUCCESS, "(err CBEMPTY)");
	   return CBEMPTY; // 4.7.2015
	}
        //for(index=0;index<namelen && index<buflen && err==CBSUCCESS;){
        for(index=0;index<namelen && index<(buflen-3) && err==CBSUCCESS && buflen>3; ){ // 3.4.2016
           err = cb_get_ucs_chr(&chr, &(*chrbuf), &index, buflen);
	   if( chr==0x0000 && err==CBSUCCESS ){ // null terminator
             cb_clog( priority, CBSUCCESS, "(null)");
	   }else if(err!=CBSUCCESS){
	     cb_clog( priority, CBSUCCESS, "(err %i)", err);
	     if(err>=CBERROR) // 4.7.2015
		return err; // 4.7.2015
	   }else{
             cb_clog( priority, CBSUCCESS, "%c", (unsigned char) chr ); // %wc is missing, %C prints null wide character
             //cb_clog( priority, CBSUCCESS, "(0x%.2x)", (unsigned int) chr ); // 21.10.2015
             //cb_clog( priority, CBSUCCESS, "(0x%lx)", chr ); // 8.6.2014
             //cb_clog( priority, CBSUCCESS, "(%#x)", (unsigned int) chr ); // 10.6.2014
	   }
        }
        return CBSUCCESS;
}

int  cb_put_ucs_chr(unsigned long int chr, unsigned char **chrbuf, int *bufindx, int buflen){
        if( chrbuf==NULL || *chrbuf==NULL || bufindx==NULL){   return CBERRALLOC; }
        if( *bufindx>(buflen-4) ){                             return CBBUFFULL; }
        (*chrbuf)[*bufindx]   = (unsigned char) (chr>>24);
        (*chrbuf)[*bufindx+1] = (unsigned char) (chr>>16);
        (*chrbuf)[*bufindx+2] = (unsigned char) (chr>>8);
        (*chrbuf)[*bufindx+3] = (unsigned char) chr;
        *bufindx+=4;
        //cb_clog( CBLOGDEBUG, CBNEGATION, "chrbuf put: [%lx]", chr);
        return CBSUCCESS;
}

int  cb_get_ucs_chr(unsigned long int *chr, unsigned char **chrbuf, int *bufindx, int buflen){
        static unsigned long int N = 0xFFFFFF00; // 0xFFFFFFFF - 0xFF = 0xFFFFFF00
        if( chr==NULL || bufindx==NULL || chrbuf==NULL || *chrbuf==NULL ){     return CBERRALLOC; }
        if( *bufindx>(buflen-4) ){               return CBARRAYOUTOFBOUNDS; }
        *chr = (unsigned long int) (*chrbuf)[*bufindx]; *chr = (*chr<<8) & N; *bufindx+=1; 
        *chr = *chr | (unsigned long int) (*chrbuf)[*bufindx]; *chr = (*chr<<8) & N; *bufindx+=1;
        *chr = *chr | (unsigned long int) (*chrbuf)[*bufindx]; *chr = (*chr<<8) & N; *bufindx+=1;
        *chr = *chr | (unsigned long int) (*chrbuf)[*bufindx]; *bufindx+=1;
        //cb_clog( CBLOGDEBUG, CBNEGATION, "[%lx]", *chr);
        return CBSUCCESS;
} 

// 16.3.2014, a regexp block, not tested yet (16.3.2014)
int  cb_copy_ucs_chrbuf_from_end(unsigned char **chrbuf, int *bufindx, int buflen, int countfromend ){
	int cpyblk=0, indx=0;
	if( bufindx==NULL || chrbuf==NULL || *chrbuf==NULL ){     return CBERRALLOC; }
	if(*bufindx >= countfromend)
	  cpyblk = countfromend; // multiple of 4, usually the buffer is full before copying
	else
	  cpyblk = 4; // multiple of 4, this case is not yet tested 17.3.2014
	for(indx=0; indx<countfromend && indx<buflen && indx<=2147483646 && *bufindx<buflen; ){
	  memmove( &( (*chrbuf)[indx] ), &( (*chrbuf)[*bufindx-cpyblk] ), (size_t) cpyblk ); // memmove is nondestructive when overlapping
	  indx += cpyblk;
	  *bufindx -= cpyblk;
	}
	if( indx >= countfromend )
	  *bufindx = countfromend;
	else
	  *bufindx = indx;
	return CBSUCCESS;
}

