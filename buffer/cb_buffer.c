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
#include <string.h> // memset
#include <errno.h>  // errno
#include <fcntl.h>  // fcntl, 20.5.2016
#include "../include/cb_encoding.h"
#include "../include/cb_buffer.h"


static signed int  cb_get_char_read_block(CBFILE **cbf, unsigned char *ch);
static signed int  cb_set_type(CBFILE **buf, unsigned char type);
static signed int  cb_allocate_empty_cbfile(CBFILE **str, signed int fd);
//static signed int  cb_get_leaf(cb_name **tree, cb_name **leaf, signed int count, signed int *left); // not tested yet 7.12.2013
static signed int  cb_print_leaves_inner(cb_name **cbn, signed int priority);
//signed signed int  cb_get_char_read_offset_block(CBFILE **cbf, unsigned char *ch, signed long int offset, signed int transferencoding, signed int transferextension);
static signed int  cb_get_char_read_offset_block(CBFILE **cbf, unsigned char *ch, signed long int offset );
static signed int  cb_allocate_buffer_from_blk(cbuf **cbf, unsigned char **blk, signed int blksize);
static signed int  cb_init_buffer_from_blk(cbuf **cbf, unsigned char **blk, signed int blksize);
static signed int  cb_init_list_rd( cbuf **buf ); // 5.8.2017
//signed int  cb_flush_cbfile_to_offset(cbuf **cb, signed int fd, signed long int offset, signed int transferencoding, signed int transferextension);
static signed int  cb_flush_cbfile_to_offset(CBFILE **cbf, signed long int offset );
static signed int  cb_set_search_method(CBFILE **cbf, unsigned char method);
static signed int  cb_set_leaf_search_method(CBFILE **cbf, unsigned char method);

/*
 * Debug
 */
