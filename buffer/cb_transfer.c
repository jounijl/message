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
#include "../include/cb_buffer.h"
#include "../include/cb_encoding.h"

static signed int  cb_flush_chunks( signed int fd, unsigned char *buf, signed long int contentlen );
static signed int  cb_read_chunk( signed long int *missingchunkbytes, signed int fd, unsigned char *buf, signed long int buflen );

/*
 * Return values like from 'read'. */
signed int  cb_transfer_read( signed int *err, CBFILE **cbf, signed long int readlength ){ // Not tested at all (not ready yet), 15.7.2016
	signed int sz=-1;
	if( err==NULL || cbf==NULL || *cbf==NULL || (**cbf).blk==NULL || (*(**cbf).blk).buf==NULL) return CBERRALLOC;
	if( (**cbf).fd<0 ) return CBERRFILEOP;
	if( readlength>(*(**cbf).blk).buflen ){
		readlength = (*(**cbf).blk).buflen;
	}
/*** test result: 100% cpu, no bytes read
     second test without: the same result, left like this (not well tested) 20.7.2021
 ***/
        if( cb_test_messageend( &(**cbf) )==CBSUCCESS ){
		// TEST 20.7.2021, if at end, do not find another.
cb_clog( CBLOGDEBUG, CBNEGATION, "\nCB:  TEST 20.7.2021: The message offset length was already read.");
cb_flush_log();
		return CBMESSAGEEND;
	}
	switch( (**cbf).transferextension ){
		case CBNOEXTENSIONS:
			break;
		default:
			cb_clog( CBLOGERR, CBERROR, "\ncb_transfer_write: invalid transfer coding extension, error %i.", CBERROR );
			break;
	}
	switch( (**cbf).transferencoding ){
		case CBTRANSENCOCTETS:
			if( (**cbf).blk==NULL || (*(**cbf).blk).buf==NULL ) return CBERRALLOC;
			sz = (signed int) read( (**cbf).fd, &(*(*(**cbf).blk).buf), (size_t) readlength );
			break;
		//23.1.2022: case CBTRANSENCFUNCTIONPOINTERREAD:
		case CBTRANSENCFUNCTIONPOINTER:
			if( (**cbf).cb_read_is_set==0 ) *err = CBERREMPTYENCODING;
			if( (**cbf).cb_read==NULL ) *err = CBERREMPTYENCODING;
/***
cb_clog( CBLOGDEBUG, CBSUCCESS, "\ncb_transfer_write: fd %i, messageoffset %li, index %li (blk index %li).", \
(**cbf).fd, (*(**cbf).cb).messageoffset, (*(**cbf).cb).index, (*(**cbf).blk).index );
cb_flush_log();
 ***/
			sz = (**cbf).cb_read( &(*err), (**cbf).fd, &(*(*(**cbf).blk).buf), readlength, (**cbf).read_timeout );
			if( sz==0 && *err==CBERRTIMEOUT ) sz = -1;
			break;
		case CBTRANSENCCHUNKS:
			sz = cb_read_chunk( &(*(**cbf).blk).chunkmissingbytes, (**cbf).fd, &(*(*(**cbf).blk).buf), readlength );
			break;
		default:
			sz = -1;
			cb_clog( CBLOGERR, CBERROR, "\ncb_transfer_read: invalid transfer encoding, error %i.", CBERROR );
			break;
	}
	return sz;
}
/*
 * Return values like from 'read'. */
