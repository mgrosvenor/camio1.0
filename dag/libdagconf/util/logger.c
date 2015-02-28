#include <stdio.h>

#include <string.h>

#if defined(_WIN32)
#include "wintypedefs.h"

#else

#include <inttypes.h>
#endif


#define BUFFER_SIZE 1024


const char*
logger_make_bitstring(uint32_t val)
{
    /* make a string representation of the bits in val */
    static char bit_string[BUFFER_SIZE];
    int i;

    memset(bit_string, 0, BUFFER_SIZE);
    for (i = 31; i >= 0; i--)
    {
        sprintf(bit_string, "%s%c", bit_string, val & (1 << i) ? '1':'0'); 
    }
    return bit_string;
}

