#include <stdio.h>
#include <string.h>     // memset

#include "../../include/cb_buffer.h"

/* 4 time character count buffer */
#define CBFIFOBUFSIZE        28
#define OUTPUTSTEXTTIMES     20
#define TESTTEXTLENGTH       6

int  main (int argc, char *argv[]);
int  main (int argc, char *argv[]){
        cb_ring cfi; int err=0, i=0, u=0;
        unsigned long int chr = ' ';
	int chrsize=1;
        //unsigned char chrbuf[CBFIFOBUFSIZE+1];
        //unsigned char szbuf[CBFIFOBUFSIZE+1];

        //memset(&(chrbuf), (int) 0x20, (size_t) CBFIFOBUFSIZE);
        //chrbuf[CBFIFOBUFSIZE] = '\0';
        //cfi.buf = &chrbuf[0];
        //cfi.storedsizes = &szbuf[0];
	memset(&(cfi.buf[0]), (int) 0x20, (size_t) CBFIFOBUFSIZE);
	memset(&(cfi.storedsizes[0]), (int) 0x20, (size_t) CBFIFOBUFSIZE);
	cfi.buf[CBFIFOBUFSIZE] = '\0';
	cfi.storedsizes[CBFIFOBUFSIZE] = '\0';
        cfi.buflen = CBFIFOBUFSIZE;
        cfi.sizeslen = CBFIFOBUFSIZE;
        cb_fifo_init_counters(&cfi);

        for(u=0; u<OUTPUTSTEXTTIMES && chr!=(unsigned long int)EOF ; ++u){
          for(i=0; i<TESTTEXTLENGTH && chr!=(unsigned long int)EOF && err==CBSUCCESS; ++i){
            //cb_fifo_print_counters(&cfi);
            chr = (unsigned long int) getc(stdin);
            if( chr!=(unsigned long int) EOF )
              err = cb_fifo_put_chr(&cfi, chr, chrsize);
	    //fprintf(stdout,"\n+ err=%i", err);
          }
          for(i=0; i<TESTTEXTLENGTH && err==CBSUCCESS; ++i){
            //cb_fifo_print_counters(&cfi);
            err = cb_fifo_get_chr(&cfi, &chr, &chrsize);
            if(err==CBSUCCESS)
              fprintf(stdout,"%c", (int) chr);
	    //fprintf(stdout,"\n- err=%i", err);
          }
        }

        return 0;
}
