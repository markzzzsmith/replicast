#include "string.h"

#include "stringz.h"


char *strnzcpy(char *dest, const char *src, const size_t n)
{


	if (n > 0) {
		strncpy(dest, src, n);
		dest[n - 1] = '\0';
	}	

	return dest;

}

