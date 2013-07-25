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
#include <string.h> // memset
#include <stdlib.h> // strtol
#include <limits.h> // strtol
#include <fcntl.h>  // open
#include <sys/types.h> // write
#include <sys/uio.h>   // write
#include <unistd.h>    // write
#include <sys/stat.h>  // mode_t, chmod at open


#define NOTFOUND 	1000
#define ERROR 		2000

#define BUFSIZE 	16384
#define BLKSIZE 	1024
#define FILENAMELEN 	100

#define ENCODINGS 2

#include "../include/cb_buffer.h"

int  main (int argc, char *argv[]);
void usage ( char *progname[]);

/*
 * Program to test the library ("cb").
 *
 * progname <encoding> <blocksize> <buffersize> <filename> [ <filename> ... ]
 *
 * Arguments are: input and outputs block size, buffer size and 
 * a list of filenames without it's postfix.
 *
 * Reads name-value pairs from input and writes them to output
 * programmatically, changes encoding and repeats to every file
 * in list.
 * - First run on one file:
 *   Reads names with nonexistent name
 * - Prints a list of names and offsets to stderr
 * - Second run on one file: finds all the names in the same order 
 *   from buffer and prints the name and value to output.
 * - Prints any occurring errors and information to stderr
 * - CBFILE has to be emptied and renewed before changing encoding
 *
 */