void  cb_print_searchmethod( CBFILE *cbs, int priority ){
        if( cbs==NULL ) return;
        if( (*cbs).cf.searchmethod==CBSEARCHUNIQUENAMES )              cb_clog( priority, CBNEGATION, "CBSEARCHUNIQUENAMES");
        if( (*cbs).cf.searchmethod==CBSEARCHNEXTNAMES )                cb_clog( priority, CBNEGATION, "CBSEARCHNEXTNAMES");
        if( (*cbs).cf.searchmethod==CBSEARCHNEXTGROUPNAMES )           cb_clog( priority, CBNEGATION, "CBSEARCHNEXTGROUPNAMES");
        if( (*cbs).cf.searchmethod==CBSEARCHNEXTLASTGROUPNAMES )       cb_clog( priority, CBNEGATION, "CBSEARCHNEXTLASTGROUPNAMES");
}
void  cb_print_leafsearchmethod( CBFILE *cbs, int priority ){
        if( cbs==NULL ) return;
        if( (*cbs).cf.leafsearchmethod==CBSEARCHUNIQUELEAVES )         cb_clog( priority, CBNEGATION, "CBSEARCHUNIQUELEAVES");
        if( (*cbs).cf.leafsearchmethod==CBSEARCHNEXTLEAVES )           cb_clog( priority, CBNEGATION, "CBSEARCHNEXTLEAVES");
        if( (*cbs).cf.leafsearchmethod==CBSEARCHNEXTGROUPLEAVES )      cb_clog( priority, CBNEGATION, "CBSEARCHNEXTGROUPLEAVES");
        if( (*cbs).cf.leafsearchmethod==CBSEARCHNEXTLASTGROUPLEAVES )  cb_clog( priority, CBNEGATION, "CBSEARCHNEXTLASTGROUPLEAVES");
}
signed int cb_print_conf(CBFILE **str, signed int priority){
	signed int err = CBSUCCESS;
	if(str==NULL || *str==NULL){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_print_conf: str was null."); return CBERRALLOC; }
	cb_clog( priority, CBNEGATION, "\nfindleaffromallnames:        \t0x%.2XH", (**str).cf.findleaffromallnames);
	cb_clog( priority, CBNEGATION, "\nid:                          \t%i", (**str).id );
	cb_clog( priority, CBNEGATION, "\ntype:                        \t" );
	if( (**str).cf.type==CBCFGSTREAM )		cb_clog( priority, CBNEGATION, "CBCFGSTREAM");
	if( (**str).cf.type==CBCFGBUFFER )		cb_clog( priority, CBNEGATION, "CBCFGBUFFER");
	if( (**str).cf.type==CBCFGFILE )         	cb_clog( priority, CBNEGATION, "CBCFGFILE");
	if( (**str).cf.type==CBCFGSEEKABLEFILE )	cb_clog( priority, CBNEGATION, "CBCFGSEEKABLEFILE");
	if( (**str).cf.type==CBCFGBOUNDLESSBUFFER )	cb_clog( priority, CBNEGATION, "CBCFGBOUNDLESSBUFFER");
	cb_clog( priority, CBNEGATION, "\nsearchstate:                 \t");
	if( (**str).cf.searchstate==CBSTATELESS )      	cb_clog( priority, CBNEGATION, "CBSTATELESS");
	if( (**str).cf.searchstate==CBSTATEFUL )      	cb_clog( priority, CBNEGATION, "CBSTATEFUL");
	if( (**str).cf.searchstate==CBSTATETOPOLOGY )  	cb_clog( priority, CBNEGATION, "CBSTATETOPOLOGY");
	if( (**str).cf.searchstate==CBSTATETREE )  	cb_clog( priority, CBNEGATION, "CBSTATETREE");
	cb_clog( priority, CBNEGATION, "\nsearchmethod:                \t");
	cb_print_searchmethod( &(**str), priority );
	/****
	if( (**str).cf.searchmethod==CBSEARCHUNIQUENAMES )		cb_clog( priority, CBNEGATION, "CBSEARCHUNIQUENAMES");
	if( (**str).cf.searchmethod==CBSEARCHNEXTNAMES )		cb_clog( priority, CBNEGATION, "CBSEARCHNEXTNAMES");
	if( (**str).cf.searchmethod==CBSEARCHNEXTGROUPNAMES )		cb_clog( priority, CBNEGATION, "CBSEARCHNEXTGROUPNAMES");
	if( (**str).cf.searchmethod==CBSEARCHNEXTLASTGROUPNAMES )	cb_clog( priority, CBNEGATION, "CBSEARCHNEXTLASTGROUPNAMES");
	 ****/
	cb_clog( priority, CBNEGATION, "\nleafsearchmethod:            \t");
	cb_print_leafsearchmethod( &(**str), priority );
	/****
	if( (**str).cf.leafsearchmethod==CBSEARCHUNIQUELEAVES )		cb_clog( priority, CBNEGATION, "CBSEARCHUNIQUELEAVES");
	if( (**str).cf.leafsearchmethod==CBSEARCHNEXTLEAVES )		cb_clog( priority, CBNEGATION, "CBSEARCHNEXTLEAVES");
	if( (**str).cf.leafsearchmethod==CBSEARCHNEXTGROUPLEAVES )	cb_clog( priority, CBNEGATION, "CBSEARCHNEXTGROUPLEAVES");
	if( (**str).cf.leafsearchmethod==CBSEARCHNEXTLASTGROUPLEAVES )	cb_clog( priority, CBNEGATION, "CBSEARCHNEXTLASTGROUPLEAVES");
	 ****/
	cb_clog( priority, CBNEGATION, "\nfindleaffromallnames:        \t0x%.2XH", (**str).cf.findleaffromallnames);
	cb_clog( priority, CBNEGATION, "\nencoding:                    \t0x%.2XH", (**str).encoding );
	cb_clog( priority, CBNEGATION, "\ntransferencoding:            \t0x%.2XH", (**str).transferencoding );
	cb_clog( priority, CBNEGATION, "\nunfold:                      \t0x%.2XH", (**str).cf.unfold);
	cb_clog( priority, CBNEGATION, "\nasciicaseinsensitive:        \t0x%.2XH", (**str).cf.asciicaseinsensitive);
	cb_clog( priority, CBNEGATION, "\nscanditcaseinsensitive:      \t0x%.2XH", (**str).cf.scanditcaseinsensitive);
	cb_clog( priority, CBNEGATION, "\nstopatheaderend:             \t0x%.2XH", (**str).cf.stopatheaderend);
	cb_clog( priority, CBNEGATION, "\nstopatmessageend:            \t0x%.2XH", (**str).cf.stopatmessageend);
	cb_clog( priority, CBNEGATION, "\nstopatbyteeof:               \t0x%.2XH", (**str).cf.stopatbyteeof);
	cb_clog( priority, CBNEGATION, "\nstopateof:                   \t0x%.2XH", (**str).cf.stopateof);
	cb_clog( priority, CBNEGATION, "\nstopafterpartialread:        \t0x%.2XH", (**str).cf.stopafterpartialread);
	cb_clog( priority, CBNEGATION, "\nstopatjsonsyntaxerr:         \t0x%.2XH", (**str).cf.stopatjsonsyntaxerr);
	cb_clog( priority, CBNEGATION, "\nusesocket:                   \t0x%.2XH", (**str).cf.usesocket);
	cb_clog( priority, CBNEGATION, "\nremovewsp:                   \t0x%.2XH", (**str).cf.removewsp);
	cb_clog( priority, CBNEGATION, "\nremoveeof:                   \t0x%.2XH", (**str).cf.removeeof);
	cb_clog( priority, CBNEGATION, "\nremovecrlf:                  \t0x%.2XH", (**str).cf.removecrlf);
	cb_clog( priority, CBNEGATION, "\nremovesemicolon:             \t0x%.2XH", (**str).cf.removesemicolon);
	cb_clog( priority, CBNEGATION, "\nremovecommentsinname:        \t0x%.2XH", (**str).cf.removecommentsinname);
	cb_clog( priority, CBNEGATION, "\nremovenamewsp:               \t0x%.2XH", (**str).cf.removenamewsp);
	cb_clog( priority, CBNEGATION, "\nleadnames:                   \t0x%.2XH", (**str).cf.leadnames);
	cb_clog( priority, CBNEGATION, "\njson:                        \t0x%.2XH", (**str).cf.json);
	cb_clog( priority, CBNEGATION, "\njsonnamecheck:               \t0x%.2XH", (**str).cf.jsonnamecheck);
	cb_clog( priority, CBNEGATION, "\njsonvaluecheck:              \t0x%.2XH", (**str).cf.jsonvaluecheck);
	cb_clog( priority, CBNEGATION, "\njsonsyntaxcheck:             \t0x%.2XH", (**str).cf.jsonsyntaxcheck);
	cb_clog( priority, CBNEGATION, "\njsonremovebypassfromcontent: \t0x%.2XH", (**str).cf.jsonremovebypassfromcontent);
	cb_clog( priority, CBNEGATION, "\ndoubledelim:                 \t0x%.2XH", (**str).cf.doubledelim);
	cb_clog( priority, CBNEGATION, "\nfindwords:                   \t0x%.2XH", (**str).cf.findwords);
	cb_clog( priority, CBNEGATION, "\nfindwordstworends:           \t0x%.2XH", (**str).cf.findwordstworends);
	cb_clog( priority, CBNEGATION, "\nfindwordssql:                \t0x%.2XH", (**str).cf.findwordssql);
	cb_clog( priority, CBNEGATION, "\nfindwordssqlplusbypass:      \t0x%.2XH", (**str).cf.findwordssqlplusbypass);
	cb_clog( priority, CBNEGATION, "\nnamelist:                    \t0x%.2XH", (**str).cf.namelist);
	cb_clog( priority, CBNEGATION, "\nwordlist:                    \t0x%.2XH", (**str).cf.wordlist); // 5.10.2017
	cb_clog( priority, CBNEGATION, "\nsearchnameonly:              \t0x%.2XH", (**str).cf.searchnameonly);
	cb_clog( priority, CBNEGATION, "\nsearchrightfromroot:         \t0x%.2XH", (**str).cf.searchrightfromroot);
	if( (**str).cb!=NULL )
	cb_clog( priority, CBNEGATION, "\nbuffer size:                 \t%liB", (*(**str).cb).buflen);
	if( (**str).blk!=NULL )
	cb_clog( priority, CBNEGATION, "\nblock size:                  \t%liB", (*(**str).blk).buflen);
	cb_clog( priority, CBNEGATION, "\nfile descriptor:             \t%i", (**str).fd);
	cb_clog( priority, CBNEGATION, "\nmessage offset:              \t%li", (*(**str).cb).messageoffset );
	cb_clog( priority, CBNEGATION, "\nmessage header offset:       \t%i", (*(**str).cb).headeroffset );
	cb_clog( priority, CBNEGATION, "\neof offset:                  \t%li", (*(**str).cb).eofoffset );
	cb_clog( priority, CBNEGATION, "\nindex:                       \t%li", (*(**str).cb).index );
	cb_clog( priority, CBNEGATION, "\nblock index:                 \t%li", (*(**str).blk).index );
	cb_clog( priority, CBNEGATION, "\nnonblocking:                 \t0x%.2XH", (**str).cf.nonblocking);
	cb_clog( priority, CBNEGATION, "\nO_NONBLOCK:                  \t");
        err = fcntl( (**str).fd, F_GETFL, O_NONBLOCK );
        if( err>=0 && ( err & O_NONBLOCK ) == O_NONBLOCK )
		cb_clog( priority, CBNEGATION, "0x01H");
	else
		cb_clog( priority, CBNEGATION, "0x00H");
	cb_clog( priority, CBNEGATION, "\nrstart:                      \t[%c, 0x%.2X]", (signed char) (**str).cf.rstart, (signed char) (**str).cf.rstart );
	cb_clog( priority, CBNEGATION, "\nrend:                        \t[%c, 0x%.2X]", (signed char) (**str).cf.rend, (signed char) (**str).cf.rend );
	cb_clog( priority, CBNEGATION, "\nsubrstart:                   \t[%c, 0x%.2X]", (signed char) (**str).cf.subrstart, (signed char) (**str).cf.subrstart );
	cb_clog( priority, CBNEGATION, "\nsubrend:                     \t[%c, 0x%.2X]", (signed char) (**str).cf.subrend, (signed char) (**str).cf.subrend );
	cb_clog( priority, CBNEGATION, "\nthirdrend:                   \t[%c, 0x%.2X]", (signed char) (**str).cf.thirdrend, (signed char) (**str).cf.thirdrend );
	cb_clog( priority, CBNEGATION, "\nbypass:                      \t[%c, 0x%.2X]", (signed char) (**str).cf.bypass, (signed char) (**str).cf.bypass );
	cb_clog( priority, CBNEGATION, "\ncstart:                      \t[%c, 0x%.2X]", (signed char) (**str).cf.cstart, (signed char) (**str).cf.cstart );
	cb_clog( priority, CBNEGATION, "\ncend:                        \t[%c, 0x%.2X]\n", (signed char) (**str).cf.cend, (signed char) (**str).cf.cend);
	return CBSUCCESS;
}
#ifdef CBBENCHMARK
signed int  cb_print_benchmark(cb_benchmark *bm){
        if(bm==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_print_benchmark: allocation error (%i).", CBERRALLOC); return CBERRALLOC; }
        cb_clog( CBLOGDEBUG, CBNEGATION, "reads %lli ", (*bm).reads );
        cb_clog( CBLOGDEBUG, CBNEGATION, "bytereads %lli ", (*bm).bytereads );
        return CBSUCCESS;
}
#endif
signed int  cb_print_leaves(cb_name **cbn, signed int priority){
	cb_name *ptr = NULL;
	if(cbn==NULL || *cbn==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_print_leaves: allocation error (%i).", CBERRALLOC); return CBERRALLOC; }
	if( (**cbn).leaf==NULL ) return CBEMPTY; // 14.11.2015
	ptr = &(* (cb_name*) (**cbn).leaf );
	return cb_print_leaves_inner( &ptr, priority );
}
signed int  cb_print_leaves_inner(cb_name **cbn, signed int priority){
	signed int err = CBSUCCESS;
	cb_name *iter = NULL, *leaf = NULL;
	if(cbn==NULL || *cbn==NULL){
	  //cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_print_leaves_inner: cbn was null.");
	  return CBSUCCESS;
	}

	iter = &(**cbn);
	if(iter==NULL){ cb_clog( CBLOGINFO, CBNEGATION, "\ncb_print_leaves_inner: iter was null."); return CBSUCCESS; }
        if( iter!=NULL ){ // Name
	  // Self
	  if(iter!=NULL){
	    if((*iter).namelen!=0){
	      cb_clog( priority, CBNEGATION, "\"");
	      if( (*iter).namebuf!=NULL && (*iter).namelen>0 && (*iter).buflen>0 ) // 14.11.2015
	        cb_print_ucs_chrbuf( priority, &(*iter).namebuf, (*iter).namelen, (*iter).buflen); // PRINT NAME
	      cb_clog( priority, CBNEGATION, "\"(%i,%li)", (*iter).group, (*iter).matchcount );
	      if( (*iter).leaf==NULL && (*iter).next!=NULL)
	        cb_clog( priority, CBNEGATION, ",");
	      else if( (*iter).leaf!=NULL)
	        cb_clog( priority, CBNEGATION, ":");
	    }
	  }
	  // Left
	  if( (*iter).leaf != NULL ){
	    leaf = &(* (cb_name*) (*iter).leaf);
	    cb_clog( priority, CBNEGATION, "{");
	    err = cb_print_leaves_inner( &leaf, priority ); // LEFT
	    cb_clog( priority, CBNEGATION, "}");
	  }
	  // Right
	  if( (*iter).next != NULL ){
	    leaf = &(* (cb_name*) (*iter).next);
	    err = cb_print_leaves_inner( &leaf, priority ); // RIGHT
	  }
	}
	return err;
}

signed int  cb_print_name(cb_name **cbn, signed int priority){
	signed int err=CBSUCCESS;
	if( cbn==NULL || *cbn==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_print_name: cbn was null." ); return CBERRALLOC; }

        cb_clog( priority, CBNEGATION, " name [" );
	if( (**cbn).namebuf!=NULL && (**cbn).buflen>0 )
          cb_print_ucs_chrbuf( priority, &(**cbn).namebuf, (**cbn).namelen, (**cbn).buflen);
        cb_clog( priority, CBNEGATION, "] namelen [%i] offset [%li] length [%i]",  (**cbn).namelen, (**cbn).offset, (**cbn).length); // 6.12.2014
        cb_clog( priority, CBNEGATION, " nameoffset [%li]\n", (**cbn).nameoffset);
        cb_clog( priority, CBNEGATION, " matchcount [%li]", (**cbn).matchcount);
        cb_clog( priority, CBNEGATION, " group [%i]", (**cbn).group);
        cb_clog( priority, CBNEGATION, " first found [%li] (seconds)", (signed long int) (**cbn).firsttimefound);
        cb_clog( priority, CBNEGATION, " last used [%li]", (**cbn).lasttimeused);
        cb_clog( priority, CBNEGATION, " matched rend [%li]\n", (**cbn).matched_rend);
        return err;
}
signed int  cb_print_names(CBFILE **str, signed int priority){
	cb_name *iter = NULL; signed int names=0;
	cb_name *ptr = NULL;
	if( str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_print_names: str was null." ); return CBERRALLOC; }
	//25.12.2021: cb_clog(  priority, CBNEGATION, "\n cb_print_names: \n");
	if(str!=NULL){
	  if( *str!=NULL && (**str).cb!=NULL ){
            if( (*(**str).cb).list.name!=NULL ){
              iter = &(*(*(**str).cb).list.name);
              do{
	        ++names;
	        cb_clog(  priority, CBNEGATION, " [%i/%lli]", names, (*(**str).cb).list.namecount ); // [%.2i/%.2li]
	        if(iter!=NULL)
	          cb_print_name( &iter, priority);
	        if( (**str).cf.searchstate==CBSTATETREE ){
	          cb_clog(  priority, CBNEGATION, "         tree: ");
	          cb_print_leaves( &iter, priority );
	          cb_clog(  priority, CBNEGATION, "\n");
	        }
		if( (*iter).next==NULL )
		  break;
		ptr = &(* (cb_name *) (*iter).next );
		iter = &(* ptr );
                //7.10.2021: iter = &(* (cb_name *) (*iter).next );
              }while( iter != NULL );
              return CBSUCCESS;
	    }else{
	      cb_clog(  priority, CBNEGATION, " namelist was empty");
              return CBSUCCESS; // 22.3.2016
	    }
	  }else{
	    cb_clog( priority, CBNEGATION, "\n *str or cb was null ");
	  }
	}else{
	  cb_clog( priority, CBNEGATION, "\n str was null "); 
	}
	//cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_print_names: str was null.");
        return CBERRALLOC;
}
// Debug
void cb_print_counters(CBFILE **cbf, signed int priority){
        if(cbf!=NULL && *cbf!=NULL){
          cb_clog(   priority, CBNEGATION, "\nnamecount:%lli \t index:%li       \tcontentlen:%li \tbuflen:%li       \treadlength:%li ", \
	    (*(**cbf).cb).list.namecount, (*(**cbf).cb).index, (*(**cbf).cb).contentlen, (*(**cbf).cb).buflen, (*(**cbf).cb).readlength );
	  cb_clog(   priority, CBNEGATION, "\ncontentlen:%li \t maxlength:%li   \theaderoffset:%i\tmessageoffset:%li\tfd:%i", (*(**cbf).cb).contentlen, (*(**cbf).cb).maxlength, (*(**cbf).cb).headeroffset, (*(**cbf).cb).messageoffset, (**cbf).fd );
	  cb_clog(   priority, CBNEGATION, "\nlastchr: [%c] \t lastchroffset:%li\tlast_level:%i  \tcurrent_root_level:%i\n", \
		(signed char)(*(**cbf).cb).list.rd.lastchr, (*(**cbf).cb).list.rd.lastchroffset, (*(**cbf).cb).list.rd.last_level, (*(**cbf).cb).list.rd.current_root_level );
	  cb_clog(   priority, CBNEGATION, "lastreadchrendedtovalue:%i\n", (*(**cbf).cb).list.rd.lastreadchrendedtovalue );
	  if( (*(**cbf).cb).list.name==NULL ){  cb_clog(   priority, CBNEGATION, "name was null, \t");  }else{  cb_clog(   priority, CBNEGATION, "name was not null, \t"); }
	  if( (*(**cbf).cb).list.last==NULL ){  cb_clog(   priority, CBNEGATION, "last was null, \t");  }else{  cb_clog(   priority, CBNEGATION, "last was not null, \t"); }
	  if( (*(**cbf).cb).list.current==NULL ){  cb_clog(   priority, CBNEGATION, "current was null, \t");  }else{  cb_clog(   priority, CBNEGATION, "current was not null, \t"); }
	  if( (*(**cbf).cb).list.currentleaf==NULL ){  cb_clog(   priority, CBNEGATION, "currentleaf was null, \t");  }else{  cb_clog(   priority, CBNEGATION, "currentleaf was not null, \t"); }
	  if( (*(**cbf).cb).list.rd.current_root==NULL ){ cb_clog( priority, CBNEGATION, "\ncurrent_root was null, \t");  }else{ cb_clog( priority, CBNEGATION, "\ncurrent_root was not null, \t"); }
	  if( (*(**cbf).cb).list.rd.last_name==NULL ){ cb_clog( priority, CBNEGATION, "last_name was null, \t");  }else{ cb_clog( priority, CBNEGATION, "last_name was not null, \t"); }
	}
}
signed int  cb_fill_empty_matchctl( cb_match *ctl ){
	if( ctl==NULL ) return CBERRALLOC;
	(*ctl).matchctl=1; (*ctl).resmcount=0; (*ctl).re=NULL;
	/*
	 * Search 'cb_set_to_name' list as 'UNIQUE' name. This
	 * search is used in setting the attribute as groupless
	 * in function 'cb_set_groupless'. */
	(*ctl).unique_namelist_search = 0; 
	(*ctl).errcode = 0; (*ctl).erroffset = 0;
	return CBSUCCESS;
}
signed int  cb_copy_name( cb_name **from, cb_name **to ){
	signed int index=0;
	if( from!=NULL && *from!=NULL && to!=NULL && *to!=NULL ){
	  if( (**from).namebuf==NULL || (**to).namebuf==NULL ) return CBERRALLOC; // 23.10.2015
	  for(index=0; index<(**from).namelen && index<(**to).buflen ; ++index)
	    (**to).namebuf[index] = (**from).namebuf[index];
	  (**to).namelen = index;
	  (**to).offset  = (**from).offset;
          (**to).length  = (**from).length;
	  (**to).nameoffset = (**from).nameoffset;
          (**to).matchcount = (**from).matchcount;
	  if( (**from).next!=NULL ){  (**to).next = &(* (cb_name*) (**from).next);  }else{  (**to).next = NULL; } // 23.7.2016
	  if( (**from).leaf!=NULL ){  (**to).leaf = &(* (cb_name*) (**from).leaf);  }else{  (**to).leaf = NULL; } // 23.7.2016
          (**to).firsttimefound = (**from).firsttimefound;
          (**to).lasttimeused = (**from).lasttimeused;
          (**to).group = (**from).group; // 11.11.2016
	  (**to).matched_rend = (**from).matched_rend; // 7.10.2021
	  return CBSUCCESS;
	}
	cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_copy_name: parameter was null." );
	return CBERRALLOC;
}

signed int  cb_init_name( cb_name **cbn  ){ // 22.7.2016
        if( cbn==NULL || *cbn==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_init_name: cbn was null."); return CBERRALLOC; }
	(**cbn).namebuf=NULL; // 8.5.2017
	(**cbn).namelen=0; // 8.5.2017
	(**cbn).buflen=0; // 8.5.2017
        (**cbn).offset=0;
        (**cbn).nameoffset=0;
        (**cbn).length=-1; // 11.12.2014
        (**cbn).matchcount=0;
        (**cbn).firsttimefound=-1;
        (**cbn).lasttimeused=-1;
	(**cbn).group=-1;
	(**cbn).matched_rend=(unsigned long int)0x20; // 1.10.2021
        //(**cbn).serial=-1;
        (**cbn).next=NULL;
        (**cbn).leaf=NULL;
        return CBSUCCESS;
}

signed int  cb_copy_conf( cb_conf *from, cb_conf *to ){
/***
# memcpy( &(* (void*) to), &(* (const void*) from), sizeof( cb_conf ) );
for I in type searchmethod leafsearchmethod searchstate unfold leadnames findleaffromallnames doubledelim findwords findwordstworends findwordssql searchnameonly namelist searchrightfromroot removenamewsp asciicaseinsensitive scanditcaseinsensitive removewsp  removeeof removecrlf removesemicolon removecommentsinname urldecodevalue json jsonnamecheck jsonremovebypassfromcontent jsonvaluecheck jsonsyntaxcheck usesocket  nonblocking stopatbyteeof stopateof stopafterpartialread stopatheaderend stopatmessageend  stopatjsonsyntaxerr rstart rend bypass cstart  cend subrstart subrend findwordssqlplusbypass
  do
     echo "        (*to).${I} = (*from).${I};"
 done
 ***/
        if( from==NULL || to==NULL ) return CBERRALLOC;
        (*to).type = (*from).type;
        (*to).searchmethod = (*from).searchmethod;
        (*to).leafsearchmethod = (*from).leafsearchmethod;
        (*to).searchstate = (*from).searchstate;
        (*to).unfold = (*from).unfold;
        (*to).leadnames = (*from).leadnames;
        (*to).findleaffromallnames = (*from).findleaffromallnames;
        (*to).doubledelim = (*from).doubledelim;
        (*to).findwords = (*from).findwords;
        (*to).findwordstworends = (*from).findwordstworends;
        (*to).findwordssql = (*from).findwordssql;
        (*to).findwordssqlplusbypass = (*from).findwordssqlplusbypass;
        (*to).searchnameonly = (*from).searchnameonly;
        (*to).namelist = (*from).namelist;
        (*to).wordlist = (*from).wordlist; // 5.10.2017
        (*to).searchrightfromroot = (*from).searchrightfromroot;
        (*to).removenamewsp = (*from).removenamewsp;
        (*to).asciicaseinsensitive = (*from).asciicaseinsensitive;
        (*to).scanditcaseinsensitive = (*from).scanditcaseinsensitive;
        (*to).removewsp = (*from).removewsp;
        (*to).removeeof = (*from).removeeof;
        (*to).removecrlf = (*from).removecrlf;
        (*to).removesemicolon = (*from).removesemicolon;
        (*to).removecommentsinname = (*from).removecommentsinname;
        (*to).urldecodevalue = (*from).urldecodevalue;
        (*to).json = (*from).json;
        (*to).jsonnamecheck = (*from).jsonnamecheck;
        (*to).jsonremovebypassfromcontent = (*from).jsonremovebypassfromcontent;
        (*to).jsonvaluecheck = (*from).jsonvaluecheck;
        (*to).jsonsyntaxcheck = (*from).jsonsyntaxcheck;
        (*to).usesocket = (*from).usesocket;
        (*to).nonblocking = (*from).nonblocking;
        (*to).stopatbyteeof = (*from).stopatbyteeof;
        (*to).stopateof = (*from).stopateof;
        (*to).stopafterpartialread = (*from).stopafterpartialread;
        (*to).stopatheaderend = (*from).stopatheaderend;
        (*to).stopatmessageend = (*from).stopatmessageend;
        (*to).stopatjsonsyntaxerr = (*from).stopatjsonsyntaxerr;
	(*to).rememberstopped = (*from).rememberstopped;
        (*to).rstart = (*from).rstart;
        (*to).rend = (*from).rend;
        (*to).bypass = (*from).bypass;
        (*to).cstart = (*from).cstart;
        (*to).cend = (*from).cend;
        (*to).subrstart = (*from).subrstart;
        (*to).subrend = (*from).subrend;
        (*to).thirdrend = (*from).thirdrend;
	return CBSUCCESS;
}

signed int  cb_allocate_name(cb_name **cbn, signed int namelen){
	//if( cbn==NULL ){
	//  cbn = (void*) malloc( sizeof( cb_name* ) ); // 8.11.2015, pointer size
	//  if( cbn==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_allocate_name: allocation error, malloc returned null."); return CBERRALLOC; }
	//}
	if( cbn==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_allocate_name: cbn was null."); return CBERRALLOC; }
	*cbn = (cb_name*) malloc( sizeof(cb_name) );
	if( *cbn==NULL){
	  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_allocate_name: malloc error, CBERRALLOC.");
	  return CBERRALLOC;
	}
	// 29.10.2018, prevent extra NULL:s appearing in the middle, just some character
	//30.10.2018: memset( &(**cbn), 0x20, (size_t) ( sizeof(cb_name) - 1 ) ); // 29.10.2018

/**
	(**cbn).offset=0;
	(**cbn).nameoffset=0;
	(**cbn).length=-1; // 11.12.2014
	(**cbn).matchcount=0;
	(**cbn).firsttimefound=-1;
	(**cbn).lasttimeused=-1;
	//(**cbn).serial=-1;
	(**cbn).next=NULL;
	(**cbn).leaf=NULL;
 **/
	cb_init_name( &(*cbn) );
	(**cbn).namebuf = (unsigned char*) malloc( sizeof(signed char)*( (unsigned int) namelen+1) ); // 7.12.2013
	if((**cbn).namebuf==NULL){
	  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_allocate_name: malloc returned null (namebuf)." );
	  return CBERRALLOC;
	}
	memset( &(**cbn).namebuf[0], 0x20, (size_t) namelen ); // 22.2.2015
	(**cbn).namebuf[namelen]='\0'; // 7.12.2013
	(**cbn).buflen=namelen; // 7.12.2013
	(**cbn).namelen=0;

	return CBSUCCESS;
}

signed int  cb_set_attributes_unread( CBFILE **cbs ){
	if( cbs==NULL || *cbs==NULL || (**cbs).cb==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_attributes_unread: cbs was null." ); return CBERRALLOC; }
	if( (*(**cbs).cb).list.name==NULL ) return CBSUCCESS;
	return cb_set_list_unread( &(*(**cbs).cb).list.name );
}
signed int  cb_set_leaf_unread( cb_name **cbname ){
        if( cbname==NULL || *cbname==NULL ) return CBERRALLOC;
	if( (**cbname).leaf==NULL )
		return CBSUCCESS;
	else
        	return cb_set_list_unread( &(*(**cbname).leaf) );
}
signed int  cb_set_list_unread( cb_name **cbname ){
	signed int err = CBSUCCESS;
	cb_name *ptr  = NULL;
	cb_name *ptr2 = NULL;
	if( cbname==NULL || *cbname==NULL ) return CBERRALLOC;
	ptr = &(**cbname) ;
	while( ptr!=NULL ){

		/*
		 * Set as unread. */
		(*ptr).matchcount=0;
		(*ptr).lasttimeused=-1;

		/*
		 * Next to leaf. */
		if( (*ptr).leaf!=NULL ){
			ptr2 = &(* (cb_name*) (*ptr).leaf );
			err  = cb_set_list_unread( &ptr2 );
			if( err>=CBERROR ){ cb_clog( CBLOGERR, err, "\ncb_set_list_unread: cb_set_list_unread, error %i.", err ); }
		}

		/*
		 * Next in list. */
		ptr2 = &(*ptr);
		if( (*ptr2).next!=NULL )
			ptr = &(* (void*) (*ptr2).next );
		else
			ptr = NULL;
	}
	return CBSUCCESS;
}

signed int  cb_set_cstart(CBFILE **str, unsigned long int cstart){ // comment start
	if(str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_cstart: str was null." ); return CBERRALLOC; }
	(**str).cf.cstart=cstart;
	return CBSUCCESS;
}
signed int  cb_set_cend(CBFILE **str, unsigned long int cend){ // comment end
	if(str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_cend: str was null." ); return CBERRALLOC; }
	(**str).cf.cend=cend;
	return CBSUCCESS;
}
signed int  cb_set_rstart(CBFILE **str, unsigned long int rstart){ // value start
	if(str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_rstart: str was null." ); return CBERRALLOC; }
	(**str).cf.rstart=rstart;
	return CBSUCCESS;
}
signed int  cb_set_rend(CBFILE **str, unsigned long int rend){ // value end
	if(str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_rend: str was null." ); return CBERRALLOC; }
	(**str).cf.rend=rend;
	return CBSUCCESS;
}
signed int  cb_set_bypass(CBFILE **str, unsigned long int bypass){ // bypass , new - added 19.12.2009
	if(str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_bypass: str was null." ); return CBERRALLOC; }
	(**str).cf.bypass=bypass;
	return CBSUCCESS;
}
signed int  cb_set_search_state(CBFILE **str, unsigned char state){
	if(str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_search_state: str was null." ); return CBERRALLOC; }
	if( state==CBSTATETREE || state==CBSTATELESS || state==CBSTATEFUL || state==CBSTATETOPOLOGY )
		(**str).cf.searchstate = state;
	else
		cb_clog(  CBLOGWARNING, CBERROR, "\nState not known.");
	return CBSUCCESS;
}
signed int  cb_get_search_state(CBFILE **str, unsigned char *state){
	if(state==NULL || str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_search_state: str was null." ); return CBERRALLOC; }
	*state = (**str).cf.searchstate;
	return CBSUCCESS;
}
signed int  cb_set_subrstart(CBFILE **str, unsigned long int subrstart){ // sublist value start
	if(str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_subrstart: str was null." ); return CBERRALLOC; }
	(**str).cf.subrstart=subrstart;
	return CBSUCCESS;
}
signed int  cb_set_subrend(CBFILE **str, unsigned long int subrend){ // sublist value end
	if(str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_subrend: str was null." ); return CBERRALLOC; }
	(**str).cf.subrend=subrend;
	return CBSUCCESS;
}
signed int  cb_set_thirdrend(CBFILE **str, unsigned long int thirdrend){ // value to use with rstart (doe not flip/flop or has any other meaning, 25.9.2021)
	if(str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_subrend: str was null." ); return CBERRALLOC; }
	(**str).cf.thirdrend=thirdrend;
	return CBSUCCESS;
}

signed int  cb_set_to_nonblocking(CBFILE **cbf){ // not yet tested, 23.5.2016
	signed int flags = 0;
	signed int err=CBSUCCESS;
	if( cbf==NULL || *cbf==NULL ) return CBERRALLOC;
	(**cbf).cf.nonblocking = 1;
	//26.1.2023: flags = fcntl( (**cbf).fd, F_GETFD );
	flags = fcntl( (**cbf).fd, F_GETFL );
	if(flags>=0){
		//26.1.2023: err = fcntl( (**cbf).fd, F_SETFD, (flags | O_NONBLOCK) );
		err = fcntl( (**cbf).fd, F_SETFL, (flags | O_NONBLOCK) );
	}
	if( err<0 || flags<0 ){
		cb_clog( CBLOGDEBUG, CBERRFILEOP, "\ncb_set_to_nonblocking_io: fcntl returned %i, errno %i '%s'.", err, errno, strerror( errno ) );
		return CBERRFILEOP;
	}
	return CBSUCCESS;
}
signed int  cb_increase_group_number( CBFILE **cbf ){
	if( cbf==NULL || *cbf==NULL || (**cbf).cb==NULL ) return CBERRALLOC;
	return cb_set_group_number( &(*cbf), ++(*(**cbf).cb).list.currentgroup );
}
signed int  cb_set_group_number( CBFILE **cbf, signed int grp ){
	if( cbf==NULL || *cbf==NULL || (**cbf).cb==NULL ) return CBERRALLOC;
	(*(**cbf).cb).list.currentgroup = grp;
	if( grp>(*(**cbf).cb).list.groupcount )
		(*(**cbf).cb).list.groupcount = grp;
	return CBSUCCESS;
}
signed int  cb_set_to_consecutive_group_names(CBFILE **cbf){
	if( cbf==NULL || *cbf==NULL ) return CBERRALLOC;
        return cb_set_search_method(&(*cbf), (unsigned char) CBSEARCHNEXTGROUPNAMES);
}
signed int  cb_set_to_consecutive_group_leaves(CBFILE **cbf){
	if( cbf==NULL || *cbf==NULL ) return CBERRALLOC;
        return cb_set_leaf_search_method(&(*cbf), (unsigned char) CBSEARCHNEXTGROUPLEAVES);
}
signed int  cb_set_to_consecutive_group_last_names(CBFILE **cbf){
	if( cbf==NULL || *cbf==NULL ) return CBERRALLOC;
        return cb_set_search_method(&(*cbf), (unsigned char) CBSEARCHNEXTLASTGROUPNAMES);
}
signed int  cb_set_to_consecutive_group_last_leaves(CBFILE **cbf){
	if( cbf==NULL || *cbf==NULL ) return CBERRALLOC;
        return cb_set_leaf_search_method(&(*cbf), (unsigned char) CBSEARCHNEXTLASTGROUPLEAVES);
}
signed int  cb_set_to_consecutive_names(CBFILE **cbf){
	if( cbf==NULL || *cbf==NULL ) return CBERRALLOC;
        return cb_set_search_method(&(*cbf), (unsigned char) CBSEARCHNEXTNAMES);
}
int  cb_set_to_consecutive_leaves(CBFILE **cbf){
	if( cbf==NULL || *cbf==NULL ) return CBERRALLOC;
        return cb_set_leaf_search_method(&(*cbf), (unsigned char) CBSEARCHNEXTLEAVES);
}
int  cb_set_to_unique_names(CBFILE **cbf){
	if( cbf==NULL || *cbf==NULL ) return CBERRALLOC;
        return cb_set_search_method(&(*cbf), (unsigned char) CBSEARCHUNIQUENAMES);
}
int  cb_set_to_unique_leaves(CBFILE **cbf){
	if( cbf==NULL || *cbf==NULL ) return CBERRALLOC;
        return cb_set_leaf_search_method(&(*cbf), (unsigned char) CBSEARCHUNIQUELEAVES);
}

int  cb_set_search_method(CBFILE **cbf, unsigned char method){
        if(cbf!=NULL){
          if((*cbf)!=NULL){
            (**cbf).cf.searchmethod=method;
            return CBSUCCESS;
          }
        }
	cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_search_method: cbf was null." );
        return CBERRALLOC;
}
int  cb_set_leaf_search_method(CBFILE **cbf, unsigned char method){
        if(cbf!=NULL){
          if((*cbf)!=NULL){
            (**cbf).cf.leafsearchmethod=method;
            return CBSUCCESS;
          }
        }
	cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_search_method: cbf was null." );
        return CBERRALLOC;
}

/*
 * Namelist without values: name1, name2, name3, ... */
/* Currently 23.8.2016, does not find. Lists names. Search again to find. */
int  cb_set_to_name_list_search( CBFILE **str ){
	if(str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_to_name_list_search: str was null." ); return CBERRALLOC; }
        cb_set_to_word_search( &(*str) );
        (**str).cf.findwords = 0;
        (**str).cf.leadnames = 1;
	(**str).cf.namelist  = 1; // add name at the end of stream
	(**str).cf.wordlist  = 0; // add name at the end of stream, even if atvalue was 1 (not: $name, instead: name, )
        cb_set_search_state( &(*str), CBSTATELESS );
        cb_set_rend( &(*str), (unsigned long int) ',');
        // rstart is not needed and should not be written in the text
	return CBSUCCESS;
}

/*
 * Wordlist is different from the other search settings. Rend and rstart are backwards.
 * The setting works only with CBSTATEFUL and with the following settings (20.3.2016):
 * findwords, removewsp, removesemicolon, removecrlf, removenamewsp, unfold.
 *
 * Just any settings can be tried with other search settings. Not with word search or search
 * of one name only.
 */
int  cb_set_to_word_search( CBFILE **str ){
	if(str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_to_word_search: str was null." ); return CBERRALLOC; }
	(**str).cf.findwords=1;
	(**str).cf.namelist=1; // add name at the end of stream, 23.8.2016
	(**str).cf.wordlist=1; // add name at the end of stream if the cursor was at attribute name, 7.10.2017
	(**str).cf.doubledelim=0;
	(**str).cf.json=0;
	(**str).cf.removewsp=1;
	(**str).cf.removesemicolon=1;
	(**str).cf.removecrlf=1;
	(**str).cf.removenamewsp=1;
	(**str).cf.unfold=1; // BUG with CBSTATELESS, (**str).cf.leadnames=0, removewsp=1, removecrlf=1, removenamewsp=1, doubledelim=0, 20.3.2016
	//(**str).cf.unfold=0;
	(**str).cf.leadnames=0;
	//(**str).cf.leadnames=1;
	cb_set_search_state( &(*str), CBSTATEFUL );
	//cb_set_search_state( &(*str), CBSTATELESS );
	cb_set_rstart( &(*str), (unsigned long int) ',' ); // default value (CSV, SQL, ...), name separator (record start)
	cb_set_rend( &(*str), (unsigned long int) '$' ); // default value (shell), record end, start of name
	cb_set_subrend( &(*str), (unsigned long int) '&' ); // record end, start of name, 3.11.2016
	cb_set_thirdrend( &(*str), (unsigned long int) -1 ); // default value, not set, 25.9.2021
	return CBSUCCESS;
}
/*
 * Not well tested (20.3.2016). */
int  cb_set_to_search_one_name_only( CBFILE **str ){
	if(str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_to_search_one_name_only: str was null." ); return CBERRALLOC; }
	(**str).cf.findwords=1;
	(**str).cf.doubledelim=0;
	(**str).cf.searchnameonly=1; // Stop at found name and never save any names to a list or tree
	(**str).cf.json=0;
	(**str).cf.removewsp=1;
	(**str).cf.removesemicolon=1;
	(**str).cf.removecrlf=1;
	(**str).cf.removenamewsp=1;
	(**str).cf.unfold=1;
	(**str).cf.leadnames=0;
	cb_set_search_state( &(*str), CBSTATELESS );
	cb_set_rstart( &(*str), (unsigned long int) '$' );
	cb_set_rend( &(*str), (unsigned long int) '<' );
	cb_set_thirdrend( &(*str), (unsigned long int) -1 );
	return CBSUCCESS;
}
signed int  cb_set_to_conf( CBFILE **str ){
	//
	// example:
	//
	// name {
	//    name2 = value2 ; # Comment
	//    # subrstart '=', rstart '{'
	//    name3 { name4 = value4 ; }
	// }
	//
	if(str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_to_conf: str was null." ); return CBERRALLOC; }
        cb_set_search_state( &(*str), CBSTATETREE );
        cb_use_as_file( &(*str) );
        cb_set_to_consecutive_names( &(*str) ); // 30.6.2015 (needed inside values, in leafs)
        cb_set_to_consecutive_leaves( &(*str) ); // 30.6.2015 (needed inside values, in leafs)
        cb_set_rstart( &(*str), (unsigned long int) '=' ); // BEFORE TEST 18.8.2015
        cb_set_rend( &(*str), (unsigned long int) ';' ); // BEFORE TEST 18.8.2015
        cb_set_subrstart( &(*str), (unsigned long int) '{' ); // BEFORE TEST 18.8.2015
        cb_set_subrend( &(*str), (unsigned long int) '}' ); // BEFORE TEST 18.8.2015
	cb_set_thirdrend( &(*str), (unsigned long int) -1 );
        cb_set_cstart( &(*str), (unsigned long int) '#' );
        cb_set_cend( &(*str), (unsigned long int) 0x0A ); // new line
        cb_set_bypass( &(*str), (unsigned long int) '\\' );
        (**str).cf.doubledelim=1;
        (**str).cf.removecrlf=1;
        (**str).cf.removewsp=1;
        (**str).cf.jsonnamecheck=0;
        (**str).cf.jsonvaluecheck=0;
        (**str).cf.jsonsyntaxcheck=0;
	(**str).cf.jsonremovebypassfromcontent=0; // 9.10.2016
        (**str).cf.json=0;
        (**str).cf.leadnames=0;
        //(**str).cf.rfc2822headerend=0;
        (**str).cf.stopatheaderend=0;
        (**str).cf.stopatmessageend=0;
        (**str).cf.asciicaseinsensitive=0;
        (**str).cf.unfold=1;
	(**str).cf.findwords=0;
	return CBSUCCESS;
}
signed int  cb_set_to_html_post_text_plain( CBFILE **str ){
	if(str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_to_html_text_plain: str was null." ); return CBERRALLOC; }
	cb_set_to_html_post( &(*str) );
	cb_set_rend( &(*str), (unsigned long int) 0x0D ); // <CR>
	cb_set_thirdrend( &(*str), (unsigned long int) -1 );
	(**str).cf.removecrlf = 1; // remove <LF> from name (Opera: last <LF> is missing. Chrome and Opera: <CR><LF>)
	return CBSUCCESS;
}
signed int  cb_set_to_html_post( CBFILE **str ){
	if(str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_to_html_post: str was null." ); return CBERRALLOC; }
        cb_set_rstart( &(*str), (unsigned long int) '=' );
        cb_set_rend( &(*str), (unsigned long int) '&' ); // URL-encoding
	cb_set_thirdrend( &(*str), (unsigned long int) -1 );
        cb_set_cstart( &(*str), (unsigned long int) 0x1D ); // 'GS' group separator, some non-alphanumeric characters (these have to be encoded in POST with percent sign and hexadecimal value)
        cb_set_cend( &(*str), (unsigned long int) 0x1F ); // 'US' unit separator, "
        cb_set_bypass( &(*str), (unsigned long int) '\\' ); // "semantically invisible" [2822 3.2.3], should be encoded with percent sign
	cb_set_search_state( &(*str), CBSTATEFUL );
        //(**str).cf.rfc2822headerend=0;
        (**str).cf.stopatheaderend=0;
        (**str).cf.stopatmessageend=1; // 9.8.2016
        (**str).cf.asciicaseinsensitive=0;
        (**str).cf.unfold=0;
        (**str).cf.doubledelim=0;
        (**str).cf.removecrlf=1;    // between name and value and in name (cr or lf has to be percent encoded anyway)
        (**str).cf.removewsp=1;     // between name and value
	(**str).cf.removenamewsp=0; // leave wsp:s in name
	(**str).cf.removesemicolon=0;
	(**str).cf.removecommentsinname=0;
	(**str).cf.searchnameonly=0;
        (**str).cf.jsonnamecheck=0;
        (**str).cf.jsonvaluecheck=0;
        (**str).cf.jsonsyntaxcheck=0;
	(**str).cf.jsonremovebypassfromcontent=0;
        (**str).cf.json=0;
        (**str).cf.leadnames=0;
	(**str).cf.findwords=0;
	// (**str).cf.stopateof=1;
	return CBSUCCESS;
}

signed int  cb_set_message_end( CBFILE **str, signed long int contentoffset ){
	if(str==NULL || *str==NULL || (**str).cb==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_message_end: str or cb was null." ); return CBERRALLOC; }
	if( contentoffset<0 ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_message_end: offset was negative." ); return CBOVERFLOW; }
	(*(**str).cb).messageoffset = contentoffset;
	return CBSUCCESS;
}
/*
 * Name usesocket may be misleading. Setting to read up to messagelength characters. */
signed int  cb_set_to_socket( CBFILE **str ){
        if(str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_socket: str was null." ); return CBERRALLOC; }
        (**str).cf.usesocket=1;
	(**str).cf.stopateof=0; // 9.6.2016
        return CBSUCCESS;
}

//signed int  cb_set_to_rfc2822( CBFILE **str ){
signed int  cb_set_to_message_format( CBFILE **str ){
	if(str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_to_rfc2822: str was null." ); return CBERRALLOC; }
        cb_set_rstart( &(*str), (unsigned long int) ':' );
        cb_set_rend( &(*str), (unsigned long int) 0x0A );
	cb_set_thirdrend( &(*str), (unsigned long int) -1 );
        cb_set_cstart( &(*str), (unsigned long int) '(' ); // rfc 2822 allowed comment, comments are folded [2822 3.2.3]
        cb_set_cend( &(*str), (unsigned long int) ')' ); // rfc 2822 allowed comment
        cb_set_bypass( &(*str), (unsigned long int) '\\' ); // "semantically invisible" [2822 3.2.3]
        //(**str).cf.rfc2822headerend=1;
        (**str).cf.stopatheaderend=1;
        (**str).cf.stopatmessageend=1;
        (**str).cf.asciicaseinsensitive=1;
        (**str).cf.unfold=1;
        (**str).cf.doubledelim=0;
        (**str).cf.removecrlf=0;
        (**str).cf.removewsp=0;
	(**str).cf.removenamewsp=0;
        (**str).cf.jsonnamecheck=0;
        (**str).cf.jsonvaluecheck=0;
        (**str).cf.jsonsyntaxcheck=0;
        (**str).cf.json=0;
        (**str).cf.leadnames=0;
	(**str).cf.findwords=0;
	(**str).cf.stopateof=0; // different content types, deflate method for example, 9.6.2016
	return CBSUCCESS;
}
signed int  cb_set_to_json( CBFILE **str ){
	if(str==NULL || *str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_to_json: str was null." ); return CBERRALLOC; }
	/*
	 * Tree structure of JSON is made with object curls. */
	cb_set_rstart( &(*str), (unsigned long int) ':'  );
	cb_set_rend( &(*str), (unsigned long int) ',' );
	/*
	 * Value from ':' to ',' or to '}' - arrays, (strings, numbers
	 * and literals) are considered here as a value. With setting
	 * 'json=1', '}' changes value even if, ',' was missing. See ascii
	 * diagram in function cb_put_leaf . */
	cb_set_subrstart( &(*str), (unsigned long int) '{' ); // object start
	cb_set_subrend( &(*str), (unsigned long int) '}' ); // object end
	cb_set_thirdrend( &(*str), (unsigned long int) -1 );
	(**str).cf.json = 1;
	(**str).cf.jsonnamecheck = 1; // check name before saving it to list or tree
        (**str).cf.jsonvaluecheck = 1; // additionally check the value if it is read with the functions from cb_read.c
	(**str).cf.jsonsyntaxcheck = 0; // additionally check the spaces between the value and the name, 14.7.2017, 17.7.2017
	(**str).cf.stopatjsonsyntaxerr = 0;
	(**str).cf.jsonremovebypassfromcontent = 1;
	(**str).cf.doubledelim = 1;
	/* JSON can't remove comments: JSON does not have comments and comments are attached to the array [ ] (8.11.2015)
	 * Arrays have commas inside them. inside array has been added and it is not tested without brackets as comment characters. */
	(**str).cf.removecommentsinname = 0; // JSON can't remove comments: JSON does not have comments and comments are attached to the array [ ] 
	(**str).cf.removecrlf = 1; // remove cr and lf character between value and name
	(**str).cf.removewsp = 1; // remove linear white space characters between value and name
	// (**str).cf.leadnames = 1; // leadnames are not in effect in CBSTATETREE
	(**str).cf.unfold = 0; // JSON is mostly used in values and larger aggregates (not in protocols)
	(**str).cf.findwords=0;
	(**str).cf.stopateof=1;
	/*
	 * JSON is Unicode encoded. */
	cb_set_encoding( &(*str), CBENCUTF8 );
	/*
	 * Arrays commas ',' are bypassed by setting array as a comment.
	 * This way array value can be read later and it wont interfere
	 * with values commas. */
	cb_set_cstart( &(*str), (unsigned long int) '[' );
	cb_set_cend( &(*str), (unsigned long int) ']' );
	/*
	 * Tree. */
	cb_set_search_state( &(*str), CBSTATETREE );

	return CBSUCCESS;
}

signed int  cb_allocate_cbfile(CBFILE **str, signed int fd, signed int bufsize, signed int blocksize){
	unsigned char *blk = NULL;
	return cb_allocate_cbfile_from_blk(str, fd, bufsize, &blk, blocksize);
}

signed int  cb_allocate_empty_cbfile(CBFILE **str, signed int fd){
	signed int err = CBSUCCESS;
	//if(str==NULL)
	//	str = (void*) malloc( sizeof( CBFILE* ) ); // 13.11.2015, pointer size
	if(str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_allocate_empty_cbfile: str was null." ); return CBERRALLOC; }

	*str = (CBFILE*) malloc(sizeof(CBFILE));
	if(*str==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_allocate_empty_cbfile: malloc returned null." ); return CBERRALLOC; }

	// Avoid extra NULL:s in between, 29.10.2018
	//30.10.2018: memset( &(**str), 0x20, (size_t) ( sizeof(CBFILE) - 1 ) ); // 29.10.2018

	(**str).cb = NULL; (**str).blk = NULL;
        (**str).cf.type=CBCFGSTREAM; // default
        //(**str).cf.type=CBCFGFILE; // first test was ok
	(**str).cf.searchstate=CBSTATETOPOLOGY;
	//(**str).cf.doubledelim=1; // default
	(**str).cf.doubledelim=0; // test
	(**str).cf.unfold=0;
	(**str).cf.asciicaseinsensitive=0;
	(**str).cf.scanditcaseinsensitive=0;
	(**str).cf.leadnames=0;
	(**str).cf.findleaffromallnames=0;
	(**str).cf.stopafterpartialread=0;
	(**str).cf.stopatbyteeof=0;
	(**str).cf.stopateof=1;
	//(**str).cf.usesocket=1; // test messageend every time if it is set, 2.5.2016
	(**str).cf.usesocket=0; // test messageend every time if it is set, 2.5.2016
	(**str).cf.nonblocking=0; // if set, fd should be set with fcntl flag O_NONBLOCK
	(**str).cf.findwords=0; // do not find words in list, instead be ready to use trees
	(**str).cf.findwordstworends=0; // use two rends (rend and subrend) with findwords
	(**str).cf.findwordssql=0;
	(**str).cf.findwordssqlplusbypass=0;
	(**str).cf.searchnameonly=0; // Find and save every name in list or tree
	(**str).cf.namelist=0;
	(**str).cf.wordlist=0;
	(**str).cf.jsonremovebypassfromcontent=1;
	(**str).cf.jsonvaluecheck=0;
	(**str).cf.jsonsyntaxcheck=0;
	(**str).cf.stopatjsonsyntaxerr=0;
	(**str).cf.jsonnamecheck=0;
	(**str).cf.urldecodevalue=0;
	(**str).cf.removecrlf=0;
	(**str).cf.removewsp=0;
	(**str).cf.removeeof=0;
	(**str).cf.removesemicolon=0; // wordlist has many differences because rend and rstart are backwards and not all settings are compatible, default is off
	(**str).cf.removenamewsp=0;
	(**str).cf.removecommentsinname=0;
	//(**str).cf.logpriority=CBLOGDEBUG; // removed 28.11.2016
        (**str).cf.searchmethod=CBSEARCHNEXTNAMES; // default
        //(**str).cf.searchmethod=CBSEARCHUNIQUENAMES;
	(**str).cf.leafsearchmethod=CBSEARCHNEXTLEAVES; // 11.5.2016, default
	(**str).cf.searchrightfromroot = 0; // 22.2.2017
        (**str).cf.stopatheaderend=0; // default
        (**str).cf.stopatmessageend=0; // default
#ifdef CBMESSAGEFORMAT
	(**str).cf.asciicaseinsensitive=1;
	(**str).cf.unfold=1;
	//(**str).cf.removewsp=1; // test
	//(**str).cf.removecrlf=1; // test
	(**str).cf.removewsp=0; // default
	(**str).cf.removecrlf=0; // default
	(**str).cf.stopatheaderend=1;  // default, stop at header end, 26.3.2016
	(**str).cf.stopatmessageend=1; // default, stop at message end, 26.3.2016
	//(**str).cf.rfc2822headerend=1; // default, stop at headerend
	//(**str).cf.rstart=0x00003A; // ':', default
	//(**str).cf.rend=0x00000A;   // LF, default
	(**str).cf.rstart=CBRESULTSTART; // tmp
	(**str).cf.rend=CBRESULTEND; // tmp
	(**str).cf.searchstate=CBSTATEFUL;
	(**str).cf.usesocket=1;
#else
	(**str).cf.asciicaseinsensitive=0; // default
	(**str).cf.unfold=0;
	(**str).cf.removewsp=1;
	(**str).cf.removecrlf=1;
	(**str).cf.stopatheaderend=0;
	(**str).cf.stopatmessageend=0;
	(**str).cf.usesocket=0;
	//(**str).cf.rfc2822headerend=0;
	(**str).cf.rstart=CBRESULTSTART;
	(**str).cf.rend=CBRESULTEND;
#endif
	(**str).cf.rememberstopped=0;
	(**str).cf.removenamewsp=0;
	(**str).cf.bypass=CBBYPASS;
	(**str).cf.cstart=CBCOMMENTSTART;
	(**str).cf.cend=CBCOMMENTEND;
	(**str).cf.subrstart=CBSUBRESULTSTART;
	(**str).cf.subrend=CBSUBRESULTEND;
	(**str).cf.thirdrend=-1;
	(**str).cf.searchstate=CBSTATELESS;
	(**str).cf.leadnames=0; // default
#ifdef CBSETSTATEFUL
	(**str).cf.searchstate=CBSTATEFUL;
	(**str).cf.leadnames=0;
#endif
#ifdef CBSETSTATETOPOLOGY
	(**str).cf.searchstate=CBSTATETOPOLOGY;
	(**str).cf.leadnames=1;
#endif
#ifdef CBSETSTATELESS
	(**str).cf.searchstate=CBSTATELESS;
	(**str).cf.leadnames=1;
#endif
#ifdef CBSETSTATETREE
	(**str).cf.searchstate=CBSTATETREE;
	(**str).cf.leadnames=1; // important
#endif
	(**str).cf.json=0;
	(**str).cf.jsonnamecheck=0;
        (**str).cf.jsonvaluecheck=0;
        (**str).cf.jsonsyntaxcheck=0;
	(**str).cf.stopatjsonsyntaxerr=0;
	(**str).cf.jsonremovebypassfromcontent=1;
	(**str).cf.removecommentsinname = 1;
#ifdef CBSETJSON
	(**str).cf.json=1;
	(**str).cf.jsonnamecheck=1;
        (**str).cf.jsonvaluecheck=1;
        (**str).cf.jsonsyntaxcheck=0;
	(**str).cf.stopatjsonsyntaxerr=0;
	(**str).cf.removecommentsinname = 0;
#endif
	//(**str).cf.leadnames=1; // test
	(**str).cf.findleaffromallnames=0;
	(**str).encodingbytes=CBDEFAULTENCODINGBYTES;
	(**str).encoding=CBDEFAULTENCODING;
	(**str).transferencoding=CBTRANSENCOCTETS;
	(**str).transferextension=CBNOEXTENSIONS;
	//(**str).fd = dup(fd); // tama tuli poistaa jo aiemmin? 21.8.2015
	(**str).fd = fd; // 30.6.2016
	(**str).id = -1; // 19.7.2021
	(**str).read_timeout = -1; // 20.7.2021
	(**str).write_timeout = -1; // 20.7.2021
	if((**str).fd == -1){ err = CBERRFILEOP; (**str).cf.type=CBCFGBUFFER; } // 20.8.2013

	cb_fifo_allocate_buffers( &(**str).ahd ); // 11.4.2019
	cb_fifo_init_counters( &(**str).ahd );

	(**str).cb_read_is_set = 0;
	(**str).cb_read = NULL;
	(**str).cb_write_is_set = 0;
	(**str).cb_write = NULL;

#ifdef CBBENCHMARK
        (**str).bm.reads=0;
        (**str).bm.bytereads=0;
#endif

	(**str).ahd.emptypad = 0; // 16.11.2018
	(**str).cf.emptypad = 0; // 16.11.2018
	(**str).emptypad2 = 0; // 16.11.2018

	return err;
}

signed int  cb_allocate_cbfile_from_blk(CBFILE **str, signed int fd, signed int bufsize, unsigned char **blk, signed int blklen){ 
	signed int err = CBSUCCESS;
	if( str==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_allocate_cbfile_from_blk: str was null." ); return CBERRALLOC; }
	err = cb_allocate_empty_cbfile( &(*str), fd );
	if(err!=CBSUCCESS && err!=CBERRFILEOP){ cb_clog( CBLOGALERT, err, "\ncb_allocate_cbfile_from_blk: cb_allocate_empty_cbfile, %i.", err ); return err; } // file operation error added 19.3.2016
	err = cb_allocate_buffer( &(**str).cb, bufsize );
	if(err!=CBSUCCESS){ cb_clog( CBLOGALERT, err, "\ncb_allocate_cbfile_from_blk: cb_allocate_buffer, %i.", err ); return err; }
	if(*blk==NULL){
	  err = cb_allocate_buffer( &(**str).blk, blklen );
	}else{
	  err = cb_allocate_buffer( &(**str).blk, 0 ); // blk
	  if(err==-1){ return CBERRALLOC; }
	  if( (*(**str).blk).buflen>0 && (*(**str).blk).buf!=NULL ){ // 5.8.2016
	    memset( &(*(**str).blk).buf[0], 0x20, (size_t) (*(**str).blk).buflen ); // 5.8.2016
	    (*(**str).blk).buf[ (*(**str).blk).buflen ] = '\0'; // 5.8.2016
	    free( (*(**str).blk).buf );
	  }
	  (*(**str).blk).buf = &(**blk);
	  (*(**str).blk).buflen = (long) blklen;
	  (*(**str).blk).contentlen = (long) blklen;
	}
	if(err==-1){ return CBERRALLOC;}
	return CBSUCCESS;
}

signed int  cb_allocate_buffer(cbuf **cbf, signed int bufsize){
	unsigned char *bl = NULL;
	return cb_allocate_buffer_from_blk( &(*cbf), &bl, bufsize );
}

signed int  cb_allocate_buffer_from_blk(cbuf **cbf, unsigned char **blk, signed int blksize){
	if( cbf==NULL ){ 
	  cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_allocate_buffer: cbf was null.");
	  return CBERRALLOC;
	}
	//if( *cbf==NULL ) // TEST 29.11.2016
	*cbf = (cbuf *) malloc(sizeof(cbuf));
	if( *cbf==NULL ){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_allocate_buffer: allocation error, malloc (%i).", CBERRALLOC ); return CBERRALLOC; } // 13.11.2015
	// Avoid extra NULL:s in between, write any character, 29.10.2018
	//30.10.2018: memset( &(**cbf), 0x20, (size_t) ( sizeof(cbuf) - 1 ) ); // 29.10.2018

	//8.10.2017: (**cbf).staticblockwasused = 0; // 6.10.2017
	return cb_init_buffer_from_blk( &(*cbf), &(*blk), blksize );
}
/*
 * Outside reuse. 28.2.2016
 *
 * Reinits block and buffer. Attaches the parameter block to the block
 * of CBFILE. This function is useful for a CBCFGBUFFER and CBCFGBOUNDLESSBUFFER
 * when the cb buffer is empty or fixed length. 19.3.2016
 *
 * Freeing the buffer blk after use has to be careful. The buffer may have to be
 * set as null before freeing the CBFILE.
 *
 * Block has to be managed by the programmer (free at end and set to point to
 * null), 22.8.2016.
 *
 */
signed int  cb_reinit_cbfile_from_blk( CBFILE **cbf, unsigned char **blk, signed int blksize ){
	signed int err = CBSUCCESS;
	if( cbf==NULL ){   cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_reinit_cbfile_from_blk: cbf was null." );  return CBERRALLOC; }
	if( *cbf==NULL ){  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_reinit_cbfile_from_blk: *cbf was null." );  return CBERRALLOC; }
	if( blk==NULL ){   cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_reinit_cbfile_from_blk: parameter was null." );  return CBERRALLOC; }
	if( (**cbf).cb==NULL ){  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_reinit_cbfile_from_blk: buffer was null." );  return CBERRALLOC; }
	//30.6.2016: if( (**cbf).blk==NULL ){  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_reinit_cbfile_from_blk: block was null." );  return CBERRALLOC; }
	if( (**cbf).blk==NULL ){  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_reinit_cbfile_from_blk: block was null." );  return CBERRALLOC; } // 27.10.2016

	//cb_clog( CBLOGDEBUG, CBSUCCESS, "\ncb_reinit_cbfile_from_blk: size %i, content [", blksize );
        //cb_print_ucs_chrbuf( 0, &(*blk), blksize, blksize );
        //cb_clog( CBLOGDEBUG, CBSUCCESS, "]." );

	/*
	 * Zero counters. */
	(*(**cbf).blk).buf = NULL; // 6.8.2017, has to be free'd elsewhere
	err = cb_reinit_cbfile( &(*cbf) ); // 19.3.2016

	/*
	 * Attach a new buffer to the block. */
	if( blk!=NULL && *blk!=NULL && blksize>=0 ){
	  (*(**cbf).blk).buf = &(**blk);
	  (*(**cbf).blk).buflen = blksize;
	  (*(**cbf).blk).contentlen = blksize; // 19.3.2016
 	  //8.10.2017: (*(**cbf).blk).staticblockwasused = 1; // 6.10.2017
	}else{
	  cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_reinit_cbfile_from_blk: parameter blk was null." );
	}

	return err;
}

signed int  cb_init_buffer_from_blk(cbuf **cbf, unsigned char **blk, signed int blksize){ 
	if( cbf==NULL || *cbf==NULL ){  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_init_buffer_from_blk: cbf was null." );  return CBERRALLOC; }
	if( blk==NULL || *blk==NULL ){
	  if( blksize == 0 )
	  	blksize = 2; // 25.4.2017, to be able to free the memory
	  else if( blksize < 0 )
		return CBERRALLOC;
	  (**cbf).buf = (unsigned char *) malloc( sizeof(signed char)*( (unsigned int) blksize+1 ) );
	  if( (**cbf).buf == NULL ){ 
		cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_init_buffer_from_blk: malloc returned null." ); 
		return CBERRALLOC; 
	  }
	  (**cbf).buf[blksize]='\0';
	}else{
	  (**cbf).buf = &(**blk);
	}

	// 28.2.2016, here: cb_empty_names( &(**cbf).cb ); ?

	memset( &(**cbf).buf[0], 0x20, (size_t) blksize ); // 21.2.2015 checked the place of memset is correct

	(**cbf).buflen=blksize;
	(**cbf).contentlen=0;
	(**cbf).list.namecount=0;
	(**cbf).list.nodecount=0;     // 28.10.2015
	(**cbf).list.groupcount=0;    // 10.11.2016 in testing
	(**cbf).list.currentgroup=0;  // 11.11.2016
	(**cbf).list.toterminal=0;
	//(**cbf).list.openpairs=0; // 28.9.2015
	/* To read from end of stream. */
	(**cbf).list.rd.lastchr=0x20; // 7.10.2015, 2.5.2016
	(**cbf).list.rd.lastchroffset=0; // 7.10.2015
	(**cbf).list.rd.lastreadchrendedtovalue=0; // 7.10.2015
	(**cbf).list.rd.syntaxerrorindx=-1; // 17.7.2017
	(**cbf).list.rd.syntaxerrorreason=-1; // 17.7.2017
	(**cbf).list.rd.encodingerroroccurred = CBSUCCESS; // 28.9.2017
	/* To read with cb_set_to_name and cb_set_to_leaf */
	(**cbf).list.rd.last_level = 0;
	(**cbf).list.rd.last_name  = NULL;
	(**cbf).list.rd.current_root_level = 0;
	(**cbf).list.rd.previous_root_level = 0;
	(**cbf).list.rd.current_root = NULL;
	(**cbf).list.rd.previous_root = NULL; // 4.3.2023
	(**cbf).list.rd.stopped = 0;

	//29.10.2018: padbytes( (**cbf).list.rd.pad4to64, 4 ); // 26.10.2018
	memset( &(**cbf).list.rd.pad4to64[0], 0x00, 4 ); // 4.3.2023
	//29.10.2018: padbytes( (**cbf).list.rd.pad5to64, 5 ); // 26.10.2018
	//29.10.2018: padbytes( (**cbf).list.pad4to64, 4 ); // 26.10.2018

	(**cbf).index=0;
	(**cbf).readlength=0; // 21.2.2015
	(**cbf).maxlength=0; // 21.2.2015
        //(**cbf).headeroffset=0; // 26.3.2016
        //(**cbf).messageoffset=0; // 26.3.2016
        (**cbf).headeroffset=-1; // 26.3.2016, 30.3.2016
        (**cbf).messageoffset=-1; // 26.3.2016, 30.3.2016
        (**cbf).eofoffset=-1; // 30.10.2016
	(**cbf).chunkmissingbytes=0; // 28.5.2016
	(**cbf).lastblockreadpartial=0; // 10.6.2016
	(**cbf).list.name=NULL;
	(**cbf).list.current=NULL;
	(**cbf).list.last=NULL;
	(**cbf).list.currentleaf=NULL; // 11.12.2013

	//4.3.2023: (**cbf).list.rd.emptypad = 0; // 16.11.2018
	(**cbf).list.emptypad = 0; // 16.11.2018
	(**cbf).emptypad = 0; // 16.11.2018

	return CBSUCCESS;
}

/*
 * messageoffset: see instructions from cb_reinit_buffer comments.*/
signed int  cb_reinit_cbfile(CBFILE **buf){
	signed int err=CBSUCCESS;
	if( buf==NULL || *buf==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_reinit_cbfile: buf was null." ); return CBERRALLOC; }

	if( (**buf).cb!=NULL ){ // 30.6.2016
		err = cb_reinit_buffer( &(**buf).cb );
		if(err>=CBERROR) cb_clog( CBLOGERR, err, "\ncb_reinit_cbfile: cb_reinit_buffer, error %i.", err);
	}
	if( (**buf).blk!=NULL ){ // 30.6.2016
		err = cb_reinit_buffer( &(**buf).blk );
		if(err>=CBERROR) cb_clog( CBLOGERR, err, "\ncb_reinit_cbfile: cb_reinit_buffer, error %i.", err);
	}

	err = cb_fifo_init_counters( &(**buf).ahd ); // 19.3.2016
	if(err>=CBERROR) cb_clog( CBLOGERR, err, "\ncb_reinit_cbfile: cb_fifo_init_counters, error %i.", err);

	cb_init_list_rd( &(**buf).cb ); // 5.8.2017

	//(**cbf).encodingbytes=CBDEFAULTENCODINGBYTES;
	//(**str).encoding=CBDEFAULTENCODING;
	//(**str).transferencoding=CBTRANSENCOCTETS;
	//(**str).transferextension=CBNOEXTENSIONS;

	return err;
}

signed int  cb_free_cbfile(CBFILE **buf){
	signed int err=CBSUCCESS;
	if( buf==NULL || *buf==NULL ) return CBSUCCESS; // 18.3.2016
	if( (**buf).cb!=NULL ){ // 18.3.2016
	  cb_reinit_buffer( &(**buf).cb ); // free names
	  if( (**buf).cb!=NULL && (*(**buf).cb).buf!=NULL ){
	    if( (*(**buf).cb).buflen>0 ){
	      //memset( &(*(*(**buf).cb).buf), 0x20, (size_t) ( (*(**buf).cb).buflen - 1 ) ); // 15.11.2015 write something to overwrite nulls, uncommented 15.7.2016
	      memset( &(*(*(**buf).cb).buf), 0x20, (size_t) (*(**buf).cb).buflen ); // 15.11.2015 write something to overwrite nulls, uncommented 15.7.2016, 5.8.2016
	      (*(**buf).cb).buf[ (*(**buf).cb).buflen ] = '\0'; // 17.7.2016
	    }
	    free( (*(**buf).cb).buf ); // free buffer data
	    (*(**buf).cb).buf = NULL; // 30.6.2016
	    (*(**buf).cb).buflen = 0; // 15.7.2016
	  }
	}
	if( (**buf).cb!=NULL ){
	  free((**buf).cb); // free buffer
	  (**buf).cb = NULL; // 30.6.2016
	}
	if( (**buf).blk!=NULL ){
	  if( (*(**buf).blk).buf!=NULL ){ // 15.7.2016
	    if( (*(**buf).blk).buflen>0 ){
	      //memset( &(*(*(**buf).blk).buf), 0x20, (size_t) ( (*(**buf).blk).buflen - 1 ) ); // write something to overwrite nulls, 15.7.2016
	      memset( &(*(*(**buf).blk).buf), 0x20, (size_t) (*(**buf).blk).buflen ); // write something to overwrite nulls, 15.7.2016, 5.8.2016
	      (*(**buf).blk).buf[ (*(**buf).blk).buflen ] = '\0'; // 17.7.2016
	    }
	    //8.10.2017: if( (*(**buf).blk).staticblockwasused!=1 ) // 6.10.2017, blk.buf has to be freed otherwice
            free( (*(**buf).blk).buf ); // free block data
	  }
	  (*(**buf).blk).buf = NULL;
	  (*(**buf).blk).buflen = 0; // 15.7.2016
	}
	if( (**buf).blk!=NULL ){ // 17.7.2016
		free((**buf).blk); // free block
		(**buf).blk = NULL; // 30.6.2016
	}
	if( (**buf).cf.type!=CBCFGBUFFER && (**buf).cf.type!=CBCFGBOUNDLESSBUFFER ){ // 20.8.2013, 28.2.2016
	  if( (**buf).fd!=-1 ){ // -1 chosen here (18.7.2016)
	    err = close( (**buf).fd ); // close stream
	    if(err==-1){ err=CBERRFILEOP; }
	  }
	}

	cb_fifo_free_buffers( &(**buf).ahd ); // 4.8.2021

	free(*buf); // free buf
	*buf = NULL; // 30.6.2016
	return err;
}


signed int  cb_free_buffer(cbuf **buf){
        signed int err=CBSUCCESS;
        err = cb_reinit_buffer( &(*buf) );
	memset( &(**buf).buf, 0x20, (size_t) (**buf).buflen ); // 15.11.2015
	(**buf).buf[ (**buf).buflen ] = '\0';
        free( (**buf).buf );
	(**buf).buf = NULL; // 30.6.2016
        free( *buf );
	*buf = NULL; // 30.6.2016
        return err;
}

signed int  cb_free_name(cb_name **name, signed int *freecount){
	cb_name *nptr = NULL;

	if( name==NULL || *name==NULL )
	  return CBERRALLOC;

	if( (**name).leaf!=NULL ){ // 15.11.2015
	   nptr = &(* (cb_name*) (**name).leaf );
           cb_free_name( &nptr, &(*freecount) ); // deepest leaf first, 27.2.2015
	}
	if( (**name).next!=NULL ){ // 15.11.2015
	   nptr = &(* (cb_name*) (**name).next );
	   cb_free_name( &nptr, &(*freecount) ); // 14.12.2014, nexts leafs
	}
	free( (**name).namebuf );
	(**name).namebuf=NULL; (**name).namelen=0; (**name).buflen=0;
	(**name).next=NULL; (**name).leaf=NULL; // 21.2.2015
	/* Set last name to NULL */
	free( (* (void**) name ) );
	*freecount+=1;
	*name=NULL;
	return CBSUCCESS;
}
/*
 * Zero everything.
 *
 * If messageend information is needed after calling this, save messageoffset and contentlength and
 * move the message offset contentlength backwards after calling this function (or cb_reinit_cbfile).
 */
signed int  cb_reinit_buffer(cbuf **buf){ // free names and init
	if(buf==NULL || *buf==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_reinit_buffer: buf was null." ); return CBERRALLOC; }

	(**buf).index=0;
	(**buf).headeroffset=-1; // 21.2.2015, 26.3.2016, 19.5.2016
	//if( (**buf).contentlen<(**buf).messageoffset && (**buf).messageoffset>0 && (**buf).contentlen>0 )
	//	(**buf).messageoffset -= (**buf).contentlen; // 19.5.2016
	//else
	(**buf).messageoffset=-1; // 26.2.2016, 19.5.2016
	(**buf).contentlen=0;
	(**buf).readlength=0; // 21.2.2015
	(**buf).maxlength=0; // 21.2.2015
	(**buf).list.current=NULL; // 1.6.2013
	(**buf).list.currentleaf=NULL; // 11.12.2013
	(**buf).list.last=NULL; // 1.6.2013
	cb_empty_names(&(*buf));
	(**buf).list.name=NULL; // 1.6.2013
	(**buf).list.namecount=0; // 21.2.2015
	(**buf).list.nodecount=0; // 15.11.2015
	(**buf).list.toterminal=0; // 29.9.2015
	//(**buf).list.openpairs=0; // 28.9.2015

	(**buf).list.currentgroup = 0; // 5.8.2017
	(**buf).list.groupcount = 0; // 5.8.2017

	cb_init_list_rd( &(*buf) ); // 5.8.2017

	if( (**buf).buf!=NULL ){ // 6.8.2017
           memset( &(**buf).buf[0], 0x20, (size_t) (**buf).buflen ); // 5.8.2017
 	   (**buf).buf[(**buf).buflen] = '\0';
	}

        (**buf).eofoffset=-1; // 30.10.2016, 5.8.2017
        (**buf).chunkmissingbytes=0; // 28.5.2016, 5.8.2017
        (**buf).lastblockreadpartial=0; // 10.6.2016, 5.8.2017

	return CBSUCCESS;
}

signed int  cb_init_list_rd( cbuf **buf ){ // 5.8.2017
	if(buf==NULL || *buf==NULL){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_init_list_rd: buf was null." ); return CBERRALLOC; }
        (**buf).list.rd.lastchr = 0x20;
        (**buf).list.rd.lastreadchrendedtovalue = 0;
        (**buf).list.rd.lastchroffset = 0;

	(**buf).list.rd.last_level = 0; // new 5.8.2017
	(**buf).list.rd.current_root_level = 0; // new 5.8.2017
	(**buf).list.rd.syntaxerrorreason = -1; // new 5.8.2017
	(**buf).list.rd.syntaxerrorindx = -1; // new 5.8.2017

	(**buf).list.rd.current_root = NULL;
	(**buf).list.rd.last_name = NULL;

	return CBSUCCESS;
}

/*
 * Used only with CBCFGSEEKABLEFILE (seekable) to clear the block before
 * writing in between with offset and after or reading from an offset.
 *
 * reading = 1 , reading, rewind with lseek buffers contentlength and empty buffer
 * reading = 0 , writing, append last block (cb_flush)
 *
 */
signed int  cb_empty_block(CBFILE **buf, signed char reading ){
	signed int err = CBSUCCESS;
	off_t lerr=0;
	if( buf==NULL || *buf==NULL || (**buf).blk==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_empty_block: buf was null." ); return CBERRALLOC; }
	if( (**buf).cf.type!=CBCFGSEEKABLEFILE ){ 	return CBOPERATIONNOTALLOWED; }
	if( reading==1 && (*(**buf).blk).contentlen > 0 ){ // rewind
		lerr = lseek( (**buf).fd, (off_t) 0-(*(**buf).blk).contentlen, SEEK_CUR );
	}else if( reading==0 && (*(**buf).blk).contentlen > 0 ){ // flush last, append
		err = cb_flush( &(*buf) );
		cb_clog( CBLOGALERT, CBERROR, "\ncb_empty_read_block: cb_flush, error %i.", err );
	}
	(*(**buf).blk).contentlen = 0;
	(*(**buf).blk).index = 0;
	if( lerr==-1 ){
	  // Extra logging 8.11.2017
	  //cb_clog(  CBLOGALERT, CBERROR, "\nLSEEK ERROR: FD %i, CONTENTLEN %li", (**buf).fd, (*(**buf).blk).contentlen );
	  cb_clog(  CBLOGALERT, CBERROR, "\ncb_empty_read_block: lseek returned -1 errno %i .", errno);
	  return CBERRFILEOP;
	}
	return CBSUCCESS;
}
signed int  cb_empty_names(cbuf **buf){
	signed int err=CBSUCCESS, freecount=0;
	if( buf==NULL || *buf==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_empty_names: buf was null." ); return CBERRALLOC; }

	if( (**buf).list.name==NULL )
		return CBSUCCESS; // 15.11.2015

	if( (**buf).list.name!=NULL ){
	  err = cb_free_name( &( (**buf).list.name ), &freecount );
	  //if( err>=CBNEGATION ){ cb_clog( CBLOGDEBUG, err, "\ncb_empty_names: cb_free_name returned %i.", err ); }
	}
	(**buf).list.namecount = 0; // namecount of main list (leafs are not counted)
	(**buf).list.nodecount = 0; // 15.11.2015
	(**buf).list.name = NULL;
	(**buf).list.last = NULL; // 21.2.2015
	(**buf).list.current = NULL; // 21.2.2015
	(**buf).list.currentleaf = NULL; // 21.2.2015

	(**buf).list.toterminal = 0; // 29.9.2015

	return err;
}
signed int  cb_free_names_from(cb_name **cbn, signed int *freecount){
	signed int err=CBSUCCESS;
	cb_name *name = NULL;
	//cb_name *nextname = NULL;
	if( cbn==NULL || *cbn==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_free_names_from: cbn was null." ); return CBERRALLOC; }
 	if( (**cbn).next==NULL )
		return CBSUCCESS;

	name = &(* (cb_name*) (**cbn).next);
	err = cb_free_name( &name, &(*freecount) ); // frees leaves and the names from the next
	(**cbn).next = NULL;

	return err;
}
signed int  cb_empty_names_from_name(cbuf **buf, cb_name **cbn){
	signed int err=CBSUCCESS, freecount=0;
	cb_name *name = NULL;
	cb_name *nextname = NULL;
	if( buf==NULL || *buf==NULL || cbn==NULL || *cbn==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_empty_names_from_name: parameter was null." ); return CBERRALLOC; }

	err = cb_free_names_from( &(*cbn), &freecount );
	if( err>=CBERROR ){ cb_clog( CBLOGDEBUG, err, "\ncb_empty_names_from_name: cb_free_names_from, error %i", err); return err; }

	(**buf).list.last = &(**cbn); // 21.2.2015
	(*(**buf).list.last).next = NULL; // 25.2.2015
	(**buf).list.current = NULL; // 21.2.2015
	(**buf).list.currentleaf = NULL; // 21.2.2015

	/* Update namecount. */
	err = 0;
	if( (**buf).list.name!=NULL ){
		name = &(* (cb_name*) (**buf).list.name);
		while(name != NULL){
			++err;
			nextname = &(* (cb_name*) (*name).next);
			name = &(* nextname);
		}
	}
	(**buf).list.namecount = err;
	(**buf).list.nodecount -= freecount; // 15.11.2015

	(**buf).list.toterminal = 0; // 15.11.2015, cbn must be a name, it is not verified here

	return CBSUCCESS;
}

signed int  cb_use_as_buffer(CBFILE **buf){
        return cb_set_type(&(*buf), (unsigned char) CBCFGBUFFER);
}
signed int  cb_use_as_boundless_buffer(CBFILE **buf){ // only as a block, not tested yet: 28.2.2016
        return cb_set_type(&(*buf), (unsigned char) CBCFGBOUNDLESSBUFFER);
}
signed int  cb_use_as_seekable_file(CBFILE **buf){
	cb_use_as_file( &(*buf) );
	return cb_set_type(&(*buf), (unsigned char) CBCFGSEEKABLEFILE);
}
signed int  cb_use_as_file(CBFILE **buf){
        return cb_set_type(&(*buf), (unsigned char) CBCFGFILE);
}
signed int  cb_use_as_stream(CBFILE **buf){
        return cb_set_type(&(*buf), (unsigned char) CBCFGSTREAM);
}
signed int  cb_set_type(CBFILE **buf, unsigned char type){
	if(buf!=NULL){
	  if((*buf)!=NULL){
	    (**buf).cf.type = type;
	    return CBSUCCESS;
	  }
	}
	cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_set_type: buf was null." );
	return CBERRALLOC;
}

signed int  cb_get_char_read_block(CBFILE **cbf, unsigned char *ch){
	if( cbf==NULL || *cbf==NULL || ch==NULL ){
	  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_get_char_read_block: parameter was null." );
	  return CBERRALLOC;
	}
	return cb_get_char_read_offset_block( &(*cbf), &(*ch), -1 );
}
//signed int  cb_get_char_read_offset_block(CBFILE **cbf, unsigned char *ch, signed long int offset, signed int transferencoding, signed int transferextension ){ 
signed int  cb_get_char_read_offset_block(CBFILE **cbf, unsigned char *ch, signed long int offset ){
	ssize_t sz=0;
	signed long int readlength=0; // 30.3.2016, 28.5.2016, 10.6.2016
	signed int err=CBSUCCESS; // 20.5.2016
	cblk *blk = NULL;
	//11.2.2023: if( cbf==NULL || *cbf==NULL || ch==NULL || (**cbf).blk==NULL || (*(**cbf).blk).buf==NULL ){ // 28.5.2016
	if( cbf==NULL || *cbf==NULL || ch==NULL || (**cbf).blk==NULL || ( (*(**cbf).blk).buflen>0 && (*(**cbf).blk).buf==NULL ) || (*(**cbf).blk).buflen<0 ){ // 28.5.2016, 11.2.2023
	  if( cbf==NULL || *cbf==NULL ) cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_get_char_read_offset_block: parameter CBFILE was null." );
	  if( (**cbf).blk==NULL ) cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_get_char_read_offset_block: parameters CBFILE block was null." );
	  else if( (*(**cbf).blk).buf==NULL ) cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_get_char_read_offset_block: parameters CBFILE blocks buffer was null." );
	  if( (**cbf).cb==NULL ) cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_get_char_read_offset_block: parameters CBFILE buffer was null." );
	  else if( (**cbf).cb==NULL ) cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_get_char_read_offset_block: parameters CBFILE buffer data was null." );
	  if( ch==NULL ) cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_get_char_read_offset_block: parameter 'ch' was null." );
	  else cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_get_char_read_offset_block: parameter was null." );
//cb_clog( CBLOGDEBUG, CBNEGATION, "[FA:%c]", *ch );
	  return CBERRALLOC;
	}
	if( offset > 0 && (**cbf).cf.type!=CBCFGSEEKABLEFILE ){
	  cb_clog( CBLOGERR, CBOPERATIONNOTALLOWED, "\ncb_get_char_read_offset_block: attempt to seek to offset of unwritable (unseekable) file (CBCFGSEEKABLEFILE is not set).");
//cb_clog( CBLOGDEBUG, CBNEGATION, "[FF:%c]", *ch );
	  return CBOPERATIONNOTALLOWED;
	}

	blk = &(*(**cbf).blk);      // 28.5.2016
	if(blk!=NULL){
	  if( ( (*blk).index < (*blk).contentlen ) && ( (*blk).contentlen <= (*blk).buflen ) ){
	    // return char
	    *ch = (*blk).buf[(*blk).index];
//cb_clog( CBLOGDEBUG, CBNEGATION, "[ch %c, index %li]", *ch, (*blk).index );
//21.7.2021: cb_clog( CBLOGDEBUG, CBNEGATION, "[%c]", *ch );
	    ++(*blk).index;
	  }else if( (**cbf).cf.type!=CBCFGBUFFER && (**cbf).cf.type!=CBCFGBOUNDLESSBUFFER && \
	             ! ( (**cbf).cf.rememberstopped==1 && (*(**cbf).cb).list.rd.stopped==1 ) ){ // 20.8.2013, 28.2.2016, 15.7.2017 rd.stopped
	    // read a block and return char
	    /*
	     * If write-operations are wanted in between file, the next is
	     * the only available option. Block has to be emptied before and
	     * after read or write. After write, flush. Buffers index and
	     * contentlength has to be set to current offset after every
	     * write.
	     */

	    readlength = (*blk).buflen; // 10.6.2016

	    //cb_clog( CBLOGDEBUG, CBNEGATION, "\nREAD (fd %i) usesocket=%i, messageend=%li, headerend=%i", (**cbf).fd, (**cbf).cf.usesocket, (*(**cbf).cb).messageoffset, (*(**cbf).cb).headeroffset );
	    //cb_clog( CBLOGDEBUG, CBNEGATION, ", nonblock=%i", ( fcntl( (**cbf).fd, F_GETFL, O_NONBLOCK ) & O_NONBLOCK ) );

	    if( (**cbf).cf.stopafterpartialread==1 && (*(**cbf).cb).lastblockreadpartial==1 ){ // 10.6.2016
	       (*(**cbf).cb).lastblockreadpartial=0;
//cb_clog( CBLOGDEBUG, CBNEGATION, "[F1:%c]", *ch );
	       return CBSTREAMEND;
	    }
	    if( (**cbf).cf.type==CBCFGSEEKABLEFILE && offset>0 ){ // offset from seekable file
	       /* Internal use only. Block has to be emptied after use. File pointer is not updated in the following: */
	       if( (**cbf).transferencoding!=CBTRANSENCOCTETS )
		  cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_get_char_read_offset_block: can't use transferencoding with offsets." );
	       sz = pread( (**cbf).fd, &(*(*blk).buf), (size_t)(*blk).buflen, offset ); // read block has to be emptied after use, 28.5.2016
	    }else{ // stream

	       if( (**cbf).cf.usesocket!=1 ){ // 29.3.2016
//test 20.7.2021 if( (**cbf).transferencoding==CBTRANSENCOCTETS ){
//                 	sz = read( (**cbf).fd, &(*(*blk).buf), (size_t)(*blk).buflen);
//		 }else{
			sz = cb_transfer_read( &err, &(*cbf), (*blk).buflen );
//		 }

               }else{ // 29.3.2016
                 readlength = (*blk).buflen;
                 if( (**cbf).cf.stopatheaderend==1 && (*(**cbf).cb).headeroffset>=0 && (*(**cbf).cb).contentlen<(*(**cbf).cb).headeroffset && \
                         ( (*(**cbf).cb).headeroffset - (*(**cbf).cb).contentlen )<(*blk).buflen )
                   readlength = (*(**cbf).cb).headeroffset  - (*(**cbf).cb).contentlen;
                 else if( (**cbf).cf.stopatmessageend==1 && (*(**cbf).cb).messageoffset>=0 && (*(**cbf).cb).contentlen<(*(**cbf).cb).messageoffset && \
                         ( (*(**cbf).cb).messageoffset - (*(**cbf).cb).contentlen )<(*blk).buflen )
                   readlength = (*(**cbf).cb).messageoffset - (*(**cbf).cb).contentlen;

		 if( (**cbf).cf.stopatmessageend==1 && (*(**cbf).cb).messageoffset>=0 && (*(**cbf).cb).messageoffset <= (*(**cbf).cb).contentlen ){
//cb_clog( CBLOGDEBUG, CBNEGATION, "[H1:%c]", *ch );
		    return CBMESSAGEEND;
		 }else if( (**cbf).cf.stopatheaderend==1 && (*(**cbf).cb).headeroffset>=0 && (*(**cbf).cb).headeroffset <= (*(**cbf).cb).contentlen ){
//cb_clog( CBLOGDEBUG, CBNEGATION, "[H2:%c]", *ch );
		    return CBMESSAGEHEADEREND;
		 }else{
//test 20.7.2021    if( (**cbf).transferencoding==CBTRANSENCOCTETS )
//                     sz = read( (**cbf).fd, &(*(*blk).buf), (size_t) readlength );
//		    else
		       sz = cb_transfer_read( &err, &(*cbf), readlength );
		 }
               }
	       //cb_clog( CBLOGDEBUG, CBSUCCESS, "\nblock read: %i (%i)", sz, (**cbf).fd );
	    }
// HERE
	    if( (signed long int) sz < readlength ) // 10.5.2016
	       (*(**cbf).cb).lastblockreadpartial=1;
	    else
	       (*(**cbf).cb).lastblockreadpartial=0;

	    (*blk).contentlen = (signed long int) sz; // 6.12.2014
	    if( 0 < (signed int) sz ){ // read more than zero bytes
	      (*blk).contentlen = (signed long int) sz; // 6.12.2014
	      (*blk).index = 0;
	      *ch = (*blk).buf[(*blk).index];
//cb_clog( CBLOGDEBUG, CBNEGATION, "|%c|", *ch );
	      ++(*blk).index;
	    }else{ // error or end of file
	      *ch = ' ';
	      (*blk).index = 0; // 20.7.2021 moved this line inside the EAGAIN block, 6.8.2021 as a test moved back here
	      if( sz<=0 && err==CBERRTIMEOUT ){ // error in reading, 18.7.2021, 20.7.2021
//cb_clog( CBLOGDEBUG, CBNEGATION, "|tyhja, blk.index=0|" );
//cb_clog( CBLOGDEBUG, CBSUCCESS, " CBERRTIMEOUT" ); 
//cb_flush_log();
		return CBERRTIMEOUT;
	      }else if( sz<0 && errno == EAGAIN ){ // errno from read (previous) and flag from fcntl
	        //6.8.2021: (*blk).index = 0; // 20.7.2021 moved this line inside the EAGAIN block
		err = fcntl( (**cbf).fd, F_GETFL, O_NONBLOCK );
		if( ( err & O_NONBLOCK ) == O_NONBLOCK ){
		  /*
		   * If non-blocking was configured, further data may be available later, 20.5.2016. (Late addition, propably missing from most configurations.) */
		  if( (**cbf).cf.nonblocking==0 ){
			cb_clog( CBLOGDEBUG, CBSTREAMEAGAIN, "\ncb_get_ch: EAGAIN returned and CBFILE is not set to nonblock.");
//cb_clog( CBLOGDEBUG, CBNEGATION, "[F3:%c]", *ch );
			return CBSTREAMEND; // 23.5.2016
		  }
		  return CBSTREAMEAGAIN; // 20.5.2016
		}else if(err<0){
		  cb_clog( CBLOGERR, CBNEGATION, "\ncb_get_ch: fcntl returned %i, errno %i '%s'.", err, errno, strerror( errno ) );
		}
	      }
	      //cb_clog( CBLOGDEBUG, CBSUCCESS, " CBSTREAMEND" );
//cb_clog( CBLOGDEBUG, CBNEGATION, "[F4:%c]", *ch );
	      return CBSTREAMEND; // pre 20.5.2016
	    }
	  }else{ // use as block
//cb_clog( CBLOGDEBUG, CBNEGATION, "[F5:%c]", *ch );
	    return CBSTREAMEND;
	  }
//cb_clog( CBLOGDEBUG, CBNEGATION, "[%c]", *ch );
	  return CBSUCCESS;
	}
	return CBERRALLOC;
}

signed int  cb_flush(CBFILE **cbs){
	if( cbs==NULL || *cbs==NULL ){
	  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_flush: cbs was null." );
	  return CBERRALLOC;
	}
	return cb_flush_to_offset( &(*cbs), -1 );
}

//signed int  cb_flush_cbfile_to_offset(cbuf **cb, signed int fd, signed long int offset, signed int transferencoding, signed int transferextension){
signed int  cb_flush_cbfile_to_offset(CBFILE **cbf, signed long int offset ){
	signed int err = CBSUCCESS, terr = CBSUCCESS; // err2=-1;
	if(cbf==NULL || *cbf==NULL || (**cbf).blk==NULL ){
	  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_flush_cbfile_to_offset: cb was null." );
	  return CBERRALLOC;
	}
        if( (*(**cbf).blk).buf==NULL ){
	  cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_flush_cbfile_to_offset: block buffer was null." );
	  return CBERRALLOC;
	}
	if( (*(**cbf).blk).contentlen <= (*(**cbf).blk).buflen ){
	  if( offset < 0 ){ // Append (usual)
/*** 20.7.2021
	    if( (**cbf).transferencoding==CBTRANSENCOCTETS ){
		if( (**cbf).fd<0 ){ // 1.8.2016
		    err = CBERRFILEOP;
		}else{
		    err = (signed int) write( (**cbf).fd, &(*(*(**cbf).blk).buf), (size_t) (*(**cbf).blk).contentlen );
		    if( err<0 )
			cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_flush_cbfile_to_offset: write %i, errno %i '%s'", err, errno, strerror( errno ) );
		}
	    }else{
 ***/
		err = cb_transfer_write( &terr, &(*cbf) );
//20.7.2021 }
	  }else{ // Write (replace)
	    // in the following, the file pointer is not updated ["Adv. Progr."]:
	    if( (**cbf).transferencoding!=CBTRANSENCOCTETS )
		cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_flush_cbfile_to_offset: can't use transferencoding with offsets." );
	    if( (**cbf).fd<0 )
		err = CBERRFILEOP;
	    else
	        err = (signed int) pwrite( (**cbf).fd, &(*(*(**cbf).blk).buf), (size_t) (*(**cbf).blk).contentlen, (off_t) offset );
	  }
	}else{
	  if( offset < 0 ){ // Append
/*** 20.7.2021
	    if( (**cbf).transferencoding==CBTRANSENCOCTETS ){
		if( (**cbf).fd<0 )
		  err = CBERRFILEOP;
	        else
		  err = (signed int) write( (**cbf).fd, &(*(*(**cbf).blk).buf), (size_t) (*(**cbf).blk).buflen );
	    }else{
 ***/
		err = cb_transfer_write( &terr, &(*cbf) );
//	    }
	  }else{ // Write (replace)
	    // in the following, the file pointer is not updated ["Adv. Progr."]:
	    if( (**cbf).transferencoding!=CBTRANSENCOCTETS )
		cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_flush_cbfile_to_offset: can't use transferencoding with offsets." );
	    if( (**cbf).fd<0 )
		err = CBERRFILEOP;
	    else
	        err = (signed int) pwrite( (**cbf).fd, &(*(*(**cbf).blk).buf), (size_t) (*(**cbf).blk).buflen, (off_t) offset );
	  }
	}
	if(err<0){
	  cb_clog( CBLOGINFO, CBERRFILEWRITE, "\ncb_flush_cbfile_to_offset: errno %i (fd %i).", errno, (**cbf).fd );
	  err = CBERRFILEWRITE ; 
	}else{
	  err = CBSUCCESS;
	  (*(**cbf).blk).contentlen=0; // Block is set to zero here (write or append is not possible without this)
	}
	//if(fd>=0){ // 18.3.2016
	//  err2 = fsync( fd ); // 2-3/2016
        //  if(err2<0)
        //    cb_clog( CBLOGNOTICE, CBNEGATION, "\ncb_flush_cbfile_to_offset: fsync %i, errno %i '%s'", err2, errno, strerror(errno) );
	//}
	return err;
}
signed int  cb_flush_to_offset(CBFILE **cbs, signed long int offset){
	if( cbs==NULL || *cbs==NULL ){
	  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_flush_to_offset: cbs was null." );
	  return CBERRALLOC;
	}

	if( *cbs!=NULL ){
 	  //if((**cbs).cf.type!=CBCFGBUFFER){
 	  if( (**cbs).cf.type!=CBCFGBUFFER && (**cbs).cf.type!=CBCFGBOUNDLESSBUFFER ){
	    if((**cbs).blk!=NULL){
	      if( offset > 0 && (**cbs).cf.type!=CBCFGSEEKABLEFILE ){
		cb_clog(  CBLOGERR, CBOPERATIONNOTALLOWED, "\ncb_flush_to_offset: attempt to seek to offset of unwritable file (CBCFGSEEKABLEFILE is not set).");
		return CBOPERATIONNOTALLOWED;
	      }
	      //return cb_flush_cbfile_to_offset( &(**cbs).blk, (**cbs).fd, offset, (**cbs).transferencoding, (**cbs).transferextension );
	      return cb_flush_cbfile_to_offset( &(*cbs), offset );
	    }
	  }else
	    return CBUSEDASBUFFER;
	}
	return CBERRALLOC;
}

signed int  cb_write(CBFILE **cbs, unsigned char *buf, signed long int size){
	signed int err=0;
	signed long int indx=0;
	if( cbs!=NULL && *cbs!=NULL && buf!=NULL){
	  if((**cbs).blk!=NULL){
	    for(indx=0; indx<size; ++indx){
	      err = cb_put_ch( &(*cbs), buf[indx]);
	    }
	    err = cb_flush( &(*cbs) );
	    return err;
	  }
	}
 	cb_clog( CBLOGDEBUG, CBERROR, "\ncb_write: parameter" );
	if( cbs==NULL || (**cbs).blk==NULL )
	 	cb_clog( CBLOGDEBUG, CBERROR, " cbs" );
	if( buf==NULL )
	 	cb_clog( CBLOGDEBUG, CBERROR, " buf" );
 	cb_clog( CBLOGDEBUG, CBERRALLOC, " was null." );
	return CBERRALLOC;
}
signed int  cb_write_cbuf(CBFILE **cbs, cbuf *cbf){
	if( cbs==NULL || *cbs==NULL || cbf==NULL || (*cbf).buf==NULL ){
	  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_write_cbuf: parameter was null." );
	  return CBERRALLOC;
	}
	return cb_write( &(*cbs), &(*(*cbf).buf), (*cbf).contentlen);
}


/*
 * This is append only function. Block has to be used individually to
 * write in arbitrary location in a file (with offset).
 */
signed int  cb_put_ch(CBFILE **cbs, unsigned char ch){ // 12.8.2013
	signed int err=CBSUCCESS;
	if( cbs==NULL || *cbs==NULL || (**cbs).blk==NULL ){
	   cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_put_ch: parameter was null." );
	   return CBERRALLOC;
	}
	if((**cbs).blk!=NULL){
cb_put_ch_put:
	  if((*(**cbs).blk).contentlen < (*(**cbs).blk).buflen ){
            (*(**cbs).blk).buf[(*(**cbs).blk).contentlen] = ch; // 12.8.2013
	    ++(*(**cbs).blk).contentlen;
	  //}else if((**cbs).cf.type!=CBCFGBUFFER){ // 20.8.2013
	  }else if( (**cbs).cf.type!=CBCFGBUFFER && (**cbs).cf.type!=CBCFGBOUNDLESSBUFFER ){ // 20.8.2013, 28.2.2016
	    //err = cb_flush(cbs); // new block
	    err = cb_flush( &(*cbs) ); // new block, 10.12.2014
	    goto cb_put_ch_put;
	  }else if( (**cbs).cf.type==CBCFGBUFFER || (**cbs).cf.type==CBCFGBOUNDLESSBUFFER ){ // 20.8.2013
	    return CBBUFFULL;
	  }
	  return err;
	}
	return CBERRALLOC;
}

signed int  cb_get_ch(CBFILE **cbs, unsigned char *ch){ // Copy ch to buffer and return it until end of buffer
	unsigned char chr=' '; signed int err=0;
#ifdef CBBENCHMARK
          ++(**cbs).bm.bytereads;
#endif
	if( cbs!=NULL && *cbs!=NULL ){

	  //ORIG: 6.8.2017: if( (*(**cbs).cb).index < (*(**cbs).cb).contentlen){
	  if( ( (*(**cbs).cb).index + 1 ) < (*(**cbs).cb).contentlen){ // TEST: first success, 6.8.2017, changed.
	     ++(*(**cbs).cb).index;
	     *ch = (*(**cbs).cb).buf[ (*(**cbs).cb).index ];
	     return CBSUCCESS;
	  }
	  *ch=' ';
	  // get char
	  //err = cb_get_char_read_block(cbs, &chr);
// 11.2.2023
	  err = cb_get_char_read_block( &(*cbs), &chr); // 16.8.2016

	  if( err == CBSTREAMEND || err >= CBERROR ){ return err; }
	  if( (**cbs).cf.stopatmessageend==1 && err == CBMESSAGEEND ){ return err; } // 30.3.2016
	  if( (**cbs).cf.stopatheaderend==1 && err == CBMESSAGEHEADEREND ){ return err; } // 30.3.2016

	  // copy char if name-value -buffer is not full
	  if((*(**cbs).cb).contentlen < (*(**cbs).cb).buflen ){
	    //if( chr != (unsigned char) EOF ){
	    if( ! ( (**cbs).cf.stopatbyteeof == 1 && chr == (unsigned char) EOF ) ){ // 9.6.2016
	      (*(**cbs).cb).buf[(*(**cbs).cb).contentlen] = chr;
	      ++(*(**cbs).cb).contentlen;
	      ++(*(**cbs).cb).readlength;
	      if((*(**cbs).cb).readlength > (*(**cbs).cb).maxlength )
	        (*(**cbs).cb).maxlength = (*(**cbs).cb).readlength;
	      (*(**cbs).cb).index = (*(**cbs).cb).contentlen;
	      *ch = chr;
	      return CBSUCCESS;
	    }else{
	      *ch=chr;
	      (*(**cbs).cb).index = (*(**cbs).cb).contentlen;
	      return CBSTREAMEND;
	    }
	  }else{
	    *ch=chr;
	    //if( chr == (unsigned char) EOF )
	    if( (**cbs).cf.stopatbyteeof == 1 && chr == (unsigned char) EOF ) // 9.6.2016
	      return CBSTREAMEND;
	    //if( (*(**cbs).cb).readlength < 0x7FFFFFFFFFFFFFFF ) // long long, > 64 bit
	    if( (*(**cbs).cb).readlength < 0x7FFFFFFF ) // long, > 32 bit
	      ++(*(**cbs).cb).readlength; // (seekable filedescriptor and index >= buflen)
	    if((*(**cbs).cb).readlength > (*(**cbs).cb).maxlength )
	      (*(**cbs).cb).maxlength = (*(**cbs).cb).readlength;
	    if((*(**cbs).cb).contentlen > (*(**cbs).cb).buflen && (**cbs).cf.type==CBCFGSEEKABLEFILE )
	      return CBFILESTREAM;
	    if((*(**cbs).cb).contentlen > (*(**cbs).cb).buflen )
	      return CBSTREAM;
	    return err; // at edge of buffer and stream
	  }
	}
	cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_get_ch: cbs was null." );
	return CBERRALLOC;
}

/*
 * CBFILE can be used as a block: set buffer size to 0.
 * These functions can be used to set and get blocks. The block
 * has to be in the encoding of the CBFILE.
 */

signed int  cb_free_cbfile_get_block(CBFILE **cbf, unsigned char **blk, signed int *blklen, signed int *contentlen){
	if( blklen==NULL || blk==NULL || *blk==NULL || cbf==NULL || *cbf==NULL || (**cbf).blk==NULL ){
	  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_free_cbfile_get_block: parameter was null." );
	  return CBERRALLOC;
	}
	(*blk) = &(*(**cbf).blk).buf[0];
	(*(**cbf).blk).buf = NULL;
	(*(**cbf).blk).buflen = 0; // 30.6.2016
	*contentlen = (signed int) (*(**cbf).blk).contentlen; // 24.10.2018
	*blklen = (signed int) (*(**cbf).blk).buflen; // 24.10.2018
	return cb_free_cbfile( cbf );
}

signed int  cb_get_buffer(cbuf *cbs, unsigned char **buf, signed long int *size){
        signed long int from=0, to=0;
        to = *size;
	if( cbs==NULL || buf==NULL || size==NULL ){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_get_buffer: parameter was null."); return CBERRALLOC; } // *buf is allocated next
        return cb_get_buffer_range( &(*cbs), &(*buf), &(*size), &from, &to );
}

// Allocates new buffer (or a block if cblk)
signed int  cb_get_buffer_range(cbuf *cbs, unsigned char **buf, signed long int *size, signed long int *from, signed long int *to){ 
        signed long int index=0;
        if( cbs==NULL || (*cbs).buf==NULL || from==NULL || to==NULL || size==NULL ){
	  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_get_buffer_range: parameter was null." );
	  return CBERRALLOC;
	}
	//if(buf==NULL )
	//  buf = (void*) malloc( sizeof( unsigned char* ) ); // 13.11.2015, pointer size
        if( buf==NULL ){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_get_sub_buffer: buf was null."); return CBERRALLOC; }
        *buf = (unsigned char *) malloc( sizeof(signed char)*( (unsigned long int)  *size+1 ) );
        if(*buf==NULL){ cb_clog( CBLOGALERT, CBERRALLOC, "\ncb_get_sub_buffer: malloc returned null."); return CBERRALLOC; }
        (*buf)[(*size)] = '\0';
        for(index=0;(index<(*to-*from) && index<(*size) && (index+*from)<(*cbs).contentlen); ++index){
          (*buf)[index] = (*cbs).buf[index+*from];
        }
        *size = index;
        return CBSUCCESS;
}

/*
 * Content of list may change in writing. Updating the namelist is in the
 * responsibility of the user. Buffer is swapped temporarily and function can
 * not be threaded unless it is used atomically.
 */
signed int  cb_write_to_offset(CBFILE **cbf, unsigned char **ucsbuf, signed int ucssize, signed int *byteswritten, signed long int offset, signed long int offsetlimit){
	signed int index=0, charcount=0; signed int err = CBSUCCESS;
	signed int bufindx=0, bytecount=0, storedsize=0;
	unsigned long int chr = 0x20;
	cbuf *origcb = NULL;
	cbuf *cb = NULL;
	cbuf  cbdata;
	unsigned char buf[CBSEEKABLEWRITESIZE+1];
	unsigned char *ubf = NULL;
	signed long int offindex = offset, contentlen = 0;

	if( cbf==NULL || *cbf==NULL || (**cbf).blk==NULL || byteswritten==NULL || ucsbuf==NULL || *ucsbuf==NULL ){
	  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_write_to_offset: parameter was null." );
	  return CBERRALLOC;
	}
	if( (**cbf).cf.type!=CBCFGSEEKABLEFILE ){
	  cb_clog(   CBLOGALERT, CBOPERATIONNOTALLOWED, "\ncb_write_to_offset: attempt to write to unseekable file, type %i. Returning CBOPERATIONNOTALLOWED .", (**cbf).cf.type );
	  return CBOPERATIONNOTALLOWED;
	}
	*byteswritten=0;

	//cb_remove_ahead_offset( &(*cbf), &(**cbf).ahd );
	//cb_remove_ahead_offset( &(*cbf) ); // 27.7.2015, poistettu 2.8.2015

	/* Replace block with small write block */
	origcb = &(*(**cbf).blk);
	if( (*(**cbf).blk).buflen < CBSEEKABLEWRITESIZE || (*(**cbf).blk).contentlen!=0 ){ // swap block if it is not flushed or smaller
		cb = &cbdata;
		buf[CBSEEKABLEWRITESIZE]='\0';
		ubf = &buf[0];
		cb_init_buffer_from_blk( &cb, &ubf, CBSEEKABLEWRITESIZE);
		(**cbf).blk = &(*cb);
	}

	//cb_clog(   CBLOGDEBUG, CBNEGATION, "\ncb_write_to_offset: data size %i, offset %li, offsetlimit %li, ", ucssize, offset, offsetlimit );
	//cb_clog(   CBLOGDEBUG, CBNEGATION, "blocksize %li .", (*(**cbf).blk).buflen );
	//cb_clog(   CBLOGDEBUG, CBNEGATION, "\ncb_write_to_offset: writing from %li size %i characters (r: %i) %i bytes [", offset, (ucssize/4), (ucssize%4), ucssize);
	//cb_print_ucs_chrbuf( CBLOGDEBUG, &(*ucsbuf), ucssize, ucssize);
	//cb_clog(   CBLOGDEBUG, CBNEGATION, "] :\n");

	// Write block length and flush to offset
	while( ucssize>=0 && bufindx<ucssize && err<CBERROR && err!=CBNOFIT ){
	  // Flush full block
	  if( index >= (*(**cbf).blk).buflen-6 && (*(**cbf).blk).buflen>=6 ){ // minus longest known bytecount of character
	    contentlen = (*(**cbf).blk).contentlen;
	    cb_flush_to_offset( &(*cbf), (offindex+1) );
	    offindex += contentlen;
	  }
	  // Write to block
	  err = cb_get_ucs_chr( &chr, &(*ucsbuf), &bufindx, ucssize);
	  ++charcount;
	  cb_put_chr( &(*cbf), chr, &bytecount, &storedsize );
	  index += storedsize;
	  *byteswritten += storedsize;
	  if( offindex >= offsetlimit || index+offset >= offsetlimit )
	    err = CBNOFIT;
	}
	// Flush the rest
	contentlen = (*(**cbf).blk).contentlen;
	err = cb_flush_to_offset( &(*cbf), (offindex+1) ); // does not update filepointer
	offindex += contentlen;

	// Return the original block (if it was swapped)
	(**cbf).blk = &(*origcb);

	// Return and error check
	if( charcount*4 < ucssize || index+offset >= offsetlimit )
	  return CBNOFIT;
        return CBSUCCESS;
}

/*
 * Storage data of one character is stored in stg and it's size to stgsize. */
signed int  cb_character_size(CBFILE **cbf, unsigned long int ucschr, unsigned char **stg, signed int *stgsize){
        signed int err = CBSUCCESS;
        signed int bcount=0;
        cbuf *origcb = NULL;
        cbuf *cb = NULL;
        cbuf  cbdata;
        unsigned char buf[6+1]; // longest known character+1
        unsigned char *ubf = NULL;

        if( cbf==NULL || *cbf==NULL || (**cbf).blk==NULL || stg==NULL || stgsize==NULL ){
	  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_character_size: parameter was null." );
	  return CBERRALLOC; 
	}

        //cb_remove_ahead_offset( &(*cbf), &(**cbf).ahd );
        //cb_remove_ahead_offset( &(*cbf) ); // 27.7.2015, poistettu 2.8.2015

        // Replace block with small write block
        cb = &cbdata;
        buf[6]='\0';
        ubf = &buf[0];
        cb_init_buffer_from_blk( &cb, &ubf, 6);

        origcb = &(*(**cbf).blk);
        (**cbf).blk = &(*cb);

        // Write character
        err = cb_put_chr( &(*cbf), ucschr, &bcount, &(*stgsize) );

        // Return the original block
        (**cbf).blk = &(*origcb);

        if(err>=CBNEGATION){
                *stgsize = 0;
                return err;
        }

        // Allocate and copy characterarray
        *stg = (unsigned char*) malloc( (size_t) sizeof( char )*( (unsigned int) (*stgsize) + 1 ) );
        if(*stg==NULL){
	  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_character_size: malloc returned null (%i).", CBERRALLOC);
	  return CBERRALLOC;
	}
        (*stg)[*stgsize] = '\0';
        for( bcount=(*stgsize-1) ; bcount>=0; --bcount){
          (*stg)[bcount] = buf[bcount];
        }

        return CBSUCCESS;
}


/*
 * Fills with chr:s from offset to offsetlimit. If not even at end, does not fill. */
signed int  cb_erase(CBFILE **cbf, unsigned long int chr, signed long int offset, signed long int offsetlimit){
        signed int err=CBSUCCESS, bcount=0;
        unsigned char *cdata = NULL;

        if( cbf==NULL || *cbf==NULL ){  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_erase: cbf was null."); return CBERRALLOC; }
        if( offset<0 || offsetlimit<0 ){
                cb_clog(   CBLOGERR, CBOVERFLOW, "\ncb_erase: warning, a parameter was negative.");
                return CBOVERFLOW;
        }

        cb_clog(   CBLOGDEBUG, CBNEGATION, "\ncb_erase: erasing from %li to limit %li (bytes in buffer).", offset, offsetlimit );

        if( (**cbf).cf.type!=CBCFGSEEKABLEFILE )
                if( (*(**cbf).cb).index >= (*(**cbf).cb).contentlen || (*(**cbf).cb).readlength>=(*(**cbf).cb).contentlen )
                        cb_clog(   CBLOGWARNING, CBERROR, "\ncb_erase: warning, attempting to erase a stream.");

        /*
         * Character size. (Slow and consuming.) */
        err = cb_character_size( &(*cbf), chr, &cdata, &bcount);
        if( err!=CBSUCCESS || cdata==NULL || bcount==0 ){ return err; }

	cb_clog(   CBLOGDEBUG, CBNEGATION, "\ncb_erase: charactersize %i .", bcount );

        /*
         * Write (( offsetlimit-offset/bcount )) characters and remainder. */
	err=0;
        while( err>=0 && offset <= ( offsetlimit - (bcount-1) ) ){
		cb_clog(   CBLOGDEBUG, CBNEGATION, "\ncb_erase: writing %i to %li .", bcount, offset );
                err = (signed int) pwrite( (**cbf).fd, &(*cdata), (size_t) bcount, (off_t) offset );
                offset+=bcount;
        }

	free(cdata);
        return err;
}

/*
 * Seek to beginning of file and renew all buffers and namelist.
 * For example to reread a configuration file.
 */
signed int  cb_reread_file( CBFILE **cbf ){
        return cb_reread_new_file( &(*cbf), -1 );
}

/*
 * Renews to new filedescriptor. Does not close old. */
signed int  cb_reread_new_file( CBFILE **cbf, signed int newfd ){
        signed int err2=CBSUCCESS;
	signed long long int err=CBSUCCESS;
        if( cbf==NULL || *cbf==NULL ){
	  cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_reread_new_file: cbf was null." );
	  return CBERRALLOC;
	}
        if( (**cbf).cf.type!=CBCFGSEEKABLEFILE ){
          cb_clog(   CBLOGWARNING, CBOPERATIONNOTALLOWED, "\ncb_seek: warning, trying to seek an unseekable file.");
          return CBOPERATIONNOTALLOWED;
        }
        // Remove readahead
        //err2 = cb_remove_ahead_offset( &(*cbf), &(**cbf).ahd );
        //err2 = cb_remove_ahead_offset( &(*cbf) ); // 27.7.2015, poistettu 2.8.2015
	cb_fifo_init_counters( &(**cbf).ahd ); // lisatty 2.8.2015
	if( err2>=CBNEGATION ){
	  cb_clog( CBLOGNOTICE, (signed char) err2, "\ncb_reread_file: could not remove ahead offset, %i.", err2);
	  if( err2>=CBERROR )
	    cb_clog( CBLOGWARNING, (signed char) err2, "\ncb_reread_file: Error %i.", err2);
	}

        // Do not close old filedescriptor to use it's file elsewhere (for example in child processes), even if new descriptor was given
        if( newfd>=0 ){
          (**cbf).fd = newfd; // attach new stream (for example old file was renamed and new file replaced old)
        }

        // Seek filedescriptor
        err = lseek( (**cbf).fd, (off_t) 0, SEEK_SET );
        if(err<0){
          cb_clog( CBLOGNOTICE, CBERRFILEOP, "\ncb_reread_file: could not seek new file descriptor, lseek errno %i .", errno );
          err2=CBERRFILEOP;
        }else{

	  // The next is the same as in cb_reinit_cbfile, 5.3.2016

          // Deallocate previous CBFILE:s buffers (without closing filedescriptor)
          err2 = cb_reinit_buffer(&(**cbf).cb); // free names, reset buffer
          err2 = cb_reinit_buffer(&(**cbf).blk); // reset block
          return err2;
        }
        return err2;
}

