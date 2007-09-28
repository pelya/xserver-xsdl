/************************************************************

Author: Eamon Walsh <ewalsh@epoch.ncsc.mil>

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
this permission notice appear in supporting documentation.  This permission
notice shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

********************************************************/

/*
 * Portions of this code copyright (c) 2005 by Trusted Computer Solutions, Inc.
 * All rights reserved.
 */

#include <selinux/selinux.h>
#include <selinux/label.h>
#include <selinux/avc.h>

#include <libaudit.h>

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/Xfuncproto.h>
#include "dixstruct.h"
#include "extnsionst.h"
#include "resource.h"
#include "selection.h"
#include "xacestr.h"
#include "xselinux.h"
#define XSERV_t
#define TRANS_SERVER
#include <X11/Xtrans/Xtrans.h>
#include "../os/osdep.h"
#include <stdio.h>
#include <stdarg.h>
#include "modinit.h"

#ifndef XSELINUXCONFIGFILE
#warning "XSELinux Policy file is not defined"
#define XSELINUXCONFIGFILE  NULL
#endif

/* devPrivates in client and extension */
static int clientPrivateIndex;
static int extnsnPrivateIndex;

/* audit file descriptor */
static int audit_fd;

/* structure passed to auditing callback */
typedef struct {
    ClientPtr client;	/* client */
    char *property;	/* property name, if any */
    char *extension;	/* extension name, if any */
} XSELinuxAuditRec;

/* labeling handle */
static struct selabel_handle *label_hnd;

/* Atoms for SELinux window labeling properties */
Atom atom_ctx;
Atom atom_client_ctx;

/* Selection stuff from dix */
extern Selection *CurrentSelections;
extern int NumCurrentSelections;

/* Dynamically allocated security classes and permissions */
static struct security_class_mapping map[] = {
    { "drawable",
      { "create", "destroy", "draw", "copy", "getattr", NULL }},
    { "window",
      { "addchild", "create", "destroy", "map", "unmap", "chstack",
	"chproplist", "chprop", "listprop", "getattr", "setattr", "setfocus",
	"move", "chselection", "chparent", "ctrllife", "enumerate",
	"transparent", "mousemotion", "clientcomevent", "inputevent",
	"drawevent", "windowchangeevent", "windowchangerequest",
	"serverchangeevent", "extensionevent", NULL }},
    { "gc",
      { "create", "free", "getattr", "setattr", NULL }},
    { "font",
      { "load", "free", "getattr", "use", NULL }},
    { "colormap",
      { "create", "free", "install", "uninstall", "list", "read", "store",
	"getattr", "setattr", NULL }},
    { "property",
      { "create", "free", "read", "write", NULL }},
    { "cursor",
      { "create", "createglyph", "free", "assign", "setattr", NULL }},
    { "xclient",
      { "kill", NULL }},
    { "xinput",
      { "lookup", "getattr", "setattr", "setfocus", "warppointer",
	"activegrab", "passivegrab", "ungrab", "bell", "mousemotion",
	"relabelinput", NULL }},
    { "xserver",
      { "screensaver", "gethostlist", "sethostlist", "getfontpath",
	"setfontpath", "getattr", "grab", "ungrab", NULL }},
    { "xextension",
      { "query", "use", NULL }},
    { NULL }
};

/*
 * list of classes corresponding to SIDs in the
 * rsid array of the security state structure (below).
 *
 * XXX SIDs should be stored in their native objects, not all
 * bunched together in the client structure.  However, this will
 * require modification to the resource manager.
 */
static security_class_t sClasses[] = {
    SECCLASS_WINDOW,
    SECCLASS_DRAWABLE,
    SECCLASS_GC,
    SECCLASS_CURSOR,
    SECCLASS_FONT,
    SECCLASS_COLORMAP,
    SECCLASS_PROPERTY,
    SECCLASS_XCLIENT,
    SECCLASS_XINPUT,
    SECCLASS_XSERVER
};
#define NRES (sizeof(sClasses)/sizeof(sClasses[0]))

/* This is what we store for client security state */
typedef struct {
    int haveState;
    security_id_t sid;
    security_id_t rsid[NRES];
    struct avc_entry_ref aeref;
} XSELinuxClientStateRec;

/* Convenience macros for accessing security state fields */
#define STATEPTR(client) \
    ((client)->devPrivates[clientPrivateIndex].ptr)
#define HAVESTATE(client) \
    (((XSELinuxClientStateRec*)STATEPTR(client))->haveState)
#define SID(client) \
    (((XSELinuxClientStateRec*)STATEPTR(client))->sid)
#define RSID(client,n) \
    (((XSELinuxClientStateRec*)STATEPTR(client))->rsid[n])
#define AEREF(client) \
    (((XSELinuxClientStateRec*)STATEPTR(client))->aeref)
#define EXTENSIONSID(ext) \
    ((ext)->devPrivates[extnsnPrivateIndex].ptr)

/*
 * Returns the index into the rsid array where the SID for the
 * given class is stored.
 */
static int
IndexByClass(security_class_t class)
{
    int i;
    for (i=0; i<NRES; i++)
	if (class == sClasses[i])
	    return i;
    return 0;
}

/*
 * Does sanity checking on a resource ID.  This can be removed after
 * testing.
 */
static void
CheckXID(XID id)
{
    /*
    XID c = CLIENT_ID(id);

    if (c > 10)
	ErrorF("Warning: possibly mangled ID %x\n", id);

    c = id & RESOURCE_ID_MASK;
    if (c > 100)
	ErrorF("Warning: possibly mangled ID %x\n", id);
        */
}

/*
 * Byte-swap a CARD32 id if necessary.
 */
static XID
SwapXID(ClientPtr client, XID id)
{
    register char n;
    if (client->swapped)
	swapl(&id, n);
    return id;
}

