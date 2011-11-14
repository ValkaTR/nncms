// #############################################################################

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sha2.h"

// #############################################################################

int
main( int argc, char *argv[] )
{
    unsigned char digest[SHA512_DIGEST_SIZE]; int i;
    unsigned char output[2 * SHA512_DIGEST_SIZE + 1];

    if( argc != 2 )
    {
        fprintf( stderr, "Usage: %s <string>\n", argv[0] );
        exit( 1 );
    }

    sha512( argv[1], strlen( (char *) argv[1] ), digest );

    output[2 * SHA512_DIGEST_SIZE] = '\0';
    for( i = 0; i < (int) SHA512_DIGEST_SIZE ; i++ )
    {
       sprintf( (char *) output + 2 * i, "%02x", digest[i] );
    }

    printf( "String: %s\n", argv[1] );
    printf( "Hash: %s\n", output );
}

// #############################################################################
