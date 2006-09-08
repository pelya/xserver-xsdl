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

#include <selinux/flask.h>
#include <selinux/av_permissions.h>
#include <selinux/selinux.h>
#include <selinux/context.h>
#include <selinux/avc.h>

#include <libaudit.h>

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
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


/* Make sure a locally connecting client has a valid context.  The context
 * for this client is retrieved again later on in AssignClientState(), but
 * by that point it's too late to reject the client.
 */
static char *
XSELinuxValidContext (ClientPtr client)
{
    security_context_t ctx = NULL;
    XtransConnInfo ci = ((OsCommPtr)client->osPrivate)->trans_conn;
    char reason[256];
    char *ret = (char *)NULL;

    if (_XSERVTransIsLocal(ci))
    {
        int fd = _XSERVTransGetConnectionNumber(ci);
        if (getpeercon(fd, &ctx) < 0)
        {
            snprintf(reason, sizeof(reason), "Failed to retrieve SELinux context from socket");
            ret = reason;
            goto out;
        }
        if (security_check_context(ctx))
        {
            snprintf(reason, sizeof(reason), "Client's SELinux context is invalid: %s", ctx);
            ret = reason;
        }

        freecon(ctx);
    }

out:
    return ret;
}


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

/*
 * Table of SELinux types for property names.
 */
static char **propertyTypes = NULL;
static int propertyTypesCount = 0;
char *XSELinuxPropertyTypeDefault = NULL;

/*
 * Table of SELinux types for each extension.
 */
static char **extensionTypes = NULL;
static int extensionTypesCount = 0;
static char *XSELinuxExtensionTypeDefault = NULL;

/* security context for non-local clients */
static char *XSELinuxNonlocalContextDefault = NULL;

/* security context for the root window */
static char *XSELinuxRootWindowContext = NULL;

/* Selection stuff from dix */
extern Selection *CurrentSelections;
extern int NumCurrentSelections;

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
    SECCLASS_XINPUT
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
 * Returns: boolean TRUE=allowed, FALSE=denied.
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
            if (errno != EACCES)
                ErrorF("ServerPerm: unexpected error %d\n", errno);
            return FALSE;
        }
    }
    else
    {
	ErrorF("No client state in server-perm check!\n");
        return TRUE;
    }

    return TRUE;
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
 * Returns: boolean TRUE=allowed, FALSE=denied.
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
	return TRUE;

    CheckXID(id);
    tclient = clients[CLIENT_ID(id)];

    /*
     * This happens in the case where a client has
     * disconnected.  XXX might want to make the server
     * own orphaned resources...
     */
    if (!tclient || !HAVESTATE(tclient) || !HAVESTATE(sclient))
    {
	return TRUE;
    }

    auditdata.client = sclient;
    auditdata.property = NULL;
    auditdata.extension = NULL;
    errno = 0;
    if (avc_has_perm(SID(sclient), RSID(tclient,idx), class,
		     perm, &AEREF(sclient), &auditdata) < 0)
    {
	if (errno != EACCES)
	    ErrorF("IDPerm: unexpected error %d\n", errno);
	return FALSE;
    }

    return TRUE;
}

/*
 * ObjectSIDByLabel - get SID for an extension or property.
 *
 * Arguments:
 * class: should be SECCLASS_XEXTENSION or SECCLASS_PROPERTY.
 * name: name of the extension or property.
 *
 * Returns: proper SID for the object or NULL on error.
 */
