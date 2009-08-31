/************************************************************

Author: Eamon Walsh <ewalsh@tycho.nsa.gov>

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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <selinux/label.h>

#include "registry.h"
#include "xselinuxint.h"

/* selection and property atom cache */
typedef struct {
    SELinuxObjectRec prp;
    SELinuxObjectRec sel;
} SELinuxAtomRec;

/* labeling handle */
static struct selabel_handle *label_hnd;

/* Array of object classes indexed by resource type */
static security_class_t *knownTypes;
static unsigned numKnownTypes;

/* Array of event SIDs indexed by event type */
static security_id_t *knownEvents;
static unsigned numKnownEvents;

/* Array of property and selection SID structures */
static SELinuxAtomRec *knownAtoms;
static unsigned numKnownAtoms;


/*
 * Looks up a name in the selection or property mappings
 */
static int
SELinuxAtomToSIDLookup(Atom atom, SELinuxObjectRec *obj, int map, int polymap)
{
    const char *name = NameForAtom(atom);
    security_context_t ctx;
    int rc = Success;

    obj->poly = 1;

    /* Look in the mappings of names to contexts */
    if (selabel_lookup_raw(label_hnd, &ctx, name, map) == 0) {
	obj->poly = 0;
    } else if (errno != ENOENT) {
	ErrorF("SELinux: a property label lookup failed!\n");
	return BadValue;
    } else if (selabel_lookup_raw(label_hnd, &ctx, name, polymap) < 0) {
	ErrorF("SELinux: a property label lookup failed!\n");
	return BadValue;
    }

    /* Get a SID for context */
    if (avc_context_to_sid_raw(ctx, &obj->sid) < 0) {
	ErrorF("SELinux: a context_to_SID_raw call failed!\n");
	rc = BadAlloc;
    }

    freecon(ctx);
    return rc;
}

/*
 * Looks up the SID corresponding to the given property or selection atom
 */
int
SELinuxAtomToSID(Atom atom, int prop, SELinuxObjectRec **obj_rtn)
{
    SELinuxObjectRec *obj;
    int rc, map, polymap;

    if (atom >= numKnownAtoms) {
	/* Need to increase size of atoms array */
	unsigned size = sizeof(SELinuxAtomRec);
	knownAtoms = xrealloc(knownAtoms, (atom + 1) * size);
	if (!knownAtoms)
	    return BadAlloc;
	memset(knownAtoms + numKnownAtoms, 0,
	       (atom - numKnownAtoms + 1) * size);
	numKnownAtoms = atom + 1;
    }

    if (prop) {
	obj = &knownAtoms[atom].prp;
	map = SELABEL_X_PROP;
	polymap = SELABEL_X_POLYPROP;
    } else {
	obj = &knownAtoms[atom].sel;
	map = SELABEL_X_SELN;
	polymap = SELABEL_X_POLYSELN;
    }

    if (!obj->sid) {
	rc = SELinuxAtomToSIDLookup(atom, obj, map, polymap);
	if (rc != Success)
	    goto out;
    }

    *obj_rtn = obj;
    rc = Success;
out:
    return rc;
}

/*
 * Looks up a SID for a selection/subject pair
 */
int
SELinuxSelectionToSID(Atom selection, SELinuxSubjectRec *subj,
		      security_id_t *sid_rtn, int *poly_rtn)
{
    int rc;
    SELinuxObjectRec *obj;
    security_id_t tsid;

    /* Get the default context and polyinstantiation bit */
    rc = SELinuxAtomToSID(selection, 0, &obj);
    if (rc != Success)
	return rc;

    /* Check for an override context next */
    if (subj->sel_use_sid) {
	sidget(tsid = subj->sel_use_sid);
	goto out;
    }

    sidget(tsid = obj->sid);

    /* Polyinstantiate if necessary to obtain the final SID */
    if (obj->poly) {
	sidput(tsid);
	if (avc_compute_member(subj->sid, obj->sid,
			       SECCLASS_X_SELECTION, &tsid) < 0) {
	    ErrorF("SELinux: a compute_member call failed!\n");
	    return BadValue;
	}
    }
out:
    *sid_rtn = tsid;
    if (poly_rtn)
	*poly_rtn = obj->poly;
    return Success;
}

