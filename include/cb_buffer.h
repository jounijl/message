/*
 * Library to read and write streams. Valuepair searchlist with different encodings.
 * 
 * Copyright (C) 2009, 2010, 2013, 2014, 2015 and 2016. Jouni Laakso
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

#define CBSUCCESS               0
#define CBSTREAM                1
#define CBFILESTREAM            2    // Buffer end has been reached but file is seekable
#define CBMATCH                 3    // Matched and the lengths are the same
#define CBUSEDASBUFFER          4
#define CBUTFBOM                5
#define CBMESSAGEHEADEREND      6
#define CBMESSAGEEND            7
#define CBMATCHLENGTH           8    // Matched the given length
#define CBADDEDNAME             9
#define CBADDEDLEAF            10
#define CBADDEDNEXTTOLEAF      11
#define CBMATCHMULTIPLE        12    // regexp multiple occurences 03/2014
#define CBMATCHGROUP           13    // regexp group 03/2014
#define CBSUCCESSLEAVESEXIST   14
//cb_json.h 27.5.2016 #define CBSUCCESSJSONQUOTES    15    // cb_read cb_copy_content returns this if JSON string, quotes removed
//cb_json.h 27.5.2016 #define CBSUCCESSJSONARRAY     16    // cb_read cb_copy_content returns this if JSON array

#define CBNEGATION             50
#define CBSTREAMEND            51
#define CBVALUEEND             52    // opencount < 0, CBSTATETREE and CBSTATETOPOLOGY
#define CBBUFFULL              53
#define CBNOTFOUND             54
#define CBNOTFOUNDLEAVESEXIST  55
#define CBNAMEOUTOFBUF         56
#define CBNOTUTF               57
#define CBNOENCODING           58
#define CBMATCHPART            59    // 30.3.2013, shorter name is the same as longer names beginning
#define CBEMPTY                60
#define CBNOTSET               61
#define CBAUTOENCFAIL          62    // First bytes bom was not in recognisable format
#define CBWRONGENCODINGCALL    63
#define CBUCSCHAROUTOFRANGE    64
#define CBREPATTERNNULL        65    // given pattern text was null 03/2014
#define CBGREATERTHAN          66    // same as not found with lexical comparison of name1 to name2
#define CBLESSTHAN             67
#define CBOPERATIONNOTALLOWED  68    // seek was asked and CBFILE was not set as seekable
#define CBNOFIT                69    // tried to write to too small space (characters left over)
#define CBNAMEVALUEWASREAD     70    // name value (all the leaves) had been already read (because the length of last leaf was >= 0 ), at the same time telling "not found"
#define CBLEAFVALUEWASREAD     71    // value (all the leaves) had been already read to last rend '&' 
#define CBMISMATCH             72
#define CBLENGTHNOTKNOWN       73
#define CBREWASNULL            74
#define CBSTREAMEAGAIN         75    // if file descriptor was configured to use non-blocking io, read returns CBSTREAMEAGAIN if data was not available and may be available later, 20.5.2016
#define CBENDOFFILE            76
//#define CBNOTJSON              75
//#define CBNOTJSONVALUE         76
//#define CBNOTJSONSTRING        77
//#define CBNOTJSONBOOLEAN       78
//#define CBNOTJSONNUMBER        79
//#define CBNOTJSONARRAY         80
//#define CBNOTJSONOBJECT        81
//#define CBNOTJSONNULL          82

#define CBERROR	              200
#define CBERRALLOC            201
#define CBERRFD               202
#define CBERRFILEOP           203
#define CBERRFILEWRITE        204
#define CBERRBYTECOUNT        205
#define CBARRAYOUTOFBOUNDS    206
#define CBINDEXOUTOFBOUNDS    207
#define CBLEAFCOUNTERROR      208
#define CBERRREGEXCOMP        209    // regexp pattern compile error 03/2014
#define CBERRREGEXEC          210    // regexp exec error 03/2014
#define CBBIGENDIAN           211
#define CBLITTLEENDIAN        212
#define CBUNKNOWNENDIANNESS   213
#define CBOVERFLOW            214
#define CBOVERMAXNAMES        215    // 19.10.2015
#define CBOVERMAXLEAVES       216    // 19.10.2015
//#define CBINVALIDTRANSENC     217    // 28.5.2016
//#define CBINVALIDTRANSEXT     218    // 28.5.2016

/*
 * Log priorities (log verbosity).
 */
