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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef XREGISTRY

#include <X11/X.h>
#include <X11/Xproto.h>
#include "resource.h"
#include "registry.h"

#define BASE_SIZE 16
#define CORE "X11:"

static const char ***requests, **events, **errors, **resources;
static unsigned nmajor, *nminor, nevent, nerror, nresource;

static int double_size(void *p, unsigned n, unsigned size)
{
    char **ptr = (char **)p;
    unsigned s, f;

    if (n) {
	s = n * size;
	n *= 2 * size;
	f = n;
    } else {
	s = 0;
	n = f = BASE_SIZE * size;
    }

    *ptr = xrealloc(*ptr, n);
    if (!*ptr) {
	dixResetRegistry();
	return FALSE;
    }
    memset(*ptr + s, 0, f - s);
    return TRUE;
}       

/*
 * Registration functions
 */

void
RegisterRequestName(unsigned major, unsigned minor, const char *name)
{
    while (major >= nmajor) {
	if (!double_size(&requests, nmajor, sizeof(const char **)))
	    return;
	if (!double_size(&nminor, nmajor, sizeof(unsigned)))
	    return;
	nmajor = nmajor ? nmajor * 2 : BASE_SIZE;
    }
    while (minor >= nminor[major]) {
	if (!double_size(requests+major, nminor[major], sizeof(const char *)))
	    return;
	nminor[major] = nminor[major] ? nminor[major] * 2 : BASE_SIZE;
    }

    requests[major][minor] = name;
}

void
RegisterEventName(unsigned event, const char *name) {
    while (event >= nevent) {
	if (!double_size(&events, nevent, sizeof(const char *)))
	    return;
	nevent = nevent ? nevent * 2 : BASE_SIZE;
    }

    events[event] = name;
}

void
RegisterErrorName(unsigned error, const char *name) {
    while (error >= nerror) {
	if (!double_size(&errors, nerror, sizeof(const char *)))
	    return;
	nerror = nerror ? nerror * 2 : BASE_SIZE;
    }

    errors[error] = name;
}

void
RegisterResourceName(RESTYPE resource, const char *name)
{
    resource &= TypeMask;

    while (resource >= nresource) {
	if (!double_size(&resources, nresource, sizeof(const char *)))
	    return;
	nresource = nresource ? nresource * 2 : BASE_SIZE;
    }

    resources[resource] = name;
}

/*
 * Lookup functions
 */

const char *
LookupRequestName(int major, int minor)
{
    if (major >= nmajor)
	return XREGISTRY_UNKNOWN;
    if (minor >= nminor[major])
	return XREGISTRY_UNKNOWN;

    return requests[major][minor] ? requests[major][minor] : XREGISTRY_UNKNOWN;
}

const char *
LookupEventName(int event)
{
    event &= 127;
    if (event >= nevent)
	return XREGISTRY_UNKNOWN;

    return events[event] ? events[event] : XREGISTRY_UNKNOWN;
}

const char *
LookupErrorName(int error)
{
    if (error >= nerror)
	return XREGISTRY_UNKNOWN;

    return errors[error] ? errors[error] : XREGISTRY_UNKNOWN;
}

const char *
LookupResourceName(RESTYPE resource)
{
    resource &= TypeMask;
    if (resource >= nresource)
	return XREGISTRY_UNKNOWN;

    return resources[resource] ? resources[resource] : XREGISTRY_UNKNOWN;
}

/*
 * Setup and teardown
 */
