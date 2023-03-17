/*
 * Timeout read transfer encoding.
 *
 * Copyright (C) 2021. Jouni Laakso
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
#include <errno.h>
#include <sys/select.h>
#include <unistd.h> // read
#include "../include/cb_buffer.h"
#include "../include/cb_encoding.h"
#include "../include/cb_read.h"


void cb_set_read_timeout( CBFILE *cbf, signed int timeout_in_seconds){
	if( cbf!=NULL ) (*cbf).read_timeout = timeout_in_seconds;
}
void cb_set_write_timeout( CBFILE *cbf, signed int timeout_in_seconds){
	if( cbf!=NULL ) (*cbf).write_timeout = timeout_in_seconds;
}
//23.2.2022: signed int  cb_set_timeout_read( CBFILE *cbf ){
signed int  cb_set_timeout_io( CBFILE *cbf ){
	if( cbf==NULL ) return CBERRALLOC;
	(*cbf).cb_read = &cb_timeout_read;
	(*cbf).cb_read_is_set = 1;
	(*cbf).cb_write = &cb_timeout_write;
	(*cbf).cb_write_is_set = 1;
	//23.2.2022 (*cbf).transferencoding = CBTRANSENCFUNCTIONPOINTERREAD;
	(*cbf).transferencoding = CBTRANSENCFUNCTIONPOINTER;
	if( (*cbf).read_timeout==-1 ) (*cbf).read_timeout = CBREADTIMEOUT;
	if( (*cbf).write_timeout==-1 ) (*cbf).write_timeout = CBREADTIMEOUT;
	return CBSUCCESS;
}
signed long int  cb_timeout_read( signed int *err, signed int fd, void *bf, signed long int nb, signed int wtime ){
        signed long int ret = -1;
        fd_set readfds;
        struct timeval tval;
        tval.tv_sec = wtime; tval.tv_usec = 0;
//cb_clog( CBLOGDEBUG, CBSUCCESS, "\ncb_timeout_read." ); cb_flush_log();
        if( bf==NULL ) return CBERRALLOC;
        FD_ZERO( &readfds );
        FD_SET( fd, &readfds );
//cb_clog( CBLOGDEBUG, CBSUCCESS, "\ncb_timeout_read: to select, timeout %i s, fd %i.", wtime, fd ); cb_flush_log();
        ret = (signed long int) select( (fd+1), &readfds, NULL, NULL, &tval );
//cb_clog( CBLOGDEBUG, CBSUCCESS, "\ncb_timeout_read: select, %i fd:s, fd %i.", (int) ret, fd ); cb_flush_log();
        if( ret>0 && FD_ISSET( fd, &readfds ) ){
                if( err!=NULL ){ *err = CBSUCCESS; }
//cb_clog( CBLOGDEBUG, CBSUCCESS, "\ncb_timeout_read: to read %i B.", (int) nb ); cb_flush_log();
                ret = (signed long int) read( fd, &(*bf), nb );
//cb_clog( CBLOGDEBUG, CBSUCCESS, "\ncb_timeout_read: read %i B.", (int) ret ); cb_flush_log();
                if( ret<0 ){
                        if( err!=NULL ){ *err = CBERRREAD; }
                        cb_clog( CBLOGDEBUG, CBSUCCESS, "\nRead error %i, errno %i '%s'.", CBERRREAD, errno, strerror( errno ) );
                }
                return ret;
        }
        if( ret==0 ){
                if( err!=NULL ){ *err = CBERRTIMEOUT; }
                cb_clog( CBLOGDEBUG, CBSUCCESS, "\nRead timeout after %is, %i.", wtime, CBERRTIMEOUT );
		cb_flush_log();
		return -1; // 18.7.2021
        }else if( ret<0 ){
                if( err!=NULL ){ *err = CBERRSELECT; }
                cb_clog( CBLOGDEBUG, CBSUCCESS, "\nRead error %i (select), errno %i '%s'.", CBERRSELECT, errno, strerror( errno ) );
		cb_flush_log();
        }
        return ret;
}

/****** removed 23.1.2022 ('cb_set_timeout_io')
signed int  cb_set_timeout_write( CBFILE *cbf ){
	if( cbf==NULL ) return CBERRALLOC;
	(*cbf).cb_write = &cb_timeout_write;
	(*cbf).cb_write_is_set = 1;
	//23.1.2022: (*cbf).transferencoding = CBTRANSENCFUNCTIONPOINTERWRITE;
	(*cbf).transferencoding = CBTRANSENCFUNCTIONPOINTER;
	if( (*cbf).write_timeout==-1 ) (*cbf).write_timeout = CBWRITETIMEOUT;
	return CBSUCCESS;
}
 ******/
signed long int  cb_timeout_write( signed int *err, signed int fd, void *bf, signed long int nb, signed int wtime ){
        signed long int ret = -1;
        fd_set writefds;
        struct timeval tval;
        tval.tv_sec = wtime; tval.tv_usec = 0;
//cb_clog( CBLOGDEBUG, CBSUCCESS, "\ncb_timeout_write." ); cb_flush_log();
        if( bf==NULL ) return CBERRALLOC;
        FD_ZERO( &writefds );
        FD_SET( fd, &writefds );
//cb_clog( CBLOGDEBUG, CBSUCCESS, "\ncb_timeout_write: to select, timeout %i s, fd %i.", timeout_write, fd ); cb_flush_log();
        ret = (signed long int) select( (fd+1), NULL, &writefds, NULL, &tval );
//cb_clog( CBLOGDEBUG, CBSUCCESS, "\ncb_timeout_write: select, %i fd:s, fd %i.", (int) ret, fd ); cb_flush_log();
        if( ret>0 && FD_ISSET( fd, &writefds ) ){
                if( err!=NULL ){ *err = CBSUCCESS; }
//cb_clog( CBLOGDEBUG, CBSUCCESS, "\ncb_timeout_write: to write %i B.", (int) nb ); cb_flush_log();
                ret = (signed long int) write( fd, &(*bf), nb );
//cb_clog( CBLOGDEBUG, CBSUCCESS, "\ncb_timeout_write: write %i B.", (int) ret ); cb_flush_log();
                if( ret<0 ){
                        if( err!=NULL ){ *err = CBERRREAD; }
                        cb_clog( CBLOGDEBUG, CBSUCCESS, "\nRead error %i, errno %i '%s'.", CBERRREAD, errno, strerror( errno ) );
                }
                return ret;
        }
        if( ret==0 ){
                if( err!=NULL ){ *err = CBERRTIMEOUT; }
                cb_clog( CBLOGDEBUG, CBSUCCESS, "\nWrite timeout after %is, %i.", wtime, CBERRTIMEOUT );
		cb_flush_log();
        }else if( ret<0 ){
                if( err!=NULL ){ *err = CBERRSELECT; }
                cb_clog( CBLOGDEBUG, CBSUCCESS, "\nWrite error %i (select), errno %i '%s'.", CBERRSELECT, errno, strerror( errno ) );
		cb_flush_log();
        }
        return ret;
}