static security_id_t
ObjectSIDByLabel(security_context_t basecontext, security_class_t class,
                 const char *name)
{
    security_context_t base, new;
    context_t con;
    security_id_t sid = NULL;
    char **ptr, *type = NULL;

    if (basecontext != NULL)
    {
        /* use the supplied context */
        base = strdup(basecontext);
        if (base == NULL)
            goto out;
    }
    else
    {
        /* get server context */
        if (getcon(&base) < 0)
            goto out;
    }

    /* make a new context-manipulation object */
    con = context_new(base);
    if (!con)
	goto out2;
    
    /* look in the mappings of names to types */
    ptr = (class == SECCLASS_PROPERTY) ? propertyTypes : extensionTypes;
    for (; *ptr; ptr+=2)
	if (!strcmp(*ptr, name))
	    break;
    type = ptr[1];

    /* set the role and type in the context (user unchanged) */
    if (context_type_set(con, type) ||
	context_role_set(con, "object_r"))
	goto out3;

    /* get a context string from the context-manipulation object */
    new = context_str(con);
    if (!new)
	goto out3;

    /* get a SID for the context */
    if (avc_context_to_sid(new, &sid) < 0)
	goto out3;

  out3:
    context_free(con);
  out2:
    freecon(base);
  out:
    return sid;
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
    int i, needToFree = 0;
    security_context_t basectx, objctx;
    XSELinuxClientStateRec *state = (XSELinuxClientStateRec*)STATEPTR(client);
    Bool isServerClient = FALSE;

    avc_entry_ref_init(&state->aeref);

    if (client->index > 0)
    {
	XtransConnInfo ci = ((OsCommPtr)client->osPrivate)->trans_conn;
	if (_XSERVTransIsLocal(ci)) {
	    /* for local clients, can get context from the socket */
	    int fd = _XSERVTransGetConnectionNumber(ci);
	    if (getpeercon(fd, &basectx) < 0)
	    {
		FatalError("Client %d: couldn't get context from socket\n",
			   client->index);
	    }
	    needToFree = 1;
	}
        else
        {
	    /* for remote clients, need to use a default context */
	    basectx = XSELinuxNonlocalContextDefault;
	}
    }
    else
    {
        isServerClient = TRUE;

	/* use the context of the X server process for the serverClient */
	if (getcon(&basectx) < 0)
	{
	    FatalError("Couldn't get context of X server process\n");
	}
	needToFree = 1;
    }

    /* get a SID from the context */
    if (avc_context_to_sid(basectx, &state->sid) < 0)
    {
	FatalError("Client %d: couldn't get security ID for client\n",
		   client->index);
    }

    /* get contexts and then SIDs for each resource type */
    for (i=0; i<NRES; i++)
    {
	if (security_compute_create(basectx, basectx, sClasses[i],
				    &objctx) < 0)
	{
	    FatalError("Client %d: couldn't get context for class %x\n",
		       client->index, sClasses[i]);
	}
	else if (avc_context_to_sid(objctx, &state->rsid[i]) < 0)
	{
	    FatalError("Client %d: couldn't get SID for class %x\n",
		       client->index, sClasses[i]);
	}
	freecon(objctx);
    }

    /* special handling for serverClient windows (that is, root windows) */
    if (isServerClient == TRUE)
    {
        i = IndexByClass(SECCLASS_WINDOW);
        sidput(state->rsid[i]);
        if (avc_context_to_sid(XSELinuxRootWindowContext, &state->rsid[i]))
        {
            FatalError("Failed to set SID for root window\n");
        }
    }

    /* mark as set up, free base context if necessary, and return */
    state->haveState = TRUE;
    if (needToFree)
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
    (REQUEST_SIZE_CHECK(client,req) && \
    IDPerm(client, SwapXID(client,((req*)stuff)->field), class, perm))
#define CALLBACK(name) static void \
name(CallbackListPtr *pcbl, pointer nulldata, pointer calldata)

static int
CheckSendEventPerms(ClientPtr client)
{
    register char n;
    access_vector_t perm = 0;
    REQUEST(xSendEventReq);

    /* might need type bounds checking here */
    if (!REQUEST_SIZE_CHECK(client, xSendEventReq))
	return FALSE;

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
    int rval = TRUE;
    REQUEST(xConvertSelectionReq);
    
    if (!REQUEST_SIZE_CHECK(client, xConvertSelectionReq))
	return FALSE;

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
	if (i < NumCurrentSelections)
	    rval = rval && IDPerm(client, CurrentSelections[i].window,
				  SECCLASS_WINDOW, WINDOW__CLIENTCOMEVENT);
    }
    rval = rval && IDPerm(client, stuff->requestor,
			  SECCLASS_WINDOW, WINDOW__CLIENTCOMEVENT);
    return rval;
}

