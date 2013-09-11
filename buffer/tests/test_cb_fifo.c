#include <stdio.h>
#include <string.h>     // memset

#include "../../include/cb_buffer.h"

/* 4 time character count buffer */
#define OUTPUTSTEXTTIMES     40
#define TESTTEXTLENGTH       4 		// readaheadsize on 20 joka jaettuna neljalla on viisi (28->7)

int  main (int argc, char *argv[]);
int  main (int argc, char *argv[]){
        cb_ring cfi; int err=0, i=0, u=0;
        unsigned long int chr = ' ';
	int chrsize=1;

	memset(&(cfi.buf[0]), (int) 0x20, (size_t) CBREADAHEADSIZE);
	memset(&(cfi.storedsizes[0]), (int) 0x20, (size_t) CBREADAHEADSIZE);
        cb_fifo_init_counters(&cfi);

        for(u=0; u<OUTPUTSTEXTTIMES && chr!=(unsigned long int)EOF ; ++u){
          for(i=0; i<TESTTEXTLENGTH && chr!=(unsigned long int)EOF; ++i){
            //cb_fifo_print_counters(&cfi);
            chr = (unsigned long int) getc(stdin);
            if( chr!=(unsigned long int) EOF )
              err = cb_fifo_put_chr(&cfi, chr, chrsize);
	    //fprintf(stdout,"\n+ err=%i", err);
          }

	  //fprintf(stdout,"\n1. err=%i", err);
	  //err = cb_fifo_revert_chr(&cfi, &chr, &chrsize);
	  //fprintf(stdout,"\nReverted: %c , err=%i", chr, err);
          //cb_fifo_print_counters(&cfi);

          for(i=0; i<TESTTEXTLENGTH; ++i){
            //cb_fifo_print_counters(&cfi);
            err = cb_fifo_get_chr(&cfi, &chr, &chrsize);
            if(err==CBSUCCESS)
              fprintf(stdout,"%c", (int) chr);
	    //fprintf(stdout,"\n- err=%i", err);
          }
        }

        return 0;
}
