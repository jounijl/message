/*
 * Library to read and write streams. Valuepair searchlist with different encodings.
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

#define CBSUCCESS              0
#define CBSTREAM               1
#define CBFILESTREAM           2    // Buffer end has been reached but file is seekable
#define CBMATCH                3    // Matched and the lengths are the same
#define CBUSEDASBUFFER         4
#define CBUTFBOM               5
#define CB2822HEADEREND        6
#define CBMATCHLENGTH          7    // Matched the given length
#define CBADDEDNAME            8
#define CBADDEDLEAF            9
#define CBADDEDNEXTTOLEAF     10
#define CBMATCHMULTIPLE       11    // regexp multiple occurences 03/2014
#define CBMATCHGROUP          12    // regexp group 03/2014

#define CBNEGATION            20
#define CBSTREAMEND           21
#define CBVALUEEND            22    // opencount < 0, CBSTATETREE and CBSTATETOPOLOGY
#define CBBUFFULL             23
#define CBNOTFOUND            24
#define CBNAMEOUTOFBUF        25
#define CBNOTUTF              26
#define CBNOENCODING          27
#define CBMATCHPART           28    // 30.3.2013, shorter name is the same as longer names beginning
#define CBEMPTY               29
#define CBNOTSET              30
#define CBAUTOENCFAIL         31    // First bytes bom was not in recognisable format
#define CBWRONGENCODINGCALL   32
#define CBUCSCHAROUTOFRANGE   33
#define CBREPATTERNNULL       34    // given pattern text was null 03/2014
#define CBGREATERTHAN         35    // same as not found with lexical comparison of name1 to name2
#define CBLESSTHAN            36
#define CBOPERATIONNOTALLOWED 37    // seek was asked and CBFILE was not set as seekable
#define CBNOFIT               38    // tried to write to too small space (characters left over)

#define CBERROR	              40
#define CBERRALLOC            41
#define CBERRFD               42
#define CBERRFILEOP           43
#define CBERRFILEWRITE        44
#define CBERRBYTECOUNT        45
#define CBARRAYOUTOFBOUNDS    46
#define CBINDEXOUTOFBOUNDS    47
#define CBLEAFCOUNTERROR      48
#define CBERRREGEXCOMP        49    // regexp pattern compile error 03/2014
#define CBERRREGEXEC          50    // regexp exec error 03/2014
#define CBBIGENDIAN           51
#define CBLITTLEENDIAN        52
#define CBUNKNOWNENDIANNESS   53
#define CBOVERFLOW            54

/*
 * Default values (multiple of 4)
 */
#define CBNAMEBUFLEN        1024
#define CBREADAHEADSIZE       28
#define CBSEEKABLEWRITESIZE  128

/*
 CBREADAHEADSIZE is used to read RFC-822 unfolding characters. It's size should be small
 to use the CBFILE in other purposes without too much overhead and to avoid reading
 too much before without a cause. Size is multiple of 4 (4-bytes characters).
 */


#define CBMAXLEAVES         1000  // If CBSTATETREE is set, count of leaves one name can have (*next is unbounded and uncounted). 
                                  // (this can be set to a maximum number of unsigned integer)

#define CBRESULTSTART       '='
#define CBRESULTEND         '&'
#define CBBYPASS            '\\'
#define CBCOMMENTSTART      '#'  // Allowed inside valuename from rend to rstart
#define CBCOMMENTEND        '\n'

#define CBSUBRESULTSTART    '='  // If set to another, in CBSTATETREE, swithes every other separator (every odd separator)
#define CBSUBRESULTEND      '&'  // 


/*
 * Configuration options
 */

/*
 * Search options */
#define CBSEARCHUNIQUENAMES       0   // Names are unique (returns allways the first name in list)
#define CBSEARCHNEXTNAMES         1   // Multiple same names (returns name if matchcount is zero, otherwice searches next in list or in stream)

/*
 * Third search option is to leave buffer empty amd search allways the next name, with either above setting.
 */

/*
 * Use options, changes functionality as below */
#define CBCFGSTREAM          0x00   // Use as stream (namelist is bound by buffer)
#define CBCFGBUFFER          0x01   // Use only as buffer (fd is not used at all)
#define CBCFGFILE            0x02   // Use as file (namelist is bound by file),
                                    // cb_reinit_buffer empties names but does not seek, use: lseek((**cbf).fd if needed
