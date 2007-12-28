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

#include <X11/Xatom.h>
#include "resource.h"
#include "privates.h"
#include "registry.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "scrnintstr.h"
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


/*
 * Globals
 */

/* private state record */
static DevPrivateKey stateKey = &stateKey;

/* This is what we store for security state */
typedef struct {
    security_id_t sid;
    struct avc_entry_ref aeref;
    char *command;
} SELinuxStateRec;

/* selection manager */
typedef struct {
    Atom selection;
    security_id_t sid;
} SELinuxSelectionRec;

static ClientPtr selectionManager;
static Window selectionWindow;

/* audit file descriptor */
static int audit_fd;

/* structure passed to auditing callback */
typedef struct {
    ClientPtr client;	/* client */
    char *command;	/* client's executable path */
    unsigned id;	/* resource id, if any */
    int restype;	/* resource type, if any */
    int event;		/* event type, if any */
    Atom property;	/* property name, if any */
    Atom selection;	/* selection name, if any */
    char *extension;	/* extension name, if any */
} SELinuxAuditRec;

/* labeling handle */
static struct selabel_handle *label_hnd;

/* whether AVC is active */
static int avc_active;

/* atoms for window label properties */
static Atom atom_ctx;
static Atom atom_client_ctx;

/* The unlabeled SID */
static security_id_t unlabeled_sid;

/* Array of object classes indexed by resource type */
static security_class_t *knownTypes;
static unsigned numKnownTypes;

/* Array of event SIDs indexed by event type */
static security_id_t *knownEvents;
static unsigned numKnownEvents;

/* Array of selection SID structures */
static SELinuxSelectionRec *knownSelections;
static unsigned numKnownSelections;

