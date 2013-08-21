/* 
 * Library to read and write streams. Valuepair indexing with different character encodings.
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

#define CBSUCCESS            0
#define CBSTREAM             1
#define CBMATCH              2
#define CBSTREAMEDGE         3
#define CBUSEDASBUFFER       4
#define CBUTFBOM             5
#define CB2822HEADEREND      6
//#define CB2822MESSAGE        7

#define CBNEGATION          10
#define CBSTREAMEND         11
#define CBBUFFULL           12
#define CBNOTFOUND          13
#define CBNAMEOUTOFBUF      14
#define CBNOTUTF            15
#define CBNOENCODING        16
#define CBMATCHPART         17    // 30.3.2013, shorter name is the same as longer names beginning
#define CBEMPTY             18
#define CBWRONGENCODINGCALL 19
#define CBUCSCHAROUTOFRANGE 20

#define CBERROR	            30
#define CBERRALLOC          31
#define CBERRFD             32
#define CBERRFILEOP         33
#define CBERRFILEWRITE      34
#define CBERRBYTECOUNT      35

/*
 * Default values
 */
#define CBNAMEBUFLEN        1024
#define CBRESULTSTART       '='
#define CBRESULTEND         '&'
#define CBBYPASS            '\\'
#define CBCOMMENTSTART      '#'  // Allowed inside valuename from rend to rstart
#define CBCOMMENTEND        '\n'

/*
 * Configuration options
 */
#define CBSEARCHFIRST        0   // Names are unique (returns allways the first name in list)
#define CBSEARCHNEXT         1   // Multiple names (returns name if matchcount is zero, otherwice searches next in list or in stream)

#define CBCFGSTREAM          0   // Use as stream (namelist is bound by buffer)
#define CBCFGBUFFER          1   // Use only as buffer (fd is not used at all)
#define CBCFGFILE            2   // Use as file (namelist is bound by file),
                                 // cb_reinit_buffer empties names but does not seek, use: lseek((**cbf).fd if needed

/*
 * This setting enables multibyte, UTF-16 and UTF-32 support if
 * the machine processor has big endian words (for example PowerPC, ARM, 
 * Sparc and some Motorola processors). Multibyte does not yet function
 * properly 11.8.2013.
 */
//#undef BIGENDIAN

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
//#define CBSTATETOPOLOGY

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
//#define CBSTATEFUL

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
//#undef CBSTATEFUL
//#undef CBSTATETOPOLOGY

/*
 * With this define, cb_set_cursor stops searching the stream when it encounters
 * a character sequence at header end, between header and message.
 * In RFC-822 and RFC-2822 this is two sequential <cr><lf> characters.
 */
#define CBSTOPAT2822HEADEREND

/*
 * This is not used. LWS was used in folding, this might not be exactly it. It was used
 * in removing characters from name. CR LF Space Tab (RFC 5198: CR can appear only 
 * followed by LF)
 */
#define LWS( x )              ( x == 0x0D && x == 0x0A && x == 0x20 && x == 0x09 )

/*
 * Characters to remove from name: CR LF Space Tab and BOM. Comment string is removed 
 * elsewhere. UTF-8: "Should be stripped ...", RFC-3629 page-6, UTF-16:
 * http://www.unicode.org/L2/L2005/05356-utc-bomsig.html
 */
#define NAMEXCL( x )        ( x == 0x0D && x == 0x0A && x == 0x20 && x == 0x09 && x == 0xFEFF )

#include "./cb_encoding.h"

typedef struct cb_conf{
        char                type;         // stream (default), file or only buffer
        char                searchmethod; // search next name (multiple names) or search allways first name (unique names)
} cb_conf; // 20.8.2013

typedef struct cb_name{
        unsigned char      *namebuf;      // name 
	int                 buflen;       // name+excess buffer space
        int                 namelen;      // name length
        long int            offset;       // offset from the beginning of data
        int                 length;       // unknown (almost allways -1) (length of data), possibly set after it's known
        long int            matchcount;   // if CBSEARCHNEXT, increases by one when traversed by, zero only if name is not searched yet
        void               *next;         // Last is NULL
} cb_name;

typedef struct cbuf{
        unsigned char      *buf; 
        long int            buflen;       // In bytes
	long int            index;
	long int            contentlen;   // Count in numbers (first starts from 1), comment: 7.11.2009
        cb_name            *name;
	cb_name            *current;
	cb_name            *last;
	long int            namecount;
#ifdef CBSTOPAT2822HEADEREND
        int                 offset2822;   // offset of RFC-2822 header end with end characters
#endif
} cbuf;

typedef struct cbuf cblk;

typedef struct CBFILE{
	int                 fd;	// Stream file descriptor
	cbuf               *cb;	// Data in valuepairs (preferably in applications order)
	cblk               *blk;	// Input read or output write -block 
	//int               onlybuffer; // If fd is not in use
        cb_conf             cf;         // All configurations, 20.8.2013
	unsigned long int   rstart;	// Result start character
	unsigned long int   rend;	// Result end character
	unsigned long int   bypass;	// Bypass character, bypasses next special characters function
	unsigned long int   cstart;	// Comment start character (comment can appear from rend to rstart)
	unsigned long int   cend;	// Comment end character
	int                 encodingbytes; // Maximum bytecount for one character, 0 is any count, 4 is utf low 6 utf normal, 1 any one byte characterset
	int                 encoding;   // list of encodings is in file cb_encoding.h 
} CBFILE;

