/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $Xorg: tables.c,v 1.4 2001/02/09 02:04:41 xorgcvs Exp $ */

extern int    ProcInitialConnection(), ProcEstablishConnection();

extern int    ProcBadRequest(), ProcCreateWindow(),
    ProcChangeWindowAttributes(), ProcGetWindowAttributes(),
    ProcDestroyWindow(), ProcDestroySubwindows(), ProcChangeSaveSet(),
    ProcReparentWindow(), ProcMapWindow(), ProcMapSubwindows(),
    ProcUnmapWindow(), ProcUnmapSubwindows(), ProcConfigureWindow(),
    ProcCirculateWindow(), ProcGetGeometry(), ProcQueryTree(),
    ProcInternAtom(), ProcGetAtomName(), ProcChangeProperty(),
    ProcDeleteProperty(), ProcGetProperty(), ProcListProperties(),
    ProcSetSelectionOwner(), ProcGetSelectionOwner(), ProcConvertSelection(),
    ProcSendEvent(), ProcGrabPointer(), ProcUngrabPointer(),
    ProcGrabButton(), ProcUngrabButton(), ProcChangeActivePointerGrab(),
    ProcGrabKeyboard(), ProcUngrabKeyboard(), ProcGrabKey(),
    ProcUngrabKey(), ProcAllowEvents(), ProcGrabServer(),
    ProcUngrabServer(), ProcQueryPointer(), ProcGetMotionEvents(),
    ProcTranslateCoords(), ProcWarpPointer(), ProcSetInputFocus(),
    ProcGetInputFocus(), ProcQueryKeymap(), ProcOpenFont(),
    ProcCloseFont(), ProcQueryFont(), ProcQueryTextExtents(),
    ProcListFonts(), ProcListFontsWithInfo(), ProcSetFontPath(),
    ProcGetFontPath(), ProcCreatePixmap(), ProcFreePixmap(),
    ProcCreateGC(), ProcChangeGC(), ProcCopyGC(),
    ProcSetDashes(), ProcSetClipRectangles(), ProcFreeGC(),
    ProcClearToBackground(), ProcCopyArea(), ProcCopyPlane(),
    ProcPolyPoint(), ProcPolyLine(), ProcPolySegment(),
    ProcPolyRectangle(), ProcPolyArc(), ProcFillPoly(),
    ProcPolyFillRectangle(), ProcPolyFillArc(), ProcPutImage(),
    ProcGetImage(), ProcPolyText(),
    ProcImageText8(), ProcImageText16(), ProcCreateColormap(),
    ProcFreeColormap(), ProcCopyColormapAndFree(), ProcInstallColormap(),
    ProcUninstallColormap(), ProcListInstalledColormaps(), ProcAllocColor(),
    ProcAllocNamedColor(), ProcAllocColorCells(), ProcAllocColorPlanes(),
    ProcFreeColors(), ProcStoreColors(), ProcStoreNamedColor(),
    ProcQueryColors(), ProcLookupColor(), ProcCreateCursor(),
    ProcCreateGlyphCursor(), ProcFreeCursor(), ProcRecolorCursor(),
    ProcQueryBestSize(), ProcQueryExtension(), ProcListExtensions(),
    ProcChangeKeyboardMapping(), ProcSetPointerMapping(),
    ProcGetKeyboardMapping(), ProcGetPointerMapping(),
    ProcChangeKeyboardControl(),
    ProcGetKeyboardControl(), ProcBell(), ProcChangePointerControl(),
    ProcGetPointerControl(), ProcSetScreenSaver(), ProcGetScreenSaver(),
    ProcChangeHosts(), ProcListHosts(), ProcChangeAccessControl(),
    ProcChangeCloseDownMode(), ProcKillClient(),
    ProcRotateProperties(), ProcForceScreenSaver(),
    ProcSetModifierMapping(), ProcGetModifierMapping(),
    ProcNoOperation();

