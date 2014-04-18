/*
 * Library to read and write streams. Functions to compare.
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

#include <pcre.h>	// to use matchctl -7 and -8 (cb_match has void* re and re_extra to leave these ctls optionally out if needed)

/*
 * Size of block to compare the re and size of
 * the overlap to next block. */
#define CBREINPUTBUFFERSIZE   1024
#define CBREINPUTOVERLAPSIZE  256

/*
 * pcre result vector size (array multiple of 3). */
#define OVECSIZE      (3*64)

/*
 * Utility functions encapsulated by function cb_compare . #include "cb_buffer.h" is needed to use
 * these, struct cb_match is in cb_buffer.h .
 */

int  cb_compare_strict(unsigned char **name1, int len1, unsigned char **name2, int len2, int from2);
int  cb_compare_rfc2822(unsigned char **name1, int len1, unsigned char **name2, int len2, int from2);
int  cb_compare_get_matchctl(unsigned char **pattern, int patsize, int options, cb_match *ctl, int matchctl); 
int  cb_compare_regexp(unsigned char **name2, int len2, int startoffset, int options, cb_match *mctl);