/*
 * ServerPerm - check access permissions on a server-owned object.
 *
 * Arguments:
 * client: Client doing the request.
 * class: Security class of the server object being accessed.
 * perm: Permissions required on the object.
 *
 * Returns: X status code.
 */
static int
ServerPerm(ClientPtr client,
	   security_class_t class,
	   access_vector_t perm)
{
    int idx = IndexByClass(class);
    if (HAVESTATE(client))
    {
	XSELinuxAuditRec auditdata;
	auditdata.client = client;
	auditdata.property = NULL;
	auditdata.extension = NULL;
	errno = 0;
        if (avc_has_perm(SID(client), RSID(serverClient,idx), class,
                         perm, &AEREF(client), &auditdata) < 0)
        {
            if (errno == EACCES)
		return BadAccess;
	    ErrorF("ServerPerm: unexpected error %d\n", errno);
	    return BadValue;
        }
    }
    else
    {
	ErrorF("No client state in server-perm check!\n");
        return Success;
    }

    return Success;
}

/*
 * IDPerm - check access permissions on a resource.
 *
 * Arguments:
 * client: Client doing the request.
 * id: resource id of the resource being accessed.
 * class: Security class of the resource being accessed.
 * perm: Permissions required on the resource.
 *
 * Returns: X status code.
 */
static int
IDPerm(ClientPtr sclient,
	 XID id,
	 security_class_t class,
	 access_vector_t perm)
{
    ClientPtr tclient;
    int idx = IndexByClass(class);
    XSELinuxAuditRec auditdata;

    if (id == None)
	return Success;

    CheckXID(id);
    tclient = clients[CLIENT_ID(id)];

    /*
     * This happens in the case where a client has
     * disconnected.  XXX might want to make the server
     * own orphaned resources...
     */
    if (!tclient || !HAVESTATE(tclient) || !HAVESTATE(sclient))
    {
	return Success;
    }

    auditdata.client = sclient;
    auditdata.property = NULL;
    auditdata.extension = NULL;
    errno = 0;
    if (avc_has_perm(SID(sclient), RSID(tclient,idx), class,
		     perm, &AEREF(sclient), &auditdata) < 0)
    {
	if (errno == EACCES)
	    return BadAccess;
	ErrorF("IDPerm: unexpected error %d\n", errno);
	return BadValue;
    }

    return Success;
}

/*
 * GetPropertySID - compute SID for a property object.
 *
 * Arguments:
 * basecontext: context of client owning the property.
 * name: name of the property.
 *
 * Returns: proper SID for the object or NULL on error.
 */
static security_id_t
GetPropertySID(security_context_t base, const char *name)
{
    security_context_t con, result;
    security_id_t sid = NULL;

    /* look in the mappings of names to types */
    if (selabel_lookup(label_hnd, &con, name, SELABEL_X_PROP) < 0)
	goto out;

    /* perform a transition to obtain the final context */
    if (security_compute_create(base, con, SECCLASS_PROPERTY, &result) < 0)
	goto out2;

    /* get a SID for the context */
    avc_context_to_sid(result, &sid);
    freecon(result);
  out2:
    freecon(con);
  out:
    return sid;
}

/*
 * GetExtensionSID - compute SID for an extension object.
 *
 * Arguments:
 * name: name of the extension.
 *
 * Returns: proper SID for the object or NULL on error.
 */
static security_id_t
GetExtensionSID(const char *name)
{
    security_context_t base, con, result;
    security_id_t sid = NULL;

    /* get server context */
    if (getcon(&base) < 0)
	goto out;

    /* look in the mappings of names to types */
    if (selabel_lookup(label_hnd, &con, name, SELABEL_X_EXT) < 0)
	goto out2;

    /* perform a transition to obtain the final context */
    if (security_compute_create(base, con, SECCLASS_XEXTENSION, &result) < 0)
	goto out3;

    /* get a SID for the context */
    avc_context_to_sid(result, &sid);
    freecon(result);
  out3:
    freecon(con);
  out2:
    freecon(base);
  out:
    return sid;
}

/*
 * AssignServerState - set up server security state.
 *
 * Arguments:
 */
static void
AssignServerState(void)
{
    int i;
    security_context_t basectx, objctx;
    XSELinuxClientStateRec *state;

    state = (XSELinuxClientStateRec*)STATEPTR(serverClient);
    avc_entry_ref_init(&state->aeref);

    /* use the context of the X server process for the serverClient */
    if (getcon(&basectx) < 0)
	FatalError("Couldn't get context of X server process\n");

    /* get a SID from the context */
    if (avc_context_to_sid(basectx, &state->sid) < 0)
	FatalError("Client %d: context_to_sid(%s) failed\n", 0, basectx);

    /* get contexts and then SIDs for each resource type */
    for (i=0; i<NRES; i++) {
	if (security_compute_create(basectx, basectx, sClasses[i],
				    &objctx) < 0)
	    FatalError("Client %d: compute_create(base=%s, cls=%d) failed\n",
		       0, basectx, sClasses[i]);

	if (avc_context_to_sid(objctx, &state->rsid[i]) < 0)
	    FatalError("Client %d: context_to_sid(%s) failed\n",
		       0, objctx);

	freecon(objctx);
    }

    /* mark as set up, free base context, and return */
    state->haveState = TRUE;
    freecon(basectx);
}

/*
 * AssignClientState - set up client security state.
 *
 * Arguments:
 * client: client to set up (can be serverClient).
 */
