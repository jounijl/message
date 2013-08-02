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

#define byteisascii(x)            ( (x|0x7F)==0x7F ) // (x|01111111==01111111)
#define byteisutf8tail(x)         ( (x|0x3F)==0xBF ) // (x|00111111==10111111)
#define byteisutf8head2(x)        ( (x|0x1F)==0xDF ) // (x|00011111==11011111)
#define byteisutf8head3(x)        ( (x|0x0F)==0xEF ) // (x|00001111==11101111)
#define byteisutf8head4(x)        ( (x|0x07)==0xF7 ) // (x|00000111==11110111)
#define byteisutf8head5(x)        ( (x|0x03)==0xFB ) // (x|00000011==11111011)
#define byteisutf8head6(x)        ( (x|0x01)==0xFD ) // (x|00000001==11111101)
#define intisutf8bom(x)           ( x==0xEFBBBF )            // 0000 FEFF
#define intisbombigebom(x)        ( x==0xFEFF )              // 0000 FEFF
#define intisbomlittleebom(x)     ( x==0xFFFE )              // 0000 FEFF
#define intisutf16bigebom(x)      ( x==65279 )               // 0000 FEFF
#define intisutf16littleebom(x)   ( x==65534 )               // 0000 FFFE
#define bytesareutf8bom(h,l)      ( h==0xFE && l==0xFF )     // 0000 FEFF
#define bytesareutf16bigebom(h,l)   bytesareutf8bom(h,l)     // 0000 FEFF
#define bytesareutf16littleebom(h,l)  ( l==0xFE && h==0xFF ) // 0000 FFFE

#define masktoutf8tail(x)           x = ( ( x & 0x3F ) | 0x80 ) 
#define masktoutf8head2(x)          x = ( ( x & 0x1F ) | 0xC0 ) 
#define masktoutf8head3(x)          x = ( ( x & 0x0F ) | 0xE0 ) 
#define masktoutf8head4(x)          x = ( ( x & 0x07 ) | 0xF0 ) 
#define masktoutf8head5(x)          x = ( ( x & 0x03 ) | 0xF8 ) 
#define masktoutf8head6(x)          x = ( ( x & 0x01 ) | 0xFC ) 

/*    0xxxxxxx
 *    110xxxxx 10xxxxxx
 *    1110xxxx 10xxxxxx 10xxxxxx
 *    11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 *    111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 *    1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 */

#define CBENCAUTO 		0
#define CBENC1BYTE 		1
#define CBENC2BYTE 		2
#define CBENCUTF8 		3
#define CBENC4BYTE 		4
#define CBENCUTF16LE 		5
#define CBENCPOSSIBLEUTF16LE 	6
#define CBENCUTF16BE 		7
#define CBENCUTF32LE		8
#define CBENCUTF32BE		9

#define CBDEFAULTENCODING           0
#define CBDEFAULTENCODINGBYTES      1   // Default maximum count of bytes, set as 0 for any count number of bytes, 1 if auto
