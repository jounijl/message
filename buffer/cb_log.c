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
#include <stdarg.h>
#include <stdlib.h>                        // abort
#include <sys/types.h>                     // write
#include <unistd.h>                        // write, STDERR_FILENO
#include "../include/cb_buffer.h"

/*
 * Prints (**cbn).cf.logpriority level logging messages to stderr. 
 */

// http://clang.llvm.org/docs/AttributeReference.html#format-gnu-format
// https://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html

/*
 * "C99 Standard" 6.7.8/10 (ISO/IEC 9899:TC3):
 * "If an object that has automatic storage duration is not initialized explicitly, its value is
 *  indeterminate. If an object that has static storage duration is not initialized explicitly,
 *  then:
 *  - if it has pointer type, it is initialized to a null pointer;
 *  - if it has arithmetic type, it is initialized to (positive or unsigned) zero;
 *  - if it is an aggregate, every member is initialized (recursively) according to these rules;
 *  - if it is a union, the first named member is initialized (recursively) according to these
 *  rules."
 *
 * The following is initialized to NULL (without the NULL value or with the NULL value):
 */
static CBFILE         *cblog = NULL;
static signed int      logpriority = CBLOGDEBUG;

int  cb_log_set_cbfile( CBFILE **cbf ){
	if( cbf==NULL || *cbf==NULL ) return CBERRALLOC;
	cblog = &( **cbf );
	return CBSUCCESS;
}

int  cb_log_get_logpriority( void ){
	return logpriority;
}
int  cb_log_set_logpriority( signed int logpr ){
	if(logpr<0 && logpriority>=0) return CBNOTSET;
	logpriority = logpr;
	return CBSUCCESS;
}

/*
 * Log. */
int  cb_clog( int priority, int errtype, const char* restrict format, ... ){
	int err = CBSUCCESS, indx=0, bc=0, sb=0;
	char snbuf[CBLOGLINELENGTH+1];
	unsigned char *snb = NULL;
	int  sncontentlen = 0;
	va_list argptr;
	snbuf[CBLOGLINELENGTH] = '\0';
	snb = &( (unsigned char*) snbuf)[0];
	if( ( logpriority>=priority && logpriority<CBLOGINDIVIDUAL ) || \
	 	( logpriority>=CBLOGINDIVIDUAL && logpriority<CBLOGMULTIPLE && priority==logpriority ) || \
		    ( logpriority>=CBLOGMULTIPLE && logpriority>=priority ) ){

  		/* Log only if exactly the loglevel and CBLOGINDIVIDUAL<loglevel<CBLOGMULTIPLE                            *
		 * Log if the loglevel>CBLOGMULTIPLE and CBLOGMULTIPLE<loglevel<CBLOGINDIVIDUAL or loglevel<CBLOGMULTIPLE */

		va_start( argptr, format );
		//vfprintf( stderr, format, argptr );
		sncontentlen = vsnprintf( &snbuf[0], CBLOGLINELENGTH, format, argptr ); // 28.11.2016
		va_end( argptr );
		if( cblog!=NULL && (*cblog).blk!=NULL && snb!=NULL ){
			for(indx=0; indx<sncontentlen && err<CBERROR; ++indx)
				err = cb_put_chr( &cblog, (unsigned long int) snb[ indx ], &sb, &bc ); // all character encodings
			//err = cb_write( &cblog, &(*snb), (long int) sncontentlen ); // byte by byte
			if(err>=CBERROR){ fprintf( stderr, "\ncb_log: cb_write, error %i.", err ); }
			cb_flush( &cblog ); // 22.12.2016
		}else{
			err = write( STDERR_FILENO, &snbuf[0], (size_t) sncontentlen );
			if(err<0){ fprintf( stderr, "\ncb_log: write, error %i.", err ); }
		}
	}
	if( errtype==CBERRALLOC ){
		if( cblog==NULL )
			fflush( stderr ); // write has already been done, fflush is wrong command here
		else
			cb_flush( &cblog );
		if( priority==CBLOGDEBUG )
			abort();
		else
			exit( errtype );
	}
	return CBSUCCESS;
}
