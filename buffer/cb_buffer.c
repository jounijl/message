/* 
 * Library to read and write streams. Valuepair list and search. Different character encodings.
 * 
 * Copyright (C) 2009, 2010 and 2013. Jouni Laakso
 * 
 * This library is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General
 * Public License version 2.1 as published by the Free Software Foundation 6. of June year 2012;
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
 * more details. You should have received a copy of the GNU Lesser General Public License along with this library; if
 * not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * licence text is in file LIBRARY_LICENCE.TXT with a copyright notice of the licence text.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h> // memset
#include "../include/cb_buffer.h"

/*
 Todo:
 x Not into this: 
   - Nesting =<text>& => <value1>=<text><value1.1>=<text2>&<text3>&
     ( cb_buffer only shows the start of <text> and has nothing to do with the
     rest of the <text>.)
     - Make nesting to a reader function and writer function.
       - Number showing nesting level and subfiles for example
         for subnests.
   x As in stanza or http, two '\n'-characters ends value (now default is '&')
     ( not into this, same explanation as before )
     - use lwspfolding and rend='\n'
   - Text to file functions (somewhere else than in this file):
     - Fixed length blocks:
       - put_next_pair
       - get_next_pair
 x Document on using the library in www
   - html meta tags
 - Testing plan 
 - Bug tracking
 - cb_get_chr tunnistamaan useamman eri koodauksen virheellinen tavu, CBNOTUTF jne.
   erilaiset yhteen funktiossa cb_get_chr ja siita yleisemmin CBENOTINENCODING tms.
 x Testaukseen, kokeillaan muuttaa arvosta muuttujan arvoa. Laitetaan arvoksi nimiparin nakoinen yhdistelma. 
   (Tama tietenkin korvaantuu ohjelmoijan lisaamilla ohitusmerkeilla, http:ssa ei tarvitse.)
 x CBSTATETOPOLOGY
   - nice names
 x character encoding tests, multibyte and utf-32
 - boundary tests
 - stress test
 - cb_put_name cb_name malloc nimen koon perusteella ennen listaan tallettamista
 x Nimet sisaltavan valilyonteja ja tabeja. Ohjeen mukaan ne poistetaan ennen vertausta.
   -> nimiin ei kosketa, nimen ja arvon valista voi ottaa nyt valilyonnit ja tabit pois
 x Nimimoodi ei toimi, listamoodi toimii: cat tests/testi.txt | ./cbsearch -c 1 -b 2048 -l 512 unknown
 x Jos lisaa loppuun esim. 2> /dev/null, listan loppuun tulostuu merkkeja ^@^@^@...
 x Haku nimi3 loytaa nimet nimi1 ja nimi2 mutta ei jos unfold on pois paalta.
 - bypass ja RFC 2822 viestit eivat sovi yhteen. Jos bypassin korvaa erikoismerkilla,
   sita ei saisi lukea ja ainoastaa viesti ei ole valid jos siina on erikoismerkkeja.
 - Ohjeeseen: rstart ei saa olla "unfolding" erikoismerkki
 - Unfolding erikoismerkit pitaa verrata ohjausmerkkeihin ja tarkastaa 
 - Jokainen taulukko on vain taulukko ilman pointeria ensimmaiseen?
   x jokainen malloc tarkistettu
 - space/tab ei saa esiintya rfc2822:n nimissa, arvon lopetusmerkiksi?
   (viimeiset muutokset ja kommentit ovat huonoja)
 x CTL:ia ei saa poistaa, niita ei saa nimessa kuitenkaan olla (korkeintaan virheilmoitus)
   -> qualify name tms. johon kuuluvat muutkin maareet joita nimen tulee toteuttaa
 x BOM poisto takaisin
 x valkoista jaa viela nimen alkuun (2 kpl) vaikka unfolding on paalla
 x cb_fifo_revert_chr testi
 x #define CB2822MESSAGE siirtaa tekstin eteenpain ja laittaa valilyonnin eteen,
   myos vertaus palauttaa oikein vaikka viimeinen kirjain olisi eri.  
 - Ohjeeseen: arvon pituus ei paivity heti, vasta toisen nimen lisayksen yhteydessa
   Se ei ole heti kaytettavissa.
 x cbsearch ei loyda ensimmaista nimea heti vaan vasta toisella kerralla.
 - Testausohjelmiin kuvaavat virheilmoitukset.
 ===
 - cb_conf:iin myos eri hakutavat
 x listaan lisaksi aikaleima milloin viimeksi haettu
 */