static void
AssignClientState(ClientPtr client)
{
    int i;
    security_context_t basectx, objctx;
    XSELinuxClientStateRec *state;

    state = (XSELinuxClientStateRec*)STATEPTR(client);
    avc_entry_ref_init(&state->aeref);

    XtransConnInfo ci = ((OsCommPtr)client->osPrivate)->trans_conn;
    if (_XSERVTransIsLocal(ci)) {
	/* for local clients, can get context from the socket */
	int fd = _XSERVTransGetConnectionNumber(ci);
	if (getpeercon(fd, &basectx) < 0)
	    FatalError("Client %d: couldn't get context from socket\n",
		       client->index);
    }
    else
	/* for remote clients, need to use a default context */
	if (selabel_lookup(label_hnd, &basectx, NULL, SELABEL_X_CLIENT) < 0)
	    FatalError("Client %d: couldn't get default remote connection context\n",
		       client->index);

    /* get a SID from the context */
    if (avc_context_to_sid(basectx, &state->sid) < 0)
	FatalError("Client %d: context_to_sid(%s) failed\n",
		   client->index, basectx);

    /* get contexts and then SIDs for each resource type */
    for (i=0; i<NRES; i++) {
	if (security_compute_create(basectx, basectx, sClasses[i],
				    &objctx) < 0)
	    FatalError("Client %d: compute_create(base=%s, cls=%d) failed\n",
		       client->index, basectx, sClasses[i]);

	if (avc_context_to_sid(objctx, &state->rsid[i]) < 0)
	    FatalError("Client %d: context_to_sid(%s) failed\n",
		       client->index, objctx);

	freecon(objctx);
    }

    /* mark as set up, free base context, and return */
    state->haveState = TRUE;
    freecon(basectx);
}

/*
 * FreeClientState - tear down client security state.
 *
 * Arguments:
 * client: client to release (can be serverClient).
 */
static void
FreeClientState(ClientPtr client)
{
    int i;
    XSELinuxClientStateRec *state = (XSELinuxClientStateRec*)STATEPTR(client);

    /* client state may not be set up if its auth was rejected */
    if (state->haveState) {
	state = (XSELinuxClientStateRec*)STATEPTR(client);
	sidput(state->sid);
	for (i=0; i<NRES; i++)
	    sidput(state->rsid[i]);
	state->haveState = FALSE;
    }
}

#define REQUEST_SIZE_CHECK(client, req) \
    (client->req_len >= (sizeof(req) >> 2))
#define IDPERM(client, req, field, class, perm) \
    (REQUEST_SIZE_CHECK(client,req) ? \
     IDPerm(client, SwapXID(client,((req*)stuff)->field), class, perm) : \
     BadLength)

static int
CheckSendEventPerms(ClientPtr client)
{
    register char n;
    access_vector_t perm = 0;
    REQUEST(xSendEventReq);

    /* might need type bounds checking here */
    if (!REQUEST_SIZE_CHECK(client, xSendEventReq))
	return BadLength;

    switch (stuff->event.u.u.type) {
	case SelectionClear:
	case SelectionNotify:
	case SelectionRequest:
	case ClientMessage:
	case PropertyNotify:
	    perm = WINDOW__CLIENTCOMEVENT;
	    break;
	case ButtonPress:
	case ButtonRelease:
	case KeyPress:
	case KeyRelease:
	case KeymapNotify:
	case MotionNotify:
	case EnterNotify:
	case LeaveNotify:
	case FocusIn:
	case FocusOut:
	    perm = WINDOW__INPUTEVENT;
	    break;
	case Expose:
	case GraphicsExpose:
	case NoExpose:
	case VisibilityNotify:
	    perm = WINDOW__DRAWEVENT;
	    break;
	case CirculateNotify:
	case ConfigureNotify:
	case CreateNotify:
	case DestroyNotify:
	case MapNotify:
	case UnmapNotify:
	case GravityNotify:
	case ReparentNotify:
	    perm = WINDOW__WINDOWCHANGEEVENT;
	    break;
	case CirculateRequest:
	case ConfigureRequest:
	case MapRequest:
	case ResizeRequest:
	    perm = WINDOW__WINDOWCHANGEREQUEST;
	    break;
	case ColormapNotify:
	case MappingNotify:
	    perm = WINDOW__SERVERCHANGEEVENT;
	    break;
	default:
	    perm = WINDOW__EXTENSIONEVENT;
	    break;
    }
    if (client->swapped)
	swapl(&stuff->destination, n);
    return IDPerm(client, stuff->destination, SECCLASS_WINDOW, perm);
}

static int
CheckConvertSelectionPerms(ClientPtr client)
{
    register char n;
    int rval = Success;
    REQUEST(xConvertSelectionReq);

    if (!REQUEST_SIZE_CHECK(client, xConvertSelectionReq))
	return BadLength;

    if (client->swapped)
    {
	swapl(&stuff->selection, n);
	swapl(&stuff->requestor, n);
    }

    if (ValidAtom(stuff->selection))
    {
	int i = 0;
	while ((i < NumCurrentSelections) &&
	       CurrentSelections[i].selection != stuff->selection) i++;
	if (i < NumCurrentSelections) {
	    rval = IDPerm(client, CurrentSelections[i].window,
			  SECCLASS_WINDOW, WINDOW__CLIENTCOMEVENT);
	    if (rval != Success)
		return rval;
	}
    }
    return IDPerm(client, stuff->requestor,
		  SECCLASS_WINDOW, WINDOW__CLIENTCOMEVENT);
}

static int
CheckSetSelectionOwnerPerms(ClientPtr client)
{
    register char n;
    int rval = Success;
    REQUEST(xSetSelectionOwnerReq);

    if (!REQUEST_SIZE_CHECK(client, xSetSelectionOwnerReq))
	return BadLength;

    if (client->swapped)
    {
	swapl(&stuff->selection, n);
	swapl(&stuff->window, n);
    }

    if (ValidAtom(stuff->selection))
    {
	int i = 0;
	while ((i < NumCurrentSelections) &&
	       CurrentSelections[i].selection != stuff->selection) i++;
	if (i < NumCurrentSelections) {
	    rval = IDPerm(client, CurrentSelections[i].window,
			  SECCLASS_WINDOW, WINDOW__CHSELECTION);
	    if (rval != Success)
		return rval;
	}
    }
    return IDPerm(client, stuff->window,
			  SECCLASS_WINDOW, WINDOW__CHSELECTION);
}

