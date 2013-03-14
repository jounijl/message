/*
 * Program to test the library.
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
#include <errno.h>  // int errno

#define NOTFOUND 1
#define ERROR 2

#include "../include/cb_buffer.h"
// #include "../get_option/get_option.h"

int  main (int argc, char *argv[]);
void usage ( char *progname[]);

int main (int argc, char *argv[]) {

	// if(err==CBSTREAM){ err = cb_remove_name_from_stream(str); } // indicate out of buffer for next searchs

        // Find file position for data fields with variable name
        //if( buf!=NULL && contentlen!=0 ){
        //  err = cb_set_cursor( result, &buf, &contentlen );
        //  if(err<=CBERROR && err>=CBNEGATION){ // CBOUTOFBUFFER tai CBSTREAM eika CBSTREAMEND sisally
        //  }
        //  if(err==CBSTREAMEND || err>CBERROR){
        //    return NOTFOUND;
        //  }
        //  if(err==CBNOTFOUND){
        //    return NOTFOUND;
        //  }
        //}else{
        //  return ERROR;
        //}
	return CBSUCCESS;
}
