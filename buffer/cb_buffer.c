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
 x cb_put_name cb_name malloc nimen koon perusteella ennen listaan tallettamista
 (- cb_put_name ja cb_save_name_from_charbuf allokoivat edelleen kahteen kertaan, ok)
 x Nimet sisaltavan valilyonteja ja tabeja. Ohjeen mukaan ne poistetaan ennen vertausta.
   -> nimiin ei kosketa, nimen ja arvon valista voi ottaa nyt valilyonnit ja tabit pois
 x Nimimoodi ei toimi, listamoodi toimii: cat tests/testi.txt | ./cbsearch -c 1 -b 2048 -l 512 unknown
 x Jos lisaa loppuun esim. 2> /dev/null, listan loppuun tulostuu merkkeja ^@^@^@...
 x Haku nimi3 loytaa nimet nimi1 ja nimi2 mutta ei jos unfold on pois paalta.
 - bypass ja RFC 2822 viestit eivat sovi yhteen. Jos bypassin korvaa erikoismerkilla,
   sita ei saisi lukea ja ainoastaan viesti ei ole valid jos siina on erikoismerkkeja.
 - Ohjeeseen: rstart ei saa olla "unfolding" erikoismerkki
 - Unfolding erikoismerkit pitaa verrata ohjausmerkkeihin ja tarkastaa 
 - Jokainen taulukko on vain taulukko ilman pointeria ensimmaiseen?
   x jokainen malloc tarkistettu < 11/2013
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
 - 32-bittinen regexp loytaa tekstin mutta regexp erikoismerkit ja haut eivat toimi
 ===
 x cb_conf:iin myos eri hakutavat
 x listaan lisaksi aikaleima milloin viimeksi haettu
 x cb_read.c omaan hakemistoon
 x Kaikki lukufunktiot omaan hakemistoon
 - Tarkastettava viela kaikki ahead puskurin toiminnot
   - arvon loppu rajan kohdalla (ja alku) set_stream ja set_rend 
   - loppu rajan kohdalla
   - unfolding, samoja testeja
   - blokin koko, puskurin koko ja reuna-alueet  
 (- Puskurista rengaspuskuri tarvittaessa, "bias")
 x set_cursor: alloc_name vasta cb_save_name_from_charbuf:iin
   - Myos nimen pituus muuttuu kaksi kertaa, muistin kokonaiskaytto jaa pienemmaksi vaikka vahan (malloc ja kopiointi) hitaampi
 x print_leaves printtaa yhden tason liian pitkalle kokeiltaessa testiohjelmalla test_cbsearch.c
 x bugi joka on merkattu tiedostoon cb_search.c merkilla "VIRHE"
   -> tarkistus viela
 x test_cbsearch.c printtaa oksat/lehdet huonosti
 x listasta ei viela loydy oikein: cat tests/testi.txt | ./cbsearch.CBSETSTATETREE -c 4 -b 2048 -l 512 -t -s "nimi1 bb.dd unknown" 2>&1 | more
 x cat tests/testi.txt | ./cbsearch.CBSETSTATETREE -c 4 -b 2048 -l 512 -t -s "nimi1 bb.dd unknown" 2>&1 | more # dd ei loydy (hakemalla listasta, dd on cc:sta yksi oikealla, molemmat yhden vasemmalla)
   cat tests/testi.txt | ./cbsearch.CBSETSTATETREE -c 4 -b 2048 -l 512 -t -s "bb.dd unknown" 2>&1 | more # dd loytyy (hakemalla puskurista)
   cat tests/testi.txt | ./cbsearch.CBSETSTATETREE -c 4 -b 2048 -l 512 -t -s "nimi1 bb.cc unknown" 2>&1 | more # cc loytyy (seuraava oikealla)
 x bitwice cb_conf
 x Laitetaanko jokaiseen ** mallociin myos pointerin allokointi
   x ei
 - json testi, joka toinen rstart ja rend, joka toinen subrstart ja subrend
 - dup pois
 - UTF-32, UTF-16 endianness
   - BE 32 / LE 32 on vaarinpain
 - voi tehda vertausfunktion viela leksikaalisen pienempi tai suurempi kuin vertaukselle
   - joka tavu muutetaan host byte muotoon, taman lisaksi regexp on ainoa missa tata kaytetaan
     - pcre:n pattern ja subject -funktioissa ainoastaan: "host byte order",
       muutoin aina alkuperaisessa muodossa.
 - encodingbytes on mahdollista valita erikseen ja ohjelma voi jaada tilaan jossa
   se ei lue merkkeja oikein jos: pelkka alustus ja err = cb_set_encodingbytes(&cbf, 0);

 ===
 x removewsp ja removecrlf, "nelj? nimi 4"
   x removenamewsp
 */