static void
XSELinuxCoreDispatch(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceCoreDispatchRec *rec = (XaceCoreDispatchRec*)calldata;
    ClientPtr client = rec->client;
    REQUEST(xReq);
    int rval = Success, rval2 = Success, rval3 = Success;

    switch(stuff->reqType) {
    /* Drawable class control requirements */
    case X_ClearArea:
	rval = IDPERM(client, xClearAreaReq, window,
		      SECCLASS_DRAWABLE, DRAWABLE__DRAW);
	break;
    case X_PolySegment:
    case X_PolyRectangle:
    case X_PolyArc:
    case X_PolyFillRectangle:
    case X_PolyFillArc:
	rval = IDPERM(client, xPolySegmentReq, drawable,
		      SECCLASS_DRAWABLE, DRAWABLE__DRAW);
	break;
    case X_PolyPoint:
    case X_PolyLine:
	rval = IDPERM(client, xPolyPointReq, drawable,
		      SECCLASS_DRAWABLE, DRAWABLE__DRAW);
	break;
    case X_FillPoly:
	rval = IDPERM(client, xFillPolyReq, drawable,
		      SECCLASS_DRAWABLE, DRAWABLE__DRAW);
	break;
    case X_PutImage:
	rval = IDPERM(client, xPutImageReq, drawable,
		      SECCLASS_DRAWABLE, DRAWABLE__DRAW);
	break;
    case X_CopyArea:
    case X_CopyPlane:
	rval = IDPERM(client, xCopyAreaReq, srcDrawable,
		      SECCLASS_DRAWABLE, DRAWABLE__COPY);
	rval2 = IDPERM(client, xCopyAreaReq, dstDrawable,
		       SECCLASS_DRAWABLE, DRAWABLE__DRAW);
	break;
    case X_GetImage:
	rval = IDPERM(client, xGetImageReq, drawable,
		      SECCLASS_DRAWABLE, DRAWABLE__COPY);
	break;
    case X_GetGeometry:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_DRAWABLE, DRAWABLE__GETATTR);
	break;

    /* Window class control requirements */
    case X_ChangeProperty:
	rval = IDPERM(client, xChangePropertyReq, window,
		      SECCLASS_WINDOW,
		      WINDOW__CHPROPLIST | WINDOW__CHPROP |
		      WINDOW__LISTPROP);
	break;
    case X_ChangeSaveSet:
	rval = IDPERM(client, xChangeSaveSetReq, window,
		      SECCLASS_WINDOW,
		      WINDOW__CTRLLIFE | WINDOW__CHPARENT);
	break;
    case X_ChangeWindowAttributes:
	rval = IDPERM(client, xChangeWindowAttributesReq, window,
		      SECCLASS_WINDOW, WINDOW__SETATTR);
	break;
    case X_CirculateWindow:
	rval = IDPERM(client, xCirculateWindowReq, window,
		      SECCLASS_WINDOW, WINDOW__CHSTACK);
	break;
    case X_ConfigureWindow:
	rval = IDPERM(client, xConfigureWindowReq, window,
		      SECCLASS_WINDOW,
		      WINDOW__SETATTR | WINDOW__MOVE | WINDOW__CHSTACK);
	break;
    case X_ConvertSelection:
	rval = CheckConvertSelectionPerms(client);
	break;
    case X_CreateWindow:
	rval = IDPERM(client, xCreateWindowReq, wid,
		      SECCLASS_WINDOW,
		      WINDOW__CREATE | WINDOW__SETATTR | WINDOW__MOVE);
	rval2 = IDPERM(client, xCreateWindowReq, parent,
		       SECCLASS_WINDOW,
		       WINDOW__CHSTACK | WINDOW__ADDCHILD);
	rval3 = IDPERM(client, xCreateWindowReq, wid,
		       SECCLASS_DRAWABLE, DRAWABLE__CREATE);
	break;
    case X_DeleteProperty:
	rval = IDPERM(client, xDeletePropertyReq, window,
		      SECCLASS_WINDOW,
		      WINDOW__CHPROP | WINDOW__CHPROPLIST);
	break;
    case X_DestroyWindow:
    case X_DestroySubwindows:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_WINDOW,
		      WINDOW__ENUMERATE | WINDOW__UNMAP | WINDOW__DESTROY);
	rval2 = IDPERM(client, xResourceReq, id,
		       SECCLASS_DRAWABLE, DRAWABLE__DESTROY);
	break;
    case X_GetMotionEvents:
	rval = IDPERM(client, xGetMotionEventsReq, window,
		      SECCLASS_WINDOW, WINDOW__MOUSEMOTION);
	break;
    case X_GetProperty:
	rval = IDPERM(client, xGetPropertyReq, window,
		      SECCLASS_WINDOW, WINDOW__LISTPROP);
	break;
    case X_GetWindowAttributes:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_WINDOW, WINDOW__GETATTR);
	break;
    case X_KillClient:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_XCLIENT, XCLIENT__KILL);
	break;
    case X_ListProperties:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_WINDOW, WINDOW__LISTPROP);
	break;
    case X_MapWindow:
    case X_MapSubwindows:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_WINDOW,
		      WINDOW__ENUMERATE | WINDOW__GETATTR | WINDOW__MAP);
	break;
    case X_QueryTree:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_WINDOW, WINDOW__ENUMERATE | WINDOW__GETATTR);
	break;
    case X_RotateProperties:
	rval = IDPERM(client, xRotatePropertiesReq, window,
		      SECCLASS_WINDOW, WINDOW__CHPROP | WINDOW__CHPROPLIST);
	break;
    case X_ReparentWindow:
	rval = IDPERM(client, xReparentWindowReq, window,
		      SECCLASS_WINDOW, WINDOW__CHPARENT | WINDOW__MOVE);
	rval2 = IDPERM(client, xReparentWindowReq, parent,
		       SECCLASS_WINDOW, WINDOW__CHSTACK | WINDOW__ADDCHILD);
	break;
    case X_SendEvent:
	rval = CheckSendEventPerms(client);
	break;
    case X_SetInputFocus:
	rval = IDPERM(client, xSetInputFocusReq, focus,
		      SECCLASS_WINDOW, WINDOW__SETFOCUS);
	rval2 = ServerPerm(client, SECCLASS_XINPUT, XINPUT__SETFOCUS);
	break;
    case X_SetSelectionOwner:
	rval = CheckSetSelectionOwnerPerms(client);
	break;
    case X_TranslateCoords:
	rval = IDPERM(client, xTranslateCoordsReq, srcWid,
		      SECCLASS_WINDOW, WINDOW__GETATTR);
	rval2 = IDPERM(client, xTranslateCoordsReq, dstWid,
		       SECCLASS_WINDOW, WINDOW__GETATTR);
	break;
    case X_UnmapWindow:
    case X_UnmapSubwindows:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_WINDOW,
		      WINDOW__ENUMERATE | WINDOW__GETATTR |
		      WINDOW__UNMAP);
	break;
    case X_WarpPointer:
	rval = IDPERM(client, xWarpPointerReq, srcWid,
		      SECCLASS_WINDOW, WINDOW__GETATTR);
	rval2 = IDPERM(client, xWarpPointerReq, dstWid,
		       SECCLASS_WINDOW, WINDOW__GETATTR);
	rval3 = ServerPerm(client, SECCLASS_XINPUT, XINPUT__WARPPOINTER);
	break;

    /* Input class control requirements */
    case X_GrabButton:
    case X_GrabKey:
	rval = ServerPerm(client, SECCLASS_XINPUT, XINPUT__PASSIVEGRAB);
	break;
    case X_GrabKeyboard:
    case X_GrabPointer:
    case X_ChangeActivePointerGrab:
	rval = ServerPerm(client, SECCLASS_XINPUT, XINPUT__ACTIVEGRAB);
	break;
    case X_AllowEvents:
    case X_UngrabButton:
    case X_UngrabKey:
    case X_UngrabKeyboard:
    case X_UngrabPointer:
	rval = ServerPerm(client, SECCLASS_XINPUT, XINPUT__UNGRAB);
	break;
    case X_GetKeyboardControl:
    case X_GetKeyboardMapping:
    case X_GetPointerControl:
    case X_GetPointerMapping:
    case X_GetModifierMapping:
    case X_QueryKeymap:
    case X_QueryPointer:
	rval = ServerPerm(client, SECCLASS_XINPUT, XINPUT__GETATTR);
	break;
    case X_ChangeKeyboardControl:
    case X_ChangePointerControl:
    case X_ChangeKeyboardMapping:
    case X_SetModifierMapping:
    case X_SetPointerMapping:
	rval = ServerPerm(client, SECCLASS_XINPUT, XINPUT__SETATTR);
	break;
    case X_Bell:
	rval = ServerPerm(client, SECCLASS_XINPUT, XINPUT__BELL);
	break;

    /* Colormap class control requirements */
    case X_AllocColor:
    case X_AllocColorCells:
    case X_AllocColorPlanes:
    case X_AllocNamedColor:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_COLORMAP,
		      COLORMAP__READ | COLORMAP__STORE);
	break;
    case X_CopyColormapAndFree:
	rval = IDPERM(client, xCopyColormapAndFreeReq, mid,
		      SECCLASS_COLORMAP, COLORMAP__CREATE);
	rval2 = IDPERM(client, xCopyColormapAndFreeReq, srcCmap,
		       SECCLASS_COLORMAP,
		       COLORMAP__READ | COLORMAP__FREE);
	break;
    case X_CreateColormap:
	rval = IDPERM(client, xCreateColormapReq, mid,
		      SECCLASS_COLORMAP, COLORMAP__CREATE);
	rval2 = IDPERM(client, xCreateColormapReq, window,
		       SECCLASS_DRAWABLE, DRAWABLE__DRAW);
	break;
    case X_FreeColormap:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_COLORMAP, COLORMAP__FREE);
	break;
    case X_FreeColors:
	rval = IDPERM(client, xFreeColorsReq, cmap,
		      SECCLASS_COLORMAP, COLORMAP__STORE);
	break;
    case X_InstallColormap:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_COLORMAP, COLORMAP__INSTALL);
	rval2 = ServerPerm(client, SECCLASS_COLORMAP, COLORMAP__INSTALL);
	break;
    case X_ListInstalledColormaps:
	rval = ServerPerm(client, SECCLASS_COLORMAP, COLORMAP__LIST);
	break;
    case X_LookupColor:
    case X_QueryColors:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_COLORMAP, COLORMAP__READ);
	break;
    case X_StoreColors:
    case X_StoreNamedColor:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_COLORMAP, COLORMAP__STORE);
	break;
    case X_UninstallColormap:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_COLORMAP, COLORMAP__UNINSTALL);
	rval2 = ServerPerm(client, SECCLASS_COLORMAP, COLORMAP__UNINSTALL);
	break;

    /* Font class control requirements */
    case X_CloseFont:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_FONT, FONT__FREE);
	break;
    case X_ImageText8:
    case X_ImageText16:
	/* Font accesses checked through the resource manager */
	rval = IDPERM(client, xImageTextReq, drawable,
		      SECCLASS_DRAWABLE, DRAWABLE__DRAW);
	break;
    case X_OpenFont:
	rval = ServerPerm(client, SECCLASS_FONT, FONT__LOAD);
	rval2 = IDPERM(client, xOpenFontReq, fid,
		       SECCLASS_FONT, FONT__USE);
	break;
    case X_PolyText8:
    case X_PolyText16:
	/* Font accesses checked through the resource manager */
	rval = ServerPerm(client, SECCLASS_FONT, FONT__LOAD);
	rval2 = IDPERM(client, xPolyTextReq, gc,
		       SECCLASS_GC, GC__SETATTR);
	rval3 = IDPERM(client, xPolyTextReq, drawable,
		       SECCLASS_DRAWABLE, DRAWABLE__DRAW);
	break;

    /* Pixmap class control requirements */
    case X_CreatePixmap:
	rval = IDPERM(client, xCreatePixmapReq, pid,
		      SECCLASS_DRAWABLE, DRAWABLE__CREATE);
	break;
    case X_FreePixmap:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_DRAWABLE, DRAWABLE__DESTROY);
	break;

    /* Cursor class control requirements */
    case X_CreateCursor:
	rval = IDPERM(client, xCreateCursorReq, cid,
		      SECCLASS_CURSOR, CURSOR__CREATE);
	rval2 = IDPERM(client, xCreateCursorReq, source,
		       SECCLASS_DRAWABLE, DRAWABLE__DRAW);
	rval3 = IDPERM(client, xCreateCursorReq, mask,
		       SECCLASS_DRAWABLE, DRAWABLE__COPY);
	break;
    case X_CreateGlyphCursor:
	rval = IDPERM(client, xCreateGlyphCursorReq, cid,
		      SECCLASS_CURSOR, CURSOR__CREATEGLYPH);
	rval2 = IDPERM(client, xCreateGlyphCursorReq, source,
		       SECCLASS_FONT, FONT__USE);
	rval3 = IDPERM(client, xCreateGlyphCursorReq, mask,
		       SECCLASS_FONT, FONT__USE);
	break;
    case X_RecolorCursor:
	rval = IDPERM(client, xRecolorCursorReq, cursor,
		      SECCLASS_CURSOR, CURSOR__SETATTR);
	break;
    case X_FreeCursor:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_CURSOR, CURSOR__FREE);
	break;

    /* GC class control requirements */
    case X_CreateGC:
	rval = IDPERM(client, xCreateGCReq, gc,
		      SECCLASS_GC, GC__CREATE | GC__SETATTR);
	break;
    case X_ChangeGC:
    case X_SetDashes:
    case X_SetClipRectangles:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_GC, GC__SETATTR);
	break;
    case X_CopyGC:
	rval = IDPERM(client, xCopyGCReq, srcGC,
		      SECCLASS_GC, GC__GETATTR);
	rval2 = IDPERM(client, xCopyGCReq, dstGC,
		       SECCLASS_GC, GC__SETATTR);
	break;
    case X_FreeGC:
	rval = IDPERM(client, xResourceReq, id,
		      SECCLASS_GC, GC__FREE);
	break;

    /* Server class control requirements */
    case X_GrabServer:
	rval = ServerPerm(client, SECCLASS_XSERVER, XSERVER__GRAB);
	break;
    case X_UngrabServer:
	rval = ServerPerm(client, SECCLASS_XSERVER, XSERVER__UNGRAB);
	break;
    case X_ForceScreenSaver:
    case X_GetScreenSaver:
    case X_SetScreenSaver:
	rval = ServerPerm(client, SECCLASS_XSERVER, XSERVER__SCREENSAVER);
	break;
    case X_ListHosts:
	rval = ServerPerm(client, SECCLASS_XSERVER, XSERVER__GETHOSTLIST);
	break;
    case X_ChangeHosts:
    case X_SetAccessControl:
	rval = ServerPerm(client, SECCLASS_XSERVER, XSERVER__SETHOSTLIST);
	break;
    case X_GetFontPath:
	rval = ServerPerm(client, SECCLASS_XSERVER, XSERVER__GETFONTPATH);
	break;
    case X_SetFontPath:
	rval = ServerPerm(client, SECCLASS_XSERVER, XSERVER__SETFONTPATH);
	break;
    case X_QueryBestSize:
	rval = ServerPerm(client, SECCLASS_XSERVER, XSERVER__GETATTR);
	break;

    default:
	break;
    }
    if (rval != Success)
	rec->status = rval;
    if (rval2 != Success)
	rec->status = rval2;
    if (rval != Success)
	rec->status = rval3;
}

