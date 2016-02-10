/* 
 * Functions to check JSON values. 
 * 
 * Copyright (C) 2015 and 2016. Jouni Laakso
 *
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 * 
 * Otherwice, this library is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser
 * General Public License version 2.1 as published by the Free Software Foundation 6. of June year 2012;
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
 * more details. You should have received a copy of the GNU Lesser General Public License along with this library; if
 * not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * licence text is in file LIBRARY_LICENCE.TXT with a copyright notice of the licence text.
 */


#include <stdio.h>
#include <errno.h>
#include <string.h> // memset
#include <unistd.h> // usleep tmp

#include "../include/cb_buffer.h"
#include "../include/cb_compare.h"
#include "../include/cb_json.h"

#define EXPECTCOLON           1
#define EXPECTCOMMA           2
#define EXPECTCOMMAORNAME     3
#define EXPECTDIGIT           4
#define EXPECTDIGITORDOT      5
#define EXPECTDOTOREXP        6
#define EXPECTDIGITBEFOREEXP  7
#define EXPECTEXP             8
#define EXPECTEND             9


typedef struct name {
        unsigned char *name;
        int namelen;
} name;


/* cb_check_json_string(  unsigned char **ucsname, int *namelength, int from  ) is in cb_buffer.h */
int  cb_check_json_substring( CBFILE **cfg, unsigned char **ucsvalue, int ucsvaluelen, int *from );
int  cb_check_json_value_subfunction( CBFILE **cfg, unsigned char **ucsvalue, int ucsvaluelen, int *from );

/*
 * Function reading the white spaces away from the end. 
 *
 * *from is the current character index.
 * If *from is not used, cb_compare compares witn -2 the first names length
 * and *from is updated with the name length.
 */
