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

/*
 * cb_read.c These use the base library:  (a library using the basic library).
 */

#define MAXCONTENTLEN     12582912 	// 1024 some old limit (to be removed), 10.12.2016

/*
 * 4.11.2013
 * Finds next unused name, first from the list and then after the cursors
 * position. New 'ucsname' is allocated and the name is copied 4-bytes
 * per character. Length is bytelength. ocoffset can not be used. */
signed int  cb_get_next_name_ucs(CBFILE **cbs, unsigned char **ucsname, signed int *namelength);
signed int  cb_get_next_name_ucs_with_delimiter(CBFILE **cbs, unsigned char **ucsname, signed int *namelength, signed long int *delim );
signed int  cb_copy_next_name_ucs(CBFILE **cbs, unsigned char **ucsname, signed int *namelength, signed int namebuflen);

/*
 * 30.11.2013. Allocates new ucsname and copies current name to it. */
signed int  cb_get_current_name(CBFILE **cbs, unsigned char **ucsname, signed int *namelength );
signed int  cb_get_current_name_with_delim(CBFILE **cbs, unsigned char **ucsname, signed int *namelength, signed long int *delim );
signed int  cb_copy_currentleaf_name(CBFILE **cbs, unsigned char **ucsname, signed int *namelength, signed int namebuflen );
signed int  cb_get_currentleaf_name(CBFILE **cbs, unsigned char **ucsname, signed int *namelength );
signed int  cb_copy_current_name(CBFILE **cbs, unsigned char **ucsname, signed int *namelength, signed int namebuflen );
signed int  cb_copy_current_name_with_delim(CBFILE **cbs, unsigned char **ucsname, signed int *namelength, signed int namebuflen, signed long int *delim );

/*
 * 4.11.2013
 * To find every unused name to the list. Search reaches the end of the stream.
 * Buffer has to be larger than the data to use the names from the list. */
signed int  cb_find_every_name(CBFILE **cbs);

/*
 * 29.6.2015, NOT TESTED OR USED YET 3.7.2015 (under construction or will be removed)
 *
 * Find 'ucsname' leaf from current at any ocoffset level.
 *
 * Find name under the current name from all the leafs
 * and return it's ocoffset level. Reads from ocoffset
 * onwards and updates ocoffset to the level of found
 * name.
 * Returns: CBSUCCESS
 * May return: CBNOTFOUND
 * Errors: CBERRALLOC and all from cb_set_cursor. */
signed int  cb_find_leaf_from_current(CBFILE **cbs, unsigned char **ucsname, signed int *namelength, signed int *ocoffset, signed int matchctl);
signed int  cb_find_leaf_from_current_matchctl(CBFILE **cbs, unsigned char **ucsname, signed int *namelength, signed int *ocoffset, cb_match *mctl );

/*
 * Find next leaf (does not set root in the function) using CBSEARCHNEXTLEAVES and
 * add the found next leaf in the same group (to the current group before starting
 * the search). Uses a group value one bigger to the current group and sets
 * back the group value used in calling the function. USes function
 * 'cb_set_cursor', single character text. 17.3.2023.
 */
signed int  cb_search_next_leaf_add_to_group( CBFILE **cbs, unsigned char **name, signed int namelength );

/*
 * Reads all leaves from the value (reads next name at the same time and
 * reduces next names matchcount).
 *
 * Return values
 * Success: CBSUCCESS
 * Error: CBERRALLOC and all from cb_set_cursor_match_length_ucs_matchctl
 * May return: CBNOTFOUND
 */
signed int  cb_read_value_leaves( CBFILE **cbs );

/*
 * 30.11.2013. Finds a variable described by dot-separated namelist
 * name1.name2.name3 from the values of the previous variables. (moved to test_cbsearch.c) */
//signed int  cb_tree_set_cursor_ucs(CBFILE **cbs, unsigned char **dotname, signed int namelen, signed int matchctl); // this function is in cbsearch

/*
 * 18.12.2014 Content reading
 */
