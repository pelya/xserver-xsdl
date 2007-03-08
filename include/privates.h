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

#include "dix.h"
#include "resource.h"

/*****************************************************************
 * STUFF FOR PRIVATES
 *****************************************************************/

typedef char devprivate_key_t;

typedef struct _Private {
    devprivate_key_t	*key;
    pointer		value;
    struct _Private	*next;
} PrivateRec;

/*
 * Backwards compatibility macro.  Use to get the proper PrivateRec
 * reference from any of the structure types that supported the old
 * devPrivates mechanism.
 */
#define DEVPRIV_PTR(foo) ((PrivateRec **)(&(foo)->devPrivates[0].ptr))

/*
 * Request pre-allocated private space for your driver/module.
 * Calling this is not necessary if only a pointer by itself is needed.
 */
extern int
dixRequestPrivate(devprivate_key_t *const key, unsigned size);

/*
 * Allocates a new private and attaches it to an existing object.
 */
extern pointer *
dixAllocatePrivate(PrivateRec **privates, devprivate_key_t *const key);

/*
 * Look up a private pointer.
 */
static _X_INLINE pointer
dixLookupPrivate(PrivateRec **privates, devprivate_key_t *const key)
{
    PrivateRec *rec = *privates;
    pointer *ptr;

    while (rec) {
	if (rec->key == key)
	    return rec->value;
	rec = rec->next;
    }

    ptr = dixAllocatePrivate(privates, key);
    return ptr ? *ptr : NULL;
}

/*
 * Look up the address of a private pointer.
 */
static _X_INLINE pointer *
dixLookupPrivateAddr(PrivateRec **privates, devprivate_key_t *const key)
{
    PrivateRec *rec = *privates;

    while (rec) {
	if (rec->key == key)
	    return &rec->value;
	rec = rec->next;
    }

    return dixAllocatePrivate(privates, key);
}

/*
 * Set a private pointer.
 */
static _X_INLINE int
dixSetPrivate(PrivateRec **privates, devprivate_key_t *const key, pointer val)
{
    PrivateRec *rec;

 top:
    rec = *privates;
    while (rec) {
	if (rec->key == key) {
	    rec->value = val;
	    return TRUE;
	}
	rec = rec->next;
    }

    if (!dixAllocatePrivate(privates, key))
	return FALSE;
    goto top;
}

/*
 * Register callbacks to be called on private allocation/freeing.
 * The calldata argument to the callbacks is a PrivateCallbackPtr.
 */
typedef struct _PrivateCallback {
    devprivate_key_t *key;	/* private registration key */
    pointer value;		/* pointer to private */
} PrivateCallbackRec;

extern int
dixRegisterPrivateInitFunc(devprivate_key_t *const key,
			   CallbackProcPtr callback, pointer userdata);

extern int
dixRegisterPrivateDeleteFunc(devprivate_key_t *const key,
			     CallbackProcPtr callback, pointer userdata);

/*
 * Frees private data.
 */
extern void
dixFreePrivates(PrivateRec *privates);

/*
 * Resets the subsystem, called from the main loop.
 */
extern int
dixResetPrivates(void);

/*
 * These next two functions are necessary because the position of
 * the devPrivates field varies by structure and calling code might
 * only know the resource type, not the structure definition.
 */

/*
 * Looks up the offset where the devPrivates field is located.
 * Returns -1 if no offset has been registered for the resource type.
 */
extern int
dixLookupPrivateOffset(RESTYPE type);

/*
 * Specifies the offset where the devPrivates field is located.
 */
extern int
dixRegisterPrivateOffset(RESTYPE type, unsigned offset);

/* Used by the legacy support, don't rely on this being here */
#define PadToLong(w) ((((w) + sizeof(long)-1) / sizeof(long)) * sizeof(long))

#endif /* PRIVATES_H */