extern int    SProcSProcBadRequest(), SProcCreateWindow(),
    SProcChangeWindowAttributes(), 
    SProcReparentWindow(), SProcConfigureWindow(),
    SProcInternAtom(), SProcChangeProperty(),
    SProcDeleteProperty(), SProcGetProperty(),
    SProcSetSelectionOwner(),
    SProcConvertSelection(),
    SProcSendEvent(), SProcGrabPointer(),
    SProcGrabButton(), SProcUngrabButton(), SProcChangeActivePointerGrab(),
    SProcGrabKeyboard(), SProcGrabKey(),
    SProcUngrabKey(), SProcGetMotionEvents(),
    SProcTranslateCoords(), SProcWarpPointer(), SProcSetInputFocus(),
    SProcOpenFont(),
    SProcListFonts(), SProcListFontsWithInfo(), SProcSetFontPath(),
    SProcCreatePixmap(),
    SProcCreateGC(), SProcChangeGC(), SProcCopyGC(),
    SProcSetDashes(), SProcSetClipRectangles(),
    SProcClearToBackground(), SProcCopyArea(), SProcCopyPlane(),
    SProcPoly(), SProcFillPoly(), SProcPutImage(),
    SProcGetImage(), SProcPolyText(), 
    SProcImageText(), SProcCreateColormap(),
    SProcCopyColormapAndFree(), SProcAllocColor(),
    SProcAllocNamedColor(), SProcAllocColorCells(), SProcAllocColorPlanes(),
    SProcFreeColors(), SProcStoreColors(), SProcStoreNamedColor(),
    SProcQueryColors(), SProcLookupColor(), SProcCreateCursor(),
    SProcCreateGlyphCursor(), SProcRecolorCursor(),
    SProcQueryBestSize(), SProcQueryExtension(),
    SProcChangeKeyboardMapping(), SProcChangeKeyboardControl(),
    SProcChangePointerControl(),
    SProcSetScreenSaver(),
    SProcChangeHosts(),
    SProcRotateProperties(), 
    SProcNoOperation(), SProcResourceReq(), SProcSimpleReq();

extern void 
    SErrorEvent(), NotImplemented(), SKeyButtonPtrEvent(), SEnterLeaveEvent(),
    SFocusEvent(), SKeymapNotifyEvent(), SExposeEvent(),
    SGraphicsExposureEvent(), SNoExposureEvent(), SVisibilityEvent(),
    SCreateNotifyEvent(), SDestroyNotifyEvent(), SUnmapNotifyEvent(),
    SMapNotifyEvent(), SMapRequestEvent(), SReparentEvent(),
    SConfigureNotifyEvent(), SConfigureRequestEvent(), SGravityEvent(),
    SResizeRequestEvent(), SCirculateEvent(),
    SPropertyEvent(), SSelectionClearEvent(), SSelectionRequestEvent(),
    SSelectionNotifyEvent(), SColormapEvent(), SClientMessageEvent(), SMappingEvent();

extern void
    SGetWindowAttributesReply(), SGetGeometryReply(), SQueryTreeReply(),
    SInternAtomReply(), SGetAtomNameReply(), SGetPropertyReply(),
    SListPropertiesReply(), 
    SGetSelectionOwnerReply(),
    SQueryPointerReply(), SGetMotionEventsReply(), STranslateCoordsReply(),
    SGetInputFocusReply(), SQueryKeymapReply(), SQueryFontReply(),
    SQueryTextExtentsReply(), SListFontsReply(), SListFontsWithInfoReply(),
    SGetFontPathReply(), SGetImageReply(), SListInstalledColormapsReply(),
    SAllocColorReply(), SAllocNamedColorReply(), SAllocColorCellsReply(),
    SAllocColorPlanesReply(), SQueryColorsReply(), SLookupColorReply(),
    SQueryBestSizeReply(), SListExtensionsReply(),
    SGetKeyboardMappingReply(), SGetKeyboardControlReply(), 
    SGetPointerControlReply(), SGetScreenSaverReply(), 
    SListHostsReply(), SGetPointerMappingReply(),
    SGetModifierMappingReply(), SGenericReply();

#ifdef K5AUTH
extern int
    k5_stage1(), k5_stage2(), k5_stage3(), k5_bad();
#endif

int (* InitialVector[3]) () =
{
    0,
    ProcInitialConnection,
    ProcEstablishConnection
};

