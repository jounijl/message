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


/*
 * Copy attributes from input to output using the rstart, rend and encoding
 * of the CBFILE:s. Does not copy leaves. 27.1.2022. */
signed int  cb_copy_attributes( CBFILE *in, CBFILE *out ){
	signed int err = CBSUCCESS;

	
	return err;
}


//signed int  cbi_put_chr( CBFILE **cbf, unsigned long int chr );
//signed int  cbi_get_chr( CBFILE **cbf, unsigned long int *chr );






