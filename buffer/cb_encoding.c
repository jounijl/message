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

void cb_bytecount(unsigned long int *chr, int *bytecount);

int  cb_get_multibyte_ch(CBFILE **cbs, unsigned long int *ch);
int  cb_put_multibyte_ch(CBFILE **cbs, unsigned long int  ch);
int  cb_multibyte_write(CBFILE **cbs, char *buf, int size); // Convert char to long int, put them and flush

int  cb_get_chr(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes){
       int err=CBSUCCESS;
       if(cbs==NULL||*cbs==NULL){ return CBERRALLOC; }
        if((**cbs).encoding==CBENCAUTO) // auto
          return cb_get_ucs_ch(&(*cbs), &(*chr), &(*bytecount), &(*storedbytes) );
        if((**cbs).encoding==CBENCUTF8) // utf-8
         return cb_get_ucs_ch(&(*cbs), &(*chr), &(*bytecount), &(*storedbytes) );
        if((**cbs).encoding==CBENC1BYTE){ // 1 byte or more
          *bytecount=(**cbs).encodingbytes;
          *storedbytes=(**cbs).encodingbytes;
          return cb_get_multibyte_ch(&(*cbs), &(*chr));
        }
        if((**cbs).encoding==CBENC2BYTE || (**cbs).encoding==CBENC4BYTE){ // 2 bytes or more, endianness
          *bytecount=(**cbs).encodingbytes;
          *storedbytes=(**cbs).encodingbytes;
          err = cb_get_multibyte_ch(&(*cbs), &(*chr)); 
#ifdef BIGENDIAN
          // In stream allways in little endian form if LE machine or if BIGENDIAN is defined in BE machines
          if( (**cbs).encoding==CBENC4BYTE )
            *chr = cb_reverse_four_bytes(*chr);
          if( (**cbs).encoding==CBENC2BYTE )
            *chr = cb_reverse_two_bytes(*chr);
#endif
          return err;
        }
        if((**cbs).encoding==CBENCUTF32LE || (**cbs).encoding==CBENCUTF32BE) // utf-32
          return cb_get_utf32_ch(&(*cbs), &(*chr), &(*bytecount), &(*storedbytes) );
        if((**cbs).encoding==CBENCUTF16LE || (**cbs).encoding==CBENCUTF16BE || (**cbs).encoding==CBENCPOSSIBLEUTF16LE) // utf-16
          return cb_get_utf16_ch(&(*cbs), &(*chr), &(*bytecount), &(*storedbytes) );
        return CBNOENCODING;
}
//int  cb_put_chr(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes){
int  cb_put_chr(CBFILE **cbs, unsigned long int chr, int *bytecount, int *storedbytes){ // 12.8.2013
        unsigned long int schr; schr = chr;
        int err=CBSUCCESS; 
        if(cbs==NULL||*cbs==NULL){ return CBERRALLOC; }
        if((**cbs).encoding==CBENCUTF8) // utf
          return cb_put_ucs_ch(cbs, &chr, &(*bytecount), &(*storedbytes) );
        if((**cbs).encoding==CBENC1BYTE){ // 1 byte
          *bytecount=(**cbs).encodingbytes;
          *storedbytes=(**cbs).encodingbytes;
          return cb_put_multibyte_ch( cbs, chr );
        }
        if((**cbs).encoding==CBENC2BYTE || (**cbs).encoding==CBENC4BYTE){ // 2 bytes or more, endianness
          *bytecount=(**cbs).encodingbytes;
          *storedbytes=(**cbs).encodingbytes;
#ifdef BIGENDIAN
          // In stream allways in little endian order if LE machine or if BIGENDIAN is defined in BE machines
          if( (**cbs).encoding==CBENC4BYTE )
            schr = cb_reverse_four_bytes(chr);
          if( (**cbs).encoding==CBENC2BYTE )
            schr = cb_reverse_two_bytes(chr);
#endif
          err = cb_put_multibyte_ch( cbs, schr );
          return err;
        }
        if((**cbs).encoding==CBENCUTF32LE || (**cbs).encoding==CBENCUTF32BE) // utf-32
          return cb_put_utf32_ch(&(*cbs), &chr, &(*bytecount), &(*storedbytes) );
        if((**cbs).encoding==CBENCUTF16LE || (**cbs).encoding==CBENCUTF16BE || (**cbs).encoding==CBENCPOSSIBLEUTF16LE) // utf-16
          return cb_put_utf16_ch(&(*cbs), &chr, &(*bytecount), &(*storedbytes) );
        return CBNOENCODING;
}

