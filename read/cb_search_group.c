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
#include <string.h>
#include "../include/cb_buffer.h"
#include "../include/cb_json.h"
#include "../include/cb_read.h"

/*
 * Root should be set before calling the function, 17.3.2023. */
signed int  cb_search_next_leaf_add_to_group( CBFILE **cbs, unsigned char **name, signed int namelength ){
	signed int err = CBSUCCESS;
	signed int current_group = 0;
	signed int previous_search_option = CBSEARCHNEXTGROUPLEAVES;
	if( cbs==NULL || *cbs==NULL || name==NULL || *name==NULL ) return CBERRALLOC;

	// Save old group and search option
	current_group = (*(**cbs).cb).list.currentgroup;
	previous_search_option = (**cbs).cf.leafsearchmethod;

	// Increase group
	cb_increase_group_number( &(*cbs) );

	// Set search option
	(**cbs).cf.leafsearchmethod = CBSEARCHNEXTLEAVES;

	// Search
	err = cb_set_cursor( &(*cbs), &(*name), &namelength);
	if( err==CBSUCCESS || err==CBSTREAM ){

		// If found, add the leaf to the currentgroup
		if( (*(**cbs).cb).list.currentleaf!=NULL ) (*(*(**cbs).cb).list.currentleaf).group = current_group;
	}

	// Restore the group and searh option
	(**cbs).cf.leafsearchmethod = previous_search_option;
	(*(**cbs).cb).list.currentgroup = current_group;
	return err;
}





