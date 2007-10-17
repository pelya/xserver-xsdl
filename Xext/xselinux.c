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
    char *client_path;
} SELinuxStateRec;

/* audit file descriptor */
static int audit_fd;

/* structure passed to auditing callback */
typedef struct {
    ClientPtr client;	/* client */
    char *client_path;	/* client's executable path */
    unsigned id;	/* resource id, if any */
    int restype;	/* resource type, if any */
    Atom property;	/* property name, if any */
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
    { "x_resource", { "read", "write", "write", "write", "read", "write", "read", "read", "write", "read", "write", "read", "write", "write", "write", "read", "read", "write", "write", "write", "write", "write", "write", "read", "read", "write", "read", "write", NULL }},
    { NULL }
};

/* forward declarations */
static void SELinuxScreen(CallbackListPtr *, pointer, pointer);


/*
 * Support Routines
 */

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
SELinuxDoCheck(ClientPtr client, SELinuxStateRec *obj, security_class_t class,
	       Mask access_mode, SELinuxAuditRec *auditdata)
{
    SELinuxStateRec *subj;

    /* serverClient requests OK */
    if (client->index == 0)
	return Success;

    subj = dixLookupPrivate(&client->devPrivates, stateKey);
    auditdata->client = client;
    auditdata->client_path = subj->client_path;
    errno = 0;

    if (avc_has_perm(subj->sid, obj->sid, class, access_mode, &subj->aeref,
		     auditdata) < 0) {
	if (errno == EACCES)
	    return BadAccess;
	ErrorF("ServerPerm: unexpected error %d\n", errno);
	return BadValue;
    }

    return Success;
}

/*
 * Labels initial server objects.
 */
static void
SELinuxFixupLabels(void)
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
    char idNum[16], *propertyName;
    int major = 0, minor = 0;
    REQUEST(xReq);

    if (audit->id)
	snprintf(idNum, 16, "%x", audit->id);
    if (stuff) {
	major = stuff->reqType;
	minor = (major < 128) ? 0 : MinorOpcodeOfRequest(client);
    }

    propertyName = audit->property ? NameForAtom(audit->property) : NULL;

    return snprintf(msgbuf, msgbufsize, "%s%s%s%s%s%s%s%s%s%s%s%s",
		    stuff ? "request=" : "",
		    stuff ? LookupRequestName(major, minor) : "",
		    audit->client_path ? " comm=" : "",
		    audit->client_path ? audit->client_path : "",
		    audit->id ? " resid=" : "",
		    audit->id ? idNum : "",
		    audit->restype ? " restype=" : "",
		    audit->restype ? LookupResourceName(audit->restype) : "",
		    audit->property ? " property=" : "",
		    audit->property ? propertyName : "",
		    audit->extension ? " extension=" : "",
		    audit->extension ? audit->extension : "");
}

static int
SELinuxLog(int type, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    VErrorF(fmt, ap);
    va_end(ap);
    return 0;
}

/*
 * XACE Callbacks
 */

static void
SELinuxExtension(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceExtAccessRec *rec = calldata;
    SELinuxStateRec *subj, *obj, *serv;
    SELinuxAuditRec auditdata = { NULL, NULL, 0, 0, 0, NULL };
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
    rc = SELinuxDoCheck(rec->client, obj, SECCLASS_X_EXTENSION,
			rec->access_mode, &auditdata);
    if (rc != Success)
	rec->status = rc;
}

static void
SELinuxProperty(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XacePropertyAccessRec *rec = calldata;
    SELinuxStateRec *subj, *obj;
    SELinuxAuditRec auditdata = { NULL, NULL, 0, 0, 0, NULL };
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
    rc = SELinuxDoCheck(rec->client, obj, SECCLASS_X_PROPERTY,
			rec->access_mode, &auditdata);
    if (rc != Success)
	rec->status = rc;
}

static void
SELinuxResource(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceResourceAccessRec *rec = calldata;
    SELinuxStateRec *subj, *obj, *pobj;
    SELinuxAuditRec auditdata = { NULL, NULL, 0, 0, 0, NULL };
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
    rc = SELinuxDoCheck(rec->client, obj, class, rec->access_mode, &auditdata);
    if (rc != Success)
	rec->status = rc;
}

static void
SELinuxScreen(CallbackListPtr *pcbl, pointer is_saver, pointer calldata)
{
    XaceScreenAccessRec *rec = calldata;
    SELinuxStateRec *subj, *obj;
    SELinuxAuditRec auditdata = { NULL, NULL, 0, 0, 0, NULL };
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

    rc = SELinuxDoCheck(rec->client, obj, SECCLASS_X_SCREEN,
			access_mode, &auditdata);
    if (rc != Success)
	rec->status = rc;
}

