/* 
 * Library to read and write streams. Valuepair list and search. Different character encodings.
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
#include "../include/cb_buffer.h"
#include "../include/cb_read.h"

/*
 * set_cursor can not use URL-encoding. URL-encoding can be used only in reading the value. Use URL-encoding
 * in the names everywhere if URL-encoding is needed.
 *
 * These functions can be used only in converting the content to native encoding from URL-encoding and
 * to output URL-encoded characters.
 *
 * https://www.iana.org/assignments/media-types/application/x-www-form-urlencoded
 * https://www.w3.org/TR/html/forms.html#attr-fs-enctype-urlencoded
 *
 * UCS -> UTF-8 -> Percentage encoding -> wire -> Percentage decoding -> UTF-8 -> UCS [https://www.w3.org/International/O-URL-code.html]
 *
 */

#define url_special_ch( x ) 	( x == 0x20 || x==0x0D || x==0x0A || x == '$' || x == '-' || x == '_' || x == '.' || x == '!' || x == '*' || x == 0x27 || x == '(' || x == ')' || x == ','  )

/*
 * To put characters '=' and '&' (or other URL characters),
 * cb_put_ch should be used instead of cb_put_chr. 
 */
int  cb_put_url_encode(CBFILE **cbs, unsigned long int chr, int *bc, int *sb){
	char hex[7] = { 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, '\0' };
	char *hexptr = NULL;
	int  hexlen = 6, err=CBSUCCESS, indx=0;
	hexptr = &hex[0];
	err = cb_copy_url_encoded_bytes( &hexptr, &hexlen, chr, &(*bc), &(*sb) );
	if(err>=CBERROR){ cb_clog( CBLOGERR, err, "\ncb_put_url_encode: cb_copy_url_encoded_bytes, error %i.", err); }
	for(indx=0; indx<hexlen && indx<6 && err<CBERROR; ++indx){
		err = cb_put_ch( &(*cbs), (unsigned char) hex[indx] );
	}
	return err;
}

/*
 * Copy UCS string from URL encoded data. Does not allocate. 
 *
 * Can be used with only one buffer, the same buffer can be the resultbuffer.
 */