static int
CheckSetSelectionOwnerPerms(ClientPtr client)
{
    register char n;
    int rval = TRUE;
    REQUEST(xSetSelectionOwnerReq);

    if (!REQUEST_SIZE_CHECK(client, xSetSelectionOwnerReq))
	return FALSE;

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
	if (i < NumCurrentSelections)
	    rval = rval && IDPerm(client, CurrentSelections[i].window,
				  SECCLASS_WINDOW, WINDOW__CHSELECTION);
    }
    rval = rval && IDPerm(client, stuff->window, 
			  SECCLASS_WINDOW, WINDOW__CHSELECTION);
    return rval;
}
    
CALLBACK(XSELinuxCoreDispatch)
{
    XaceCoreDispatchRec *rec = (XaceCoreDispatchRec*)calldata;
    ClientPtr client = rec->client;
    REQUEST(xReq);
    Bool rval;

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
		      SECCLASS_DRAWABLE, DRAWABLE__COPY)
	    && IDPERM(client, xCopyAreaReq, dstDrawable,
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
		      WINDOW__CREATE | WINDOW__SETATTR | WINDOW__MOVE)
	    && IDPERM(client, xCreateWindowReq, parent,
		      SECCLASS_WINDOW,
		      WINDOW__CHSTACK | WINDOW__ADDCHILD)
	    && IDPERM(client, xCreateWindowReq, wid,
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
		      WINDOW__ENUMERATE | WINDOW__UNMAP | WINDOW__DESTROY)
	    && IDPERM(client, xResourceReq, id,
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
		      SECCLASS_WINDOW, WINDOW__CHPARENT | WINDOW__MOVE)
	    && IDPERM(client, xReparentWindowReq, parent,
		      SECCLASS_WINDOW, WINDOW__CHSTACK | WINDOW__ADDCHILD);
	break;
    case X_SendEvent:
	rval = CheckSendEventPerms(client);
	break;
    case X_SetInputFocus:
	rval = IDPERM(client, xSetInputFocusReq, focus,
		      SECCLASS_WINDOW, WINDOW__SETFOCUS)
	    && ServerPerm(client, SECCLASS_XINPUT, XINPUT__SETFOCUS);
	break;
    case X_SetSelectionOwner:
	rval = CheckSetSelectionOwnerPerms(client);
	break;
    case X_TranslateCoords:
	rval = IDPERM(client, xTranslateCoordsReq, srcWid,
		      SECCLASS_WINDOW, WINDOW__GETATTR)
	    && IDPERM(client, xTranslateCoordsReq, dstWid,
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
		      SECCLASS_WINDOW, WINDOW__GETATTR)
	    && IDPERM(client, xWarpPointerReq, dstWid,
		      SECCLASS_WINDOW, WINDOW__GETATTR)
	    && ServerPerm(client, SECCLASS_XINPUT, XINPUT__WARPPOINTER);
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
		      SECCLASS_COLORMAP, COLORMAP__CREATE)
	    && IDPERM(client, xCopyColormapAndFreeReq, srcCmap,
		      SECCLASS_COLORMAP,
		      COLORMAP__READ | COLORMAP__FREE);
	break;
    case X_CreateColormap:
	rval = IDPERM(client, xCreateColormapReq, mid,
		      SECCLASS_COLORMAP, COLORMAP__CREATE)
	    && IDPERM(client, xCreateColormapReq, window,
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
		      SECCLASS_COLORMAP, COLORMAP__INSTALL)
	    && ServerPerm(client, SECCLASS_COLORMAP, COLORMAP__INSTALL);
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
		      SECCLASS_COLORMAP, COLORMAP__UNINSTALL)
	    && ServerPerm(client, SECCLASS_COLORMAP, COLORMAP__UNINSTALL);
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
	rval = ServerPerm(client, SECCLASS_FONT, FONT__LOAD)
	    && IDPERM(client, xOpenFontReq, fid,
		      SECCLASS_FONT, FONT__USE);
	break;
    case X_PolyText8:
    case X_PolyText16:
	/* Font accesses checked through the resource manager */
	rval = ServerPerm(client, SECCLASS_FONT, FONT__LOAD)
	    && IDPERM(client, xPolyTextReq, gc,
		      SECCLASS_GC, GC__SETATTR)
	    && IDPERM(client, xPolyTextReq, drawable,
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
		      SECCLASS_CURSOR, CURSOR__CREATE)
	    && IDPERM(client, xCreateCursorReq, source,
		      SECCLASS_DRAWABLE, DRAWABLE__DRAW)
	    && IDPERM(client, xCreateCursorReq, mask,
		      SECCLASS_DRAWABLE, DRAWABLE__COPY);
	break;
    case X_CreateGlyphCursor:
	rval = IDPERM(client, xCreateGlyphCursorReq, cid,
		      SECCLASS_CURSOR, CURSOR__CREATEGLYPH)
	    && IDPERM(client, xCreateGlyphCursorReq, source,
		      SECCLASS_FONT, FONT__USE)
	    && IDPERM(client, xCreateGlyphCursorReq, mask,
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
		      SECCLASS_GC, GC__GETATTR)
	    && IDPERM(client, xCopyGCReq, dstGC,
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
	rval = TRUE;
	break;
    }
    if (!rval)
	rec->rval = FALSE;
}