/* dynamically allocated security classes and permissions */
static struct security_class_mapping map[] = {
    { "x_drawable", { "read", "write", "destroy", "create", "getattr", "setattr", "list_property", "get_property", "set_property", "", "", "list_child", "add_child", "remove_child", "hide", "show", "blend", "override", "", "", "", "", "send", "receive", "", "manage", NULL }},
    { "x_screen", { "", "", "", "", "getattr", "setattr", "saver_getattr", "saver_setattr", "", "", "", "", "", "", "hide_cursor", "show_cursor", "saver_hide", "saver_show", NULL }},
    { "x_gc", { "", "", "destroy", "create", "getattr", "setattr", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "use", NULL }},
    { "x_font", { "", "", "destroy", "create", "getattr", "", "", "", "", "", "", "", "add_glyph", "remove_glyph", "", "", "", "", "", "", "", "", "", "", "use", NULL }},
    { "x_colormap", { "read", "write", "destroy", "create", "getattr", "", "", "", "", "", "", "", "add_color", "remove_color", "", "", "", "", "", "", "install", "uninstall", "", "", "use", NULL }},
    { "x_property", { "read", "write", "destroy", "create", NULL }},
    { "x_selection", { "read", "", "", "", "getattr", "setattr", NULL }},
    { "x_cursor", { "read", "write", "destroy", "create", "getattr", "setattr", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "use", NULL }},
    { "x_client", { "", "", "destroy", "", "getattr", "setattr", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "manage", NULL }},
    { "x_device", { "read", "write", "", "", "getattr", "setattr", "", "", "", "getfocus", "setfocus", "", "", "", "", "", "", "grab", "freeze", "force_cursor", "", "", "", "", "", "manage", "", "bell", NULL }},
    { "x_server", { "record", "", "", "", "getattr", "setattr", "", "", "", "", "", "", "", "", "", "", "", "grab", "", "", "", "", "", "", "", "manage", "debug", NULL }},
    { "x_extension", { "", "", "", "", "query", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "use", NULL }},
    { "x_event", { "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "send", "receive", NULL }},
    { "x_synthetic_event", { "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "send", "receive", NULL }},
    { "x_resource", { "read", "write", "write", "write", "read", "write", "read", "read", "write", "read", "write", "read", "write", "write", "write", "read", "read", "write", "write", "write", "write", "write", "write", "read", "read", "write", "read", "write", NULL }},
    { NULL }
};

/* forward declarations */
static void SELinuxScreen(CallbackListPtr *, pointer, pointer);

/* "true" pointer value for use as callback data */
static pointer truep = (pointer)1;


/*
 * Support Routines
 */

/*
 * Looks up the SID corresponding to the given selection atom
 */
static int
SELinuxSelectionToSID(Atom selection, SELinuxStateRec *sid_return)
{
    const char *name;
    unsigned i, size;

    for (i = 0; i < numKnownSelections; i++)
	if (knownSelections[i].selection == selection) {
	    sid_return->sid = knownSelections[i].sid;
	    return Success;
	}

    /* Need to increase size of array */
    i = numKnownSelections;
    size = (i + 1) * sizeof(SELinuxSelectionRec);
    knownSelections = xrealloc(knownSelections, size);
    if (!knownSelections)
	return BadAlloc;
    knownSelections[i].selection = selection;

    /* Look in the mappings of selection names to contexts */
    name = NameForAtom(selection);
    if (name) {
	security_context_t con;
	security_id_t sid;

	if (selabel_lookup(label_hnd, &con, name, SELABEL_X_SELN) < 0) {
	    ErrorF("XSELinux: a selection label lookup failed!\n");
	    return BadValue;
	}
	/* Get a SID for context */
	if (avc_context_to_sid(con, &sid) < 0) {
	    ErrorF("XSELinux: a context_to_SID call failed!\n");
	    return BadAlloc;
	}
	freecon(con);
	knownSelections[i].sid = sid_return->sid = sid;
    } else
	knownSelections[i].sid = sid_return->sid = unlabeled_sid;

    return Success;
}

/*
 * Looks up the SID corresponding to the given event type
 */
static int
SELinuxEventToSID(unsigned type, security_id_t sid_of_window,
		  SELinuxStateRec *sid_return)
{
    const char *name = LookupEventName(type);
    security_context_t con;
    type &= 127;

    if (type >= numKnownEvents) {
	/* Need to increase size of classes array */
	unsigned size = sizeof(security_id_t);
	knownEvents = xrealloc(knownEvents, (type + 1) * size);
	if (!knownEvents)
	    return BadAlloc;
	memset(knownEvents + numKnownEvents, 0,
	       (type - numKnownEvents + 1) * size);
	numKnownEvents = type + 1;
    }

    if (!knownEvents[type]) {
	/* Look in the mappings of event names to contexts */
	if (selabel_lookup(label_hnd, &con, name, SELABEL_X_EVENT) < 0) {
	    ErrorF("XSELinux: an event label lookup failed!\n");
	    return BadValue;
	}
	/* Get a SID for context */
	if (avc_context_to_sid(con, knownEvents + type) < 0) {
	    ErrorF("XSELinux: a context_to_SID call failed!\n");
	    return BadAlloc;
	}
	freecon(con);
    }

    /* Perform a transition to obtain the final SID */
    if (avc_compute_create(sid_of_window, knownEvents[type], SECCLASS_X_EVENT,
			   &sid_return->sid) < 0) {
	ErrorF("XSELinux: a compute_create call failed!\n");
	return BadValue;
    }

    return Success;
}

/*
 * Returns the object class corresponding to the given resource type.
 */
static security_class_t
SELinuxTypeToClass(RESTYPE type)
{
    RESTYPE fulltype = type;
    type &= TypeMask;

    if (type >= numKnownTypes) {
	/* Need to increase size of classes array */
	unsigned size = sizeof(security_class_t);
	knownTypes = xrealloc(knownTypes, (type + 1) * size);
	if (!knownTypes)
	    return 0;
	memset(knownTypes + numKnownTypes, 0,
	       (type - numKnownTypes + 1) * size);
	numKnownTypes = type + 1;
    }

    if (!knownTypes[type]) {
	const char *str;
	knownTypes[type] = SECCLASS_X_RESOURCE;

	if (fulltype & RC_DRAWABLE)
	    knownTypes[type] = SECCLASS_X_DRAWABLE;
	if (fulltype == RT_GC)
	    knownTypes[type] = SECCLASS_X_GC;
	if (fulltype == RT_FONT)
	    knownTypes[type] = SECCLASS_X_FONT;
	if (fulltype == RT_CURSOR)
	    knownTypes[type] = SECCLASS_X_CURSOR;
	if (fulltype == RT_COLORMAP)
	    knownTypes[type] = SECCLASS_X_COLORMAP;
	
	/* Need to do a string lookup */
	str = LookupResourceName(fulltype);
	if (!strcmp(str, "PICTURE"))
	    knownTypes[type] = SECCLASS_X_DRAWABLE;
	if (!strcmp(str, "GLYPHSET"))
	    knownTypes[type] = SECCLASS_X_FONT;
    }

    return knownTypes[type];
}

/*
 * Performs an SELinux permission check.
 */
static int
SELinuxDoCheck(int clientIndex, SELinuxStateRec *subj, SELinuxStateRec *obj,
	       security_class_t class, Mask mode, SELinuxAuditRec *auditdata)
{
    /* serverClient requests OK */
    if (clientIndex == 0)
	return Success;

    auditdata->command = subj->command;
    errno = 0;

    if (avc_has_perm(subj->sid, obj->sid, class, mode, &subj->aeref,
		     auditdata) < 0) {
	if (errno == EACCES)
	    return BadAccess;
	ErrorF("ServerPerm: unexpected error %d\n", errno);
	return BadValue;
    }

    return Success;
}

/*
 * Labels a newly connected client.
 */
static void
SELinuxLabelClient(ClientPtr client)
{
    XtransConnInfo ci = ((OsCommPtr)client->osPrivate)->trans_conn;
    SELinuxStateRec *state;
    security_context_t ctx;

    state = dixLookupPrivate(&client->devPrivates, stateKey);
    sidput(state->sid);

    if (_XSERVTransIsLocal(ci)) {
	int fd = _XSERVTransGetConnectionNumber(ci);
	struct ucred creds;
	socklen_t len = sizeof(creds);
	char path[PATH_MAX + 1];
	size_t bytes;

	/* For local clients, can get context from the socket */
	if (getpeercon(fd, &ctx) < 0)
	    FatalError("Client %d: couldn't get context from socket\n",
		       client->index);

	/* Try and determine the client's executable name */
	memset(&creds, 0, sizeof(creds));
	if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &creds, &len) < 0)
	    goto finish;

	snprintf(path, PATH_MAX + 1, "/proc/%d/cmdline", creds.pid);
	fd = open(path, O_RDONLY);
	if (fd < 0)
	    goto finish;

	bytes = read(fd, path, PATH_MAX + 1);
	close(fd);
	if (bytes <= 0)
	    goto finish;

	state->command = xalloc(bytes);
	if (!state->command)
	    goto finish;

	memcpy(state->command, path, bytes);
	state->command[bytes - 1] = 0;
    } else
	/* For remote clients, need to use a default context */
	if (selabel_lookup(label_hnd, &ctx, NULL, SELABEL_X_CLIENT) < 0)
	    FatalError("Client %d: couldn't get default remote context\n",
		       client->index);

finish:
    /* Get a SID from the context */
    if (avc_context_to_sid(ctx, &state->sid) < 0)
	FatalError("Client %d: context_to_sid(%s) failed\n",
		   client->index, ctx);

    freecon(ctx);
}

/*
 * Labels initial server objects.
 */
static void
SELinuxLabelInitial(void)
{
    int i;
    XaceScreenAccessRec srec;
    SELinuxStateRec *state;
    security_context_t ctx;
    pointer unused;

    /* Do the serverClient */
    state = dixLookupPrivate(&serverClient->devPrivates, stateKey);
    sidput(state->sid);

    /* Use the context of the X server process for the serverClient */
    if (getcon(&ctx) < 0)
	FatalError("Couldn't get context of X server process\n");

    /* Get a SID from the context */
    if (avc_context_to_sid(ctx, &state->sid) < 0)
	FatalError("serverClient: context_to_sid(%s) failed\n", ctx);

    freecon(ctx);

    srec.client = serverClient;
    srec.access_mode = DixCreateAccess;
    srec.status = Success;

    for (i = 0; i < screenInfo.numScreens; i++) {
	/* Do the screen object */
	srec.screen = screenInfo.screens[i];
	SELinuxScreen(NULL, NULL, &srec);

	/* Do the default colormap */
	dixLookupResource(&unused, screenInfo.screens[i]->defColormap,
			  RT_COLORMAP, serverClient, DixCreateAccess);
    }
}


/*
 * Libselinux Callbacks
 */

static int
SELinuxAudit(void *auditdata,
	     security_class_t class,
	     char *msgbuf,
	     size_t msgbufsize)
{
    SELinuxAuditRec *audit = auditdata;
    ClientPtr client = audit->client;
    char idNum[16], *propertyName, *selectionName;
    int major = -1, minor = -1;

    if (client) {
	REQUEST(xReq);
	if (stuff) {
	    major = stuff->reqType;
	    minor = MinorOpcodeOfRequest(client);
	}
    }
    if (audit->id)
	snprintf(idNum, 16, "%x", audit->id);

    propertyName = audit->property ? NameForAtom(audit->property) : NULL;
    selectionName = audit->selection ? NameForAtom(audit->selection) : NULL;

    return snprintf(msgbuf, msgbufsize, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		    (major >= 0) ? "request=" : "",
		    (major >= 0) ? LookupRequestName(major, minor) : "",
		    audit->command ? " comm=" : "",
		    audit->command ? audit->command : "",
		    audit->id ? " resid=" : "",
		    audit->id ? idNum : "",
		    audit->restype ? " restype=" : "",
		    audit->restype ? LookupResourceName(audit->restype) : "",
		    audit->event ? " event=" : "",
		    audit->event ? LookupEventName(audit->event & 127) : "",
		    audit->property ? " property=" : "",
		    audit->property ? propertyName : "",
		    audit->selection ? " selection=" : "",
		    audit->selection ? selectionName : "",
		    audit->extension ? " extension=" : "",
		    audit->extension ? audit->extension : "");
}

static int
SELinuxLog(int type, const char *fmt, ...)
{
    va_list ap;
    char buf[MAX_AUDIT_MESSAGE_LENGTH];
    int rc, aut = AUDIT_USER_AVC;

    va_start(ap, fmt);
    vsnprintf(buf, MAX_AUDIT_MESSAGE_LENGTH, fmt, ap);
    rc = audit_log_user_avc_message(audit_fd, aut, buf, NULL, NULL, NULL, 0);
    va_end(ap);
    return 0;
}

/*
 * XACE Callbacks
 */

static void
SELinuxDevice(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceDeviceAccessRec *rec = calldata;
    SELinuxStateRec *subj, *obj;
    SELinuxAuditRec auditdata = { .client = rec->client };
    int rc;

    subj = dixLookupPrivate(&rec->client->devPrivates, stateKey);
    obj = dixLookupPrivate(&rec->dev->devPrivates, stateKey);

    /* If this is a new object that needs labeling, do it now */
    if (rec->access_mode & DixCreateAccess) {
	sidput(obj->sid);

	/* Label the device directly with the process SID */
	sidget(subj->sid);
	obj->sid = subj->sid;
    }

    rc = SELinuxDoCheck(rec->client->index, subj, obj, SECCLASS_X_DEVICE,
			rec->access_mode, &auditdata);
    if (rc != Success)
	rec->status = rc;
}

static void
SELinuxSend(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceSendAccessRec *rec = calldata;
    SELinuxStateRec *subj, *obj, ev_sid;
    SELinuxAuditRec auditdata = { .client = rec->client };
    security_class_t class;
    int rc, i, type, clientIndex;

    if (rec->dev) {
	subj = dixLookupPrivate(&rec->dev->devPrivates, stateKey);
	clientIndex = -1; /* some nonzero value */
    } else {
	subj = dixLookupPrivate(&rec->client->devPrivates, stateKey);
	clientIndex = rec->client->index;
    }

    obj = dixLookupPrivate(&rec->pWin->devPrivates, stateKey);

    /* Check send permission on window */
    rc = SELinuxDoCheck(clientIndex, subj, obj, SECCLASS_X_DRAWABLE,
			DixSendAccess, &auditdata);
    if (rc != Success)
	goto err;

    /* Check send permission on specific event types */
    for (i = 0; i < rec->count; i++) {
	type = rec->events[i].u.u.type;
	class = (type & 128) ? SECCLASS_X_FAKEEVENT : SECCLASS_X_EVENT;

	rc = SELinuxEventToSID(type, obj->sid, &ev_sid);
	if (rc != Success)
	    goto err;

	auditdata.event = type;
	rc = SELinuxDoCheck(clientIndex, subj, &ev_sid, class,
			    DixSendAccess, &auditdata);
	if (rc != Success)
	    goto err;
    }
    return;
err:
    rec->status = rc;
}

static void
SELinuxReceive(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceReceiveAccessRec *rec = calldata;
    SELinuxStateRec *subj, *obj, ev_sid;
    SELinuxAuditRec auditdata = { .client = NULL };
    security_class_t class;
    int rc, i, type;

    subj = dixLookupPrivate(&rec->client->devPrivates, stateKey);
    obj = dixLookupPrivate(&rec->pWin->devPrivates, stateKey);

    /* Check receive permission on window */
    rc = SELinuxDoCheck(rec->client->index, subj, obj, SECCLASS_X_DRAWABLE,
			DixReceiveAccess, &auditdata);
    if (rc != Success)
	goto err;

    /* Check receive permission on specific event types */
    for (i = 0; i < rec->count; i++) {
	type = rec->events[i].u.u.type;
	class = (type & 128) ? SECCLASS_X_FAKEEVENT : SECCLASS_X_EVENT;

	rc = SELinuxEventToSID(type, obj->sid, &ev_sid);
	if (rc != Success)
	    goto err;

	auditdata.event = type;
	rc = SELinuxDoCheck(rec->client->index, subj, &ev_sid, class,
			    DixReceiveAccess, &auditdata);
	if (rc != Success)
	    goto err;
    }
    return;
err:
    rec->status = rc;
}

static void
SELinuxExtension(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceExtAccessRec *rec = calldata;
    SELinuxStateRec *subj, *obj, *serv;
    SELinuxAuditRec auditdata = { .client = rec->client };
    int rc;

    subj = dixLookupPrivate(&rec->client->devPrivates, stateKey);
    obj = dixLookupPrivate(&rec->ext->devPrivates, stateKey);

    /* If this is a new object that needs labeling, do it now */
    /* XXX there should be a separate callback for this */
    if (obj->sid == unlabeled_sid) {
	const char *name = rec->ext->name;
	security_context_t con;
	security_id_t sid;

	serv = dixLookupPrivate(&serverClient->devPrivates, stateKey);

	/* Look in the mappings of property names to contexts */
	if (selabel_lookup(label_hnd, &con, name, SELABEL_X_EXT) < 0) {
	    ErrorF("XSELinux: a property label lookup failed!\n");
	    rec->status = BadValue;
	    return;
	}
	/* Get a SID for context */
	if (avc_context_to_sid(con, &sid) < 0) {
	    ErrorF("XSELinux: a context_to_SID call failed!\n");
	    rec->status = BadAlloc;
	    return;
	}

	sidput(obj->sid);

	/* Perform a transition to obtain the final SID */
	if (avc_compute_create(serv->sid, sid, SECCLASS_X_EXTENSION,
			       &obj->sid) < 0) {
	    ErrorF("XSELinux: a SID transition call failed!\n");
	    freecon(con);
	    rec->status = BadValue;
	    return;
	}
	freecon(con);
    }

    /* Perform the security check */
    auditdata.extension = rec->ext->name;
    rc = SELinuxDoCheck(rec->client->index, subj, obj, SECCLASS_X_EXTENSION,
			rec->access_mode, &auditdata);
    if (rc != Success)
	rec->status = rc;
}

static void
SELinuxProperty(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XacePropertyAccessRec *rec = calldata;
    SELinuxStateRec *subj, *obj;
    SELinuxAuditRec auditdata = { .client = rec->client };
    int rc;

    subj = dixLookupPrivate(&rec->client->devPrivates, stateKey);
    obj = dixLookupPrivate(&rec->pProp->devPrivates, stateKey);

    /* If this is a new object that needs labeling, do it now */
    if (rec->access_mode & DixCreateAccess) {
	const char *name = NameForAtom(rec->pProp->propertyName);
	security_context_t con;
	security_id_t sid;

	/* Look in the mappings of property names to contexts */
	if (selabel_lookup(label_hnd, &con, name, SELABEL_X_PROP) < 0) {
	    ErrorF("XSELinux: a property label lookup failed!\n");
	    rec->status = BadValue;
	    return;
	}
	/* Get a SID for context */
	if (avc_context_to_sid(con, &sid) < 0) {
	    ErrorF("XSELinux: a context_to_SID call failed!\n");
	    rec->status = BadAlloc;
	    return;
	}

	sidput(obj->sid);

	/* Perform a transition to obtain the final SID */
	if (avc_compute_create(subj->sid, sid, SECCLASS_X_PROPERTY,
			       &obj->sid) < 0) {
	    ErrorF("XSELinux: a SID transition call failed!\n");
	    freecon(con);
	    rec->status = BadValue;
	    return;
	}
	freecon(con);
	avc_entry_ref_init(&obj->aeref);
    }

    /* Perform the security check */
    auditdata.property = rec->pProp->propertyName;
    rc = SELinuxDoCheck(rec->client->index, subj, obj, SECCLASS_X_PROPERTY,
			rec->access_mode, &auditdata);
    if (rc != Success)
	rec->status = rc;
}

static void
SELinuxResource(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceResourceAccessRec *rec = calldata;
    SELinuxStateRec *subj, *obj, *pobj;
    SELinuxAuditRec auditdata = { .client = rec->client };
    PrivateRec **privatePtr;
    security_class_t class;
    int rc, offset;

    subj = dixLookupPrivate(&rec->client->devPrivates, stateKey);

    /* Determine if the resource object has a devPrivates field */
    offset = dixLookupPrivateOffset(rec->rtype);
    if (offset < 0) {
	/* No: use the SID of the owning client */
	class = SECCLASS_X_RESOURCE;
	privatePtr = &clients[CLIENT_ID(rec->id)]->devPrivates;
	obj = dixLookupPrivate(privatePtr, stateKey);
    } else {
	/* Yes: use the SID from the resource object itself */
	class = SELinuxTypeToClass(rec->rtype);
	privatePtr = DEVPRIV_AT(rec->res, offset);
	obj = dixLookupPrivate(privatePtr, stateKey);
    }

    /* If this is a new object that needs labeling, do it now */
    if (rec->access_mode & DixCreateAccess && offset >= 0) {
	if (rec->parent)
	    offset = dixLookupPrivateOffset(rec->ptype);
	if (rec->parent && offset >= 0)
	    /* Use the SID of the parent object in the labeling operation */
	    pobj = dixLookupPrivate(DEVPRIV_AT(rec->parent, offset), stateKey);
	else
	    /* Use the SID of the subject */
	    pobj = subj;

	sidput(obj->sid);

	/* Perform a transition to obtain the final SID */
	if (avc_compute_create(subj->sid, pobj->sid, class, &obj->sid) < 0) {
	    ErrorF("XSELinux: a compute_create call failed!\n");
	    rec->status = BadValue;
	    return;
	}
    }

    /* Perform the security check */
    auditdata.restype = rec->rtype;
    auditdata.id = rec->id;
    rc = SELinuxDoCheck(rec->client->index, subj, obj, class,
			rec->access_mode, &auditdata);
    if (rc != Success)
	rec->status = rc;
}

static void
SELinuxScreen(CallbackListPtr *pcbl, pointer is_saver, pointer calldata)
{
    XaceScreenAccessRec *rec = calldata;
    SELinuxStateRec *subj, *obj;
    SELinuxAuditRec auditdata = { .client = rec->client };
    Mask access_mode = rec->access_mode;
    int rc;

    subj = dixLookupPrivate(&rec->client->devPrivates, stateKey);
    obj = dixLookupPrivate(&rec->screen->devPrivates, stateKey);

    /* If this is a new object that needs labeling, do it now */
    if (access_mode & DixCreateAccess) {
	sidput(obj->sid);

	/* Perform a transition to obtain the final SID */
	if (avc_compute_create(subj->sid, subj->sid, SECCLASS_X_SCREEN,
			       &obj->sid) < 0) {
	    ErrorF("XSELinux: a compute_create call failed!\n");
	    rec->status = BadValue;
	    return;
	}
    }

    if (is_saver)
	access_mode <<= 2;

    rc = SELinuxDoCheck(rec->client->index, subj, obj, SECCLASS_X_SCREEN,
			access_mode, &auditdata);
    if (rc != Success)
	rec->status = rc;
}

static void
SELinuxClient(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceClientAccessRec *rec = calldata;
    SELinuxStateRec *subj, *obj;
    SELinuxAuditRec auditdata = { .client = rec->client };
    int rc;

    subj = dixLookupPrivate(&rec->client->devPrivates, stateKey);
    obj = dixLookupPrivate(&rec->target->devPrivates, stateKey);

    rc = SELinuxDoCheck(rec->client->index, subj, obj, SECCLASS_X_CLIENT,
			rec->access_mode, &auditdata);
    if (rc != Success)
	rec->status = rc;
}

static void
SELinuxServer(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceServerAccessRec *rec = calldata;
    SELinuxStateRec *subj, *obj;
    SELinuxAuditRec auditdata = { .client = rec->client };
    int rc;

    subj = dixLookupPrivate(&rec->client->devPrivates, stateKey);
    obj = dixLookupPrivate(&serverClient->devPrivates, stateKey);

    rc = SELinuxDoCheck(rec->client->index, subj, obj, SECCLASS_X_SERVER,
			rec->access_mode, &auditdata);
    if (rc != Success)
	rec->status = rc;
}

static void
SELinuxSelection(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceSelectionAccessRec *rec = (XaceSelectionAccessRec *)calldata;
    SELinuxStateRec *subj, sel_sid;
    SELinuxAuditRec auditdata = { .client = rec->client };
    int rc;

    subj = dixLookupPrivate(&rec->client->devPrivates, stateKey);

    rc = SELinuxSelectionToSID(rec->name, &sel_sid);
    if (rc != Success) {
	rec->status = rc;
	return;
    }

    auditdata.selection = rec->name;
    rc = SELinuxDoCheck(rec->client->index, subj, &sel_sid,
			SECCLASS_X_SELECTION, rec->access_mode, &auditdata);
    if (rc != Success)
	rec->status = rc;
}


/*
 * DIX Callbacks
 */

static void
SELinuxClientState(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    NewClientInfoRec *pci = calldata;

    switch (pci->client->clientState) {
    case ClientStateInitial:
	SELinuxLabelClient(pci->client);
	break;

    case ClientStateRetained:
    case ClientStateGone:
	if (pci->client == selectionManager) {
	    selectionManager = NULL;
	    selectionWindow = 0;
	}
	break;

    default:
	break;
    }
}

static void
SELinuxResourceState(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    ResourceStateInfoRec *rec = calldata;
    SELinuxStateRec *state;
    WindowPtr pWin;

    if (rec->type != RT_WINDOW)
	return;

    pWin = (WindowPtr)rec->value;
    state = dixLookupPrivate(&wClient(pWin)->devPrivates, stateKey);

    if (state->sid) {
	security_context_t ctx;
	int rc = avc_sid_to_context(state->sid, &ctx);
	if (rc < 0)
	    FatalError("XSELinux: Failed to get security context!\n");
	rc = dixChangeWindowProperty(serverClient,
				     pWin, atom_client_ctx, XA_STRING, 8,
				     PropModeReplace, strlen(ctx), ctx, FALSE);
	if (rc != Success)
	    FatalError("XSELinux: Failed to set label property on window!\n");
	freecon(ctx);
    }
    else
	FatalError("XSELinux: Unexpected unlabeled client found\n");

    state = dixLookupPrivate(&pWin->devPrivates, stateKey);

    if (state->sid) {
	security_context_t ctx;
	int rc = avc_sid_to_context(state->sid, &ctx);
	if (rc < 0)
	    FatalError("XSELinux: Failed to get security context!\n");
	rc = dixChangeWindowProperty(serverClient,
				     pWin, atom_ctx, XA_STRING, 8,
				     PropModeReplace, strlen(ctx), ctx, FALSE);
	if (rc != Success)
	    FatalError("XSELinux: Failed to set label property on window!\n");
	freecon(ctx);
    }
    else
	FatalError("XSELinux: Unexpected unlabeled window found\n");
}

static void
SELinuxSelectionState(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    SelectionInfoRec *rec = calldata;
    SELinuxStateRec *subj, *obj;

    switch (rec->kind) {
    case SelectionSetOwner:
	/* save off the "real" owner of the selection */
	rec->selection->alt_client = rec->selection->client;
	rec->selection->alt_window = rec->selection->window;

	/* figure out the new label for the content */
	subj = dixLookupPrivate(&rec->client->devPrivates, stateKey);
	obj = dixLookupPrivate(&rec->selection->devPrivates, stateKey);
	sidput(obj->sid);

	if (avc_compute_create(subj->sid, subj->sid, SECCLASS_X_SELECTION,
			       &obj->sid) < 0) {
	    ErrorF("XSELinux: a compute_create call failed!\n");
	    obj->sid = unlabeled_sid;
	}
	break;

    case SelectionGetOwner:
	/* restore the real owner */
	rec->selection->window = rec->selection->alt_window;
	break;

    case SelectionConvertSelection:
	/* redirect the convert request if necessary */
	if (selectionManager && selectionManager != rec->client) {
	    rec->selection->client = selectionManager;
	    rec->selection->window = selectionWindow;
	} else {
	    rec->selection->client = rec->selection->alt_client;
	    rec->selection->window = rec->selection->alt_window;
	}
	break;
    default:
	break;
    }
}


/*
 * DevPrivates Callbacks
 */

static void
SELinuxStateInit(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    PrivateCallbackRec *rec = calldata;
    SELinuxStateRec *state = *rec->value;

    sidget(unlabeled_sid);
    state->sid = unlabeled_sid;

    avc_entry_ref_init(&state->aeref);
}

static void
SELinuxStateFree(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    PrivateCallbackRec *rec = calldata;
    SELinuxStateRec *state = *rec->value;

    xfree(state->command);

    if (avc_active)
	sidput(state->sid);
}


/*
 * Extension Dispatch
 */

static int
ProcSELinuxQueryVersion(ClientPtr client)
{
    SELinuxQueryVersionReply rep;

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.server_major = XSELINUX_MAJOR_VERSION;
    rep.server_minor = XSELINUX_MINOR_VERSION;
    if (client->swapped) {
	int n;
	swaps(&rep.sequenceNumber, n);
	swapl(&rep.length, n);
	swaps(&rep.server_major, n);
	swaps(&rep.server_minor, n);
    }
    WriteToClient(client, sizeof(rep), (char *)&rep);
    return (client->noClientException);
}

static int
ProcSELinuxSetSelectionManager(ClientPtr client)
{
    WindowPtr pWin;
    int rc;

    REQUEST(SELinuxSetSelectionManagerReq);
    REQUEST_SIZE_MATCH(SELinuxSetSelectionManagerReq);

    if (stuff->window == None) {
	selectionManager = NULL;
	selectionWindow = None;
    } else {
	rc = dixLookupResource((pointer *)&pWin, stuff->window, RT_WINDOW,
			       client, DixGetAttrAccess);
	if (rc != Success)
	    return rc;

	selectionManager = client;
	selectionWindow = stuff->window;
    }

    return Success;
}

static int
ProcSELinuxGetSelectionManager(ClientPtr client)
{
    SELinuxGetSelectionManagerReply rep;

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.window = selectionWindow;
    if (client->swapped) {
	int n;
	swaps(&rep.sequenceNumber, n);
	swapl(&rep.length, n);
	swapl(&rep.window, n);
    }
    WriteToClient(client, sizeof(rep), (char *)&rep);
    return (client->noClientException);
}

static int
ProcSELinuxSetDeviceCreateContext(ClientPtr client)
{
    return Success;
}

static int
ProcSELinuxGetDeviceCreateContext(ClientPtr client)
{
    return Success;
}

static int
ProcSELinuxSetDeviceContext(ClientPtr client)
{
    char *ctx;
    security_id_t sid;
    DeviceIntPtr dev;
    SELinuxStateRec *state;
    int rc;

    REQUEST(SELinuxSetContextReq);
    REQUEST_FIXED_SIZE(SELinuxSetContextReq, stuff->context_len);

    ctx = (char *)(stuff + 1);
    if (ctx[stuff->context_len - 1])
	return BadLength;

    rc = dixLookupDevice(&dev, stuff->id, client, DixManageAccess);
    if (rc != Success)
	return rc;

    rc = avc_context_to_sid(ctx, &sid);
    if (rc != Success)
	return BadValue;

    state = dixLookupPrivate(&dev->devPrivates, stateKey);
    sidput(state->sid);
    state->sid = sid;
    return Success;
}

static int
ProcSELinuxGetDeviceContext(ClientPtr client)
{
    return Success;
}

static int
ProcSELinuxSetPropertyCreateContext(ClientPtr client)
{
    return Success;
}

static int
ProcSELinuxGetPropertyCreateContext(ClientPtr client)
{
    return Success;
}

static int
ProcSELinuxGetPropertyContext(ClientPtr client)
{
    return Success;
}

static int
ProcSELinuxSetWindowCreateContext(ClientPtr client)
{
    return Success;
}

static int
ProcSELinuxGetWindowCreateContext(ClientPtr client)
{
    return Success;
}

static int
ProcSELinuxGetWindowContext(ClientPtr client)
{
    return Success;
}

static int
ProcSELinuxDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_SELinuxQueryVersion:
        return ProcSELinuxQueryVersion(client);
    case X_SELinuxSetSelectionManager:
	return ProcSELinuxSetSelectionManager(client);
    case X_SELinuxGetSelectionManager:
    	return ProcSELinuxGetSelectionManager(client);
    case X_SELinuxSetDeviceCreateContext:
    	return ProcSELinuxSetDeviceCreateContext(client);
    case X_SELinuxGetDeviceCreateContext:
    	return ProcSELinuxGetDeviceCreateContext(client);
    case X_SELinuxSetDeviceContext:
    	return ProcSELinuxSetDeviceContext(client);
    case X_SELinuxGetDeviceContext:
    	return ProcSELinuxGetDeviceContext(client);
    case X_SELinuxSetPropertyCreateContext:
    	return ProcSELinuxSetPropertyCreateContext(client);
    case X_SELinuxGetPropertyCreateContext:
    	return ProcSELinuxGetPropertyCreateContext(client);
    case X_SELinuxGetPropertyContext:
    	return ProcSELinuxGetPropertyContext(client);
    case X_SELinuxSetWindowCreateContext:
    	return ProcSELinuxSetWindowCreateContext(client);
    case X_SELinuxGetWindowCreateContext:
    	return ProcSELinuxGetWindowCreateContext(client);
    case X_SELinuxGetWindowContext:
    	return ProcSELinuxGetWindowContext(client);
    default:
	return BadRequest;
    }
}

