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
#include <errno.h>  // errno
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
 x space/tab ei saa esiintya rfc2822:n nimissa, arvon lopetusmerkiksi?
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
 x dup pois
 - UTF-32, UTF-16 endianness
   - BE 32 / LE 32 on vaarinpain
 x voi tehda vertausfunktion viela leksikaalisen pienempi tai suurempi kuin vertaukselle
   - joka tavu muutetaan host byte muotoon, taman lisaksi regexp on ainoa missa tata kaytetaan
     - pcre:n pattern ja subject -funktioissa ainoastaan: "host byte order",
       muutoin aina alkuperaisessa muodossa.
 - encodingbytes on mahdollista valita erikseen ja ohjelma voi jaada tilaan jossa
   se ei lue merkkeja oikein jos: pelkka alustus ja err = cb_set_encodingbytes(&cbf, 0);
 x test_cb namelist is in error 22.2.2015 
   x cb_free_name correct order

 ===
 x removewsp ja removecrlf, "nelj? nimi 4"
   x removenamewsp
 */

int  cb_get_char_read_block(CBFILE **cbf, unsigned char *ch);
int  cb_set_type(CBFILE **buf, unsigned char type);
int  cb_allocate_empty_cbfile(CBFILE **str, int fd);
int  cb_get_leaf(cb_name **tree, cb_name **leaf, int count, int *left); // not tested yet 7.12.2013
int  cb_print_leaves_inner(cb_name **cbn, char priority);
int  cb_get_char_read_offset_block(CBFILE **cbf, unsigned char *ch, signed long int offset);
int  cb_allocate_buffer_from_blk(cbuf **cbf, unsigned char **blk, int blksize);
int  cb_init_buffer_from_blk(cbuf **cbf, unsigned char **blk, int blksize);
int  cb_flush_cbfile_to_offset(cbuf **cb, int fd, signed long int offset);

/*
 * Debug
 */

int cb_print_conf(CBFILE **str, char priority){
	if(str==NULL || *str==NULL){ cb_log(&(*str), CBLOGALERT, "\ncb_print_conf: str was null."); return CBERRALLOC; }
	cb_log(&(*str), priority, "\ntype:            \t0x%.2X", (**str).cf.type);
	cb_log(&(*str), priority, "\nsearchmethod:    \t0x%.2X", (**str).cf.searchmethod);
	cb_log(&(*str), priority, "\nunfold:          \t0x%.2X", (**str).cf.unfold);
	cb_log(&(*str), priority, "\nasciicaseinsensitive: \t0x%.2X", (**str).cf.asciicaseinsensitive);
	cb_log(&(*str), priority, "\nrfc2822headerend: \t0x%.2X", (**str).cf.rfc2822headerend);
	cb_log(&(*str), priority, "\nremovewsp:        \t0x%.2X", (**str).cf.removewsp);
	cb_log(&(*str), priority, "\nremovecrlf:       \t0x%.2X", (**str).cf.removecrlf);
	cb_log(&(*str), priority, "\nremovenamewsp:    \t0x%.2X", (**str).cf.removenamewsp);
	cb_log(&(*str), priority, "\nleadnames:        \t0x%.2X", (**str).cf.leadnames);
	cb_log(&(*str), priority, "\njson:             \t0x%.2X", (**str).cf.json);
	cb_log(&(*str), priority, "\ndoubledelim:      \t0x%.2X", (**str).cf.doubledelim);
	cb_log(&(*str), priority, "\nsearchstate:      \t0x%.2X\n", (**str).cf.searchstate);
	return CBSUCCESS;
}

int  cb_print_leaves(cb_name **cbn, char priority){ 
	cb_name *ptr = NULL;
	if(cbn==NULL || *cbn==NULL)
	  return CBERRALLOC;
	ptr = &(* (cb_name*) (**cbn).leaf);
	return cb_print_leaves_inner( &ptr, priority );
}
int  cb_print_leaves_inner(cb_name **cbn, char priority){ 
	int err = CBSUCCESS;
	cb_name *iter = NULL, *leaf = NULL;
	if(cbn==NULL || *cbn==NULL){ 
	  //cb_clog( CBLOGDEBUG, "\ncb_print_leaves_inner: cbn was null."); 
	  return CBSUCCESS; 
	}

	iter = &(**cbn);
	if(iter==NULL){ cb_clog( CBLOGINFO, "\ncb_print_leaves_inner: iter was null."); return CBSUCCESS; }
        if( iter!=NULL ){ // Name
	  // Self
	  if(iter!=NULL){
	    if((*iter).namelen!=0){
	      cb_clog( priority, "\"");
	      cb_print_ucs_chrbuf( priority, &(*iter).namebuf, (*iter).namelen, (*iter).buflen); // PRINT NAME
	      cb_clog( priority, "\"");
	      if( (*iter).leaf==NULL && (*iter).next!=NULL)
	        cb_clog( priority, ",");
	      else if( (*iter).leaf!=NULL)
	        cb_clog( priority, ":");
	    }
	  }
	  // Left
	  if( (*iter).leaf != NULL ){
	    leaf = &(* (cb_name*) (*iter).leaf);
	    cb_clog( priority, "{");
	    err = cb_print_leaves_inner( &leaf, priority ); // LEFT
	    cb_clog( priority, "}");
	  }
	  // Right
	  if( (*iter).next != NULL ){
	    leaf = &(* (cb_name*) (*iter).next);
	    err = cb_print_leaves_inner( &leaf, priority ); // RIGHT
	  }
	}
	return err;
}