int  cb_get_char_read_block(CBFILE **cbf, char *ch);
int  cb_set_type(CBFILE **buf, char type);
int  cb_compare_strict(unsigned char **name1, int len1, unsigned char **name2, int len2, int from2);
int  cb_compare_rfc2822(unsigned char **name1, int len1, unsigned char **name2, int len2, int from2);


/*
 * Debug
 */
int  cb_print_names(CBFILE **str){ 
	cb_name *iter = NULL; int names=0;
	fprintf(stderr, "\n cb_print_names: \n");
	if(str!=NULL){
	  if( *str !=NULL){
            iter = &(*(*(**str).cb).name);
            if(iter!=NULL){
              do{
	        ++names;
	        fprintf(stderr, " [%i/%li] name [", names, (*(**str).cb).namecount );
                if(iter!=NULL){
	            cb_print_ucs_chrbuf(&(*iter).namebuf, (*iter).namelen, (*iter).buflen);
	        }
                fprintf(stderr, "] offset [%lli] length [%i]", (*iter).offset, (*iter).length);
                fprintf(stderr, " matchcount [%li]", (*iter).matchcount);
                fprintf(stderr, " first found [%li]", (*iter).firsttimefound);
                fprintf(stderr, " last time used [%li]\n", (*iter).lasttimeused);
                iter = &(* (cb_name *) (*iter).next );
              }while( iter != NULL );
              return CBSUCCESS;
	    }
	  }
	}else{
	  fprintf(stderr, "\n str was null "); 
	}
        return CBERRALLOC;
}
// Debug
void cb_print_counters(CBFILE **cbf){
        if(cbf!=NULL && *cbf!=NULL)
          fprintf(stderr, "\nnamecount:%li \t index:%li \t contentlen:%li \t  buflen:%li", (*(**cbf).cb).namecount, \
             (*(**cbf).cb).index, (*(**cbf).cb).contentlen, (*(**cbf).cb).buflen );
}

int  cb_compare_rfc2822(unsigned char **name1, int len1, unsigned char **name2, int len2, int from2){ // from2 23.11.2013
	unsigned long int chr1=0x65, chr2=0x65;
	int indx1=0, indx2=0, err1=CBSUCCESS, err2=CBSUCCESS;
	if(name1==NULL || name2==NULL || *name1==NULL || *name2==NULL)
	  return CBERRALLOC;

	//fprintf(stderr,"\ncb_compare_rfc2822: [");
	//cb_print_ucs_chrbuf(&(*name1), len1, len1); fprintf(stderr,"] [");
	//cb_print_ucs_chrbuf(&(*name2), len2, len2); fprintf(stderr,"] len1: %i, len2: %i.", len1, len2);

	indx2 = from2;
	while( indx1<len1 && indx2<len2 && err1==CBSUCCESS && err2==CBSUCCESS ){ // 9.11.2013

	  err1 = cb_get_ucs_chr(&chr1, &(*name1), &indx1, len1);
	  err2 = cb_get_ucs_chr(&chr2, &(*name2), &indx2, len2);

	  if(err1>=CBNEGATION || err2>=CBNEGATION){
	    return CBNOTFOUND;
	  }
	
	  if( chr2 == chr1 ){
	    continue; // while
	  }else if( chr1 >= 65 && chr1 <= 90 ){ // large
	    if( chr2 >= 97 && chr2 <= 122 ) // small, difference is 32
	      if( chr2 == (chr1+32) ) // ASCII, 'A' + 32 = 'a'
	        continue;
	  }else if( chr2 >= 65 && chr2 <= 90 ){ // large
	    if( chr1 >= 97 && chr1 <= 122 ) // small
	      if( chr1 == (chr2+32) ) 
	        continue;
	  }
	  return CBNOTFOUND;
	}

	if( len1==len2 || len1==(len2-from2) )
	  return CBMATCH;
	else if( len2>len1 || len1<(len2-from2) ) // 9.11.2013, 23.11.2013
	  return CBMATCHLENGTH;
	else if( len2<len1 || len1>(len2-from2)) // 9.11.2013, 23.11.2013
	  return CBMATCHPART;

	return err1;
}
/*
 * Compares if name1 matches name2. CBMATCHPART returns if name1 is longer
 * than name2. CBMATCHLENGTH returns if name1 is shorter than name2.
 */
