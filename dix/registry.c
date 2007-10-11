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
    RegisterRequestName(X_CreateWindow, 0, "CreateWindow");
    RegisterRequestName(X_ChangeWindowAttributes, 0, "ChangeWindowAttributes");
    RegisterRequestName(X_GetWindowAttributes, 0, "GetWindowAttributes");
    RegisterRequestName(X_DestroyWindow, 0, "DestroyWindow");
    RegisterRequestName(X_DestroySubwindows, 0, "DestroySubwindows");
    RegisterRequestName(X_ChangeSaveSet, 0, "ChangeSaveSet");
    RegisterRequestName(X_ReparentWindow, 0, "ReparentWindow");
    RegisterRequestName(X_MapWindow, 0, "MapWindow");
    RegisterRequestName(X_MapSubwindows, 0, "MapSubwindows");
    RegisterRequestName(X_UnmapWindow, 0, "UnmapWindow");
    RegisterRequestName(X_UnmapSubwindows, 0, "UnmapSubwindows");
    RegisterRequestName(X_ConfigureWindow, 0, "ConfigureWindow");
    RegisterRequestName(X_CirculateWindow, 0, "CirculateWindow");
    RegisterRequestName(X_GetGeometry, 0, "GetGeometry");
    RegisterRequestName(X_QueryTree, 0, "QueryTree");
    RegisterRequestName(X_InternAtom, 0, "InternAtom");
    RegisterRequestName(X_GetAtomName, 0, "GetAtomName");
    RegisterRequestName(X_ChangeProperty, 0, "ChangeProperty");
    RegisterRequestName(X_DeleteProperty, 0, "DeleteProperty");
    RegisterRequestName(X_GetProperty, 0, "GetProperty");
    RegisterRequestName(X_ListProperties, 0, "ListProperties");
    RegisterRequestName(X_SetSelectionOwner, 0, "SetSelectionOwner");
    RegisterRequestName(X_GetSelectionOwner, 0, "GetSelectionOwner");
    RegisterRequestName(X_ConvertSelection, 0, "ConvertSelection");
    RegisterRequestName(X_SendEvent, 0, "SendEvent");
    RegisterRequestName(X_GrabPointer, 0, "GrabPointer");
    RegisterRequestName(X_UngrabPointer, 0, "UngrabPointer");
    RegisterRequestName(X_GrabButton, 0, "GrabButton");
    RegisterRequestName(X_UngrabButton, 0, "UngrabButton");
    RegisterRequestName(X_ChangeActivePointerGrab, 0, "ChangeActivePointerGrab");
    RegisterRequestName(X_GrabKeyboard, 0, "GrabKeyboard");
    RegisterRequestName(X_UngrabKeyboard, 0, "UngrabKeyboard");
    RegisterRequestName(X_GrabKey, 0, "GrabKey");
    RegisterRequestName(X_UngrabKey, 0, "UngrabKey");
    RegisterRequestName(X_AllowEvents, 0, "AllowEvents");
    RegisterRequestName(X_GrabServer, 0, "GrabServer");
    RegisterRequestName(X_UngrabServer, 0, "UngrabServer");
    RegisterRequestName(X_QueryPointer, 0, "QueryPointer");
    RegisterRequestName(X_GetMotionEvents, 0, "GetMotionEvents");
    RegisterRequestName(X_TranslateCoords, 0, "TranslateCoords");
    RegisterRequestName(X_WarpPointer, 0, "WarpPointer");
    RegisterRequestName(X_SetInputFocus, 0, "SetInputFocus");
    RegisterRequestName(X_GetInputFocus, 0, "GetInputFocus");
    RegisterRequestName(X_QueryKeymap, 0, "QueryKeymap");
    RegisterRequestName(X_OpenFont, 0, "OpenFont");
    RegisterRequestName(X_CloseFont, 0, "CloseFont");
    RegisterRequestName(X_QueryFont, 0, "QueryFont");
    RegisterRequestName(X_QueryTextExtents, 0, "QueryTextExtents");
    RegisterRequestName(X_ListFonts, 0, "ListFonts");
    RegisterRequestName(X_ListFontsWithInfo, 0, "ListFontsWithInfo");
    RegisterRequestName(X_SetFontPath, 0, "SetFontPath");
    RegisterRequestName(X_GetFontPath, 0, "GetFontPath");
    RegisterRequestName(X_CreatePixmap, 0, "CreatePixmap");
    RegisterRequestName(X_FreePixmap, 0, "FreePixmap");
    RegisterRequestName(X_CreateGC, 0, "CreateGC");
    RegisterRequestName(X_ChangeGC, 0, "ChangeGC");
    RegisterRequestName(X_CopyGC, 0, "CopyGC");
    RegisterRequestName(X_SetDashes, 0, "SetDashes");
    RegisterRequestName(X_SetClipRectangles, 0, "SetClipRectangles");
    RegisterRequestName(X_FreeGC, 0, "FreeGC");
    RegisterRequestName(X_ClearArea, 0, "ClearArea");
    RegisterRequestName(X_CopyArea, 0, "CopyArea");
    RegisterRequestName(X_CopyPlane, 0, "CopyPlane");
    RegisterRequestName(X_PolyPoint, 0, "PolyPoint");
    RegisterRequestName(X_PolyLine, 0, "PolyLine");
    RegisterRequestName(X_PolySegment, 0, "PolySegment");
    RegisterRequestName(X_PolyRectangle, 0, "PolyRectangle");
    RegisterRequestName(X_PolyArc, 0, "PolyArc");
    RegisterRequestName(X_FillPoly, 0, "FillPoly");
    RegisterRequestName(X_PolyFillRectangle, 0, "PolyFillRectangle");
    RegisterRequestName(X_PolyFillArc, 0, "PolyFillArc");
    RegisterRequestName(X_PutImage, 0, "PutImage");
    RegisterRequestName(X_GetImage, 0, "GetImage");
    RegisterRequestName(X_PolyText8, 0, "PolyText8");
    RegisterRequestName(X_PolyText16, 0, "PolyText16");
    RegisterRequestName(X_ImageText8, 0, "ImageText8");
    RegisterRequestName(X_ImageText16, 0, "ImageText16");
    RegisterRequestName(X_CreateColormap, 0, "CreateColormap");
    RegisterRequestName(X_FreeColormap, 0, "FreeColormap");
    RegisterRequestName(X_CopyColormapAndFree, 0, "CopyColormapAndFree");
    RegisterRequestName(X_InstallColormap, 0, "InstallColormap");
    RegisterRequestName(X_UninstallColormap, 0, "UninstallColormap");
    RegisterRequestName(X_ListInstalledColormaps, 0, "ListInstalledColormaps");
    RegisterRequestName(X_AllocColor, 0, "AllocColor");
    RegisterRequestName(X_AllocNamedColor, 0, "AllocNamedColor");
    RegisterRequestName(X_AllocColorCells, 0, "AllocColorCells");
    RegisterRequestName(X_AllocColorPlanes, 0, "AllocColorPlanes");
    RegisterRequestName(X_FreeColors, 0, "FreeColors");
    RegisterRequestName(X_StoreColors, 0, "StoreColors");
    RegisterRequestName(X_StoreNamedColor, 0, "StoreNamedColor");
    RegisterRequestName(X_QueryColors, 0, "QueryColors");
    RegisterRequestName(X_LookupColor, 0, "LookupColor");
    RegisterRequestName(X_CreateCursor, 0, "CreateCursor");
    RegisterRequestName(X_CreateGlyphCursor, 0, "CreateGlyphCursor");
    RegisterRequestName(X_FreeCursor, 0, "FreeCursor");
    RegisterRequestName(X_RecolorCursor, 0, "RecolorCursor");
    RegisterRequestName(X_QueryBestSize, 0, "QueryBestSize");
    RegisterRequestName(X_QueryExtension, 0, "QueryExtension");
    RegisterRequestName(X_ListExtensions, 0, "ListExtensions");
    RegisterRequestName(X_ChangeKeyboardMapping, 0, "ChangeKeyboardMapping");
    RegisterRequestName(X_GetKeyboardMapping, 0, "GetKeyboardMapping");
    RegisterRequestName(X_ChangeKeyboardControl, 0, "ChangeKeyboardControl");
    RegisterRequestName(X_GetKeyboardControl, 0, "GetKeyboardControl");
    RegisterRequestName(X_Bell, 0, "Bell");
    RegisterRequestName(X_ChangePointerControl, 0, "ChangePointerControl");
    RegisterRequestName(X_GetPointerControl, 0, "GetPointerControl");
    RegisterRequestName(X_SetScreenSaver, 0, "SetScreenSaver");
    RegisterRequestName(X_GetScreenSaver, 0, "GetScreenSaver");
    RegisterRequestName(X_ChangeHosts, 0, "ChangeHosts");
    RegisterRequestName(X_ListHosts, 0, "ListHosts");
    RegisterRequestName(X_SetAccessControl, 0, "SetAccessControl");
    RegisterRequestName(X_SetCloseDownMode, 0, "SetCloseDownMode");
    RegisterRequestName(X_KillClient, 0, "KillClient");
    RegisterRequestName(X_RotateProperties, 0, "RotateProperties");
    RegisterRequestName(X_ForceScreenSaver, 0, "ForceScreenSaver");
    RegisterRequestName(X_SetPointerMapping, 0, "SetPointerMapping");
    RegisterRequestName(X_GetPointerMapping, 0, "GetPointerMapping");
    RegisterRequestName(X_SetModifierMapping, 0, "SetModifierMapping");
    RegisterRequestName(X_GetModifierMapping, 0, "GetModifierMapping");
    RegisterRequestName(X_NoOperation, 0, "NoOperation");

    RegisterErrorName(Success, "Success");
    RegisterErrorName(BadRequest, "BadRequest");
    RegisterErrorName(BadValue, "BadValue");
    RegisterErrorName(BadWindow, "BadWindow");
    RegisterErrorName(BadPixmap, "BadPixmap");
    RegisterErrorName(BadAtom, "BadAtom");
    RegisterErrorName(BadCursor, "BadCursor");
    RegisterErrorName(BadFont, "BadFont");
    RegisterErrorName(BadMatch, "BadMatch");
    RegisterErrorName(BadDrawable, "BadDrawable");
    RegisterErrorName(BadAccess, "BadAccess");
    RegisterErrorName(BadAlloc, "BadAlloc");
    RegisterErrorName(BadColor, "BadColor");
    RegisterErrorName(BadGC, "BadGC");
    RegisterErrorName(BadIDChoice, "BadIDChoice");
    RegisterErrorName(BadName, "BadName");
    RegisterErrorName(BadLength, "BadLength");
    RegisterErrorName(BadImplementation, "BadImplementation");

    RegisterEventName(X_Error, "Error");
    RegisterEventName(X_Reply, "Reply");
    RegisterEventName(KeyPress, "KeyPress");
    RegisterEventName(KeyRelease, "KeyRelease");
    RegisterEventName(ButtonPress, "ButtonPress");
    RegisterEventName(ButtonRelease, "ButtonRelease");
    RegisterEventName(MotionNotify, "MotionNotify");
    RegisterEventName(EnterNotify, "EnterNotify");
    RegisterEventName(LeaveNotify, "LeaveNotify");
    RegisterEventName(FocusIn, "FocusIn");
    RegisterEventName(FocusOut, "FocusOut");
    RegisterEventName(KeymapNotify, "KeymapNotify");
    RegisterEventName(Expose, "Expose");
    RegisterEventName(GraphicsExpose, "GraphicsExpose");
    RegisterEventName(NoExpose, "NoExpose");
    RegisterEventName(VisibilityNotify, "VisibilityNotify");
    RegisterEventName(CreateNotify, "CreateNotify");
    RegisterEventName(DestroyNotify, "DestroyNotify");
    RegisterEventName(UnmapNotify, "UnmapNotify");
    RegisterEventName(MapNotify, "MapNotify");
    RegisterEventName(MapRequest, "MapRequest");
    RegisterEventName(ReparentNotify, "ReparentNotify");
    RegisterEventName(ConfigureNotify, "ConfigureNotify");
    RegisterEventName(ConfigureRequest, "ConfigureRequest");
    RegisterEventName(GravityNotify, "GravityNotify");
    RegisterEventName(ResizeRequest, "ResizeRequest");
    RegisterEventName(CirculateNotify, "CirculateNotify");
    RegisterEventName(CirculateRequest, "CirculateRequest");
    RegisterEventName(PropertyNotify, "PropertyNotify");
    RegisterEventName(SelectionClear, "SelectionClear");
    RegisterEventName(SelectionRequest, "SelectionRequest");
    RegisterEventName(SelectionNotify, "SelectionNotify");
    RegisterEventName(ColormapNotify, "ColormapNotify");
    RegisterEventName(ClientMessage, "ClientMessage");
    RegisterEventName(MappingNotify, "MappingNotify");
}

#endif /* XREGISTRY */
