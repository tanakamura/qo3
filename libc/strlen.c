#include <string.h>

size_t
strlen(const char *s)
{
	int size = 0;
	while (*s++) size++;

	return size;
}
