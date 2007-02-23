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

#include <X11/X.h>
#include "scrnintstr.h"
#include "misc.h"
#include "os.h"
#include "windowstr.h"
#include "resource.h"
#include "privates.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "colormapst.h"
#include "servermd.h"
#include "site.h"
#include "inputstr.h"
#include "extnsionst.h"

typedef struct _PrivateDescItem {
    int index;
    RESTYPE type;
    pointer parent;
    unsigned size;
    CallbackListPtr initfuncs;
    CallbackListPtr deletefuncs;
} PrivateDescItemRec, *PrivateDescItemPtr;

/* keeps track of whether resource objects have been created */
static char *instances = NULL;
static RESTYPE instancesSize = 0;
static char anyInstances = 0;

/* list of all allocated privates */
static PrivateDescItemPtr items = NULL;
static unsigned itemsSize = 0;
static unsigned nextPrivateIndex = 0;

/* number of extra slots to add when resizing the tables */
#define PRIV_TAB_INCREMENT 48
/* set in index value for privates registered after resources were created */
#define PRIV_DYN_MASK (1<<30)
/* descriptor item lookup convenience macro */
#define GET_DESCRIPTOR(index) (items + ((index) & (PRIV_DYN_MASK - 1)))
/* type mask convenience macro */
#define TYPE_BITS(type) ((type) & TypeMask)

static _X_INLINE ResourcePtr
findResourceBucket(RESTYPE type, pointer instance) {
    ResourcePtr res = *((ResourcePtr *)instance);

    while (res->type != type)
	res = res->nexttype;
    return res;
}

/*
 * Request functions; the latter calls the former internally.
 */
_X_EXPORT int
dixRequestPrivate(RESTYPE type, unsigned size, pointer parent)
{
    int index = nextPrivateIndex;

    /* check if privates descriptor table needs to be resized */
    if (nextPrivateIndex >= itemsSize) {
	unsigned bytes;
	unsigned size = itemsSize;

	while (nextPrivateIndex >= size)
	    size += PRIV_TAB_INCREMENT;

	bytes = size * sizeof(PrivateDescItemRec);
	items = (PrivateDescItemPtr)xrealloc(items, bytes);
	if (!items) {
	    itemsSize = nextPrivateIndex = 0;
	    return -1;
	}
	memset(items + itemsSize, 0,
	       (size - itemsSize) * sizeof(PrivateDescItemRec));
    }

    /* figure out if resource instances already exist */
    if ((type != RC_ANY && instances[TYPE_BITS(type)]) ||
	(type == RC_ANY && anyInstances))
	index |= PRIV_DYN_MASK;

    /* add privates descriptor */
    items[nextPrivateIndex].index = index;
    items[nextPrivateIndex].type = type;
    items[nextPrivateIndex].parent = parent;
    items[nextPrivateIndex].size = size;
    nextPrivateIndex++;
    return index;
}

_X_EXPORT int
dixRequestSinglePrivate(RESTYPE type, unsigned size, pointer instance)
{
    PrivatePtr ptr;
    ResourcePtr res = findResourceBucket(type, instance);
    int index = dixRequestPrivate(type, size, instance);
    if (index < 0)
	return index;

    ptr = (PrivatePtr)xalloc(sizeof(PrivateRec) + size);
    if (!ptr)
	return -1;
    ptr->index = index;
    ptr->value = ptr + 1;
    ptr->next = res->privates;
    res->privates = ptr;
    return index;
}

/*
 * Lookup function (some of this could be static inlined)
 */
_X_EXPORT pointer
dixLookupPrivate(RESTYPE type, int index, pointer instance)
{
    ResourcePtr res = findResourceBucket(type, instance);
    PrivatePtr ptr = res->privates;
    PrivateDescItemPtr item;
    PrivateCallbackRec calldata;

    /* see if private has already been allocated (likely) */
    while (ptr) {
	if (ptr->index == index)
	    return ptr->value;
	ptr = ptr->next;
    }

    /* past this point, need to create private on the fly */
    /* create the new private */
    item = GET_DESCRIPTOR(index);
    ptr = (PrivatePtr)xalloc(sizeof(PrivateRec) + item->size);
    if (!ptr)
	return NULL;
    memset(ptr, 0, sizeof(PrivateRec) + item->size);
    ptr->index = index;
    ptr->value = ptr + 1;
    ptr->next = res->privates;
    res->privates = ptr;

    /* call any init funcs and return */
    calldata.value = ptr->value;
    calldata.index = index;
    calldata.resource = res;
    CallCallbacks(&item->initfuncs, &calldata);
    return ptr->value;
}