int  cb_set_encodingbytes(CBFILE **str, int bytecount){
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).encodingbytes=bytecount;
	return CBSUCCESS;
}
int  cb_set_encoding(CBFILE **str, int number){
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).encoding=number;
	if(number==CBENCAUTO) // automatic, tries bom and reverts to default if encoding is not found
	  cb_set_encodingbytes(str,1); // detection has to be at one byte
	if(number==CBENC1BYTE) // 1 byte
	  cb_set_encodingbytes(str,1); // one byte
	if(number==CBENC2BYTE) // 2 byte
	  cb_set_encodingbytes(str,2); 
	if(number==CBENCUTF8) // UTF-8
	  cb_set_encodingbytes(str,0); // zero is any length
	if(number==CBENC4BYTE) // 4 byte
	  cb_set_encodingbytes(str,4); 
	if(number==CBENCUTF16LE) // UTF-16 LE
	  cb_set_encodingbytes(str,2); // in case of third, reads one character more (not yet done 11.8.2013)
	if(number==CBENCPOSSIBLEUTF16LE) // Possible UTF-16 LE (bytecount in detection was less than 4), used as it is the same, UTF-16 LE
	  cb_set_encodingbytes(str,2); 
	if(number==CBENCUTF16BE) // UTF-16 BE
	  cb_set_encodingbytes(str,2); 
	if(number==5) // UTF-16 BE
	  cb_set_encodingbytes(str,2); 
	if(number==CBENCUTF32LE) // UTF-32 LE
	  cb_set_encodingbytes(str,4); 
	if(number==CBENCUTF32BE) // UTF-32 BE
	  cb_set_encodingbytes(str,4); 
	return CBSUCCESS;
}

int  cb_multibyte_write(CBFILE **cbs, char *buf, int size){
	int err=0, indx=0;
	if(*cbs!=NULL && buf!=NULL)
	  if((**cbs).blk!=NULL){
	    for(indx=0; indx<size && err<CBERROR; ++indx){
	      err = cb_put_multibyte_ch(cbs, (unsigned long int)buf[indx] );
	    }
	    err = cb_flush(cbs);
	    return err;
	  }
	return CBERRALLOC;
}

int  cb_get_multibyte_ch(CBFILE **cbs, unsigned long int *ch){
	int err=CBSUCCESS, index=0; unsigned char byte=0x00; // null character
	*ch = 0;
	for(index=0; index<(**cbs).encodingbytes && index<32 && err<CBERROR ;++index){
	  err = cb_get_ch(cbs, &byte);
	  *ch+=(long int)byte;
	  if(index<((**cbs).encodingbytes-1) && index<32 && err<CBERROR){ *ch=(*ch)<<8;}
	}
	return err;
}

int  cb_put_multibyte_ch(CBFILE **cbs, unsigned long int ch){
	int err=CBSUCCESS, index=0, indx=0;
	unsigned char byte='0'; unsigned long int tmp=0;
	tmp = ch;
	for(index=0; index<(**cbs).encodingbytes && index<32 && err<CBERROR ;++index){
	  for(indx=((**cbs).encodingbytes-index); indx>1; --indx){
	    tmp = tmp>>8;
	  }
	  byte = (unsigned char) tmp; tmp = ch;
	  //err  = cb_put_ch(cbs, &byte);
	  err  = cb_put_ch(cbs, byte); // 12.8.2013
	}
	return err;
}

/*
 * Put character which is in UTF-8 format. Return 'bytecount' written and err.
 * If character is not in form of UTF-8, writes only one byte. If bytecount is
 * lower than 4, only chr is used and chr_high is ignored.
 * UTF-8, rfc-3629
 */
