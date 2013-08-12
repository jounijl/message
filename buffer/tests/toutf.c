#include <unistd.h>
#include <fcntl.h> // F_DUPFD, fcntl
#include <errno.h>
#include <stdio.h> // printf
#include "../../include/cb_buffer.h"

#define TESTFILESIZE      	16384

/* Generate characters in UCS to use in testing, outputs them in UTF. */

/*
 * Verification of output codes:
 * For example gedit shows Ancient Greek 5 byte text as image showing
 * the code in image. It can show Greek 2 byte text.
 */

int write_ucs_2B( int count );
int write_ucs_5B( int count );
void usage (char *progname[]);

// 
// Reads bytes from /dev/random
// outputs a word from them in these encodings
int main(int argc, char **argv) {
	int err=0;
	int count=TESTFILESIZE;
	char *str = NULL;
	if(argc>=3 && argv[2]!=NULL){ // count of characters
	  count = (int) strtol(argv[2],&str,10);
          if(count==0 && errno==EINVAL)
            count = TESTFILESIZE;
	}
	if(argc>=2 && argv[1]!=NULL){ // unicodes bytecount range
 	  err = strncmp(argv[1],"5",1);
	  if( err == 0 ){
	    return write_ucs_5B( count );
	  }
 	  err = strncmp(argv[1],"2",1);
	  if( err == 0 ){
	    return write_ucs_2B( count );
 	  }
	}else{ // usage message
	  if(argv[0]!=NULL)
	    usage(&argv[0]);
	}
	return err;
}

/*
   Generate characters in Unicode 5 bytes
   Ancient greek numbers: http://www.unicode.org/charts/PDF/U10140.pdf
   10140 - 1018A 
*/

int write_ucs_5B( int count ) {
        char ch=0;
        unsigned char str[2]={'0','\0'};
        long int num=0;
        int base = 16;
        CBFILE *out = NULL;
	int fd=1;
        unsigned long int chr = 0;
        unsigned long int orchr = 65792; // 10100H
        int bcount=3, strdbytes=5, err=0;

	fd = fcntl(STDOUT_FILENO, F_DUPFD, 0); 
        err = cb_allocate_cbfile(&out, fd, 0, 512); // stdout
        if(err!=CBSUCCESS){ fprintf(stderr,"\nError at cb_allocate_cbfile: %i.", err); return CBERRALLOC;}

        while ( ch = read(0, &(*str), (size_t) 1 ) && count > 0 ){
                if(ch==-1 && errno==0)
                   return 0;
                else if(ch<0){
                   perror("error");
                   return 1;
                }
                if( str[0]<0x40 || str[0]>0x8A )
	          continue;

		chr = orchr | (unsigned int) str[0] ;
                //err = cb_put_chr(&out, &chr, &bcount, &strdbytes );
                err = cb_put_chr(&out, chr, &bcount, &strdbytes ); // 12.8.2013
		--count;
		//fprintf(stderr, "|%lx", chr );
		if(err>CBERROR)
		  fprintf(stderr,"\nError at cb_put_chr.");
        }
	err = cb_flush(&out);
	if(err!=CBSUCCESS){ fprintf(stderr,"\nError at cb_flush: %i.", err); }
	cb_free_cbfile(&out);
        return CBSUCCESS;
}
/*
   Generate characters in Unicode 2 byte
   Greek and coptic: http://www.unicode.org/charts/PDF/U0370.pdf
   0370 - 03FF 
  (Except:
   0380,   0381,   0382,   0383,   03A2,   
   0378,   0379,   038B,   038D,   037F )
*/
int write_ucs_2B( int count ) {
        char ch=0;
        unsigned char str[2]={'0','\0'};
	int fd=1;
        CBFILE *out = NULL;
        unsigned long int chr = 0, orchr = (0x03<<8 | 0x00) ; // 0300H
        int bcount=5, strdbytes=0, err=0;

	fd = fcntl(STDOUT_FILENO, F_DUPFD, 0); 
        err = cb_allocate_cbfile(&out, fd, 0, 512); 
        if(err!=CBSUCCESS){ fprintf(stderr,"\nError at cb_allocate_cbfile: %i.", err); return CBERRALLOC;}

        while ( ch = read(0, &(*str), (size_t) 1 ) && count > 0 ){
                if(ch==-1 && errno==0)
                   return 0;
                else if(ch<0){
                   perror("error");
                   return 1;
                }
                // Lower parts of code, higher part is 03
                if( str[0]<0x70 ) // || str[0]>0xFF )
                  continue;
                if( str[0]>=0x80 && str[0]<=0x83 )
                  continue;
                if( str[0]==0xA2 || str[0]==0x78 || str[0]==0x79 || str[0]==0x8B || str[0]==0x8D || str[0]==0x7F )
                  continue;

		chr = orchr | str[0] ;
		//fprintf(stderr, "|%x%x", (int) chr>>8, (int) ( chr & 0xff ) );
	        //err = cb_put_chr(&out, &chr, &bcount, &strdbytes );
	        err = cb_put_chr(&out, chr, &bcount, &strdbytes ); // 12.8.2013
		--count;
		if(err>CBERROR)
		  fprintf(stderr,"\nError at cb_put_chr.");
        }
	err = cb_flush(&out);
	if(err!=CBSUCCESS){ fprintf(stderr,"\nError at cb_flush: %i.", err); }
	cb_free_cbfile(&out);
        return CBSUCCESS;
}
void usage (char *progname[]){
        printf("Usage:\n");
        printf("\tcat /dev/random | %s 2B|5B [character count]\n", progname[0]);
        printf("\tPrints 2 or 5 byte UTF encoded Unicode characters.\n");
        printf("\t(Greek and Coptic 2B: U0370 and Ancient greek\n");
	printf("\tnumbers 5B: U10140).\n");
}
