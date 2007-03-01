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

typedef struct _PrivateKey {
    int unused;
} devprivate_key_t;

typedef struct _Private {
    devprivate_key_t	*key;
    int			dontfree;
    pointer		value;
    struct _Private	*next;
} PrivateRec;

/*
 * Request pre-allocated private space for your driver/module.
 * A non-null pScreen argument restricts to objects on a given screen.
 */
extern int
dixRequestPrivate(RESTYPE type, devprivate_key_t *const key,
		  unsigned size, pointer pScreen);

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
 * Allocates all pre-requested private space in one chunk.
 */
extern PrivateRec *
dixAllocatePrivates(RESTYPE type, pointer parent);

/*
 * Frees any private space that is not part of an object.
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
 * Looks up the offset where the devPrivates field is located by type.
 */
extern unsigned
dixLookupPrivateOffset(RESTYPE type);

/*
 * Specifies the offset where the devPrivates field is located.
 */
extern int
dixRegisterPrivateOffset(RESTYPE type, unsigned offset);

#endif /* PRIVATES_H */