#define CBCFGSEEKABLEFILE    0x03   // Use as file capable of seeking the cursor position - buffer is partly truncated and block is emptied after every offset read or write


/*
 * This setting enables multibyte, UTF-16 and UTF-32 support if
 * the machine processor has big endian words (for example PowerPC, ARM, 
 * Sparc and some Motorola processors). Not yet tested 31.8.2013.
 */
//#undef BIGENDIAN



/*
 * When CBSTATETREE is used, CBSETJSON setting causes
 * the name parsing to:
 * - Ignore the last delimiter in list if the next character is
 *   even openpairs count (the first delimiter is "stronger" than
 *   the second and it reduces openpairs twice if the last name-
 *   value pair had no end character. - not yet tested 5.12.2013
 * - Empty names must be allowed to use empty object (without name)
 *   and twice the delimiters for example: { Name:{ Name2: val } },
 *   Object starts with no name and : and { are together. These must
 *   both be one empty name. WSP:s should be removed between : and {
 *   (in json, character after ':' is allways '"' , '[' or '{').
 *   if(inteven(openpairs)) - not yet done or tested 5.12.2013
 */
//#define CBSETJSON

/*
 * When using CBSTATETREE, uses other delimiters every second
 * time when the openpairs count increases.
 */
//#define CBSETDOUBLEDELIM 


/* If leafs are needed, CBSETSTATETREE has to be set before writing
 * to the file (functions in cb_write.c) to update the namelist
 * correctly. */

/*
 * Sets CBSTATETREE. CBSTATETREE saves name-valuepairs from value to leaves.
 * For example: a=b=c&d=f&& becomes  a { b { c } d { f } } , not just a{ }
 * Form is not an ordinary binary tree. Setting saves the next inner name-value
 * to the left and next name-value to the right. The order stays the same as it
 * was in input if reading is done in order: node, left, right.
 */
//#define CBSETSTATETREE

/*
 * This setting takes account only outermost name and leaves all inner 
 * name-value pairs in the value. 
 *
 * It is possible to open a new CBFILE to read from a value. Inner names 
 * can not be used from the same CBFILE.
 *
 * Reading a value is unlimited. Buffer is in use only from '&' to '='.
 *
 * Reader function should also implement counter counting the open pairs.
 * It should count every open '=' and read beyond every paired '&' until 
 * count is zero at last '&'.
 */
//#define CBSETSTATETOPOLOGY

/*
 * Stateful. After first value separator ('='), state is changed to read to next name separator 
 * ('&') when leaving the value unread. State changes back at rend ('&'). This way values 
 * can not be used to change values occurring after for example programmatically or by changing
 * values deliberately from the user interface.
 *
 * This setting improves safety and is more restrictive. Cursor has to be left at the end
 * of the value. If reader function ignores flow control characters, this setting might 
 * cause invalid names in name-valuepair list. This is a better setting for input.
 *
 * Reading a value is unlimited. Buffer is in use only from '&' to '='.
 */
//#define CBSETSTATEFUL

/*
 * Default setting with no defines.
 *
 * To use only value separator ('=') to separate names. Values can contain new
 * inner names and new values. 
 *
 * Values can be defined as values and the value can contain new values depending 
 * on what rstart or rend the programmer leaves the cursor at. Searching the next 
 * name starts where the reading of the value ended. 
 *
 * If reader ignores flow control characters, this setting might cause invalid names
 * in name-valuepair list. 
 *
 * This is more relaxed setting and provides more features. Programmer has to decide
 * how to read the values flow control characters. Cursor has to be left at the end
 * of the value. This is a good setting to read output from known input source, in
 * reporting for example.
 *
 * Reading names and values are restricted to read buffer size, size is CBNAMEBUFLEN 
 * with 4 byte characters.
 */
//#undef CBSETSTATEFUL
//#undef CBSETSTATETOPOLOGY
//#undef CBSETSTATETREE
//#define CBSETSTATELESS

/*
 * The use of states in searching the name (as in cb_set_cursor_ucs ) */