/*
 * Callback registration
 */
_X_EXPORT int
dixRegisterPrivateInitFunc(RESTYPE type, int index,
			   CallbackProcPtr callback, pointer data)
{
    return AddCallback(&GET_DESCRIPTOR(index)->initfuncs, callback, data);
}

_X_EXPORT int
dixRegisterPrivateDeleteFunc(RESTYPE type, int index,
			     CallbackProcPtr callback, pointer data)
{
    return AddCallback(&GET_DESCRIPTOR(index)->deletefuncs, callback, data);
}

/*
 * Internal function called from the main loop to reset the subsystem.
 */
void
dixResetPrivates(void)
{
    if (items)
	xfree(items);
    items = NULL;
    itemsSize = 0;
    nextPrivateIndex = 0;
    
    if (instances)
	xfree(instances);
    instances = NULL;
    instancesSize = 0;
    anyInstances = 0;
}

/*
 * Internal function called from CreateNewResourceType.
 */
int
dixUpdatePrivates(void)
{
    RESTYPE next = lastResourceType + 1;

    /* check if instances table needs to be resized */
    if (next >= instancesSize) {
	RESTYPE size = instancesSize;

	while (next >= size)
	    size += PRIV_TAB_INCREMENT;

	instances = (char *)xrealloc(instances, size);
	if (!instances) {
	    instancesSize = 0;
	    return FALSE;
	}
	memset(instances + instancesSize, 0, size - instancesSize);
	instancesSize = size;
    }
    return TRUE;
}

/*
 * Internal function called from dixAddResource.
 * Allocates a ResourceRec along with any private space all in one chunk.
 */
ResourcePtr
dixAllocateResourceRec(RESTYPE type, pointer instance, pointer parent)
{
    unsigned i, count = 0, size = sizeof(ResourceRec);
    ResourcePtr res;
    PrivatePtr ptr;
    char *value;
    
    /* first pass figures out total size */
    for (i=0; i<nextPrivateIndex; i++)
	if (items[i].type == type &&
	    (items[i].parent == NULL || items[i].parent == parent)) {

	    size += sizeof(PrivateRec) + items[i].size;
	    count++;
	}

    /* allocate resource bucket */
    res = (ResourcePtr)xalloc(size);
    if (!res)
	return res;
    memset(res, 0, size);
    ptr = (PrivatePtr)(res + 1);
    value = (char *)(ptr + count);
    res->privates = (count > 0) ? ptr : NULL;

    /* second pass sets up privates records */
    count = 0;
    for (i=0; i<nextPrivateIndex; i++)
	if (items[i].type == type &&
	    (items[i].parent == NULL || items[i].parent == parent)) {

	    ptr[count].index = items[i].index;
	    ptr[count].value = value;
	    ptr[count].next = ptr + (count + 1);
	    count++;
	    value += items[i].size;
	}

    if (count > 0)
	ptr[count-1].next = NULL;

    /* hook up back-pointer to resource record(s) */
    if (type & RC_PRIVATES) {
	res->nexttype = *((ResourcePtr *)instance);
	*((ResourcePtr *)instance) = res;
    }

    instances[TYPE_BITS(type)] = anyInstances = 1;
    return res;
}
    
/*
 * Internal function called from dixAddResource.
 * Calls the init functions on a newly allocated resource.
 */
void
dixCallPrivateInitFuncs(ResourcePtr res)
{
    PrivatePtr ptr = res->privates;
    PrivateCallbackRec calldata;

    calldata.resource = res;
    while (ptr) {
	calldata.value = ptr->value;
	calldata.index = ptr->index;
	CallCallbacks(&GET_DESCRIPTOR(ptr->index)->initfuncs, &calldata);
	ptr = ptr->next;
    }
}