int  cb_decode_url_encoded_bytes(unsigned char **ucshexdata, int ucshexdatalen, unsigned char **ucsdata, int *ucsdatalen, int ucsbuflen ){
	int err = CBSUCCESS, indx=0, num=0, ucsindx=0; // ucsindx is important to be able to use only one parameter buffer, 23.5.2016
	char cr_on=0;
	unsigned long int chr=0x20, chprev=0x20;
	char hex[3] = { '0', '0', '\0' };
	if( ucshexdata==NULL || *ucshexdata==NULL || ucsdata==NULL || *ucsdata==NULL || ucsdatalen==NULL ){
		cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_decode_url_encoded_bytes: parameter was null, error %i.", CBERRALLOC );
		return CBERRALLOC; 
	}

	//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_decode_url_encoded_bytes: encoding data [");
	//cb_print_ucs_chrbuf( CBLOGDEBUG, &(*ucshexdata), ucshexdatalen, ucshexdatalen );
	//cb_clog( CBLOGDEBUG, CBNEGATION, "]");

	for(indx=0; indx<ucshexdatalen && ucsindx<ucsbuflen && err<CBNEGATION;){
		chprev = chr;

		err = cb_get_ucs_chr( &chr, &(*ucshexdata), &indx, ucshexdatalen);
		if(err>CBERROR){ cb_clog( CBLOGDEBUG, err, "\ncb_decode_url_encoded_bytes: cb_get_ucs_chr (1), error %i.", err ); continue; } // stop

// TASTA

		if( chr == '%' && ucsindx<ucsbuflen && indx<ucshexdatalen && err<CBNEGATION ){
			chprev = chr;
			err = cb_get_ucs_chr( &chr, &(*ucshexdata), &indx, ucshexdatalen);
			if(err>CBERROR){ cb_clog( CBLOGDEBUG, err, "\ncb_decode_url_encoded_bytes: cb_get_ucs_chr (2), error %i.", err ); continue; } // stop
			if( chr == '0' && ucsindx<ucsbuflen && indx<ucshexdatalen && err<CBNEGATION ){
				chprev = chr;
				err = cb_get_ucs_chr( &chr, &(*ucshexdata), &indx, ucshexdatalen);
				if(err>CBERROR){ cb_clog( CBLOGDEBUG, err, "\ncb_decode_url_encoded_bytes: cb_get_ucs_chr (3), error %i.", err ); continue; } // stop
				if( chr == 'D' || chr == 'd' ){ // ignore
					cr_on = 1;
					continue;
				}
				if( ( chr == 'A' || chr == 'a' ) && ucsindx<ucsbuflen && err<CBNEGATION ){ // LF -> CRLF
					err = cb_put_ucs_chr( (unsigned long int) 0x0D, &(*ucsdata), &ucsindx, ucsbuflen); // CR #1
					cr_on = 0;
					if( ucsindx<ucsbuflen && err<CBNEGATION )
						err = cb_put_ucs_chr( (unsigned long int) 0x0A, &(*ucsdata), &ucsindx, ucsbuflen);
					continue;
				}else if( cr_on==1 ){ // CR #2
					err = cb_put_ucs_chr( (unsigned long int) 0x0D, &(*ucsdata), &ucsindx, ucsbuflen);
					cr_on = 0;
				}
				if( ucsindx<ucsbuflen && err<CBNEGATION ){ // %<hexnum><hexnum> #1
					hex[0] = (char) chprev; hex[1] = (char) chr;
					num = (int) strtol( &hex[0], NULL, 16);
					err = cb_put_ucs_chr( (unsigned long int) num, &(*ucsdata), &ucsindx, ucsbuflen);
				}
			}else{
			 	if(cr_on==1){ // CR #3
					err = cb_put_ucs_chr( (unsigned long int) 0x0D, &(*ucsdata), &ucsindx, ucsbuflen);
					cr_on = 0;
				}
				if( ucsindx<ucsbuflen && err<CBNEGATION ){ // %<hexnum><hexnum> #2
					hex[0] = (char) chr; 
					chprev = chr;
					err = cb_get_ucs_chr( &chr, &(*ucshexdata), &indx, ucshexdatalen);
					if(err>CBERROR){ cb_clog( CBLOGDEBUG, err, "\ncb_decode_url_encoded_bytes: cb_get_ucs_chr (5), error %i.", err ); continue; } // stop
					hex[1] = (char) chr;
					num = (int) strtol( &hex[0], NULL, 16);
					err = cb_put_ucs_chr( (unsigned long int) num, &(*ucsdata), &ucsindx, ucsbuflen);
				}
			}
			continue;
		}
	 	if(cr_on==1){ // CR #4
			err = cb_put_ucs_chr( (unsigned long int) 0x0D, &(*ucsdata), &ucsindx, ucsbuflen);
			cr_on = 0;
		}
		if( chr == '+' ){ 
			err = cb_put_ucs_chr( (unsigned long int) 0x20, &(*ucsdata), &ucsindx, ucsbuflen); continue; // SP
		}else if( chr != '%' ){ // alpha, digit, special characters
			err = cb_put_ucs_chr( chr, &(*ucsdata), &ucsindx, ucsbuflen);
		}
// TANNE

	}
	*ucsdatalen = ucsindx;

	//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_decode_url_encoded_bytes: encoded data  [");
	//cb_print_ucs_chrbuf( CBLOGDEBUG, &(*ucsdata), *ucsdatalen, ucsbuflen );
	//cb_clog( CBLOGDEBUG, CBNEGATION, "]");

	return err;
}

/*
 * Copies URL encoded bytes to buffer 'hex' and return sizes. Does not allocate. */