#define CBSTATELESS              0x00
#define CBSTATEFUL               0x01
#define CBSTATETOPOLOGY          0x02
#define CBSTATETREE              0x03


/*
 * With this define, cb_set_cursor stops searching the stream when it encounters
 * a character sequence at header end, between header and message (1).
 * In RFC-822 and RFC-2822 this is two sequential <cr><lf> characters.
 */
#define CB2822MESSAGE

// RFC 2822 unfolding, (RFC 5198: CR can appear only followed by LF)

/* ABNF names RFC 2234 6.1 */
#define WSP( x )              ( ( x ) == 0x20 || ( x ) == 0x09 )
#define CR( x )               ( ( x ) == 0x0D )
#define LF( x )               ( ( x ) == 0x0A )
#define CTL( x )              ( ( ( x ) >= 0 && ( x ) <= 31 ) || ( x ) == 127 )

/* FWS (folding white space), linear white space */
#define LWSP( prev2 , prev , chr )             ( (  ( (prev2) == 0x0D && (prev) == 0x0A ) || \
                                                    ( (prev) == 0x20 || (prev) == 0x09 )     ) && \
                                                    ( (chr) == 0x20 || (chr)  == 0x09 )      )

/*
 * Characters to remove from name: BOM. Comment string is removed elsewhere.
 * UTF-8: "initial U+FEFF character may be stripped", RFC-3629 page-6, UTF-16: 
 * http://www.unicode.org/L2/L2005/05356-utc-bomsig.html
 */
#define NAMEXCL( x )          ( ( x ) == 0xFEFF )

/* 4 bytes - 1 bit. ANSI C 1999, 6.5.5: "The operands
 * of the % operator shall have integer type." */
#define intiseven( x )        ( ( x )%2 == 0 )

#include "./cb_encoding.h"	// Encoding macros

/* Pointer size */
#define PSIZE                 void*

/*
 * Compare ctl */
typedef struct cb_match {
        int matchctl; // If match function is not regexp match, next can be NULL
        void *re; // cast to pcre32 (void here to be able to compile the files without pcre)
        void *re_extra; // cast to pcre_extra32
} cb_match;

/*
 * Fifo ring structure */
typedef struct cb_ring {
	char fpadding;        // compiler warnings, pad to next storage size (32 bit)
	char spadding;        // pad to next storage size (32 bit)
        unsigned char buf[CBREADAHEADSIZE+1];
        unsigned char storedsizes[CBREADAHEADSIZE+1];
        int buflen;
        int sizeslen;
        int ahead;
        int bytesahead;
        int first;
        int last;
	int streamstart;      // id of first character from stream
	int streamstop;       // id of stop character
} cb_ring;

typedef struct cb_conf{
        unsigned char       type:4;                 // stream (default), file (large namelist), only buffer (fd is not in use) or seekable file (large namelist and offset operations)
        unsigned char       searchmethod:4;         // search next name (multiple names) or search allways first name (unique names), CBSEARCH*
        unsigned char       unfold:2;               // Search names unfolding the text first, RFC 2822
        unsigned char       asciicaseinsensitive:2; // Names are case insensitive, ABNF "name" "Name" "nAme" "naMe" ..., RFC 2822
        unsigned char       rfc2822headerend:2;     // Stop after RFC 2822 header end (<cr><lf><cr><lf>) 
        unsigned char       removewsp:2;            // Remove linear white space characters (space and htab) between value and name (not RFC 2822 compatible)
        unsigned char       removecrlf:2;           // Remove every CR:s and LF:s between value and name (not RFC 2822 compatible)
	unsigned char       removenamewsp:2;        // Remove white space characters inside name
	unsigned char       leadnames:2;            // Saves names from inside values, from '=' to '=' and from '&' to '=', not just from '&' to '=', a pointer to name name1=name2=name2value;
	unsigned char       json:2;                 // When using CBSTATETREE, form of data is JSON compatible (without '"':s and '[':s in values), also doubledelim must be set
	unsigned char       doubledelim:2;          // When using CBSTATETREE, after every second openpair, rstart and rstop are changed to another
	unsigned char       searchstate:6;          // No states = 0 (CBSTATELESS), CBSTATEFUL, CBSTATETOPOLOGY, CBSTATETREE

	unsigned long int   rstart;	// Result start character
	unsigned long int   rend;	// Result end character
	unsigned long int   bypass;	// Bypass character, bypasses next special characters function
	unsigned long int   cstart;	// Comment start character (comment can appear from rend to rstart)
	unsigned long int   cend;	// Comment end character

	unsigned long int   subrstart;   // If CBSTATETREE and doubledelim is set, rstart of every other openpairs (odd), subtrees delimiters
	unsigned long int   subrend;     // If CBSTATETREE and doubledelim is set, rend of every other openpairs (odd), subtrees delimiters 
} cb_conf; 

