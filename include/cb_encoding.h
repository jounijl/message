#define byteisascii(x)            ( (x|0x7F)==0x7F ) // (x|01111111==01111111)
#define byteisutf8tail(x)         ( (x|0x3F)==0xBF ) // (x|00111111==10111111)
#define byteisutf8head2(x)        ( (x|0x1F)==0xDF ) // (x|00011111==11011111)
#define byteisutf8head3(x)        ( (x|0x0F)==0xEF ) // (x|00001111==11101111)
#define byteisutf8head4(x)        ( (x|0x07)==0xF7 ) // (x|00000111==11110111)
#define byteisutf8head5(x)        ( (x|0x03)==0xFB ) // (x|00000011==11111011)
#define byteisutf8head6(x)        ( (x|0x01)==0xFD ) // (x|00000001==11111101)
#define intisutf8bom(x)           ( x==0xEFBBBF )       // 0000 FEFF
#define intisbombigebom(x)        ( x==0xFEFF )       // 0000 FEFF
#define intisbomlittleebom(x)     ( x==0xFFFE )       // 0000 FEFF
#define intisutf16bigebom(x)      ( x==65279 )       // 0000 FEFF
#define intisutf16littleebom(x)   ( x==65534 )       // 0000 FFFE
#define bytesareutf8bom(h,l)      ( h==0xFE && l==0xFF ) // 0000 FEFF
#define bytesareutf16bigebom(h,l)   bytesareutf8bom(h,l) //  0000 FEFF
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
