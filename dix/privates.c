/*

Copyright 1993, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stddef.h>
#include "windowstr.h"
#include "resource.h"
#include "privates.h"
#include "gcstruct.h"
#include "cursorstr.h"
#include "colormapst.h"
#include "inputstr.h"

struct _Private {
    int state;
    pointer value;
};

typedef struct _PrivateDesc {
    DevPrivateKey key;
    unsigned size;
} PrivateDescRec;

#define PRIV_MAX 256
#define PRIV_STEP 16

static int number_privates_allocated;
static int number_private_ptrs_allocated;
static int bytes_private_data_allocated;

/* list of all allocated privates */
static PrivateDescRec items[PRIV_MAX];
static int nextPriv;

static PrivateDescRec *
findItem(const DevPrivateKey key)
{
    if (!key->key) {
	if (nextPriv >= PRIV_MAX)
	    return NULL;

	items[nextPriv].key = key;
	key->key = nextPriv;
	nextPriv++;
    }

    return items + key->key;
}

static _X_INLINE int
privateExists(PrivateRec **privates, const DevPrivateKey key)
{
    return key->key && *privates &&
	(*privates)[0].state > key->key &&
	(*privates)[key->key].state;
}

/*
 * Request pre-allocated space.
 */
int
dixRegisterPrivateKey(const DevPrivateKey key, DevPrivateType type, unsigned size)
{
    PrivateDescRec *item = findItem(key);
    if (!item)
	return FALSE;
    if (size > item->size)
	item->size = size;
    return TRUE;
}

/*
 * Allocate a private and attach it to an existing object.
 */
static pointer *
dixAllocatePrivate(PrivateRec **privates, const DevPrivateKey key)
{
    PrivateDescRec *item = findItem(key);
    PrivateRec *ptr;
    pointer value;
    int oldsize, newsize;

    newsize = (key->key / PRIV_STEP + 1) * PRIV_STEP;

    /* resize or init privates array */
    if (!item)
	return NULL;

    /* initialize privates array if necessary */
    if (!*privates) {
	++number_privates_allocated;
	number_private_ptrs_allocated += newsize;
	ptr = calloc(newsize, sizeof(*ptr));
	if (!ptr)
	    return NULL;
	*privates = ptr;
	(*privates)[0].state = newsize;
    }

    oldsize = (*privates)[0].state;

    /* resize privates array if necessary */
    if (key->key >= oldsize) {
	ptr = realloc(*privates, newsize * sizeof(*ptr));
	if (!ptr)
	    return NULL;
	memset(ptr + oldsize, 0, (newsize - oldsize) * sizeof(*ptr));
	*privates = ptr;
	(*privates)[0].state = newsize;
	number_private_ptrs_allocated -= oldsize;
	number_private_ptrs_allocated += newsize;
    }

    /* initialize slot */
    ptr = *privates + key->key;
    ptr->state = 1;
    if (item->size) {
	value = calloc(item->size, 1);
	if (!value)
	    return NULL;
	bytes_private_data_allocated += item->size;
	ptr->value = value;
    }

    return &ptr->value;
}

/*
 * Look up a private pointer.
 */
pointer
dixLookupPrivate(PrivateRec **privates, const DevPrivateKey key)
{
    pointer *ptr;

    assert (key->key != 0);
    if (privateExists(privates, key))
	return (*privates)[key->key].value;

    ptr = dixAllocatePrivate(privates, key);
    return ptr ? *ptr : NULL;
}

/*
 * Look up the address of a private pointer.
 */
pointer *
dixLookupPrivateAddr(PrivateRec **privates, const DevPrivateKey key)
{
    assert (key->key != 0);

    if (privateExists(privates, key))
	return &(*privates)[key->key].value;

    return dixAllocatePrivate(privates, key);
}

/*
 * Set a private pointer.
 */
int
dixSetPrivate(PrivateRec **privates, const DevPrivateKey key, pointer val)
{
    assert (key->key != 0);
 top:
    if (privateExists(privates, key)) {
	(*privates)[key->key].value = val;
	return TRUE;
    }

    if (!dixAllocatePrivate(privates, key))
	return FALSE;
    goto top;
}

/*
 * Called to free privates at object deletion time.
 */
void
dixFreePrivates(PrivateRec *privates, DevPrivateType type)
{
    int i;

    if (privates) {
	number_private_ptrs_allocated -= privates->state;
	number_privates_allocated--;
	for (i = 1; i < privates->state; i++)
	    if (privates[i].state) {
		/* free pre-allocated memory */
		if (items[i].size)
		    free(privates[i].value);
		bytes_private_data_allocated -= items[i].size;
	    }
    }

    free(privates);
}

/* Table of devPrivates offsets */
static const int offsets[] = {
    -1,					/* RT_NONE */
    offsetof(WindowRec, devPrivates),	/* RT_WINDOW */
    offsetof(PixmapRec, devPrivates),	/* RT_PIXMAP */
    offsetof(GC, devPrivates),		/* RT_GC */
    -1,		    			/* RT_FONT */
    offsetof(CursorRec, devPrivates),	/* RT_CURSOR */
    offsetof(ColormapRec, devPrivates),	/* RT_COLORMAP */
};

#define NUM_OFFSETS	(sizeof (offsets) / sizeof (offsets[0]))

int
dixLookupPrivateOffset(RESTYPE type)
{
    /*
     * Special kludge for DBE which registers a new resource type that
     * points at pixmaps (thanks, DBE)
     */
    if (type & RC_DRAWABLE) {
	if (type == RT_WINDOW)
	    return offsets[RT_WINDOW & TypeMask];
	else
	    return offsets[RT_PIXMAP & TypeMask];
    }
    type = type & TypeMask;
    if (type < NUM_OFFSETS)
	return offsets[type];
    return -1;
}

void
dixPrivateUsage(void)
{
    ErrorF("number of private structures: %d\n",
	   number_privates_allocated);
    ErrorF("total number of private pointers: %d (%zd bytes)\n",
	   number_private_ptrs_allocated,
	   number_private_ptrs_allocated * sizeof (struct _Private));
    ErrorF("bytes of extra private data: %d\n",
	   bytes_private_data_allocated);
    ErrorF("Total privates memory usage: %zd\n",
	   bytes_private_data_allocated +
	   number_private_ptrs_allocated * sizeof (struct _Private));
}

void
dixResetPrivates(void)
{
    int i;

    /* reset private descriptors */
    for (i = 1; i < nextPriv; i++) {
	items[i].key->key = 0;
	items[i].size = 0;
    }
    nextPriv = 1;
    if (number_privates_allocated)
	dixPrivateUsage();
}