// signed to detect overflow

typedef struct cb_name{
        unsigned char        *namebuf;        // name
	int                   buflen;         // name+excess namebuffer space
        int                   namelen;        // name length
        signed long int       offset;         // offset from the beginning of data (to '=')
        signed long int       nameoffset;     // offset of the beginning of last data (after reading '&'), 6.12.2014
        int                   length;         // length of namepairs area, from previous '&' to next '&', unknown (almost allways -1), possibly empty, set after it's known
        signed long int       matchcount;     // if CBSEARCHNEXT, increases by one when traversed by, zero only if name is not searched yet
        void                 *next;           // Last is NULL {1}{2}{3}{4}
	signed long int       firsttimefound; // Time in seconds the name was first found and/or used (set by set_cursor)
	signed long int       lasttimeused;   // Time in seconds the name was last searched or used (set by set_cursor)
	void                 *leaf;           // { {1}{2} { {3}{4} } } , last is NULL
} cb_name;

typedef struct cb_namelist{
        cb_name            *name;
	cb_name            *current;
	cb_name            *last;
	signed long int     namecount;
	cb_name            *currentleaf;      // 9.12.2013, sets as null every time 'current' is updated
} cb_namelist;

typedef struct cbuf{
        unsigned char           *buf;
        signed long int          buflen;          // In bytes
	signed long int          index;           // Cursor offset in bytes (in buffer)
	signed long int          contentlen;      // Bytecount in numbers (first starts from 1), comment: 7.11.2009
	signed long int          readlength; 	  // CBSEEKABLEFILE: Read position in bytes (from filestream), 15.12.2014 Late addition: useful with seekable files (useless when appending or reading).
	signed long int          maxlength; 	  // CBSEEKABLEFILE: Overall read length in bytes (from filestream), 15.12.2014 Late addition: useful with seekable files (useless when appending or reading).
	cb_namelist              list;
        int                      offsetrfc2822;   // offset of RFC-2822 header end with end characters (offset set at last new line character)
} cbuf;

typedef struct cbuf cblk;

typedef struct CBFILE{
	int                 fd;		// Stream file descriptor
	cbuf               *cb;		// Data in valuepairs (preferably in applications order)
	cblk               *blk;	// Input read or output write -block 
	cb_ring             ahd;	// Ring buffer to save data read ahead
        cb_conf             cf;         // All configurations, 20.8.2013
	int                 encodingbytes; // Maximum bytecount for one character, 0 is any count, 4 is utf low 6 utf normal, 1 any one byte characterset
	int                 encoding;   // list of encodings is in file cb_encoding.h 
} CBFILE;

/*
 * name is a pointer to a namelength size unsigned char array to
 * search and set cursor to.
 *
 * Set cursor to position name or return CBNOTFOUND or CBSTREAM.
 * 'bufsize' buf is saved and other data is read as a stream or
 * lost. Cursor is allways at the end of read buffer and characters
 * are read from cursor. Names are read from cb_set_cursor.
 *
 * CBFILE can be set to bypass sp:s and tabs before name:s to use
 * the buffer in writing the name-value pairs in fixed length blocks
 * in any memory fd points to. Settings are 'removewsp' and 'removecrlf'. 
 * In these cases, for example &   <name>=<value>& and &<name>=<value>&
 * are similar.
 *
 * Names are kept internally in a character array where four bytes
 * represent one character (UCS, 31-bits). 
 */

/* 
 * One byte represents a character */
int  cb_set_cursor(CBFILE **cbs, unsigned char **name, int *namelength); 

/*
 * Four bytes represents one character */