int  cb_put_utf8_ch(CBFILE **cbs, unsigned long int *chr, unsigned long int *chr_high, int *bytecount, int *storedbytes ){
	int err=CBSUCCESS, indx=0; char byteswritten=0, utfbytes=0;
	unsigned char byte=0, byte2=0;
	unsigned long int chr_tmp=0, chr_tmp2=0;
	if(cbs==NULL || *cbs==NULL){ return CBERRALLOC; }

	if( *bytecount>4 && ( (**cbs).encodingbytes==0||(**cbs).encodingbytes>4) ){
	  // To ease the use of cb_put_utf8_ch which has the same chr_high,
	  // swap high to low and reuse chr_high as a low word and write it last
	  // if bytecount indicates that both are needed.
	  chr_tmp=*chr_high; *chr_high=*chr; *chr=chr_tmp;
	}

	while(*bytecount>byteswritten && byteswritten<6 && ( byteswritten<(**cbs).encodingbytes || (**cbs).encodingbytes==0 ) ){
	  byte2 = byte;
	  if( byteswritten<=4 ) chr_tmp2 = *chr; // low end
	  else chr_tmp2 = *chr_high; // high end
	  chr_tmp = chr_tmp2;
	  for(indx=1;indx<(*bytecount-byteswritten) && indx<4;++indx){
	    chr_tmp = chr_tmp2>>8; chr_tmp2=chr_tmp;
	  }
	  byte = (unsigned char) chr_tmp;
	  // err = cb_put_ch(cbs, &byte); byteswritten++;
	  err = cb_put_ch(cbs, byte); byteswritten++; // 12.8.2013
	  if(err>CBNEGATION){ fprintf(stderr,"\ncb_put_utf8_ch: cb_put_ch: bytecount %i error %i.", byteswritten, err); return err;}
	  if(byteswritten==1){
	        if(byteisutf8head2( byte )){  utfbytes=2;
		}else if(byteisutf8head3( byte )){  utfbytes=3;
	        }else if(byteisutf8head4( byte )){  utfbytes=4;
	        }else if(byteisutf8head5( byte )){  utfbytes=5;
	        }else if(byteisutf8head6( byte )){  utfbytes=6;
		}else if( byteisascii( byte ) ){
	  	  if( *bytecount==1 && (**cbs).encodingbytes>=0 ){
		    *bytecount = byteswritten;
	            *storedbytes = byteswritten; // uusi
	    	    return CBSUCCESS;
		  }else if( (**cbs).encodingbytes<0 || *bytecount!=1 ){
	 	    goto cb_put_utf8_ch_return_bytecount_error;
		  }
		}else if(*bytecount==1){
		  goto cb_put_utf8_ch_return_not_utf;
		}else
		  continue;
	  }else if(byteswritten==2){
		if( bytesareutf8bom( byte, byte2 ) && *bytecount==2 ){
		  return CBUTFBOM;
		}else if( utfbytes==byteswritten && ( (**cbs).encodingbytes>=byteswritten || (**cbs).encodingbytes==0 ) ){
		  *bytecount = byteswritten;
	          *storedbytes = byteswritten; // uusi
		  return CBSUCCESS;
		}else if( ! byteisutf8tail( byte ) ){
		  goto cb_put_utf8_ch_return_not_utf;
		}else if( utfbytes!=byteswritten && ( (**cbs).encodingbytes==byteswritten || *bytecount==byteswritten ) ){
		  goto cb_put_utf8_ch_return_bytecount_error;
		}
	  }else{
		if( utfbytes==byteswritten && ( (**cbs).encodingbytes>=byteswritten || (**cbs).encodingbytes==0 ) ){
		  *bytecount = byteswritten;
	          *storedbytes = byteswritten; // uusi
		  return CBSUCCESS;
		}else if( ! byteisutf8tail( byte ) ){
		  goto cb_put_utf8_ch_return_not_utf;
		}else if( utfbytes!=byteswritten && (**cbs).encodingbytes==byteswritten ){
		  goto cb_put_utf8_ch_return_not_utf;
		}else if( utfbytes!=byteswritten && ( (**cbs).encodingbytes==byteswritten || *bytecount==byteswritten ) ){
		  goto cb_put_utf8_ch_return_bytecount_error;
		}
	  }
	}
cb_put_utf8_ch_return_not_utf:
	fprintf(stderr,"\ncb_put_utf8_ch: not utf, writecount: %i, high: 0x%X low: 0x%X.", byteswritten, (int) *chr_high, (int) *chr);
	*bytecount = byteswritten;
        *storedbytes = byteswritten; // uusi
        return CBNOTUTF;

cb_put_utf8_ch_return_bytecount_error:
	fprintf(stderr,"\ncb_put_utf8_ch: bytecount %i is over the allowed limit %i or %i, chr_high: 0x%X chr: 0x%X.", \
		byteswritten, (**cbs).encodingbytes, *bytecount, (int) *chr_high, (int) *chr);
	*bytecount = byteswritten;
        *storedbytes = byteswritten; // uusi
        return CBERRBYTECOUNT;
}

