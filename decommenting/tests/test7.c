#include /*Header File 1*/<stdio.h>
#include /*Header File 2*/<stdlib.h> 

/* Program Author: Muhammad Asim Jamshed */
/* This covers box 11. This should not compile. */
/*
 * / * Prints Hello World * /
 */

int main( int argc, char** argv ) {

	printf( "/***\"Hello World!\"***/\n );

	printf( '/***\"Hello World!\"*
		**/\n );

	return EXIT_SUCCESS;
}
