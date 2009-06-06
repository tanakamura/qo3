#include <stdlib.h>
#include <ctype.h>

int
atoi(const char *nptr)
{
        int val = 0;

        while (1) {
                int c;
                if (!isdigit(*nptr))
                        break;

                c = *nptr - '0';
                val = val*10 + c;
                nptr++;
        }

        return val;
}