int (* ProcVector[256]) () =
{
    ProcBadRequest,
    ProcCreateWindow,
    ProcChangeWindowAttributes,
    ProcGetWindowAttributes,
    ProcDestroyWindow,
    ProcDestroySubwindows,		/* 5 */
    ProcChangeSaveSet,
    ProcReparentWindow,
    ProcMapWindow,
    ProcMapSubwindows,
    ProcUnmapWindow,			/* 10 */
    ProcUnmapSubwindows,
    ProcConfigureWindow,
    ProcCirculateWindow,
    ProcGetGeometry,
    ProcQueryTree,			/* 15 */
    ProcInternAtom,
    ProcGetAtomName,
    ProcChangeProperty,
    ProcDeleteProperty,
    ProcGetProperty,			/* 20 */
    ProcListProperties,
    ProcSetSelectionOwner,
    ProcGetSelectionOwner,
    ProcConvertSelection,
    ProcSendEvent,			/* 25 */
    ProcGrabPointer,
    ProcUngrabPointer,
    ProcGrabButton,
    ProcUngrabButton,
    ProcChangeActivePointerGrab,	/* 30 */
    ProcGrabKeyboard,
    ProcUngrabKeyboard,
    ProcGrabKey,
    ProcUngrabKey,
    ProcAllowEvents,			/* 35 */
    ProcGrabServer,
    ProcUngrabServer,
    ProcQueryPointer,
    ProcGetMotionEvents,
    ProcTranslateCoords,		/* 40 */
    ProcWarpPointer,
    ProcSetInputFocus,
    ProcGetInputFocus,
    ProcQueryKeymap,
    ProcOpenFont,			/* 45 */
    ProcCloseFont,
    ProcQueryFont,
    ProcQueryTextExtents,
    ProcListFonts,
    ProcListFontsWithInfo,		/* 50 */
    ProcSetFontPath,
    ProcGetFontPath,
    ProcCreatePixmap,
    ProcFreePixmap,
    ProcCreateGC,			/* 55 */
    ProcChangeGC,
    ProcCopyGC,
    ProcSetDashes,
    ProcSetClipRectangles,
    ProcFreeGC,				/* 60 */
    ProcClearToBackground,
    ProcCopyArea,
    ProcCopyPlane,
    ProcPolyPoint,
    ProcPolyLine,			/* 65 */
    ProcPolySegment,
    ProcPolyRectangle,
    ProcPolyArc,
    ProcFillPoly,
    ProcPolyFillRectangle,		/* 70 */
    ProcPolyFillArc,
    ProcPutImage,
    ProcGetImage,
    ProcPolyText,
    ProcPolyText,			/* 75 */
    ProcImageText8,
    ProcImageText16,
    ProcCreateColormap,
    ProcFreeColormap,
    ProcCopyColormapAndFree,		/* 80 */
    ProcInstallColormap,
    ProcUninstallColormap,
    ProcListInstalledColormaps,
    ProcAllocColor,
    ProcAllocNamedColor,		/* 85 */
    ProcAllocColorCells,
    ProcAllocColorPlanes,
    ProcFreeColors,
    ProcStoreColors,
    ProcStoreNamedColor,		/* 90 */
    ProcQueryColors,
    ProcLookupColor,
    ProcCreateCursor,
    ProcCreateGlyphCursor,
    ProcFreeCursor,			/* 95 */
    ProcRecolorCursor,
    ProcQueryBestSize,
    ProcQueryExtension,
    ProcListExtensions,
    ProcChangeKeyboardMapping,		/* 100 */
    ProcGetKeyboardMapping,
    ProcChangeKeyboardControl,
    ProcGetKeyboardControl,
    ProcBell,
    ProcChangePointerControl,		/* 105 */
    ProcGetPointerControl,
    ProcSetScreenSaver,
    ProcGetScreenSaver,
    ProcChangeHosts,
    ProcListHosts,			/* 110 */
    ProcChangeAccessControl,
    ProcChangeCloseDownMode,
    ProcKillClient,
    ProcRotateProperties,
    ProcForceScreenSaver,		/* 115 */
    ProcSetPointerMapping,
    ProcGetPointerMapping,
    ProcSetModifierMapping,
    ProcGetModifierMapping,
    0,					/* 120 */
    0,
    0,
    0,
    0,
    0,					/* 125 */
    0,
    ProcNoOperation    
};