/*
 * Get character which is in UTF-8 format. Return 'bytecount' read.
 * chr_high is MSB end of the character if bytecount is larger than 4.
 * If bytecount is less than four, only chr is used, otherwice the the big
 * end of number is shift to chr_high as much as is needed. Returns any 
 * one byte in any encoding if encodingbytes is set to 1.
 * UTF-8, rfc-3629
 */
int  cb_get_utf8_ch(CBFILE **cbs, unsigned long int *chr, unsigned long int *chr_high, int *bytecount, int *storedbytes ){
	int err=CBSUCCESS, state=0, indx=0; 
	unsigned char byte='0';	
	if(cbs==NULL||*cbs==NULL){ return CBERRALLOC; }
	*chr=0; *chr_high=0; *bytecount=0;

	err = cb_get_ch(cbs, &byte);
	*chr=byte; *bytecount=1;

	//
	if( *chr == EOF || err > CBNEGATION ) { return err; } // pitaako err kasitella paremmin ... 30.3.2013
	//

	if( byteisascii( byte ) || (**cbs).encodingbytes==1 ){ // Return success even if byte is any one byte byte
          return CBSUCCESS;
	}else if( byteisutf8head2( byte ) && ( (**cbs).encodingbytes==0 || (**cbs).encodingbytes>=2 ))  state=2;
	else if( byteisutf8head3( byte )  && ( (**cbs).encodingbytes==0 || (**cbs).encodingbytes>=3 ))  state=3;
	else if( byteisutf8head4( byte )  && ( (**cbs).encodingbytes==0 || (**cbs).encodingbytes>=4 ))  state=4;
	else if( byteisutf8head5( byte )  && ( (**cbs).encodingbytes==0 || (**cbs).encodingbytes>=5 ))  state=5;
	else if( byteisutf8head6( byte )  && ( (**cbs).encodingbytes==0 || (**cbs).encodingbytes>=6 ))  state=6;
	else{  // not utf
	  fprintf(stderr,"\ncb_get_utf8_ch: first byte was not in utf format.");
          return CBNOTUTF;
	}

	if( state>1 && state<=6 ){
	  for(indx=1;indx<state;++indx){
	    err = cb_get_ch(cbs, &byte); *bytecount=*bytecount+1;
	    if(state<=4){
	      *chr=*chr<<8; *chr+=byte; 
	      if(state==3){
	        if(intisutf8bom( *chr ))
		  goto cb_get_utf8_ch_return_bom;
	      }
	    }else if(state==5){
	      *chr_high=byte; 
	    }else if(state==6){
	      *chr_high=*chr_high<<8; *chr_high+=byte; 
	    }
	    if( err > CBNEGATION ){
	      fprintf(stderr,"\ncb_get_utf8_ch: cb_get_ch: err %i.", err);
	      return err;
	    }
	    if( ! byteisutf8tail( byte ) ){
	      fprintf(stderr,"\ncb_get_utf8_ch: byte after header is not in utf format, bytecount %i.", *bytecount);
	      return CBNOTUTF;
	    }
	  }
	}
	return CBSUCCESS;
cb_get_utf8_ch_return_bom:
	return CBUTFBOM;
}
/*
 * Returns one 32 bit UCS word from UTF-8 and a number of bytes UTF-8 uses to save
 * it, 'bytecount'. Returns errors.
 */