static int
SProcSELinuxQueryVersion(ClientPtr client)
{
    REQUEST(SELinuxQueryVersionReq);
    int n;

    REQUEST_SIZE_MATCH (SELinuxQueryVersionReq);
    swaps(&stuff->client_major,n);
    swaps(&stuff->client_minor,n);
    return ProcSELinuxQueryVersion(client);
}

static int
SProcSELinuxSetSelectionManager(ClientPtr client)
{
    REQUEST(SELinuxSetSelectionManagerReq);
    int n;

    REQUEST_SIZE_MATCH (SELinuxSetSelectionManagerReq);
    swapl(&stuff->window,n);
    return ProcSELinuxSetSelectionManager(client);
}

static int
SProcSELinuxSetDeviceCreateContext(ClientPtr client)
{
    REQUEST(SELinuxSetCreateContextReq);
    int n;

    REQUEST_AT_LEAST_SIZE(SELinuxSetCreateContextReq);
    swaps(&stuff->context_len,n);
    return ProcSELinuxSetDeviceCreateContext(client);
}

static int
SProcSELinuxSetDeviceContext(ClientPtr client)
{
    REQUEST(SELinuxSetContextReq);
    int n;

    REQUEST_AT_LEAST_SIZE(SELinuxSetContextReq);
    swapl(&stuff->id,n);
    swaps(&stuff->context_len,n);
    return ProcSELinuxSetDeviceContext(client);
}

