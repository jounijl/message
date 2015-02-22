/*
 * cb_read.c These use the base library:  (a library using the basic library).
 */

#define MAXCONTENTLEN     1024

/*
 * 4.11.2013
 * Finds next unused name, first from the list and then after the cursors 
 * position. New 'ucsname' is allocated and the name is copied 4-bytes
 * per character. Length is bytelength. */
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
 * 17.2.2015 get pointer to the current name in list.
 */
//int  cb_get_current_pointer( CBFILE **cbf, cb_name **cn );