signed int  cb_transfer_write( signed int *err, CBFILE **cbf ){ // Not yet well tested, 15.7.2017
	signed int wrote = -1;
	if( err==NULL || cbf==NULL || *cbf==NULL || (**cbf).blk==NULL || (*(**cbf).blk).buf==NULL || (**cbf).cb==NULL ){
		cb_clog( CBLOGERR, CBERRALLOC, "\ncb_read_chunk: parameter was null." ); return -1; 
	}
	if( (**cbf).fd<0 ){ cb_clog( CBLOGERR, CBERRFILEOP, "\ncb_transfer_write: fd was %i, error.", (**cbf).fd ); return -1; }
	switch( (**cbf).transferextension ){
		case CBNOEXTENSIONS:
			break;
		default:
			cb_clog( CBLOGERR, CBERROR, "\ncb_transfer_write: invalid transfer coding extension, error %i.", CBERROR );
			break;
	}
	switch( (**cbf).transferencoding ){
		case CBTRANSENCOCTETS:
			wrote = (signed int) write( (**cbf).fd, &(*(*(**cbf).blk).buf), (size_t) (*(**cbf).blk).contentlen );
			break;
		//23.1.2022: case CBTRANSENCFUNCTIONPOINTERWRITE:
		case CBTRANSENCFUNCTIONPOINTER:
			if( (**cbf).cb_write_is_set==0 ) *err = CBERREMPTYENCODING;
			if( (**cbf).cb_write==NULL ) *err = CBERREMPTYENCODING;
/***
cb_clog( CBLOGDEBUG, CBSUCCESS, "\ncb_transfer_write: fd %i, messageoffset %li, index %li (blk index %li).",
(**cbf).fd, (*(**cbf).cb).messageoffset, (*(**cbf).cb).index, (*(**cbf).blk).index );
cb_flush_log();
 ***/
			wrote = (**cbf).cb_write( &(*err), (**cbf).fd, &(*(*(**cbf).blk).buf), (*(**cbf).blk).contentlen, (**cbf).write_timeout );
			if( wrote==0 && *err!=CBSUCCESS ) wrote = -1;
			break;
		case CBTRANSENCCHUNKS:
			wrote = cb_flush_chunks( (**cbf).fd, &(*(*(**cbf).blk).buf), (*(**cbf).blk).contentlen );
			break;
		default:
			wrote = -1;
			cb_clog( CBLOGERR, CBERROR, "\ncb_transfer_write: invalid transfer encoding %i, error %i.", (signed int) (**cbf).transferencoding, wrote );
			break;
	}
	return wrote;
}
signed int  cb_terminate_transfer( CBFILE **cbf ){
	signed int wrote = -1;
	//signed char chunkedtermination[6] = { 0x20, '0', 0x20, 0x0D, 0x0A, '\0' };
	//signed int  chunkedterminationlen = 5;
	signed char chunkedtermination[4]   = { '0', 0x0D, 0x0A, '\0' }; // Chrome, 2.8.2016
	signed int  chunkedterminationlen   = 3;
	//signed char octettermination[3]   = { 0x0D, 0x0A, '\0' };
	//signed int  octetterminationlen   = 2;
	if( cbf==NULL || *cbf==NULL ){	cb_clog( CBLOGERR, CBERRALLOC, "\ncb_terminate_transfer: parameter was null." ); return CBERRALLOC; }
	if( (**cbf).fd<0 ){ cb_clog( CBLOGERR, CBERRFILEOP, "\ncb_transfer_write: fd was %i, error.", (**cbf).fd ); return -1; }
        switch( (**cbf).transferextension ){
                case CBNOEXTENSIONS:
                        break;
                default:
                        cb_clog( CBLOGERR, CBERROR, "\ncb_terminate_transfer: invalid transfer coding extension, error %i.", CBERROR );
                        break;
        }
        switch( (**cbf).transferencoding ){
		case CBTRANSENCOCTETS:
			// nothing wrote = write( (**cbf).fd, &octettermination[0], (size_t) octetterminationlen );
			break;
                case CBTRANSENCCHUNKS:
			wrote = (signed int) write( (**cbf).fd, &chunkedtermination[0], (size_t) chunkedterminationlen );
			// cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_flush_chunks: WROTE TERMINATING CHUNK [" );
                        // write( 2, &chunkedtermination[0], (size_t) chunkedterminationlen ); // TMP DEBUG
			// cb_clog( CBLOGDEBUG, CBNEGATION, "] length %i.", chunkedterminationlen );
                        break;
                default:
                        wrote = -1;
                        cb_clog( CBLOGERR, CBERROR, "\ncb_terminate_transfer: invalid transfer encoding, error %i.", wrote );
                        break;
        }
        return wrote;

}
/*
 * Missingbytes are bytes left from previous read and to next read if block size was smaller
 * than the chunk. */