static int
SProcSELinuxGetDeviceContext(ClientPtr client)
{
    REQUEST(SELinuxGetContextReq);
    int n;

    REQUEST_SIZE_MATCH(SELinuxGetContextReq);
    swapl(&stuff->id,n);
    return ProcSELinuxGetDeviceContext(client);
}

static int
SProcSELinuxSetPropertyCreateContext(ClientPtr client)
{
    REQUEST(SELinuxSetCreateContextReq);
    int n;

    REQUEST_AT_LEAST_SIZE(SELinuxSetCreateContextReq);
    swaps(&stuff->context_len,n);
    return ProcSELinuxSetPropertyCreateContext(client);
}

static int
SProcSELinuxGetPropertyContext(ClientPtr client)
{
    REQUEST(SELinuxGetPropertyContextReq);
    int n;

    REQUEST_SIZE_MATCH(SELinuxGetPropertyContextReq);
    swapl(&stuff->window,n);
    swapl(&stuff->property,n);
    return ProcSELinuxGetPropertyContext(client);
}

static int
SProcSELinuxSetWindowCreateContext(ClientPtr client)
{
    REQUEST(SELinuxSetCreateContextReq);
    int n;

    REQUEST_AT_LEAST_SIZE(SELinuxSetCreateContextReq);
    swaps(&stuff->context_len,n);
    return ProcSELinuxSetWindowCreateContext(client);
}