static void
SELinuxClient(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceClientAccessRec *rec = calldata;
    SELinuxStateRec *obj;
    SELinuxAuditRec auditdata = { NULL, NULL, 0, 0, 0, NULL };
    int rc;

    obj = dixLookupPrivate(&rec->target->devPrivates, stateKey);

    rc = SELinuxDoCheck(rec->client, obj, SECCLASS_X_CLIENT,
			rec->access_mode, &auditdata);
    if (rc != Success)
	rec->status = rc;
}

static void
SELinuxServer(CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceServerAccessRec *rec = calldata;
    SELinuxStateRec *obj;
    SELinuxAuditRec auditdata = { NULL, NULL, 0, 0, 0, NULL };
    int rc;

    obj = dixLookupPrivate(&serverClient->devPrivates, stateKey);

    rc = SELinuxDoCheck(rec->client, obj, SECCLASS_X_SERVER,
			rec->access_mode, &auditdata);
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
    ClientPtr client = pci->client;
    XtransConnInfo ci = ((OsCommPtr)client->osPrivate)->trans_conn;
    SELinuxStateRec *state;
    security_context_t ctx;

    if (client->clientState != ClientStateInitial)
	return;

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

	state->client_path = xalloc(bytes);
	if (!state->client_path)
	    goto finish;

	memcpy(state->client_path, path, bytes);
	state->client_path[bytes - 1] = 0;
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

    xfree(state->client_path);

    if (avc_active)
	sidput(state->sid);
}


/*
 * Extension Dispatch
 */

static int
ProcSELinuxDispatch(ClientPtr client)
{
    return BadRequest;
}


/*
 * Extension Setup / Teardown
 */

static void
SELinuxResetProc(ExtensionEntry *extEntry)
{
    /* XXX unregister all callbacks here */

    selabel_close(label_hnd);
    label_hnd = NULL;

    audit_close(audit_fd);

    avc_destroy();
    avc_active = 0;

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
        ErrorF("XSELinux: Extension failed to load: SELinux not enabled\n");
        return;
    }

    selinux_set_callback(SELINUX_CB_LOG, (union selinux_callback)SELinuxLog);
    selinux_set_callback(SELINUX_CB_AUDIT, (union selinux_callback)SELinuxAudit);

    if (selinux_set_mapping(map) < 0)
	FatalError("XSELinux: Failed to set up security class mapping\n");

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

    ret &= AddCallback(&ClientStateCallback, SELinuxClientState, 0);
    ret &= AddCallback(&ResourceStateCallback, SELinuxResourceState, 0);

    ret &= XaceRegisterCallback(XACE_EXT_DISPATCH, SELinuxExtension, 0);
    ret &= XaceRegisterCallback(XACE_RESOURCE_ACCESS, SELinuxResource, 0);
//    ret &= XaceRegisterCallback(XACE_DEVICE_ACCESS, SELinuxDevice, 0);
    ret &= XaceRegisterCallback(XACE_PROPERTY_ACCESS, SELinuxProperty, 0);
//    ret &= XaceRegisterCallback(XACE_SEND_ACCESS, SELinuxSend, 0);
//    ret &= XaceRegisterCallback(XACE_RECEIVE_ACCESS, SELinuxReceive, 0);
    ret &= XaceRegisterCallback(XACE_CLIENT_ACCESS, SELinuxClient, 0);
    ret &= XaceRegisterCallback(XACE_EXT_ACCESS, SELinuxExtension, 0);
    ret &= XaceRegisterCallback(XACE_SERVER_ACCESS, SELinuxServer, 0);
//    ret &= XaceRegisterCallback(XACE_SELECTION_ACCESS, SELinuxSelection, 0);
    ret &= XaceRegisterCallback(XACE_SCREEN_ACCESS, SELinuxScreen, 0);
    ret &= XaceRegisterCallback(XACE_SCREENSAVER_ACCESS, SELinuxScreen, &ret);
    if (!ret)
	FatalError("XSELinux: Failed to register one or more callbacks\n");

    /* Add extension to server */
    extEntry = AddExtension(XSELINUX_EXTENSION_NAME,
			    XSELinuxNumberEvents, XSELinuxNumberErrors,
			    ProcSELinuxDispatch, ProcSELinuxDispatch,
			    SELinuxResetProc, StandardMinorOpcode);

    /* Label objects that were created before we could register ourself */
    SELinuxFixupLabels();
}