CALLBACK(XSELinuxExtDispatch)
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
	extsid = ObjectSIDByLabel(NULL, SECCLASS_XEXTENSION, ext->name);
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
	    if (errno != EACCES)
		ErrorF("ExtDispatch: unexpected error %d\n", errno);
	    rec->rval = FALSE;
	}
    } else
	ErrorF("No client state in extension dispatcher!\n");
} /* XSELinuxExtDispatch */

CALLBACK(XSELinuxProperty)
{
    XacePropertyAccessRec *rec = (XacePropertyAccessRec*)calldata;
    WindowPtr pWin = rec->pWin;
    ClientPtr client = rec->client;
    ClientPtr tclient;
    access_vector_t perm = 0;
    security_id_t propsid;
    char *propname = NameForAtom(rec->propertyName);

    tclient = wClient(pWin);
    if (!tclient || !HAVESTATE(tclient))
        return;

    propsid = ObjectSIDByLabel(SID(tclient)->ctx, SECCLASS_PROPERTY, propname);
    if (!propsid)
	return;

    if (rec->access_mode & SecurityReadAccess)
	perm |= PROPERTY__READ;
    if (rec->access_mode & SecurityWriteAccess)
	perm |= PROPERTY__WRITE;
    if (rec->access_mode & SecurityDestroyAccess)
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
	    if (errno != EACCES)
		ErrorF("Property: unexpected error %d\n", errno);
	    rec->rval = SecurityIgnoreOperation;
	}
    } else
	ErrorF("No client state in property callback!\n");

    /* XXX this should be saved in the property structure */
    sidput(propsid);
} /* XSELinuxProperty */

CALLBACK(XSELinuxResLookup)
{
    XaceResourceAccessRec *rec = (XaceResourceAccessRec*)calldata;
    ClientPtr client = rec->client;
    REQUEST(xReq);
    access_vector_t perm = 0;
    Bool rval = TRUE;

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
    if (!rval)
	rec->rval = FALSE;
} /* XSELinuxResLookup */

CALLBACK(XSELinuxMap)
{
    XaceMapAccessRec *rec = (XaceMapAccessRec*)calldata;
    if (!IDPerm(rec->client, rec->pWin->drawable.id,
		SECCLASS_WINDOW, WINDOW__MAP))
	rec->rval = FALSE;
} /* XSELinuxMap */

CALLBACK(XSELinuxBackgrnd)
{
    XaceMapAccessRec *rec = (XaceMapAccessRec*)calldata;
    if (!IDPerm(rec->client, rec->pWin->drawable.id,
		SECCLASS_WINDOW, WINDOW__TRANSPARENT))
	rec->rval = FALSE;
} /* XSELinuxBackgrnd */

CALLBACK(XSELinuxDrawable)
{
    XaceDrawableAccessRec *rec = (XaceDrawableAccessRec*)calldata;
    if (!IDPerm(rec->client, rec->pDraw->id,
		SECCLASS_DRAWABLE, DRAWABLE__COPY))
	rec->rval = FALSE;
} /* XSELinuxDrawable */

CALLBACK(XSELinuxHostlist)
{
    XaceHostlistAccessRec *rec = (XaceHostlistAccessRec*)calldata;
    access_vector_t perm = (rec->access_mode == SecurityReadAccess) ?
	XSERVER__GETHOSTLIST : XSERVER__SETHOSTLIST;

    if (!ServerPerm(rec->client, SECCLASS_XSERVER, perm))
	rec->rval = FALSE;
} /* XSELinuxHostlist */