int (* SwappedProcVector[256]) () =
{
    ProcBadRequest,
    SProcCreateWindow,
    SProcChangeWindowAttributes,
    SProcResourceReq,			/* GetWindowAttributes */
    SProcResourceReq,			/* DestroyWindow */
    SProcResourceReq,			/* 5 DestroySubwindows */
    SProcResourceReq,			/* SProcChangeSaveSet, */
    SProcReparentWindow,
    SProcResourceReq,			/* MapWindow */
    SProcResourceReq,			/* MapSubwindows */
    SProcResourceReq,			/* 10 UnmapWindow */
    SProcResourceReq,			/* UnmapSubwindows */
    SProcConfigureWindow,
    SProcResourceReq,			/* SProcCirculateWindow, */
    SProcResourceReq,			/* GetGeometry */
    SProcResourceReq,			/* 15 QueryTree */
    SProcInternAtom,
    SProcResourceReq,			/* SProcGetAtomName, */
    SProcChangeProperty,
    SProcDeleteProperty,
    SProcGetProperty,			/* 20 */
    SProcResourceReq,			/* SProcListProperties, */
    SProcSetSelectionOwner,
    SProcResourceReq, 			/* SProcGetSelectionOwner, */
    SProcConvertSelection,
    SProcSendEvent,			/* 25 */
    SProcGrabPointer,
    SProcResourceReq, 			/* SProcUngrabPointer, */
    SProcGrabButton,
    SProcUngrabButton,
    SProcChangeActivePointerGrab,	/* 30 */
    SProcGrabKeyboard,
    SProcResourceReq,			/* SProcUngrabKeyboard, */
    SProcGrabKey,
    SProcUngrabKey,
    SProcResourceReq,			/* 35 SProcAllowEvents, */
    SProcSimpleReq,			/* SProcGrabServer, */
    SProcSimpleReq,			/* SProcUngrabServer, */
    SProcResourceReq,			/* SProcQueryPointer, */
    SProcGetMotionEvents,
    SProcTranslateCoords,		/*40 */
    SProcWarpPointer,
    SProcSetInputFocus,
    SProcSimpleReq,			/* SProcGetInputFocus, */
    SProcSimpleReq,			/* QueryKeymap, */
    SProcOpenFont,			/* 45 */
    SProcResourceReq,			/* SProcCloseFont, */
    SProcResourceReq, 			/* SProcQueryFont, */
    SProcResourceReq,			/* SProcQueryTextExtents,  */
    SProcListFonts,
    SProcListFontsWithInfo,		/* 50 */
    SProcSetFontPath,
    SProcSimpleReq,			/* GetFontPath, */
    SProcCreatePixmap,
    SProcResourceReq,			/* SProcFreePixmap, */
    SProcCreateGC,			/* 55 */
    SProcChangeGC,
    SProcCopyGC,
    SProcSetDashes,
    SProcSetClipRectangles,
    SProcResourceReq,			/* 60 SProcFreeGC, */
    SProcClearToBackground,
    SProcCopyArea,
    SProcCopyPlane,
    SProcPoly,				/* PolyPoint, */
    SProcPoly,				/* 65 PolyLine */
    SProcPoly,				/* PolySegment, */
    SProcPoly,				/* PolyRectangle, */
    SProcPoly,				/* PolyArc, */
    SProcFillPoly,
    SProcPoly,				/* 70 PolyFillRectangle */
    SProcPoly,				/* PolyFillArc, */
    SProcPutImage,
    SProcGetImage,
    SProcPolyText,
    SProcPolyText,			/* 75 */
    SProcImageText,
    SProcImageText,
    SProcCreateColormap,
    SProcResourceReq,			/* SProcFreeColormap, */
    SProcCopyColormapAndFree,		/* 80 */
    SProcResourceReq,			/* SProcInstallColormap, */
    SProcResourceReq,			/* SProcUninstallColormap, */
    SProcResourceReq, 			/* SProcListInstalledColormaps, */
    SProcAllocColor,
    SProcAllocNamedColor,		/* 85 */
    SProcAllocColorCells,
    SProcAllocColorPlanes,
    SProcFreeColors,
    SProcStoreColors,
    SProcStoreNamedColor,		/* 90 */
    SProcQueryColors,
    SProcLookupColor,
    SProcCreateCursor,
    SProcCreateGlyphCursor,
    SProcResourceReq,			/* 95 SProcFreeCursor, */
    SProcRecolorCursor,
    SProcQueryBestSize,
    SProcQueryExtension,
    SProcSimpleReq,			/* ListExtensions, */
    SProcChangeKeyboardMapping,		/* 100 */
    SProcSimpleReq,			/* GetKeyboardMapping, */
    SProcChangeKeyboardControl,
    SProcSimpleReq,			/* GetKeyboardControl, */
    SProcSimpleReq,			/* Bell, */
    SProcChangePointerControl,		/* 105 */
    SProcSimpleReq,			/* GetPointerControl, */
    SProcSetScreenSaver,
    SProcSimpleReq,			/* GetScreenSaver, */
    SProcChangeHosts,
    SProcSimpleReq,			/* 110 ListHosts, */
    SProcSimpleReq,			/* SProcChangeAccessControl, */
    SProcSimpleReq,			/* SProcChangeCloseDownMode, */
    SProcResourceReq,			/* SProcKillClient, */
    SProcRotateProperties,
    SProcSimpleReq,			/* 115 ForceScreenSaver */
    SProcSimpleReq,			/* SetPointerMapping, */
    SProcSimpleReq,			/* GetPointerMapping, */
    SProcSimpleReq,			/* SetModifierMapping, */
    SProcSimpleReq,			/* GetModifierMapping, */
    0,					/* 120 */
    0,
    0,
    0,
    0,
    0,					/* 125 */
    0,
    SProcNoOperation
};