/*
 * Internal function called from the various delete resource functions.
 * Calls delete callbacks before freeing the ResourceRec and other bits.
 */
void
dixFreeResourceRec(ResourcePtr res)
{
    ResourcePtr *tmp;
    PrivatePtr ptr, next, base;
    PrivateCallbackRec calldata;

    /* first pass calls the delete callbacks */
    ptr = res->privates;
    calldata.resource = res;
    while (ptr) {
	calldata.value = ptr->value;
	calldata.index = ptr->index;
	CallCallbacks(&GET_DESCRIPTOR(ptr->index)->deletefuncs, &calldata);
	ptr = ptr->next;
    }

    /* second pass frees any off-struct private records */
    ptr = res->privates;
    base = (PrivatePtr)(res + 1);
    while (ptr && ptr != base) {
	next = ptr->next;
	xfree(ptr);
	ptr = next;
    }

    /* remove the record from the nexttype linked list and free it*/
    if (res->type & RC_PRIVATES) {
	tmp = (ResourcePtr *)res->value;
	while (*tmp != res)
	    tmp = &(*tmp)->nexttype;
	*tmp = (*tmp)->nexttype;
    }
    xfree(res);
}

/*
 *  Following is the old devPrivates support.  These functions and variables
 *  are deprecated, and should no longer be used.
 */

/*
 *  See the Wrappers and devPrivates section in "Definition of the
 *  Porting Layer for the X v11 Sample Server" (doc/Server/ddx.tbl.ms)
 *  for information on how to use devPrivates.
 */

/*
 *  extension private machinery
 */

static int  extensionPrivateCount;
int extensionPrivateLen;
unsigned *extensionPrivateSizes;
unsigned totalExtensionSize;

void
ResetExtensionPrivates()
{
    extensionPrivateCount = 0;
    extensionPrivateLen = 0;
    xfree(extensionPrivateSizes);
    extensionPrivateSizes = (unsigned *)NULL;
    totalExtensionSize =
	((sizeof(ExtensionEntry) + sizeof(long) - 1) / sizeof(long)) * sizeof(long);
}

_X_EXPORT int
AllocateExtensionPrivateIndex()
{
    return extensionPrivateCount++;
}

_X_EXPORT Bool
AllocateExtensionPrivate(int index2, unsigned amount)
{
    unsigned oldamount;

    /* Round up sizes for proper alignment */
    amount = ((amount + (sizeof(long) - 1)) / sizeof(long)) * sizeof(long);

    if (index2 >= extensionPrivateLen)
    {
	unsigned *nsizes;
	nsizes = (unsigned *)xrealloc(extensionPrivateSizes,
				      (index2 + 1) * sizeof(unsigned));
	if (!nsizes)
	    return FALSE;
	while (extensionPrivateLen <= index2)
	{
	    nsizes[extensionPrivateLen++] = 0;
	    totalExtensionSize += sizeof(DevUnion);
	}
	extensionPrivateSizes = nsizes;
    }
    oldamount = extensionPrivateSizes[index2];
    if (amount > oldamount)
    {
	extensionPrivateSizes[index2] = amount;
	totalExtensionSize += (amount - oldamount);
    }
    return TRUE;
}

/*
 *  client private machinery
 */

static int  clientPrivateCount;
int clientPrivateLen;
unsigned *clientPrivateSizes;
unsigned totalClientSize;

void
ResetClientPrivates()
{
    clientPrivateCount = 0;
    clientPrivateLen = 0;
    xfree(clientPrivateSizes);
    clientPrivateSizes = (unsigned *)NULL;
    totalClientSize =
	((sizeof(ClientRec) + sizeof(long) - 1) / sizeof(long)) * sizeof(long);
}

_X_EXPORT int
AllocateClientPrivateIndex()
{
    return clientPrivateCount++;
}