int  cb_get_ucs_ch(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes ){
	unsigned long int low=0, high=0, tmp1=0, tmp2=0, result=0;
	int bytes=0, err=0; *chr=0; *bytecount=0;
	err = cb_get_utf8_ch(cbs, &low, &high, &bytes, storedbytes);

	*chr=low;
	if( bytes==1 ){ high=0; *bytecount=1;
//	  fprintf(stderr,"[%c]%i", (char)*chr, *bytecount );
	  return err;
	} // 7 bits, ascii, 0x7F
	// 00111111 = 0x3F ; 11000000 = 0xC0; 
	// 1) maski viimeisista kuudesta bitista
	result += (unsigned char) low & 0x3F; // 6 bits, 0x3F

	// 2) shift oikealle 2 bittia ja maski kahdesta bitista
	low=*chr; tmp1=result;
	result = tmp1 | ( low>>2 & 0xC0 ); tmp1=result; // 8 bits, 0x1FF

	// 3) maski toisen tavun kuudesta bitista ja shift oikealle 2 bittia
	low=*chr; tmp1=result;
	tmp2 = low & 0x3F00;
	result = tmp1 | tmp2>>2; tmp1 = result; // 14 bits, 0x3FFF
	if(bytes==2){ result = tmp1 & 0x7FF; *bytecount=2;
	  goto cb_get_ucs_ch_return;
	} // 11 bits needed, 0x7FF

	// 4) shift oikealle 4 bittia ja maski toisen tavun kahdesta bitista
	low=*chr; tmp1=result;
	result = tmp1 | ( low>>4 & 0xC000 ); tmp1=result; // 16 bits, 0xFFFF

	// 5) maski kolmannen tavun kuudesta bitista ja shift oikealle 4 bittia
	low=*chr; tmp1=result;
	tmp2 = low & 0x3F0000; // ei 0x3F0000 00111111 ... vaan 00----11 = 
	result = tmp1 |  tmp2>>4; tmp1 = result; // 22 bits, 3FFFFF
	if(bytes==3){ result = tmp1 & 0xFFFF; *bytecount=2;
	  goto cb_get_ucs_ch_return;
	} // 16 bits needed, 0xFFFF

	// 6) shift oikealle 6 bittia ja maski kolmannen tavun kahdesta bitista
	low=*chr; tmp1=result;
	result = tmp1 | ( low>>6 & 0xC00000 ); tmp1=result; // 24 bits, 0xFFFFFF
	if(bytes==4){ result = tmp1 & 0x1FFFFF; *bytecount=3;
	  goto cb_get_ucs_ch_return;
	} // 21 bits needed, 0x1FFFFF

	// 7) maski viimeisista kuudesta bitista neljanteen tavuun
	//    uusi verrattava 4.-5. tavu
	low=*chr; tmp1=result; tmp2=high;
	result = tmp1 | ( ( tmp2 & 0x3F )<<24 ); tmp1=result; tmp2=high; // 30 bits, 0x3FFFFFFF
	if(bytes==5){ result = tmp1 & 0x3FFFFFF ; *bytecount=4;
	  goto cb_get_ucs_ch_return;
	} // 26 bits needed, 3FFFFFF

	// 8) shift oikealle 2 bittia ja maski kahdesta bitista neljanteen tavuun
	low=*chr; tmp1=result; tmp2=high;
	result = tmp1 | ( ( tmp2>>2 & 0xC0 )<<24 ); tmp2=high; tmp1=result; // 31 bits, 0x7FFFFFFF
	if(bytes==6){ result = tmp1 & 0x7FFFFFFF; *bytecount=4;
	  goto cb_get_ucs_ch_return; // 31 bits needed, 0x7FFFFFFF	  
	}else{
	  fprintf(stderr, "\ncb_get_ucs_ch: error, bytes over 6.");
	}
cb_get_ucs_ch_return:
	if(*bytecount==0) *bytecount=1;
	*chr = result;
	//return CBSUCCESS;
	return err; // 30.3.2013, all from cb_get_utf8_ch
}