int  cb_get_char_read_block(CBFILE **cbf, unsigned char *ch);
int  cb_set_type(CBFILE **buf, char type);
int  cb_allocate_empty_cbfile(CBFILE **str, int fd);
int  cb_get_leaf(cb_name **tree, cb_name **leaf, int count, int *left); // not tested yet 7.12.2013
int  cb_print_leaves_inner(cb_name **cbn);
int  cb_get_char_read_offset_block(CBFILE **cbf, unsigned char *ch, signed long int offset);

/*
 * Debug
 */

int  cb_print_leaves(cb_name **cbn){ 
	cb_name *ptr = NULL;
	if(cbn==NULL || *cbn==NULL)
	  return CBERRALLOC;
	ptr = &(* (cb_name*) (**cbn).leaf);
	return cb_print_leaves_inner( &ptr );
}
int  cb_print_leaves_inner(cb_name **cbn){ 
	int err = CBSUCCESS;
	cb_name *iter = NULL, *leaf = NULL;
	if(cbn==NULL || *cbn==NULL){ fprintf(stderr," null."); return err; }

	iter = &(**cbn);
	if(iter==NULL){ fprintf(stderr," null."); return CBSUCCESS; }
        if( iter!=NULL ){ // Name
	  // Self
	  if(iter!=NULL){
	    if((*iter).namelen!=0){
	      fprintf(stderr,"\"");
	      cb_print_ucs_chrbuf( &(*iter).namebuf, (*iter).namelen, (*iter).buflen); // PRINT NAME
	      fprintf(stderr,"\"");
	      if( (*iter).leaf==NULL && (*iter).next!=NULL)
	        fprintf(stderr,",");
	      else if( (*iter).leaf!=NULL)
	        fprintf(stderr,":");
	    }
	  }
	  // Left
	  if( (*iter).leaf != NULL ){
	    leaf = &(* (cb_name*) (*iter).leaf);
	    fprintf(stderr,"{");
	    err = cb_print_leaves_inner( &leaf ); // LEFT
	    fprintf(stderr,"}");
	  }
	  // Right
	  if( (*iter).next != NULL ){
	    leaf = &(* (cb_name*) (*iter).next);
	    err = cb_print_leaves_inner( &leaf ); // RIGHT
	  }
	}
	return err;
}