void
dixResetRegistry(void)
{
    /* Free all memory */
    while (nmajor)
	xfree(requests[--nmajor]);
    xfree(requests);
    xfree(nminor);
    xfree(events);
    xfree(errors);
    xfree(resources);

    requests = NULL;
    nminor = NULL;
    events = NULL;
    errors = NULL;
    resources = NULL;

    nmajor = nevent = nerror = nresource = 0;

    /* Add built-in resources */
    RegisterResourceName(RT_NONE, "NONE");
    RegisterResourceName(RT_WINDOW, "WINDOW");
    RegisterResourceName(RT_PIXMAP, "PIXMAP");
    RegisterResourceName(RT_GC, "GC");
    RegisterResourceName(RT_FONT, "FONT");
    RegisterResourceName(RT_CURSOR, "CURSOR");
    RegisterResourceName(RT_COLORMAP, "COLORMAP");
    RegisterResourceName(RT_CMAPENTRY, "COLORMAP ENTRY");
    RegisterResourceName(RT_OTHERCLIENT, "OTHER CLIENT");
    RegisterResourceName(RT_PASSIVEGRAB, "PASSIVE GRAB");

    /* Add the core protocol */
    RegisterRequestName(X_CreateWindow, 0, CORE "CreateWindow");
    RegisterRequestName(X_ChangeWindowAttributes, 0, CORE "ChangeWindowAttributes");
    RegisterRequestName(X_GetWindowAttributes, 0, CORE "GetWindowAttributes");
    RegisterRequestName(X_DestroyWindow, 0, CORE "DestroyWindow");
    RegisterRequestName(X_DestroySubwindows, 0, CORE "DestroySubwindows");
    RegisterRequestName(X_ChangeSaveSet, 0, CORE "ChangeSaveSet");
    RegisterRequestName(X_ReparentWindow, 0, CORE "ReparentWindow");
    RegisterRequestName(X_MapWindow, 0, CORE "MapWindow");
    RegisterRequestName(X_MapSubwindows, 0, CORE "MapSubwindows");
    RegisterRequestName(X_UnmapWindow, 0, CORE "UnmapWindow");
    RegisterRequestName(X_UnmapSubwindows, 0, CORE "UnmapSubwindows");
    RegisterRequestName(X_ConfigureWindow, 0, CORE "ConfigureWindow");
    RegisterRequestName(X_CirculateWindow, 0, CORE "CirculateWindow");
    RegisterRequestName(X_GetGeometry, 0, CORE "GetGeometry");
    RegisterRequestName(X_QueryTree, 0, CORE "QueryTree");
    RegisterRequestName(X_InternAtom, 0, CORE "InternAtom");
    RegisterRequestName(X_GetAtomName, 0, CORE "GetAtomName");
    RegisterRequestName(X_ChangeProperty, 0, CORE "ChangeProperty");
    RegisterRequestName(X_DeleteProperty, 0, CORE "DeleteProperty");
    RegisterRequestName(X_GetProperty, 0, CORE "GetProperty");
    RegisterRequestName(X_ListProperties, 0, CORE "ListProperties");
    RegisterRequestName(X_SetSelectionOwner, 0, CORE "SetSelectionOwner");
    RegisterRequestName(X_GetSelectionOwner, 0, CORE "GetSelectionOwner");
    RegisterRequestName(X_ConvertSelection, 0, CORE "ConvertSelection");
    RegisterRequestName(X_SendEvent, 0, CORE "SendEvent");
    RegisterRequestName(X_GrabPointer, 0, CORE "GrabPointer");
    RegisterRequestName(X_UngrabPointer, 0, CORE "UngrabPointer");
    RegisterRequestName(X_GrabButton, 0, CORE "GrabButton");
    RegisterRequestName(X_UngrabButton, 0, CORE "UngrabButton");
    RegisterRequestName(X_ChangeActivePointerGrab, 0, CORE "ChangeActivePointerGrab");
    RegisterRequestName(X_GrabKeyboard, 0, CORE "GrabKeyboard");
    RegisterRequestName(X_UngrabKeyboard, 0, CORE "UngrabKeyboard");
    RegisterRequestName(X_GrabKey, 0, CORE "GrabKey");
    RegisterRequestName(X_UngrabKey, 0, CORE "UngrabKey");
    RegisterRequestName(X_AllowEvents, 0, CORE "AllowEvents");
    RegisterRequestName(X_GrabServer, 0, CORE "GrabServer");
    RegisterRequestName(X_UngrabServer, 0, CORE "UngrabServer");
    RegisterRequestName(X_QueryPointer, 0, CORE "QueryPointer");
    RegisterRequestName(X_GetMotionEvents, 0, CORE "GetMotionEvents");
    RegisterRequestName(X_TranslateCoords, 0, CORE "TranslateCoords");
    RegisterRequestName(X_WarpPointer, 0, CORE "WarpPointer");
    RegisterRequestName(X_SetInputFocus, 0, CORE "SetInputFocus");
    RegisterRequestName(X_GetInputFocus, 0, CORE "GetInputFocus");
    RegisterRequestName(X_QueryKeymap, 0, CORE "QueryKeymap");
    RegisterRequestName(X_OpenFont, 0, CORE "OpenFont");
    RegisterRequestName(X_CloseFont, 0, CORE "CloseFont");
    RegisterRequestName(X_QueryFont, 0, CORE "QueryFont");
    RegisterRequestName(X_QueryTextExtents, 0, CORE "QueryTextExtents");
    RegisterRequestName(X_ListFonts, 0, CORE "ListFonts");
    RegisterRequestName(X_ListFontsWithInfo, 0, CORE "ListFontsWithInfo");
    RegisterRequestName(X_SetFontPath, 0, CORE "SetFontPath");
    RegisterRequestName(X_GetFontPath, 0, CORE "GetFontPath");
    RegisterRequestName(X_CreatePixmap, 0, CORE "CreatePixmap");
    RegisterRequestName(X_FreePixmap, 0, CORE "FreePixmap");
    RegisterRequestName(X_CreateGC, 0, CORE "CreateGC");
    RegisterRequestName(X_ChangeGC, 0, CORE "ChangeGC");
    RegisterRequestName(X_CopyGC, 0, CORE "CopyGC");
    RegisterRequestName(X_SetDashes, 0, CORE "SetDashes");
    RegisterRequestName(X_SetClipRectangles, 0, CORE "SetClipRectangles");
    RegisterRequestName(X_FreeGC, 0, CORE "FreeGC");
    RegisterRequestName(X_ClearArea, 0, CORE "ClearArea");
    RegisterRequestName(X_CopyArea, 0, CORE "CopyArea");
    RegisterRequestName(X_CopyPlane, 0, CORE "CopyPlane");
    RegisterRequestName(X_PolyPoint, 0, CORE "PolyPoint");
    RegisterRequestName(X_PolyLine, 0, CORE "PolyLine");
    RegisterRequestName(X_PolySegment, 0, CORE "PolySegment");
    RegisterRequestName(X_PolyRectangle, 0, CORE "PolyRectangle");
    RegisterRequestName(X_PolyArc, 0, CORE "PolyArc");
    RegisterRequestName(X_FillPoly, 0, CORE "FillPoly");
    RegisterRequestName(X_PolyFillRectangle, 0, CORE "PolyFillRectangle");
    RegisterRequestName(X_PolyFillArc, 0, CORE "PolyFillArc");
    RegisterRequestName(X_PutImage, 0, CORE "PutImage");
    RegisterRequestName(X_GetImage, 0, CORE "GetImage");
    RegisterRequestName(X_PolyText8, 0, CORE "PolyText8");
    RegisterRequestName(X_PolyText16, 0, CORE "PolyText16");
    RegisterRequestName(X_ImageText8, 0, CORE "ImageText8");
    RegisterRequestName(X_ImageText16, 0, CORE "ImageText16");
    RegisterRequestName(X_CreateColormap, 0, CORE "CreateColormap");
    RegisterRequestName(X_FreeColormap, 0, CORE "FreeColormap");
    RegisterRequestName(X_CopyColormapAndFree, 0, CORE "CopyColormapAndFree");
    RegisterRequestName(X_InstallColormap, 0, CORE "InstallColormap");
    RegisterRequestName(X_UninstallColormap, 0, CORE "UninstallColormap");
    RegisterRequestName(X_ListInstalledColormaps, 0, CORE "ListInstalledColormaps");
    RegisterRequestName(X_AllocColor, 0, CORE "AllocColor");
    RegisterRequestName(X_AllocNamedColor, 0, CORE "AllocNamedColor");
    RegisterRequestName(X_AllocColorCells, 0, CORE "AllocColorCells");
    RegisterRequestName(X_AllocColorPlanes, 0, CORE "AllocColorPlanes");
    RegisterRequestName(X_FreeColors, 0, CORE "FreeColors");
    RegisterRequestName(X_StoreColors, 0, CORE "StoreColors");
    RegisterRequestName(X_StoreNamedColor, 0, CORE "StoreNamedColor");
    RegisterRequestName(X_QueryColors, 0, CORE "QueryColors");
    RegisterRequestName(X_LookupColor, 0, CORE "LookupColor");
    RegisterRequestName(X_CreateCursor, 0, CORE "CreateCursor");
    RegisterRequestName(X_CreateGlyphCursor, 0, CORE "CreateGlyphCursor");
    RegisterRequestName(X_FreeCursor, 0, CORE "FreeCursor");
    RegisterRequestName(X_RecolorCursor, 0, CORE "RecolorCursor");
    RegisterRequestName(X_QueryBestSize, 0, CORE "QueryBestSize");
    RegisterRequestName(X_QueryExtension, 0, CORE "QueryExtension");
    RegisterRequestName(X_ListExtensions, 0, CORE "ListExtensions");
    RegisterRequestName(X_ChangeKeyboardMapping, 0, CORE "ChangeKeyboardMapping");
    RegisterRequestName(X_GetKeyboardMapping, 0, CORE "GetKeyboardMapping");
    RegisterRequestName(X_ChangeKeyboardControl, 0, CORE "ChangeKeyboardControl");
    RegisterRequestName(X_GetKeyboardControl, 0, CORE "GetKeyboardControl");
    RegisterRequestName(X_Bell, 0, CORE "Bell");
    RegisterRequestName(X_ChangePointerControl, 0, CORE "ChangePointerControl");
    RegisterRequestName(X_GetPointerControl, 0, CORE "GetPointerControl");
    RegisterRequestName(X_SetScreenSaver, 0, CORE "SetScreenSaver");
    RegisterRequestName(X_GetScreenSaver, 0, CORE "GetScreenSaver");
    RegisterRequestName(X_ChangeHosts, 0, CORE "ChangeHosts");
    RegisterRequestName(X_ListHosts, 0, CORE "ListHosts");
    RegisterRequestName(X_SetAccessControl, 0, CORE "SetAccessControl");
    RegisterRequestName(X_SetCloseDownMode, 0, CORE "SetCloseDownMode");
    RegisterRequestName(X_KillClient, 0, CORE "KillClient");
    RegisterRequestName(X_RotateProperties, 0, CORE "RotateProperties");
    RegisterRequestName(X_ForceScreenSaver, 0, CORE "ForceScreenSaver");
    RegisterRequestName(X_SetPointerMapping, 0, CORE "SetPointerMapping");
    RegisterRequestName(X_GetPointerMapping, 0, CORE "GetPointerMapping");
    RegisterRequestName(X_SetModifierMapping, 0, CORE "SetModifierMapping");
    RegisterRequestName(X_GetModifierMapping, 0, CORE "GetModifierMapping");
    RegisterRequestName(X_NoOperation, 0, CORE "NoOperation");

    RegisterErrorName(Success, CORE "Success");
    RegisterErrorName(BadRequest, CORE "BadRequest");
    RegisterErrorName(BadValue, CORE "BadValue");
    RegisterErrorName(BadWindow, CORE "BadWindow");
    RegisterErrorName(BadPixmap, CORE "BadPixmap");
    RegisterErrorName(BadAtom, CORE "BadAtom");
    RegisterErrorName(BadCursor, CORE "BadCursor");
    RegisterErrorName(BadFont, CORE "BadFont");
    RegisterErrorName(BadMatch, CORE "BadMatch");
    RegisterErrorName(BadDrawable, CORE "BadDrawable");
    RegisterErrorName(BadAccess, CORE "BadAccess");
    RegisterErrorName(BadAlloc, CORE "BadAlloc");
    RegisterErrorName(BadColor, CORE "BadColor");
    RegisterErrorName(BadGC, CORE "BadGC");
    RegisterErrorName(BadIDChoice, CORE "BadIDChoice");
    RegisterErrorName(BadName, CORE "BadName");
    RegisterErrorName(BadLength, CORE "BadLength");
    RegisterErrorName(BadImplementation, CORE "BadImplementation");

    RegisterEventName(X_Error, CORE "Error");
    RegisterEventName(X_Reply, CORE "Reply");
    RegisterEventName(KeyPress, CORE "KeyPress");
    RegisterEventName(KeyRelease, CORE "KeyRelease");
    RegisterEventName(ButtonPress, CORE "ButtonPress");
    RegisterEventName(ButtonRelease, CORE "ButtonRelease");
    RegisterEventName(MotionNotify, CORE "MotionNotify");
    RegisterEventName(EnterNotify, CORE "EnterNotify");
    RegisterEventName(LeaveNotify, CORE "LeaveNotify");
    RegisterEventName(FocusIn, CORE "FocusIn");
    RegisterEventName(FocusOut, CORE "FocusOut");
    RegisterEventName(KeymapNotify, CORE "KeymapNotify");
    RegisterEventName(Expose, CORE "Expose");
    RegisterEventName(GraphicsExpose, CORE "GraphicsExpose");
    RegisterEventName(NoExpose, CORE "NoExpose");
    RegisterEventName(VisibilityNotify, CORE "VisibilityNotify");
    RegisterEventName(CreateNotify, CORE "CreateNotify");
    RegisterEventName(DestroyNotify, CORE "DestroyNotify");
    RegisterEventName(UnmapNotify, CORE "UnmapNotify");
    RegisterEventName(MapNotify, CORE "MapNotify");
    RegisterEventName(MapRequest, CORE "MapRequest");
    RegisterEventName(ReparentNotify, CORE "ReparentNotify");
    RegisterEventName(ConfigureNotify, CORE "ConfigureNotify");
    RegisterEventName(ConfigureRequest, CORE "ConfigureRequest");
    RegisterEventName(GravityNotify, CORE "GravityNotify");
    RegisterEventName(ResizeRequest, CORE "ResizeRequest");
    RegisterEventName(CirculateNotify, CORE "CirculateNotify");
    RegisterEventName(CirculateRequest, CORE "CirculateRequest");
    RegisterEventName(PropertyNotify, CORE "PropertyNotify");
    RegisterEventName(SelectionClear, CORE "SelectionClear");
    RegisterEventName(SelectionRequest, CORE "SelectionRequest");
    RegisterEventName(SelectionNotify, CORE "SelectionNotify");
    RegisterEventName(ColormapNotify, CORE "ColormapNotify");
    RegisterEventName(ClientMessage, CORE "ClientMessage");
    RegisterEventName(MappingNotify, CORE "MappingNotify");
}

#endif /* XREGISTRY */