static int
SProcSELinuxGetWindowContext(ClientPtr client)
{
    REQUEST(SELinuxGetContextReq);
    int n;

    REQUEST_SIZE_MATCH(SELinuxGetContextReq);
    swapl(&stuff->id,n);
    return ProcSELinuxGetWindowContext(client);
}

static int
SProcSELinuxDispatch(ClientPtr client)
{
    REQUEST(xReq);
    int n;

    swaps(&stuff->length, n);

    switch (stuff->data) {
    case X_SELinuxQueryVersion:
        return SProcSELinuxQueryVersion(client);
    case X_SELinuxSetSelectionManager:
	return SProcSELinuxSetSelectionManager(client);
    case X_SELinuxGetSelectionManager:
    	return ProcSELinuxGetSelectionManager(client);
    case X_SELinuxSetDeviceCreateContext:
    	return SProcSELinuxSetDeviceCreateContext(client);
    case X_SELinuxGetDeviceCreateContext:
    	return ProcSELinuxGetDeviceCreateContext(client);
    case X_SELinuxSetDeviceContext:
    	return SProcSELinuxSetDeviceContext(client);
    case X_SELinuxGetDeviceContext:
    	return SProcSELinuxGetDeviceContext(client);
    case X_SELinuxSetPropertyCreateContext:
    	return SProcSELinuxSetPropertyCreateContext(client);
    case X_SELinuxGetPropertyCreateContext:
    	return ProcSELinuxGetPropertyCreateContext(client);
    case X_SELinuxGetPropertyContext:
    	return SProcSELinuxGetPropertyContext(client);
    case X_SELinuxSetWindowCreateContext:
    	return SProcSELinuxSetWindowCreateContext(client);
    case X_SELinuxGetWindowCreateContext:
    	return ProcSELinuxGetWindowCreateContext(client);
    case X_SELinuxGetWindowContext:
    	return SProcSELinuxGetWindowContext(client);
    default:
	return BadRequest;
    }
}