static void
XSELinuxExtDispatch(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceExtAccessRec *rec = (XaceExtAccessRec*)calldata;
    ClientPtr client = rec->client;
    ExtensionEntry *ext = rec->ext;
    security_id_t extsid;
    access_vector_t perm;
    REQUEST(xReq);

    /* XXX there should be a separate callback for this */
    if (!EXTENSIONSID(ext))
    {
	extsid = GetExtensionSID(ext->name);
	if (!extsid)
	    return;
	EXTENSIONSID(ext) = extsid;
    }

    extsid = (security_id_t)EXTENSIONSID(ext);
    perm = ((stuff->reqType == X_QueryExtension) ||
	    (stuff->reqType == X_ListExtensions)) ?
	XEXTENSION__QUERY : XEXTENSION__USE;

    if (HAVESTATE(client))
    {
	XSELinuxAuditRec auditdata;
	auditdata.client = client;
	auditdata.property = NULL;
	auditdata.extension = ext->name;
	errno = 0;
	if (avc_has_perm(SID(client), extsid, SECCLASS_XEXTENSION,
			 perm, &AEREF(client), &auditdata) < 0)
	{
	    if (errno == EACCES)
		rec->status = BadAccess;
	    ErrorF("ExtDispatch: unexpected error %d\n", errno);
	    rec->status = BadValue;
	}
    } else
	ErrorF("No client state in extension dispatcher!\n");
} /* XSELinuxExtDispatch */

