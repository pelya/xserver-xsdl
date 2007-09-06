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

typedef void *DevPrivateKey;

typedef struct _Private {
    DevPrivateKey	key;
    pointer		value;
    struct _Private	*next;
} PrivateRec;

/*
 * Request pre-allocated private space for your driver/module.
 * Calling this is not necessary if only a pointer by itself is needed.
 */
extern int
dixRequestPrivate(const DevPrivateKey key, unsigned size);

/*
 * Allocates a new private and attaches it to an existing object.
 */
extern pointer *
dixAllocatePrivate(PrivateRec **privates, const DevPrivateKey key);

/*
 * Look up a private pointer.
 */
static _X_INLINE pointer
dixLookupPrivate(PrivateRec **privates, const DevPrivateKey key)
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
dixLookupPrivateAddr(PrivateRec **privates, const DevPrivateKey key)
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
dixSetPrivate(PrivateRec **privates, const DevPrivateKey key, pointer val)
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
    DevPrivateKey key;	/* private registration key */
    pointer *value;	/* address of private pointer */
} PrivateCallbackRec;

extern int
dixRegisterPrivateInitFunc(const DevPrivateKey key, 
			   CallbackProcPtr callback, pointer userdata);

extern int
dixRegisterPrivateDeleteFunc(const DevPrivateKey key,
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
 * A negative value indicates no devPrivates field is available.
 */
extern int
dixRegisterPrivateOffset(RESTYPE type, int offset);

/*
 * Convenience macro for adding an offset to an object pointer
 * when making a call to one of the devPrivates functions
 */
#define DEVPRIV_AT(ptr, offset) ((PrivateRec **)((char *)ptr + offset))

#endif /* PRIVATES_H */