/*
 * name is a namelength size pointer to unsigned char array to
 * search and set cursor to.
 *
 * Set cursor to position name or return CBNOTFOUND or CBSTREAM.
 * 'bufsize' buf is saved and other data is read as a stream or
 * lost. Cursor is allways at the end of read buffer and characters
 * are read from cursor. Names are read from cb_set_cursor.
 *
 * Bypasses sp:s and tags before name:s so cb_buffer can be used in
 * writing the name-value pairs in fixed length blocks in any memory
 * fd points to. For example &  <name>=<value>& and &<name>=<value>&
 * are similar.
 *
 * Names are kept internally in character array where four bytes 
 * represent one character (UCS, 31-bits). 
 */

/* 
 * One byte represents a character */
int  cb_set_cursor(CBFILE **cbs, unsigned char **name, int *namelength); 
/*
 * Four bytes represents one character */
int  cb_set_cursor_ucs(CBFILE **cbs, unsigned char **ucsname, int *namelength); 

/*
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

// Characters according to bytecount and encoding.
int  cb_get_chr(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes);
//int  cb_put_chr(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes);
int  cb_put_chr(CBFILE **cbs, unsigned long int chr, int *bytecount, int *storedbytes); // 12.8.2013
// From unicode to and from utf-8
int  cb_get_ucs_ch(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes );
int  cb_put_ucs_ch(CBFILE **cbs, unsigned long int *chr, int *bytecount, int *storedbytes );
// Transfer encoding
int  cb_get_utf8_ch(CBFILE **cbs, unsigned long int *chr, unsigned long int *chr_high, int *bytecount, int *storedbytes );
int  cb_put_utf8_ch(CBFILE **cbs, unsigned long int *chr, unsigned long int *chr_high, int *bytecount, int *storedbytes );
// Data
int  cb_get_ch(CBFILE **cbs, unsigned char *ch);
int  cb_put_ch(CBFILE **cbs, unsigned char ch); // *ch -> ch 12.8.2013
int  cb_write_cbuf(CBFILE **cbs, cbuf *cbf); // multibyte
int  cb_write(CBFILE **cbs, unsigned char *buf, int size); // byte by byte
int  cb_flush(CBFILE **cbs);
int  cb_write_to_block(CBFILE **cbs, unsigned char *buf, int size); // special: write to block to read again (others use filedescriptors to read or write if useasbuffer was not set)

int  cb_allocate_cbfile(CBFILE **buf, int fd, int bufsize, int blocksize);
int  cb_allocate_cbfile_from_blk(CBFILE **buf, int fd, int bufsize, unsigned char **blk, int blklen);
int  cb_allocate_buffer(cbuf **cbf, int bufsize);
int  cb_allocate_name(cb_name **cbn);
int  cb_reinit_buffer(cbuf **buf); // empty names
int  cb_reinit_cbfile(CBFILE **buf);
int  cb_free_cbfile(CBFILE **buf);
int  cb_free_buffer(cbuf **buf);
int  cb_free_fname(cb_name **name);

int  cb_use_as_buffer(CBFILE **buf); // file descriptor is not used
int  cb_use_as_file(CBFILE **buf);   // namelist is bound by filesize
int  cb_use_as_stream(CBFILE **buf); // namelist is bound by buffer size
int  cb_set_search_method(CBFILE **buf, char method); // defined in cb_buffer.h, names CBSEARCH*
int  cb_get_buffer(cbuf *cbs, unsigned char **buf, int *size); // Allocate new text and copy it's content from 'cbs'
int  cb_get_buffer_range(cbuf *cbs, unsigned char **buf, int *size, int *from, int *to); // Allocate and copy range, new

int  cb_copy_name(cb_name **from, cb_name **to);
int  cb_compare(unsigned char **name1, int len1, unsigned char **name2, int len2);
//int  cb_compare_chr(CBFILE **cbs, int index, unsigned long int chr); // not tested

int  cb_set_rstart(CBFILE **str, unsigned long int rstart); // character between valuename and value, '='
int  cb_set_rend(CBFILE **str, unsigned long int rend); // character between value and next valuename, '&'
int  cb_set_cstart(CBFILE **str, unsigned long int cstart); // comment start character inside valuename, '#'
int  cb_set_cend(CBFILE **str, unsigned long int cend); // comment end character, '\n'
int  cb_set_bypass(CBFILE **str, unsigned long int bypass); // character to bypass next special character, '\\' (late, 14.12.2009)
int  cb_set_encodingbytes(CBFILE **str, int bytecount); // 0 any, 1 one byte
int  cb_set_encoding(CBFILE **str, int number); // 0 utf, 1 one byte

// 4-byte character array
int cb_get_ucs_chr(unsigned long int *chr, unsigned char **chrbuf, int *bufindx, int bufsize);
int cb_put_ucs_chr(unsigned long int chr, unsigned char **chrbuf, int *bufindx, int bufsize);
int cb_print_ucs_chrbuf(unsigned char **chrbuf, int namelen, int buflen);

// Debug
int  cb_print_names(CBFILE **str);
void cb_print_counters(CBFILE **str);

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
