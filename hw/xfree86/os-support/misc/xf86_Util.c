/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/misc/xf86_Util.c,v 3.4 1996/12/23 06:50:25 dawes Exp $ */
/*
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Wexelblat not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  David Wexelblat makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID WEXELBLAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $Xorg: xf86_Util.c,v 1.3 2000/08/17 19:51:26 cpqbld Exp $ */

/*
 * This file is for utility functions that will be shared by other pieces
 * of the system.  Putting things here ensure that all the linking order
 * dependencies are dealt with, as this library will be linked in last.
 */

#include <ctype.h>

/*
 * A portable hack at implementing strcasecmp()
 * The characters '_', ' ', and '\t' are ignored in the comparison
 */
int StrCaseCmp(s1, s2)
const char *s1, *s2;
{
	char c1, c2;

	if (*s1 == 0)
		if (*s2 == 0)
			return(0);
		else
			return(1);

	while (*s1 == '_' || *s1 == ' ' || *s1 == '\t')
		s1++;
	while (*s2 == '_' || *s2 == ' ' || *s2 == '\t')
		s2++;
	c1 = (isupper(*s1) ? tolower(*s1) : *s1);
	c2 = (isupper(*s2) ? tolower(*s2) : *s2);
	while (c1 == c2)
	{
		if (c1 == '\0')
			return(0);
		s1++; s2++;
		while (*s1 == '_' || *s1 == ' ' || *s1 == '\t')
			s1++;
		while (*s2 == '_' || *s2 == ' ' || *s2 == '\t')
			s2++;
		c1 = (isupper(*s1) ? tolower(*s1) : *s1);
		c2 = (isupper(*s2) ? tolower(*s2) : *s2);
	}
	return(c1 - c2);
}


/* For use only with gcc */
#ifdef __GNUC__

#include "os.h"

char *debug_alloca(file, line, size)
char *file;
int line;
int size;
{
	char *ptr;

	ptr = (char *)Xalloc(size);
	ErrorF("Alloc: %s line %d; ptr = 0x%x, length = %d\n", file, line,
	       ptr, size);
	return ptr;
}

void debug_dealloca(file, line, ptr)
char *file;
int line;
char *ptr;
{
	ErrorF("Dealloc: %s line %d; ptr = 0x%x\n", file, line, ptr);
	Xfree(ptr);
}
#endif

#if defined(ISC) || defined(Lynx)

#include <math.h>

/* Needed for apm_driver.c */
/* These functions are modeled after the functions inside gnu's libc */

static double copysign(double x, double y)
{
	x = fabs(x);
	return y < 0 ? - x : x;
}

double RInt(double x)
{
	double s,t;
	const double one = 1.0;
	const static double L = 4503599627370496.0E0;

	if (x!=x)
		return(x);
	if (copysign(x,one) >= L)
		return(x);
	s = copysign(L,x);
	t = x + s;
	return (t - s);
}
#endif
