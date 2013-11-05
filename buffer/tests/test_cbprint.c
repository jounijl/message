#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../../include/cb_buffer.h"

/*
 * Read from utf and output UCS converted to one character.
 *
 * Text editors, for example gedit or OpenOffice writer can be used 
 * to read multibyte characters.
 */

int main(int argc, char **argv);

int main(int argc, char **argv) {
	int err = 0, err2 = CBSUCCESS;
	unsigned long int chr = 0, chprev = 0;
	unsigned char *name = NULL;
	//unsigned char pad = '\0';
	int namelen = 0;
	long int offset=0;
	CBFILE *in = NULL;
	//name = &pad;

	err = cb_allocate_cbfile(&in, 0, 2048, 512);
        if(err!=CBSUCCESS){ fprintf(stderr,"\nError at cb_allocate_cbfile: %i.", err); return CBERRALLOC;}

	cb_set_encoding(&in, 1);

	do{
          putchar( (int) 0x000A );
	  err = cb_get_next_name_ucs(&in, &name, &namelen);
	  fprintf(stderr,"err=%i.", err);
	  if( err==CBSTREAM || err==CBSUCCESS ){
	    cb_print_ucs_chrbuf(&name, namelen, namelen);	
            fprintf( stderr, "=" );
	    err2==CBSUCCESS;
	    while( ( err2==CBSTREAM || err2==CBSUCCESS ) && ! ( chr==(*in).rend && chprev!=(*in).bypass ) ){
	      chprev = chr;
	      err2 = cb_search_get_chr(&in, &chr, &offset );
	      if( ( err2==CBSTREAM || err2==CBSUCCESS ) && ! ( chr==(*in).rend && chprev!=(*in).bypass ) )
                fprintf( stderr, "%lc", chr );
	    }
	  }
	  free(name);
	}while( err!=CBNOTFOUND && err!=CBSTREAMEND && err<CBERROR );

	cb_free_cbfile(&in);

	return err;
}