int  cb_set_cursor_ucs(CBFILE **cbs, unsigned char **ucsname, int *namelength); 

/*
 * The same functions overloaded with match length of the name. These are 
 * useful internally and in some searches. 
 *
 * matchctl  1 - strict match, CBMATCH
 * matchctl  0 - searches any next name not yet used, once
 * matchctl -1 - searches endlessly without matching any name listing all the
 *               rest of the unused names
 * matchctl -2 - match names length text, CBMATCHLEN (name like nam% or nam*)
 * matchctl -3 - match if part of name matches to other names length text, CBMATCHPART
 * matchctl -4 - match if part of name matches to either names length, CBMATCHPART or CBMATCHLENGTH
 * matchctl -5 - match from end name1 length (name like %ame or *ame)
 * matchctl -6 - match in between name1 length (name like %am% or *am*)
 * re strict:
 * matchctl -7 - match once or group with provided re and pcre_extra (in cb_match, not null) using 4-byte characterbuffer and pcre32
 * matchctl -8 - match once or group with null terminated pattern in name1 compiling it to re just before searching (using 4-byte characterbuffer and pcre32)
 * re multiple:
 * matchctl -9 - match once, group or multiple times with provided re and pcre_extra (in cb_match, not null) using 4-byte characterbuffer and pcre32
 * matchctl -10 - match once, group or multiple times with null terminated pattern in name1 compiling it to re just before searching (using 4-byte characterbuffer and pcre32)
 * matchctl -11 - match if name1 is in UCS codechart ("lexically") less or equal than name2 (not yet tested 31.7.2014 ->)
 * matchctl -12 - match if name1 is in UCS coding greater or equal than name2 
 * matchctl -13 - match if name1 is in UCS coding less than name2
 * matchctl -14 - match if name1 is in UCS coding greater than name2
 *
 * In CBSTATETREE and CBSTATETOPOLOGY, ocoffset updates openpairs -count. The reading stops
 * when openpairs is negative. Next rend after value ends reading. This parameter can be used
 * to read leafs inside values. Currentleaf is updated if leaf is found with depth ocoffset.
 *
 * Returns on success: CBSUCCESS, CBSTREAM, CBFILESTREAM (only if CBCFGSEEKABLEFILE is set) or
 * CB2822HEADEREND of it was set.
 * May return: CBNOTFOUND, CBVALUEEND
 * Possible errors: CBERRALLOC
 * cb_search_get_chr: CBSTREAMEND, CBNOENCODING, CBNOTUTF, CBUTFBOM
 *
 */
int  cb_set_cursor_match_length(CBFILE **cbs, unsigned char **name, int *namelength, int ocoffset, int matchctl);
int  cb_set_cursor_match_length_ucs(CBFILE **cbs, unsigned char **ucsname, int *namelength, int ocoffset, int matchctl);
int  cb_set_cursor_match_length_ucs_matchctl(CBFILE **cbs, unsigned char **ucsname, int *namelength, int ocoffset, cb_match *ctl);


/*
 *
 * If while reading a character, is found, that cb_set_cursor found a name
 * but one of the get_ch functions returned CBSTREAM after it, removes the
 * erroneus name from memorybuffer after reading it from the stream once.
 *
 * Sets namelegth to indicate that the names content is out of buffer.
 */

int  cb_remove_name_from_stream(CBFILE **cbs);


/*
 * One character at a time. Output flushes when the buffer is full.
 * Otherwice use write or flush when at the end of writing. Input
 * copies characters in a receivebuffer to read the values
 * afterwards if necessary. If name-value pairs are in use,
 * use cb_set_cursor instead to save the names in a structure
 * to search them fast.
 *
 * Remember to test if return value is STREAMEND. If it is, use
 * cb_remove_name_from_stream to set it's content length to remove the
 * name from buffered searched names list and remove mixed buffer and 
 * stream content from next searches.
 */

/*
 * Read or append characters according to bytecount and encoding. */
int  cb_get_chr(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes);
int  cb_put_chr(CBFILE **cbs, unsigned long int chr, int *bytecount, int *storedbytes);

/*
 * Write to offset if file is seekable. (Block is replaced, not atomic function). */