int  cb_print_name(cb_name **cbn){ 
	int err=CBSUCCESS;
	if( cbn==NULL || *cbn==NULL) return CBERRALLOC;

        fprintf(stderr, " name [" );
	if( (**cbn).namebuf!=NULL && (**cbn).buflen>0 )
          cb_print_ucs_chrbuf(&(**cbn).namebuf, (**cbn).namelen, (**cbn).buflen);
        //fprintf(stderr, "] offset [%lli] length [%i]", (**cbn).offset, (**cbn).length);
        fprintf(stderr, "] offset [%li] length [%i]", (**cbn).offset, (**cbn).length); // 6.12.2014
        //fprintf(stderr, " nameoffset [%lli]\n", (**cbn).nameoffset);
        fprintf(stderr, " nameoffset [%li]\n", (**cbn).nameoffset);
        fprintf(stderr, " matchcount [%li]", (**cbn).matchcount);
        fprintf(stderr, " first found [%li] (seconds)", (signed long int) (**cbn).firsttimefound);
        fprintf(stderr, " last used [%li]\n", (**cbn).lasttimeused);
        return err;
}
int  cb_print_names(CBFILE **str){ 
	cb_name *iter = NULL; int names=0;
	fprintf(stderr, "\n cb_print_names: \n");
	if(str!=NULL){
	  if( *str !=NULL){
            iter = &(*(*(**str).cb).list.name);
            if(iter!=NULL){
              do{
	        ++names;
	        fprintf(stderr, " [%i/%li]", names, (*(**str).cb).list.namecount ); // [%.2i/%.2li]
	        if(iter!=NULL)
	          cb_print_name( &iter);
	        if( (**str).cf.searchstate==CBSTATETREE ){
	          fprintf(stderr, "         tree: ");
	          cb_print_leaves( &iter );
	          fprintf(stderr, "\n");
	        }
                iter = &(* (cb_name *) (*iter).next );
              }while( iter != NULL );
              return CBSUCCESS;
	    }else{
	      fprintf(stderr, "\n namelist was empty");
	    }
	  }else{
	    fprintf(stderr, "\n *str was null "); 
	  }
	}else{
	  fprintf(stderr, "\n str was null "); 
	}
        return CBERRALLOC;
}
// Debug
void cb_print_counters(CBFILE **cbf){
        if(cbf!=NULL && *cbf!=NULL)
          fprintf(stderr, "\nnamecount:%li \t index:%li \t contentlen:%li \t  buflen:%li", (*(**cbf).cb).list.namecount, \
             (*(**cbf).cb).index, (*(**cbf).cb).contentlen, (*(**cbf).cb).buflen );
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
	  (**to).next   = &(* (cb_name*) (**from).next); // This knows where this points to, so it only can be copied but references to this can not
	  (**to).leaf   = &(* (cb_name*) (**from).leaf); // 2.12.2013, 9.12.2013
          (**to).firsttimefound = (**from).firsttimefound; // 2.12.2013
          (**to).lasttimeused = (**from).lasttimeused; // 2.12.2013
	  return CBSUCCESS;
	}
	return CBERRALLOC;
}

int  cb_allocate_name(cb_name **cbn, int namelen){ 
	int indx=0;
	*cbn = (cb_name*) malloc(sizeof(cb_name));
	if(*cbn==NULL)
	  return CBERRALLOC;
	(**cbn).namebuf = (unsigned char*) malloc( sizeof(char)*( (unsigned int) namelen+1) ); // 7.12.2013
	if((**cbn).namebuf==NULL)
	  return CBERRALLOC;
	for(indx=0;indx<namelen && indx<CBNAMEBUFLEN;++indx) // 7.12.2013
	  (**cbn).namebuf[indx]=' ';
	(**cbn).namebuf[namelen]='\0'; // 7.12.2013
	(**cbn).buflen=namelen; // 7.12.2013
	(**cbn).namelen=0;
	(**cbn).offset=0; 
	//(**cbn).length=0;
	(**cbn).length=-1; // 11.12.2014
	(**cbn).matchcount=0;
	(**cbn).firsttimefound=-1;
	(**cbn).lasttimeused=-1;
	(**cbn).next=NULL;
	(**cbn).leaf=NULL;
	return CBSUCCESS;
}

int  cb_set_cstart(CBFILE **str, unsigned long int cstart){ // comment start
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).cf.cstart=cstart;
	return CBSUCCESS;
}
int  cb_set_cend(CBFILE **str, unsigned long int cend){ // comment end
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).cf.cend=cend;
	return CBSUCCESS;
}
int  cb_set_rstart(CBFILE **str, unsigned long int rstart){ // value start
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).cf.rstart=rstart;
	return CBSUCCESS;
}
int  cb_set_rend(CBFILE **str, unsigned long int rend){ // value end
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).cf.rend=rend;
	return CBSUCCESS;
}
int  cb_set_bypass(CBFILE **str, unsigned long int bypass){ // bypass , new - added 19.12.2009
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).cf.bypass=bypass;
	return CBSUCCESS;
}
int  cb_set_search_state(CBFILE **str, char state){
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	if( state==CBSTATETREE || state==CBSTATELESS || state==CBSTATEFUL || state==CBSTATETOPOLOGY )
		(**str).cf.searchstate = (**str).cf.searchstate;
	else
		fprintf(stderr, "\nState not known.");
	return CBSUCCESS;
}
int  cb_set_subrstart(CBFILE **str, unsigned long int subrstart){ // sublist value start
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).cf.subrstart=subrstart;
	return CBSUCCESS;
}
int  cb_set_subrend(CBFILE **str, unsigned long int subrend){ // sublist value end
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	(**str).cf.subrend=subrend;
	return CBSUCCESS;
}

