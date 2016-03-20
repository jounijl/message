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

#define MAXCONTENTLEN     1024

/*
 * 4.11.2013
 * Finds next unused name, first from the list and then after the cursors 
 * position. New 'ucsname' is allocated and the name is copied 4-bytes
 * per character. Length is bytelength. ocoffset can not be used. */
int  cb_get_next_name_ucs(CBFILE **cbs, unsigned char **ucsname, int *namelength); 

/*
 * 30.11.2013. Allocates new ucsname and copies current name to it. */
int  cb_get_current_name(CBFILE **cbs, unsigned char **ucsname, int *namelength );
int  cb_get_currentleaf_name(CBFILE **cbs, unsigned char **ucsname, int *namelength );

/*
 * 4.11.2013
 * To find every unused name to the list. Search reaches the end of the stream.
 * Buffer has to be larger than the data to use the names from the list. */
int  cb_find_every_name(CBFILE **cbs); 

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
int  cb_find_leaf_from_current(CBFILE **cbs, unsigned char **ucsname, int *namelength, int *ocoffset, int matchctl);
int  cb_find_leaf_from_current_matchctl(CBFILE **cbs, unsigned char **ucsname, int *namelength, int *ocoffset, cb_match *mctl );

/*
 * Reads all leaves from the value (reads next name at the same time and 
 * reduces next names matchcount). 
 *
 * Return values
 * Success: CBSUCCESS
 * Error: CBERRALLOC and all from cb_set_cursor_match_length_ucs_matchctl
 * May return: CBNOTFOUND
 */
int  cb_read_value_leaves( CBFILE **cbs );

/*
 * 30.11.2013. Finds a variable described by dot-separated namelist
 * name1.name2.name3 from the values of the previous variables. (moved to test_cbsearch.c) */
//int  cb_tree_set_cursor_ucs(CBFILE **cbs, unsigned char **dotname, int namelen, int matchctl); // this function is in cbsearch

/*
 * 18.12.2014 Content reading
 */
/* Allocates ucscontent. */
int  cb_get_current_content( CBFILE **cbf, unsigned char **ucscontent, int *clength );
int  cb_get_currentleaf_content( CBFILE **cbf, unsigned char **ucscontent, int *clength );
int  cb_get_content( CBFILE **cbf, cb_name **cn, unsigned char **ucscontent, int *clength, int maxlength );
/* Copies ucscontent maxlength. */
int  cb_copy_content( CBFILE **cbf, cb_name **cn, unsigned char **ucscontent, int *clength, int maxlength );
int  cb_copy_currentleaf_content( CBFILE **cbf, unsigned char **ucscontent, int *clength );
int  cb_copy_current_content( CBFILE **cbf, unsigned char **ucscontent, int *clength );

/*
 * 30.6.2015 Conversion to help in reading names from configuration files.
 */
int  cb_allocate_ucsname_from_onebyte( unsigned char **ucsname, int *ucsnamelen, unsigned char **onebytename, int *onebytenamelen );
int  cb_copy_ucsname_from_onebyte( unsigned char **ucsname, int *ucsnamelen, unsigned char **onebytename, int *onebytenamelen );
int  cb_convert_from_ucs_to_onebyte( unsigned char **name, int *namelen );

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
//int cb_search_leaf_from_currentname(CBFILE **cbf, unsigned char **ucsparameter, int ucsparameterlen);
//int cb_search_leaf_from_currentleaf(CBFILE **cbf, unsigned char **ucsparameter, int ucsparameterlen, int levelfromprevious);

/*
 * Integer text to long int conversion. */
int  cb_get_long_int( unsigned char **ucsnumber, int ucsnumlen, signed long int *nmbr );
