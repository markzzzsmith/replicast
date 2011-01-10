#include "string.h"

#include "stringnz.h"


char *strnzcpy(char *dest, const char *src, const size_t n)
{


	if (n > 0) {
		strncpy(dest, src, n);
		dest[n - 1] = '\0';
	}	

	return dest;

}


char *strnchr(const char *str, const int c, const size_t n)
{
	size_t pos = 0;
	const size_t max_pos = n - 1;


	if (str == NULL || n == 0) {
		return NULL;
	}	

	while (str[pos] != c && str[pos] != '\0' && pos < max_pos) {
		pos++;
	}		

	if (str[pos] == c) {
		return (char *)&(str[pos]);
	} else {
		return NULL;
	}

}


char *strnrchr(const char *str, const int c, const size_t n)
{
	char *s = strnchr(str, '\0', n);


	while (*s != c && s != str) {
		s--;
	}

	if (*s == c) {
		return s;
	} else {
		return NULL;
	}

}
