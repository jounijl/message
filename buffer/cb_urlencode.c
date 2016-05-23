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


int  cb_get_url_encode(CBFILE **cbs, unsigned long int *chr, int *bc, int *sb);
int  cb_put_url_encode(CBFILE **cbs, unsigned long int chr, int *bc, int *sb);

/*
 * URL percent encoding. MIME-type of the default FORM: application/x-www-form-urlencoded
 * (other one is for binary files)
 *
 * IANA: [ https://www.iana.org/assignments/media-types/application/x-www-form-urlencoded ]
 * HTML: [ https://www.w3.org/TR/html/forms.html#attr-fs-enctype-urlencoded ]
 *
 */
int  cb_get_url_encode(CBFILE **cbs, unsigned long int *chr, int *bc, int *sb){
	if( cbs==NULL || *cbs==NULL || chr==NULL ) return CBERRALLOC;
	*bc=0; *sb=0; *chr=0x20;
	//cb_get_ch();
	return CBSUCCESS;
}
int  cb_put_url_encode(CBFILE **cbs, unsigned long int chr, int *bc, int *sb){
	if( cbs==NULL || *cbs==NULL ) return CBERRALLOC;
	unsigned char ch = 0x20;
	*bc=0; *sb=0; chr=0x20;
	if( chr==0x20 )
		ch = '+';
	//else if(  )
	//cb_put_ch();
	return CBSUCCESS;
}