static void
XSELinuxProperty(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XacePropertyAccessRec *rec = (XacePropertyAccessRec*)calldata;
    WindowPtr pWin = rec->pWin;
    ClientPtr client = rec->client;
    ClientPtr tclient;
    access_vector_t perm = 0;
    security_id_t propsid;
    char *propname = NameForAtom(rec->pProp->propertyName);

    tclient = wClient(pWin);
    if (!tclient || !HAVESTATE(tclient))
        return;

    propsid = GetPropertySID(SID(tclient)->ctx, propname);
    if (!propsid)
	return;

    if (rec->access_mode & DixReadAccess)
	perm |= PROPERTY__READ;
    if (rec->access_mode & DixWriteAccess)
	perm |= PROPERTY__WRITE;
    if (rec->access_mode & DixDestroyAccess)
	perm |= PROPERTY__FREE;
    if (!rec->access_mode)
	perm = PROPERTY__READ | PROPERTY__WRITE | PROPERTY__FREE;

    if (HAVESTATE(client))
    {
	XSELinuxAuditRec auditdata;
	auditdata.client = client;
	auditdata.property = propname;
	auditdata.extension = NULL;
	errno = 0;
	if (avc_has_perm(SID(client), propsid, SECCLASS_PROPERTY,
			 perm, &AEREF(client), &auditdata) < 0)
	{
	    if (errno == EACCES)
		rec->status = BadAccess;
	    ErrorF("Property: unexpected error %d\n", errno);
	    rec->status = BadValue;
	}
    } else
	ErrorF("No client state in property callback!\n");

    /* XXX this should be saved in the property structure */
    sidput(propsid);
} /* XSELinuxProperty */