signed int  cb_read_chunk( signed long int *missingbytes, signed int fd, unsigned char *buf, signed long int buflen ){ // 28.5.2016
        signed int rd = 0, wasread = 1, indx=0, numindx=0;
	signed long int chunksize       = 0;
	unsigned char numbuf[14] = { 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, '\0' }; // text max length 2147483647, 10 characters + 0x + H, 13
	signed int  numbuflen           = 13;
	unsigned char extensionbuf[513];
	signed int  extensionbuflen     = 512;
	const char                 *ptr = NULL;
	unsigned char ch = 0x20;
	signed char stop=0;
	extensionbuf[ extensionbuflen ] = '\0';
	if( buf==NULL || missingbytes==NULL){ cb_clog( CBLOGERR, CBERRALLOC, "\ncb_read_chunk: parameter was null." ); return -1; }
	if( fd<0 ){ cb_clog( CBLOGERR, CBERRFILEOP, "\ncb_read_chunk: fd was %i, error.", fd ); return -1; }
	if( buflen<=0 ) return 0;
	if( *missingbytes>0 ){
		/* Read rest of the bytes from the previous read. */
		if(*missingbytes<buflen)
			buflen = *missingbytes;
		wasread = (signed int) read( fd, &(*buf), (size_t) buflen );
		if(wasread>0) rd += wasread;
	}else{
		/* Remove folding characters. */
		while( ( WSP( ch ) || LF( ch ) || CR( ch ) ) && wasread>0 )
			wasread = (signed int) read( fd, &ch, (size_t) 1 );
		/* Read length of the next chunk. */
		for( indx=0; indx<numbuflen && wasread>0 && ! ( WSP( ch ) || LF( ch ) || CR( ch ) ) ;++indx ){
			wasread = (signed int) read( fd, &ch, (size_t) 1 );
			if( wasread>0 && ch>0x2F && ch<0x3A){ // [0-9]
				numbuf[ numindx ] = ch;
				++numindx;
			}
		}
		/* Read extension until CRLF removing WSP:s and ';' -characters (only chunk-ext-name=chunk-ext-val is saved) */
		indx=0;
		while( stop < 2 && wasread>0 ){
			wasread = (signed int) read( fd, &ch, (size_t) 1 );
			if( WSP( ch ) || ch==0x3B ){ continue; }
			if( CR( ch ) && stop==0 ){ ++stop; continue; }
			if( LF( ch ) && stop==1 ){ ++stop; continue; }
			if( stop<2 ) stop=0;
			extensionbuf[ indx ] = ch;
			++indx;
		}
		ptr = &( (const char*) extensionbuf)[0];
		chunksize = strtol( &(*ptr), NULL, 16 );
		if( chunksize<0 ){ cb_clog( CBLOGERR, CBNEGATION, "\ncb_read_chunk: strtol, error %li.", chunksize ); return  (signed int) chunksize; }
		/* Read the next chunk. */
		wasread = (signed int) read( fd, &(*buf), (size_t) chunksize );;
		if( wasread>0 ) rd += wasread;
		if( wasread<0 ){ cb_clog( CBLOGERR, CBNEGATION, "\ncb_read_chunk: read, error %i.", wasread ); return rd; }
		/* Update the missing bytes information. */
		if( rd<chunksize )
			*missingbytes = chunksize - rd;
		else
			*missingbytes = 0;
	}
        return rd;
}

signed int  cb_flush_chunks( signed int fd, unsigned char *buf, signed long int contentlen ){ // 27.5.2016
        signed int chunksize = 0;
        signed int written   = 1, wrote = 0;
        char crlfbuf[23]     = { 0x0D, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, '\0' };
        signed int   crlfbuflen = 22;
        signed int          len = 1;
        char               *ptr = NULL;
	unsigned char   *bufptr = NULL;
	signed long int  bufpos = 0;
        ptr = &crlfbuf[0];
        if( buf==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_flush_chunks: buf was null." ); return CBERRALLOC; }
        bufptr = &buf[ bufpos ];
        while( bufpos < (signed int) contentlen && written>0 && bufpos>=0 ){
                if( (contentlen-bufpos) < CBTRANSENCODINGCHUNKSIZE )
                        chunksize = (signed int) ( ( (signed int) contentlen )-bufpos);
                else
                        chunksize = CBTRANSENCODINGCHUNKSIZE;
		if(chunksize<=0) break;
                /* Print length number in hexadecimal notation. */
                len = snprintf( &(*ptr), (size_t) crlfbuflen, "%x%c%c", chunksize, 0x0D, 0x0A );
                wrote = (signed int) write( fd, &(*ptr), (size_t) len );
		// cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_flush_chunks: WROTE LENGTH %i [", wrote );
                // write( 2, &(*ptr), (size_t) len ); // TMP DEBUG
		// cb_clog( CBLOGDEBUG, CBNEGATION, "]" );
                if(len>0 && wrote>=0 ){
                        written += wrote;
                        /* Print chunk. */
                        wrote = (signed int) write( fd, &(*bufptr), (size_t) chunksize );
			// cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_flush_chunks: WROTE CHUNK SIZE %i [", chunksize );
                        // write( 2, &(*bufptr), (size_t) chunksize ); // TMP DEBUG
			// cb_clog( CBLOGDEBUG, CBNEGATION, "]" );
                        if(wrote>0){
				written += wrote;
				bufpos += wrote;
				if( bufpos<contentlen )
					bufptr = &buf[ bufpos ];
				// cb_clog( CBLOGDEBUG, CBNEGATION, ", BUFPOS %li.", bufpos );
			}
                }
                /* Print terminating <cr><lf>. */
                len = snprintf( &(*ptr), (size_t) crlfbuflen, "%c%c", 0x0D, 0x0A );
                wrote = (signed int) write( fd, &(*ptr), (size_t) len );
		// cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_flush_chunks: WROTE LENGTH %i [", wrote );
                // write( 2, &(*ptr), (size_t) wrote ); // TMP DEBUG
		// cb_clog( CBLOGDEBUG, CBNEGATION, "]" );
		if(wrote>0) written += wrote;
        }
        return written;
}


