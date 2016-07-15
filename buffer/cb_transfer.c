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

int  cb_flush_chunks( int fd, unsigned char *buf, signed long int contentlen );
int  cb_read_chunk( signed long int *missingchunkbytes, int fd, unsigned char *buf, signed long int buflen );

/*
 * Return values like from 'read'. */
int  cb_transfer_read( CBFILE **cbf, signed long int readlength ){ // Not tested at all (not ready yet), 15.7.2016
	int read=-1;
	if( cbf==NULL || *cbf==NULL || (**cbf).blk==NULL || (*(**cbf).blk).buf==NULL) return CBERRALLOC;
	if( (**cbf).fd<0 ) return CBERRFILEOP;
	if( readlength>(*(**cbf).blk).buflen )
		readlength = (*(**cbf).blk).buflen;
	switch( (**cbf).transferextension ){
		case CBNOEXTENSIONS:
			break;
		default:
			cb_clog( CBLOGERR, CBERROR, "\ncb_transfer_write: invalid transfer coding extension, error %i.", CBERROR );
			break;
	}
	switch( (**cbf).transferencoding ){
		case CBTRANSENCCHUNKS:
			read = cb_read_chunk( &(*(**cbf).blk).chunkmissingbytes, (**cbf).fd, &(*(*(**cbf).blk).buf), readlength );			  
			break;
		default:
			read = -1;
			cb_clog( CBLOGERR, CBERROR, "\ncb_transfer_read: invalid transfer encoding, error %i.", CBERROR );
			break;
	}
	return read;
}
/*
 * Return values like from 'read'. */
int  cb_transfer_write( CBFILE **cbf ){ // Not yet well tested, 15.7.2017
	int wrote = -1;
	if( cbf==NULL || *cbf==NULL || (**cbf).blk==NULL || (*(**cbf).blk).buf==NULL || (**cbf).cb==NULL ){ 
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
		case CBTRANSENCCHUNKS:
			wrote = cb_flush_chunks( (**cbf).fd, &(*(*(**cbf).blk).buf), (*(**cbf).blk).contentlen );
			break;
		default:
			wrote = -1;
			cb_clog( CBLOGERR, CBERROR, "\ncb_transfer_write: invalid transfer encoding, error %i.", wrote );
			break;
	}
	return wrote;
}
int  cb_terminate_transfer( CBFILE **cbf ){
	int wrote = -1;
	static char chunkedtermination[6] = { 0x20, '0', 0x20, 0x0D, 0x0A, '\0' };
	static int  chunkedterminationlen = 5;
	//static char octettermination[3]   = { 0x0D, 0x0A, '\0' };
	//static int  octetterminationlen   = 2;
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
			wrote = write( (**cbf).fd, &chunkedtermination[0], (size_t) chunkedterminationlen );
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
int  cb_read_chunk( signed long int *missingbytes, int fd, unsigned char *buf, signed long int buflen ){ // 28.5.2016
        int rd = 0, wasread = 1, indx=0, numindx=0;
	signed long int chunksize       = 0;
	static unsigned char numbuf[14] = { 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, '\0' }; // text max length 2147483647, 10 characters + 0x + H, 13
	static int  numbuflen           = 13;
	static unsigned char extensionbuf[513];
	static int  extensionbuflen     = 512;
	const char                 *ptr = NULL;
	unsigned char ch = 0x20;
	char stop=0;
	extensionbuf[ extensionbuflen ] = '\0';
	if( buf==NULL || missingbytes==NULL){ cb_clog( CBLOGERR, CBERRALLOC, "\ncb_read_chunk: parameter was null." ); return -1; }
	if( fd<0 ){ cb_clog( CBLOGERR, CBERRFILEOP, "\ncb_read_chunk: fd was %i, error.", fd ); return -1; }
	if( buflen<=0 ) return 0;
	if( *missingbytes>0 ){
		/* Read rest of the bytes from the previous read. */
		if(*missingbytes<buflen)
			buflen = *missingbytes;
		wasread = read( fd, &(*buf), (size_t) buflen );
		if(wasread>0) rd += wasread;
	}else{
		/* Remove folding characters. */
		while( ( WSP( ch ) || LF( ch ) || CR( ch ) ) && wasread>0 )
			wasread = read( fd, &ch, (size_t) 1 );
		/* Read length of the next chunk. */
		for( indx=0; indx<numbuflen && wasread>0 && ! ( WSP( ch ) || LF( ch ) || CR( ch ) ) ;++indx ){
			wasread = read( fd, &ch, (size_t) 1 );
			if( wasread>0 && ch>0x2F && ch<0x3A){ // [0-9]
				numbuf[ numindx ] = ch;
				++numindx;
			}
		}
		/* Read extension until CRLF removing WSP:s and ';' -characters (only chunk-ext-name=chunk-ext-val is saved) */
		indx=0;
		while( stop < 2 && wasread>0 ){
			wasread = read( fd, &ch, (size_t) 1 );
			if( WSP( ch ) || ch==0x3B ){ continue; }
			if( CR( ch ) && stop==0 ){ ++stop; continue; }
			if( LF( ch ) && stop==1 ){ ++stop; continue; }
			if( stop<2 ) stop=0;
			extensionbuf[ indx ] = ch;
			++indx;
		}
		ptr = &( (const char*) extensionbuf)[0];
		chunksize = strtol( &(*ptr), NULL, 16 );
		if( chunksize<0 ){ cb_clog( CBLOGERR, CBNEGATION, "\ncb_read_chunk: strtol, error %li.", chunksize ); return chunksize; }
		/* Read the next chunk. */
		wasread = read( fd, &(*buf), (size_t) chunksize );;
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

int  cb_flush_chunks( int fd, unsigned char *buf, signed long int contentlen ){ // 27.5.2016
        int chunksize = 0;
        int written   = 1, wrote = 0;
        static char crlfbuf[15] = { 0x0D, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, '\0' };
        int          crlfbuflen = 14;
        int                 len = 1;
        char               *ptr = NULL;  
        ptr = &crlfbuf[0];
        if( buf==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_flush_chunks: buf was null." ); return CBERRALLOC; }
        while( written < (int) contentlen && written>0 ){
                if( contentlen < CBTRANSENCODINGCHUNKSIZE )
                        chunksize = contentlen;
                else
                        chunksize = CBTRANSENCODINGCHUNKSIZE;
		if(chunksize<=0) break;        
                /* Print length number in hexadecimal notation. */
                len = snprintf( &(*ptr), (size_t) crlfbuflen, " %x %c%c", chunksize, 0x0D, 0x0A );
                wrote = (int) write( fd, &(*ptr), (size_t) len );
                if(len>0 && wrote>=0 ){
                        written += wrote;
                        /* Print chunk. */
                        wrote = (int) write( fd, &(*buf), (size_t) chunksize );
                        if(wrote>0) written += wrote;
                }
                /* Print terminating <cr><lf>. */
                len = snprintf( &(*ptr), (size_t) crlfbuflen, "%c%c", 0x0D, 0x0A );
                wrote = (int) write( fd, &(*ptr), (size_t) len );
		if(wrote>0) written += wrote;
        }
        return written;
}