int  cb_write_to_offset(CBFILE **cbf, unsigned char **ucsbuf, int ucssize, signed long int offset, signed long int offsetlimit);

// From unicode to and from utf-8
int  cb_get_ucs_ch(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes );
int  cb_put_ucs_ch(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes );
// Transfer encoding
int  cb_get_utf8_ch(CBFILE **cbs, unsigned long int *chr, unsigned long int *chr_high, int *bytecount, int *storedbytes );
int  cb_put_utf8_ch(CBFILE **cbs, unsigned long int *chr, unsigned long int *chr_high, int *bytecount, int *storedbytes );
/*
 * Unfolds read characters. Characters are read by cb_get_chr.
 * cb_ring contains readahead buffer used in folding. The same
 * readahead buffer has to be moved across function calls. At
 * the end, the fifo has to be emptied of characters by 
 * decreasing counters in buffer.
 */
int  cb_get_chr_unfold(CBFILE **cbs, unsigned long int *chr, long int *chroffset);
/*
 * Decreases readahead from CBFILE:s length information and zeros
 * readahead counters.
 */
int  cb_remove_ahead_offset(CBFILE **cbf, cb_ring *readahead); 

// Data
int  cb_get_ch(CBFILE **cbs, unsigned char *ch);
int  cb_put_ch(CBFILE **cbs, unsigned char ch); // *ch -> ch 12.8.2013
int  cb_write_cbuf(CBFILE **cbs, cbuf *cbf);
int  cb_write(CBFILE **cbs, unsigned char *buf, long int size); // byte by byte
int  cb_flush(CBFILE **cbs);
int  cb_flush_to_offset(CBFILE **cbs, signed long int offset); // helper function to able cb_write_to_offset

int  cb_allocate_cbfile(CBFILE **buf, int fd, int bufsize, int blocksize); 
int  cb_allocate_buffer(cbuf **cbf, int bufsize);
int  cb_allocate_name(cb_name **cbn, int namelen);
int  cb_reinit_buffer(cbuf **buf); // zero contentlen, index and empties names
int  cb_empty_names(cbuf **buf); // frees names and zero namecount
int  cb_empty_names_from_name(cbuf **buf, cb_name **cbn); // free names from name and count names to namecount (leafs are not counted)
int  cb_empty_block(CBFILE **buf, char reading); // reading=1 to read (rewind) or 0 to append.
int  cb_reinit_cbfile(CBFILE **buf);
int  cb_free_cbfile(CBFILE **buf);
int  cb_free_buffer(cbuf **buf);
int  cb_free_name(cb_name **name);

int  cb_copy_name(cb_name **from, cb_name **to);
int  cb_compare(CBFILE **cbs, unsigned char **name1, int len1, unsigned char **name2, int len2, cb_match *ctl); // compares name1 to name2
int  cb_get_matchctl(CBFILE **cbs, unsigned char **pattern, int patsize, int options, cb_match *ctl, int matchctl); // compiles re in ctl from pattern (to use matchctl -7 and compile before), 12.4.2014

int  cb_set_rstart(CBFILE **str, unsigned long int rstart); // character between valuename and value, '='
int  cb_set_rend(CBFILE **str, unsigned long int rend); // character between value and next valuename, '&'
int  cb_set_cstart(CBFILE **str, unsigned long int cstart); // comment start character inside valuename, '#'
int  cb_set_cend(CBFILE **str, unsigned long int cend); // comment end character, '\n'
int  cb_set_bypass(CBFILE **str, unsigned long int bypass); // character to bypass next special character, '\\' (late, 14.12.2009)
int  cb_set_subrstart(CBFILE **str, unsigned long int subrstart); // If set to another, every second open pair uses these '='
int  cb_set_subrend(CBFILE **str, unsigned long int subrend); // If set to another, every second open pair uses these '&'
int  cb_set_search_state(CBFILE **str, unsigned char state); // CBSTATELESS, CBSTATEFUL, CBSTATETOPOLOGY, CBSTATETREE, ...
int  cb_set_encodingbytes(CBFILE **str, int bytecount); // 0 any, 1 one byte
int  cb_set_encoding(CBFILE **str, int number); 
int  cb_get_encoding(CBFILE **str, int *number); 