int  cb_compare(CBFILE **cbs, unsigned char **name1, int len1, unsigned char **name2, int len2, int matchctl){
	int err = CBSUCCESS, dfr = 0, indx = 0;
	unsigned char stb[3] = { 0x20, 0x20, '\0' };
	unsigned char *stbp = NULL;
	stbp = &stb[0]; 


	if(matchctl==-1)
	  return CBNOTFOUND; // no match
	if( cbs==NULL || *cbs==NULL )
	  return CBERRALLOC;
	if( matchctl==-6 ){ // %am%, not tested or used 22.11.2013
	  dfr = (len2 - len1) - 1; // index of name2 to start searching
	  if(dfr<0){
	    err = CBNOTFOUND;
	  }else{
	    for(indx=0; ( indx<dfr && err!=CBMATCH && err!=CBMATCHLENGTH ); indx+=4){ // %am%, comparisons: dfr*len1
	      if( (**cbs).cf.asciicaseinsensitive==1 && (len2-indx) > 0 ){
	        err = cb_compare_rfc2822( &(*name1), len1, &(*name2), len2, indx );
	      }else if( (len2-indx) > 0 ){
	        err = cb_compare_strict( &(*name1), len1, &(*name2), len2, indx );
	      }else{
	        err = CBNOTFOUND;
	      }
	    }
	  }
	}else if( matchctl==-5 ){ // %ame, not tested or used 22.11.2013
	  dfr = len2 - len1;
	  if(dfr<0){
	    err = CBNOTFOUND;
	  }else if( (**cbs).cf.asciicaseinsensitive==1 && (len2-dfr) > 0 ){
	    err = cb_compare_rfc2822( &(*name1), len1, &(*name2), len2, dfr );
	  }else if( (len2-dfr) > 0 ){
	    err = cb_compare_strict( &(*name1), len1, &(*name2), len2, dfr );
	  }else{
	    err = CBNOTFOUND;
	  }
	}else if( matchctl==0 ){ // all
	  stbp = &stb[0]; 
	  err = cb_compare_strict( &stbp, 0, &(*name2), len2, 0 );
	}else if((**cbs).cf.asciicaseinsensitive==1){ // name or nam%
	  err = cb_compare_rfc2822( &(*name1), len1, &(*name2), len2, 0 );
	}else{ // name or nam%
	  err = cb_compare_strict( &(*name1), len1, &(*name2), len2, 0 );
	}

	switch (matchctl) {
	  case  0:
	    if( err==CBMATCHLENGTH )
	      return CBMATCH; // any
	    break;
	  case -2:
	    if( err==CBMATCH || err==CBMATCHLENGTH )
	      return CBMATCH; // length (name can be cut for example nam matches name1)
	    break;
	  case -3:
	    if( err==CBMATCH || err==CBMATCHPART )
	      return CBMATCH; // part
	    break;
	  case -4:
	    if( err==CBMATCH || err==CBMATCHPART || err==CBMATCHLENGTH )
	      return CBMATCH; // part or length
	    break;
	  case -5:
	    if( err==CBMATCH )
	      return CBMATCH; // ( cut from end ame1 matches name1 )
	    break;
	  case -6:
	    if( err==CBMATCH || err==CBMATCHLENGTH )
	      return CBMATCH; // in the middle
	    break;
	  default:
	    return err; 
	}
	return err; // other information
}
int  cb_compare_strict(unsigned char **name1, int len1, unsigned char **name2, int len2, int from2){ // from2 23.11.2013
	int indx1=0, indx2=0, err=CBMATCHLENGTH, num=0;
	if( name1==NULL || name2==NULL || *name1==NULL || *name2==NULL )
	  return CBERRALLOC;

	//fprintf(stderr,"cb_compare: name1=[");
	//cb_print_ucs_chrbuf(&(*name1), len1, len1); fprintf(stderr,"] name2=[");
	//cb_print_ucs_chrbuf(&(*name2), len2, len2); fprintf(stderr,"]\n");

	num=len1;
	if(len1>len2)
	  num=len2;
	indx1=0;
	for(indx2=from2; indx1<num && indx2<num ;++indx2){
	  if((*name1)[indx1]!=(*name2)[indx2]){
	    indx2=num+7; err=CBNOTFOUND;
	  }else{
	    if( len1==len2 || len1==(len2-from2) )
	      err=CBMATCH;
	    else
	      err=CBMATCHPART; // 30.3.2013, was: match, not notfound
	  }
	  ++indx1;
	}
	if( len1==len2 )
	  err=CBMATCH;
	else if( len2>len1 ) // 9.11.2013
	  err=CBMATCHLENGTH;
	else if( len2<len1 ) // 9.11.2013
	  err=CBMATCHPART;

	return err;
}