static void
XSELinuxResLookup(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceResourceAccessRec *rec = (XaceResourceAccessRec*)calldata;
    ClientPtr client = rec->client;
    REQUEST(xReq);
    access_vector_t perm = 0;
    int rval = Success;

    /* serverClient requests OK */
    if (client->index == 0)
	return;

    switch(rec->rtype) {
	case RT_FONT: {
	    switch(stuff->reqType) {
		case X_ImageText8:
		case X_ImageText16:
		case X_PolyText8:
		case X_PolyText16:
		    perm = FONT__USE;
		    break;
		case X_ListFonts:
		case X_ListFontsWithInfo:
		case X_QueryFont:
		case X_QueryTextExtents:
		    perm = FONT__GETATTR;
		    break;
		default:
		    break;
	    }
	    if (perm)
		rval = IDPerm(client, rec->id, SECCLASS_FONT, perm);
	    break;
	}
	default:
	    break;
    }
    if (rval != Success)
	rec->status = rval;
} /* XSELinuxResLookup */

static void
XSELinuxMap(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceMapAccessRec *rec = (XaceMapAccessRec*)calldata;
    if (IDPerm(rec->client, rec->pWin->drawable.id,
               SECCLASS_WINDOW, WINDOW__MAP) != Success)
	rec->status = BadAccess;
} /* XSELinuxMap */

static void
XSELinuxDrawable(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceDrawableAccessRec *rec = (XaceDrawableAccessRec*)calldata;
    if (IDPerm(rec->client, rec->pDraw->id,
               SECCLASS_DRAWABLE, DRAWABLE__COPY) != Success)
	rec->status = BadAccess;
} /* XSELinuxDrawable */

static void
XSELinuxServer(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceServerAccessRec *rec = (XaceServerAccessRec*)calldata;
    access_vector_t perm = (rec->access_mode == DixReadAccess) ?
	XSERVER__GETHOSTLIST : XSERVER__SETHOSTLIST;

    if (ServerPerm(rec->client, SECCLASS_XSERVER, perm) != Success)
	rec->status = BadAccess;
} /* XSELinuxServer */

/* Extension callbacks */
static void
XSELinuxClientState(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    NewClientInfoRec *pci = (NewClientInfoRec *)calldata;
    ClientPtr client = pci->client;

    switch(client->clientState)
    {
    case ClientStateInitial:
	AssignServerState();
	break;

	case ClientStateRunning:
	{
	    AssignClientState(client);
	    break;
	}
	case ClientStateGone:
	case ClientStateRetained:
	{
	    FreeClientState(client);
	    break;
	}
	default: break;
    }
} /* XSELinuxClientState */

/* Labeling callbacks */
static void
XSELinuxResourceState(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    ResourceStateInfoRec *rec = (ResourceStateInfoRec *)calldata;
    WindowPtr pWin;
    ClientPtr client;
    security_context_t ctx;
    int rc;

    if (rec->type != RT_WINDOW)
	return;

    pWin = (WindowPtr)rec->value;
    client = wClient(pWin);

    if (HAVESTATE(client)) {
	rc = avc_sid_to_context(SID(client), &ctx);
	if (rc < 0)
	    FatalError("XSELinux: Failed to get security context!\n");
	rc = dixChangeWindowProperty(serverClient,
				     pWin, atom_client_ctx, XA_STRING, 8,
				     PropModeReplace, strlen(ctx), ctx, FALSE);
	freecon(ctx);
    }
    else
	rc = dixChangeWindowProperty(serverClient,
				     pWin, atom_client_ctx, XA_STRING, 8,
				     PropModeReplace, 10, "UNLABELED!", FALSE);
    if (rc != Success)
	FatalError("XSELinux: Failed to set context property on window!\n");
} /* XSELinuxResourceState */

