#include <wchar.h>
#include <stdlib.h>

/* Copied from Bionic libc */
static int
replacement_wctomb(char *s, wchar_t wchar)
{
	static mbstate_t mbs;
	size_t rval;

	if (s == NULL) {
		/* No support for state dependent encodings. */
		memset(&mbs, 0, sizeof(mbs));
		return (0);
	}
	if ((rval = wcrtomb(s, wchar, &mbs)) == (size_t)-1)
		return (-1);
	return ((int)rval);
}

#define wctomb replacement_wctomb