int  cb_copy_name( cb_name **from, cb_name **to ){
	int index=0;
	if( from!=NULL && *from!=NULL && to!=NULL && *to!=NULL ){
	  for(index=0; index<(**from).namelen && index<(**to).buflen ; ++index)
	    (**to).namebuf[index] = (**from).namebuf[index];
	  (**to).namelen = index;
	  (**to).offset  = (**from).offset;
          (**to).length  = (**from).length; // 20.8.2013
          (**to).matchcount = (**from).matchcount; // 20.8.2013
	  (**to).next   = (**from).next; // This knows where this points to, so it only can be copied but references to this can not
	  return CBSUCCESS;
	}
	return CBERRALLOC;
}

int  cb_allocate_name(cb_name **cbn){ 
	int err=0;
	*cbn = (cb_name*) malloc(sizeof(cb_name));
	if(*cbn==NULL)
	  return CBERRALLOC;
	(**cbn).namebuf = (unsigned char*) malloc(sizeof(char)*(CBNAMEBUFLEN+1));
	if((**cbn).namebuf==NULL)
	  return CBERRALLOC;
	for(err=0;err<CBNAMEBUFLEN;++err)
	  (**cbn).namebuf[err]=' ';
	(**cbn).namebuf[CBNAMEBUFLEN]='\0';
	(**cbn).buflen=CBNAMEBUFLEN;
	(**cbn).namelen=0;
	(**cbn).offset=0; 
	(**cbn).length=0;
	(**cbn).matchcount=0;
	(**cbn).firsttimefound=-1;
	(**cbn).lasttimeused=-1;
	(**cbn).next=NULL;
	return CBSUCCESS;
}

int  cb_set_cstart(CBFILE **str, unsigned long int cstart){ // comment start
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).cstart=cstart;
	return CBSUCCESS;
}
int  cb_set_cend(CBFILE **str, unsigned long int cend){ // comment end
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).cend=cend;
	return CBSUCCESS;
}
int  cb_set_rstart(CBFILE **str, unsigned long int rstart){ // value start
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).rstart=rstart;
	return CBSUCCESS;
}
int  cb_set_rend(CBFILE **str, unsigned long int rend){ // value end
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).rend=rend;
	return CBSUCCESS;
}
int  cb_set_bypass(CBFILE **str, unsigned long int bypass){ // bypass , new - added 19.12.2009
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).bypass=bypass;
	return CBSUCCESS;
}
int  cb_set_likechr(CBFILE **str, unsigned long int likechr){ // bypass , new - added 19.12.2009
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).likechr=likechr;
	return CBSUCCESS;
}

int  cb_allocate_cbfile(CBFILE **str, int fd, int bufsize, int blocksize){
	unsigned char *blk = NULL; 
	return cb_allocate_cbfile_from_blk(str, fd, bufsize, &blk, blocksize);
}

