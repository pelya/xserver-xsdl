/***********************************************************

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#ifndef DIX_ACCESS_H
#define DIX_ACCESS_H

/* These are the access modes that can be passed in the last parameter
 * to several of the dix lookup functions.  They were originally part
 * of the Security extension, now used by XACE.
 *
 * You can or these values together to indicate multiple modes
 * simultaneously.
 */

#define DixUnknownAccess	0	/* don't know intentions */
#define DixReadAccess		(1<<0)	/* inspecting the object */
#define DixWriteAccess		(1<<1)	/* changing the object */
#define DixDestroyAccess	(1<<2)	/* destroying the object */
#define DixCreateAccess		(1<<3)	/* creating the object */

#endif /* DIX_ACCESS_H */
