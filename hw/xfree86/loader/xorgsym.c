/*
 * Copyright 1995-1998 by Metro Link, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Metro Link, Inc. not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Metro Link, Inc. makes no
 * representations about the suitability of this software for any purpose.
 *  It is provided "as is" without express or implied warranty.
 *
 * METRO LINK, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL METRO LINK, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s),
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "colormap.h"
#include "cursor.h"
#include "cursorstr.h"
#include "dix.h"
#include "dixevents.h"
#include "dixstruct.h"
#include "misc.h"
#include "globals.h"
#include "os.h"
#include "osdep.h"
#include "privates.h"
#include "resource.h"
#include "registry.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "extension.h"
#define EXTENSION_PROC_ARGS void *
#include "extnsionst.h"
#include "swaprep.h"
#include "swapreq.h"
#include "inputstr.h"
#include <X11/extensions/XIproto.h>
#include "exevents.h"
#include "extinit.h"
#ifdef XV
#include "xvmodproc.h"
#endif
#include "dgaproc.h"
#ifdef RENDER
#include "mipict.h"
#include "renderedge.h"
#endif
#include "selection.h"
#ifdef XKB
#include <xkbsrv.h>
#endif
#ifdef PANORAMIX
#include "panoramiX.h"
#include "panoramiXsrv.h"
#endif
#include "sleepuntil.h"

#include "misc.h"
#include "mi.h"
#include "mibank.h"
#include "miwideline.h"
#include "mibstore.h"
#include "mipointer.h"
#include "migc.h"
#include "miline.h"
#include "mizerarc.h"
#include "mifillarc.h"
#include "micmap.h"
#include "mioverlay.h"
#ifdef COMPOSITE
#include "cw.h"
#endif
#ifdef DAMAGE
#include "damage.h"
#endif

#ifdef HAS_SHM
# include "shmint.h"
#endif

#include "loaderProcs.h"
#include "xf86Pci.h"
#include "xf86.h"
#include "xf86Resources.h"
#include "xf86_OSproc.h"
#include "xf86Parser.h"
#include "xf86Config.h"
#include "xf86Xinput.h"
#ifdef XV
#include "xf86xv.h"
#include "xf86xvmc.h"
#endif
#include "xf86cmap.h"
#include "xf86fbman.h"
#include "dgaproc.h"

#ifdef DPMSExtension
#include "dpmsproc.h"
#endif
#include "vidmodeproc.h"
#include "loader.h"
#include "xisb.h"
#include "vbe.h"
#ifndef __OpenBSD__
#include "xf86sbusBus.h"
#endif
#include "compiler.h"
#include "xf86Crtc.h"
#include "xf86Modes.h"

#ifdef RANDR
#include "xf86RandR12.h"
#endif
#include "xf86DDC.h"
#include "edid.h"
#include "xf86Cursor.h"
#include "xf86RamDac.h"
#include "BT.h"
#include "IBM.h"
#include "TI.h"

#include "xf86RamDac.h"

#define SYMBOL(symbol)			&symbol

_X_HIDDEN void *xorgLookupTab[] = {
    SYMBOL(MakeAtom),
    SYMBOL(NameForAtom),
    SYMBOL(ValidAtom),
    SYMBOL(AllocColor),
    SYMBOL(CreateColormap),
    SYMBOL(FakeAllocColor),
    SYMBOL(FakeFreeColor),
    SYMBOL(FreeColors),
    SYMBOL(StoreColors),
    SYMBOL(TellLostMap),
    SYMBOL(TellGainedMap),
    SYMBOL(QueryColors),
    SYMBOL(FreeCursor),
    SYMBOL(LookupClient),
    SYMBOL(LookupDrawable),
    SYMBOL(LookupWindow),
    SYMBOL(SecurityLookupDrawable),
    SYMBOL(SecurityLookupWindow),
    SYMBOL(LookupIDByType),
    SYMBOL(LookupIDByClass),
    SYMBOL(SecurityLookupIDByClass),
    SYMBOL(SecurityLookupIDByType),
    SYMBOL(Ones),
    SYMBOL(InitButtonClassDeviceStruct),
    SYMBOL(InitFocusClassDeviceStruct),
    SYMBOL(InitLedFeedbackClassDeviceStruct),
    SYMBOL(InitPtrFeedbackClassDeviceStruct),
    SYMBOL(InitKbdFeedbackClassDeviceStruct),
    SYMBOL(InitIntegerFeedbackClassDeviceStruct),
    SYMBOL(InitStringFeedbackClassDeviceStruct),
    SYMBOL(InitBellFeedbackClassDeviceStruct),
    SYMBOL(InitValuatorClassDeviceStruct),
    SYMBOL(InitKeyClassDeviceStruct),
    SYMBOL(InitKeyboardDeviceStruct),
    SYMBOL(SendMappingNotify),
    SYMBOL(InitPointerDeviceStruct),
    SYMBOL(SendErrorToClient),
    SYMBOL(UpdateCurrentTime),
    SYMBOL(UpdateCurrentTimeIf),
    SYMBOL(ProcBadRequest),
    SYMBOL(dispatchException),
    SYMBOL(isItTimeToYield),
    SYMBOL(ClientStateCallback),
    SYMBOL(ServerGrabCallback),
    SYMBOL(AddCallback),
    SYMBOL(ClientSleep),
    SYMBOL(ClientTimeToServerTime),
    SYMBOL(ClientWakeup),
    SYMBOL(CompareTimeStamps),
    SYMBOL(CopyISOLatin1Lowered),
    SYMBOL(DeleteCallback),
    SYMBOL(dixLookupDrawable),
    SYMBOL(dixLookupWindow),
    SYMBOL(dixLookupClient),
    SYMBOL(dixLookupGC),
    SYMBOL(NoopDDA),
    SYMBOL(QueueWorkProc),
    SYMBOL(RegisterBlockAndWakeupHandlers),
    SYMBOL(RemoveBlockAndWakeupHandlers),
    SYMBOL(CheckCursorConfinement),
    SYMBOL(DeliverEvents),
    SYMBOL(NewCurrentScreen),
    SYMBOL(PointerConfinedToScreen),
    SYMBOL(TryClientEvents),
    SYMBOL(WriteEventsToClient),
    SYMBOL(GetCurrentRootWindow),
    SYMBOL(GetSpritePosition),
    SYMBOL(GetSpriteWindow),
    SYMBOL(GetSpriteCursor),
    SYMBOL(DeviceEventCallback),
    SYMBOL(EventCallback),
    SYMBOL(inputInfo),
    SYMBOL(SetCriticalEvent),
#ifdef PANORAMIX
    SYMBOL(XineramaGetCursorScreen),
#endif
    SYMBOL(dixLookupProperty),
    SYMBOL(ChangeWindowProperty),
    SYMBOL(dixChangeWindowProperty),
    SYMBOL(dixLookupSelection),
    SYMBOL(CurrentSelections),
    SYMBOL(AddExtension),
    SYMBOL(AddExtensionAlias),
    SYMBOL(CheckExtension),
    SYMBOL(MinorOpcodeOfRequest),
    SYMBOL(StandardMinorOpcode),
    SYMBOL(CopyGC),
    SYMBOL(CreateGC),
    SYMBOL(CreateScratchGC),
    SYMBOL(ChangeGC),
    SYMBOL(dixChangeGC),
    SYMBOL(DoChangeGC),
    SYMBOL(FreeGC),
    SYMBOL(FreeScratchGC),
    SYMBOL(GetScratchGC),
    SYMBOL(ValidateGC),
    SYMBOL(VerifyRectOrder),
    SYMBOL(ScreenSaverTime),
#ifdef DPMSExtension
    SYMBOL(DPMSEnabled),
    SYMBOL(DPMSCapableFlag),
    SYMBOL(DPMSOffTime),
    SYMBOL(DPMSPowerLevel),
    SYMBOL(DPMSStandbyTime),
    SYMBOL(DPMSSuspendTime),
    SYMBOL(DPMSEnabledSwitch),
    SYMBOL(DPMSDisabledSwitch),
#endif
#ifdef XV
    SYMBOL(XvScreenInitProc),
    SYMBOL(XvGetScreenKeyProc),
    SYMBOL(XvGetRTPortProc),
    SYMBOL(XvMCScreenInitProc),
#endif
    SYMBOL(ScreenSaverBlanking),
    SYMBOL(WindowTable),
    SYMBOL(clients),
    SYMBOL(currentMaxClients),
    SYMBOL(currentTime),
    SYMBOL(defaultColorVisualClass),
    SYMBOL(display),
    SYMBOL(globalSerialNumber),
    SYMBOL(lastDeviceEventTime),
    SYMBOL(monitorResolution),
    SYMBOL(screenInfo),
    SYMBOL(serverClient),
    SYMBOL(serverGeneration),
    SYMBOL(NotImplemented),
    SYMBOL(PixmapWidthPaddingInfo),
    SYMBOL(AllocatePixmap),
    SYMBOL(GetScratchPixmapHeader),
    SYMBOL(FreeScratchPixmapHeader),
    SYMBOL(dixRequestPrivate),
    SYMBOL(dixRegisterPrivateInitFunc),
    SYMBOL(dixRegisterPrivateDeleteFunc),
    SYMBOL(dixAllocatePrivate),
    SYMBOL(dixLookupPrivate),
    SYMBOL(dixLookupPrivateAddr),
    SYMBOL(dixSetPrivate),
    SYMBOL(dixFreePrivates),
    SYMBOL(dixRegisterPrivateOffset),
    SYMBOL(dixLookupPrivateOffset),
    SYMBOL(AddResource),
    SYMBOL(ChangeResourceValue),
    SYMBOL(CreateNewResourceClass),
    SYMBOL(CreateNewResourceType),
    SYMBOL(dixLookupResource),
    SYMBOL(FakeClientID),
    SYMBOL(FreeResource),
    SYMBOL(FreeResourceByType),
    SYMBOL(LegalNewID),
    SYMBOL(FindClientResourcesByType),
    SYMBOL(FindAllClientResources),
    SYMBOL(lastResourceType),
    SYMBOL(TypeMask),
    SYMBOL(ResourceStateCallback),
#ifdef XREGISTRY
    SYMBOL(RegisterResourceName),
    SYMBOL(LookupMajorName),
    SYMBOL(LookupRequestName),
    SYMBOL(LookupEventName),
    SYMBOL(LookupErrorName),
    SYMBOL(LookupResourceName),
#endif
    SYMBOL(CopySwap32Write),
    SYMBOL(Swap32Write),
    SYMBOL(SwapConnSetupInfo),
    SYMBOL(SwapConnSetupPrefix),
    SYMBOL(SwapShorts),
    SYMBOL(SwapLongs),
    SYMBOL(SwapColorItem),
    SYMBOL(EventSwapVector),
    SYMBOL(ChangeWindowAttributes),
    SYMBOL(CheckWindowOptionalNeed),
    SYMBOL(CreateUnclippedWinSize),
    SYMBOL(CreateWindow),
    SYMBOL(FindWindowWithOptional),
    SYMBOL(GravityTranslate),
    SYMBOL(MakeWindowOptional),
    SYMBOL(MapWindow),
    SYMBOL(NotClippedByChildren),
    SYMBOL(SaveScreens),
    SYMBOL(dixSaveScreens),
    SYMBOL(TraverseTree),
    SYMBOL(UnmapWindow),
    SYMBOL(WalkTree),
    SYMBOL(savedScreenInfo),
    SYMBOL(screenIsSaved),
    SYMBOL(LocalClient),
    SYMBOL(Xstrdup),
    SYMBOL(XNFstrdup),
    SYMBOL(AdjustWaitForDelay),
    SYMBOL(noTestExtensions),
    SYMBOL(GiveUp),
#ifdef COMPOSITE
    SYMBOL(noCompositeExtension),
#endif
#ifdef DAMAGE
    SYMBOL(noDamageExtension),
#endif
#ifdef DBE
    SYMBOL(noDbeExtension),
#endif
#ifdef DPMSExtension
    SYMBOL(noDPMSExtension),
#endif
#ifdef GLXEXT
    SYMBOL(noGlxExtension),
#endif
#ifdef SCREENSAVER
    SYMBOL(noScreenSaverExtension),
#endif
#ifdef MITSHM
    SYMBOL(noMITShmExtension),
#endif
#ifdef MULTIBUFFER
    SYMBOL(noMultibufferExtension),
#endif
#ifdef RANDR
    SYMBOL(noRRExtension),
#endif
#ifdef RENDER
    SYMBOL(noRenderExtension),
#endif
#ifdef XCSECURITY
    SYMBOL(noSecurityExtension),
#endif
#ifdef RES
    SYMBOL(noResExtension),
#endif
#ifdef XF86BIGFONT
    SYMBOL(noXFree86BigfontExtension),
#endif
#ifdef XFreeXDGA
    SYMBOL(noXFree86DGAExtension),
#endif
#ifdef XF86DRI
    SYMBOL(noXFree86DRIExtension),
#endif
#ifdef XF86VIDMODE
    SYMBOL(noXFree86VidModeExtension),
#endif
#ifdef XFIXES
    SYMBOL(noXFixesExtension),
#endif
#ifdef XKB
    SYMBOL(noXkbExtension),
#endif
#ifdef PANORAMIX
    SYMBOL(noPanoramiXExtension),
#endif
#ifdef XSELINUX
    SYMBOL(noSELinuxExtension),
#endif
#ifdef XV
    SYMBOL(noXvExtension),
#endif
    SYMBOL(LogVWrite),
    SYMBOL(LogWrite),
    SYMBOL(LogVMessageVerb),
    SYMBOL(LogMessageVerb),
    SYMBOL(LogMessage),
    SYMBOL(FatalError),
    SYMBOL(VErrorF),
    SYMBOL(ErrorF),
    SYMBOL(Error),
    SYMBOL(XNFalloc),
    SYMBOL(XNFcalloc),
    SYMBOL(XNFrealloc),
    SYMBOL(Xalloc),
    SYMBOL(Xcalloc),
    SYMBOL(Xfree),
    SYMBOL(Xrealloc),
    SYMBOL(TimerFree),
    SYMBOL(TimerSet),
    SYMBOL(TimerCancel),
    SYMBOL(WriteToClient),
    SYMBOL(SetCriticalOutputPending),
    SYMBOL(FlushCallback),
    SYMBOL(ReplyCallback),
    SYMBOL(ResetCurrentRequest),
    SYMBOL(IgnoreClient),
    SYMBOL(AttendClient),
    SYMBOL(AddEnabledDevice),
    SYMBOL(RemoveEnabledDevice),
    SYMBOL(GrabInProgress),
#ifdef XKB
    SYMBOL(XkbInitKeyboardDeviceStruct),
    SYMBOL(XkbSetRulesDflts),
    SYMBOL(XkbDfltRepeatDelay),
    SYMBOL(XkbDfltRepeatInterval),
#endif
    SYMBOL(InitValuatorAxisStruct),
    SYMBOL(InitProximityClassDeviceStruct),
    SYMBOL(XDGAEventBase),
#ifdef RENDER
    SYMBOL(PictureInit),
    SYMBOL(PictureTransformPoint),
    SYMBOL(PictureTransformPoint3d),
    SYMBOL(PictureGetSubpixelOrder),
    SYMBOL(PictureSetSubpixelOrder),
    SYMBOL(PictureScreenPrivateKey),
    SYMBOL(miPictureInit),
    SYMBOL(miComputeCompositeRegion),
    SYMBOL(miGlyphs),
    SYMBOL(miCompositeRects),
    SYMBOL(PictureAddFilter),
    SYMBOL(PictureSetFilterAlias),
    SYMBOL(RenderSampleCeilY),
    SYMBOL(RenderSampleFloorY),
    SYMBOL(RenderEdgeStep),
    SYMBOL(RenderEdgeInit),
    SYMBOL(RenderLineFixedEdgeInit),
#endif
    SYMBOL(ClientSleepUntil),
#ifdef HAS_SHM
    SYMBOL(ShmCompletionCode),
    SYMBOL(BadShmSegCode),
    SYMBOL(ShmSegType),
#endif
#ifdef PANORAMIX
    SYMBOL(XineramaRegisterConnectionBlockCallback),
    SYMBOL(XineramaDeleteResource),
    SYMBOL(PanoramiXNumScreens),
    SYMBOL(panoramiXdataPtr),
    SYMBOL(XRT_WINDOW),
    SYMBOL(XRT_PIXMAP),
    SYMBOL(XRT_GC),
    SYMBOL(XRT_COLORMAP),
    SYMBOL(XRC_DRAWABLE),
#endif
    SYMBOL(miChangeClip),
    SYMBOL(miChangeGC),
    SYMBOL(miClearDrawable),
    SYMBOL(miClearToBackground),
    SYMBOL(miClearVisualTypes),
    SYMBOL(miClipSpans),
    SYMBOL(miComputeCompositeClip),
    SYMBOL(miCopyClip),
    SYMBOL(miCopyGC),
    SYMBOL(miCreateDefColormap),
    SYMBOL(miCreateScreenResources),
    SYMBOL(miDCInitialize),
    SYMBOL(miDestroyClip),
    SYMBOL(miDestroyGC),
    SYMBOL(miExpandDirectColors),
    SYMBOL(miFillArcSetup),
    SYMBOL(miFillArcSliceSetup),
    SYMBOL(miFillPolygon),
    SYMBOL(miGetDefaultVisualMask),
    SYMBOL(miHandleExposures),
    SYMBOL(miImageGlyphBlt),
    SYMBOL(miImageText16),
    SYMBOL(miImageText8),
    SYMBOL(miInitOverlay),
    SYMBOL(miInitVisuals),
    SYMBOL(miInitializeBackingStore),
    SYMBOL(miInitializeBanking),
    SYMBOL(miInitializeColormap),
    SYMBOL(miInstallColormap),
    SYMBOL(miIntersect),
    SYMBOL(miInverse),
    SYMBOL(miListInstalledColormaps),
    SYMBOL(miModifyPixmapHeader),
    SYMBOL(miOverlayCollectUnderlayRegions),
    SYMBOL(miOverlayComputeCompositeClip),
    SYMBOL(miOverlayCopyUnderlay),
    SYMBOL(miOverlayGetPrivateClips),
    SYMBOL(miOverlaySetRootClip),
    SYMBOL(miOverlaySetTransFunction),
    SYMBOL(miPointInRegion),
    SYMBOL(miPointerAbsoluteCursor),
    SYMBOL(miPointerCurrentScreen),
    SYMBOL(miPointerInitialize),
    SYMBOL(miPointerWarpCursor),
    SYMBOL(miPolyArc),
    SYMBOL(miPolyBuildEdge),
    SYMBOL(miPolyBuildPoly),
    SYMBOL(miPolyFillArc),
    SYMBOL(miPolyFillRect),
    SYMBOL(miPolyGlyphBlt),
    SYMBOL(miPolyPoint),
    SYMBOL(miPolyRectangle),
    SYMBOL(miPolySegment),
    SYMBOL(miPolyText16),
    SYMBOL(miPolyText8),
    SYMBOL(miRectAlloc),
    SYMBOL(miRectIn),
    SYMBOL(miRectsToRegion),
    SYMBOL(miRegionAppend),
    SYMBOL(miRegionCopy),
    SYMBOL(miRegionCreate),
    SYMBOL(miRegionDestroy),
    SYMBOL(miRegionEmpty),
    SYMBOL(miRegionEqual),
    SYMBOL(miRegionExtents),
    SYMBOL(miRegionInit),
    SYMBOL(miRegionNotEmpty),
    SYMBOL(miRegionReset),
    SYMBOL(miRegionUninit),
    SYMBOL(miRegionValidate),
    SYMBOL(miResolveColor),
    SYMBOL(miRoundCapClip),
    SYMBOL(miRoundJoinClip),
    SYMBOL(miScreenInit),
    SYMBOL(miSegregateChildren),
    SYMBOL(miSetPixmapDepths),
    SYMBOL(miSetVisualTypes),
    SYMBOL(miSetVisualTypesAndMasks),
    SYMBOL(miSetZeroLineBias),
    SYMBOL(miSubtract),
    SYMBOL(miTranslateRegion),
    SYMBOL(miUninstallColormap),
    SYMBOL(miUnion),
    SYMBOL(miWideDash),
    SYMBOL(miWideLine),
    SYMBOL(miWindowExposures),
    SYMBOL(miZeroArcSetup),
    SYMBOL(miZeroClipLine),
    SYMBOL(miZeroLine),
    SYMBOL(miZeroPolyArc),
    SYMBOL(miEmptyBox),
    SYMBOL(miEmptyData),
    SYMBOL(miInstalledMaps),
    SYMBOL(miPointerScreenKey),
    SYMBOL(miZeroLineScreenKey),
#ifdef DAMAGE
    SYMBOL(DamageDamageRegion),
#endif
    SYMBOL(xf86ReadBIOS),
    SYMBOL(xf86EnableIO),
    SYMBOL(xf86DisableIO),
    SYMBOL(xf86LinearVidMem),
    SYMBOL(xf86CheckMTRR),
    SYMBOL(xf86MapVidMem),
    SYMBOL(xf86UnMapVidMem),
    SYMBOL(xf86MapReadSideEffects),
    SYMBOL(xf86MapDomainMemory),
    SYMBOL(xf86UDelay),
    SYMBOL(xf86SlowBcopy),
    SYMBOL(xf86SetReallySlowBcopy),
#ifdef __alpha__
    SYMBOL(xf86SlowBCopyToBus),
    SYMBOL(xf86SlowBCopyFromBus),
#endif
    SYMBOL(xf86BusToMem),
    SYMBOL(xf86MemToBus),
    SYMBOL(xf86OpenSerial),
    SYMBOL(xf86SetSerial),
    SYMBOL(xf86SetSerialSpeed),
    SYMBOL(xf86ReadSerial),
    SYMBOL(xf86WriteSerial),
    SYMBOL(xf86CloseSerial),
    SYMBOL(xf86WaitForInput),
    SYMBOL(xf86SerialSendBreak),
    SYMBOL(xf86FlushInput),
    SYMBOL(xf86SetSerialModemState),
    SYMBOL(xf86GetSerialModemState),
    SYMBOL(xf86SerialModemSetBits),
    SYMBOL(xf86SerialModemClearBits),
    SYMBOL(xf86LoadKernelModule),
    SYMBOL(xf86AgpGARTSupported),
    SYMBOL(xf86GetAGPInfo),
    SYMBOL(xf86AcquireGART),
    SYMBOL(xf86ReleaseGART),
    SYMBOL(xf86AllocateGARTMemory),
    SYMBOL(xf86DeallocateGARTMemory),
    SYMBOL(xf86BindGARTMemory),
    SYMBOL(xf86UnbindGARTMemory),
    SYMBOL(xf86EnableAGP),
    SYMBOL(xf86GARTCloseScreen),
    SYMBOL(XisbNew),
    SYMBOL(XisbFree),
    SYMBOL(XisbRead),
    SYMBOL(XisbWrite),
    SYMBOL(XisbTrace),
    SYMBOL(XisbBlockDuration),
    SYMBOL(xf86CheckPciSlot),
    SYMBOL(xf86ClaimPciSlot),
    SYMBOL(xf86ClaimFbSlot),
    SYMBOL(xf86ClaimNoSlot),
    SYMBOL(xf86ParsePciBusString),
    SYMBOL(xf86ComparePciBusString),
    SYMBOL(xf86FormatPciBusNumber),
    SYMBOL(xf86EnableAccess),
    SYMBOL(xf86SetCurrentAccess),
    SYMBOL(xf86IsPrimaryPci),
    SYMBOL(xf86FreeResList),
    SYMBOL(xf86ClaimFixedResources),
    SYMBOL(xf86AddEntityToScreen),
    SYMBOL(xf86SetEntityInstanceForScreen),
    SYMBOL(xf86RemoveEntityFromScreen),
    SYMBOL(xf86GetEntityInfo),
    SYMBOL(xf86GetNumEntityInstances),
    SYMBOL(xf86GetDevFromEntity),
    SYMBOL(xf86GetPciInfoForEntity),
    SYMBOL(xf86RegisterResources),
    SYMBOL(xf86CheckPciMemBase),
    SYMBOL(xf86SetAccessFuncs),
    SYMBOL(xf86IsEntityPrimary),
    SYMBOL(xf86SetOperatingState),
    SYMBOL(xf86FindScreenForEntity),
    SYMBOL(xf86RegisterStateChangeNotificationCallback),
    SYMBOL(xf86DeregisterStateChangeNotificationCallback),
    SYMBOL(xf86GetLastScrnFlag),
    SYMBOL(xf86SetLastScrnFlag),
    SYMBOL(xf86IsEntityShared),
    SYMBOL(xf86SetEntityShared),
    SYMBOL(xf86IsEntitySharable),
    SYMBOL(xf86SetEntitySharable),
    SYMBOL(xf86IsPrimInitDone),
    SYMBOL(xf86SetPrimInitDone),
    SYMBOL(xf86ClearPrimInitDone),
    SYMBOL(xf86AllocateEntityPrivateIndex),
    SYMBOL(xf86GetEntityPrivate),
    SYMBOL(xf86GetPointerScreenFuncs),
    SYMBOL(DGAInit),
    SYMBOL(DGAReInitModes),
    SYMBOL(DGAAvailable),
    SYMBOL(DGAActive),
    SYMBOL(DGASetMode),
    SYMBOL(DGASetInputMode),
    SYMBOL(DGASelectInput),
    SYMBOL(DGAGetViewportStatus),
    SYMBOL(DGASetViewport),
    SYMBOL(DGAInstallCmap),
    SYMBOL(DGASync),
    SYMBOL(DGAFillRect),
    SYMBOL(DGABlitRect),
    SYMBOL(DGABlitTransRect),
    SYMBOL(DGAGetModes),
    SYMBOL(DGAGetOldDGAMode),
    SYMBOL(DGAGetModeInfo),
    SYMBOL(DGAChangePixmapMode),
    SYMBOL(DGACreateColormap),
    SYMBOL(DGAOpenFramebuffer),
    SYMBOL(DGACloseFramebuffer),
    SYMBOL(xf86DPMSInit),
    SYMBOL(SetTimeSinceLastInputEvent),
    SYMBOL(xf86AddInputHandler),
    SYMBOL(xf86RemoveInputHandler),
    SYMBOL(xf86DisableInputHandler),
    SYMBOL(xf86EnableInputHandler),
    SYMBOL(xf86AddEnabledDevice),
    SYMBOL(xf86RemoveEnabledDevice),
    SYMBOL(xf86InterceptSignals),
    SYMBOL(xf86InterceptSigIll),
    SYMBOL(xf86EnableVTSwitch),
    SYMBOL(xf86AddDriver),
    SYMBOL(xf86AddInputDriver),
    SYMBOL(xf86DeleteDriver),
    SYMBOL(xf86DeleteInput),
    SYMBOL(xf86AllocateInput),
    SYMBOL(xf86AllocateScreen),
    SYMBOL(xf86DeleteScreen),
    SYMBOL(xf86AllocateScrnInfoPrivateIndex),
    SYMBOL(xf86AddPixFormat),
    SYMBOL(xf86SetDepthBpp),
    SYMBOL(xf86PrintDepthBpp),
    SYMBOL(xf86SetWeight),
    SYMBOL(xf86SetDefaultVisual),
    SYMBOL(xf86SetGamma),
    SYMBOL(xf86SetDpi),
    SYMBOL(xf86SetBlackWhitePixels),
    SYMBOL(xf86EnableDisableFBAccess),
    SYMBOL(xf86VDrvMsgVerb),
    SYMBOL(xf86DrvMsgVerb),
    SYMBOL(xf86DrvMsg),
    SYMBOL(xf86MsgVerb),
    SYMBOL(xf86Msg),
    SYMBOL(xf86ErrorFVerb),
    SYMBOL(xf86ErrorF),
    SYMBOL(xf86TokenToString),
    SYMBOL(xf86StringToToken),
    SYMBOL(xf86ShowClocks),
    SYMBOL(xf86PrintChipsets),
    SYMBOL(xf86MatchDevice),
    SYMBOL(xf86MatchPciInstances),
    SYMBOL(xf86GetVerbosity),
    SYMBOL(xf86GetVisualName),
    SYMBOL(xf86GetPix24),
    SYMBOL(xf86GetDepth),
    SYMBOL(xf86GetWeight),
    SYMBOL(xf86GetGamma),
    SYMBOL(xf86GetFlipPixels),
    SYMBOL(xf86GetServerName),
    SYMBOL(xf86ServerIsExiting),
    SYMBOL(xf86ServerIsOnlyDetecting),
    SYMBOL(xf86ServerIsOnlyProbing),
    SYMBOL(xf86ServerIsResetting),
    SYMBOL(xf86CaughtSignal),
    SYMBOL(xf86GetVidModeAllowNonLocal),
    SYMBOL(xf86GetVidModeEnabled),
    SYMBOL(xf86GetModInDevAllowNonLocal),
    SYMBOL(xf86GetModInDevEnabled),
    SYMBOL(xf86GetAllowMouseOpenFail),
    SYMBOL(xf86IsPc98),
    SYMBOL(xf86DisableRandR),
    SYMBOL(xf86GetRotation),
    SYMBOL(xf86GetModuleVersion),
    SYMBOL(xf86GetClocks),
    SYMBOL(xf86SetPriority),
    SYMBOL(xf86LoadDrvSubModule),
    SYMBOL(xf86LoadSubModule),
    SYMBOL(xf86LoadOneModule),
    SYMBOL(xf86UnloadSubModule),
    SYMBOL(xf86LoaderCheckSymbol),
    SYMBOL(xf86LoaderRefSymLists),
    SYMBOL(xf86LoaderRefSymbols),
    SYMBOL(xf86LoaderReqSymLists),
    SYMBOL(xf86LoaderReqSymbols),
    SYMBOL(xf86SetBackingStore),
    SYMBOL(xf86SetSilkenMouse),
    SYMBOL(xf86FindXvOptions),
    SYMBOL(xf86GetOS),
    SYMBOL(xf86ConfigPciEntity),
    SYMBOL(xf86ConfigFbEntity),
    SYMBOL(xf86ConfigActivePciEntity),
    SYMBOL(xf86ConfigPciEntityInactive),
    SYMBOL(xf86IsScreenPrimary),
    SYMBOL(xf86RegisterRootWindowProperty),
    SYMBOL(xf86IsUnblank),
#if (defined(__sparc__) || defined(__sparc)) && !defined(__OpenBSD__)
    SYMBOL(xf86MatchSbusInstances),
    SYMBOL(xf86GetSbusInfoForEntity),
    SYMBOL(xf86GetEntityForSbusInfo),
    SYMBOL(xf86SbusUseBuiltinMode),
    SYMBOL(xf86MapSbusMem),
    SYMBOL(xf86UnmapSbusMem),
    SYMBOL(xf86SbusHideOsHwCursor),
    SYMBOL(xf86SbusSetOsHwCursorCmap),
    SYMBOL(xf86SbusHandleColormaps),
    SYMBOL(sparcPromInit),
    SYMBOL(sparcPromClose),
    SYMBOL(sparcPromGetProperty),
    SYMBOL(sparcPromGetBool),
#endif
    SYMBOL(xf86GetPixFormat),
    SYMBOL(xf86GetBppFromDepth),
    SYMBOL(xf86GetNearestClock),
    SYMBOL(xf86ModeStatusToString),
    SYMBOL(xf86LookupMode),
    SYMBOL(xf86CheckModeForMonitor),
    SYMBOL(xf86InitialCheckModeForDriver),
    SYMBOL(xf86CheckModeForDriver),
    SYMBOL(xf86ValidateModes),
    SYMBOL(xf86DeleteMode),
    SYMBOL(xf86PruneDriverModes),
    SYMBOL(xf86SetCrtcForModes),
    SYMBOL(xf86PrintModes),
    SYMBOL(xf86ShowClockRanges),
    SYMBOL(xf86CollectOptions),
    SYMBOL(xf86CollectInputOptions),
    SYMBOL(xf86AddNewOption),
    SYMBOL(xf86NewOption),
    SYMBOL(xf86NextOption),
    SYMBOL(xf86OptionListCreate),
    SYMBOL(xf86OptionListMerge),
    SYMBOL(xf86OptionListFree),
    SYMBOL(xf86OptionName),
    SYMBOL(xf86OptionValue),
    SYMBOL(xf86OptionListReport),
    SYMBOL(xf86SetIntOption),
    SYMBOL(xf86SetRealOption),
    SYMBOL(xf86SetStrOption),
    SYMBOL(xf86SetBoolOption),
    SYMBOL(xf86CheckIntOption),
    SYMBOL(xf86CheckRealOption),
    SYMBOL(xf86CheckStrOption),
    SYMBOL(xf86CheckBoolOption),
    SYMBOL(xf86ReplaceIntOption),
    SYMBOL(xf86ReplaceRealOption),
    SYMBOL(xf86ReplaceStrOption),
    SYMBOL(xf86ReplaceBoolOption),
    SYMBOL(xf86FindOption),
    SYMBOL(xf86FindOptionValue),
    SYMBOL(xf86MarkOptionUsed),
    SYMBOL(xf86MarkOptionUsedByName),
    SYMBOL(xf86CheckIfOptionUsed),
    SYMBOL(xf86CheckIfOptionUsedByName),
    SYMBOL(xf86ShowUnusedOptions),
    SYMBOL(xf86ProcessOptions),
    SYMBOL(xf86TokenToOptinfo),
    SYMBOL(xf86TokenToOptName),
    SYMBOL(xf86IsOptionSet),
    SYMBOL(xf86GetOptValString),
    SYMBOL(xf86GetOptValInteger),
    SYMBOL(xf86GetOptValULong),
    SYMBOL(xf86GetOptValReal),
    SYMBOL(xf86GetOptValFreq),
    SYMBOL(xf86GetOptValBool),
    SYMBOL(xf86ReturnOptValBool),
    SYMBOL(xf86NameCmp),
    SYMBOL(xf86InitValuatorAxisStruct),
    SYMBOL(xf86InitValuatorDefaults),
    SYMBOL(xf86InitFBManager),
    SYMBOL(xf86InitFBManagerArea),
    SYMBOL(xf86InitFBManagerRegion),
    SYMBOL(xf86InitFBManagerLinear),
    SYMBOL(xf86RegisterFreeBoxCallback),
    SYMBOL(xf86FreeOffscreenArea),
    SYMBOL(xf86AllocateOffscreenArea),
    SYMBOL(xf86AllocateLinearOffscreenArea),
    SYMBOL(xf86ResizeOffscreenArea),
    SYMBOL(xf86FBManagerRunning),
    SYMBOL(xf86QueryLargestOffscreenArea),
    SYMBOL(xf86PurgeUnlockedOffscreenAreas),
    SYMBOL(xf86RegisterOffscreenManager),
    SYMBOL(xf86AllocateOffscreenLinear),
    SYMBOL(xf86ResizeOffscreenLinear),
    SYMBOL(xf86QueryLargestOffscreenLinear),
    SYMBOL(xf86FreeOffscreenLinear),
    SYMBOL(xf86HandleColormaps),
    SYMBOL(xf86GetGammaRampSize),
    SYMBOL(xf86GetGammaRamp),
    SYMBOL(xf86ChangeGammaRamp),
#ifdef RANDR
    SYMBOL(xf86RandRSetNewVirtualAndDimensions),
#endif
#ifdef XV
    SYMBOL(xf86XVScreenInit),
    SYMBOL(xf86XVRegisterGenericAdaptorDriver),
    SYMBOL(xf86XVListGenericAdaptors),
    SYMBOL(xf86XVRegisterOffscreenImages),
    SYMBOL(xf86XVQueryOffscreenImages),
    SYMBOL(xf86XVAllocateVideoAdaptorRec),
    SYMBOL(xf86XVFreeVideoAdaptorRec),
    SYMBOL(xf86XVFillKeyHelper),
    SYMBOL(xf86XVFillKeyHelperDrawable),
    SYMBOL(xf86XVClipVideoHelper),
    SYMBOL(xf86XVCopyYUV12ToPacked),
    SYMBOL(xf86XVCopyPacked),
    SYMBOL(xf86XvMCScreenInit),
    SYMBOL(xf86XvMCCreateAdaptorRec),
    SYMBOL(xf86XvMCDestroyAdaptorRec),
#endif
    SYMBOL(VidModeExtensionInit),
#ifdef XF86VIDMODE
    SYMBOL(VidModeGetCurrentModeline),
    SYMBOL(VidModeGetFirstModeline),
    SYMBOL(VidModeGetNextModeline),
    SYMBOL(VidModeDeleteModeline),
    SYMBOL(VidModeZoomViewport),
    SYMBOL(VidModeGetViewPort),
    SYMBOL(VidModeSetViewPort),
    SYMBOL(VidModeSwitchMode),
    SYMBOL(VidModeLockZoom),
    SYMBOL(VidModeGetMonitor),
    SYMBOL(VidModeGetNumOfClocks),
    SYMBOL(VidModeGetClocks),
    SYMBOL(VidModeCheckModeForMonitor),
    SYMBOL(VidModeCheckModeForDriver),
    SYMBOL(VidModeSetCrtcForMode),
    SYMBOL(VidModeAddModeline),
    SYMBOL(VidModeGetDotClock),
    SYMBOL(VidModeGetNumOfModes),
    SYMBOL(VidModeSetGamma),
    SYMBOL(VidModeGetGamma),
    SYMBOL(VidModeCreateMode),
    SYMBOL(VidModeCopyMode),
    SYMBOL(VidModeGetModeValue),
    SYMBOL(VidModeSetModeValue),
    SYMBOL(VidModeGetMonitorValue),
    SYMBOL(VidModeSetGammaRamp),
    SYMBOL(VidModeGetGammaRamp),
    SYMBOL(VidModeGetGammaRampSize),
#endif
    SYMBOL(GetTimeInMillis),
    SYMBOL(xf86ProcessCommonOptions),
    SYMBOL(xf86PostMotionEvent),
    SYMBOL(xf86PostProximityEvent),
    SYMBOL(xf86PostButtonEvent),
    SYMBOL(xf86PostKeyEvent),
    SYMBOL(xf86PostKeyboardEvent),
    SYMBOL(xf86FirstLocalDevice),
    SYMBOL(xf86ActivateDevice),
    SYMBOL(xf86XInputSetScreen),
    SYMBOL(xf86ScaleAxis),
    SYMBOL(NewInputDeviceRequest),
    SYMBOL(DeleteInputDeviceRequest),
#ifdef DPMSExtension
    SYMBOL(DPMSGet),
    SYMBOL(DPMSSet),
    SYMBOL(DPMSSupported),
#endif
    SYMBOL(pciTag),
    SYMBOL(pciBusAddrToHostAddr),
    SYMBOL(xf86scanpci),
    SYMBOL(LoadSubModule),
    SYMBOL(DuplicateModule),
    SYMBOL(LoaderErrorMsg),
    SYMBOL(LoaderCheckUnresolved),
    SYMBOL(LoadExtension),
    SYMBOL(LoaderReqSymbols),
    SYMBOL(LoaderReqSymLists),
    SYMBOL(LoaderRefSymbols),
    SYMBOL(LoaderRefSymLists),
    SYMBOL(UnloadSubModule),
    SYMBOL(LoaderSymbol),
    SYMBOL(LoaderListDirs),
    SYMBOL(LoaderFreeDirList),
    SYMBOL(LoaderGetOS),
    SYMBOL(LoaderShouldIgnoreABI),
    SYMBOL(LoaderGetABIVersion),
#ifdef XF86DRI
    SYMBOL(xf86InstallSIGIOHandler),
    SYMBOL(xf86RemoveSIGIOHandler),
# if defined(__alpha__) && defined(linux)
    SYMBOL(_bus_base),
# endif
#endif
    SYMBOL(xf86BlockSIGIO),
    SYMBOL(xf86UnblockSIGIO),
#if defined(__alpha__)
    SYMBOL(__divl),
    SYMBOL(__reml),
    SYMBOL(__divlu),
    SYMBOL(__remlu),
    SYMBOL(__divq),
    SYMBOL(__divqu),
    SYMBOL(__remq),
    SYMBOL(__remqu),
# ifdef linux
    SYMBOL(_outw),
    SYMBOL(_outb),
    SYMBOL(_outl),
    SYMBOL(_inb),
    SYMBOL(_inw),
    SYMBOL(_inl),
    SYMBOL(_alpha_outw),
    SYMBOL(_alpha_outb),
    SYMBOL(_alpha_outl),
    SYMBOL(_alpha_inb),
    SYMBOL(_alpha_inw),
    SYMBOL(_alpha_inl),
# else
    SYMBOL(outw),
    SYMBOL(outb),
    SYMBOL(outl),
    SYMBOL(inb),
    SYMBOL(inw),
    SYMBOL(inl),
# endif
    SYMBOL(xf86ReadMmio32),
    SYMBOL(xf86ReadMmio16),
    SYMBOL(xf86ReadMmio8),
    SYMBOL(xf86WriteMmio32),
    SYMBOL(xf86WriteMmio16),
    SYMBOL(xf86WriteMmio8),
    SYMBOL(xf86WriteMmioNB32),
    SYMBOL(xf86WriteMmioNB16),
    SYMBOL(xf86WriteMmioNB8),
#endif
#if defined(sun) && defined(SVR4)
    SYMBOL(inb),
    SYMBOL(inw),
    SYMBOL(inl),
    SYMBOL(outb),
    SYMBOL(outw),
    SYMBOL(outl),
#endif
#if defined(__powerpc__) && !defined(__OpenBSD__)
    SYMBOL(inb),
    SYMBOL(inw),
    SYMBOL(inl),
    SYMBOL(outb),
    SYMBOL(outw),
    SYMBOL(outl),
# if defined(NO_INLINE)
    SYMBOL(mem_barrier),
    SYMBOL(ldl_u),
    SYMBOL(eieio),
    SYMBOL(ldl_brx),
    SYMBOL(ldw_brx),
    SYMBOL(stl_brx),
    SYMBOL(stw_brx),
    SYMBOL(ldq_u),
    SYMBOL(ldw_u),
    SYMBOL(stl_u),
    SYMBOL(stq_u),
    SYMBOL(stw_u),
    SYMBOL(write_mem_barrier),
# endif
#endif
#if defined(__ia64__) || defined(__arm__)
    SYMBOL(outw),
    SYMBOL(outb),
    SYMBOL(outl),
    SYMBOL(inb),
    SYMBOL(inw),
    SYMBOL(inl),
#endif
    SYMBOL(xf86ScreenKey),
    SYMBOL(xf86PixmapKey),
    SYMBOL(xf86Screens),
    SYMBOL(byte_reversed),
    SYMBOL(xf86inSuspend),
    SYMBOL(resVgaExclusive),
    SYMBOL(resVgaShared),
    SYMBOL(resVgaMemShared),
    SYMBOL(resVgaIoShared),
    SYMBOL(resVgaUnusedExclusive),
    SYMBOL(resVgaUnusedShared),
    SYMBOL(resVgaSparseExclusive),
    SYMBOL(resVgaSparseShared),
    SYMBOL(res8514Exclusive),
    SYMBOL(res8514Shared),
#if defined(__powerpc__) && !defined(NO_INLINE)
    SYMBOL(ioBase),
#endif
    SYMBOL(xf86ConfigDRI),
    SYMBOL(ConfiguredMonitor),
    SYMBOL(xf86CrtcConfigPrivateIndex),
    SYMBOL(xf86CrtcConfigInit),
    SYMBOL(xf86CrtcConfigPrivateIndex),
    SYMBOL(xf86CrtcCreate),
    SYMBOL(xf86CrtcDestroy),
    SYMBOL(xf86CrtcInUse),
    SYMBOL(xf86CrtcSetScreenSubpixelOrder),
    SYMBOL(xf86RotateFreeShadow),
    SYMBOL(xf86RotateCloseScreen),
    SYMBOL(xf86CrtcRotate),
    SYMBOL(xf86CrtcSetMode),
    SYMBOL(xf86CrtcSetSizeRange),
    SYMBOL(xf86CrtcScreenInit),
    SYMBOL(xf86CVTMode),
    SYMBOL(xf86GTFMode),
    SYMBOL(xf86DisableUnusedFunctions),
    SYMBOL(xf86DPMSSet),
    SYMBOL(xf86DuplicateMode),
    SYMBOL(xf86DuplicateModes),
    SYMBOL(xf86GetDefaultModes),
    SYMBOL(xf86GetMonitorModes),
    SYMBOL(xf86InitialConfiguration),
    SYMBOL(xf86ModeHSync),
    SYMBOL(xf86ModesAdd),
    SYMBOL(xf86ModesEqual),
    SYMBOL(xf86ModeVRefresh),
    SYMBOL(xf86ModeWidth),
    SYMBOL(xf86ModeHeight),
    SYMBOL(xf86OutputCreate),
    SYMBOL(xf86OutputDestroy),
    SYMBOL(xf86OutputGetEDID),
    SYMBOL(xf86ConnectorGetName),
    SYMBOL(xf86OutputGetEDIDModes),
    SYMBOL(xf86OutputRename),
    SYMBOL(xf86OutputUseScreenMonitor),
    SYMBOL(xf86OutputSetEDID),
    SYMBOL(xf86OutputFindClosestMode),
    SYMBOL(xf86PrintModeline),
    SYMBOL(xf86ProbeOutputModes),
    SYMBOL(xf86PruneInvalidModes),
    SYMBOL(xf86SetModeCrtc),
    SYMBOL(xf86SetModeDefaultName),
    SYMBOL(xf86SetScrnInfoModes),
    SYMBOL(xf86SetDesiredModes),
    SYMBOL(xf86SetSingleMode),
    SYMBOL(xf86ValidateModesClocks),
    SYMBOL(xf86ValidateModesFlags),
    SYMBOL(xf86ValidateModesSize),
    SYMBOL(xf86ValidateModesSync),
    SYMBOL(xf86ValidateModesUserConfig),
    SYMBOL(xf86DiDGAInit),
    SYMBOL(xf86DiDGAReInit),
    SYMBOL(xf86DDCGetModes),
    SYMBOL(xf86SaveScreen),
#ifdef RANDR
    SYMBOL(xf86RandR12CreateScreenResources),
    SYMBOL(xf86RandR12GetOriginalVirtualSize),
    SYMBOL(xf86RandR12GetRotation),
    SYMBOL(xf86RandR12Init),
    SYMBOL(xf86RandR12PreInit),
    SYMBOL(xf86RandR12SetConfig),
    SYMBOL(xf86RandR12SetRotations),
    SYMBOL(xf86RandR12TellChanged),
#endif
    SYMBOL(xf86_cursors_init),
    SYMBOL(xf86_reload_cursors),
    SYMBOL(xf86_show_cursors),
    SYMBOL(xf86_hide_cursors),
    SYMBOL(xf86_cursors_fini),
    SYMBOL(xf86_crtc_clip_video_helper),
    SYMBOL(xf86DoEDID_DDC1),
    SYMBOL(xf86DoEDID_DDC2),
    SYMBOL(xf86InterpretEDID),
    SYMBOL(xf86PrintEDID),
    SYMBOL(xf86DoEEDID),
    SYMBOL(xf86DDCMonitorSet),
    SYMBOL(xf86SetDDCproperties),
    SYMBOL(xf86MonitorIsHDMI),
    SYMBOL(xf86CreateI2CBusRec),
    SYMBOL(xf86CreateI2CDevRec),
    SYMBOL(xf86DestroyI2CBusRec),
    SYMBOL(xf86DestroyI2CDevRec),
    SYMBOL(xf86I2CBusInit),
    SYMBOL(xf86I2CDevInit),
    SYMBOL(xf86I2CFindBus),
    SYMBOL(xf86I2CFindDev),
    SYMBOL(xf86I2CGetScreenBuses),
    SYMBOL(xf86I2CProbeAddress),
    SYMBOL(xf86I2CReadByte),
    SYMBOL(xf86I2CReadBytes),
    SYMBOL(xf86I2CReadStatus),
    SYMBOL(xf86I2CReadWord),
    SYMBOL(xf86I2CWriteByte),
    SYMBOL(xf86I2CWriteBytes),
    SYMBOL(xf86I2CWriteRead),
    SYMBOL(xf86I2CWriteVec),
    SYMBOL(xf86I2CWriteWord),
    SYMBOL(RamDacCreateInfoRec),
    SYMBOL(RamDacHelperCreateInfoRec),
    SYMBOL(RamDacDestroyInfoRec),
    SYMBOL(RamDacHelperDestroyInfoRec),
    SYMBOL(RamDacInit),
    SYMBOL(RamDacHandleColormaps),
    SYMBOL(RamDacFreeRec),
    SYMBOL(RamDacGetHWIndex),
    SYMBOL(RamDacHWPrivateIndex),
    SYMBOL(RamDacScreenPrivateIndex),
    SYMBOL(xf86InitCursor),
    SYMBOL(xf86CreateCursorInfoRec),
    SYMBOL(xf86DestroyCursorInfoRec),
    SYMBOL(xf86ForceHWCursor),
    SYMBOL(BTramdacProbe),
    SYMBOL(BTramdacSave),
    SYMBOL(BTramdacRestore),
    SYMBOL(BTramdacSetBpp),
    SYMBOL(IBMramdacProbe),
    SYMBOL(IBMramdacSave),
    SYMBOL(IBMramdacRestore),
    SYMBOL(IBMramdac526SetBpp),
    SYMBOL(IBMramdac640SetBpp),
    SYMBOL(IBMramdac526CalculateMNPCForClock),
    SYMBOL(IBMramdac640CalculateMNPCForClock),
    SYMBOL(IBMramdac526HWCursorInit),
    SYMBOL(IBMramdac640HWCursorInit),
    SYMBOL(IBMramdac526SetBppWeak),
    SYMBOL(TIramdacCalculateMNPForClock),
    SYMBOL(TIramdacProbe),
    SYMBOL(TIramdacSave),
    SYMBOL(TIramdacRestore),
    SYMBOL(TIramdac3026SetBpp),
    SYMBOL(TIramdac3030SetBpp),
    SYMBOL(TIramdacHWCursorInit),
    SYMBOL(TIramdacLoadPalette),
};