int  cb_allocate_cbfile_from_blk(CBFILE **str, int fd, int bufsize, unsigned char **blk, int blklen){ 
	int err=CBSUCCESS;
	*str = (CBFILE*) malloc(sizeof(CBFILE));
	if(*str==NULL)
	  return CBERRALLOC;
	(**str).cb = NULL; (**str).blk = NULL;
        (**str).cf.type=CBCFGSTREAM; // default
        //(**str).cf.type=CBCFGFILE; // first test was ok
        (**str).cf.searchmethod=CBSEARCHNEXTNAMES; // default
        //(**str).cf.searchmethod=CBSEARCHUNIQUENAMES;
#ifdef CB2822MESSAGE
	(**str).cf.asciicaseinsensitive=1;
	(**str).cf.unfold=1;
	(**str).cf.removewsp=0; // default
	(**str).cf.removecrlf=0; // default
	(**str).cf.rfc2822headerend=1; // default, stop at headerend
	//(**str).rstart=0x00003A; // ':', default
	//(**str).rend=0x00000A;   // LF, default
	(**str).rstart=CBRESULTSTART; // tmp
	(**str).rend=CBRESULTEND; // tmp
#else
	(**str).cf.asciicaseinsensitive=0; // default
	//(**str).cf.asciicaseinsensitive=1; // test
	(**str).cf.unfold=0;
	(**str).cf.removewsp=1;
	(**str).cf.removecrlf=1;
	(**str).cf.rfc2822headerend=0;
	(**str).rstart=CBRESULTSTART;
	(**str).rend=CBRESULTEND;
#endif
	(**str).bypass=CBBYPASS;
	(**str).cstart=CBCOMMENTSTART;
	(**str).cend=CBCOMMENTEND;
	(**str).likechr=CBLIKECHR;
	(**str).encodingbytes=CBDEFAULTENCODINGBYTES;
	(**str).encoding=CBDEFAULTENCODING;
	(**str).fd = dup(fd);
	if((**str).fd == -1){ err = CBERRFILEOP; (**str).cf.type=CBCFGBUFFER; } // 20.8.2013

	cb_fifo_init_counters( &(**str).ahd );
	err = cb_allocate_buffer(&(**str).cb, bufsize);
	if(err==-1){ return CBERRALLOC;}
	if(*blk==NULL){
	  err = cb_allocate_buffer(&(**str).blk, blklen);
	}else{
	  err = cb_allocate_buffer(&(**str).blk, 0); // blk
	  if(err==-1){ return CBERRALLOC;}
	  free( (*(**str).blk).buf );
	  (*(**str).blk).buf = &(**blk);
	  (*(**str).blk).buflen = blklen;
	  (*(**str).blk).contentlen = blklen;
	}
	if(err==-1){ return CBERRALLOC;}
	return CBSUCCESS;
}

int  cb_allocate_buffer(cbuf **cbf, int bufsize){ 
	int err=0;
	*cbf = (cbuf *) malloc(sizeof(cbuf));
	if(cbf==NULL)
	  return CBERRALLOC;
	(**cbf).buf = (unsigned char *) malloc(sizeof(char)*(bufsize+1));
	if( (**cbf).buf == NULL )
	  return CBERRALLOC;

	for(err=0;err<bufsize;++err)
	  (**cbf).buf[err]=' ';
	(**cbf).buf[bufsize]='\0';
	(**cbf).buflen=bufsize;
	(**cbf).contentlen=0;
	(**cbf).namecount=0;
	(**cbf).index=0;
        (**cbf).offsetrfc2822=0;
	(**cbf).name=NULL;
	(**cbf).current=NULL;
	(**cbf).last=NULL;
	return CBSUCCESS;
}

int  cb_reinit_cbfile(CBFILE **buf){
	int err=CBSUCCESS;
	if( buf==NULL || *buf==NULL ){ return CBERRALLOC; }
	err = cb_reinit_buffer(&(**buf).cb);
	err = cb_reinit_buffer(&(**buf).blk);
	return err;
}

int  cb_free_cbfile(CBFILE **buf){ 
	int err=CBSUCCESS; 
	cb_reinit_buffer(&(**buf).cb); // free names
	if((*(**buf).cb).buf!=NULL)
	  free((**buf).cb->buf); // free buffer data
	free((**buf).cb); // free buffer
	if((*(**buf).blk).buf!=NULL)
          free((**buf).blk->buf); // free block data
	free((**buf).blk); // free block
	if((**buf).cf.type!=CBCFGBUFFER){ // 20.8.2013
	  err = close((**buf).fd); // close stream
	  if(err==-1){ err=CBERRFILEOP;}
	}
	free(*buf); // free buf
	return err;
}
int  cb_free_buffer(cbuf **buf){
        int err=CBSUCCESS;
        err = cb_reinit_buffer( &(*buf) );
        free( (**buf).buf );
        free( *buf );
        return err;
}
int  cb_free_fname(cb_name **name){
        int err=CBSUCCESS;
        if(name!=NULL && *name!=NULL)
          free((**name).namebuf);
        else
          err=CBERRALLOC; 
        free(*name);
        return err;
}
int  cb_reinit_buffer(cbuf **buf){ // free names and init
	cb_name *name = NULL;
	cb_name *nextname = NULL;
	if(buf!=NULL && *buf!=NULL){
	  (**buf).index=0;
	  (**buf).contentlen=0;
	  name = (**buf).name;
	  while(name != NULL){
	    nextname = &(* (cb_name*) (*name).next);
            cb_free_fname(&name);
	    name = &(* nextname);
	  }
	  (**buf).namecount=0;
	}
	free(name); free(nextname);
	(**buf).name=NULL; // 1.6.2013
	(**buf).current=NULL; // 1.6.2013
	(**buf).last=NULL; // 1.6.2013
	return CBSUCCESS;
}