/*
 * Looks up a SID for a property/subject pair
 */
int
SELinuxPropertyToSID(Atom property, SELinuxSubjectRec *subj,
		     security_id_t *sid_rtn, int *poly_rtn)
{
    int rc;
    SELinuxObjectRec *obj;
    security_id_t tsid, tsid2;

    /* Get the default context and polyinstantiation bit */
    rc = SELinuxAtomToSID(property, 1, &obj);
    if (rc != Success)
	return rc;

    /* Check for an override context next */
    if (subj->prp_use_sid) {
	sidget(tsid = subj->prp_use_sid);
	goto out;
    }

    /* Perform a transition */
    if (avc_compute_create(subj->sid, obj->sid,
			   SECCLASS_X_PROPERTY, &tsid) < 0) {
	ErrorF("SELinux: a compute_create call failed!\n");
	return BadValue;
    }

    /* Polyinstantiate if necessary to obtain the final SID */
    if (obj->poly) {
	tsid2 = tsid;
	if (avc_compute_member(subj->sid, tsid2,
			       SECCLASS_X_PROPERTY, &tsid) < 0) {
	    ErrorF("SELinux: a compute_member call failed!\n");
	    sidput(tsid2);
	    return BadValue;
	}
	sidput(tsid2);
    }
out:
    *sid_rtn = tsid;
    if (poly_rtn)
	*poly_rtn = obj->poly;
    return Success;
}

/*
 * Looks up the SID corresponding to the given event type
 */
int
SELinuxEventToSID(unsigned type, security_id_t sid_of_window,
		  SELinuxObjectRec *sid_return)
{
    const char *name = LookupEventName(type);
    security_context_t ctx;
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
	if (selabel_lookup_raw(label_hnd, &ctx, name, SELABEL_X_EVENT) < 0) {
	    ErrorF("SELinux: an event label lookup failed!\n");
	    return BadValue;
	}
	/* Get a SID for context */
	if (avc_context_to_sid_raw(ctx, knownEvents + type) < 0) {
	    ErrorF("SELinux: a context_to_SID_raw call failed!\n");
	    return BadAlloc;
	}
	freecon(ctx);
    }

    /* Perform a transition to obtain the final SID */
    if (avc_compute_create(sid_of_window, knownEvents[type], SECCLASS_X_EVENT,
			   &sid_return->sid) < 0) {
	ErrorF("SELinux: a compute_create call failed!\n");
	return BadValue;
    }

    return Success;
}

int
SELinuxExtensionToSID(const char *name, security_id_t *sid_rtn)
{
    security_context_t ctx;

    /* Look in the mappings of extension names to contexts */
    if (selabel_lookup_raw(label_hnd, &ctx, name, SELABEL_X_EXT) < 0) {
	ErrorF("SELinux: a property label lookup failed!\n");
	return BadValue;
    }
    /* Get a SID for context */
    if (avc_context_to_sid_raw(ctx, sid_rtn) < 0) {
	ErrorF("SELinux: a context_to_SID_raw call failed!\n");
	freecon(ctx);
	return BadAlloc;
    }
    freecon(ctx);
    return Success;
}

/*
 * Returns the object class corresponding to the given resource type.
 */
security_class_t
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

security_context_t
SELinuxDefaultClientLabel(void)
{
    security_context_t ctx;

    if (selabel_lookup_raw(label_hnd, &ctx, "remote", SELABEL_X_CLIENT) < 0)
	FatalError("SELinux: failed to look up remote-client context\n");

    return ctx;
}

void
SELinuxLabelInit(void)
{
    struct selinux_opt selabel_option = { SELABEL_OPT_VALIDATE, (char *)1 };

    label_hnd = selabel_open(SELABEL_CTX_X, &selabel_option, 1);
    if (!label_hnd)
	FatalError("SELinux: Failed to open x_contexts mapping in policy\n");
}

void
SELinuxLabelReset(void)
{
    selabel_close(label_hnd);
    label_hnd = NULL;

    /* Free local state */
    xfree(knownAtoms);
    knownAtoms = NULL;
    numKnownAtoms = 0;

    xfree(knownEvents);
    knownEvents = NULL;
    numKnownEvents = 0;

    xfree(knownTypes);
    knownTypes = NULL;
    numKnownTypes = 0;
}