int main (int argc, char *argv[]) {
	unsigned char *nonexname;
	int nonexnamelen = 7;
	CBFILE *in = NULL;
	CBFILE *out = NULL;
	cbuf *name_list = NULL;
	cbuf *name_list_ptr = NULL;
	cb_name *nameptr = NULL;
	cb_name *nameptrtmp = NULL;
	int indx=0, indx2=0, encoding=0, encbytes=0, strdbytes=0, bufsize=BUFSIZE, blksize=BLKSIZE;
	int fromend=0,	encodingstested=0, atoms=0;
	unsigned char *filename = NULL;
	char infile[FILENAMELEN+5];
	char outfile[FILENAMELEN+6];
	int filenamelen = 0, err = 0;
	ssize_t ret = (ssize_t) 0;
	unsigned long int chr = 0;
	unsigned long int prevchr = 0;
	char *str_err = NULL;

	// Allocate new name list
	// Allocation 
        err = cb_allocate_cbfile(&in, 0, bufsize, blksize); 
        if(err!=CBSUCCESS){ fprintf(stderr,"\ttest: error at cb_allocate_cbfile: %i.", err); return CBERRALLOC;}
        err = cb_allocate_cbfile(&out, 1, bufsize, blksize); 
        if(err!=CBSUCCESS){ fprintf(stderr,"\ttest: error at cb_allocate_cbfile: %i.", err); return CBERRALLOC;}
	err = cb_allocate_buffer(&name_list, bufsize);
	if(err!=CBSUCCESS){ fprintf(stderr,"\ttest: error at cb_allocate_buffer, namelist: %i.", err); return CBERRALLOC;}
	// Filenames
	memset(&(*infile), ' ', (size_t) 100); infile[FILENAMELEN+5]='\0';
	memset(&(*outfile), ' ', (size_t) 104); outfile[FILENAMELEN+6]='\0';

	// Unknown name
	nonexname = (unsigned char *) malloc( 8*sizeof(unsigned char) );
	if(nonexname==NULL){ fprintf(stderr,"\ttest: nonexname malloc error, errno %i.", errno); return CBERRALLOC; }
	nonexname[7]='\0'; 
	snprintf( (char *) nonexname, (size_t) 8, "unknown" );

	// Encoding number
	if(argc>=2 && argv[1]!=NULL){
	  encoding = (int) strtol(argv[1],&str_err,10);
	}
	// Block and buffer size
	if(argc>=3 && argv[2]!=NULL){
	  blksize = (int) strtol(argv[2],&str_err,10);
	}
	if(argc>=4 && argv[3]!=NULL){
	  bufsize = (int) strtol(argv[3],&str_err,10);
	}else{
 	  fprintf(stderr,"\ttest: not enough parameters: %i.", argc);
	  if(argv[0]!=NULL)
	    usage(&argv[0]);
          return ERROR;
	}

        // Special case: last parameter was null (FreeBSD)
        atoms=argc;
        if(argv[(atoms-1)]==NULL && argc>0)
          --atoms;

	//
	// Every filename
	for(indx=4; indx<atoms; ++indx){
		// Filenames
		if(argv[indx]==NULL){
	          fprintf(stderr,"\ttest: Filename was null, argc %i, indx %i.\n", argc, indx); 
		  return ERROR;
		}
		filename = &( * (unsigned char*) argv[indx]);
		filenamelen = (int) strlen(argv[indx]);
		if(filenamelen>FILENAMELEN+6){  fprintf(stderr,"\ttest: Filename was too long."); return ERROR;	}
		for(indx2=0; indx2<filenamelen; ++indx2){
	           infile[indx2] = filename[indx2];
	           outfile[indx2] = filename[indx2];
		}
	        if( infile[indx2]!='\0' && infile[indx2]!=' ' && infile[indx2]!='\t' && infile[indx2]!='\n' ){ ++indx2; }
		infile[indx2]='\0'; outfile[indx2]='.'; outfile[indx2+1]='0'; outfile[indx2+2]='.'; 
	        outfile[indx2+3]='o'; outfile[indx2+4]='u'; outfile[indx2+5]='t'; outfile[indx2+6]='\0';
	        fprintf(stderr,"\ninfile: [%s] encoding: %i.", infile, encoding);

		//
		// Every encoding to output (with file prefixes)
		cb_set_encoding(&in, encoding);
		while(encodingstested<ENCODINGS){ // 0 one byte, 1 utf-8, 2 not ready (encodingbytes not set from set_encoding)
			// Set encoding
			cb_set_encoding(&out, encoding);

			fprintf(stderr,"\n\t Encoding %i", encoding);
			fprintf(stderr,"\n\t -----------\n");

			// Open files
			ret = 0;
                        outfile[indx2+1]=(char) ( 0x30 + encoding ); 
			if(infile[0]=='-'){
			  (*in).fd = 0; // stdin
			  outfile[0]='0';
			}else{
			  (*in).fd  = open( &(*infile), ( O_RDONLY ) ); 
			}
			(*out).fd = open( &(*outfile), ( O_RDWR | O_CREAT ), (mode_t)( S_IRWXU | S_IRWXG ) );
			if((*in).fd<=-1 || (*out).fd<=-1 ){
			  fprintf(stderr,"\ttest: open failed, fd in %i out %i.\n", (*in).fd, (*out).fd);
			  fprintf(stderr,"\ttest: open failed, infile %s outfile %s.\n", infile, outfile);
        		  return ERROR;
		        }

			name_list_ptr = &(*(*in).cb);
			(*in).cb = &(*name_list);

			//
			// First run: Reading list of names
		        if( in!=NULL && nonexname!=NULL && nonexnamelen!=0 ){
			   err = cb_set_cursor( &in, &nonexname , &nonexnamelen );
		           if(err==CBSTREAM){ 
			      err = cb_remove_name_from_stream(&in); // Is this necessary, get_chr is not used
			      if(err!=CBSUCCESS){ fprintf(stderr,"\ttest: error in cb_remove_name_from_stream, %i.\n", err ); }
			   }
		           if(err==CBNOTFOUND){ fprintf(stderr,"\ttest: cb_set_cursor, CBNOTFOUND, %i.\n", err ); }
			   if(err!=CBNOTFOUND && ( err<=CBERROR && err>=CBNEGATION ) ){ // CBOUTOFBUFFER tai CBSTREAM eika CBSTREAMEND sisally
			      fprintf(stderr,"\ttest: cb_set_cursor, %i.\n", err ); }
			   if(err==CBSTREAMEND || err>CBERROR){
			      fprintf(stderr,"\ttest: cb_set_cursor, STREAMEND or CBERROR, %i.\n", err ); }
        		}else{
			   fprintf(stderr,"\ttest: Error, in was null or nonexnamelen was 0.\n"); 
        		}
			fprintf(stderr,"\nFirst run, names are:\n"); 
			err = cb_print_names(&in);
			if(err!=CBSUCCESS){ fprintf(stderr,"\ttest: cb_print_names returned %i.", err ); }
			(*in).cb = &(*name_list_ptr); 

			//
			// Second run: finds all names in order and prints them to output (comments are lost)
			// After buffer, the rest of the stream. (One test: Unknown names literal name in it.) 
			cb_reinit_cbfile(&in); 
			err = lseek( (*in).fd, (off_t) 0 , (int) SEEK_SET); // off_t on tyyppia long long, 64 bit
			if(err<0){fprintf(stderr,"\ttest: fseek returned %i, errno set is %i.", err, errno ); }
			fprintf(stderr,"\nSecond run, names are backwards:\n"); 
		        if( in!=NULL ){
			   nameptr = &(* (cb_name *) (*name_list).name );
			   fromend=0;
			   while( nameptr != NULL && fromend<(*name_list).namecount ){
				// Next name from end
				if(fromend<(*name_list).namecount && nameptr!=NULL){
			           nameptr = &(* (cb_name *) (*name_list).name );
			           for(err=fromend; err<((*name_list).namecount-1) && fromend<(*name_list).namecount && nameptr!=NULL ;++err){
				      //fprintf(stderr," [ %i | %i | %i ]", err, fromend, (*name_list).namecount);
			              nameptrtmp = &(* (cb_name *) (*nameptr).next ); // count-1 pointers and a null pointer
			              if(nameptrtmp!=NULL)
				        nameptr = &(* nameptrtmp);
				   }
				}
// 24.5.2013, jäi kohtaan: nimi ei tulostu, aiempi segfaultkorjaus (pointerien takia) voi olla huonosti tehty
if(nameptr==NULL){
  fprintf(stderr," nameptr was NULL ");
}
				fromend++;
				fprintf(stderr," [%i/%i] setting cursor to name [", ((*name_list).namecount-fromend+1), (*name_list).namecount );
				err = cb_print_ucs_chrbuf( &(*nameptr).namebuf, (*nameptr).namelen, (*nameptr).buflen );
				if(err>=CBERROR){ fprintf(stderr,"\ttest: cb_print_ucs_chrbuf, namebuf, err %i.", err); }
				fprintf(stderr,"], length %i.\n", (*nameptr).namelen);
			        err = cb_set_cursor( &in, &(*nameptr).namebuf, &(*nameptr).namelen );
				if(err==CBNOTFOUND){
			           fprintf(stderr,"\ttest: cb_set_cursor, CBNOTFOUND, %i.\n", err ); }
				if(err!=CBNOTFOUND && ( err<=CBERROR && err>=CBNEGATION ) ){ // CBOUTOFBUFFER tai CBSTREAM eika CBSTREAMEND sisally
			           fprintf(stderr,"\ttest: cb_set_cursor, %i.\n", err ); }
				if(err==CBSTREAMEND || err>CBERROR){
			           fprintf(stderr,"\ttest: cb_set_cursor, STREAMEND or CBERROR, %i.\n", err );
				}else if(err==CBSUCCESS || err==CBSTREAM){
				   if(err==CBSTREAM){ 
				      err = cb_remove_name_from_stream(&in); // Is this necessary, get_chr is not used
			              if(err!=CBSUCCESS){ fprintf(stderr,"\ttest: error in cb_remove_name_from_stream, %i.\n", err ); }
				   }
				   //
				   // Write everything to output
				   //
				   // Name:
				   if( nameptr!=NULL && (*nameptr).namebuf!=NULL )
                                     ret = write( (*out).fd, &( (*nameptr).namebuf ), (size_t) (*nameptr).namelen ); 
                                   if(ret<0){ fprintf(stderr,"\ttest: write error %i, errno %i.", (int)ret, errno); }
				   // Value:
				   // From '=' and after it to the last nonbypassed '&' and it.
	                           err = cb_get_chr(&in, &chr, &encbytes, &strdbytes );
				   fprintf(stderr," (*out).encoding=%d (*out).encodingbytes=%d content: [", (*out).encoding, (*out).encodingbytes );
				   while( ! ( (*out).bypass != prevchr && (*out).rend == chr ) && err<CBNEGATION){
				      if(err==CBNOTUTF){
					fprintf(stderr,"\ttest: read something not in UTF format, CBNOTUTF.\n");
				      }else if(err<CBERROR){
				        err = cb_put_chr(&out, &chr, &encbytes, &strdbytes ); 
				      }
				      if( err!=CBSUCCESS && err!=CBSTREAM ){
				        fprintf(stderr,"\ttest: encbytes=%d strdbytes=%d cb_get_chr cb_put_chr err=%i.\n", encbytes, strdbytes, err ); }
				      fprintf(stderr,"%C", (unsigned int) chr );
				      prevchr = chr;
		                      err = cb_get_chr(&in, &chr, &encbytes, &strdbytes );
				   } // while
				   fprintf(stderr,"]\n" );
				} // else if
                           } // while
        		}else{ // if in not null
			   fprintf(stderr,"\ttest: Error, in was null.\n"); 
        		}

			//
			// Return
			cb_reinit_cbfile(&in); 
			cb_reinit_cbfile(&out); 			
			cb_reinit_buffer(&name_list); // free names and init
		        if(encoding>=(ENCODINGS-1))
	                  encoding=0;
	                else
	                  ++encoding;
	                ++encodingstested;

			err = close( (*out).fd ); if(err!=0){ fprintf(stderr,"\ttest: close out failed."); }
		} // while (encodings)
		// Close files
		if(infile[0]=='-'){
		  err = close( (*in).fd ); if(err!=0){ fprintf(stderr,"\ttest: close in failed."); }
		}
	} // for (files)
	return CBSUCCESS;
}

void usage (char *progname[]){
        printf("\nUsage:\n");
        printf("\t%s <encoding> <blocksize> <buffersize> <filename> [<filename> [<filename> ...] ]\n", progname[0]);
        printf("\tProgram to test reading and writing. Reads filename in encoding <encoding>\n");
        printf("\tand outputs its found valuenames and values to \n");
        printf("\tfilename.<encodingnumber>.out with every encoding. Stdin to EOF is '-' .\n");
}