_X_EXPORT Bool
AllocateClientPrivate(int index2, unsigned amount)
{
    unsigned oldamount;

    /* Round up sizes for proper alignment */
    amount = ((amount + (sizeof(long) - 1)) / sizeof(long)) * sizeof(long);

    if (index2 >= clientPrivateLen)
    {
	unsigned *nsizes;
	nsizes = (unsigned *)xrealloc(clientPrivateSizes,
				      (index2 + 1) * sizeof(unsigned));
	if (!nsizes)
	    return FALSE;
	while (clientPrivateLen <= index2)
	{
	    nsizes[clientPrivateLen++] = 0;
	    totalClientSize += sizeof(DevUnion);
	}
	clientPrivateSizes = nsizes;
    }
    oldamount = clientPrivateSizes[index2];
    if (amount > oldamount)
    {
	clientPrivateSizes[index2] = amount;
	totalClientSize += (amount - oldamount);
    }
    return TRUE;
}

/*
 *  screen private machinery
 */

int  screenPrivateCount;

void
ResetScreenPrivates()
{
    screenPrivateCount = 0;
}

/* this can be called after some screens have been created,
 * so we have to worry about resizing existing devPrivates
 */
_X_EXPORT int
AllocateScreenPrivateIndex()
{
    int		idx;
    int		i;
    ScreenPtr	pScreen;
    DevUnion	*nprivs;

    idx = screenPrivateCount++;
    for (i = 0; i < screenInfo.numScreens; i++)
    {
	pScreen = screenInfo.screens[i];
	nprivs = (DevUnion *)xrealloc(pScreen->devPrivates,
				      screenPrivateCount * sizeof(DevUnion));
	if (!nprivs)
	{
	    screenPrivateCount--;
	    return -1;
	}
	/* Zero the new private */
	bzero(&nprivs[idx], sizeof(DevUnion));
	pScreen->devPrivates = nprivs;
    }
    return idx;
}


/*
 *  window private machinery
 */

static int  windowPrivateCount;

void
ResetWindowPrivates()
{
    windowPrivateCount = 0;
}

_X_EXPORT int
AllocateWindowPrivateIndex()
{
    return windowPrivateCount++;
}

_X_EXPORT Bool
AllocateWindowPrivate(register ScreenPtr pScreen, int index2, unsigned amount)
{
    unsigned oldamount;

    /* Round up sizes for proper alignment */
    amount = ((amount + (sizeof(long) - 1)) / sizeof(long)) * sizeof(long);

    if (index2 >= pScreen->WindowPrivateLen)
    {
	unsigned *nsizes;
	nsizes = (unsigned *)xrealloc(pScreen->WindowPrivateSizes,
				      (index2 + 1) * sizeof(unsigned));
	if (!nsizes)
	    return FALSE;
	while (pScreen->WindowPrivateLen <= index2)
	{
	    nsizes[pScreen->WindowPrivateLen++] = 0;
	    pScreen->totalWindowSize += sizeof(DevUnion);
	}
	pScreen->WindowPrivateSizes = nsizes;
    }
    oldamount = pScreen->WindowPrivateSizes[index2];
    if (amount > oldamount)
    {
	pScreen->WindowPrivateSizes[index2] = amount;
	pScreen->totalWindowSize += (amount - oldamount);
    }
    return TRUE;
}


/*
 *  gc private machinery 
 */

static int  gcPrivateCount;

void
ResetGCPrivates()
{
    gcPrivateCount = 0;
}

_X_EXPORT int
AllocateGCPrivateIndex()
{
    return gcPrivateCount++;
}

_X_EXPORT Bool
AllocateGCPrivate(register ScreenPtr pScreen, int index2, unsigned amount)
{
    unsigned oldamount;

    /* Round up sizes for proper alignment */
    amount = ((amount + (sizeof(long) - 1)) / sizeof(long)) * sizeof(long);

    if (index2 >= pScreen->GCPrivateLen)
    {
	unsigned *nsizes;
	nsizes = (unsigned *)xrealloc(pScreen->GCPrivateSizes,
				      (index2 + 1) * sizeof(unsigned));
	if (!nsizes)
	    return FALSE;
	while (pScreen->GCPrivateLen <= index2)
	{
	    nsizes[pScreen->GCPrivateLen++] = 0;
	    pScreen->totalGCSize += sizeof(DevUnion);
	}
	pScreen->GCPrivateSizes = nsizes;
    }
    oldamount = pScreen->GCPrivateSizes[index2];
    if (amount > oldamount)
    {
	pScreen->GCPrivateSizes[index2] = amount;
	pScreen->totalGCSize += (amount - oldamount);
    }
    return TRUE;
}