int  cb_use_as_buffer(CBFILE **buf){
        return cb_set_type(&(*buf), (char) CBCFGBUFFER);
}
int  cb_use_as_file(CBFILE **buf){
        return cb_set_type(&(*buf), (char) CBCFGFILE);
}
int  cb_use_as_stream(CBFILE **buf){
        return cb_set_type(&(*buf), (char) CBCFGSTREAM);
}
int  cb_set_type(CBFILE **buf, char type){ // 20.8.2013
	if(buf!=NULL){
	  if((*buf)!=NULL){
	    (**buf).cf.type=type; // 20.8.2013
	    return CBSUCCESS;
	  }
	}
	return CBERRALLOC;
}

int  cb_get_char_read_block(CBFILE **cbf, char *ch){
	ssize_t sz=0; // int err=0;
	cblk *blk = NULL; 
	blk = (**cbf).blk;

	if(blk!=NULL){
	  if( ( (*blk).index < (*blk).contentlen ) && ( (*blk).contentlen <= (*blk).buflen ) ){
	    // return char
	    *ch = (*blk).buf[(*blk).index];
	    ++(*blk).index;
	  }else if((**cbf).cf.type!=CBCFGBUFFER){ // 20.8.2013
	    // read a block and return char
	    sz = read((**cbf).fd, (*blk).buf, (ssize_t)(*blk).buflen);
	    (*blk).contentlen = (int) sz;
	    if( 0 < (int) sz ){ // read more than zero bytes
	      (*blk).contentlen = (int) sz;
	      (*blk).index = 0;
	      *ch = (*blk).buf[(*blk).index];
	      ++(*blk).index;
	    }else{ // error or end of file
//	      (*blk).contentlen = 0; (*blk).index = 0; *ch = ' ';
	      (*blk).index = 0; *ch = ' ';
	      return CBSTREAMEND;
	    }
	  }else{ // use as block
	    return CBSTREAMEND;
	  }
	  return CBSUCCESS;
	}else
	  return CBERRALLOC;
}

int  cb_flush(CBFILE **cbs){
	int err=CBSUCCESS; // indx=0;
	if(*cbs!=NULL){
 	  if((**cbs).cf.type!=CBCFGBUFFER){
	    if((**cbs).blk!=NULL){
	      if((*(**cbs).blk).contentlen <= (*(**cbs).blk).buflen )
	        err = (int) write((**cbs).fd, (*(**cbs).blk).buf, (size_t) (*(**cbs).blk).contentlen);
	      else
	        err = (int) write((**cbs).fd, (*(**cbs).blk).buf, (size_t) (*(**cbs).blk).buflen); // indeksi nollasta ?
	      if(err<0){
	        err = CBERRFILEWRITE ; 
	      }else{
	        err = CBSUCCESS;
	        (*(**cbs).blk).contentlen=0;
	      }
	      return err;
	    }
	  }else
	    return CBUSEDASBUFFER;
	}
	return CBERRALLOC;
}

/* added 15.12.2009, not yet tested */
// Write to block to read again (in regular expression matches for example).
int  cb_write_to_block(CBFILE **cbs, unsigned char *buf, int size){ 
	int indx=0;
	if(*cbs!=NULL && buf!=NULL && size!=0)
	  if((**cbs).blk!=NULL){
	    for(indx=0; indx<size; ++indx){
	      if((*(**cbs).blk).contentlen < (*(**cbs).blk).buflen ){
	        (*(**cbs).blk).buf[(*(**cbs).blk).contentlen] = buf[indx];
	        ++(*(**cbs).blk).contentlen;
	      }else{
	        return CBBUFFULL;
	      }
	    }
	    return CBSUCCESS;
	  }
	return CBERRALLOC;
}