int  cb_put_ucs_ch(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes ){
	int bytes=0; 
	unsigned long int tmp=0, low=0, high=0;
	unsigned char byte1=0, byte2=0, byte3=0, byte4=0, byte5=0, byte6=0;

	if(cbs==NULL || *cbs==NULL) { return CBERRALLOC; }
	cb_bytecount( chr, &bytes ); // if first bit is 1, should put two bytes, first byte 0x00
	if( (**cbs).encodingbytes<bytes && (**cbs).encodingbytes!=0 ){
	  return CBERRBYTECOUNT;
	}

	byte1 = (unsigned char) *chr; *bytecount=bytes;
	if( bytes==1 && byteisascii( byte1 ) ){
	  low=byte1;
	  return cb_put_utf8_ch(cbs, &low, &high, bytecount, storedbytes);
	}
	// Tails
	tmp = *chr; byte1 = (unsigned char) tmp; masktoutf8tail( byte1 );
	low = tmp>>6; tmp=low; byte2 = (unsigned char) tmp; masktoutf8tail( byte2 );
	low = tmp>>6; tmp=low; byte3 = (unsigned char) tmp; masktoutf8tail( byte3 );
	low = tmp>>6; tmp=low; byte4 = (unsigned char) tmp; masktoutf8tail( byte4 );
	low = tmp>>6; tmp=low; byte5 = (unsigned char) tmp; masktoutf8tail( byte5 );
        low = tmp>>6; tmp=low; byte6 = (unsigned char) tmp; masktoutf8tail( byte6 );
	// Heads
	if( bytes==2 ){
	  masktoutf8head2( byte2 ); tmp=byte2; low=tmp<<8;
	  low+=byte1;
	  return cb_put_utf8_ch(cbs, &low, &high, bytecount, storedbytes);
	}else if( bytes==3 ){
	  masktoutf8head3( byte3 );
          tmp=byte3; low=tmp<<8; tmp=low+byte2;
	  low=tmp<<8; low+=byte1;
	  return cb_put_utf8_ch(cbs, &low, &high, bytecount, storedbytes);
	}else if( bytes==4 ){
	  masktoutf8head4( byte4 ); tmp=byte4; low=tmp<<8; tmp=low+byte3;
 	  low=tmp<<8; tmp=low+byte2; low=tmp<<8; low+=byte1;
	  return cb_put_utf8_ch(cbs, &low, &high, bytecount, storedbytes);
	}else if( bytes==5 ){
	  masktoutf8head5( byte5 ); tmp=byte4; low=tmp<<8; tmp=low+byte3;
	  low=tmp<<8; tmp=low+byte2; low=tmp<<8; low+=byte1;
	  high=byte5; 
	  return cb_put_utf8_ch(cbs, &low, &high, bytecount, storedbytes);
	}else if( bytes==6 ){
	  masktoutf8head5( byte6 ); tmp=byte4; low=tmp<<8; tmp=low+byte3;
	  low=tmp<<8; tmp=low+byte2; low=tmp<<8; low+=byte1;
	  tmp=byte6; high=tmp<<8; high+=byte5;
	  return cb_put_utf8_ch(cbs, &low, &high, bytecount, storedbytes);
	}
	return CBERRBYTECOUNT;
}
// Count utf-8 words bytes
void cb_bytecount(unsigned long int *chr, int *count){
        if( *chr <= 0x7F ) *count = 1; // <= 7 bits
        else if( *chr > 0x7F && *chr <= 0x7FF ) *count = 2; // <= 11 bits
        else if( *chr > 0x7FF && *chr <= 0xFFFF ) *count = 3; // <= 16 bits
        else if( *chr > 0xFFFF && *chr <= 0x1FFFFF ) *count = 4; // <= 21 bits
        else if( *chr > 0x1FFFFF && *chr <= 0x7FFFFFF ) *count = 5; // <= 26 bits
	else if( *chr <= 0x7FFFFFFF ) *count = 6; // <= 31 bits
	else count = 0;
}

// From four first bytes, use if contentlen is more than two, three or four
int  cb_bom_encoding(CBFILE **cbs){
	if( cbs!=NULL && *cbs!=NULL && (**cbs).cb!=NULL ){
	  if( (*(**cbs).cb).contentlen >= 3 ){
	    if( (*(**cbs).cb).buf[0]==0xEF && (*(**cbs).cb).buf[1]==0xBB && (*(**cbs).cb).buf[2]==0xBF ) // UTF-8
	      return CBENCUTF8;
	  }
	  if( (*(**cbs).cb).contentlen >= 2 ){
            if( (*(**cbs).cb).buf[0]==0xFE && (*(**cbs).cb).buf[1]==0xFF ) // UTF-16 big endian
	      return CBENCUTF16BE;
	  }else
	    return CBEMPTY;
	  if( (*(**cbs).cb).contentlen >= 4 ){
	    if( (*(**cbs).cb).buf[0]==0x00 && (*(**cbs).cb).buf[1]==0x00 && (*(**cbs).cb).buf[2]==0xFE && (*(**cbs).cb).buf[3]==0xFF ) // UTF-32 big endian
	      return CBENCUTF32BE;
	    if( (*(**cbs).cb).buf[0]==0xFF && (*(**cbs).cb).buf[1]==0xFE && (*(**cbs).cb).buf[2]==0x00 && (*(**cbs).cb).buf[3]==0x00 ) // UTF-32 little endian
	      return CBENCUTF32LE;
  	  }
 	  if( (*(**cbs).cb).contentlen >= 2 )
	    if( (*(**cbs).cb).buf[0]==0xFF && (*(**cbs).cb).buf[1]==0xFE ){ // UTF-16 little endian
 	      if((*(**cbs).cb).contentlen<=3)
	        return CBENCPOSSIBLEUTF16LE;
	      else
	        return CBENCUTF16LE;
	    }
	  if( (*(**cbs).cb).contentlen < 4 )
 	    return CBEMPTY;
	}else
	    return CBERRALLOC;
	return CBNOENCODING;
}
/*
 * BOM should allways be the first characters, U+FEFF in encoding.
 */
