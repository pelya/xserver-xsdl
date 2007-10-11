/***********************************************************

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#ifndef DIX_REGISTRY_H
#define DIX_REGISTRY_H

#ifdef XREGISTRY

#include "resource.h"

/* Internal string registry - for auditing, debugging, security, etc. */

/*
 * Registration functions.  The name string is not copied, so it must
 * not be a stack variable.
 */
void RegisterRequestName(unsigned major, unsigned minor, const char *name);
void RegisterEventName(unsigned event, const char *name);
void RegisterErrorName(unsigned error, const char *name);
void RegisterResourceName(RESTYPE type, const char *name);

/*
 * Lookup functions.  The returned string must not be modified.
 */
const char *LookupRequestName(int major, int minor);
const char *LookupEventName(int event);
const char *LookupErrorName(int error);
const char *LookupResourceName(RESTYPE rtype);

/*
 * Result returned from any unsuccessful lookup
 */
#define XREGISTRY_UNKNOWN "<unknown>"

/*
 * Setup and teardown
 */
void dixResetRegistry(void);

#else /* XREGISTRY */

/* Define calls away when the registry is not being built. */

#define RegisterRequestName(a, b, c) { ; }
#define RegisterEventName(a, b) { ; }
#define RegisterErrorName(a, b) { ; }
#define RegisterResourceName(a, b) { ; }

#define LookupRequestName(a, b) XREGISTRY_UNKNOWN
#define LookupEventName(a) XREGISTRY_UNKNOWN
#define LookupErrorName(a) XREGISTRY_UNKNOWN
#define LookupResourceName(a) XREGISTRY_UNKNOWN

#define dixResetRegistry() { ; }

#endif /* XREGISTRY */
#endif /* DIX_REGISTRY_H */