/*
 *  pixmap private machinery
 */
#ifdef PIXPRIV
static int  pixmapPrivateCount;

void
ResetPixmapPrivates()
{
    pixmapPrivateCount = 0;
}

_X_EXPORT int
AllocatePixmapPrivateIndex()
{
    return pixmapPrivateCount++;
}

_X_EXPORT Bool
AllocatePixmapPrivate(register ScreenPtr pScreen, int index2, unsigned amount)
{
    unsigned oldamount;

    /* Round up sizes for proper alignment */
    amount = ((amount + (sizeof(long) - 1)) / sizeof(long)) * sizeof(long);

    if (index2 >= pScreen->PixmapPrivateLen)
    {
	unsigned *nsizes;
	nsizes = (unsigned *)xrealloc(pScreen->PixmapPrivateSizes,
				      (index2 + 1) * sizeof(unsigned));
	if (!nsizes)
	    return FALSE;
	while (pScreen->PixmapPrivateLen <= index2)
	{
	    nsizes[pScreen->PixmapPrivateLen++] = 0;
	    pScreen->totalPixmapSize += sizeof(DevUnion);
	}
	pScreen->PixmapPrivateSizes = nsizes;
    }
    oldamount = pScreen->PixmapPrivateSizes[index2];
    if (amount > oldamount)
    {
	pScreen->PixmapPrivateSizes[index2] = amount;
	pScreen->totalPixmapSize += (amount - oldamount);
    }
    pScreen->totalPixmapSize = BitmapBytePad(pScreen->totalPixmapSize * 8);
    return TRUE;
}
#endif


/*
 *  colormap private machinery
 */

int  colormapPrivateCount;

void
ResetColormapPrivates()
{
    colormapPrivateCount = 0;
}


_X_EXPORT int
AllocateColormapPrivateIndex (InitCmapPrivFunc initPrivFunc)
{
    int		index;
    int		i;
    ColormapPtr	pColormap;
    DevUnion	*privs;

    index = colormapPrivateCount++;

    for (i = 0; i < screenInfo.numScreens; i++)
    {
	/*
	 * AllocateColormapPrivateIndex may be called after the
	 * default colormap has been created on each screen!
	 *
	 * We must resize the devPrivates array for the default
	 * colormap on each screen, making room for this new private.
	 * We also call the initialization function 'initPrivFunc' on
	 * the new private allocated for each default colormap.
	 */

	ScreenPtr pScreen = screenInfo.screens[i];

	pColormap = (ColormapPtr) LookupIDByType (
	    pScreen->defColormap, RT_COLORMAP);

	if (pColormap)
	{
	    privs = (DevUnion *) xrealloc (pColormap->devPrivates,
		colormapPrivateCount * sizeof(DevUnion));
	    if (!privs) {
		colormapPrivateCount--;
		return -1;
	    }
	    bzero(&privs[index], sizeof(DevUnion));
	    pColormap->devPrivates = privs;
	    if (!(*initPrivFunc)(pColormap,index))
	    {
		colormapPrivateCount--;
		return -1;
	    }
	}
    }

    return index;
}

/*
 *  device private machinery
 */

static int devicePrivateIndex = 0;

_X_EXPORT int
AllocateDevicePrivateIndex()
{
    return devicePrivateIndex++;
}

_X_EXPORT Bool
AllocateDevicePrivate(DeviceIntPtr device, int index)
{
    if (device->nPrivates < ++index) {
	DevUnion *nprivs = (DevUnion *) xrealloc(device->devPrivates,
						 index * sizeof(DevUnion));
	if (!nprivs)
	    return FALSE;
	device->devPrivates = nprivs;
	bzero(&nprivs[device->nPrivates], sizeof(DevUnion)
	      * (index - device->nPrivates));
	device->nPrivates = index;
	return TRUE;
    } else {
	return TRUE;
    }
}

void
ResetDevicePrivateIndex(void)
{
    devicePrivateIndex = 0;
}