#define CBLOGEMERG              1
#define CBLOGALERT              2
#define CBLOGCRIT               3
#define CBLOGERR                4
#define CBLOGWARNING            5
#define CBLOGNOTICE             6
#define CBLOGINFO               7
#define CBLOGDEBUG              8

#define CBDEFAULTLOGPRIORITY    8

/*
 * Default values (multiple of 4)
 */
#define CBNAMEBUFLEN         4096
#define CBREADAHEADSIZE        28
#define CBSEEKABLEWRITESIZE   128

/*
 CBREADAHEADSIZE is used to read RFC-822 unfolding characters. It's size should be small
 to use the CBFILE in other purposes without too much overhead and to avoid reading
 too much before without a cause. Size is multiple of 4 (4-bytes characters).
 */

/*
 * Proportional to maximum memory would be better than static defines:
 */

// TEST 22.10.2015, 27.10.2015:
#define CBMAXNAMES          2147480000
#define CBMAXLEAVES         2147480000

/*
 * Memory has to be restricted otherwice. Numbers below should be maximum size of an unsigned integer. */
//#define CBMAXNAMES          10000        // Tested value 10000 with max leaves 4 19.10.2015 (CBSTATETREE) (*next is not anymore unbounded, it's counted)
//#define CBMAXLEAVES         10000        // If CBSTATETREE is set, count of leaves one name can have (*next is unbounded and uncounted). 
                                         // (this can be set to a maximum number of unsigned integer)

#define CBRESULTSTART       '='
#define CBRESULTEND         '&'
#define CBBYPASS            '\\'
#define CBCOMMENTSTART      '#'  // Allowed inside valuename from rend to rstart
#define CBCOMMENTEND        '\n'

#define CBSUBRESULTSTART    '='  // If set to another, in CBSTATETREE, swithes every other separator (every odd separator)
#define CBSUBRESULTEND      '&'  // 

/*
 * Collect benchmark data.
 */
#define CBBENCHMARK
//#undef CBBENCHMARK

/*
 * Configuration options
 */

/*
 * Search options */
#define CBSEARCHUNIQUENAMES       0   // Names are unique (returns allways the first name in list)
#define CBSEARCHNEXTNAMES         1   // Multiple same names (returns name if matchcount is zero, otherwice searches next in list or in stream)
#define CBSEARCHUNIQUELEAVES      0   // Leaves are unique (returns allways the first name in list)
#define CBSEARCHNEXTLEAVES        1   // Multiple same leaves (returns leaf if matchcount is zero, otherwice searches next in name value, also in stream)
#define CBSEARCHNEXTGROUPNAMES    2   // Next names with matching to group number
#define CBSEARCHNEXTGROUPLEAVES   4   // Next leaves with matching to group number

/**
 CBSEARCHNEXTGROUP 11.11.2016
        Matches either the same group number again or a new attribute not yet matched. A group of attributes
        can be set to belong to a group and found with the group number.
 **/


/*
 * Third search option is to leave buffer empty amd search allways the next name, with either above setting.
 */

/*
 * Use options, changes functionality as below */
#define CBCFGSTREAM            0x00   // Use as stream (namelist is bound by buffer)
#define CBCFGBUFFER            0x01   // Use only as buffer (fd is not used at all)
#define CBCFGFILE              0x02   // Use as file (namelist is bound by file),
                                      // cb_reinit_buffer empties names but does not seek, use: lseek((**cbf).fd if needed
#define CBCFGSEEKABLEFILE      0x03   // Use as file capable of seeking the cursor position - buffer is partly truncated and block is emptied after every offset read or write
#define CBCFGBOUNDLESSBUFFER   0x04   // Endless namelist and buffersize can be zero. Use of only block is possible.

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
//#define CB2822MESSAGE
#define CBMESSAGEFORMAT

/*
 * Unicode chart U0080 EOF 0x00FF */
#define CEOF     ( (unsigned long int) 0x00FF )

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

/* JSON control characters if JSON is used, http://www.json.org . */
#define JSON_CTRL( chr )                       ( chr=='"' || chr=='\\' || chr=='/' || chr=='b' || chr=='f' || chr=='n' || chr=='r' || chr=='t' || chr=='u' )
#define JSON_HEX( chr )                        ( ( chr>='0' && chr<='9' ) || ( chr>='A' && chr<='F' ) || ( chr>='a' && chr<='f' ) )

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