int  cb_allocate_cbfile(CBFILE **str, int fd, int bufsize, int blocksize){
	unsigned char *blk = NULL; 
	return cb_allocate_cbfile_from_blk(str, fd, bufsize, &blk, blocksize);
}

int  cb_allocate_empty_cbfile(CBFILE **str, int fd){ 
	int err = CBSUCCESS;
	*str = (CBFILE*) malloc(sizeof(CBFILE));
	if(*str==NULL)
	  return CBERRALLOC;
	(**str).cb = NULL; (**str).blk = NULL;
        (**str).cf.type=CBCFGSTREAM; // default
        //(**str).cf.type=CBCFGFILE; // first test was ok
	(**str).cf.searchstate=CBSTATETOPOLOGY;
	//(**str).cf.doubledelim=1; // default
	(**str).cf.doubledelim=0; // test
        (**str).cf.searchmethod=CBSEARCHNEXTNAMES; // default
        //(**str).cf.searchmethod=CBSEARCHUNIQUENAMES;
#ifdef CB2822MESSAGE
	(**str).cf.asciicaseinsensitive=1;
	(**str).cf.unfold=1;
	(**str).cf.removewsp=1; // test
	(**str).cf.removecrlf=1; // test
	//(**str).cf.removewsp=0; // default
	//(**str).cf.removecrlf=0; // default
	(**str).cf.rfc2822headerend=1; // default, stop at headerend
	//(**str).cf.rstart=0x00003A; // ':', default
	//(**str).cf.rend=0x00000A;   // LF, default
	(**str).cf.rstart=CBRESULTSTART; // tmp
	(**str).cf.rend=CBRESULTEND; // tmp
	(**str).cf.searchstate=CBSTATEFUL;
#else
	(**str).cf.asciicaseinsensitive=0; // default
	//(**str).cf.asciicaseinsensitive=1; // test
	(**str).cf.unfold=0;
	(**str).cf.removewsp=1;
	(**str).cf.removecrlf=1;
	(**str).cf.rfc2822headerend=0;
	(**str).cf.rstart=CBRESULTSTART;
	(**str).cf.rend=CBRESULTEND;
#endif
	(**str).cf.removenamewsp=0;
	(**str).cf.bypass=CBBYPASS;
	(**str).cf.cstart=CBCOMMENTSTART;
	(**str).cf.cend=CBCOMMENTEND;
	(**str).cf.subrstart=CBSUBRESULTSTART;
	(**str).cf.subrend=CBSUBRESULTEND;
	(**str).cf.searchstate=CBSTATELESS;
#ifdef CBSETSTATEFUL
	(**str).cf.searchstate=CBSTATEFUL;
#endif
#ifdef CBSETSTATETOPOLOGY
	(**str).cf.searchstate=CBSTATETOPOLOGY;
#endif
#ifdef CBSETSTATELESS
	(**str).cf.searchstate=CBSTATELESS;
#endif
#ifdef CBSETSTATETREE
	(**str).cf.searchstate=CBSTATETREE;
#endif
	(**str).cf.json=0;
#ifdef CBSETJSON
	(**str).cf.json=1;
#endif
	(**str).cf.leadnames=0; // default
	//(**str).cf.leadnames=1; // test
	(**str).encodingbytes=CBDEFAULTENCODINGBYTES;
	(**str).encoding=CBDEFAULTENCODING;
	(**str).fd = dup(fd);
	if((**str).fd == -1){ err = CBERRFILEOP; (**str).cf.type=CBCFGBUFFER; } // 20.8.2013

	cb_fifo_init_counters( &(**str).ahd );
	return err;
}