/* Allocates ucscontent. */
signed int  cb_get_current_content( CBFILE **cbf, unsigned char **ucscontent, signed int *clength, signed int maxlength );
signed int  cb_get_currentleaf_content( CBFILE **cbf, unsigned char **ucscontent, signed int *clength, signed int maxlength );
signed int  cb_get_content( CBFILE **cbf, cb_name **cn, unsigned char **ucscontent, signed int *clength, signed int maxlength );
/* Copies ucscontent maxlength. */
signed int  cb_copy_content( CBFILE **cbf, cb_name **cn, unsigned char **ucscontent, signed int *clength, signed int maxlength );
signed int  cb_copy_currentleaf_content( CBFILE **cbf, unsigned char **ucscontent, signed int *clength, signed int maxlength );
signed int  cb_copy_current_content( CBFILE **cbf, unsigned char **ucscontent, signed int *clength, signed int maxlength );

/*
 * 30.6.2015 Conversion to help in reading names from configuration files.
 */
signed int  cb_allocate_ucsname_from_onebyte( unsigned char **ucsname, signed int *ucsnamelen, unsigned char **onebytename, signed int *onebytenamelen );
signed int  cb_convert_from_ucs_to_onebyte( unsigned char **name, signed int *namelen );
signed int  cb_copy_ucsname_from_onebyte( unsigned char **ucsname, signed int *ucsnamelen, unsigned char **onebytename, signed int *onebytenamelen );
signed int  cb_copy_ucsname_to_onebyte( unsigned char **ucsname, signed int ucsnamelen, unsigned char **onebytename, signed int *onebytenamelen, signed int onebytebuflen );

/*
 * Hierarchical searches. These are at the moment all usable search methods.
 * If names length is still -1, searches all leafs of the name (until the next name)
 * and searches the wanted named leaf by searching from the resulted tree.
 *
 * If search is not done name by name (the names being in the list), the tree structure
 * will be broken. It is possible to find just one name like this. During other
 * searches, the tree would be broken. 19.8.2015
 *
 * With these, it's possible to search names like name1.name2.name3 with changing
 * the names in other searches.
 *
 * May be removed if not needed (27.8.2015).
 */
//signed int cb_search_leaf_from_currentname(CBFILE **cbf, unsigned char **ucsparameter, signed int ucsparameterlen);
//signed int cb_search_leaf_from_currentleaf(CBFILE **cbf, unsigned char **ucsparameter, signed int ucsparameterlen, signed int levelfromprevious);

/*
 * Integer text to long int conversion. */
signed int  cb_get_long_int( unsigned char **ucsnumber, signed int ucsnumlen, signed long int *nmbr );

/*
 * URL-decoding functions. */
signed int  cb_put_url_encode(CBFILE **cbs, unsigned long int chr, signed int *bc, signed int *sb);
signed int  cb_copy_url_encoded_bytes( char **hexdata, signed int *hexdatalen, unsigned long int chr, signed int *bc, signed int *sb); // sets written hexdatalen, bytecount bc and stored bytes sb
signed int  cb_decode_url_encoded_bytes(unsigned char **ucshexdata, signed int ucshexdatalen, unsigned char **ucsdata, signed int *ucsdatalen, signed int ucsbuflen );

/*
 * Write with bypassing all the special characters, 16.8.2017. */
signed int  cbi_put_chr( CBFILE **cbf, unsigned long int chr );
signed int  cbi_get_chr( CBFILE **cbf, unsigned long int *chr ); // Normal get without removing special characters, 16.8.2017

/*
 * Forward attributes, 27.1.2022. */
signed int  cb_copy_attributes( CBFILE *in, CBFILE *out );

/*
 * Some transfer functions, 11.7.2021. */
signed long int  cb_timeout_read( signed int *err, signed int fd, void *bf, signed long int nb, signed int wtime );
signed long int  cb_timeout_write( signed int *err, signed int fd, void *bf, signed long int nb, signed int wtime );
signed int  cb_set_timeout_io( CBFILE *cbf );
//23.1.2022: signed int  cb_set_timeout_write( CBFILE *cbf );
void cb_set_read_timeout( CBFILE *cbf, signed int timeout_in_seconds );
void cb_set_write_timeout( CBFILE *cbf, signed int timeout_in_seconds );

/* Some helper function, 5.8.2021. */
signed int  cb_write_ceof( CBFILE *out );