int  cb_write_bom(CBFILE **cbs){
        int err=CBNOENCODING; int e=0, y=0;
        if(cbs==NULL || *cbs==NULL)
          return CBERRALLOC;
        // (**cbs).encoding==CBENCUTF8
        if( (**cbs).encoding==CBENCUTF16LE || (**cbs).encoding==CBENCUTF16BE || \
            (**cbs).encoding==CBENCUTF32LE || (**cbs).encoding==CBENCUTF32BE ){
          err = cb_put_chr(&(*cbs), (unsigned long int) 0xFEFF, &e, &y);
        }
        return err;
}
/* 
 * UTF-16 Bit Distribution (Table 3-5, Unicode standard v.6.2)
 * Scalar Value			UTF-16
 * xxxxxxxxxxxxxxxx		xxxxxxxxxxxxxxxx
 * 000uuuuuxxxxxxxxxxxxxxxx 	110110wwwwxxxxxx 110111xxxxxxxxxx
 * wwww = uuuuu - 1 , in case if chr is greater than 0xFFFF and still less or equal than 0xFFFFF
 */
int  cb_put_utf16_ch(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes ){
        unsigned long int upper=0xD800, lower=0xDC00; // 110110 0000 000000 = 0xD800 ; 110111 00000 00000 = 0xDC00
        int err=CBSUCCESS;
	if(cbs==NULL || *cbs==NULL || chr==NULL )
	  return CBERRALLOC;

        if(*chr<=0xFFFF){
          if( (**cbs).encoding==CBENCUTF16BE )
            return cb_put_multibyte_ch(&(*cbs), *chr);
          if( (**cbs).encoding==CBENCUTF16LE )
            return cb_put_multibyte_ch(&(*cbs), cb_reverse_two_bytes(*chr) );            
        }else if(*chr<=0xFFFFF){
          lower = lower | ( *chr & 0x3FF ) ; // last ten bits
          upper = upper | ( (*chr)>>10 & 0x3FF ) ; // upper ten bits
          if( (**cbs).encoding==CBENCUTF16BE ){
            err = cb_put_multibyte_ch(&(*cbs), upper ); // order of upper and lower is propably the same both in LE and BE
            err = cb_put_multibyte_ch(&(*cbs), lower );
          }
          if( (**cbs).encoding==CBENCUTF16LE || (**cbs).encoding==CBENCPOSSIBLEUTF16LE ){
            err = cb_put_multibyte_ch(&(*cbs), cb_reverse_two_bytes(upper) );
            err = cb_put_multibyte_ch(&(*cbs), cb_reverse_two_bytes(lower) );
          }
          return err;
        }else{
          fprintf(stderr,"\ncb_put_utf16_ch: char %lX was out of range, err=%i.", *chr, err);
          return CBUCSCHAROUTOFRANGE;
        }
        return CBWRONGENCODINGCALL;
}
int  cb_get_utf16_ch(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes ){
        unsigned long int upper=0, lower=0; 
        int err=CBSUCCESS; 
	if(cbs==NULL || *cbs==NULL || chr==NULL )
	  return CBERRALLOC;

cb_get_utf16_ch_get_next_chr:
        err = cb_get_multibyte_ch( &(*cbs), &(*chr));
        if(err>CBERROR){ fprintf(stderr,"\ncb_get_utf16_ch: error in cb_get_multibyte_ch, err=%i.", err); };
        if( ( *chr & 0xFC00 ) == 0xD800 ){ // 1111110000000000 = 0xFC00 ; 110110 0000 000000 = 0xD800
          upper = *chr & 0x3FF;
          goto cb_get_utf16_ch_get_next_chr;
        }
        if( ( *chr & 0xFC00 ) == 0xDC00 ){ // 1111110000000000 = 0xFC00 ; 110111 00000 00000 = 0xDC00
          lower = *chr & 0x3FF;
          *chr = upper; *chr = (*chr)<<10;
          *chr = *chr | lower;
        }
#ifdef BIGENDIAN 
// turha (?)
//        if( (**cbs).encoding==CBENCUTF16LE || (**cbs).encoding==CBENCPOSSIBLEUTF16LE )
//          *chr = cb_reverse_two_bytes(*chr); // correct to UCS after reading like BE
#else
        if( (**cbs).encoding==CBENCUTF16BE )
          *chr = cb_reverse_two_bytes(*chr); // to UCS from BE
#endif            
        if( (**cbs).encoding==CBENCUTF16LE || (**cbs).encoding==CBENCUTF16BE || (**cbs).encoding==CBENCPOSSIBLEUTF16LE )
          return CBWRONGENCODINGCALL;
        if( *chr > 0xFFFFF )
          return CBUCSCHAROUTOFRANGE;
        return err;
}

