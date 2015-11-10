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
//#include <sys/uio.h>   // write
#include <unistd.h>    // write
#include <sys/stat.h>  // mode_t, chmod at open


//#define NOTFOUND 	1000
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

int tmp_gdb_test_call(int num);
int tmp_gdb_test_call(int num){
	return num;
}

int main (int argc, char *argv[]) {
	unsigned char *nonexname;
	int nonexnamelen = 7, err2=0;
	CBFILE *in = NULL;
	CBFILE *out = NULL;
//	cbuf *name_list = NULL;
	cb_namelist name_list;
//	cbuf *name_list_ptr = NULL;
	cb_namelist name_list_empty;
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
	unsigned char infile[FILENAMELEN+5+1];
	unsigned char outfile[FILENAMELEN+6+1];
	int filenamelen = 0; long long int err = 0;
	ssize_t ret = (ssize_t) 0;
	unsigned long int chr = 0, chrout = 0;
	unsigned long int prevchr = 0;
	char *str_err = NULL;
	int openpairs=0; // cbstatetopology

	// Unknown name
	nonexname = (unsigned char *) malloc( 8*sizeof(unsigned char) );
	if(nonexname==NULL){ fprintf(stderr,"\ttest: nonexname malloc error, errno %i.", errno); return CBERRALLOC; }
	nonexname[7]='\0'; 
	snprintf( (char *) nonexname, (size_t) 8, "unknown" ); // 7+'\0'

	// Encoding number
	if(argc>=2 && argv[1]!=NULL){
	  encoding = (int) strtol(argv[1],&str_err,10);
	  fprintf( stderr, "\nSetting encoding: %i.", encoding );
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
        if(err!=CBSUCCESS){ fprintf(stderr,"\ttest: error at cb_allocate_cbfile: %lli.", err); return CBERRALLOC;}
        err = cb_allocate_cbfile(&out, 1, bufsize, blksize); 
        if(err!=CBSUCCESS){ fprintf(stderr,"\ttest: error at cb_allocate_cbfile: %lli.", err); return CBERRALLOC;}
	// Allocate name list to save names to test with them
	//err = cb_allocate_buffer(&name_list, bufsize);
	name_list.name=NULL; name_list.current=NULL; name_list.last=NULL; 
	name_list.currentleaf=NULL; name_list.namecount=0;
	name_list_empty.name=NULL; name_list_empty.current=NULL; name_list_empty.last=NULL; 
	name_list_empty.currentleaf=NULL; name_list_empty.namecount=0;
	if(err!=CBSUCCESS){ fprintf(stderr,"\ttest: error at cb_allocate_buffer, namelist: %lli.", err); return CBERRALLOC;}
	// Filenames
	memset(&(*infile), ' ', (size_t) FILENAMELEN); infile[FILENAMELEN+5]='\0';
	memset(&(*outfile), ' ', (size_t) FILENAMELEN+4); outfile[FILENAMELEN+6]='\0';

	// Test offset in reading 13.12.2014:
	cb_use_as_seekable_file(&in);
	//cb_use_as_stream(&in);
	//cb_set_search_state(&in, CBSTATEFUL);
	//cb_set_search_state(&in, CBSTATELESS);
	//cb_set_search_state(&in, CBSTATETOPOLOGY);
	cb_set_search_state(&in, CBSTATETREE);

	// Test offset in appending 13.12.2014:
	cb_use_as_seekable_file(&out);
	//cb_use_as_stream(&out);
	//cb_set_search_state(&out, CBSTATEFUL);
	cb_set_search_state(&out, CBSTATETREE);

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
	        fprintf(stderr,"\ninfile: [%s] encoding: %i", infile, encoding);

		//
		// Input encoding
		cb_set_encoding(&in, encoding);

		//
		// Input files
		if(infile[0]=='-'){
		  (*in).fd = 0;   // stdin
		}else{
		  (*in).fd  = open( &(* (char*) infile), ( O_RDONLY ) ); 
		}
	        fprintf(stderr," (errno %i).", errno);

		//
		// Name list pointer to remember the names 
		name_list_empty  = (*(*in).cb).list;
		(*(*in).cb).list = name_list;

		//
		// First run: Reading list of names
	        if( in!=NULL && nonexname!=NULL && nonexnamelen!=0 ){
		   err = cb_set_cursor( &in, &nonexname , &nonexnamelen );
	           if(err==CBSTREAM){
		      //err = cb_remove_name_from_stream(&in); // This is not necessary, get_chr is not used
		      fprintf(stderr,"\ttest_cb: CBSTREAM err %lli.\n", err );
                   }
	           if(err==CBNOTFOUND){ fprintf(stderr,"\ttest: cb_set_cursor, CBNOTFOUND, %lli.\n", err ); }
		   if(err!=CBNOTFOUND && ( err<=CBERROR && err>=CBNEGATION ) ){ // CBOUTOFBUFFER tai CBSTREAM eika CBSTREAMEND sisally
		      fprintf(stderr,"\ttest: cb_set_cursor, %lli.\n", err ); }
		   if(err==CBSTREAMEND || err>CBERROR){
		      fprintf(stderr,"\ttest: cb_set_cursor, STREAMEND or CBERROR, %lli.\n", err ); }
       		}else{
		   fprintf(stderr,"\ttest: Error, in was null or nonexnamelen was 0.\n"); 
       		}
		fprintf(stderr,"\nFirst run, names are:\n"); 
		err = cb_print_names(&in, CBLOGDEBUG);
		if(err!=CBSUCCESS){ fprintf(stderr,"\ttest: cb_print_names returned %lli.", err ); }

		name_list  = (*(*in).cb).list;
		(*(*in).cb).list = name_list_empty;

		// name_list is the list of names, name_list_ptr frees with cbfile

		//
		// Output files in every encoding
		while(encodingstested < ENCODINGS){ // && inputstream==0){ 


			// tmp_gdb_test_call( encoding );

			/* 21.2.2015: cb_free_name error from encoding 2 onwards: CBSTATETREE, not: CBSTATELESS, CBSTATEFUL, CBSTATETOPOLOGY. */
	  	        err2 = cb_reinit_cbfile(&in); // free unused name_list_ptr and zero buffer counters
			if(err2>=CBNEGATION){ cb_clog( CBLOGWARNING, err2, "\ntest_cb: cb_reinit_cbfile(&in) returned %i.", err2 );  }

			//
 			// Output encoding
			cb_set_encoding(&out, encoding);

			fprintf(stderr,"\n\n\t Encoding %i", encoding);
			fprintf(stderr,"\n\t -----------\n");

			//
			// Open files (output with prefixes)
			ret = 0;
                        outfile[indx2+1]= (unsigned char) ( 0x30 + encoding ); 
			if(infile[0]=='-'){
			  (*in).fd = 0;   // stdin
			  outfile[0]='0'; // name of outputfile if input file is stdin
                          inputstream=1;  // to stop the loop after first loop
			}else{
			  (*in).fd  = open( &(* (char*) infile), ( O_RDONLY ) );
                          fprintf(stderr,"\nOpened inputfile [%s], fd %i.", infile, (*in).fd );
			}
                        fprintf(stderr,"\noutfile: [%s] encoding: %i", outfile, encoding );
			(*out).fd = open( &(* (char*) outfile), ( O_RDWR | O_CREAT | O_TRUNC ), (mode_t)( S_IRWXU | S_IRWXG ) );
			if((*in).fd<=-1 || (*out).fd<=-1 ){
			  fprintf(stderr,"\ttest: open failed, fd in %i out %i.\n", (*in).fd, (*out).fd);
			  fprintf(stderr,"\ttest: open failed, infile %s outfile %s.\n", infile, outfile);
        		  return ERROR;
		        }
			fprintf(stderr," (errno %i).\n", errno);

			//
                        // Write byte order mark
                        if((*out).encoding!=CBENCUTF8)
                          cb_write_bom(&out);

			//
			// Second run: finds all names in order and prints them to output (comments are lost)
			// After buffer, the rest of the stream. (One test: Unknown names literal name in it.) 
                        if(inputstream!=1){
			  err = lseek( (*in).fd, (off_t) 0 , (int) SEEK_SET); // off_t on tyyppia long long, 64 bit
			  if(err<0){fprintf(stderr,"\ttest: fseek returned %lli, errno set is %i.", err, errno ); }
                        }

			fprintf(stderr,"\nSecond run, names are backwards:\n\n"); 
		        if( in!=NULL ){
			   nameptr = &(* (cb_name *) name_list.name );
			   fromend=0;
			   while( nameptr != NULL && fromend < name_list.namecount ){
				// Next name from end
				if( fromend<name_list.namecount && nameptr!=NULL){
			           nameptr = &(* (cb_name *) name_list.name );
			           for(err=fromend; err<(name_list.namecount-1) && fromend<name_list.namecount && nameptr!=NULL ;++err){
			              nameptrtmp = &(* (cb_name *) (*nameptr).next ); // count-1 pointers and a null pointer
			              if(nameptrtmp!=NULL)
				        nameptr = &(* nameptrtmp);
				   }
				}

				if(nameptr==NULL){ // 24.5.2013, ehka turha
				  fprintf(stderr," nameptr was NULL ");
				}

				fromend++;
				fprintf(stderr," [%lli/%lli] setting cursor to name [", (name_list.namecount-fromend+1), name_list.namecount );
				err = cb_print_ucs_chrbuf( CBLOGDEBUG, &(*nameptr).namebuf, (*nameptr).namelen, (*nameptr).buflen );
				if(err>=CBERROR){ fprintf(stderr,"\ttest: cb_print_ucs_chrbuf, namebuf, err %lli.", err); }
				fprintf(stderr,"], length %i, [%li/", (*nameptr).namelen, (*nameptr).nameoffset);
				fprintf(stderr,"%li/%i].\n", (*nameptr).offset, (*nameptr).length);

				if(in==NULL)
					cb_clog( CBLOGDEBUG, CBNEGATION, "\n in was null." );
				if( nameptr==NULL )
					cb_clog( CBLOGDEBUG, CBNEGATION, "\n nameptr was null." );
				if( (*nameptr).namebuf==NULL )
					cb_clog( CBLOGDEBUG, CBNEGATION, "\n (*nameptr).namebuf was null." );
				if( (*in).cb==NULL)
					cb_clog( CBLOGDEBUG, CBNEGATION, "\n (*in).cb was null." );
				if( (*in).blk==NULL)
					cb_clog( CBLOGDEBUG, CBNEGATION, "\n (*in).blk was null." );
				//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncalling cb_set_cursor_ucs length %i, errno %i.", (*nameptr).namelen, errno );

		//		if(encodingstested>=1){				exit(-1); } // DEBUG TMP
				tmp_gdb_test_call( encoding );

			        err = cb_set_cursor_ucs( &in, &(*nameptr).namebuf, &(*nameptr).namelen );
				//cb_clog( CBLOGDEBUG, err, "\n ERR %lli.", err);

				if(err==CBNOTFOUND){
			           fprintf(stderr,"\ttest: cb_set_cursor, CBNOTFOUND, %lli.\n", err ); }
				if(err!=CBNOTFOUND && ( err<=CBERROR && err>=CBNEGATION ) ){ // CBOUTOFBUFFER tai CBSTREAM eika CBSTREAMEND sisally
			           fprintf(stderr,"\ttest: cb_set_cursor, %lli.\n", err ); }
				if(err==CBSTREAMEND || err>CBERROR){
			           fprintf(stderr,"\ttest: cb_set_cursor, STREAMEND or CBERROR, %lli.\n", err );
				}else if(err==CBSUCCESS || err==CBSUCCESSLEAVESEXIST || err==CBSTREAM || err==CBFILESTREAM){
				   //if(err==CBSTREAM){ 
				   //   err = cb_remove_name_from_stream(&in); // This should not necessary, get_chr is not used
			           //   if(err!=CBSUCCESS){ fprintf(stderr,"\ttest: error in cb_remove_name_from_stream, %lli.\n", err ); }
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
                                       if( err!=CBSUCCESS ){ fprintf(stderr,"\ttest:  cb_put_chr, err %lli.", err ); }
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
			              if(err!=CBSUCCESS){ fprintf(stderr,"\ttest: error in cb_remove_name_from_stream, %lli.\n", err ); }
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
				       if( err!=CBSUCCESS && err!=CBSTREAM && err!=CBFILESTREAM ){
				         fprintf(stderr,"\ttest: encbytes=%d strdbytes=%d cb_get_chr cb_put_chr err=%lli.\n", encbytes, strdbytes, err ); }
				       fprintf(stderr,"%lc", (int) chr ); // %C
				       prevchr = chr;
		                       err = cb_get_chr(&in, &chr, &encbytes, &strdbytes );
				       if(err==CBSTREAM){ 
				         err = cb_remove_name_from_stream(&in); // Test if the names value was out of buffer
			                 if(err!=CBSUCCESS){ fprintf(stderr,"\ttest_cb: error in cb_remove_name_from_stream, %lli.\n", err ); }
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
			err2 = cb_reinit_cbfile(&out);
			if(err2>=CBNEGATION){ cb_clog( CBLOGWARNING, err2, "\ntest_cb: cb_reinit_cbfile(&out) returned %i.", err2 );  }
	                ++encoding;
	                ++encodingstested;

			err = close( (*out).fd ); if(err!=0){ fprintf(stderr,"\ttest: close out failed."); }
                
		} // while (encodings)
		err2 = cb_free_names_from( &name_list.name ); // 25.2.2015
		if(err2>=CBNEGATION){ cb_clog( CBLOGWARNING, err2, "\ntest_cb: cb_free_buffer(&name_list) returned %i.", err2 );  }
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