int  cb_print_name(cb_name **cbn, char priority){ 
	int err=CBSUCCESS;
	if( cbn==NULL || *cbn==NULL) return CBERRALLOC;

        cb_clog( priority, " name [" );
	if( (**cbn).namebuf!=NULL && (**cbn).buflen>0 )
          cb_print_ucs_chrbuf( priority, &(**cbn).namebuf, (**cbn).namelen, (**cbn).buflen);
        cb_clog( priority, "] offset [%li] length [%i]", (**cbn).offset, (**cbn).length); // 6.12.2014
        cb_clog( priority, " nameoffset [%li]\n", (**cbn).nameoffset);
        cb_clog( priority, " matchcount [%li]", (**cbn).matchcount);
        cb_clog( priority, " first found [%li] (seconds)", (signed long int) (**cbn).firsttimefound);
        cb_clog( priority, " last used [%li]\n", (**cbn).lasttimeused);
        return err;
}
int  cb_print_names(CBFILE **str, char priority){ 
	cb_name *iter = NULL; int names=0;
	cb_log( &(*str), priority, "\n cb_print_names: \n");
	if(str!=NULL){
	  if( *str !=NULL){
            iter = &(*(*(**str).cb).list.name);
            if(iter!=NULL){
              do{
	        ++names;
	        cb_log( &(*str), priority, " [%i/%li]", names, (*(**str).cb).list.namecount ); // [%.2i/%.2li]
	        if(iter!=NULL)
	          cb_print_name( &iter, priority);
	        if( (**str).cf.searchstate==CBSTATETREE ){
	          cb_log( &(*str), priority, "         tree: ");
	          cb_print_leaves( &iter, priority );
	          cb_log( &(*str), priority, "\n");
	        }
                iter = &(* (cb_name *) (*iter).next );
              }while( iter != NULL );
              return CBSUCCESS;
	    }else{
	      cb_log( &(*str), priority, "\n namelist was empty");
	    }
	  }else{
	    cb_log( &(*str), priority, "\n *str was null "); 
	  }
	}else{
	  cb_log( &(*str), priority, "\n str was null "); 
	}
        return CBERRALLOC;
}
// Debug
void cb_print_counters(CBFILE **cbf, char priority){
        if(cbf!=NULL && *cbf!=NULL){
          cb_log( &(*cbf), priority, "\nnamecount:%li \t index:%li \t contentlen:%li \t  buflen:%li ", \
	    (*(**cbf).cb).list.namecount, (*(**cbf).cb).index, (*(**cbf).cb).contentlen, (*(**cbf).cb).buflen );
	  cb_log( &(*cbf), priority, "\nreadlength:%li \t contentlen:%li \t maxlength:%li \n", (*(**cbf).cb).readlength, (*(**cbf).cb).contentlen, (*(**cbf).cb).maxlength );
	  if( (*(**cbf).cb).list.name==NULL ){  cb_log( &(*cbf), priority, "name was null, \t");  }else{  cb_log( &(*cbf), priority, "name was not null, \t"); }
	  if( (*(**cbf).cb).list.last==NULL ){  cb_log( &(*cbf), priority, "last was null, \t");  }else{  cb_log( &(*cbf), priority, "last was not null, \t"); }
	  if( (*(**cbf).cb).list.current==NULL ){  cb_log( &(*cbf), priority, "\ncurrent was null, \t");  }else{  cb_log( &(*cbf), priority, "\ncurrent was not null, \t"); }
	  if( (*(**cbf).cb).list.currentleaf==NULL ){  cb_log( &(*cbf), priority, "currentleaf was null, \t");  }else{  cb_log( &(*cbf), priority, "currentleaf was not null, \t"); }
	}
}