#ifdef CBBENCHMARK
typedef struct cb_benchmark {
	long long int reads;       // character reads (cb_get_chr)
	long long int bytereads;   // bytes reads (cb_get_ch)
} cb_benchmark;
#endif

/*
 * Reader function state to update the level of the tree
 * correctly in cb_get_chr. A method to communicate with
 * the reader function.  29.9.2015 
 *
 * 7.10.2015: saves last read character. How the value is read,
 * should not be restricted. If variables preserving the state
 * is needed, they should be in cb_read.
 */
typedef struct cb_read {
	/* 
	 * Read state variables can be moved here if the state can be detached from set_cursor_... 
	 * and moved to some reading function. */
	unsigned long int  lastchr; // If value is read, it is read to the last rend. This is needed only in CBSTATETREE, not in other searches.
	long int           lastchroffset;
	unsigned char      lastreadchrendedtovalue;
	unsigned char      padto64bit[7];
} cb_read;

/*
 * Compare ctl */
typedef struct cb_match {
        int matchctl; // If match function is not regexp match, next can be NULL
	int resmcount; // Place to save the matchcount by cb_compare. If matchctl -9 or -10 is used, this value can be used to evaluate the result of the comparison.
        void *re; // cast to pcre2_code (void here to be able to compile the files without pcre)
} cb_match;

/*
 * Fifo ring structure */
typedef struct cb_ring {
	char padto64bit[6];        // compiler warnings, pad to next storage size (32 bit)
        unsigned char buf[CBREADAHEADSIZE+1];
        unsigned char storedsizes[CBREADAHEADSIZE+1];
	signed long int currentindex;     // 28.7.2015, place of last read character to remember it the next time
        int buflen;
        int sizeslen;
        int ahead;            // bytes in UCS encoding (usually bigger because a character is 4 bytes)
        int bytesahead;       // bytes in transfer encoding (usually smaller, for example one byte encoding)
        int first;
        int last;
	int streamstart;      // id of first character from stream
	int streamstop;       // id of stop character
} cb_ring;

