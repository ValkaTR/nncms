#include <stdbool.h>

main()
{

    bool a = true;
    bool b = a + true * false + a + true + !false;

    printf( "b = %s\n", b == false ? "false" : "!false" );
    printf( "b = %s\n", b == true ? "true" : "!true" );

}