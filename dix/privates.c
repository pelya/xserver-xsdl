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
#include "colormapst.h"
#include "inputstr.h"

typedef struct _PrivateDesc {
    DevPrivateKey key;
    unsigned size;
    CallbackListPtr initfuncs;
    CallbackListPtr deletefuncs;
    struct _PrivateDesc *next;
} PrivateDescRec;

/* list of all allocated privates */
static PrivateDescRec *items = NULL;

static _X_INLINE PrivateDescRec *
findItem(const DevPrivateKey key)
{
    PrivateDescRec *item = items;
    while (item) {
	if (item->key == key)
	    return item;
	item = item->next;
    }
    return NULL;
}

/*
 * Request pre-allocated space.
 */
_X_EXPORT int
dixRequestPrivate(const DevPrivateKey key, unsigned size)
{
    PrivateDescRec *item = findItem(key);
    if (item) {
	if (size > item->size)
	    item->size = size;
    } else {
	item = (PrivateDescRec *)xalloc(sizeof(PrivateDescRec));
	if (!item)
	    return FALSE;
	memset(item, 0, sizeof(PrivateDescRec));

	/* add privates descriptor */
	item->key = key;
	item->size = size;
	item->next = items;
	items = item;
    }
    return TRUE;
}

/*
 * Allocate a private and attach it to an existing object.
 */
_X_EXPORT pointer *
dixAllocatePrivate(PrivateRec **privates, const DevPrivateKey key)
{
    PrivateDescRec *item = findItem(key);
    PrivateRec *ptr;
    unsigned size = sizeof(PrivateRec);
    
    if (item)
	size += item->size;

    ptr = (PrivateRec *)xcalloc(size, 1);
    if (!ptr)
	return NULL;
    ptr->key = key;
    ptr->value = (size > sizeof(PrivateRec)) ? (ptr + 1) : NULL;
    ptr->next = *privates;
    *privates = ptr;

    /* call any init funcs and return */
    if (item) {
	PrivateCallbackRec calldata = { key, &ptr->value };
	CallCallbacks(&item->initfuncs, &calldata);
    }
    return &ptr->value;
}

/*
 * Called to free privates at object deletion time.
 */
_X_EXPORT void
dixFreePrivates(PrivateRec *privates)
{
    PrivateRec *ptr, *next;
    PrivateDescRec *item;
    PrivateCallbackRec calldata;

    /* first pass calls the delete callbacks */
    for (ptr = privates; ptr; ptr = ptr->next) {
	item = findItem(ptr->key);
	if (item) {
	    calldata.key = ptr->key;
	    calldata.value = &ptr->value;
	    CallCallbacks(&item->deletefuncs, &calldata);
	}
    }
	
    /* second pass frees the memory */
    ptr = privates;
    while (ptr) {
	next = ptr->next;
	xfree(ptr);
	ptr = next;
    }
}

/*
 * Callback registration
 */
_X_EXPORT int
dixRegisterPrivateInitFunc(const DevPrivateKey key,
			   CallbackProcPtr callback, pointer data)
{
    PrivateDescRec *item = findItem(key);
    if (!item) {
	if (!dixRequestPrivate(key, 0))
	    return FALSE;
	item = findItem(key);
    }
    return AddCallback(&item->initfuncs, callback, data);
}

_X_EXPORT int
dixRegisterPrivateDeleteFunc(const DevPrivateKey key,
			     CallbackProcPtr callback, pointer data)
{
    PrivateDescRec *item = findItem(key);
    if (!item) {
	if (!dixRequestPrivate(key, 0))
	    return FALSE;
	item = findItem(key);
    }
    return AddCallback(&item->deletefuncs, callback, data);
}

/* Table of devPrivates offsets */
static unsigned *offsets = NULL;
static unsigned offsetsSize = 0;

/*
 * Specify where the devPrivates field is located in a structure type
 */
_X_EXPORT int
dixRegisterPrivateOffset(RESTYPE type, unsigned offset)
{
    type = type & TypeMask;

    /* resize offsets table if necessary */
    while (type >= offsetsSize) {
	unsigned i = offsetsSize * 2 * sizeof(int);
	offsets = (unsigned *)xrealloc(offsets, i);
	if (!offsets) {
	    offsetsSize = 0;
	    return FALSE;
	}
	for (i=offsetsSize; i < 2*offsetsSize; i++)
	    offsets[i] = -1;
	offsetsSize *= 2;
    }

    offsets[type] = offset;
    return TRUE;
}

_X_EXPORT int
dixLookupPrivateOffset(RESTYPE type)
{
    type = type & TypeMask;
    assert(type < offsetsSize);
    return offsets[type];
}

int
dixResetPrivates(void)
{
    PrivateDescRec *next;
    unsigned i;

    /* reset internal structures */
    while (items) {
	next = items->next;
	xfree(items);
	items = next;
    }
    if (offsets)
	xfree(offsets);
    offsetsSize = 16;
    offsets = (unsigned *)xalloc(offsetsSize * sizeof(unsigned));
    if (!offsets)
	return FALSE;
    for (i=0; i < offsetsSize; i++)
	offsets[i] = -1;

    /* register basic resource offsets */
    return dixRegisterPrivateOffset(RT_WINDOW,
				    offsetof(WindowRec, devPrivates)) &&
	dixRegisterPrivateOffset(RT_PIXMAP,
				 offsetof(PixmapRec, devPrivates)) &&
	dixRegisterPrivateOffset(RT_GC,
				 offsetof(GC, devPrivates)) &&
	dixRegisterPrivateOffset(RT_COLORMAP,
				 offsetof(ColormapRec, devPrivates));
}