typedef struct cb_conf {
	/* File type */
        unsigned char       type:4;                        // stream (default), file (large namelist), only buffer (fd is not in use) or seekable file (large namelist and offset operations), buffer with boundless namelist size
	/* Search settings */
        unsigned char       searchmethod:4;                // search next name (multiple names) or search allways first name (unique names), CBSEARCH*

        unsigned char       leafsearchmethod:4;            // search leaf name (multiple leaves) or search allways first leaf (unique leaves), CBSEARCH*
	unsigned char       searchstate:4;                 // No states = 0 (CBSTATELESS), CBSTATEFUL, CBSTATETOPOLOGY, CBSTATETREE

        unsigned char       unfold:1;                      // Search names unfolding the text first, RFC 2822
	unsigned char       leadnames:1;                   // Saves names from inside values, from '=' to '=' and from '&' to '=', not just from '&' to '=', a pointer to name name1=name2=name2value (this is not in use in CBSTATETOPOLOGY and CBSTATETREE).
	unsigned char       findleaffromallnames:1;        // Find leaf from all names (1) or from the current name only (0). If levels are less than ocoffset, stops with CBNOTFOUND. Not tested yet 27.8.2015.
	unsigned char       doubledelim:1;                 // When using CBSTATETREE, after every second openpair, rstart and rstop are changed to another
	unsigned char       findwords:1;                   // <rend>word<rstart>imaginary record<rend> ... . Compare WSP, CR, NL and rstart characters and not only rstart characters in order to find a word starting with a rend character. Only CBSTATEFUL should be used because every SP or TAB would alter the height information of the tree. (This time the word only is used and not the value or the record.) 
	unsigned char       findwordstworends:1;           // wordlist with two rend-characters.
	unsigned char       searchnameonly:1;              // Find only one named name. Do not save the names in the tree or list. Return if found. 4.2.2016
	unsigned char       namelist:1;                    // Setting to save the last name of namelist at the end of the buffer: "name1, name2, name3", see cb_set_to_name_list_search
	/* Name parsing options */
	unsigned char       removenamewsp:1;               // Remove white space characters inside name

        unsigned char       asciicaseinsensitive:1;        // Names are case insensitive, ABNF "name" "Name" "nAme" "naMe" ..., RFC 2822
        unsigned char       scanditcaseinsensitive:1;      // Names are case insensitive, ABNF "nämä" "Nämä" "nÄmä" "näMä" ..., RFC 2822 + scandinavic letters
        unsigned char       removewsp:1;                   // Remove linear white space characters (space and htab) between value and name (not RFC 2822 compatible)
        unsigned char       removeeof:1;                   // Remove EOF from name, 26.8.2016
        unsigned char       removecrlf:1;                  // Remove every CR:s and LF:s between value and name (not RFC 2822 compatible) and in name
        unsigned char       removesemicolon:1;             // Remove semicolon between value and name (not RFC 2822 compatible) 
	unsigned char       removecommentsinname:1;        // Remove comments inside names (JSON can't do this, it does not have comments)
	/* Value parsing options */
	unsigned char       urldecodevalue:1;              // If set, cb_read.c decodes key-value pairs value in cb_copy_content (all read functions).

	/* JSON options */
	unsigned char       json:1;                        // When using CBSTATETREE, form of data is JSON compatible (without '"':s and '[':s in values), also doubledelim must be set
	unsigned char       jsonnamecheck:2;               // Check the form of the name of the JSON attribute (in cb_search.c).
        unsigned char       jsonremovebypassfromcontent:2; // Normal allways, removes '\' in front of '"' and '\"
	unsigned char       jsonvaluecheck:2;              // When reading (with cb_read.h), check the form of the JSON values, 10.2.2016.
	//unsigned char       jsonallownlhtcr:1;             // Allow JSON control characters \t \n \r in text as they are and remove them from content, 24.10.2016

	/* Log options */
	unsigned char       logpriority:5;                 // Log output priority (one of from CBLOGEMERG to CBLOGDEBUG)
	/* Read methods */
	unsigned char       usesocket:1;                   // Read only headeroffset and messageoffset length blocks
	unsigned char       nonblocking:2;                 // (do not use) fd is set to O_NONBLOCK and the reading is set similarly, O_NONBLOCKING not tested yet, 23.5.2016 - reading may stop in between key-value -pair, do not use O_NONBLOCK.

	/* Stream end recognition */
	unsigned char       stopatbyteeof:1;               // Recognize CBSTREAMEND at EOF character. Default is 1. Every octet disregarding the character code.
	unsigned char       stopateof:1;                   // Recognize CBENDOFFILE at EOF character. 
	unsigned char       stopafterpartialread:2;        // If reading a block only part was returned, returns CBSTREAMEND and the next time before reading the next block. This one can be used if the reading would block and if the source is known, for example a file with 0xFF -characters.
        unsigned char       stopatheaderend:2;             // Stop after RFC 2822 header end (<cr><lf><cr><lf>)
        unsigned char       stopatmessageend:2;            // Stop after RFC 2822 message end (<cr><lf><cr><lf>), the end has to be set with a function (currently 10.6.2016: only messageoffset is used, not <cr><lf><cr><lf> sequence)

	unsigned char       pad[1];                        // pad to next integer size (word length 32 or 64, now 64)

	unsigned long int   rstart;          // Result start character
	unsigned long int   rend;            // Result end character
	unsigned long int   bypass;          // Bypass character, bypasses next special characters function
	unsigned long int   cstart;          // Comment start character (comment can appear from rend to rstart)
	unsigned long int   cend;            // Comment end character

	unsigned long int   subrstart;       // If CBSTATETREE and doubledelim is set, rstart of every other openpairs (odd), subtrees delimiters
	unsigned long int   subrend;         // If CBSTATETREE and doubledelim is set, rend of every other openpairs (odd), subtrees delimiters 
} cb_conf; 

// signed to detect overflow

typedef struct cb_name{
        unsigned char        *namebuf;        // name
	int                   buflen;         // name+excess namebuffer space
        int                   namelen;        // name length
        signed long int       offset;         // offset from the beginning of data (to '=')
        signed long int       nameoffset;     // offset of the beginning of last data (after reading '&'), 6.12.2014
        int                   length;         // character count, length of namepairs area, from previous '&' to next '&', unknown (almost allways -1), possibly empty, set after it's known
        signed long int       matchcount;     // if CBSEARCHNEXT, increases by one when mathed, zero only if name is not searched yet
	int                   group;          // May be set from outside if needed. A group id to mark attributes to belong together.
        void                 *next;           // Last is NULL {1}{2}{3}{4}
	signed long int       firsttimefound; // Time in seconds the name was first found and/or used (set by set_cursor)
	signed long int       lasttimeused;   // Time in seconds the name was last searched or used (set by set_cursor)
	void                 *leaf;           // { {1}{2} { {3}{4} } } , last is NULL
} cb_name;

