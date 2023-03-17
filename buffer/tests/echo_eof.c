#include <unistd.h> // write

//int  main(int argc, char *argv[]);
//int  main(int argc, char *argv[]){
int  main(void);
int  main(void){
	const unsigned char chr = 0x00;
	write( STDOUT_FILENO, &chr, 1 );
	return 0;
}
