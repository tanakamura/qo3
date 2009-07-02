#include <string.h>

void *
memset(void *p, int c, size_t len)
{
        char *s = p;
	size_t i;
	for (i=0; i<len; i++) {
		s[i] = c;
	}
        return p;
}
void *
memcpy(void *p, const void *src, size_t len)
{
        char *d = p;
	const char *s = src;
	size_t i;
	for (i=0; i<len; i++) {
		d[i] = s[i];
	}

        return p;
}

char *
strcpy(char *dest, const char *src)
{
        char *ret = dest;
	while (*src)
		*dest++ = *src++;
        return ret;
}

int
strcmp(const char *s1, const char *s2)
{
        while (1) {
                if (*s1 != *s2)
                        return *s1 - *s2;

                if (*s1 == '\0')
                        return 0;

                s1++;
                s2++;
        }
}
int
memcmp(const void *s1, const void *s2, size_t n)
{
        const char *p1 = s1;
        const char *p2 = s2;

        size_t i;
        for (i=0; i<n; i++) {
                if (p1[i] != p2[i]) {
                        return p1[i] - p2[i];
                }
        }
        return 0;
}
