/* $Xorg: lbxzerorep.c,v 1.4 2001/02/09 02:05:17 xorgcvs Exp $ */

/*

Copyright 1996, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/* $XFree86: xc/programs/Xserver/lbx/lbxzerorep.c,v 1.4 2001/12/14 20:00:02 dawes Exp $ */

/*
 * This module handles zeroing out unused pad bytes in core X replies.
 * This will hopefully improve both stream and delta compression,
 * since we are removing the random values in pad bytes.
 */

#define NEED_REPLIES
#include "X.h"
#include <X11/Xproto.h>

void
ZeroReplyPadBytes (char *buf,
		   int reqType)
{
    switch (reqType) {
    case X_GetWindowAttributes:
    {
	xGetWindowAttributesReply *reply = (xGetWindowAttributesReply *) buf;
	reply->pad = 0;
	break;
    }
    case X_GetGeometry:
    {
	xGetGeometryReply *reply = (xGetGeometryReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	break;
    }
    case X_QueryTree:
    {
	xQueryTreeReply *reply = (xQueryTreeReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	break;
    }
    case X_InternAtom:
    {
	xInternAtomReply *reply = (xInternAtomReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	break;
    }
    case X_GetAtomName:
    {
	xGetAtomNameReply *reply = (xGetAtomNameReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	reply->pad7 = 0;
	break;
    }
    case X_GetProperty:
    {
	xGetPropertyReply *reply = (xGetPropertyReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	break;
    }
    case X_ListProperties:
    {
	xListPropertiesReply *reply = (xListPropertiesReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	reply->pad7 = 0;
	break;
    }
    case X_GetSelectionOwner:
    {
	xGetSelectionOwnerReply *reply = (xGetSelectionOwnerReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	break;
    }
    case X_GrabKeyboard:
    case X_GrabPointer:
    {
	xGrabKeyboardReply *reply = (xGrabKeyboardReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	break;
    }
    case X_QueryPointer:
    {
	xQueryPointerReply *reply = (xQueryPointerReply *) buf;
	reply->pad1 = 0;
	reply->pad = 0;
	break;
    }
    case X_GetMotionEvents:
    {
	xGetMotionEventsReply *reply = (xGetMotionEventsReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	break;
    }
    case X_TranslateCoords:
    {
	xTranslateCoordsReply *reply = (xTranslateCoordsReply *) buf;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	break;
    }
    case X_GetInputFocus:
    {
	xGetInputFocusReply *reply = (xGetInputFocusReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	break;
    }
    case X_QueryKeymap:
    {
	xQueryKeymapReply *reply = (xQueryKeymapReply *) buf;
	reply->pad1 = 0;
	break;
    }
    case X_QueryFont:
    {
	xQueryFontReply *reply = (xQueryFontReply *) buf;
	reply->pad1 = 0;
	break;
    }
    case X_QueryTextExtents:
    {
	xQueryTextExtentsReply *reply = (xQueryTextExtentsReply *) buf;
	reply->pad = 0;
	break;
    }
    case X_ListFonts:
    {
	xListFontsReply *reply = (xListFontsReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	reply->pad7 = 0;
	break;
    }
    case X_GetFontPath:
    {
	xGetFontPathReply *reply = (xGetFontPathReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	reply->pad7 = 0;
	break;
    }
    case X_GetImage:
    {
	xGetImageReply *reply = (xGetImageReply *) buf;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	reply->pad7 = 0;
	break;
    }
    case X_ListInstalledColormaps:
    {
	xListInstalledColormapsReply *reply =
		(xListInstalledColormapsReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	reply->pad7 = 0;
	break;
    }
    case X_AllocColor:
    {
	xAllocColorReply *reply = (xAllocColorReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	break;
    }
    case X_AllocNamedColor:
    {
	xAllocNamedColorReply *reply = (xAllocNamedColorReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	break;
    }
    case X_AllocColorCells:
    {
	xAllocColorCellsReply *reply = (xAllocColorCellsReply *) buf;
	reply->pad1 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	reply->pad7 = 0;
	break;
    }
    case X_AllocColorPlanes:
    {
	xAllocColorPlanesReply *reply = (xAllocColorPlanesReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	break;
    }
    case X_QueryColors:
    {
	xQueryColorsReply *reply = (xQueryColorsReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	reply->pad7 = 0;
	break;
    }
    case X_LookupColor:
    {
	xLookupColorReply *reply = (xLookupColorReply *) buf;
	reply->pad1 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	break;
    }
    case X_QueryBestSize:
    {
	xQueryBestSizeReply *reply = (xQueryBestSizeReply *) buf;
	reply->pad1 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	reply->pad7 = 0;
	break;
    }
    case X_QueryExtension:
    {
	xQueryExtensionReply *reply = (xQueryExtensionReply *) buf;
	reply->pad1 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	reply->pad7 = 0;
	break;
    }
    case X_ListExtensions:
    {
	xListExtensionsReply *reply = (xListExtensionsReply *) buf;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	reply->pad7 = 0;
	break;
    }
    case X_SetPointerMapping:
    case X_SetModifierMapping:
    {
	xSetMappingReply *reply = (xSetMappingReply *) buf;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	reply->pad7 = 0;
	break;
    }
    case X_GetPointerMapping:
    {
	xGetPointerMappingReply *reply = (xGetPointerMappingReply *) buf;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	reply->pad7 = 0;
	break;
    }
    case X_GetKeyboardMapping:
    {
	xGetKeyboardMappingReply *reply = (xGetKeyboardMappingReply *) buf;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	reply->pad7 = 0;
	break;
    }
    case X_GetModifierMapping:
    {
	xGetModifierMappingReply *reply = (xGetModifierMappingReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	break;
    }
    case X_GetKeyboardControl:
    {
	xGetKeyboardControlReply *reply = (xGetKeyboardControlReply *) buf;
	reply->pad = 0;
	break;
    }
    case X_GetPointerControl:
    {
	xGetPointerControlReply *reply = (xGetPointerControlReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	break;
    }
    case X_GetScreenSaver:
    {
	xGetScreenSaverReply *reply = (xGetScreenSaverReply *) buf;
	reply->pad1 = 0;
	reply->pad2 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	break;
    }
    case X_ListHosts:
    {
	xListHostsReply *reply = (xListHostsReply *) buf;
	reply->pad1 = 0;
	reply->pad3 = 0;
	reply->pad4 = 0;
	reply->pad5 = 0;
	reply->pad6 = 0;
	reply->pad7 = 0;
	break;
    }
    }
}