static Bool
XSELinuxLoadConfigFile(void)
{
    struct selinux_opt options[] = {
	{ SELABEL_OPT_PATH, XSELINUXCONFIGFILE },
	{ SELABEL_OPT_VALIDATE, (char *)1 },
    };

    if (!XSELINUXCONFIGFILE)
        return FALSE;

    label_hnd = selabel_open(SELABEL_CTX_X, options, 2);
    return !!label_hnd;
} /* XSELinuxLoadConfigFile */

static void
XSELinuxFreeConfigData(void)
{
    selabel_close(label_hnd);
    label_hnd = NULL;
} /* XSELinuxFreeConfigData */

/* Extension dispatch functions */
static int
ProcXSELinuxDispatch(ClientPtr client)
{
    return BadRequest;
} /* ProcXSELinuxDispatch */

static void
XSELinuxResetProc(ExtensionEntry *extEntry)
{
    FreeClientState(serverClient);

    XSELinuxFreeConfigData();

    audit_close(audit_fd);

    avc_destroy();
} /* XSELinuxResetProc */

static void
XSELinuxAVCAudit(void *auditdata,
		 security_class_t class,
		 char *msgbuf,
		 size_t msgbufsize)
{
    XSELinuxAuditRec *audit = (XSELinuxAuditRec*)auditdata;
    ClientPtr client = audit->client;
    char requestNum[8];
    REQUEST(xReq);

    if (stuff)
	snprintf(requestNum, 8, "%d", stuff->reqType);

    snprintf(msgbuf, msgbufsize, "%s%s%s%s%s%s",
	     stuff ? "request=" : "",
	     stuff ? requestNum : "",
	     audit->property ? " property=" : "",
	     audit->property ? audit->property : "",
	     audit->extension ? " extension=" : "",
	     audit->extension ? audit->extension : "");
}

static void
XSELinuxAVCLog(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    VErrorF(fmt, ap);
    va_end(ap);
}

/* XSELinuxExtensionSetup
 *
 * Set up the XSELinux Extension (pre-init)
 */
void
XSELinuxExtensionSetup(INITARGS)
{
    /* Allocate the client private index */
    clientPrivateIndex = AllocateClientPrivateIndex();
    if (!AllocateClientPrivate(clientPrivateIndex,
			       sizeof (XSELinuxClientStateRec)))
	FatalError("XSELinux: Failed to allocate client private.\n");

    /* Allocate the extension private index */
    extnsnPrivateIndex = AllocateExtensionPrivateIndex();
    if (!AllocateExtensionPrivate(extnsnPrivateIndex, 0))
	FatalError("XSELinux: Failed to allocate extension private.\n");
}

/* XSELinuxExtensionInit
 *
 * Initialize the XSELinux Extension
 */
void
XSELinuxExtensionInit(INITARGS)
{
    ExtensionEntry	*extEntry;
    struct avc_log_callback alc = {XSELinuxAVCLog, XSELinuxAVCAudit};

    if (!is_selinux_enabled())
    {
        ErrorF("XSELinux: Extension failed to load: SELinux not enabled\n");
        return;
    }

    if (selinux_set_mapping(map) < 0) {
	FatalError("XSELinux: Failed to set up security class mapping\n");
    }

    if (avc_init("xserver", NULL, &alc, NULL, NULL) < 0)
    {
	FatalError("XSELinux: Couldn't initialize SELinux userspace AVC\n");
    }

    if (!AddCallback(&ClientStateCallback, XSELinuxClientState, NULL))
	return;
    if (!AddCallback(&ResourceStateCallback, XSELinuxResourceState, NULL))
	return;

    /* Create atoms for doing window labeling */
    atom_ctx = MakeAtom("_SELINUX_CONTEXT", 16, 1);
    if (atom_ctx == BAD_RESOURCE)
	FatalError("XSELinux: Failed to create atom\n");
    atom_client_ctx = MakeAtom("_SELINUX_CLIENT_CONTEXT", 23, 1);
    if (atom_client_ctx == BAD_RESOURCE)
	FatalError("XSELinux: Failed to create atom\n");

    /* Load the config file.  If this fails, shut down the server,
     * since an unknown security status is worse than no security.
     */
    if (XSELinuxLoadConfigFile() != TRUE)
    {
	FatalError("XSELinux: Failed to load security policy\n");
    }

    /* prepare for auditing */
    audit_fd = audit_open();
    if (audit_fd < 0)
    {
        FatalError("XSELinux: Failed to open the system audit log\n");
    }

    /* register security callbacks */
    XaceRegisterCallback(XACE_CORE_DISPATCH, XSELinuxCoreDispatch, NULL);
    XaceRegisterCallback(XACE_EXT_ACCESS, XSELinuxExtDispatch, NULL);
    XaceRegisterCallback(XACE_EXT_DISPATCH, XSELinuxExtDispatch, NULL);
    XaceRegisterCallback(XACE_RESOURCE_ACCESS, XSELinuxResLookup, NULL);
    XaceRegisterCallback(XACE_MAP_ACCESS, XSELinuxMap, NULL);
    XaceRegisterCallback(XACE_SERVER_ACCESS, XSELinuxServer, NULL);
    XaceRegisterCallback(XACE_PROPERTY_ACCESS, XSELinuxProperty, NULL);
    /* XaceRegisterCallback(XACE_DECLARE_EXT_SECURE, XSELinuxDeclare, NULL);
    XaceRegisterCallback(XACE_DEVICE_ACCESS, XSELinuxDevice, NULL); */

    /* register extension with server */
    extEntry = AddExtension(XSELINUX_EXTENSION_NAME,
			    XSELinuxNumberEvents, XSELinuxNumberErrors,
			    ProcXSELinuxDispatch, ProcXSELinuxDispatch,
			    XSELinuxResetProc, StandardMinorOpcode);
}
