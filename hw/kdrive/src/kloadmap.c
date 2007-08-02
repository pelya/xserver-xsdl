/*
 * Copyright © 1999 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "kdrive.h"

#ifdef WINDOWS
#define KM_BUF	1024
#define KM_EOF	-1

typedef struct _km_file {
    HANDLE  handle;
    char    buf[KM_BUF];
    char    *bufptr;
    DWORD   remain;
} km_file;

int
km_fill (km_file *kf)
{
    BOOL    r;
    
    NCD_DEBUG ((DEBUG_INIT, "km_fill"));
    r = ReadFile (kf->handle, kf->buf, KM_BUF,
		  &kf->remain, NULL);
    NCD_DEBUG ((DEBUG_INIT, "Got %d", kf->remain));
    if (!r || !kf->remain)
	return KM_EOF;
    kf->bufptr = kf->buf;
    --kf->remain;
    return *kf->bufptr++;
}

#define km_getchar(kf)	((kf)->remain-- ? *kf->bufptr++ : km_fill (kf))
#else
#define km_getchar(kf)	getc(kf)
#endif

BOOL
km_word (km_file *kf, char *buf, int len)
{
    int	    c;

    for (;;)
    {
	switch (c = km_getchar (kf)) {
	case KM_EOF:
	    return FALSE;
	case ' ':
	case '\t':
	case '\n':
	case '\r':
	    continue;
	}
	break;
    }
    len--;
    while (len--) 
    {
	*buf++ = c;
	switch (c = km_getchar (kf)) {
	case KM_EOF:
	case ' ':
	case '\t':
	case '\n':
	case '\r':
	    *buf++ = '\0';
	    return TRUE;
	}
    }
    return FALSE;
}

BOOL
km_int (km_file *kf, int *r)
{
    char    word[64];

    if (km_word (kf, word, sizeof (word)))
    {
	*r = strtol (word, NULL, 0);
	return TRUE;
    }
    return FALSE;
}

WCHAR *winKbdExtensions[] = {
    L".xku",
    L".xkb"
};

#define NUM_KBD_EXTENSIONS  (sizeof (winKbdExtensions) / sizeof (winKbdExtensions[0]))

BOOL
winLoadKeymap (void)
{
    WCHAR   file[32 + KL_NAMELENGTH];
    WCHAR   name[KL_NAMELENGTH];
    HKL	    layout;
    km_file kf;
    int	    width;
    BOOL    ret;
    KeySym  *m;
    int	    scancode;
    int	    w;
    int	    e;

    layout = GetKeyboardLayout (0);
    /*
     * Pre-build 46 versions of ThinSTAR software return 0
     * for all layouts
     */
    if (!layout)
	return FALSE;
    NCD_DEBUG ((DEBUG_INIT, "Keyboard layout 0x%x", layout));
    for (e = 0; e < NUM_KBD_EXTENSIONS; e++)
    {
	wstrcpy (file, L"\\Storage Card\\");
	wsprintf (name, TEXT("%08x"), layout);
	wstrcat (file, name);
	wstrcat (file, winKbdExtensions[e]);
	NCD_DEBUG ((DEBUG_INIT, "Loading keymap from %S", file));
	kf.handle = CreateFile (file, 
				GENERIC_READ,
				FILE_SHARE_READ|FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL);
	if (kf.handle != INVALID_HANDLE_VALUE)
	    break;
    }
    if (kf.handle == INVALID_HANDLE_VALUE)
    {
	NCD_DEBUG ((DEBUG_INIT, "No such file"));
	return FALSE;
    }
    ret = FALSE;
    kf.remain = 0;
    /*
     * Keymap format:
     *
     *	flags (optional)
     *	width
     *	keycode -> keysym array	 (num_keycodes * width)
     */
    if (!km_int (&kf, &width))
	goto bail1;
    if (width & KEYMAP_FLAGS)
    {
	CEKeymapFlags = (unsigned long) width;
	if (!km_int (&kf, &width))
	    goto bail1;
    }
    else
	CEKeymapFlags = 0;
    if (width > MAX_WIDTH)
	goto bail1;
    NCD_DEBUG ((DEBUG_INIT, "Keymap width %d flags 0x%x", 
		width, CEKeymapFlags));
    m = CEKeymap;
    for (scancode = MIN_SCANCODE; scancode <= MAX_SCANCODE; scancode++)
    {
	for (w = 0; w < width; w++)
	{
	    if (!km_int (&kf, m))
		break;
	    m++;
	}
	if (w != width)
	    break;
    }
    CEKeySyms.mapWidth = width;
    ret = TRUE;
bail1:
    CloseHandle (kf.handle);
    return ret;
}
