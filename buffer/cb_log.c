/* 
 * Library to read and write streams. Searches valuepairs locations to a list. Uses different character encodings.
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>                        // abort
#include "../include/cb_buffer.h"

/*
 * Prints (**cbn).cf.logpriority level logging messages to stderr. 
 */

// http://clang.llvm.org/docs/AttributeReference.html#format-gnu-format
// https://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html

/*
 * Log if CBFILE can be attached to it. */
int  cb_log( CBFILE **cbn, char priority, int errtype, const char* restrict format, ... ){
	va_list argptr;
	if( cbn==NULL || *cbn==NULL ){ abort(); return CBERRALLOC; }
	if( (**cbn).cf.logpriority<priority ){
		if( errtype==CBERRALLOC )
			exit( errtype );
		return CBSUCCESS;
	}else{
		va_start( argptr, format );
		vfprintf( stderr, format, argptr );
		va_end( argptr );
	}
	if( priority==CBLOGDEBUG && errtype==CBERRALLOC ){
		fflush( stderr );
		abort();
	}else if( errtype==CBERRALLOC ){
		exit( errtype );
	}
	return CBSUCCESS;
}
/*
 * Common log. */
int  cb_clog( char priority, int errtype, const char* restrict format, ... ){
	va_list argptr;
	if( CBDEFAULTLOGPRIORITY<priority ){
		if( errtype==CBERRALLOC )
			exit( errtype );
		return CBSUCCESS;
	}else{
		va_start( argptr, format );
		vfprintf( stderr, format, argptr );
		va_end( argptr );
	}
	if( priority==CBLOGDEBUG && errtype==CBERRALLOC ){
		fflush( stderr );
		abort();
	}else if( errtype==CBERRALLOC ){
		exit( errtype );
	}
	return CBSUCCESS;
}