int  cb_allocate_cbfile_from_blk(CBFILE **str, int fd, int bufsize, unsigned char **blk, int blklen){ 
	int err = CBSUCCESS;
	err = cb_allocate_empty_cbfile(&(*str), fd);
	if(err!=CBSUCCESS){ return err; }
	err = cb_allocate_buffer(&(**str).cb, bufsize);
	if(err!=CBSUCCESS){ return err; }
	if(*blk==NULL){
	  err = cb_allocate_buffer(&(**str).blk, blklen);
	}else{
	  err = cb_allocate_buffer(&(**str).blk, 0); // blk
	  if(err==-1){ return CBERRALLOC;}
	  free( (*(**str).blk).buf );
	  (*(**str).blk).buf = &(**blk);
	  (*(**str).blk).buflen = (long) blklen;
	  (*(**str).blk).contentlen = (long) blklen;
	}
	if(err==-1){ return CBERRALLOC;}
	return CBSUCCESS;
}

int  cb_allocate_buffer(cbuf **cbf, int bufsize){ 
	int indx=0;
	*cbf = (cbuf *) malloc(sizeof(cbuf));
	if(cbf==NULL)
	  return CBERRALLOC;
	(**cbf).buf = (unsigned char *) malloc(sizeof(char)*( (unsigned int) bufsize+1));
	if( (**cbf).buf == NULL )
	  return CBERRALLOC;

	for(indx=0;indx<bufsize;++indx)
	  (**cbf).buf[indx]=' ';
	(**cbf).buf[bufsize]='\0';
	(**cbf).buflen=bufsize;
	(**cbf).contentlen=0;
	(**cbf).list.namecount=0;
	(**cbf).index=0;
        (**cbf).offsetrfc2822=0;
	(**cbf).list.name=NULL;
	(**cbf).list.current=NULL;
	(**cbf).list.last=NULL;
	(**cbf).list.currentleaf=NULL; // 11.12.2013
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
	  free( (*(**buf).cb).buf ); // free buffer data
	free((**buf).cb); // free buffer
	if((*(**buf).blk).buf!=NULL)
          free((*(**buf).blk).buf); // free block data
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

int  cb_free_name(cb_name **name){
        int err=CBSUCCESS;
	cb_name *leaf = NULL;
	cb_name *next = NULL;
	cb_name *nextleaf = NULL;
	cb_name *nextname = NULL;

        if(name!=NULL && *name!=NULL){
	  leaf = &(* (cb_name*) (**name).leaf);
	  while( leaf!=NULL ){
	    nextleaf = &(* (cb_name*) (*leaf).leaf);
	    next = &(* (cb_name*) (*leaf).next);
            cb_free_name(&leaf);
	    while( next!=NULL ){
	      nextname = &(* (cb_name*) (*next).next);
	      free( (*next).namebuf ); free(next);
	      next = &(* nextname);
	    }
            leaf = &(* nextleaf);
	  }
	  free( (**name).namebuf );
	  (**name).namebuf=NULL; (**name).namelen=0; (**name).buflen=0; // (**name).leafcount=0;
	  /* Set last name to NULL */
	  free(*name); *name=NULL; 
        }else
	  return CBERRALLOC;
	return err;

}
int  cb_reinit_buffer(cbuf **buf){ // free names and init
	if(buf!=NULL && *buf!=NULL){
	  (**buf).index=0;
	  (**buf).contentlen=0;
	  cb_empty_names(&(*buf));
	}
	(**buf).list.name=NULL; // 1.6.2013
	(**buf).list.current=NULL; // 1.6.2013
	(**buf).list.currentleaf=NULL; // 11.12.2013
	(**buf).list.last=NULL; // 1.6.2013
	return CBSUCCESS;
}
/*
 * Used only with CBCFGWRITABLEFILE (seekable) to clear the block before 
 * writing in between with offset and after. 
 */
int  cb_empty_block(CBFILE **buf){
	if(buf==NULL || *buf==NULL || (**buf).blk==NULL ){ return CBERRALLOC; }
	(*(**buf).blk).contentlen = 0;
	(*(**buf).blk).index = 0;
	return CBSUCCESS;
}
int  cb_empty_names(cbuf **buf){
	int err=CBSUCCESS;
	err = cb_empty_names_from_name( &(*buf), &( *(**buf).list.name ) );
	(**buf).list.namecount = 0; // namecount of main list (leafs are not counted)
	return err;
}
int  cb_empty_names_from_name(cbuf **buf, cb_name *cbn){
	int err=CBSUCCESS;
	cb_name *name = NULL;
	cb_name *nextname = NULL;
	if( buf==NULL || *buf==NULL ){ return CBERRALLOC; }
	name = cbn;
	while(name != NULL){
	  nextname = &(* (cb_name*) (*name).next);
	  /* Free the name and all its leafs */
          cb_free_name(&name);
	  name = &(* nextname);
	}
	free(name); free(nextname);
	/* To be sure the name was not a leaf, count the names again. */
	name = (**buf).list.name;
	err = 0;
	while(name != NULL){
		nextname = &(* (cb_name*) (*name).next);
		name = &(* nextname);
		++err;
	}
	(**buf).list.namecount = err;
	return CBSUCCESS;
}

int  cb_use_as_buffer(CBFILE **buf){
        return cb_set_type(&(*buf), (char) CBCFGBUFFER);
}
int  cb_use_as_writable_file(CBFILE **buf){
	cb_use_as_file( &(*buf) );
	return cb_set_type(&(*buf), (char) CBCFGWRITABLEFILE);
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
	    fprintf(stderr,"\nSetting type to %i.", (int) type);
	    (**buf).cf.type = type; // 20.8.2013
	    return CBSUCCESS;
	  }
	}
	return CBERRALLOC;
}

