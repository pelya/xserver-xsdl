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

/*
 * Request private space for your driver/module in all resources of a type.
 * A non-null pScreen argument restricts to resources on a given screen.
 */
extern int
dixRequestPrivate(RESTYPE type, unsigned size, pointer pScreen);

/*
 * Request private space in just one individual resource object.
 */
extern int
dixRequestSinglePrivate(RESTYPE type, unsigned size, pointer instance);

/*
 * Look up a private pointer.
 */
extern pointer
dixLookupPrivate(RESTYPE type, int index, pointer instance);

/*
 * Register callbacks to be called on private allocation/freeing.
 * The calldata argument to the callbacks is a PrivateCallbackPtr.
 */
typedef struct _PrivateCallback {
    pointer value;		/* pointer to private */
    int index;			/* registration index */
    ResourcePtr resource;	/* resource record (do not modify!) */
} PrivateCallbackRec, *PrivateCallbackPtr;

extern int
dixRegisterPrivateInitFunc(RESTYPE type, int index,
			   CallbackProcPtr callback, pointer userdata);

extern int
dixRegisterPrivateDeleteFunc(RESTYPE type, int index,
			     CallbackProcPtr callback, pointer userdata);

/*
 * Internal functions
 */
extern void
dixResetPrivates(void);

extern int
dixUpdatePrivates(void);

extern ResourcePtr
dixAllocateResourceRec(RESTYPE type, pointer value, pointer parent);

extern void
dixCallPrivateInitFuncs(ResourcePtr res);

extern void
dixFreeResourceRec(ResourcePtr res);

#endif /* PRIVATES_H */
