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

#define ENCODINGS 9

#undef WINDOWSOS

#include "../include/cb_buffer.h"

int  main (int argc, char *argv[]);
void usage ( char *progname[]);

/*
 *
 * Program to test the library ("cb").
 *
 * progname <encoding> <blocksize> <buffersize> <filename> [ <filename> [...] ]
 *
 * Arguments are: input and outputs block size, buffer size and 
 * a list of filenames. Encoding is input-files encoding. 
 * Program outputs ENCODINGS amount encodings starting from 0.
 *
 * Reads name-value pairs from input and writes them to output
 * programmatically, changes encoding and repeats to every file
 * in list.
 * - First run on one file:
 *   Reads names with nonexisting name
 * - Prints a list of names and offsets to stderr
 * - Second run on one file: finds all the names in reverse order 
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
	int fromend=0, encodingstested=0, atoms=0, outindx=0, outbcount=0, outstoredsize=0;
        char inputstream=0;
	static unsigned long int nl = 0x0A; // lf
#ifdef WINDOWSOS
	static unsigned long int cr = 0x0D; // cr
#endif
	unsigned char *filename = NULL;
	char infile[FILENAMELEN+5];
	char outfile[FILENAMELEN+6];
	int filenamelen = 0, err = 0;
	ssize_t ret = (ssize_t) 0;
	unsigned long int chr = 0, chrout = 0;
	unsigned long int prevchr = 0;
	char *str_err = NULL;
	int openpairs=0; // cbstatetopology


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

	// Allocation 
        err = cb_allocate_cbfile(&in, 0, bufsize, blksize); 
        if(err!=CBSUCCESS){ fprintf(stderr,"\ttest: error at cb_allocate_cbfile: %i.", err); return CBERRALLOC;}
        err = cb_allocate_cbfile(&out, 1, bufsize, blksize); 
        if(err!=CBSUCCESS){ fprintf(stderr,"\ttest: error at cb_allocate_cbfile: %i.", err); return CBERRALLOC;}
	// Allocate name list to save names to test with them
	err = cb_allocate_buffer(&name_list, bufsize);
	if(err!=CBSUCCESS){ fprintf(stderr,"\ttest: error at cb_allocate_buffer, namelist: %i.", err); return CBERRALLOC;}
	// Filenames
	memset(&(*infile), ' ', (size_t) 100); infile[FILENAMELEN+5]='\0';
	memset(&(*outfile), ' ', (size_t) 104); outfile[FILENAMELEN+6]='\0';

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
		// Input encoding
		cb_set_encoding(&in, encoding);

		//
		// Input files
		if(infile[0]=='-'){
		  (*in).fd = 0;   // stdin
		}else{
		  (*in).fd  = open( &(*infile), ( O_RDONLY ) ); 
		}

		//
		// Name list pointer to remember the names 
		name_list_ptr = &(*(*in).cb);
		(*in).cb = &(*name_list);

		//
		// First run: Reading list of names
	        if( in!=NULL && nonexname!=NULL && nonexnamelen!=0 ){
		   err = cb_set_cursor( &in, &nonexname , &nonexnamelen );
	           if(err==CBSTREAM){ 
		      //err = cb_remove_name_from_stream(&in); // This is not necessary, get_chr is not used
		      fprintf(stderr,"\ttest_cb: CBSTREAM err %i.\n", err ); 
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

		// name_list is the list of names, name_list_ptr frees with cbfile

		//
		// Output files in every encoding
		while(encodingstested < ENCODINGS){ // && inputstream==0){ 

	  	        cb_reinit_cbfile(&in); // free unused name_list_ptr and zero buffer counters

			//
 			// Output encoding
			cb_set_encoding(&out, encoding);

			fprintf(stderr,"\n\n\t Encoding %i", encoding);
			fprintf(stderr,"\n\t -----------\n");

			//
			// Open files (output with prefixes)
			ret = 0;
                        outfile[indx2+1]=(char) ( 0x30 + encoding ); 
			if(infile[0]=='-'){
			  (*in).fd = 0;   // stdin
			  outfile[0]='0'; // name of outputfile if input file is stdin
                          inputstream=1;  // to stop the loop after first loop
			}else{
			  (*in).fd  = open( &(*infile), ( O_RDONLY ) ); 
			}
                        fprintf(stderr,"\noutfile: [%s] encoding: %i.\n", outfile, encoding);
			(*out).fd = open( &(*outfile), ( O_RDWR | O_CREAT | O_TRUNC ), (mode_t)( S_IRWXU | S_IRWXG ) );
			if((*in).fd<=-1 || (*out).fd<=-1 ){
			  fprintf(stderr,"\ttest: open failed, fd in %i out %i.\n", (*in).fd, (*out).fd);
			  fprintf(stderr,"\ttest: open failed, infile %s outfile %s.\n", infile, outfile);
        		  return ERROR;
		        }

			//
                        // Write byte order mark
                        if((*out).encoding!=CBENCUTF8)
                          cb_write_bom(&out);

			//
			// Second run: finds all names in order and prints them to output (comments are lost)
			// After buffer, the rest of the stream. (One test: Unknown names literal name in it.) 
                        if(inputstream!=1){
			  err = lseek( (*in).fd, (off_t) 0 , (int) SEEK_SET); // off_t on tyyppia long long, 64 bit
			  if(err<0){fprintf(stderr,"\ttest: fseek returned %i, errno set is %i.", err, errno ); }
                        }
			fprintf(stderr,"\nSecond run, names are backwards:\n\n"); 
		        if( in!=NULL ){
			   nameptr = &(* (cb_name *) (*name_list).list.name );
			   fromend=0;
			   while( nameptr != NULL && fromend<(*name_list).list.namecount ){
				// Next name from end
				if(fromend<(*name_list).list.namecount && nameptr!=NULL){
			           nameptr = &(* (cb_name *) (*name_list).list.name );
			           for(err=fromend; err<((*name_list).list.namecount-1) && fromend<(*name_list).list.namecount && nameptr!=NULL ;++err){
			              nameptrtmp = &(* (cb_name *) (*nameptr).next ); // count-1 pointers and a null pointer
			              if(nameptrtmp!=NULL)
				        nameptr = &(* nameptrtmp);
				   }
				}

				if(nameptr==NULL){ // 24.5.2013, ehka turha
				  fprintf(stderr," nameptr was NULL ");
				}

				fromend++;
				fprintf(stderr," [%li/%li] setting cursor to name [", ((*name_list).list.namecount-fromend+1), (*name_list).list.namecount );
				err = cb_print_ucs_chrbuf( &(*nameptr).namebuf, (*nameptr).namelen, (*nameptr).buflen );
				if(err>=CBERROR){ fprintf(stderr,"\ttest: cb_print_ucs_chrbuf, namebuf, err %i.", err); }
				fprintf(stderr,"], length %i.\n", (*nameptr).namelen);
			        err = cb_set_cursor_ucs( &in, &(*nameptr).namebuf, &(*nameptr).namelen );
				if(err==CBNOTFOUND){
			           fprintf(stderr,"\ttest: cb_set_cursor, CBNOTFOUND, %i.\n", err ); }
				if(err!=CBNOTFOUND && ( err<=CBERROR && err>=CBNEGATION ) ){ // CBOUTOFBUFFER tai CBSTREAM eika CBSTREAMEND sisally
			           fprintf(stderr,"\ttest: cb_set_cursor, %i.\n", err ); }
				if(err==CBSTREAMEND || err>CBERROR){
			           fprintf(stderr,"\ttest: cb_set_cursor, STREAMEND or CBERROR, %i.\n", err );
				}else if(err==CBSUCCESS || err==CBSTREAM){
				   //if(err==CBSTREAM){ 
				   //   err = cb_remove_name_from_stream(&in); // This should not necessary, get_chr is not used
			           //   if(err!=CBSUCCESS){ fprintf(stderr,"\ttest: error in cb_remove_name_from_stream, %i.\n", err ); }
				   //}
				   //
				   // Write everything to output
				   //
				   // Name:
                                   err=CBSUCCESS; ret=CBSUCCESS;
				   if( nameptr!=NULL && (*nameptr).namebuf!=NULL ){
                                     while( err<CBERROR && ret==CBSUCCESS && outindx<(*nameptr).namelen){
                                       ret = cb_get_ucs_chr(&chrout, &( (*nameptr).namebuf ), &outindx, (*nameptr).namelen);
                                       outbcount = 4;
				       if( ret<CBERROR && ret!=CBBUFFULL )
                                         err = cb_put_chr(&out, chrout, &outbcount, &outstoredsize); // 12.8.2013
                                         //err = cb_put_chr(&out, &chrout, &outbcount, &outstoredsize);
                                       if( err!=CBSUCCESS ){ fprintf(stderr,"\ttest:  cb_put_chr, err %i.", err ); }
                                     }
                                   } err=CBSUCCESS; outindx=0;
                                   if(ret<0){ fprintf(stderr,"\ttest: write error %i, errno %i.", (int)ret, errno ); }

				   // '='
				   err = cb_put_chr(&out, (*out).cf.rstart, &outbcount, &outstoredsize);

                                   //
				   // Value from '=' to last nonbypassed '&' and it:
	                           err = cb_get_chr(&in, &chr, &encbytes, &strdbytes );
				   if(err==CBSTREAM){ 
				      err = cb_remove_name_from_stream(&in); 
			              if(err!=CBSUCCESS){ fprintf(stderr,"\ttest: error in cb_remove_name_from_stream, %i.\n", err ); }
				   }
				   fprintf(stderr," (*out).encoding=%d (*out).encodingbytes=%d content: [", (*out).encoding, (*out).encodingbytes );

	                           if( (*in).cf.searchstate==CBSTATETOPOLOGY && (*out).cf.rstart == chr )
				       ++openpairs;
	                           if( (*in).cf.searchstate!=CBSTATETOPOLOGY || \
	                              ( (*in).cf.searchstate==CBSTATETOPOLOGY && openpairs==0 ) )
				     while( ! ( (*out).cf.bypass != prevchr && (*out).cf.rend == chr ) && err<CBNEGATION){
				       if(err==CBNOTUTF){
				 	 fprintf(stderr,"\ttest: read something not in UTF format, CBNOTUTF.\n");
				       }else if(err<CBERROR){
				         err = cb_put_chr(&out, chr, &encbytes, &strdbytes );
				       }
				       if( err!=CBSUCCESS && err!=CBSTREAM ){
				         fprintf(stderr,"\ttest: encbytes=%d strdbytes=%d cb_get_chr cb_put_chr err=%i.\n", encbytes, strdbytes, err ); }
				       fprintf(stderr,"%C", (unsigned int) chr );
				       prevchr = chr;
		                       err = cb_get_chr(&in, &chr, &encbytes, &strdbytes );
				       if(err==CBSTREAM){ 
				         err = cb_remove_name_from_stream(&in); // Test if the names value was out of buffer
			                 if(err!=CBSUCCESS){ fprintf(stderr,"\ttest_cb: error in cb_remove_name_from_stream, %i.\n", err ); }
				       }

	                               if( (*in).cf.searchstate==CBSTATETOPOLOGY ) {
				         if( (*out).cf.bypass != prevchr && (*out).cf.rstart == chr )
				           ++openpairs;
				         if( (*out).cf.bypass != prevchr && (*out).cf.rend == chr && openpairs>0)
				           --openpairs;
	                               }

				   } // while
				   fprintf(stderr,"]\n" );
				   //
				   // '&' and new line
				   err = cb_put_chr(&out, (*out).cf.rend, &outbcount, &outstoredsize);
#ifdef WINDOWSOS
				   err = cb_put_chr(&out, cr, &outbcount, &outstoredsize); 
#endif
				   err = cb_put_chr(&out, nl, &outbcount, &outstoredsize);
                                   cb_flush(&out);
				} // else if
                           } // while
        		}else{ // if in not null
			   fprintf(stderr,"\ttest: Error, in was null.\n"); 
        		}
			cb_flush(&out);

			//
			// Return
			cb_reinit_cbfile(&out);
	                ++encoding;
	                ++encodingstested;

			err = close( (*out).fd ); if(err!=0){ fprintf(stderr,"\ttest: close out failed."); }
                
		} // while (encodings)
		cb_free_buffer(&name_list);
	} // for (files)

	// Close input file
	if(infile[0]=='-'){
	  err = close( (*in).fd ); if(err!=0){ fprintf(stderr,"\ttest: close in failed."); }
	}

        cb_free_cbfile(&in);
        cb_free_cbfile(&out);
        free(nonexname);

	return CBSUCCESS;
}

void usage (char *progname[]){
        printf("\nUsage:\n");
        printf("\t%s <encoding> <blocksize> <buffersize> <filename> [<filename> [<filename> ...] ]\n", progname[0]);
        printf("\tProgram to test reading and writing. Reads filename in encoding\n");
        printf("\t<encoding> and outputs its found valuenames and values to \n");
        printf("\tfilename.<encodingnumber>.out with every encoding. Stdin to EOF is '-' .\n");
}