int  cb_use_as_buffer(CBFILE **buf); // file descriptor is not used
int  cb_use_as_file(CBFILE **buf);   // Namelist is bound by filesize
int  cb_use_as_seekable_file(CBFILE **buf);  // Additionally to previous, set seek function available (to read anywhere and write anywhere in between the file)
int  cb_use_as_stream(CBFILE **buf); // Namelist is bound by buffer size (namelist sets names length to buffer edge if endless namelist is needed)
int  cb_set_to_unique_names(CBFILE **cbf);
int  cb_set_to_polysemantic_names(CBFILE **cbf); // Multiple same names, default

/*
 * Helper queues and queue structures 
 */
// 4-byte character array
int  cb_get_ucs_chr(unsigned long int *chr, unsigned char **chrbuf, int *bufindx, int bufsize);
int  cb_put_ucs_chr(unsigned long int chr, unsigned char **chrbuf, int *bufindx, int bufsize);
int  cb_print_ucs_chrbuf(unsigned char **chrbuf, int namelen, int buflen);
int  cb_copy_ucs_chrbuf_from_end(unsigned char **chrbuf, int *bufindx, int buflen, int chrcount ); // copies chrcount or less characters to use as a new 4-byte block

// 4-byte fifo, without buffer or it's size
int  cb_fifo_init_counters(cb_ring *cfi);
int  cb_fifo_get_chr(cb_ring *cfi, unsigned long int *chr, int *size);
int  cb_fifo_put_chr(cb_ring *cfi, unsigned long int chr, int size);
int  cb_fifo_revert_chr(cb_ring *cfi, unsigned long int *chr, int *chrsize); // remove last put character
int  cb_fifo_set_stream(cb_ring *cfi); // all get operations return CBSTREAM after this
int  cb_fifo_set_endchr(cb_ring *cfi); // all get operations return CBSTREAMEND after this

// Debug
int  cb_fifo_print_buffer(cb_ring *cfi);
int  cb_fifo_print_counters(cb_ring *cfi);

// Debug
int  cb_print_name(cb_name **cbn);
int  cb_print_names(CBFILE **str);
int  cb_print_leaves(cb_name **cbn); // Prints inner leaves of values if CBSTATETREE was used. Not yet tested 5.12.2013.
void cb_print_counters(CBFILE **str);
int  cb_print_conf(CBFILE **str);

// Returns byte order marks encoding from two, three or four first bytes (bom is allways the first character)
int  cb_bom_encoding(CBFILE **cbs); // 26.7.2013
int  cb_write_bom(CBFILE **cbs); // 12.8.2013

// New encodings 10.8.2013, chr is in UCS-encoding
int  cb_put_utf16_ch(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes );
int  cb_get_utf16_ch(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes );
int  cb_put_utf32_ch(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes );
int  cb_get_utf32_ch(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes );

// Big and little endian functions
unsigned int  cb_reverse_four_bytes(unsigned int  from); // change four bytes order
unsigned int  cb_reverse_two_bytes(unsigned  int  from); // change two bytes order (first 16 bits are lost)
unsigned int  cb_reverse_int32_bits(unsigned int  from); // 32 bits
unsigned int  cb_reverse_int16_bits(unsigned int  from); // reverse last 16 bits and return them
unsigned char cb_reverse_char8_bits(unsigned char from); // 8 bits

unsigned int  cb_from_ucs_to_host_byte_order(unsigned int chr); // Returns reversed 4-bytes chr if run time cpu endianness is LE, otherwice chr
unsigned int  cb_from_host_byte_order_to_ucs(unsigned int chr); // The same

int  cb_test_cpu_endianness(void);

/*
 * Functions to use CBFILE in different purposes, only as a block by setting buffersize to 0. */
int  cb_allocate_cbfile_from_blk(CBFILE **buf, int fd, int bufsize, unsigned char **blk, int blklen);
int  cb_get_buffer(cbuf *cbs, unsigned char **buf, long int *size); // these can be used to get a block from blk when used as buffer
int  cb_get_buffer_range(cbuf *cbs, unsigned char **buf, long int *size, long int *from, long int *to); 
int  cb_free_cbfile_get_block(CBFILE **cbf, unsigned char **blk, int *blklen, int *contentlen);
