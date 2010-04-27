/***********************************************************

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#ifndef PRIVATES_H
#define PRIVATES_H 1

#include <X11/Xdefs.h>
#include <X11/Xosdefs.h>
#include <X11/Xfuncproto.h>
#include "misc.h"

/*****************************************************************
 * STUFF FOR PRIVATES
 *****************************************************************/

typedef struct _Private PrivateRec, *PrivatePtr;

typedef enum {
    /* XSELinux uses the same private keys for numerous objects */
    PRIVATE_XSELINUX,

    /* Otherwise, you get a private in just the requested structure
     */
    /* These can have objects created before all of the keys are registered */
    PRIVATE_SCREEN,
    PRIVATE_EXTENSION,
    PRIVATE_COLORMAP,

    /* These cannot have any objects before all relevant keys are registered */
    PRIVATE_DEVICE,
    PRIVATE_CLIENT,
    PRIVATE_PROPERTY,
    PRIVATE_SELECTION,
    PRIVATE_WINDOW,
    PRIVATE_PIXMAP,
    PRIVATE_GC,
    PRIVATE_CURSOR,
    PRIVATE_CURSOR_BITS,

    /* extension privates */
    PRIVATE_DBE_WINDOW,
    PRIVATE_DAMAGE,
    PRIVATE_GLYPH,
    PRIVATE_GLYPHSET,
    PRIVATE_PICTURE,

    /* last private type */
    PRIVATE_LAST,
} DevPrivateType;

typedef struct _DevPrivateKeyRec {
    int			key;
} DevPrivateKeyRec, *DevPrivateKey;

/*
 * Let drivers know how to initialize private keys
 */

#define HAS_DEVPRIVATEKEYREC		1
#define HAS_DIXREGISTERPRIVATEKEY	1

/*
 * Register a new private index for the private type.
 *
 * This initializes the specified key and optionally requests pre-allocated
 * private space for your driver/module. If you request no extra space, you
 * may set and get a single pointer value using this private key. Otherwise,
 * you can get the address of the extra space and store whatever data you like
 * there.
 *
 * You may call dixRegisterPrivate more than once on the same key, but the
 * size and type must match or the server will abort.
 *
 * dixRegisterPrivateIndex returns FALSE if it fails to allocate memory
 * during its operation.
 */
extern _X_EXPORT Bool
dixRegisterPrivateKey(DevPrivateKey key, DevPrivateType type, unsigned size);

/*
 * Check whether a private key has been registered
 */
static inline Bool
dixPrivateKeyRegistered(DevPrivateKey key)
{
    return key->key != 0;
}

/*
 * Associate 'val' with 'key' in 'privates' so that later calls to
 * dixLookupPrivate(privates, key) will return 'val'.
 *
 * dixSetPrivate returns FALSE if a memory allocation fails.
 */
extern _X_EXPORT int
dixSetPrivate(PrivatePtr *privates, const DevPrivateKey key, pointer val);

#include "dix.h"
#include "resource.h"

/*
 * Lookup a pointer to the private record.
 *
 * For privates with defined storage, return the address of the
 * storage. For privates without defined storage, return the pointer
 * contents
 */
extern _X_EXPORT pointer
dixLookupPrivate(PrivatePtr *privates, const DevPrivateKey key);

/*
 * Look up the address of a private pointer.  If 'key' is not associated with a
 * value in 'privates', then dixLookupPrivateAddr calls dixAllocatePrivate and
 * returns a pointer to the resulting associated value.
 *
 * dixLookupPrivateAddr returns NULL if 'key' was not previously associated in
 * 'privates' and a memory allocation fails.
 */
extern _X_EXPORT pointer *
dixLookupPrivateAddr(PrivatePtr *privates, const DevPrivateKey key);

/*
 * Allocates private data separately from main object (clients and colormaps)
 */
static inline Bool
dixAllocatePrivates(PrivatePtr *privates, DevPrivateType type) { *privates = NULL; return TRUE; }

/*
 * Frees separately allocated private data (screens and clients)
 */
extern _X_EXPORT void
dixFreePrivates(PrivatePtr privates, DevPrivateType type);

/*
 * Initialize privates by zeroing them
 */
static inline void
_dixInitPrivates(PrivatePtr *privates, void *addr, DevPrivateType type) { *privates = NULL; }

#define dixInitPrivates(o, v, type) _dixInitPrivates(&(o)->devPrivates, (v), type);

/*
 * Clean up privates
 */
static inline void
_dixFiniPrivates(PrivatePtr privates, DevPrivateType type) { dixFreePrivates(privates, type); }

#define dixFiniPrivates(o,t)	_dixFiniPrivates((o)->devPrivates,t)

/*
 * Allocates private data at object creation time. Required
 * for all objects other than ScreenRecs.
 */
static inline void *
_dixAllocateObjectWithPrivates(unsigned size, unsigned clear, unsigned offset, DevPrivateType type) {
    return calloc(size, 1);
}

#define dixAllocateObjectWithPrivates(t, type) (t *) _dixAllocateObjectWithPrivates(sizeof(t), sizeof(t), offsetof(t, devPrivates), type)

static inline void
_dixFreeObjectWithPrivates(void *object, PrivatePtr privates, DevPrivateType type) {
    dixFreePrivates(privates, type);
    free(object);
}

#define dixFreeObjectWithPrivates(o,t) _dixFreeObjectWithPrivates(o, (o)->devPrivates, t)

/*
 * Return size of privates for the specified type
 */
static inline int
dixPrivatesSize(DevPrivateType type) { return 0; }

/*
 * Dump out private stats to ErrorF
 */
void
dixPrivateUsage(void);

/*
 * Resets the privates subsystem.  dixResetPrivates is called from the main loop
 * before each server generation.  This function must only be called by main().
 */
extern _X_EXPORT void
dixResetPrivates(void);

/*
 * Looks up the offset where the devPrivates field is located.
 * Returns -1 if the specified resource has no dev privates.
 * The position of the devPrivates field varies by structure
 * and calling code might only know the resource type, not the
 * structure definition.
 */
extern _X_EXPORT int
dixLookupPrivateOffset(RESTYPE type);

/*
 * Convenience macro for adding an offset to an object pointer
 * when making a call to one of the devPrivates functions
 */
#define DEVPRIV_AT(ptr, offset) ((PrivatePtr *)((char *)(ptr) + offset))

#endif /* PRIVATES_H */