typedef struct cb_namelist{
        cb_name              *name;
	cb_name              *current;
	cb_name              *last;
	signed long long int  namecount;        // names
	signed long long int  nodecount;        // names and leaves, 28.10.2015
	int                   currentgroup;     // number of the group currently in use
	int                   groupcount;       // number of the last group
	signed int            toterminal;       // 29.9.2015. Number of closing brackets after the last leaf. Addition of a new name or leaf zeroes this.
	cb_read               rd;
	cb_name              *currentleaf;      // 9.12.2013, sets as null every time 'current' is updated
} cb_namelist;

typedef struct cbuf{
        unsigned char           *buf;
        signed long int          buflen;               // In bytes
	signed long int          index;                // Cursor offset in bytes (in buffer)
	signed long int          contentlen;           // Bytecount in numbers (first starts from 1), comment: 7.11.2009
	signed long int          readlength; 	       // CBSEEKABLEFILE: Read position in bytes (from filestream), 15.12.2014 Late addition: useful with seekable files (useless when appending or reading).
	signed long int          maxlength;            // CBSEEKABLEFILE: Overall read length in bytes (from filestream), 15.12.2014 Late addition: useful with seekable files (useless when appending or reading).
	cb_namelist              list;
        int                      headeroffset;         // offset of RFC-2822 header end with end characters (offset set at last new line character)
        signed long int          messageoffset;        // offset of RFC-2822 message end from "Content-Length:". This has to be set from outside after reading the value, cb_set_message_end.
	signed long int          chunkmissingbytes;    // Bytes missing from previous read of chunk, 28.5.2016.
	signed long int          eofoffset;            // Cursor offset of first EOF in UCS 32-bit format to test if bytes are left to read, 30.10.2016
	char                     lastblockreadpartial; // If the previous reading of a block was not full (excepting the end of stream after this block)
	char                     padto64bit[7];
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
	int                 transferencoding; // Transferencoding
	int                 transferextension; // Optional transfer encoding extension
#ifdef CBBENCHMARK
	cb_benchmark        bm;
#endif
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
 * Match functions:
 *
 * The same functions overloaded with match length of the name. These are 
 * useful internally and in some searches. 
 *
 * matchctl  1 - strict match, CBMATCH
 * matchctl  0 - searches any next name not yet used, once. Note: if CBSEARCHUNIQUENAMES is set, the next attribute is allways the same. Use CBSEARCNEXTNAMES with this.
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
 * ocoffset:
 * Open pairs. If in value and ocoffset is 0, leafs ? otherwice allways 1 or the ocoffset level ? (questions 10.7.2015)
 *
 * Lengths, 22.8.2015:
 * set_cursor updates lengths when they are known. If openpairs is increasing, the leafs length can not be updated.
 * Name length in list can be updated. Some leafs are not given length (upward slope). Length may be different (22.8.2015)
 * if the next name in list is not yet read. This may affect writing to the values space or to the names space.
 *
 * Leaves:
 *
 * 1) First search the name
 * 2) Leaf is searched from the current
 *
 * 2.10.2015: still here: 1,5) Search the next name, return to the previous name by searching it again and move on to 2)
 *
 * All leaves have to be read by finding the next name from list first (main list, ocoffset 0)
 * if multiple names are searched in sequence. All leaves have to be in the tree before finding
 * the leaves from the tree (25.7.2015).
 *
 * Example: ocoffset 0, matchctl 0 searches any next name in list, not a leaf (25.7.2015).
 *
 * In CBSTATETREE and CBSTATETOPOLOGY, ocoffset updates openpairs -count. The reading stops
 * when openpairs is negative. Next rend after value ends reading. This parameter can be used
 * to read leaves inside values. Currentleaf is updated if leaf is found with depth ocoffset.
 *
 * Library is a list. Leafs are an added functionality. Searching of every leaf is started
 * from the beginning of the current name in the list.
 *
 * Function reads all leafs to the tree from the value if CBSTATETREE is set. Match is from 
 * the matching ocoffset level, for example if ocoffset is set to 1: 
 * level
 * 3                 +leftleaf+rightleaf+rightleaf
 * 2            +leftleaf    
 * 1         +leaf+rightleaf  <----- matching leafs with ocoffset 1, first 'leaf', then 'rightleaf' if searched two times or the names are known
 * 0  __listnode1________________________________listnode2__
 *
 * In searching a leaf, the previous leaf has to be known and the currentleaf has to be at the previous
 * to return the next leaf at the next ocoffset level. It is advisable to read the lists names value
 * once with it's leaves. If the reading of leaves is stopped and some leaves are not read, the cursor
 * should be returned to the start of the previous name in the list and start the reading of the leaves
 * to restore the state of reading the leaves.
 *
 * Return values:
 *
 * Returns on success: CBSUCCESS, CBSUCCESSLEAVESEXIST, CBSTREAM, CBFILESTREAM (only if CBCFGSEEKABLEFILE is set) or
 * CBMESSAGEHEADEREND or CBMESSAGEEND if they were set.
 * May return: CBNOTFOUND, CBVALUEEND, CBSTREAMEND, CBENDOFFILE (if stopateof was set)
 * Possible errors: CBERRALLOC, CBOPERATIONNOTALLOWED (offsets)
 * cb_search_get_chr: CBSTREAMEND, CBNOENCODING, CBNOTUTF, CBUTFBOM, CBSTREAMEAGAIN (26.5.2016)
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
 * Write to offset if file is seekable. (Block is replaced if it's smaller or not empty.) These are not thread-safe without locks. */
int  cb_write_to_offset(CBFILE **cbf, unsigned char **ucsbuf, int ucssize, int *byteswritten, signed long int offset, signed long int offsetlimit);
int  cb_character_size(CBFILE **cbf, unsigned long int ucschr, unsigned char **stg, int *stgsize); // character after writing (to erase), slow and uses a lot of memory
int  cb_erase(CBFILE **cbf, unsigned long int chr, signed long int offset, signed long int offsetlimit); // If not even at end, does not fill last missing characters
int  cb_reread_file( CBFILE **cbf ); // 16.2.2015
int  cb_reread_new_file( CBFILE **cbf, int newfd ); // 16.2.2015 


// From unicode to and from utf-8
int  cb_get_ucs_ch(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes );
int  cb_put_ucs_ch(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes );
// Transfer encoding
int  cb_get_utf8_ch(CBFILE **cbs, unsigned long int *chr, unsigned long int *chr_high, int *bytecount, int *storedbytes );
int  cb_put_utf8_ch(CBFILE **cbs, unsigned long int *chr, unsigned long int *chr_high, int *bytecount, int *storedbytes );

/*
 * Unfolds read characters. Characters are read by cb_get_chr.
 * cb_ring contains readahead buffer used in folding. 
 *
 * The ring buffer in CBFILE is reserved to the use of the
 * cb_set_cursor_... functions. Parameter ahd should be allocated
 * elsewhere to use this function.
 *
 * It is possible to implement an own unfolding function. There
 * are macros to use in helping to do this, the LWSP( x ) macro. 
 *
 * The position after the name does not have unfolding characters. 
 * The unfolding buffer should be empty after the name. The same
 * with the reading of the value. After the value at 'rend', there
 * should be no unfolding characters and the unfolding buffer
 * should be empty.
 *
 * Not tested in using elsewhere than in cb_set_cursor_... 
 * functions, 3.8.2015.
 */
int  cb_get_chr_unfold(CBFILE **cbs, cb_ring *ahd, unsigned long int *chr, long int *chroffset);


// Data
int  cb_get_ch(CBFILE **cbs, unsigned char *ch);
int  cb_put_ch(CBFILE **cbs, unsigned char ch); // *ch -> ch 12.8.2013
int  cb_write_cbuf(CBFILE **cbs, cbuf *cbf);
int  cb_write(CBFILE **cbs, unsigned char *buf, long int size); // byte by byte
int  cb_flush(CBFILE **cbs);
int  cb_flush_to_offset(CBFILE **cbs, signed long int offset); // helper function to able cb_write_to_offset
int  cb_transfer_read( CBFILE **cbf, signed long int readlength );
int  cb_transfer_write( CBFILE **cbf );
int  cb_terminate_transfer( CBFILE **cbf ); // send termination sequence, for example 1*("0") [ chunk-extension ] CRLF

int  cb_allocate_cbfile(CBFILE **buf, int fd, int bufsize, int blocksize); 
int  cb_allocate_buffer(cbuf **cbf, int bufsize);
int  cb_allocate_name(cb_name **cbn, int namelen);
int  cb_init_name( cb_name **cbn  ); // initialize name variables, 22.7.2016
int  cb_reinit_buffer(cbuf **buf); // zero contentlen, index and empties names
int  cb_empty_block(CBFILE **buf, char reading); // reading=1 to read (rewind) or 0 to append.
int  cb_reinit_cbfile(CBFILE **buf);
int  cb_free_cbfile(CBFILE **buf);
int  cb_free_buffer(cbuf **buf);
int  cb_empty_names(cbuf **buf); // frees names and zero cbuf namecount
int  cb_empty_names_from_name(cbuf **buf, cb_name **cbn); // free names from name and cbuf is updated 
int  cb_free_name(cb_name **name, int *freecount); 
int  cb_free_names_from(cb_name **cbn, int *freecount);

int  cb_copy_name(cb_name **from, cb_name **to);
int  cb_set_leaf_unread( cb_name **cbname ); // set every leaf as unread from the first leaf
int  cb_set_list_unread( cb_name **cbname );  // set recursively all *next and *leaf matchcount to -1
int  cb_set_attributes_unread( CBFILE **cbs ); // sets matchount to -1 to all names in cbs
int  cb_compare(CBFILE **cbs, unsigned char **name1, int len1, unsigned char **name2, int len2, cb_match *ctl); // compares name1 to name2
int  cb_get_matchctl(CBFILE **cbs, unsigned char **pattern, int patsize, unsigned int options, cb_match *ctl, int matchctl); // compiles re in ctl from pattern (to use matchctl -7 and compile before), 12.4.2014

/* Name structure type. */
//int  cb_check_json_string( unsigned char **ucsname, int namelength, int *from ); // from is the index in 4-byte ucsname in bytes
//int  cb_check_json_string_content( unsigned char **ucsname, int namelength, int *from);

int  cb_set_rstart(CBFILE **str, unsigned long int rstart); // character between valuename and value, '='
int  cb_set_rend(CBFILE **str, unsigned long int rend); // character between value and next valuename, '&'
int  cb_set_cstart(CBFILE **str, unsigned long int cstart); // comment start character inside valuename, '#'
int  cb_set_cend(CBFILE **str, unsigned long int cend); // comment end character, '\n'
int  cb_set_bypass(CBFILE **str, unsigned long int bypass); // character to bypass next special character, '\\' (late, 14.12.2009)
int  cb_set_subrstart(CBFILE **str, unsigned long int subrstart); // If set to another, every second open pair uses these '='
int  cb_set_subrend(CBFILE **str, unsigned long int subrend); // If set to another, every second open pair uses these '&'
int  cb_set_search_state(CBFILE **str, unsigned char state); // CBSTATELESS, CBSTATEFUL, CBSTATETOPOLOGY, CBSTATETREE, ...
int  cb_set_to_nonblocking(CBFILE **cbf); // read and write with flag O_NONBLOCK, not yet tested, 23.5.2016
int  cb_set_encodingbytes(CBFILE **str, int bytecount); // 0 any, 1 one byte
int  cb_set_encoding(CBFILE **str, int number); 
int  cb_get_encoding(CBFILE **str, int *number); 
int  cb_set_transfer_encoding(CBFILE **str, int number);
int  cb_set_transfer_extension(CBFILE **str, int number);

/* Functions to remember the settings (see the source if forgotten). */
int  cb_set_to_json( CBFILE **str ); // Sets doubledelim, json, jsonnamecheck, rstart, rend, substart, subrend, cstart, cend, UTF-8 and CBSTATETREE.
int  cb_set_to_conf( CBFILE **str ); // Sets doubledelim, CBSTATETREE, unique names, zeroes other options and sets default values of rstart, rend, substart, subrend, cstart and cend.
int  cb_set_to_html_post( CBFILE **str ); // Post attributes with alphanumeric characters and percent encoding for others, no folding, case sensitive
int  cb_set_to_html_post_text_plain( CBFILE **str ); // HTML POST content type text/plain, 9.8.2016
int  cb_set_to_message_format( CBFILE **str ); // Remove CR. Sets new line as rend, rstart ':', folding, ending at header end, (ASCII) case insensitive names, comments as '(' and ')'.
int  cb_set_to_word_search( CBFILE **str ); // Find a word list with words from $ ending with ',' or with the following special characters. Not usable with trees because words can end to SP, TAB, CR or LF.
int  cb_set_to_name_list_search( CBFILE **str ); // Find names without values separated by rend (default is ','): name1, name2, name3, ...
int  cb_set_to_search_one_name_only( CBFILE **str ); // Find one name only ending at SP, TAB, CR or LF and never save any names to a list or tree.

int  cb_set_message_end( CBFILE **str, long int contentoffset );
int  cb_test_message_end( CBFILE **cbs );
int  cb_test_header_end( CBFILE **cbs );
int  cb_test_eof_read( CBFILE **cbs );

int  cb_use_as_buffer(CBFILE **buf); // File descriptor is not used
int  cb_use_as_file(CBFILE **buf);   // Namelist is bound by filesize
int  cb_use_as_seekable_file(CBFILE **buf);  // Additionally to previous, set seek function available (to read anywhere and write anywhere in between the file)
int  cb_use_as_stream(CBFILE **buf); // Namelist is bound by buffer size (namelist sets names length to buffer edge if endless namelist is needed)
int  cb_use_as_boundless_buffer(CBFILE **buf); // Namelist is not bound and file descriptor is not used.

int  cb_set_to_socket( CBFILE **str ); // Any from above and usesocket=1

int  cb_set_to_unique_names(CBFILE **cbf);
int  cb_set_to_unique_leaves(CBFILE **cbf);
int  cb_set_to_consecutive_names(CBFILE **cbf); // Searches multiple same named names, default
int  cb_set_to_consecutive_leaves(CBFILE **cbf); // Searches multiple same named leaves, default
int  cb_set_to_consecutive_group_names(CBFILE **cbf); // Searches multiple same named names. If same name is found, the one already in group is used.
int  cb_set_to_consecutive_group_leaves(CBFILE **cbf); // Searches multiple same named leaves. If same name is found the one already in group is used.

/*
 * To use the group number of CBSEARCHNEXTGROUPNAMES and CBSEARCHNEXTGROUPLEAVES */
int  cb_set_group_number( CBFILE **cbf, int grp );
int  cb_increase_group_number( CBFILE **cbf );

/*
 * Helper queues and queue structures 
 */
// 4-byte character array
int  cb_get_ucs_chr(unsigned long int *chr, unsigned char **chrbuf, int *bufindx, int bufsize);
int  cb_put_ucs_chr(unsigned long int chr, unsigned char **chrbuf, int *bufindx, int bufsize);
int  cb_print_ucs_chrbuf(char priority, unsigned char **chrbuf, int namelen, int buflen);
int  cb_copy_ucs_chrbuf_from_end(unsigned char **chrbuf, int *bufindx, int buflen, int chrcount ); // copies chrcount or less characters to use as a new 4-byte block

// 4-byte fifo, without buffer or it's size
int  cb_fifo_init_counters(cb_ring *cfi);
int  cb_fifo_get_chr(cb_ring *cfi, unsigned long int *chr, int *size);
int  cb_fifo_put_chr(cb_ring *cfi, unsigned long int chr, int size);
int  cb_fifo_revert_chr(cb_ring *cfi, unsigned long int *chr, int *chrsize); // remove last put character
int  cb_fifo_set_stream(cb_ring *cfi); // all get operations return CBSTREAM after this
int  cb_fifo_set_endchr(cb_ring *cfi); // all get operations return CBSTREAMEND after this

// Debug
int  cb_fifo_print_buffer(cb_ring *cfi, char priority);
int  cb_fifo_print_counters(cb_ring *cfi, char priority);

// Debug
int  cb_print_name(cb_name **cbn, char priority);
int  cb_print_names(CBFILE **str, char priority);
int  cb_print_leaves(cb_name **cbn, char priority); // Prints inner leaves of values if CBSTATETREE was used. Not yet tested 5.12.2013.
void cb_print_counters(CBFILE **str, char priority);
int  cb_print_conf(CBFILE **str, char priority);
#ifdef CBBENCHMARK
int  cb_print_benchmark(cb_benchmark *bm);
#endif

// Log writing (as in fprintf)
int  cb_log( CBFILE **cbn, char priority, int errtype, const char * restrict format, ... ) __attribute__ ((format (printf, 4, 5))); // https://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html
int  cb_clog( char priority, int errtype, const char * restrict format, ... ) __attribute__ ((format (printf, 3, 4))); 

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
int  cb_reinit_cbfile_from_blk( CBFILE **cbf, unsigned char **blk, int blksize );
int  cb_get_buffer(cbuf *cbs, unsigned char **buf, long int *size); // these can be used to get a block from blk when used as a buffer
int  cb_get_buffer_range(cbuf *cbs, unsigned char **buf, long int *size, long int *from, long int *to); 
int  cb_free_cbfile_get_block(CBFILE **cbf, unsigned char **blk, int *blklen, int *contentlen);

