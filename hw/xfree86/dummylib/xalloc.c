/* $XFree86: xc/programs/Xserver/hw/xfree86/dummylib/xalloc.c,v 1.2 2002/11/09 09:00:13 herrb Exp $ */

#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

/*
 * Utility functions required by libxf86_os. 
 */

pointer
Xalloc(unsigned long n)
{
    if (!n)
	n = 1;
    return malloc(n);
}

pointer
Xrealloc(pointer p, unsigned long n)
{
    if (!n)
	n = 1;
    return realloc(p, n);
}

pointer
Xcalloc(unsigned long n)
{
    pointer r;

    r = Xalloc(n);
    memset(r, 0, n);
    return r;
}

pointer
XNFalloc(unsigned long n)
{
    pointer r;

    r = Xalloc(n);
    if (!r)
	FatalError("XNFalloc failed\n");
    return r;
   
}

pointer
XNFrealloc(pointer p, unsigned long n)
{
    pointer r;

    r = Xrealloc(p, n);
    if (!r)
	FatalError("XNFrealloc failed\n");
    return r;
   
}

pointer
XNFcalloc(unsigned long n)
{
    pointer r;

    r = Xcalloc(n);
    if (!r)
	FatalError("XNFcalloc failed\n");
    return r;
   
}

void
Xfree(pointer p)
{
    free(p);
}

char *
Xstrdup(const char *s)
{
    char *sd;

    if (s == NULL)
	return NULL;

    sd = (char *)Xalloc(strlen(s) + 1);
    if (sd != NULL)
	strcpy(sd, s);
    return sd;
}
