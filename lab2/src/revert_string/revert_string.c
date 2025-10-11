#include "revert_string.h"

void RevertString(char *str)
{
	char *end = str;
    while (*end) end++;
    end--;

    while (str < end) {
        char tmp = *str;
        *str++ = *end;
        *end-- = tmp;
    }
}