int  cb_copy_name( cb_name **from, cb_name **to ){
	int index=0;
	if( from!=NULL && *from!=NULL && to!=NULL && *to!=NULL ){
	  for(index=0; index<(**from).namelen && index<(**to).buflen ; ++index)
	    (**to).namebuf[index] = (**from).namebuf[index];
	  (**to).namelen = index;
	  (**to).offset  = (**from).offset;
          (**to).length  = (**from).length;
	  (**to).nameoffset = (**from).nameoffset;
          (**to).matchcount = (**from).matchcount;
	  (**to).next   = &(* (cb_name*) (**from).next);
	  (**to).leaf   = &(* (cb_name*) (**from).leaf);
          (**to).firsttimefound = (**from).firsttimefound;
          (**to).lasttimeused = (**from).lasttimeused;
	  return CBSUCCESS;
	}
	return CBERRALLOC;
}

int  cb_allocate_name(cb_name **cbn, int namelen){ 
	if( cbn==NULL ){
	  cb_clog( CBLOGDEBUG, "\ncb_allocate_name: parameter cbn was null.");
	  return CBERRALLOC;
	}
	*cbn = (cb_name*) malloc(sizeof(cb_name));
	if( *cbn==NULL){
	  cb_clog( CBLOGDEBUG, "\ncb_allocate_name: malloc error, CBERRALLOC.");
	  return CBERRALLOC;
	}
	(**cbn).namebuf = (unsigned char*) malloc( sizeof(char)*( (unsigned int) namelen+1) ); // 7.12.2013
	if((**cbn).namebuf==NULL){
	  cb_clog( CBLOGDEBUG, "\ncb_allocate_name: malloc CBERRALLOC." );
	  return CBERRALLOC;
	}

	//for(indx=0;indx<namelen && indx<CBNAMEBUFLEN;++indx) // 7.12.2013
	//  (**cbn).namebuf[indx]=' ';

	memset( &(**cbn).namebuf[0], 0x20, (size_t) namelen ); // 22.2.2015

	(**cbn).namebuf[namelen]='\0'; // 7.12.2013
	(**cbn).buflen=namelen; // 7.12.2013
	(**cbn).namelen=0;
	(**cbn).offset=0; 
	(**cbn).nameoffset=0;
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
int  cb_set_search_state(CBFILE **str, unsigned char state){
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	if( state==CBSTATETREE || state==CBSTATELESS || state==CBSTATEFUL || state==CBSTATETOPOLOGY )
		(**str).cf.searchstate = state;
	else
		cb_log( &(*str), CBLOGWARNING, "\nState not known.");
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

int  cb_set_to_json( CBFILE **str ){
	if(str==NULL || *str==NULL){ return CBERRALLOC; }
	/*
	 * Tree structure of JSON is made with object curls. */
	cb_set_rstart( &(*str), (unsigned long int) ':'  );
	cb_set_rend( &(*str), (unsigned long int) ',' );
	/*
	 * Value from ':' to ',' or to '}' - arrays, (strings, numbers 
	 * and literals) are considered here as a value. With setting
	 * 'json=1', '}' changes value even if, ',' was missing. See ascii 
	 * diagram in function cb_put_leaf . */
	cb_set_subrstart( &(*str), (unsigned long int) '{' ); // object start
	cb_set_subrend( &(*str), (unsigned long int) '}' ); // object end
	(**str).cf.json = 1;
	(**str).cf.jsonnamecheck = 1; // check name before saving it to list or tree
	(**str).cf.doubledelim = 1; 
	(**str).cf.removecrlf = 1; // remove cr and lf character between value and name
	(**str).cf.removewsp = 1; // remove linear white space characters between value and name
	// (**str).cf.leadnames = 1; // leadnames are not in effect in CBSTATETREE
	/*
	 * JSON is Unicode encoded. */
	cb_set_encoding( &(*str), CBENCUTF8 );
	/*
	 * Arrays commas ',' are bypassed by setting array as a comment.
	 * This way array value can be read later and it wont interfere
	 * with values commas. */
	cb_set_cstart( &(*str), (unsigned long int) '[' );
	cb_set_cend( &(*str), (unsigned long int) ']' );
	/*
	 * Tree. */
	cb_set_search_state( &(*str), CBSTATETREE );

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
	(**str).cf.logpriority=CBLOGDEBUG; // 
        (**str).cf.searchmethod=CBSEARCHNEXTNAMES; // default
        //(**str).cf.searchmethod=CBSEARCHUNIQUENAMES;
#ifdef CB2822MESSAGE
	(**str).cf.asciicaseinsensitive=1;
	(**str).cf.unfold=1;
	//(**str).cf.removewsp=1; // test
	//(**str).cf.removecrlf=1; // test
	(**str).cf.removewsp=0; // default
	(**str).cf.removecrlf=0; // default
	(**str).cf.rfc2822headerend=1; // default, stop at headerend
	//(**str).cf.rstart=0x00003A; // ':', default
	//(**str).cf.rend=0x00000A;   // LF, default
	(**str).cf.rstart=CBRESULTSTART; // tmp
	(**str).cf.rend=CBRESULTEND; // tmp
	(**str).cf.searchstate=CBSTATEFUL;
#else
	(**str).cf.asciicaseinsensitive=0; // default
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
	(**str).cf.leadnames=0; // default
#ifdef CBSETSTATEFUL
	(**str).cf.searchstate=CBSTATEFUL;
	(**str).cf.leadnames=0;
#endif
#ifdef CBSETSTATETOPOLOGY
	(**str).cf.searchstate=CBSTATETOPOLOGY;
	(**str).cf.leadnames=1;
#endif
#ifdef CBSETSTATELESS
	(**str).cf.searchstate=CBSTATELESS;
	(**str).cf.leadnames=1;
#endif
#ifdef CBSETSTATETREE
	(**str).cf.searchstate=CBSTATETREE;
	(**str).cf.leadnames=1; // important
#endif
	(**str).cf.json=0;
	(**str).cf.jsonnamecheck=0;
#ifdef CBSETJSON
	(**str).cf.json=1;
	(**str).cf.jsonnamecheck=1;
#endif
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
	unsigned char *bl = NULL;
	return cb_allocate_buffer_from_blk( &(*cbf), &bl, bufsize );
}
int  cb_allocate_buffer_from_blk(cbuf **cbf, unsigned char **blk, int blksize){ 
	if( cbf==NULL ) 
	  return CBERRALLOC;
	*cbf = (cbuf *) malloc(sizeof(cbuf));
	return cb_init_buffer_from_blk( &(*cbf), &(*blk), blksize );
}
int  cb_init_buffer_from_blk(cbuf **cbf, unsigned char **blk, int blksize){ 
	if(cbf==NULL || *cbf==NULL)
	  return CBERRALLOC;
	if( blk==NULL || *blk==NULL ){
	  (**cbf).buf = (unsigned char *) malloc(sizeof(char)*( (unsigned int) blksize+1));
	  if( (**cbf).buf == NULL )
	    return CBERRALLOC;
	  (**cbf).buf[blksize]='\0';
	}else{
	  (**cbf).buf = &(**blk);
	}

	memset( &(**cbf).buf[0], 0x20, (size_t) blksize ); // 21.2.2015 checked the place of memset is correct

	(**cbf).buflen=blksize;
	(**cbf).contentlen=0;
	(**cbf).list.namecount=0;
	(**cbf).index=0;
	(**cbf).readlength=0; // 21.2.2015
	(**cbf).maxlength=0; // 21.2.2015
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
	cb_name *leaf = NULL;
	cb_name *next = NULL;

	if( name==NULL || *name==NULL )
	  return CBERRALLOC;

	leaf = &(* (cb_name*) (**name).leaf );
	if( leaf!=NULL )
           cb_free_name( &leaf ); // deepest leaf first, 27.2.2015
	if( leaf!=NULL ){
	   next = &(* (cb_name*) (**name).next );
	   cb_free_name( &next ); // 14.12.2014, nexts leafs
	}
	free( (**name).namebuf );
	(**name).namebuf=NULL; (**name).namelen=0; (**name).buflen=0;
	(**name).next=NULL; (**name).leaf=NULL; // 21.2.2015
	/* Set last name to NULL */
	free( (* (void**) name ) );
	*name=NULL; 
	return CBSUCCESS;
}
int  cb_reinit_buffer(cbuf **buf){ // free names and init
	if(buf==NULL || *buf==NULL)
	  return CBERRALLOC;

	(**buf).index=0;
	(**buf).contentlen=0;
	(**buf).readlength=0; // 21.2.2015
	(**buf).maxlength=0; // 21.2.2015
	(**buf).offsetrfc2822=0; // 21.2.2015
	(**buf).list.current=NULL; // 1.6.2013
	(**buf).list.currentleaf=NULL; // 11.12.2013
	(**buf).list.last=NULL; // 1.6.2013
	cb_empty_names(&(*buf));
	(**buf).list.name=NULL; // 1.6.2013
	(**buf).list.namecount=0; // 21.2.2015
	return CBSUCCESS;
}
/*
 * Used only with CBCFGSEEKABLEFILE (seekable) to clear the block before 
 * writing in between with offset and after or reading from an offset. 
 *
 * reading = 1 , reading, rewind with lseek buffers contentlength and empty buffer
 * reading = 0 , writing, append last block (cb_flush)
 *
 */
int  cb_empty_block(CBFILE **buf, char reading ){
	if(buf==NULL || *buf==NULL || (**buf).blk==NULL ){ return CBERRALLOC; }
	off_t lerr=0;
	if( (**buf).cf.type!=CBCFGSEEKABLEFILE )
		return CBOPERATIONNOTALLOWED;
	if( reading==1 && (*(**buf).blk).contentlen > 0 ) // rewind
		lerr = lseek( (**buf).fd, (off_t) 0-(*(**buf).blk).contentlen, SEEK_CUR );
	if( reading==0 && (*(**buf).blk).contentlen > 0 ) // flush last, append
		cb_flush( &(*buf) );
	(*(**buf).blk).contentlen = 0;
	(*(**buf).blk).index = 0;
	if( lerr==-1 ){
	  cb_log( &(*buf), CBLOGALERT, "\ncb_empty_read_block: lseek returned -1 errno %i .", errno);
	  return CBERRFILEOP;
	}
	return CBSUCCESS;
}
int  cb_empty_names(cbuf **buf){
	int err=CBSUCCESS;
	if( buf==NULL || *buf==NULL )
	  return CBERRALLOC;

	//if( (**buf).list.name==NULL )
	//  cb_clog( CBLOGWARNING, "\ncb_empty_names: list.name was null.");

	if( (**buf).list.name!=NULL ){
	  err = cb_empty_names_from_name( &(*buf), &( (**buf).list.name ) );
	  //if( err>=CBNEGATION ){ cb_clog( CBLOGDEBUG, "\ncb_empty_names: cb_empty_names_from_name returned %i.", err ); }
	}
	if( (**buf).list.name!=NULL ){
	  err = cb_free_name( &( (**buf).list.name ) );
	  //if( err>=CBNEGATION ){ cb_clog( CBLOGDEBUG, "\ncb_empty_names: cb_free_name returned %i.", err ); }
	}
	(**buf).list.namecount = 0; // namecount of main list (leafs are not counted)
	(**buf).list.name = NULL;

	(**buf).list.last = NULL; // 21.2.2015
	(**buf).list.current = NULL; // 21.2.2015
	(**buf).list.currentleaf = NULL; // 21.2.2015

	return err;
}
int  cb_free_names_from(cb_name **cbn){
	int err=CBSUCCESS;
	cb_name *name = NULL;
	cb_name *nextname = NULL;
	if( cbn==NULL || *cbn==NULL ){ return CBERRALLOC; }

	name = &(* (cb_name*) (**cbn).next);
	while(name != NULL){
		nextname = &(* (cb_name*) (*name).next);
		err = cb_free_name( &name ); // frees leaves
		name = &(* nextname);
	}
	(**cbn).next = NULL;
	(**cbn).leaf = NULL;

	return err;
}
int  cb_empty_names_from_name(cbuf **buf, cb_name **cbn){
	int err=CBSUCCESS;
	cb_name *name = NULL;
	cb_name *nextname = NULL;
	if( buf==NULL || *buf==NULL || cbn==NULL || *cbn==NULL ){ return CBERRALLOC; }

	err = cb_free_names_from( &(*cbn) );
	if( err>=CBERROR ){	return err;  }

	(**buf).list.last = &(**cbn); // 21.2.2015
	(*(**buf).list.last).next = NULL; // 25.2.2015
	(**buf).list.current = NULL; // 21.2.2015
	(**buf).list.currentleaf = NULL; // 21.2.2015

	/* Update namecount. */
	err = 0;
	if( (**buf).list.name!=NULL ){
		name = &(* (cb_name*) (**buf).list.name);
		while(name != NULL){
			++err;
			nextname = &(* (cb_name*) (*name).next);
			name = &(* nextname);
		}
	}
	(**buf).list.namecount = err;

	return CBSUCCESS;
}

int  cb_use_as_buffer(CBFILE **buf){
        return cb_set_type(&(*buf), (unsigned char) CBCFGBUFFER);
}
int  cb_use_as_seekable_file(CBFILE **buf){
	cb_use_as_file( &(*buf) );
	return cb_set_type(&(*buf), (unsigned char) CBCFGSEEKABLEFILE);
}
int  cb_use_as_file(CBFILE **buf){
        return cb_set_type(&(*buf), (unsigned char) CBCFGFILE);
}
int  cb_use_as_stream(CBFILE **buf){
        return cb_set_type(&(*buf), (unsigned char) CBCFGSTREAM);
}
int  cb_set_type(CBFILE **buf, unsigned char type){
	if(buf!=NULL){
	  if((*buf)!=NULL){
	    (**buf).cf.type = type;
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
	if( offset > 0 && (**cbf).cf.type!=CBCFGSEEKABLEFILE ){
		cb_log( &(*cbf), CBLOGERR, "\ncb_get_char_read_offset_block: attempt to seek to offset of unwritable (unseekable) file (CBCFGSEEKABLEFILE is not set).");
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
	    if( (**cbf).cf.type==CBCFGSEEKABLEFILE && offset>0 ){ // offset from seekable file
	       /* Internal use only. Block has to be emptied after use. File pointer is not updated in the following: */
	       sz = pread((**cbf).fd, (*blk).buf, (size_t)(*blk).buflen, offset ); // read block has to be emptied after use
	    }else{ // stream
	       sz = read((**cbf).fd, (*blk).buf, (size_t)(*blk).buflen);
	    }
	    (*blk).contentlen = (long int) sz; // 6.12.2014
	    if( 0 < (int) sz ){ // read more than zero bytes
	      (*blk).contentlen = (long int) sz; // 6.12.2014
	      (*blk).index = 0;
	      *ch = (*blk).buf[(*blk).index];
	      ++(*blk).index;
	    }else{ // error or end of file
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
int  cb_flush_cbfile_to_offset(cbuf **cb, int fd, signed long int offset){
	int err = CBSUCCESS;
	if(cb==NULL || *cb==NULL) return CBERRALLOC;
	if( (**cb).contentlen <= (**cb).buflen ){
	  if( offset < 0 ){ // Append (usual)
	    err = (int) write( fd, (**cb).buf, (size_t) (**cb).contentlen);
	  }else{ // Write (replace)
	    // in the following, the file pointer is not updated ["Adv. Progr."]:
	    err = (int) pwrite( fd, (**cb).buf, (size_t) (**cb).contentlen, (off_t) offset );
	  }
	}else{
	  if( offset < 0 ){ // Append
	    err = (int) write( fd, (**cb).buf, (size_t) (**cb).buflen);
	  }else{ // Write (replace)
	    // in the following, the file pointer is not updated ["Adv. Progr."]:
	    err = (int) pwrite( fd, (**cb).buf, (size_t) (**cb).buflen, (off_t) offset );
	  }
	}
	if(err<0){
	  cb_clog( CBLOGINFO, "\ncb_flush_cbfile_to_offset: errno %i .", errno);
	  err = CBERRFILEWRITE ; 
	}else{
	  err = CBSUCCESS;
	  (**cb).contentlen=0; // Block is set to zero here (write or append is not possible without this)
	}
	return err;
}
int  cb_flush_to_offset(CBFILE **cbs, signed long int offset){
	if( cbs==NULL || *cbs==NULL ){ return CBERRALLOC; }

	if( *cbs!=NULL ){
 	  if((**cbs).cf.type!=CBCFGBUFFER){
	    if((**cbs).blk!=NULL){
	      if( offset > 0 && (**cbs).cf.type!=CBCFGSEEKABLEFILE ){
		cb_log( &(*cbs), CBLOGERR, "\ncb_flush_to_offset: attempt to seek to offset of unwritable file (CBCFGSEEKABLEFILE is not set).");
		return CBOPERATIONNOTALLOWED;
	      }
	      return cb_flush_cbfile_to_offset( &(**cbs).blk, (**cbs).fd, offset);
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
	      err = cb_put_ch( &(*cbs), buf[indx]);
	    }
	    err = cb_flush( &(*cbs) );
	    return err;
	  }
	}
	return CBERRALLOC;
}
int  cb_write_cbuf(CBFILE **cbs, cbuf *cbf){
	if( cbs==NULL || *cbs==NULL || cbf==NULL || (*cbf).buf==NULL ){ return CBERRALLOC; }
	return cb_write( &(*cbs), &(*(*cbf).buf), (*cbf).contentlen);
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
	      ++(*(**cbs).cb).readlength;
	      if((*(**cbs).cb).readlength > (*(**cbs).cb).maxlength )
	        (*(**cbs).cb).maxlength = (*(**cbs).cb).readlength;
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
	    //if( (*(**cbs).cb).readlength < 0x7FFFFFFFFFFFFFFF ) // long long, > 64 bit
	    if( (*(**cbs).cb).readlength < 0x7FFFFFFF ) // long, > 32 bit
	      ++(*(**cbs).cb).readlength; // (seekable filedescriptor and index >= buflen)
	    if((*(**cbs).cb).readlength > (*(**cbs).cb).maxlength )
	      (*(**cbs).cb).maxlength = (*(**cbs).cb).readlength;
	    if((*(**cbs).cb).contentlen > (*(**cbs).cb).buflen && (**cbs).cf.type==CBCFGSEEKABLEFILE )
	      return CBFILESTREAM;
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
        if(*buf==NULL){ cb_clog( CBLOGALERT, "\ncb_get_sub_buffer: malloc returned null."); return CBERRALLOC; }
        (*buf)[(*size)] = '\0';
        for(index=0;(index<(*to-*from) && index<(*size) && (index+*from)<(*cbs).contentlen); ++index){
          (*buf)[index] = (*cbs).buf[index+*from];
        }
        *size = index;
        return CBSUCCESS;
}

/*
 * Content of list may change in writing. Updating the namelist is in the
 * responsibility of the user. Buffer is swapped temporarily and function can
 * not be threaded unless it is used atomically.
 */
int  cb_write_to_offset(CBFILE **cbf, unsigned char **ucsbuf, int ucssize, int *byteswritten, signed long int offset, signed long int offsetlimit){
	int index=0, charcount=0; int err = CBSUCCESS;
	int bufindx=0, bytecount=0, storedsize=0;
	unsigned long int chr = 0x20;
	cbuf *origcb = NULL;
	cbuf *cb = NULL;
	cbuf  cbdata;
	unsigned char buf[CBSEEKABLEWRITESIZE+1];
	unsigned char *ubf = NULL;
	long int offindex = offset, contentlen = 0;

	if( cbf==NULL || *cbf==NULL || (**cbf).blk==NULL || byteswritten==NULL ){ return CBERRALLOC; }
	if( ucsbuf==NULL || *ucsbuf==NULL ){ return CBERRALLOC; }
	if( (**cbf).cf.type!=CBCFGSEEKABLEFILE ){
	  cb_log( &(*cbf), CBLOGALERT, "\ncb_write_to_offset: attempt to write to unseekable file, type %i. Returning CBOPERATIONNOTALLOWED .", (**cbf).cf.type );
	  return CBOPERATIONNOTALLOWED;
	}
	*byteswritten=0;

	cb_remove_ahead_offset( &(*cbf), &(**cbf).ahd );

	/* Replace block with small write block */
	origcb = &(*(**cbf).blk);
	if( (*(**cbf).blk).buflen < CBSEEKABLEWRITESIZE || (*(**cbf).blk).contentlen!=0 ){ // swap block if it is not flushed or smaller
		cb = &cbdata;
		buf[CBSEEKABLEWRITESIZE]='\0';
		ubf = &buf[0];
		cb_init_buffer_from_blk( &cb, &ubf, CBSEEKABLEWRITESIZE);
		(**cbf).blk = &(*cb);
	}

	//cb_log( &(*cbf), CBLOGDEBUG, "\ncb_write_to_offset: data size %i, offset %li, offsetlimit %li, ", ucssize, offset, offsetlimit );
	//cb_log( &(*cbf), CBLOGDEBUG, "blocksize %li .", (*(**cbf).blk).buflen );
	//cb_log( &(*cbf), CBLOGDEBUG, ,"\ncb_write_to_offset: writing from %li size %i characters (r: %i) %i bytes [", offset, (ucssize/4), (ucssize%4), ucssize);
	//cb_print_ucs_chrbuf( CBLOGDEBUG, &(*ucsbuf), ucssize, ucssize);
	//cb_log( &(*cbf), CBLOGDEBUG, "] :\n");
        
	// Write block length and flush to offset
	while( ucssize>=0 && bufindx<ucssize && err<CBERROR && err!=CBNOFIT ){
	  // Flush full block
	  if( index >= (*(**cbf).blk).buflen-6 && (*(**cbf).blk).buflen>=6 ){ // minus longest known bytecount of character
	    contentlen = (*(**cbf).blk).contentlen;
	    cb_flush_to_offset( &(*cbf), (offindex+1) );
	    offindex += contentlen;
	  }
	  // Write to block
	  err = cb_get_ucs_chr( &chr, &(*ucsbuf), &bufindx, ucssize);
	  ++charcount;
	  cb_put_chr( &(*cbf), chr, &bytecount, &storedsize );
	  index += storedsize;
	  *byteswritten += storedsize;
	  if( offindex >= offsetlimit || index+offset >= offsetlimit )
	    err = CBNOFIT;
	}
	// Flush the rest
	contentlen = (*(**cbf).blk).contentlen;
	err = cb_flush_to_offset( &(*cbf), (offindex+1) ); // does not update filepointer
	offindex += contentlen;

	// Return the original block (if it was swapped)
	(**cbf).blk = &(*origcb);

	// Return and error check
	if( charcount*4 < ucssize || index+offset >= offsetlimit )
	  return CBNOFIT;
        return CBSUCCESS;
}

/*
 * Storage data of one character is stored in stg and it's size to stgsize. */
int  cb_character_size(CBFILE **cbf, unsigned long int ucschr, unsigned char **stg, int *stgsize){
        int err = CBSUCCESS;
        int bcount=0;
        cbuf *origcb = NULL;
        cbuf *cb = NULL;
        cbuf  cbdata;
        unsigned char buf[6+1]; // longest known character+1
        unsigned char *ubf = NULL;
        
        if( cbf==NULL || *cbf==NULL || (**cbf).blk==NULL ){ return CBERRALLOC; }
        if( stg==NULL ){ return CBERRALLOC; }
        
        cb_remove_ahead_offset( &(*cbf), &(**cbf).ahd );
        
        // Replace block with small write block
        cb = &cbdata;
        buf[6]='\0';
        ubf = &buf[0];
        cb_init_buffer_from_blk( &cb, &ubf, 6);
        
        origcb = &(*(**cbf).blk);
        (**cbf).blk = &(*cb);
        
        // Write character
        err = cb_put_chr( &(*cbf), ucschr, &bcount, &(*stgsize) );

        // Return the original block
        (**cbf).blk = &(*origcb);
        
        if(err>=CBNEGATION){
                *stgsize = 0;
                return err;
        }

        // Allocate and copy characterarray
        *stg = (unsigned char*) malloc( (size_t) sizeof( char )*( (unsigned int) (*stgsize) + 1 ) );
        if(*stg==NULL){ return CBERRALLOC; }
        (*stg)[*stgsize] = '\0';
        for( bcount=(*stgsize-1) ; bcount>=0; --bcount){
          (*stg)[bcount] = buf[bcount];
        }
        
        return CBSUCCESS;
}


/*
 * Fills with chr:s from offset to offsetlimit. If not even at end, does not fill. */
int  cb_erase(CBFILE **cbf, unsigned long int chr, signed long int offset, signed long int offsetlimit){
        int err=CBSUCCESS, bcount=0;
        unsigned char *cdata = NULL;

        if( cbf==NULL || *cbf==NULL ){ return CBERRALLOC; }
        if( offset<0 || offsetlimit<0 ){
                cb_log( &(*cbf), CBLOGERR, "\ncb_erase: warning, a parameter was negative.");
                return CBOVERFLOW;
        }

        cb_log( &(*cbf), CBLOGDEBUG, "\ncb_erase: erasing from %li to limit %li (bytes in buffer).", offset, offsetlimit );
 
        if( (**cbf).cf.type!=CBCFGSEEKABLEFILE )
                if( (*(**cbf).cb).index >= (*(**cbf).cb).contentlen || (*(**cbf).cb).readlength>=(*(**cbf).cb).contentlen )
                        cb_log( &(*cbf), CBLOGWARNING, "\ncb_erase: warning, attempting to erase a stream.");
          
        /*
         * Character size. (Slow and consuming.) */
        err = cb_character_size( &(*cbf), chr, &cdata, &bcount);
        if( err!=CBSUCCESS || cdata==NULL || bcount==0 ){ return err; }

	cb_log( &(*cbf), CBLOGDEBUG, "\ncb_erase: charactersize %i .", bcount );
          
        /*
         * Write (( offsetlimit-offset/bcount )) characters and remainder. */
	err=0;
        while( err>=0 && offset <= ( offsetlimit - (bcount-1) ) ){
		cb_log( &(*cbf), CBLOGDEBUG, "\ncb_erase: writing %i to %li .", bcount, offset );
                err = (int) pwrite( (**cbf).fd, &(*cdata), (size_t) bcount, (off_t) offset );
                offset+=bcount;
        }

	free(cdata);                
        return err;
}

/*
 * Seek to beginning of file and renew all buffers and namelist. 
 * For example to reread a configuration file.
 */
int  cb_reread_file( CBFILE **cbf ){
        return cb_reread_new_file( &(*cbf), -1 );
}

/*
 * Renews to new filedescriptor. Does not close old. */
int  cb_reread_new_file( CBFILE **cbf, int newfd ){
        int err2=CBSUCCESS;
	long long int err=CBSUCCESS;
        if( cbf==NULL || *cbf==NULL ){ return CBERRALLOC; }
        if( (**cbf).cf.type!=CBCFGSEEKABLEFILE ){
          cb_log( &(*cbf), CBLOGWARNING, "\ncb_seek: warning, trying to seek an unseekable file.");
          return CBOPERATIONNOTALLOWED;
        }
        // Remove readahead
        err2 = cb_remove_ahead_offset( &(*cbf), &(**cbf).ahd );
	if( err2>=CBNEGATION ){
	  cb_clog( CBLOGNOTICE, "\ncb_reread_file: could not remove ahead offset, %i.", err2);
	  if( err2>=CBERROR )
	    cb_clog( CBLOGWARNING, "\ncb_reread_file: Error %i.", err2);
	}

        // Do not close old filedescriptor to use it's file elsewhere (for example in child processes), even if new descriptor was given
        if( newfd>=0 ){
          (**cbf).fd = newfd; // attach new stream (for example old file was renamed and new file replaced old)
        }
        
        // Seek filedescriptor
        err = lseek( (**cbf).fd, (off_t) 0, SEEK_SET );
        if(err<0){
          cb_clog( CBLOGNOTICE, "\ncb_reread_file: could not seek new file descriptor, lseek errno %i .", errno );
          err2=CBERRFILEOP;
        }else{
        
          // Deallocate previous CBFILE:s buffers (without closing filedescriptor)
          err2 = cb_reinit_buffer(&(**cbf).cb); // free names, reset buffer
          err2 = cb_reinit_buffer(&(**cbf).blk); // reset block
          return err2;
        }
        return err2;
}

