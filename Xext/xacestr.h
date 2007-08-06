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

#ifndef _XACESTR_H
#define _XACESTR_H

#include "dixstruct.h"
#include "resource.h"
#include "extnsionst.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "inputstr.h"
#include "propertyst.h"
#include "selection.h"
#include "xace.h"

/* XACE_CORE_DISPATCH */
typedef struct {
    ClientPtr client;
    int status;
} XaceCoreDispatchRec;

/* XACE_RESOURCE_ACCESS */
typedef struct {
    ClientPtr client;
    XID id;
    RESTYPE rtype;
    Mask access_mode;
    pointer res;
    int status;
} XaceResourceAccessRec;

/* XACE_DEVICE_ACCESS */
typedef struct {
    ClientPtr client;
    DeviceIntPtr dev;
    Bool fromRequest;
    int status;
} XaceDeviceAccessRec;

/* XACE_PROPERTY_ACCESS */
typedef struct {
    ClientPtr client;
    WindowPtr pWin;
    PropertyPtr pProp;
    Mask access_mode;
    int status;
} XacePropertyAccessRec;

/* XACE_DRAWABLE_ACCESS */
typedef struct {
    ClientPtr client;
    DrawablePtr pDraw;
    int status;
} XaceDrawableAccessRec;

/* XACE_MAP_ACCESS */
/* XACE_BACKGRND_ACCESS */
typedef struct {
    ClientPtr client;
    WindowPtr pWin;
    int status;
} XaceMapAccessRec;

/* XACE_EXT_DISPATCH */
/* XACE_EXT_ACCESS */
typedef struct {
    ClientPtr client;
    ExtensionEntry *ext;
    int status;
} XaceExtAccessRec;

/* XACE_HOSTLIST_ACCESS */
typedef struct {
    ClientPtr client;
    Mask access_mode;
    int status;
} XaceHostlistAccessRec;

/* XACE_SELECTION_ACCESS */
typedef struct {
    ClientPtr client;
    Selection *selection;
    Mask access_mode;
    int status;
} XaceSelectionAccessRec;

typedef struct {
    ClientPtr client;
    ScreenPtr screen;
    Mask access_mode;
    int status;
} XaceScreenAccessRec;

/* XACE_AUTH_AVAIL */
typedef struct {
    ClientPtr client;
    XID authId;
} XaceAuthAvailRec;

/* XACE_KEY_AVAIL */
typedef struct {
    xEventPtr event;
    DeviceIntPtr keybd;
    int count;
} XaceKeyAvailRec;

/* XACE_AUDIT_BEGIN */
/* XACE_AUDIT_END */
typedef struct {
    ClientPtr client;
    int requestResult;
} XaceAuditRec;

#endif /* _XACESTR_H */