int  cb_get_char_read_block(CBFILE **cbf, unsigned char *ch){ 
	if( cbf==NULL || *cbf==NULL || ch==NULL ){ return CBERRALLOC; }
	return cb_get_char_read_offset_block( &(*cbf), &(*ch), -1);
}
int  cb_get_char_read_offset_block(CBFILE **cbf, unsigned char *ch, signed long int offset){ 
	ssize_t sz=0; // int err=0;
	cblk *blk = NULL; 
	blk = (**cbf).blk;
	if( cbf==NULL || *cbf==NULL || ch==NULL ){ return CBERRALLOC; }
	if( offset > 0 && (**cbf).cf.type!=CBCFGWRITABLEFILE ){
		fprintf(stderr,"\ncb_get_char_read_offset_block: attempt to seek to offset of unwritable (unseekable) file (CBCFGWRITABLEFILE is not set).");
		return CBOPERATIONNOTALLOWED;
	}

	if(blk!=NULL){
	  if( ( (*blk).index < (*blk).contentlen ) && ( (*blk).contentlen <= (*blk).buflen ) ){
	    // return char
	    *ch = (*blk).buf[(*blk).index];
	    ++(*blk).index;
	  }else if((**cbf).cf.type!=CBCFGBUFFER){ // 20.8.2013
	    // read a block and return char
	    /*
	     * If write-operations are wanted in between file, the next is
	     * the only available option. Block has to be emptied before and
	     * after read or write. After write, flush. Buffers index and
	     * contentlength has to be set to current offset after every
	     * write.
	     */
	    if( (**cbf).cf.type==CBCFGWRITABLEFILE ){
	       sz = pread((**cbf).fd, (*blk).buf, (size_t)(*blk).buflen, (*(**cbf).cb).index ); // read again from index
	    }else{
	       sz = read((**cbf).fd, (*blk).buf, (size_t)(*blk).buflen);
	    }
	    (*blk).contentlen = (long int) sz; // 6.12.2014
	    if( 0 < (int) sz ){ // read more than zero bytes
	      (*blk).contentlen = (long int) sz; // 6.12.2014
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
	if( cbs==NULL || *cbs==NULL ){ return CBERRALLOC; }
	return cb_flush_to_offset( &(*cbs), -1 );
}
int  cb_flush_to_offset(CBFILE **cbs, signed long int offset){
	int err=CBSUCCESS;
	long long int lerr=CBSUCCESS;
	if( cbs==NULL || *cbs==NULL ){ return CBERRALLOC; }

	if( *cbs!=NULL ){
 	  if((**cbs).cf.type!=CBCFGBUFFER){
	    if((**cbs).blk!=NULL){
	      if((*(**cbs).blk).contentlen <= (*(**cbs).blk).buflen ){
	        if( offset > 0 && (**cbs).cf.type!=CBCFGWRITABLEFILE ){
			fprintf(stderr,"\ncb_flush_to_offset: attempt to seek to offset of unwritable file (CBCFGWRITABLEFILE is not set).");
			return CBOPERATIONNOTALLOWED;
		}
		if( offset < 0 ){ // Append (usual)
	          err = (int) write((**cbs).fd, (*(**cbs).blk).buf, (size_t) (*(**cbs).blk).contentlen);
		}else{ // Write (replace) - seek capable filedescriptor, offset function called
	          err = (int) pwrite((**cbs).fd, (*(**cbs).blk).buf, (size_t) (*(**cbs).blk).contentlen, (off_t) offset );
		  if(err>=0)
		    lerr = lseek( (**cbs).fd, (off_t) 0, SEEK_END ); // set cursor to append again
		}
	      }else{
		if( offset < 0 ){ // Append (usual)
	          err = (int) write((**cbs).fd, (*(**cbs).blk).buf, (size_t) (*(**cbs).blk).buflen);
		}else{ // Write (replace) - seek capable filedescriptor, offset function called
	          err = (int) pwrite((**cbs).fd, (*(**cbs).blk).buf, (size_t) (*(**cbs).blk).buflen, (off_t) offset );
		  if(err>=0)
		    lerr = lseek( (**cbs).fd, (off_t) 0, SEEK_END ); // set cursor to append again
		}
	      }
	      if(err<0){
	        err = CBERRFILEWRITE ; 
	      }else{
	        err = CBSUCCESS;
	        (*(**cbs).blk).contentlen=0; // Block is set to zero here (write or append is not possible without this)
	      }
	      return err;
	    }
	  }else
	    return CBUSEDASBUFFER;
	}
	return CBERRALLOC;
}

int  cb_write(CBFILE **cbs, unsigned char *buf, long int size){ 
	int err=0;
	long int indx=0;
	if( cbs!=NULL && *cbs!=NULL && buf!=NULL){
	  if((**cbs).blk!=NULL){
	    for(indx=0; indx<size; ++indx){
	      //err = cb_put_ch(cbs,buf[indx]); 
	      err = cb_put_ch( &(*cbs), buf[indx]); // 10.12.2014
	    }
	    //err = cb_flush(cbs);
	    err = cb_flush( &(*cbs) ); // 10.12.2014
	    return err;
	  }
	}
	return CBERRALLOC;
}

int  cb_write_cbuf(CBFILE **cbs, cbuf *cbf){
	if( cbs==NULL || *cbs==NULL || cbf==NULL || (*cbf).buf==NULL ){ return CBERRALLOC; }
	//return cb_write( cbs, &(*(*cbf).buf), (*cbf).contentlen);
	return cb_write( &(*cbs), &(*(*cbf).buf), (*cbf).contentlen); // 10.12.2014
}

/*
 * This is append only function. Block has to be used individually to
 * write in arbitrary location in a file (with offset). 
 */
int  cb_put_ch(CBFILE **cbs, unsigned char ch){ // 12.8.2013
	int err=CBSUCCESS;
	if(cbs==NULL || *cbs==NULL){ return CBERRALLOC; }
	if((**cbs).blk!=NULL){
cb_put_ch_put:
	  if((*(**cbs).blk).contentlen < (*(**cbs).blk).buflen ){
            (*(**cbs).blk).buf[(*(**cbs).blk).contentlen] = ch; // 12.8.2013
	    ++(*(**cbs).blk).contentlen;
	  }else if((**cbs).cf.type!=CBCFGBUFFER){ // 20.8.2013
	    //err = cb_flush(cbs); // new block	
	    err = cb_flush( &(*cbs) ); // new block, 10.12.2014
	    goto cb_put_ch_put;
	  }else if((**cbs).cf.type==CBCFGBUFFER){ // 20.8.2013
	    return CBBUFFULL;
	  }
	  return err;
	}
	return CBERRALLOC;
}

int  cb_get_ch(CBFILE **cbs, unsigned char *ch){ // Copy ch to buffer and return it until end of buffer
	unsigned char chr=' '; int err=0; 
	if( cbs!=NULL && *cbs!=NULL ){

	  if( (*(**cbs).cb).index < (*(**cbs).cb).contentlen){
	     ++(*(**cbs).cb).index;
	     *ch = (*(**cbs).cb).buf[ (*(**cbs).cb).index ];
	     return CBSUCCESS;
	  }
	  *ch=' ';
	  // get char
	  err = cb_get_char_read_block(cbs, &chr);
	  if( err == CBSTREAMEND || err >= CBERROR ){ return err; }
	  // copy char if name-value -buffer is not full
	  if((*(**cbs).cb).contentlen < (*(**cbs).cb).buflen ){
	    if( chr != (unsigned char) EOF ){
	      (*(**cbs).cb).buf[(*(**cbs).cb).contentlen] = chr;
	      ++(*(**cbs).cb).contentlen;
	      (*(**cbs).cb).index = (*(**cbs).cb).contentlen;
	      *ch = chr;
	      return CBSUCCESS;
	    }else{
	      *ch=chr;
	      (*(**cbs).cb).index = (*(**cbs).cb).contentlen;
	      return CBSTREAMEND;
	    }
	  }else{
	    *ch=chr;
	    if( chr == (unsigned char) EOF )
	      return CBSTREAMEND;
	    if((*(**cbs).cb).contentlen > (*(**cbs).cb).buflen )
	      return CBSTREAM;
	    return err; // at edge of buffer and stream
	  }
	}
	return CBERRALLOC;
}

/*
 * CBFILE can be used as a block: set buffer size to 0.
 * These functions can be used to set and get blocks. The block
 * has to be in the encoding of the CBFILE.
 */

int  cb_free_cbfile_get_block(CBFILE **cbf, unsigned char **blk, int *blklen, int *contentlen){
	if( blklen==NULL || blk==NULL || *blk==NULL || cbf==NULL || *cbf==NULL || (**cbf).blk==NULL ){ return CBERRALLOC;}
	(*blk) = &(*(**cbf).blk).buf[0];
	(*(**cbf).blk).buf = NULL;
	*contentlen = (*(**cbf).blk).contentlen;
	*blklen = (*(**cbf).blk).buflen;
	return cb_free_cbfile( cbf );
}

int  cb_get_buffer(cbuf *cbs, unsigned char **buf, long int *size){ 
        long int from=0, to=0;
        to = *size;
        return cb_get_buffer_range(cbs,buf,size,&from,&to);
}

// Allocates new buffer (or a block if cblk)
int  cb_get_buffer_range(cbuf *cbs, unsigned char **buf, long int *size, long int *from, long int *to){ 
        long int index=0;
        if( cbs==NULL || (*cbs).buf==NULL ){ return CBERRALLOC;}
        *buf = (unsigned char *) malloc( sizeof(char)*( (unsigned long int)  *size+1 ) );
        if(*buf==NULL){ fprintf(stderr, "\ncb_get_sub_buffer: malloc returned null."); return CBERRALLOC; }
        (*buf)[(*size)] = '\0';
        for(index=0;(index<(*to-*from) && index<(*size) && (index+*from)<(*cbs).contentlen); ++index){
          (*buf)[index] = (*cbs).buf[index+*from];
        }
        *size = index;
        return CBSUCCESS;
}