int  cb_check_json_value( CBFILE **cfg, unsigned char **ucsvalue, int ucsvaluelen, int *from ){
	int err = CBSUCCESS;
	unsigned long int chr=0x20;
	if( from==NULL || cfg==NULL || *cfg==NULL || ucsvalue==NULL || *ucsvalue==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_check_json_value: ucsvalue was null."); return CBERRALLOC; }
	err = cb_check_json_value_subfunction( &(*cfg), &(*ucsvalue), ucsvaluelen, &(*from) );
	/* 
	 * Length. */
	if(*from>ucsvaluelen){
		return CBNOTJSON; 
	}else if(*from<ucsvaluelen){
		/*
		 * Last ',' and ']' check missing 10.11.2015
		 */
		/* 
		 * Read away white spaces. */
		while( *from<ucsvaluelen && ucsvaluelen>0 && *from>0 && *from<CBNAMEBUFLEN && err<CBNEGATION && \
			( WSP( chr ) || CR( chr ) || LF( chr ) ) ){
			err = cb_get_ucs_chr( &chr, &(*ucsvalue), &(*from), ucsvaluelen);
			//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_check_json_value: read away white spaces chr [%c]", (char) chr );
		}
		if( *from==ucsvaluelen )
			return CBSUCCESS;
	}else if( *from==ucsvaluelen )
		return CBSUCCESS;
	return CBNOTJSON;
}
/*
 * The only function reading white spaces away from the beginning. 
 */
int  cb_check_json_value_subfunction( CBFILE **cfg, unsigned char **ucsvalue, int ucsvaluelen, int *from ){
	int err=CBSUCCESS; 
	unsigned long int chr=0x20;
	char atstart=1;
	unsigned char *chrposition = NULL;
	if( from==NULL || cfg==NULL || *cfg==NULL || ucsvalue==NULL || *ucsvalue==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_check_json_value_subfunction: ucsvalue was null."); return CBERRALLOC; }

        //cb_clog( CBLOGDEBUG, CBNEGATION, "\n\ncb_check_json_value_subfunction: attribute ["); 
        //cb_print_ucs_chrbuf( CBLOGEMERG, &(*ucsvalue), ucsvaluelen, ucsvaluelen );
        //cb_clog( CBLOGDEBUG, CBNEGATION, "], length %i, from %i.", ucsvaluelen, *from);

	while( *from<ucsvaluelen && *from<CBNAMEBUFLEN && err<CBNEGATION ){
		chrposition = &(*ucsvalue)[*from];
		err = cb_get_ucs_chr( &chr, &(*ucsvalue), &(*from), ucsvaluelen);
		if(err>=CBERROR){ cb_clog(CBLOGDEBUG, err, "\ncb_check_json_value_subfunction: cb_get_ucs_chr, error %i.", err );  return err; }
		//cb_clog(CBLOGDEBUG, CBNEGATION, "\ncb_check_json_value_subfunction: chr [%c].", (char) chr );
		if( atstart==1 && ( WSP( chr ) || CR( chr ) || LF( chr ) ) )
			continue;
		atstart=0;
		//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_check_json_value_subfunction: [%c]", (char) chr );
		switch( (unsigned char) chr ){
			case '"':
				//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_check_json_substring:" );
				*from -= 4;
				return cb_check_json_substring( &(*cfg), &(*ucsvalue), ucsvaluelen, &(*from) ); 		// ok
			case '[':
				//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_check_json_array:" );
				*from -= 4;
				return cb_check_json_array( &(*cfg), &(*ucsvalue), ucsvaluelen, &(*from) );			// ok
			case '{':
				//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_check_json_object:" );
				*from -= 4;
				return cb_check_json_object( &(*cfg), &(*ucsvalue), ucsvaluelen, &(*from) );			// ok, 12.11.2015
			case 't':
				//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_check_json_true:" );					// ok
				if(*from>0) *from-=4;
				return cb_check_json_true( &(*cfg), &(*ucsvalue), ucsvaluelen, &(*from)  );
			case 'n':
				//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_check_json_null:" );					// ok
				if(*from>0) *from-=4;
				return cb_check_json_null( &(*cfg), &(*ucsvalue), ucsvaluelen, &(*from) );
			case 'f':
				//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_check_json_false:" );					// ok
				if(*from>0) *from-=4;
				return cb_check_json_false( &(*cfg), &(*ucsvalue), ucsvaluelen, &(*from) );
			default:
				//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_check_json_number:" );					// ok
				if(*from>0) *from-=4;
				return cb_check_json_number( &(*cfg), &(*ucsvalue), ucsvaluelen, &(*from) );
		}
	}
	return CBNOTJSON;
}

int  cb_check_json_array( CBFILE **cfg, unsigned char **ucsvalue, int ucsvaluelen, int *from ){
	int err=CBSUCCESS;
	unsigned long int chr = 0x20;
	if( cfg==NULL || *cfg==NULL || ucsvalue==NULL || *ucsvalue==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_check_json_array: ucsvalue was null."); return CBERRALLOC; }
	err = cb_get_ucs_chr( &chr, &(*ucsvalue), &(*from), ucsvaluelen);
	//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_check_json_array: first chr from *from %i is [%c], err %i", (*from-4), (char) chr, err );
	if(err>=CBERROR){ cb_clog( CBLOGDEBUG, err, "\ncb_check_json_array: cb_get_ucs_chr, error %i.", err ); return err; }
	if( (unsigned char) chr == '[' ){

cb_check_json_array_value:
		/* 
		 * First value. */
		//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_check_json_array: to cb_check_json_value_subfunction, *from %i.", *from );
		err = cb_check_json_value_subfunction( &(*cfg), &(*ucsvalue), ucsvaluelen, &(*from) );
		if(err>=CBERROR){ cb_clog( CBLOGDEBUG, err, "\ncb_check_json_array: cb_check_json_value_subfunction, error %i.", err ); return err; }
		//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_check_json_array: after cb_check_json_value_subfunction, %i.", err );
		if(err==CBNOTJSON) return err;
		/* 
		 * Read comma or closing bracket. */
		while( *from<ucsvaluelen && *from<CBNAMEBUFLEN && err<CBNEGATION ){
			err = cb_get_ucs_chr( &chr, &(*ucsvalue), &(*from), ucsvaluelen);
			if(err>=CBERROR){ cb_clog( CBLOGDEBUG, err, "\ncb_check_json_array: cb_get_ucs_chr, error %i.", err ); return err; }
			//cb_clog( CBLOGDEBUG, CBNEGATION, "[ chr: %c indx: %i len: %i]", (char) chr, *from, ucsvaluelen );
			if( WSP( chr ) || CR( chr ) || LF( chr ) ){
				continue;
			}else if( (unsigned char) chr==']' ){
				//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_check_json_array: bracket, CBSUCCESS." );
				return CBSUCCESS; // 10.11.2015
			}else if( (unsigned char) chr==',' ){
				//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_check_json_array: comma, next value." );
				goto cb_check_json_array_value;
			}else
				return CBNOTJSON;
		}
	}else{
		//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_check_json_array: first char was [%c], NOTJSON.", (char) chr );
		return CBNOTJSON;
        }
        return err; 
}

int  cb_check_json_number( CBFILE **cfg, unsigned char **ucsvalue, int ucsvaluelen, int *from ){
	int err=CBSUCCESS, indx=0;
	unsigned long int chr=0x20;
	char state=0, dotcount=0, ecount=0, minuscount=0, firstzero=0, expsigncount=0, atend=0;
	if( from==NULL || ucsvalue==NULL || *ucsvalue==NULL || cfg==NULL || *cfg==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_check_json_number: ucsvalue was null."); return CBERRALLOC; }

	indx = *from;
	if(ucsvaluelen<=0)
		return CBNOTJSON;
	while( indx<ucsvaluelen && indx<CBNAMEBUFLEN && err<CBNEGATION ){
 	   *from = indx;
	   err = cb_get_ucs_chr( &chr, &(*ucsvalue), &indx, ucsvaluelen);
	   //fprintf( stderr, "[%c]", (unsigned char) chr );
	   if(atend==0){
		if( chr=='-' && minuscount==0 && state==0 ){ minuscount=1; state=EXPECTDIGIT; firstzero=0; continue; }
		if( chr=='0' && ( state==0 || ( firstzero==0 && minuscount==1 && state==EXPECTDIGIT ) ) ){ state=EXPECTDOTOREXP; firstzero=1; continue; }
		if( chr>='1' && chr<='9' && state==0 ){ state=EXPECTDIGITORDOT; firstzero=2; continue; }
		if( chr>='0' && chr<='9' && state==EXPECTEND ){ continue; }
		if( chr>='0' && chr<='9' && ( state==EXPECTDIGIT || state==EXPECTDIGITORDOT || state==EXPECTDIGITBEFOREEXP || state==EXPECTEXP ) ){ 
			if(firstzero==1 && state==EXPECTDIGIT)
				return CBNOTJSON;
			if( state==EXPECTDIGITBEFOREEXP )
				state=EXPECTEXP; 
			if( state==EXPECTDIGIT )
				state=EXPECTDIGITORDOT;
			continue; 
		}
		if( chr=='.' && dotcount==0 ){ state=EXPECTDIGITBEFOREEXP; dotcount=1; continue; }
		if( ( chr=='e' || chr=='E' ) && ecount==0 && ( ( dotcount==1 && state!=EXPECTDIGITBEFOREEXP ) || state==EXPECTDOTOREXP || state==EXPECTDIGITORDOT ) ){ 
			state=EXPECTEXP; ecount=1; continue; 
		}
		if( ( chr=='+' || chr=='-' )  && state==EXPECTEXP ){ expsigncount=1; state=EXPECTEND; continue; }
		if( WSP( chr ) || CR( chr ) || LF( chr ) ){ atend=1; continue; }
		if( chr==',' && state!=EXPECTDIGITBEFOREEXP && state!=EXPECTEXP && state!=EXPECTDIGIT ){ 
			return CBSUCCESS;
		}
		if( chr==']' && state!=EXPECTDIGITBEFOREEXP && state!=EXPECTEXP && state!=EXPECTDIGIT ){ 
			return CBSUCCESS;
		}
		return CBNOTJSON;
	   }else{
		if( ! WSP( chr ) && ! CR( chr ) && ! LF( chr ) && chr!=']' && chr!=',' ) 
			return CBNOTJSON;
	   }
	}
	return CBSUCCESS;
}

int  cb_check_json_true( CBFILE **cfg, unsigned char **ucsvalue, int ucsvaluelen, int *from ){
        int err=CBSUCCESS;
        unsigned char *chrposition = NULL;
        name istrue = (name) { (unsigned char *) "\0\0\0t\0\0\0r\0\0\0u\0\0\0e", (int) 16 };
        cb_match mctl; mctl.re=NULL; mctl.matchctl=-2; // match length without the trailing white spaces
        if( cfg==NULL || *cfg==NULL || ucsvalue==NULL || *ucsvalue==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_check_json_true: ucsvalue was null."); return CBERRALLOC; }
        if( *from>=ucsvaluelen ) return CBINDEXOUTOFBOUNDS;
        chrposition = &(*ucsvalue)[*from];

	//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_check_json_true: [");
        //if( ucsvalue!=NULL && *ucsvalue!=NULL )
        //   cb_print_ucs_chrbuf( CBLOGEMERG, &chrposition, istrue.namelen, ucsvaluelen-*from );
	//cb_clog( CBLOGDEBUG, CBNEGATION, "]");

        err = cb_compare( &(*cfg), &istrue.name, istrue.namelen, &chrposition, (ucsvaluelen-*from), &mctl ); // compares name1 to name2 (name1 length)
        if(err>=CBERROR){ cb_clog( CBLOGERR, err, "\ncb_compare_value: cb_compare, error %i.", err ); return err; }
        *from += istrue.namelen;
	//if(err<=CBNEGATION)
	//	cb_clog( CBLOGDEBUG, CBNEGATION, ", result: CBSUCCESS (%i).", err );
        return err;
}
int  cb_check_json_false( CBFILE **cfg, unsigned char **ucsvalue, int ucsvaluelen, int *from ){
        int err=CBSUCCESS;
        unsigned char *chrposition = NULL;
        name isfalse = (name) { (unsigned char *) "\0\0\0f\0\0\0a\0\0\0l\0\0\0s\0\0\0e", (int) 20 };
        cb_match mctl; mctl.re=NULL; mctl.matchctl=-2;
        if( cfg==NULL || *cfg==NULL || ucsvalue==NULL || *ucsvalue==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_check_json_false: ucsvalue was null."); return CBERRALLOC; }
        if( *from>=ucsvaluelen ) return CBINDEXOUTOFBOUNDS;
        err = cb_compare( &(*cfg), &isfalse.name, isfalse.namelen, &chrposition, (ucsvaluelen-*from), &mctl ); // compares name1 to name2 (name1 length)
        if(err>=CBERROR){ cb_clog( CBLOGERR, err, "\ncb_compare_value: cb_compare, error %i.", err ); return err; }
        *from += isfalse.namelen;
        return err;
}
int  cb_check_json_null( CBFILE **cfg, unsigned char **ucsvalue, int ucsvaluelen, int *from ){
        int err=CBSUCCESS;
        unsigned char *chrposition = NULL;
        name isnull = (name) { (unsigned char *) "\0\0\0n\0\0\0u\0\0\0l\0\0\0l", (int) 16 };
        cb_match mctl; mctl.re=NULL; mctl.matchctl=-2;
        if( cfg==NULL || *cfg==NULL || ucsvalue==NULL || *ucsvalue==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_check_json_null: ucsvalue was null."); return CBERRALLOC; }
        if( *from>=ucsvaluelen ) return CBINDEXOUTOFBOUNDS;
        err = cb_compare( &(*cfg), &isnull.name, isnull.namelen, &chrposition, (ucsvaluelen-*from), &mctl );
        if(err>=CBERROR){ cb_clog( CBLOGERR, err, "\ncb_compare_value: cb_compare, error %i.", err ); return err; }
        *from += isnull.namelen;
        return err;
}
/* To be used with arrays and objects. */
int  cb_check_json_substring( CBFILE **cfg, unsigned char **ucsvalue, int ucsvaluelen, int *from ){
	if( from==NULL || cfg==NULL || *cfg==NULL || ucsvalue==NULL || *ucsvalue==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_check_json_null: ucsvalue was null."); return CBERRALLOC; }
	int indx=0, err=CBSUCCESS;
	unsigned long int chr=0x20, chprev=0x20; // chprev is WSP character
	char quotecount=0;
	/*
	 * Read until the second quote and pass the value to cb_check_json_string. */
	indx = *from;
	while( indx<ucsvaluelen && indx<CBNAMEBUFLEN && err<CBNEGATION ){
		if( chr=='\\' && chprev=='\\' )
			chprev = 0x20; // any not bypass
		else
			chprev = chr;
		err = cb_get_ucs_chr( &chr, &(*ucsvalue), &indx, ucsvaluelen);
		//cb_clog( CBLOGDEBUG, CBNEGATION, "\n\tcb_check_json_substring: [%c]", (char) chr );
		if( (unsigned char) chr=='"' && (unsigned char) chprev!='\\' && quotecount>=1 )
			++quotecount;
		if( quotecount>=2 ){
			return cb_check_json_string_content( &(*ucsvalue), indx, &(*from) ); // updates *from
		}
		if( quotecount==0 ){
			if( (unsigned char) chr=='"' && ( ! WSP( chprev ) && ! CR( chprev ) && ! LF( chprev ) && chprev!=',' ) ){
				*from = indx;
				return CBNOTJSON;
			}else if( (unsigned char) chr=='"' && (unsigned char) chprev!='\\' ){
				++quotecount;
			}
		}
	}
	*from = indx;
	return CBNOTJSON;
}

int  cb_check_json_object( CBFILE **cfg, unsigned char **ucsvalue, int ucsvaluelen, int *from ){
	int openpairs=0, err=CBSUCCESS;
	unsigned long int chr=0x20;
	if( from==NULL || cfg==NULL || *cfg==NULL || ucsvalue==NULL || *ucsvalue==NULL ){ cb_clog( CBLOGDEBUG, CBERRALLOC, "\ncb_check_json_null: ucsvalue was null."); return CBERRALLOC; }
	char state=0;
	//cb_clog( CBLOGDEBUG, CBNEGATION, "\ncb_check_json_object:" );
	while( *from<ucsvaluelen && *from<CBNAMEBUFLEN && err<CBNEGATION ){
		err = cb_get_ucs_chr( &chr, &(*ucsvalue), &(*from), ucsvaluelen);
		//cb_clog( CBLOGDEBUG, CBNEGATION, "[%c]", (char) chr );
		if( ( WSP( chr ) || CR( chr ) || LF( chr ) ) ){
			continue;
		}else if( (unsigned char) chr == '{' && state!=EXPECTCOMMA && state!=EXPECTCOLON ){
			/*
			 * Read until '{'. 
			 */
			++openpairs;
cb_check_json_object_read_name:
			/* 
			 * Reads name within the quotes and update *from. Quotes are bypassed. */
			err = cb_check_json_substring( &(*cfg), &(*ucsvalue), ucsvaluelen, &(*from) );
			state = EXPECTCOLON;
			continue;
		}else if( state==EXPECTCOLON && (unsigned char) chr == ':' ){
			/*
			 * Read colon ':'. 
			 */
			/* 
			 * Read Value. */
			err = cb_check_json_value_subfunction( &(*cfg), &(*ucsvalue), ucsvaluelen, &(*from) );
			state = EXPECTCOMMA;
			continue;
		}else if( ( state==EXPECTCOMMA || state==EXPECTCOMMAORNAME ) && ( (unsigned char) chr == ',' || (unsigned char) chr == '}' ) ){
			/*
			 * Read comma ',' or '}' . 
			 */
			if( (unsigned char) chr == '}' ){
				state = EXPECTCOMMAORNAME;
				--openpairs;
			}else{
				/*
				 * Read next name. */
				goto cb_check_json_object_read_name;
			}
			continue;
		}else if( state==EXPECTCOMMAORNAME && (unsigned char) chr == '"' ){
			*from-=4;
			goto cb_check_json_object_read_name;
		}else if( (unsigned char) chr == '}' ){
			--openpairs;
			if( state!=EXPECTCOMMA )
				err = CBNOTJSON;
			state = 0;
			continue;
		}else
			err = CBNOTJSON;
	}
	if(openpairs==0)
		return err;
	return CBNOTJSON;
}