/* Extension callbacks */
CALLBACK(XSELinuxClientState)
{
    NewClientInfoRec *pci = (NewClientInfoRec *)calldata;
    ClientPtr client = pci->client;

    switch(client->clientState)
    {
    case ClientStateInitial:
	AssignClientState(serverClient);
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

static char *XSELinuxKeywords[] = {
#define XSELinuxKeywordComment 0
    "#",
#define XSELinuxKeywordProperty 1
    "property",
#define XSELinuxKeywordExtension 2
    "extension",
#define XSELinuxKeywordNonlocalContext 3
    "nonlocal_context",
#define XSELinuxKeywordRootWindowContext 4
    "root_window_context",
#define XSELinuxKeywordDefault 5
    "default"
};

#define NUMKEYWORDS (sizeof(XSELinuxKeywords) / sizeof(char *))

#ifndef __UNIXOS2__
#define XSELinuxIsWhitespace(c) ( (c == ' ') || (c == '\t') || (c == '\n') )
#else
#define XSELinuxIsWhitespace(c) ( (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') )
#endif

static char *
XSELinuxSkipWhitespace(
    char *p)
{
    while (XSELinuxIsWhitespace(*p))
	p++;
    return p;
} /* XSELinuxSkipWhitespace */

static char *
XSELinuxParseString(
    char **rest)
{
    char *startOfString;
    char *s = *rest;
    char endChar = 0;

    s = XSELinuxSkipWhitespace(s);

    if (*s == '"' || *s == '\'')
    {
	endChar = *s++;
	startOfString = s;
	while (*s && (*s != endChar))
	    s++;
    }
    else
    {
	startOfString = s;
	while (*s && !XSELinuxIsWhitespace(*s))
	    s++;
    }
    if (*s)
    {
	*s = '\0';
	*rest = s + 1;
	return startOfString;
    }
    else
    {
	*rest = s;
	return (endChar) ? NULL : startOfString;
    }
} /* XSELinuxParseString */

static int
XSELinuxParseKeyword(
    char **p)
{
    int i;
    char *s = *p;
    s = XSELinuxSkipWhitespace(s);
    for (i = 0; i < NUMKEYWORDS; i++)
    {
	int len = strlen(XSELinuxKeywords[i]);
	if (strncmp(s, XSELinuxKeywords[i], len) == 0)
	{
	    *p = s + len;
	    return (i);
	}
    }
    *p = s;
    return -1;
} /* XSELinuxParseKeyword */

static Bool
XSELinuxTypeIsValid(char *typename)
{
    security_context_t base, new;
    context_t con;
    Bool ret = FALSE;

    /* get the server's context */
    if (getcon(&base) < 0)
        goto out;

    /* make a new context-manipulation object */
    con = context_new(base);
    if (!con)
        goto out_free;

    /* set the role */
    if (context_role_set(con, "object_r"))
        goto out_free2;

    /* set the type */
    if (context_type_set(con, typename))
        goto out_free2;

    /* get a context string - note: context_str() returns a pointer
     * to the string inside the context; the returned pointer should
     * not be freed
     */
    new = context_str(con);
    if (!new)
        goto out_free2;

    /* finally, check to see if it's valid */
    if (security_check_context(new) == 0)
        ret = TRUE;

out_free2:
    context_free(con);
out_free:
    freecon(base);
out:
    return ret;
}

static Bool
XSELinuxParsePropertyTypeRule(char *p)
{
    int keyword;
    char *propname = NULL, *propcopy = NULL;
    char *typename = NULL, *typecopy = NULL;
    char **newTypes;
    Bool defaultPropertyType = FALSE;

    /* get property name */
    keyword = XSELinuxParseKeyword(&p);
    if (keyword == XSELinuxKeywordDefault)
    {
        defaultPropertyType = TRUE;
    }
    else
    {
        propname = XSELinuxParseString(&p);
        if (!propname || (strlen(propname) == 0))
        {
            return FALSE;
        }
    }

    /* get the SELinux type corresponding to the property */
    typename = XSELinuxParseString(&p);
    if (!typename || (strlen(typename) == 0))
        return FALSE;

    /* validate the type */
    if (XSELinuxTypeIsValid(typename) != TRUE)
        return FALSE;

    /* if it's the default property, save it to append to the end of the
     * property types list
     */
    if (defaultPropertyType == TRUE)
    {
        if (XSELinuxPropertyTypeDefault != NULL)
        {
            return FALSE;
        }
        else
        {
            XSELinuxPropertyTypeDefault = (char *)xalloc(strlen(typename)+1);
            if (!XSELinuxPropertyTypeDefault)
            {
                ErrorF("XSELinux: out of memory\n");
                return FALSE;
            }
            strcpy(XSELinuxPropertyTypeDefault, typename);
            return TRUE;
        }
    }

    /* insert the property and type into the propertyTypes array */
    propcopy = (char *)xalloc(strlen(propname)+1);
    if (!propcopy)
    {
        ErrorF("XSELinux: out of memory\n");
        return FALSE;
    }
    strcpy(propcopy, propname);

    typecopy = (char *)xalloc(strlen(typename)+1);
    if (!typecopy)
    {
        ErrorF("XSELinux: out of memory\n");
        xfree(propcopy);
        return FALSE;
    }
    strcpy(typecopy, typename);

    newTypes = (char **)xrealloc(propertyTypes, sizeof (char *) * ((propertyTypesCount+1) * 2));
    if (!newTypes)
    {
        ErrorF("XSELinux: out of memory\n");
        xfree(propcopy);
        xfree(typecopy);
        return FALSE;
    }

    propertyTypesCount++;

    newTypes[propertyTypesCount*2 - 2] = propcopy;
    newTypes[propertyTypesCount*2 - 1] = typecopy;
    
    propertyTypes = newTypes;

    return TRUE;
} /* XSELinuxParsePropertyTypeRule */

static Bool
XSELinuxParseExtensionTypeRule(char *p)
{
    int keyword;
    char *extname = NULL, *extcopy = NULL;
    char *typename = NULL, *typecopy = NULL;
    char **newTypes;
    Bool defaultExtensionType = FALSE;

    /* get extension name */
    keyword = XSELinuxParseKeyword(&p);
    if (keyword == XSELinuxKeywordDefault)
    {
        defaultExtensionType = TRUE;
    }
    else
    {
        extname = XSELinuxParseString(&p);
        if (!extname || (strlen(extname) == 0))
        {
            return FALSE;
        }
    }

    /* get the SELinux type corresponding to the extension */
    typename = XSELinuxParseString(&p);
    if (!typename || (strlen(typename) == 0))
        return FALSE;

    /* validate the type */
    if (XSELinuxTypeIsValid(typename) != TRUE)
        return FALSE;

    /* if it's the default extension, save it to append to the end of the
     * extension types list
     */
    if (defaultExtensionType == TRUE)
    {
        if (XSELinuxExtensionTypeDefault != NULL)
        {
            return FALSE;
        }
        else
        {
            XSELinuxExtensionTypeDefault = (char *)xalloc(strlen(typename)+1);
            if (!XSELinuxExtensionTypeDefault)
            {
                ErrorF("XSELinux: out of memory\n");
                return FALSE;
            }
            strcpy(XSELinuxExtensionTypeDefault, typename);
            return TRUE;
        }
    }

    /* insert the extension and type into the extensionTypes array */
    extcopy = (char *)xalloc(strlen(extname)+1);
    if (!extcopy)
    {
        ErrorF("XSELinux: out of memory\n");
        return FALSE;
    }
    strcpy(extcopy, extname);

    typecopy = (char *)xalloc(strlen(typename)+1);
    if (!typecopy)
    {
        ErrorF("XSELinux: out of memory\n");
        xfree(extcopy);
        return FALSE;
    }
    strcpy(typecopy, typename);

    newTypes = (char **)xrealloc(extensionTypes, sizeof(char *) *( (extensionTypesCount+1) * 2));
    if (!newTypes)
    {
        ErrorF("XSELinux: out of memory\n");
        xfree(extcopy);
        xfree(typecopy);
        return FALSE;
    }

    extensionTypesCount++;

    newTypes[extensionTypesCount*2 - 2] = extcopy;
    newTypes[extensionTypesCount*2 - 1] = typecopy;

    extensionTypes = newTypes;

    return TRUE;
} /* XSELinuxParseExtensionTypeRule */

static Bool
XSELinuxParseNonlocalContext(char *p)
{
    char *context;

    context = XSELinuxParseString(&p);
    if (!context || (strlen(context) == 0))
    {
        return FALSE;
    }

    if (XSELinuxNonlocalContextDefault != NULL)
    {
        return FALSE;
    }

    /* validate the context */
    if (security_check_context(context))
    {
        return FALSE;
    }

    XSELinuxNonlocalContextDefault = (char *)xalloc(strlen(context)+1);
    if (!XSELinuxNonlocalContextDefault)
    {
        ErrorF("XSELinux: out of memory\n");
        return FALSE;
    }
    strcpy(XSELinuxNonlocalContextDefault, context);

    return TRUE;
} /* XSELinuxParseNonlocalContext */

static Bool
XSELinuxParseRootWindowContext(char *p)
{
    char *context;

    context = XSELinuxParseString(&p);
    if (!context || (strlen(context) == 0))
    {
        return FALSE;
    }

    if (XSELinuxRootWindowContext != NULL)
    {
        return FALSE;
    }

    /* validate the context */
    if (security_check_context(context))
    {
        return FALSE;
    }

    XSELinuxRootWindowContext = (char *)xalloc(strlen(context)+1);
    if (!XSELinuxRootWindowContext)
    {
        ErrorF("XSELinux: out of memory\n");
        return FALSE;
    }
    strcpy(XSELinuxRootWindowContext, context);

    return TRUE;
} /* XSELinuxParseRootWindowContext */

static Bool
XSELinuxLoadConfigFile(void)
{
    FILE *f;
    int lineNumber = 0;
    char **newTypes;
    Bool ret = FALSE;
    
    if (!XSELINUXCONFIGFILE)
        return FALSE;

    /* some initial bookkeeping */
    propertyTypesCount = extensionTypesCount = 0;
    propertyTypes = extensionTypes = NULL;
    XSELinuxPropertyTypeDefault = XSELinuxExtensionTypeDefault = NULL;
    XSELinuxNonlocalContextDefault = NULL;
    XSELinuxRootWindowContext = NULL;

#ifndef __UNIXOS2__
    f = fopen(XSELINUXCONFIGFILE, "r");
#else
    f = fopen((char*)__XOS2RedirRoot(XSELINUXCONFIGFILE), "r");
#endif
    if (!f)
    {
        ErrorF("Error opening XSELinux policy file %s\n", XSELINUXCONFIGFILE);
        return FALSE;
    }

    while (!feof(f))
    {
        char buf[200];
        Bool validLine;
        char *p;

        if (!(p = fgets(buf, sizeof(buf), f)))
            break;
        lineNumber++;

        switch (XSELinuxParseKeyword(&p))
        {
            case XSELinuxKeywordComment:
                validLine = TRUE;
                break;

            case XSELinuxKeywordProperty:
                validLine = XSELinuxParsePropertyTypeRule(p);
                break;

            case XSELinuxKeywordExtension:
                validLine = XSELinuxParseExtensionTypeRule(p);
                break;

            case XSELinuxKeywordNonlocalContext:
                validLine = XSELinuxParseNonlocalContext(p);
                break;

            case XSELinuxKeywordRootWindowContext:
                validLine = XSELinuxParseRootWindowContext(p);
                break;

            default:
                validLine = (*p == '\0');
                break;
        }

        if (!validLine)
        {
            ErrorF("XSELinux: Line %d of %s is invalid\n",
                   lineNumber, XSELINUXCONFIGFILE);
            goto out;
        }
    }

    /* check to make sure the default types and the nonlocal context
     * were specified
     */
    if (XSELinuxPropertyTypeDefault == NULL)
    {
        ErrorF("XSELinux: No default property type specified\n");
        goto out;
    }
    else if (XSELinuxExtensionTypeDefault == NULL)
    {
        ErrorF("XSELinux: No default extension type specified\n");
        goto out;
    }
    else if (XSELinuxNonlocalContextDefault == NULL)
    {
        ErrorF("XSELinux: No default context for non-local clients specified\n");
        goto out;
    }
    else if (XSELinuxRootWindowContext == NULL)
    {
        ErrorF("XSELinux: No context specified for the root window\n");
        goto out;
    }

    /* Finally, append the default property and extension types to the
     * bottoms of the propertyTypes and extensionTypes arrays, respectively.
     * The 'name' of the property / extension is NULL.
     */
    newTypes = (char **)xrealloc(propertyTypes, sizeof(char *) *((propertyTypesCount+1) * 2));
    if (!newTypes)
    {
        ErrorF("XSELinux: out of memory\n");
        goto out;
    }
    propertyTypesCount++;
    newTypes[propertyTypesCount*2 - 2] = NULL;
    newTypes[propertyTypesCount*2 - 1] = XSELinuxPropertyTypeDefault;
    propertyTypes = newTypes;

    newTypes = (char **)xrealloc(extensionTypes, sizeof(char *) *((extensionTypesCount+1) * 2));
    if (!newTypes)
    {
        ErrorF("XSELinux: out of memory\n");
        goto out;
    }
    extensionTypesCount++;
    newTypes[extensionTypesCount*2 - 2] = NULL;
    newTypes[extensionTypesCount*2 - 1] = XSELinuxExtensionTypeDefault;
    extensionTypes = newTypes;

    ret = TRUE;

out:
    fclose(f);
    return ret;
} /* XSELinuxLoadConfigFile */

static void
XSELinuxFreeConfigData(void)
{
    char **ptr;

    /* Free all the memory in the table until we reach the NULL, then
     * skip one past the NULL and free the default type.  Then take care
     * of some bookkeeping.
     */
    for (ptr = propertyTypes; *ptr; ptr++)
        xfree(*ptr);
    ptr++;
    xfree(*ptr);

    XSELinuxPropertyTypeDefault = NULL;
    propertyTypesCount = 0;

    xfree(propertyTypes);
    propertyTypes = NULL;

    /* ... and the same for the extension type table */
    for (ptr = extensionTypes; *ptr; ptr++)
        xfree(*ptr);
    ptr++;
    xfree(*ptr);

    XSELinuxExtensionTypeDefault = NULL;
    extensionTypesCount = 0;

    xfree(extensionTypes);
    extensionTypes = NULL;

    /* finally, take care of the context for non-local connections */
    xfree(XSELinuxNonlocalContextDefault);
    XSELinuxNonlocalContextDefault = NULL;

    /* ... and for the root window */
    xfree(XSELinuxRootWindowContext);
    XSELinuxRootWindowContext = NULL;
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
        ErrorF("SELinux Extension failed to load: SELinux not enabled\n");
        return;
    }

    if (avc_init("xserver", NULL, &alc, NULL, NULL) < 0)
    {
	FatalError("couldn't initialize SELinux userspace AVC\n");
    }

    if (!AddCallback(&ClientStateCallback, XSELinuxClientState, NULL))
	return;

    /* Load the config file.  If this fails, shut down the server,
     * since an unknown security status is worse than no security.
     *
     * Note that this must come before we assign a security state
     * for the serverClient, because the serverClient's root windows
     * are assigned a context based on data in the config file.
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
    XaceRegisterCallback(XACE_HOSTLIST_ACCESS, XSELinuxHostlist, NULL);
    XaceRegisterCallback(XACE_BACKGRND_ACCESS, XSELinuxBackgrnd, NULL);
    XaceRegisterCallback(XACE_DRAWABLE_ACCESS, XSELinuxDrawable, NULL);
    XaceRegisterCallback(XACE_PROPERTY_ACCESS, XSELinuxProperty, NULL);
    /* XaceRegisterCallback(XACE_DECLARE_EXT_SECURE, XSELinuxDeclare, NULL);
    XaceRegisterCallback(XACE_DEVICE_ACCESS, XSELinuxDevice, NULL); */

    /* register extension with server */
    extEntry = AddExtension(XSELINUX_EXTENSION_NAME,
			    XSELinuxNumberEvents, XSELinuxNumberErrors,
			    ProcXSELinuxDispatch, ProcXSELinuxDispatch,
			    XSELinuxResetProc, StandardMinorOpcode);
}