int  cb_write(CBFILE **cbs, unsigned char *buf, int size){ 
	int err=0;
	if(*cbs!=NULL && buf!=NULL){
	  if((**cbs).blk!=NULL){
	    for(err=0; err<size; ++err){
	      //cb_put_ch(cbs,&buf[err]);
	      cb_put_ch(cbs,buf[err]); // 12.8.2013
	    }
	    err = cb_flush(cbs);
	    return err;
	  }
	}
	return CBERRALLOC;
}

int  cb_write_cbuf(CBFILE **cbs, cbuf *cbf){
	if( cbs==NULL || *cbs==NULL || cbf==NULL || (*cbf).buf==NULL ){ return CBERRALLOC; }
	return cb_write(cbs, &(*(*cbf).buf), (*cbf).contentlen);
}

int  cb_put_ch(CBFILE **cbs, unsigned char ch){ // 12.8.2013
	int err=CBSUCCESS;
	if(*cbs!=NULL)
	  if((**cbs).blk!=NULL){
cb_put_ch_put:
	    if((*(**cbs).blk).contentlen < (*(**cbs).blk).buflen ){
              (*(**cbs).blk).buf[(*(**cbs).blk).contentlen] = ch; // 12.8.2013
	      ++(*(**cbs).blk).contentlen;
	    }else if((**cbs).cf.type!=CBCFGBUFFER){ // 20.8.2013
	      err = cb_flush(cbs); // new block
	      goto cb_put_ch_put;
	    }else if((**cbs).cf.type==CBCFGBUFFER){ // 20.8.2013
	      return CBBUFFULL;
	    }
	    return err;
	  }
	return CBERRALLOC;
}

int  cb_get_ch(CBFILE **cbs, unsigned char *ch){ // Copy ch to buffer and return it until end of buffer
	char chr=' '; int err=0;
	if( cbs!=NULL && *cbs!=NULL ){
	// cb_get_ch_read_buffer
	  if( (*(**cbs).cb).index < (*(**cbs).cb).contentlen){
	     ++(*(**cbs).cb).index;
	     *ch = (*(**cbs).cb).buf[ (*(**cbs).cb).index ];
	     return CBSUCCESS;
	  }
	  // cb_get_ch_read_stream_to_buffer
	  *ch=' ';
	  // get char
	  err = cb_get_char_read_block(cbs, &chr);
	  if( err == CBSTREAMEND || err >= CBERROR ){ return err; }
	  // copy char if name-value -buffer is not full
	  if((**cbs).cb->contentlen < (**cbs).cb->buflen ){
	    if( chr != EOF ){
	      (**cbs).cb->buf[(**cbs).cb->contentlen] = chr;
	      ++(**cbs).cb->contentlen;
	      (**cbs).cb->index = (**cbs).cb->contentlen;
	      *ch = chr;
	      return CBSUCCESS;
	    }else{
	      *ch=chr;
	      (**cbs).cb->index = (**cbs).cb->contentlen;
	      return CBSTREAMEND;
	    }
	  }else{
	    *ch=chr;
	    if( chr == EOF )
	      return CBSTREAMEND;
	    if((**cbs).cb->contentlen > (**cbs).cb->buflen )
	      return CBSTREAM;
	    return err; // at edge of buffer and stream
	  }
	}
	return CBERRALLOC;
}

int  cb_get_buffer(cbuf *cbs, unsigned char **buf, int *size){ 
	int from=0, to=0;
	to = *size;
	return cb_get_buffer_range(cbs,buf,size,&from,&to);
}

// Allocates new buffer
int  cb_get_buffer_range(cbuf *cbs, unsigned char **buf, int *size, int *from, int *to){ 
	int index=0;
	if( cbs==NULL || (*cbs).buf==NULL ){ return CBERRALLOC;}
	*buf = (unsigned char *) malloc( sizeof(char)*( *size+1 ) );
	if(*buf==NULL){ fprintf(stderr, "\ncb_get_sub_buffer: malloc returned null."); return CBERRALLOC; }
	(*buf)[(*size)] = '\0';
	for(index=0;(index<(*to-*from) && index<(*size) && (index+*from)<(*cbs).contentlen); ++index){
	  (*buf)[index] = (*cbs).buf[index+*from];
	}
	*size = index;
	return CBSUCCESS;
}