/*
 * Extension Setup / Teardown
 */

static void
SELinuxResetProc(ExtensionEntry *extEntry)
{
    /* Unregister callbacks */
    DeleteCallback(&ClientStateCallback, SELinuxClientState, NULL);
    DeleteCallback(&ResourceStateCallback, SELinuxResourceState, NULL);
    DeleteCallback(&SelectionCallback, SELinuxSelectionState, NULL);

    XaceDeleteCallback(XACE_EXT_DISPATCH, SELinuxExtension, NULL);
    XaceDeleteCallback(XACE_RESOURCE_ACCESS, SELinuxResource, NULL);
    XaceDeleteCallback(XACE_DEVICE_ACCESS, SELinuxDevice, NULL);
    XaceDeleteCallback(XACE_PROPERTY_ACCESS, SELinuxProperty, NULL);
    XaceDeleteCallback(XACE_SEND_ACCESS, SELinuxSend, NULL);
    XaceDeleteCallback(XACE_RECEIVE_ACCESS, SELinuxReceive, NULL);
    XaceDeleteCallback(XACE_CLIENT_ACCESS, SELinuxClient, NULL);
    XaceDeleteCallback(XACE_EXT_ACCESS, SELinuxExtension, NULL);
    XaceDeleteCallback(XACE_SERVER_ACCESS, SELinuxServer, NULL);
    XaceDeleteCallback(XACE_SELECTION_ACCESS, SELinuxSelection, NULL);
    XaceDeleteCallback(XACE_SCREEN_ACCESS, SELinuxScreen, NULL);
    XaceDeleteCallback(XACE_SCREENSAVER_ACCESS, SELinuxScreen, truep);

    /* Tear down SELinux stuff */
    selabel_close(label_hnd);
    label_hnd = NULL;

    audit_close(audit_fd);

    avc_destroy();
    avc_active = 0;

    /* Free local state */
    xfree(knownSelections);
    knownSelections = NULL;
    numKnownSelections = 0;

    xfree(knownEvents);
    knownEvents = NULL;
    numKnownEvents = 0;

    xfree(knownTypes);
    knownTypes = NULL;
    numKnownTypes = 0;
}

