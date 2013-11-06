#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../../include/cb_buffer.h"

/*
 * Test cb.
 *
 */

int main(int argc, char **argv);

int main(int argc, char **argv) {
	int err = CBSUCCESS;
	CBFILE *in = NULL;

	err = cb_allocate_cbfile(&in, 0, 2048, 512);
        if(err!=CBSUCCESS){ fprintf(stderr,"\nError at cb_allocate_cbfile: %i.", err); return CBERRALLOC;}

	cb_set_encoding(&in, 1);

	err = cb_find_every_name(&in);
	if(err>=CBNEGATION && err!=CBNOTFOUND)
	  fprintf(stderr,"cb_get_next_name_ucs: err=%i.", err);

	cb_print_names(&in);

	cb_free_cbfile(&in);

	return err;
}
