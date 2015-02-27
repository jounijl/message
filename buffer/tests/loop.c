/*
 * Program to send file to stdout endlessly. 
 * 
 * Copyright (c) 2009, 2010 and 2013, Jouni Laakso
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 * Neither the name of the copyright owners nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <errno.h>     // int errno
#include <fcntl.h>     // open
#include <unistd.h>    // lseek
#include <sys/stat.h>  // mode_t, chmod at open
#include <stdlib.h>    // exit

#define ERROR 		2000

#define BUFSIZE 	0
#define BLKSIZE 	1024

#include "../../include/cb_buffer.h"

int  main (int argc, char *argv[]);
void usage ( char *progname[]);

/*
 * progname <filename>
 *
 * Outputs file. At end, seeks to start and repeats endlessly.
 */

int main (int argc, char *argv[]) {
	CBFILE *in = NULL;
	CBFILE *out = NULL;
        int fd=0;
	unsigned char chr = 0;
        int err1=CBSUCCESS, err2=CBSUCCESS;

        if(argv[1]==NULL){
          if(argv[0]!=NULL){
            usage(&argv[0]);
            return ERROR;
          }
        }
	err1 = argc; err1=CBSUCCESS;

        // Open file
        fd  = open( &(*argv[1]), ( O_RDONLY ) );
        if(fd<=-1){
           fprintf(stderr,"\tloop: open failed, fd was %i, file %s.\n", fd, argv[1]);
           usage(&argv[0]);
           return ERROR;
        }

        // Allocate input buffer
        err1 = cb_allocate_cbfile(&in, fd, BUFSIZE, BLKSIZE); 
        if(err1!=CBSUCCESS){ fprintf(stderr,"\tloop: error at cb_allocate_cbfile: in, err=%i.", err1); return CBERRALLOC;}
        cb_set_encoding(&in, CBENC1BYTE); // one byte encoding

        // Allocate output buffer
        err1 = cb_allocate_cbfile(&out, 1, BUFSIZE, BLKSIZE); 
        if(err1!=CBSUCCESS){ fprintf(stderr,"\tloop: error at cb_allocate_cbfile: out, err=%i.", err1); return CBERRALLOC;}
        cb_set_encoding(&out, CBENC1BYTE); // one byte encoding

        //
        // Read and write
        while(err1<CBNEGATION && err2<CBNEGATION){
          err1 = cb_get_ch(&in, &chr);
          if(err1<CBNEGATION)
            err2 = cb_put_ch(&out, chr);
          if(err1==CBSTREAMEND){
            err2 = cb_reinit_cbfile(&in); // set cursor and contentlength to start
            if( lseek( (*in).fd, (off_t)0, SEEK_SET) == -1 ){ // seek to start of file
              fprintf(stderr,"\tloop: lseek error trying to set to start.");
              cb_flush(&out);
              exit(ERROR);
            } err1=CBSUCCESS; err2=err1;
          }
        }
        cb_flush(&out);

        cb_free_cbfile(&out);
        cb_free_cbfile(&in);
        err1 = close( fd ); if(err1!=0){ fprintf(stderr,"\ttest: close file failed."); }

	return CBSUCCESS;
}

void usage (char *progname[]){
        printf("\nUsage:\n");
        printf("\t%s <filename>]\n", progname[0]);
        printf("\tLoops file content to output.\n");
}