void (* EventSwapVector[128]) () =
{
    SErrorEvent,
    NotImplemented,
    SKeyButtonPtrEvent,
    SKeyButtonPtrEvent,
    SKeyButtonPtrEvent,
    SKeyButtonPtrEvent,			/* 5 */
    SKeyButtonPtrEvent,
    SEnterLeaveEvent,
    SEnterLeaveEvent,
    SFocusEvent,
    SFocusEvent,			/* 10 */
    SKeymapNotifyEvent,
    SExposeEvent,
    SGraphicsExposureEvent,
    SNoExposureEvent,
    SVisibilityEvent,			/* 15 */
    SCreateNotifyEvent,
    SDestroyNotifyEvent,
    SUnmapNotifyEvent,
    SMapNotifyEvent,
    SMapRequestEvent,			/* 20 */
    SReparentEvent,
    SConfigureNotifyEvent,
    SConfigureRequestEvent,
    SGravityEvent,
    SResizeRequestEvent,		/* 25 */
    SCirculateEvent,
    SCirculateEvent,
    SPropertyEvent,
    SSelectionClearEvent,
    SSelectionRequestEvent,		/* 30 */
    SSelectionNotifyEvent,
    SColormapEvent,
    SClientMessageEvent,
    SMappingEvent,
};


void (* ReplySwapVector[256]) () =
{
    NotImplemented,
    NotImplemented,
    NotImplemented,
    SGetWindowAttributesReply,
    NotImplemented,
    NotImplemented,			/* 5 */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 10 */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    SGetGeometryReply,
    SQueryTreeReply,			/* 15 */
    SInternAtomReply,
    SGetAtomNameReply,
    NotImplemented,
    NotImplemented,
    SGetPropertyReply,			/* 20 */
    SListPropertiesReply,
    NotImplemented,
    SGetSelectionOwnerReply,
    NotImplemented,
    NotImplemented,			/* 25 */
    SGenericReply,			/* SGrabPointerReply, */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 30 */
    SGenericReply,			/* SGrabKeyboardReply, */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 35 */
    NotImplemented,
    NotImplemented,
    SQueryPointerReply,
    SGetMotionEventsReply,
    STranslateCoordsReply,		/* 40 */
    NotImplemented,
    NotImplemented,
    SGetInputFocusReply,
    SQueryKeymapReply,
    NotImplemented,			/* 45 */
    NotImplemented,
    SQueryFontReply,
    SQueryTextExtentsReply,
    SListFontsReply,
    SListFontsWithInfoReply,		/* 50 */
    NotImplemented,
    SGetFontPathReply,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 55 */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 60 */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 65 */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 70 */
    NotImplemented,
    NotImplemented,
    SGetImageReply,
    NotImplemented,
    NotImplemented,			/* 75 */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 80 */
    NotImplemented,
    NotImplemented,
    SListInstalledColormapsReply,
    SAllocColorReply,
    SAllocNamedColorReply,		/* 85 */
    SAllocColorCellsReply,
    SAllocColorPlanesReply,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 90 */
    SQueryColorsReply,
    SLookupColorReply,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 95 */
    NotImplemented,
    SQueryBestSizeReply,
    SGenericReply,			/* SQueryExtensionReply, */
    SListExtensionsReply,
    NotImplemented,			/* 100 */
    SGetKeyboardMappingReply,
    NotImplemented,
    SGetKeyboardControlReply,
    NotImplemented,
    NotImplemented,			/* 105 */
    SGetPointerControlReply,
    NotImplemented,
    SGetScreenSaverReply,
    NotImplemented,
    SListHostsReply,			/* 110 */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 115 */
    SGenericReply,			/* SetPointerMapping */
    SGetPointerMappingReply,
    SGenericReply,			/* SetModifierMapping */
    SGetModifierMappingReply,		/* 119 */
    NotImplemented,			/* 120 */
    NotImplemented,			/* 121 */
    NotImplemented,			/* 122 */
    NotImplemented,			/* 123 */
    NotImplemented,			/* 124 */
    NotImplemented,			/* 125 */
    NotImplemented,			/* 126 */
    NotImplemented,			/* NoOperation */
    NotImplemented
};

#ifdef K5AUTH
int (*k5_Vector[256])() =
{
    k5_bad,
    k5_stage1,
    k5_bad,
    k5_stage3
};
#endif
