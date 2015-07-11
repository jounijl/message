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
 * name1.name2.name3 from the values of the previous variables. */
//int  cb_tree_set_cursor_ucs(CBFILE **cbs, unsigned char **dotname, int namelen, int matchctl); // this function is in cbsearch

/*
 * 18.12.2014 Content reading
 */
int  cb_get_current_content( CBFILE **cbf, unsigned char **ucscontent, int *clength );
int  cb_get_currentleaf_content( CBFILE **cbf, unsigned char **ucscontent, int *clength );
int  cb_get_content( CBFILE **cbf, cb_name **cn, unsigned char **ucscontent, int *clength, int maxlength );

/*
 * 30.6.2015 Conversion to help in reading names from configuration files.
 */
int  cb_allocate_ucsname_from_onebyte( unsigned char **ucsname, int *ucsnamelen, unsigned char **onebytename, int *onebytenamelen );

