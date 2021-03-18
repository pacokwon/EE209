#include /*Header File 1*/<stdio.h>
#include /*Header File 2*/<stdlib.h> 

/* Program Author: Muhammad Asim Jamshed */
/* This covers box 9 and box 10 */
/*
 * / * Prints Hello World * /
 */

int main( int argc, char** argv ) {

	printf( "/***\"Hello World!\"*
		**/\n" );
	printf( "/***\"Hello 
		World!
		\"***/\n"  /*This should be 
		fine*/
		);



	printf( '/***\"Hello World!\"*
		**/\n' );
	printf( '/***\"Hello 
		World!
		\"***/\n'  /*This should be 
		fine*/
		);

	return EXIT_SUCCESS;
}
