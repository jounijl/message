/*
 * Library to read and write streams. Functions to compare.
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

#define PCRE2_CODE_UNIT_WIDTH 32
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#include <pcre2.h>	// to use matchctl -7 and -8 (cb_match has void* re and re_extra to leave these ctls optionally out if needed)
#pragma clang diagnostic pop

/*
 * Byte count of re subject block (one character is four bytes). */
#define CBREGSUBJBLOCK         3072

/*
 * Overlapsize of re subject block (multiple of four). */
#define CBREGOVERLAPSIZE       1024

/*
 * pcre result vector size (array multiple of 3). */
#define OVECSIZE              (3*128)

/*
 * Utility functions encapsulated by function cb_compare . #include "cb_buffer.h" is needed to use
 * these, struct cb_match is in cb_buffer.h .
 */

signed int  cb_compare_strict(unsigned char **name1, signed int len1, unsigned char **name2, signed int len2, signed int from2);
signed int  cb_compare_rfc2822(unsigned char **name1, signed int len1, unsigned char **name2, signed int len2, signed int from2);

/*
 * Pattern is converted to host byte order from 4-byte UCS representation. */
signed int  cb_compare_get_matchctl(unsigned char **pattern, signed int patsize, unsigned int options, cb_match *ctl, signed int matchctl); 

/*
 * Matches name2 in overlapping blocks. Converts name2 from 4-byte UCS form to host byte order. */
signed int  cb_compare_regexp(unsigned char **name2, signed int len2, cb_match *mctl, signed int *matchcount);

/*
 * Match one line. Name2 has to be in host byte order (pcre32). */
signed int  cb_compare_regexp_one_block(unsigned char **name2, signed int len2, signed int startoffset, cb_match *mctl, signed int *matchcount);

/*
 * Match ASCII and additionally scandinavic letters if 'scandit' is set to 1. */
signed int  cb_compare_case_insensitive(unsigned char **name1, signed int len1, unsigned char **name2, signed int len2, signed int from2, signed char scandit );

/*
 * Error text given by pcre2 ( if err was CBERRREGEXCOMP or CBERRREGEXEC ). */
signed int  cb_compare_get_regexp_error_text( signed int errorcode, unsigned char **textbuffer, signed int textbufferlen ); // 15.11.2018