void
XSELinuxExtensionInit(INITARGS)
{
    ExtensionEntry *extEntry;
    struct selinux_opt options[] = { { SELABEL_OPT_VALIDATE, (char *)1 } };
    security_context_t con;
    int ret = TRUE;

    /* Setup SELinux stuff */
    if (!is_selinux_enabled()) {
        ErrorF("XSELinux: SELinux not enabled, disabling SELinux support.\n");
        return;
    }

    selinux_set_callback(SELINUX_CB_LOG, (union selinux_callback)SELinuxLog);
    selinux_set_callback(SELINUX_CB_AUDIT, (union selinux_callback)SELinuxAudit);

    if (selinux_set_mapping(map) < 0) {
	if (errno == EINVAL) {
	    ErrorF("XSELinux: Invalid object class mapping, disabling SELinux support.\n");
	    return;
	}
	FatalError("XSELinux: Failed to set up security class mapping\n");
    }

    if (avc_open(NULL, 0) < 0)
	FatalError("XSELinux: Couldn't initialize SELinux userspace AVC\n");
    avc_active = 1;

    label_hnd = selabel_open(SELABEL_CTX_X, options, 1);
    if (!label_hnd)
	FatalError("XSELinux: Failed to open x_contexts mapping in policy\n");

    if (security_get_initial_context("unlabeled", &con) < 0)
	FatalError("XSELinux: Failed to look up unlabeled context\n");
    if (avc_context_to_sid(con, &unlabeled_sid) < 0)
	FatalError("XSELinux: a context_to_SID call failed!\n");
    freecon(con);

    /* Prepare for auditing */
    audit_fd = audit_open();
    if (audit_fd < 0)
        FatalError("XSELinux: Failed to open the system audit log\n");

    /* Allocate private storage */
    if (!dixRequestPrivate(stateKey, sizeof(SELinuxStateRec)))
	FatalError("XSELinux: Failed to allocate private storage.\n");

    /* Create atoms for doing window labeling */
    atom_ctx = MakeAtom("_SELINUX_CONTEXT", 16, TRUE);
    if (atom_ctx == BAD_RESOURCE)
	FatalError("XSELinux: Failed to create atom\n");
    atom_client_ctx = MakeAtom("_SELINUX_CLIENT_CONTEXT", 23, TRUE);
    if (atom_client_ctx == BAD_RESOURCE)
	FatalError("XSELinux: Failed to create atom\n");

    /* Register callbacks */
    ret &= dixRegisterPrivateInitFunc(stateKey, SELinuxStateInit, NULL);
    ret &= dixRegisterPrivateDeleteFunc(stateKey, SELinuxStateFree, NULL);

    ret &= AddCallback(&ClientStateCallback, SELinuxClientState, NULL);
    ret &= AddCallback(&ResourceStateCallback, SELinuxResourceState, NULL);
    ret &= AddCallback(&SelectionCallback, SELinuxSelectionState, NULL);

    ret &= XaceRegisterCallback(XACE_EXT_DISPATCH, SELinuxExtension, NULL);
    ret &= XaceRegisterCallback(XACE_RESOURCE_ACCESS, SELinuxResource, NULL);
    ret &= XaceRegisterCallback(XACE_DEVICE_ACCESS, SELinuxDevice, NULL);
    ret &= XaceRegisterCallback(XACE_PROPERTY_ACCESS, SELinuxProperty, NULL);
    ret &= XaceRegisterCallback(XACE_SEND_ACCESS, SELinuxSend, NULL);
    ret &= XaceRegisterCallback(XACE_RECEIVE_ACCESS, SELinuxReceive, NULL);
    ret &= XaceRegisterCallback(XACE_CLIENT_ACCESS, SELinuxClient, NULL);
    ret &= XaceRegisterCallback(XACE_EXT_ACCESS, SELinuxExtension, NULL);
    ret &= XaceRegisterCallback(XACE_SERVER_ACCESS, SELinuxServer, NULL);
    ret &= XaceRegisterCallback(XACE_SELECTION_ACCESS, SELinuxSelection, NULL);
    ret &= XaceRegisterCallback(XACE_SCREEN_ACCESS, SELinuxScreen, NULL);
    ret &= XaceRegisterCallback(XACE_SCREENSAVER_ACCESS, SELinuxScreen, truep);
    if (!ret)
	FatalError("XSELinux: Failed to register one or more callbacks\n");

    /* Add extension to server */
    extEntry = AddExtension(XSELINUX_EXTENSION_NAME,
			    XSELinuxNumberEvents, XSELinuxNumberErrors,
			    ProcSELinuxDispatch, SProcSELinuxDispatch,
			    SELinuxResetProc, StandardMinorOpcode);

    /* Label objects that were created before we could register ourself */
    SELinuxLabelInitial();
}