int  cb_put_utf32_ch(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes ){
	if(cbs==NULL || *cbs==NULL || chr==NULL )
	  return CBERRALLOC;
	if((**cbs).encoding==CBENCUTF32LE){
          *bytecount=4; *storedbytes=4; // allways four bytes in UTF-32
          return cb_put_multibyte_ch(&(*cbs), *chr);
        }
        if((**cbs).encoding==CBENCUTF32BE){
          *bytecount=4; *storedbytes=4; // allways four bytes in UTF-32
          return cb_put_multibyte_ch( &(*cbs), cb_reverse_four_bytes(*chr) );          
        }
        return CBWRONGENCODINGCALL;
}
int  cb_get_utf32_ch(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes ){
        unsigned long int rev = 0;
	int err=CBSUCCESS;
	if(cbs==NULL || *cbs==NULL || chr==NULL )
	  return CBERRALLOC;
#ifdef BIGENDIAN
	if((**cbs).encoding==CBENCUTF32BE){
#else
	if((**cbs).encoding==CBENCUTF32LE){
#endif
          *bytecount=4; *storedbytes=4; // allways four bytes in UTF-32
          return cb_get_multibyte_ch(&(*cbs), &(*chr));
        }
#ifdef BIGENDIAN
        if((**cbs).encoding==CBENCUTF32LE){
#else
        if((**cbs).encoding==CBENCUTF32BE){
#endif
          *bytecount=4; *storedbytes=4; // allways four bytes in UTF-32
          err = cb_get_multibyte_ch(&(*cbs), &rev);
	  if(err<CBERROR){
             *chr = cb_reverse_four_bytes(rev);
          }else{
             fprintf(stderr,"cb_get_utf32_ch: error in reading from cb_get_multibyte_ch, err=%i.\n", err);
          }
          return err;
        }
        return CBWRONGENCODINGCALL;
}

/* 
 * Reverse 32 bits.
 */
unsigned int  cb_reverse_int32_bits(unsigned int from){
	unsigned int upper=0, lower=0, new=0;
        fprintf(stderr,"cb_reverse_int32_bits(%X)\n", from);
	upper = from>>16; // 16 upper bits
	lower = 0xFFFF & from; // 16 lower bits
        lower = cb_reverse_int16_bits(lower);
	upper = cb_reverse_int16_bits(upper);
	new = lower<<16;
	new = new | upper;
	return new;
}
/* 
 * Reverse integers last 16 bits. First 16 bits are meaningless.
 */
unsigned int  cb_reverse_int16_bits(unsigned int from){
	unsigned int  new=0;
	unsigned char upper=0, lower=0;
	upper = from>>8; // 8 upper bits
	lower = 0xFF & from; // 8 lower bits
        lower = cb_reverse_char8_bits(lower);
	upper = cb_reverse_char8_bits(upper);
	new = lower<<8;
	new = new | upper;
	return new;
}
/*
 * Reverse 8 bits.
 * 1. 2. 3. 4. 5. 6. 7. 8.
 * 1  2  4  8  16 32 64 128
 */
unsigned char  cb_reverse_char8_bits(unsigned char from){
	unsigned char new=0; 
	new = new | (from & 0x01)<<7; 
	new = new | (from & 0x02)<<5;
	new = new | (from & 0x04)<<3;
	new = new | (from & 0x08)<<1;

	new = new | (from & 0x10)>>1; // 16
	new = new | (from & 0x20)>>3; // 32
	new = new | (from & 0x40)>>5; // 64
	new = new | (from & 0x80)>>7; // 128
	return new;
}
unsigned int  cb_reverse_four_bytes(unsigned int  from){
	unsigned int upper=0, lower=0, new=0;
        fprintf(stderr,"cb_reverse_int32_bits(%X)\n", from);
	upper = from>>16; // 16 upper bits
	lower = 0xFFFF & from; // 16 lower bits
        lower = cb_reverse_two_bytes(lower);
	upper = cb_reverse_two_bytes(upper);
	new = lower; new = new<<16;
	new = new | upper;
	return new;
}
unsigned int  cb_reverse_two_bytes(unsigned int  from){
	unsigned int  new=0;
	unsigned char upper=0, lower=0;
	upper = from>>8; // 8 upper bits
	lower = 0xFF & from; // 8 lower bits
	new = lower; new = new<<8;
	new = new | upper;
	return new;
}