int  cb_copy_url_encoded_bytes(char **hexdata, int *hexdatalen, unsigned long int chr, int *bc, int *sb){
	char hex[3] = { 0x02, 0x00, '\0' };
	if( bc==NULL || sb==NULL ) return CBERRALLOC;
	*bc=0; *sb=0; 
	if( hexdata==NULL || *hexdata==NULL || hexdatalen==NULL ) return CBERRALLOC;
	if( ( ( chr==0x00 || ( chr>0 && chr<0x20 ) ) || ( chr>=0x7F && chr<=0xFF ) ) && ! url_special_ch( chr ) ){
		/*
	 	 * %<hex digit><hex digit> */
		if(*hexdatalen<3){ cb_clog( CBLOGERR, CBERRALLOC, "\ncb_copy_url_encoded_bytes: hexdata buffer was too small, error %i.", CBERRALLOC ); return CBERRALLOC; }
		snprintf( &hex[0], (size_t) 3, "%.2X", (unsigned int) chr ); // size-1 characters
		(*hexdata)[0] = (unsigned char) 0x25; // '%'
		(*hexdata)[1] = (char) hex[0];
		(*hexdata)[2] = (char) hex[1];
		*sb = 3; *bc = 1; 
		*hexdatalen = 3;
	}else if( chr==0x20 ){
		if(*hexdatalen<1){ cb_clog( CBLOGERR, CBERRALLOC, "\ncb_copy_url_encoded_bytes: hexdata buffer was too small, error %i.", CBERRALLOC ); return CBERRALLOC; }
		(*hexdata)[0] = (unsigned char) 0x2B; // '+'
		*sb = 1; *bc = 1; 
		*hexdatalen = 1;
	}else if( chr==0x0D ){ // CR, ignore this and wait for the LF
		*sb = 0; *bc = 0; *hexdatalen = 0;
	}else if( chr==0x0A ){ // LF, both CR and LF
		if(*hexdatalen<6){ cb_clog( CBLOGERR, CBERRALLOC, "\ncb_copy_url_encoded_bytes: hexdata buffer was too small, error %i.", CBERRALLOC ); return CBERRALLOC; }
		snprintf( &hex[0], (size_t) 3, "%.2X", (unsigned int) 0x0D ); // size-1 characters, CR
		(*hexdata)[0] = (char) 0x25; // '%'
		(*hexdata)[1] = (char) hex[0];
		(*hexdata)[2] = (char) hex[1];
		snprintf( &hex[0], (size_t) 3, "%.2X", (unsigned int) 0x0A ); // size-1 characters, LF
		(*hexdata)[3] = (char) 0x25; // '%'
		(*hexdata)[4] = (char) hex[0];
		(*hexdata)[5] = (char) hex[1];
		*sb = 6; *bc = 2; *hexdatalen = 6;
	}else if( url_special_ch( chr ) ){ // unnecessary, 25.5.2016
		if(*hexdatalen<0){ cb_clog( CBLOGERR, CBERRALLOC, "\ncb_copy_url_encoded_bytes: hexdata buffer was too small, error %i.", CBERRALLOC ); return CBERRALLOC; }
		(*hexdata)[0] = (char) chr;
		*sb = 1; *bc = 1; *hexdatalen = 1;
	}else if( chr<=0xFF && chr>0x00 ){
		if(*hexdatalen<0){ cb_clog( CBLOGERR, CBERRALLOC, "\ncb_copy_url_encoded_bytes: hexdata buffer was too small, error %i.", CBERRALLOC ); return CBERRALLOC; }
		(*hexdata)[0] = (char) chr;
		*sb = 1; *bc = 1; *hexdatalen = 1;
	}else{
		cb_clog( CBLOGERR, CBERRBYTECOUNT, "\ncb_copy_url_encoded_bytes: character 0x%.4lX was not in range, error %i.", chr, CBERRBYTECOUNT );
		return CBERRBYTECOUNT;
	}
	return CBSUCCESS;
}
