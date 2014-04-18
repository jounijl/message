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
	int err = 0;
	unsigned long int chr = 0;
	int bcount=0, strdbytes=0;
	CBFILE *in = NULL;

	err = cb_allocate_cbfile(&in, 0, 2048, 512);
        if(err!=CBSUCCESS){ fprintf(stderr,"\nError at cb_allocate_cbfile: %i.", err); return CBERRALLOC;}

	//cb_set_encoding(&in, 0);
	cb_set_encoding(&in, 3); // 10.11.2013

	do{
	  err = cb_get_chr(&in, &chr, &bcount, &strdbytes );
	  if(err!=CBSTREAMEND && err<CBERROR)
            putchar( (int) chr );
	}while(err!=CBSTREAMEND && err<CBERROR);

	cb_free_cbfile(&in);

	return err;
}
