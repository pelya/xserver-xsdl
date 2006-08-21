/*
 * Copyright 1995,96 by Metro Link, Inc.
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
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#define INCLUDE_DEPRECATED 1

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <fcntl.h>
#include <setjmp.h>
#include "sym.h"
#include "misc.h"
#include "mi.h"
#include "cursor.h"
#include "mipointer.h"
#include "loaderProcs.h"
#include "xf86Pci.h"
#include "xf86.h"
#include "xf86Resources.h"
#include "xf86_OSproc.h"
#include "xf86Parser.h"
#include "xf86Config.h"
#ifdef XINPUT
# include "xf86Xinput.h"
#endif
#include "xf86OSmouse.h"
#include "xf86OSKbd.h"
#include "xf86xv.h"
#include "xf86xvmc.h"
#include "xf86cmap.h"
#include "xf86fbman.h"
#include "dgaproc.h"
#ifdef DPMSExtension
#include "dpmsproc.h"
#endif
#include "vidmodeproc.h"
#include "xf86miscproc.h"
#include "loader.h"
#define DONT_DEFINE_WRAPPERS
#include "xf86_ansic.h"
#include "xisb.h"
#include "vbe.h"
#ifndef __OpenBSD__
#include "xf86sbusBus.h"
#endif
#include "compiler.h"

#ifndef HAS_GLIBC_SIGSETJMP
#if defined(setjmp) && defined(__GNU_LIBRARY__) && \
    (!defined(__GLIBC__) || (__GLIBC__ < 2) || \
     ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 3)))
#define HAS_GLIBC_SIGSETJMP 1
#endif
#endif

#if defined(__alpha__)
# ifdef linux
extern unsigned long _bus_base(void);
extern void _outb(char val, unsigned short port);
extern void _outw(short val, unsigned short port);
extern void _outl(int val, unsigned short port);
extern unsigned int _inb(unsigned short port);
extern unsigned int _inw(unsigned short port);
extern unsigned int _inl(unsigned short port);
# endif

# ifdef __FreeBSD__
#  include <sys/types.h>
extern void outb(u_int32_t port, u_int8_t val);
extern void outw(u_int32_t port, u_int16_t val);
extern void outl(u_int32_t port, u_int32_t val);
extern u_int8_t inb(u_int32_t port);
extern u_int16_t inw(u_int32_t port);
extern u_int32_t inl(u_int32_t port);
# endif

extern void *__divl(long, long);
extern void *__reml(long, long);
extern void *__divlu(long, long);
extern void *__remlu(long, long);
extern void *__divq(long, long);
extern void *__divqu(long, long);
extern void *__remq(long, long);
extern void *__remqu(long, long);
#endif

#if defined(__sparc__) && defined(__FreeBSD__)
extern float _Qp_qtos(unsigned int *);
extern double _Qp_qtod(unsigned int *);
extern unsigned long long _Qp_qtoux(unsigned int *);
extern void _Qp_dtoq(unsigned int *, double);
extern void _Qp_uitoq(unsigned int *, unsigned int);
#endif

#if defined(__GNUC__)
#ifndef __UNIXOS2__
extern long __div64(long, long);
#endif
extern long __divdf3(long, long);
extern long __divdi3(long, long);
extern long __divsf3(long, long);
extern long __divsi3(long, long);
extern long __moddi3(long, long);
extern long __modsi3(long, long);

#ifndef __UNIXOS2__
extern long __mul64(long, long);
#endif
extern long __muldf3(long, long);
extern long __muldi3(long, long);
extern long __mulsf3(long, long);
extern long __mulsi3(long, long);
extern long __udivdi3(long, long);
extern long __udivsi3(long, long);
extern long __umoddi3(long, long);
extern long __umodsi3(long, long);

#ifndef __UNIXOS2__
#pragma weak __div64
#endif
#pragma weak __divdf3
#pragma weak __divdi3
#pragma weak __divsf3
#pragma weak __divsi3
#pragma weak __moddi3
#pragma weak __modsi3
#ifndef __UNIXOS2__
#pragma weak __mul64
#endif
#pragma weak __muldf3
#pragma weak __muldi3
#pragma weak __mulsf3
#pragma weak __mulsi3
#pragma weak __udivdi3
#pragma weak __udivsi3
#pragma weak __umoddi3
#pragma weak __umodsi3
#endif

#if defined(__arm__) && defined(__linux__)
#include <sys/io.h>
#endif

#if defined(__powerpc__) && (defined(Lynx) || defined(linux))
void _restf14();
void _restf17();
void _restf18();
void _restf19();
void _restf20();
void _restf22();
void _restf23();
void _restf24();
void _restf25();
void _restf26();
void _restf27();
void _restf28();
void _restf29();
void _savef14();
void _savef17();
void _savef18();
void _savef19();
void _savef20();
void _savef22();
void _savef23();
void _savef24();
void _savef25();
void _savef26();
void _savef27();
void _savef28();
void _savef29();

/* even if we compile without -DNO_INLINE we still provide
 * the usual port i/o functions for module use
 */

extern volatile unsigned char *ioBase;

/* XXX Should get all of these from elsewhere */
#ifndef linux
extern void outb(IOADDRESS, unsigned char);
extern void outw(IOADDRESS, unsigned short);
extern void outl(IOADDRESS, unsigned int);
extern unsigned int inb(IOADDRESS);
extern unsigned int inw(IOADDRESS);
extern unsigned int inl(IOADDRESS);
#endif
extern void stl_brx(unsigned long, volatile unsigned char *, int);
extern void stw_brx(unsigned short, volatile unsigned char *, int);
extern unsigned long ldl_brx(volatile unsigned char *, int);
extern unsigned short ldw_brx(volatile unsigned char *, int);
#endif

/* XFree86 things */

_X_HIDDEN void *xfree86LookupTab[] = {

    /* Public OSlib functions */
    SYMFUNC(xf86ReadBIOS)
    SYMFUNC(xf86EnableIO)
    SYMFUNC(xf86DisableIO)
    SYMFUNC(xf86DisableInterrupts)
    SYMFUNC(xf86EnableInterrupts)
    SYMFUNC(xf86LinearVidMem)
    SYMFUNC(xf86CheckMTRR)
    SYMFUNC(xf86MapVidMem)
    SYMFUNC(xf86UnMapVidMem)
    SYMFUNC(xf86MapReadSideEffects)
    SYMFUNC(xf86GetPciDomain)
    SYMFUNC(xf86MapDomainMemory)
    SYMFUNC(xf86MapDomainIO)
    SYMFUNC(xf86ReadDomainMemory)
    SYMFUNC(xf86UDelay)
    SYMFUNC(xf86IODelay)
    SYMFUNC(xf86SlowBcopy)
#ifdef __alpha__
    SYMFUNC(xf86SlowBCopyToBus)
    SYMFUNC(xf86SlowBCopyFromBus)
#endif
    SYMFUNC(xf86BusToMem)
    SYMFUNC(xf86MemToBus)
    SYMFUNC(xf86OpenSerial)
    SYMFUNC(xf86SetSerial)
    SYMFUNC(xf86SetSerialSpeed)
    SYMFUNC(xf86ReadSerial)
    SYMFUNC(xf86WriteSerial)
    SYMFUNC(xf86CloseSerial)
    SYMFUNC(xf86GetErrno)
    SYMFUNC(xf86WaitForInput)
    SYMFUNC(xf86SerialSendBreak)
    SYMFUNC(xf86FlushInput)
    SYMFUNC(xf86SetSerialModemState)
    SYMFUNC(xf86GetSerialModemState)
    SYMFUNC(xf86SerialModemSetBits)
    SYMFUNC(xf86SerialModemClearBits)
    SYMFUNC(xf86LoadKernelModule)
    SYMFUNC(xf86OSMouseInit)
    SYMFUNC(xf86OSKbdPreInit)
    SYMFUNC(xf86AgpGARTSupported)
    SYMFUNC(xf86GetAGPInfo)
    SYMFUNC(xf86AcquireGART)
    SYMFUNC(xf86ReleaseGART)
    SYMFUNC(xf86AllocateGARTMemory)
    SYMFUNC(xf86DeallocateGARTMemory)
    SYMFUNC(xf86BindGARTMemory)
    SYMFUNC(xf86UnbindGARTMemory)
    SYMFUNC(xf86EnableAGP)
    SYMFUNC(xf86SoundKbdBell)
    SYMFUNC(xf86GARTCloseScreen)
#ifdef XINPUT
    /* XISB routines  (Merged from Metrolink tree) */
    SYMFUNC(XisbNew)
    SYMFUNC(XisbFree)
    SYMFUNC(XisbRead)
    SYMFUNC(XisbWrite)
    SYMFUNC(XisbTrace)
    SYMFUNC(XisbBlockDuration)
#endif

    /* xf86Bus.c */
    SYMFUNC(xf86CheckPciSlot)
    SYMFUNC(xf86ClaimPciSlot)
    SYMFUNC(xf86GetPciVideoInfo)
    SYMFUNC(xf86GetPciEntity)
    SYMFUNC(xf86GetPciConfigInfo)
    SYMFUNC(xf86SetPciVideo)
    SYMFUNC(xf86ClaimIsaSlot)
    SYMFUNC(xf86ClaimFbSlot)
    SYMFUNC(xf86ClaimNoSlot)
    SYMFUNC(xf86ParsePciBusString)
    SYMFUNC(xf86ComparePciBusString)
    SYMFUNC(xf86FormatPciBusNumber)
    SYMFUNC(xf86ParseIsaBusString)
    SYMFUNC(xf86EnableAccess)
    SYMFUNC(xf86SetCurrentAccess)
    SYMFUNC(xf86IsPrimaryPci)
    SYMFUNC(xf86IsPrimaryIsa)
    SYMFUNC(xf86PrintResList)
    SYMFUNC(xf86AddResToList)
    SYMFUNC(xf86JoinResLists)
    SYMFUNC(xf86DupResList)
    SYMFUNC(xf86FreeResList)
    SYMFUNC(xf86ClaimFixedResources)
    SYMFUNC(xf86AddEntityToScreen)
    SYMFUNC(xf86SetEntityInstanceForScreen)
    SYMFUNC(xf86RemoveEntityFromScreen)
    SYMFUNC(xf86GetEntityInfo)
    SYMFUNC(xf86GetNumEntityInstances)
    SYMFUNC(xf86GetDevFromEntity)
    SYMFUNC(xf86GetPciInfoForEntity)
    SYMFUNC(xf86SetEntityFuncs)
    SYMFUNC(xf86DeallocateResourcesForEntity)
    SYMFUNC(xf86RegisterResources)
    SYMFUNC(xf86CheckPciMemBase)
    SYMFUNC(xf86SetAccessFuncs)
    SYMFUNC(xf86IsEntityPrimary)
    SYMFUNC(xf86FixPciResource)
    SYMFUNC(xf86SetOperatingState)
    SYMFUNC(xf86EnterServerState)
    SYMFUNC(xf86GetBlock)
    SYMFUNC(xf86GetSparse)
    SYMFUNC(xf86ReallocatePciResources)
    SYMFUNC(xf86ChkConflict)
    SYMFUNC(xf86IsPciDevPresent)
    SYMFUNC(xf86FindScreenForEntity)
    SYMFUNC(xf86FindPciDeviceVendor)
    SYMFUNC(xf86FindPciClass)
#ifdef INCLUDE_DEPRECATED
    SYMFUNC(xf86EnablePciBusMaster)
#endif
    SYMFUNC(xf86RegisterStateChangeNotificationCallback)
    SYMFUNC(xf86DeregisterStateChangeNotificationCallback)
    SYMFUNC(xf86NoSharedResources)
    /* Shared Accel Accessor Functions */
    SYMFUNC(xf86GetLastScrnFlag)
    SYMFUNC(xf86SetLastScrnFlag)
    SYMFUNC(xf86IsEntityShared)
    SYMFUNC(xf86SetEntityShared)
    SYMFUNC(xf86IsEntitySharable)
    SYMFUNC(xf86SetEntitySharable)
    SYMFUNC(xf86IsPrimInitDone)
    SYMFUNC(xf86SetPrimInitDone)
    SYMFUNC(xf86ClearPrimInitDone)
    SYMFUNC(xf86AllocateEntityPrivateIndex)
    SYMFUNC(xf86GetEntityPrivate)

    /* xf86cvt.c */
    SYMFUNC(xf86CVTMode)

    /* xf86Configure.c */
    SYMFUNC(xf86AddDeviceToConfigure)

    /* xf86Cursor.c */
    SYMFUNC(xf86GetPointerScreenFuncs)

    /* xf86DGA.c */
    /* For drivers */
    SYMFUNC(DGAInit)
    SYMFUNC(DGAReInitModes)
    /* For extmod */
    SYMFUNC(DGAAvailable)
    SYMFUNC(DGAActive)
    SYMFUNC(DGASetMode)
    SYMFUNC(DGASetInputMode)
    SYMFUNC(DGASelectInput)
    SYMFUNC(DGAGetViewportStatus)
    SYMFUNC(DGASetViewport)
    SYMFUNC(DGAInstallCmap)
    SYMFUNC(DGASync)
    SYMFUNC(DGAFillRect)
    SYMFUNC(DGABlitRect)
    SYMFUNC(DGABlitTransRect)
    SYMFUNC(DGAGetModes)
    SYMFUNC(DGAGetOldDGAMode)
    SYMFUNC(DGAGetModeInfo)
    SYMFUNC(DGAChangePixmapMode)
    SYMFUNC(DGACreateColormap)
    SYMFUNC(DGAOpenFramebuffer)
    SYMFUNC(DGACloseFramebuffer)

    /* xf86DPMS.c */
    SYMFUNC(xf86DPMSInit)

    /* xf86Events.c */
    SYMFUNC(SetTimeSinceLastInputEvent)
    SYMFUNC(xf86AddInputHandler)
    SYMFUNC(xf86RemoveInputHandler)
    SYMFUNC(xf86DisableInputHandler)
    SYMFUNC(xf86EnableInputHandler)
    SYMFUNC(xf86AddEnabledDevice)
    SYMFUNC(xf86RemoveEnabledDevice)
    SYMFUNC(xf86InterceptSignals)
    SYMFUNC(xf86InterceptSigIll)
    SYMFUNC(xf86EnableVTSwitch)

    /* xf86Helper.c */
    SYMFUNC(xf86AddDriver)
    SYMFUNC(xf86AddInputDriver)
    SYMFUNC(xf86DeleteDriver)
    SYMFUNC(xf86DeleteInput)
    SYMFUNC(xf86AllocateInput)
    SYMFUNC(xf86AllocateScreen)
    SYMFUNC(xf86DeleteScreen)
    SYMFUNC(xf86AllocateScrnInfoPrivateIndex)
    SYMFUNC(xf86AddPixFormat)
    SYMFUNC(xf86SetDepthBpp)
    SYMFUNC(xf86PrintDepthBpp)
    SYMFUNC(xf86SetWeight)
    SYMFUNC(xf86SetDefaultVisual)
    SYMFUNC(xf86SetGamma)
    SYMFUNC(xf86SetDpi)
    SYMFUNC(xf86SetBlackWhitePixels)
    SYMFUNC(xf86EnableDisableFBAccess)
    SYMFUNC(xf86VDrvMsgVerb)
    SYMFUNC(xf86DrvMsgVerb)
    SYMFUNC(xf86DrvMsg)
    SYMFUNC(xf86MsgVerb)
    SYMFUNC(xf86Msg)
    SYMFUNC(xf86ErrorFVerb)
    SYMFUNC(xf86ErrorF)
    SYMFUNC(xf86TokenToString)
    SYMFUNC(xf86StringToToken)
    SYMFUNC(xf86ShowClocks)
    SYMFUNC(xf86PrintChipsets)
    SYMFUNC(xf86MatchDevice)
    SYMFUNC(xf86MatchPciInstances)
    SYMFUNC(xf86MatchIsaInstances)
    SYMFUNC(xf86GetVerbosity)
    SYMFUNC(xf86GetVisualName)
    SYMFUNC(xf86GetPix24)
    SYMFUNC(xf86GetDepth)
    SYMFUNC(xf86GetWeight)
    SYMFUNC(xf86GetGamma)
    SYMFUNC(xf86GetFlipPixels)
    SYMFUNC(xf86GetServerName)
    SYMFUNC(xf86ServerIsExiting)
    SYMFUNC(xf86ServerIsOnlyDetecting)
    SYMFUNC(xf86ServerIsOnlyProbing)
    SYMFUNC(xf86ServerIsResetting)
    SYMFUNC(xf86CaughtSignal)
    SYMFUNC(xf86GetVidModeAllowNonLocal)
    SYMFUNC(xf86GetVidModeEnabled)
    SYMFUNC(xf86GetModInDevAllowNonLocal)
    SYMFUNC(xf86GetModInDevEnabled)
    SYMFUNC(xf86GetAllowMouseOpenFail)
    SYMFUNC(xf86CommonSpecialKey)
    SYMFUNC(xf86IsPc98)
    SYMFUNC(xf86DisableRandR)
    SYMFUNC(xf86GetRotation)
    SYMFUNC(xf86GetVersion)
    SYMFUNC(xf86GetModuleVersion)
    SYMFUNC(xf86GetClocks)
    SYMFUNC(xf86SetPriority)
    SYMFUNC(xf86LoadDrvSubModule)
    SYMFUNC(xf86LoadSubModule)
    SYMFUNC(xf86LoadOneModule)
    SYMFUNC(xf86UnloadSubModule)
    SYMFUNC(xf86LoaderCheckSymbol)
    SYMFUNC(xf86LoaderRefSymLists)
    SYMFUNC(xf86LoaderRefSymbols)
    SYMFUNC(xf86LoaderReqSymLists)
    SYMFUNC(xf86LoaderReqSymbols)
    SYMFUNC(xf86SetBackingStore)
    SYMFUNC(xf86SetSilkenMouse)
    /* SYMFUNC(xf86NewSerialNumber) */
    SYMFUNC(xf86FindXvOptions)
    SYMFUNC(xf86GetOS)
    SYMFUNC(xf86ConfigPciEntity)
    SYMFUNC(xf86ConfigIsaEntity)
    SYMFUNC(xf86ConfigFbEntity)
    SYMFUNC(xf86ConfigActivePciEntity)
    SYMFUNC(xf86ConfigActiveIsaEntity)
    SYMFUNC(xf86ConfigPciEntityInactive)
    SYMFUNC(xf86ConfigIsaEntityInactive)
    SYMFUNC(xf86IsScreenPrimary)
    SYMFUNC(xf86RegisterRootWindowProperty)
    SYMFUNC(xf86IsUnblank)
    SYMFUNC(xf86AddModuleInfo)
    SYMFUNC(xf86DeleteModuleInfo)

#if defined(__sparc__) && !defined(__OpenBSD__)
    /* xf86sbusBus.c */
    SYMFUNC(xf86MatchSbusInstances)
    SYMFUNC(xf86GetSbusInfoForEntity)
    SYMFUNC(xf86GetEntityForSbusInfo)
    SYMFUNC(xf86SbusUseBuiltinMode)
    SYMFUNC(xf86MapSbusMem)
    SYMFUNC(xf86UnmapSbusMem)
    SYMFUNC(xf86SbusHideOsHwCursor)
    SYMFUNC(xf86SbusSetOsHwCursorCmap)
    SYMFUNC(xf86SbusHandleColormaps)
    SYMFUNC(sparcPromInit)
    SYMFUNC(sparcPromClose)
    SYMFUNC(sparcPromGetProperty)
    SYMFUNC(sparcPromGetBool)
#endif

    /* xf86Init.c */
    SYMFUNC(xf86GetPixFormat)
    SYMFUNC(xf86GetBppFromDepth)

    /* xf86Mode.c */
    SYMFUNC(xf86GetNearestClock)
    SYMFUNC(xf86ModeStatusToString)
    SYMFUNC(xf86LookupMode)
    SYMFUNC(xf86CheckModeForMonitor)
    SYMFUNC(xf86InitialCheckModeForDriver)
    SYMFUNC(xf86CheckModeForDriver)
    SYMFUNC(xf86ValidateModes)
    SYMFUNC(xf86DeleteMode)
    SYMFUNC(xf86PruneDriverModes)
    SYMFUNC(xf86SetCrtcForModes)
    SYMFUNC(xf86PrintModes)
    SYMFUNC(xf86ShowClockRanges)

    /* xf86Option.c */
    SYMFUNC(xf86CollectOptions)
    SYMFUNC(xf86CollectInputOptions)
    /* Merging of XInput stuff   */
    SYMFUNC(xf86AddNewOption)
    SYMFUNC(xf86NewOption)
    SYMFUNC(xf86NextOption)
    SYMFUNC(xf86OptionListCreate)
    SYMFUNC(xf86OptionListMerge)
    SYMFUNC(xf86OptionListFree)
    SYMFUNC(xf86OptionName)
    SYMFUNC(xf86OptionValue)
    SYMFUNC(xf86OptionListReport)
    SYMFUNC(xf86SetIntOption)
    SYMFUNC(xf86SetRealOption)
    SYMFUNC(xf86SetStrOption)
    SYMFUNC(xf86SetBoolOption)
    SYMFUNC(xf86CheckIntOption)
    SYMFUNC(xf86CheckRealOption)
    SYMFUNC(xf86CheckStrOption)
    SYMFUNC(xf86CheckBoolOption)
    SYMFUNC(xf86ReplaceIntOption)
    SYMFUNC(xf86ReplaceRealOption)
    SYMFUNC(xf86ReplaceStrOption)
    SYMFUNC(xf86ReplaceBoolOption)
    SYMFUNC(xf86FindOption)
    SYMFUNC(xf86FindOptionValue)
    SYMFUNC(xf86MarkOptionUsed)
    SYMFUNC(xf86MarkOptionUsedByName)
    SYMFUNC(xf86CheckIfOptionUsed)
    SYMFUNC(xf86CheckIfOptionUsedByName)
    SYMFUNC(xf86ShowUnusedOptions)
    SYMFUNC(xf86ProcessOptions)
    SYMFUNC(xf86TokenToOptinfo)
    SYMFUNC(xf86TokenToOptName)
    SYMFUNC(xf86IsOptionSet)
    SYMFUNC(xf86GetOptValString)
    SYMFUNC(xf86GetOptValInteger)
    SYMFUNC(xf86GetOptValULong)
    SYMFUNC(xf86GetOptValReal)
    SYMFUNC(xf86GetOptValFreq)
    SYMFUNC(xf86GetOptValBool)
    SYMFUNC(xf86ReturnOptValBool)
    SYMFUNC(xf86NameCmp)
    SYMFUNC(xf86InitValuatorAxisStruct)
    SYMFUNC(xf86InitValuatorDefaults)

    /* xf86fbman.c */
    SYMFUNC(xf86InitFBManager)
    SYMFUNC(xf86InitFBManagerArea)
    SYMFUNC(xf86InitFBManagerRegion)
    SYMFUNC(xf86InitFBManagerLinear)
    SYMFUNC(xf86RegisterFreeBoxCallback)
    SYMFUNC(xf86FreeOffscreenArea)
    SYMFUNC(xf86AllocateOffscreenArea)
    SYMFUNC(xf86AllocateLinearOffscreenArea)
    SYMFUNC(xf86ResizeOffscreenArea)
    SYMFUNC(xf86FBManagerRunning)
    SYMFUNC(xf86QueryLargestOffscreenArea)
    SYMFUNC(xf86PurgeUnlockedOffscreenAreas)
    SYMFUNC(xf86RegisterOffscreenManager)
    SYMFUNC(xf86AllocateOffscreenLinear)
    SYMFUNC(xf86ResizeOffscreenLinear)
    SYMFUNC(xf86QueryLargestOffscreenLinear)
    SYMFUNC(xf86FreeOffscreenLinear)

    /* xf86cmap.c */
    SYMFUNC(xf86HandleColormaps)
    SYMFUNC(xf86GetGammaRampSize)
    SYMFUNC(xf86GetGammaRamp)
    SYMFUNC(xf86ChangeGammaRamp)

    /* xf86RandR.c */
#ifdef RANDR
    SYMFUNC(xf86RandRSetNewVirtualAndDimensions)
#endif

    /* xf86xv.c */
#ifdef XV
    SYMFUNC(xf86XVScreenInit)
    SYMFUNC(xf86XVRegisterGenericAdaptorDriver)
    SYMFUNC(xf86XVListGenericAdaptors)
    SYMFUNC(xf86XVRegisterOffscreenImages)
    SYMFUNC(xf86XVQueryOffscreenImages)
    SYMFUNC(xf86XVAllocateVideoAdaptorRec)
    SYMFUNC(xf86XVFreeVideoAdaptorRec)
    SYMFUNC(xf86XVFillKeyHelper)
    SYMFUNC(xf86XVClipVideoHelper)
    SYMFUNC(xf86XVCopyYUV12ToPacked)
    SYMFUNC(xf86XVCopyPacked)

    /* xf86xvmc.c */
    SYMFUNC(xf86XvMCScreenInit)
    SYMFUNC(xf86XvMCCreateAdaptorRec)
    SYMFUNC(xf86XvMCDestroyAdaptorRec)
#endif

    /* xf86VidMode.c */
    SYMFUNC(VidModeExtensionInit)
#ifdef XF86VIDMODE
    SYMFUNC(VidModeGetCurrentModeline)
    SYMFUNC(VidModeGetFirstModeline)
    SYMFUNC(VidModeGetNextModeline)
    SYMFUNC(VidModeDeleteModeline)
    SYMFUNC(VidModeZoomViewport)
    SYMFUNC(VidModeGetViewPort)
    SYMFUNC(VidModeSetViewPort)
    SYMFUNC(VidModeSwitchMode)
    SYMFUNC(VidModeLockZoom)
    SYMFUNC(VidModeGetMonitor)
    SYMFUNC(VidModeGetNumOfClocks)
    SYMFUNC(VidModeGetClocks)
    SYMFUNC(VidModeCheckModeForMonitor)
    SYMFUNC(VidModeCheckModeForDriver)
    SYMFUNC(VidModeSetCrtcForMode)
    SYMFUNC(VidModeAddModeline)
    SYMFUNC(VidModeGetDotClock)
    SYMFUNC(VidModeGetNumOfModes)
    SYMFUNC(VidModeSetGamma)
    SYMFUNC(VidModeGetGamma)
    SYMFUNC(VidModeCreateMode)
    SYMFUNC(VidModeCopyMode)
    SYMFUNC(VidModeGetModeValue)
    SYMFUNC(VidModeSetModeValue)
    SYMFUNC(VidModeGetMonitorValue)
    SYMFUNC(VidModeSetGammaRamp)
    SYMFUNC(VidModeGetGammaRamp)
    SYMFUNC(VidModeGetGammaRampSize)
#endif

    /* xf86Versions.c */
    SYMFUNC(xf86GetBuiltinInterfaceVersion)
    SYMFUNC(xf86RegisterBuiltinInterfaceVersion)

    /* xf86MiscExt.c */
#ifdef XF86MISC
    SYMFUNC(MiscExtGetMouseSettings)
    SYMFUNC(MiscExtGetMouseValue)
    SYMFUNC(MiscExtSetMouseValue)
    SYMFUNC(MiscExtSetMouseDevice)
    SYMFUNC(MiscExtGetKbdSettings)
    SYMFUNC(MiscExtGetKbdValue)
    SYMFUNC(MiscExtSetKbdValue)
    SYMFUNC(MiscExtSetGrabKeysState)
    SYMFUNC(MiscExtCreateStruct)
    SYMFUNC(MiscExtDestroyStruct)
    SYMFUNC(MiscExtApply)
    SYMFUNC(MiscExtGetFilePaths)
    SYMFUNC(MiscExtPassMessage)
#endif

    /* Misc */
    SYMFUNC(GetTimeInMillis)

    /* xf86Xinput.c */
#ifdef XINPUT
    SYMFUNC(xf86ProcessCommonOptions)
    SYMFUNC(xf86IsCorePointer)
    SYMFUNC(xf86PostMotionEvent)
    SYMFUNC(xf86PostProximityEvent)
    SYMFUNC(xf86PostButtonEvent)
    SYMFUNC(xf86PostKeyEvent)
    SYMFUNC(xf86PostKeyboardEvent)
    SYMFUNC(xf86GetMotionEvents)
    SYMFUNC(xf86MotionHistoryAllocate)
    SYMFUNC(xf86FirstLocalDevice)
    SYMFUNC(xf86eqEnqueue)
    SYMFUNC(xf86ActivateDevice)
/* The following segment merged from Metrolink tree */
    SYMFUNC(xf86XInputSetScreen)
    SYMFUNC(xf86ScaleAxis)
    SYMFUNC(xf86XInputSetSendCoreEvents)
/* End merged segment */
#endif
#ifdef DPMSExtension
    SYMFUNC(DPMSGet)
    SYMFUNC(DPMSSet)
    SYMFUNC(DPMSSupported)
#endif
/* xf86Debug.c */
#ifdef BUILDDEBUG
    SYMFUNC(xf86Break1)
    SYMFUNC(xf86Break2)
    SYMFUNC(xf86Break3)
    SYMFUNC(xf86SPTimestamp)
    SYMFUNC(xf86STimestamp)
#endif

    SYMFUNC(pciFindFirst)
    SYMFUNC(pciFindNext)
    SYMFUNC(pciWriteByte)
    SYMFUNC(pciWriteWord)
    SYMFUNC(pciWriteLong)
    SYMFUNC(pciReadByte)
    SYMFUNC(pciReadWord)
    SYMFUNC(pciReadLong)
    SYMFUNC(pciSetBitsLong)
    SYMFUNC(pciTag)
    SYMFUNC(pciBusAddrToHostAddr)
    SYMFUNC(pciHostAddrToBusAddr)
    SYMFUNC(xf86MapPciMem)
    SYMFUNC(xf86scanpci)
    SYMFUNC(xf86ReadPciBIOS)

    /* Loader functions */
    SYMFUNC(LoadSubModule)
    SYMFUNC(DuplicateModule)
    SYMFUNC(LoaderErrorMsg)
    SYMFUNC(LoaderCheckUnresolved)
    SYMFUNC(LoadExtension)
    SYMFUNC(LoadFont)
    SYMFUNC(LoaderReqSymbols)
    SYMFUNC(LoaderReqSymLists)
    SYMFUNC(LoaderRefSymbols)
    SYMFUNC(LoaderRefSymLists)
    SYMFUNC(UnloadSubModule)
    SYMFUNC(LoaderSymbol)
    SYMFUNC(LoaderListDirs)
    SYMFUNC(LoaderFreeDirList)
    SYMFUNC(LoaderGetOS)
    SYMFUNC(LoaderGetABIVersion)

    /*
     * These are our own interfaces to libc functions.
     */
    SYMFUNC(xf86abort)
    SYMFUNC(xf86abs)
    SYMFUNC(xf86acos)
    SYMFUNC(xf86asin)
    SYMFUNC(xf86atan)
    SYMFUNC(xf86atan2)
    SYMFUNC(xf86atof)
    SYMFUNC(xf86atoi)
    SYMFUNC(xf86atol)
    SYMFUNC(xf86bsearch)
    SYMFUNC(xf86ceil)
    SYMFUNC(xf86calloc)
    SYMFUNC(xf86clearerr)
    SYMFUNC(xf86close)
    SYMFUNC(xf86cos)
    SYMFUNC(xf86exit)
    SYMFUNC(xf86exp)
    SYMFUNC(xf86fabs)
    SYMFUNC(xf86fclose)
    SYMFUNC(xf86feof)
    SYMFUNC(xf86ferror)
    SYMFUNC(xf86fflush)
    SYMFUNC(xf86fgetc)
    SYMFUNC(xf86fgetpos)
    SYMFUNC(xf86fgets)
    SYMFUNC(xf86finite)
    SYMFUNC(xf86floor)
    SYMFUNC(xf86fmod)
    SYMFUNC(xf86fopen)
    SYMFUNC(xf86fprintf)
    SYMFUNC(xf86fputc)
    SYMFUNC(xf86fputs)
    SYMFUNC(xf86fread)
    SYMFUNC(xf86free)
    SYMFUNC(xf86freopen)
    SYMFUNC(xf86frexp)
    SYMFUNC(xf86fscanf)
    SYMFUNC(xf86fseek)
    SYMFUNC(xf86fsetpos)
    SYMFUNC(xf86ftell)
    SYMFUNC(xf86fwrite)
    SYMFUNC(xf86getc)
    SYMFUNC(xf86getenv)
    SYMFUNC(xf86getpagesize)
    SYMFUNC(xf86hypot)
    SYMFUNC(xf86ioctl)
    SYMFUNC(xf86isalnum)
    SYMFUNC(xf86isalpha)
    SYMFUNC(xf86iscntrl)
    SYMFUNC(xf86isdigit)
    SYMFUNC(xf86isgraph)
    SYMFUNC(xf86islower)
    SYMFUNC(xf86isprint)
    SYMFUNC(xf86ispunct)
    SYMFUNC(xf86isspace)
    SYMFUNC(xf86isupper)
    SYMFUNC(xf86isxdigit)
    SYMFUNC(xf86labs)
    SYMFUNC(xf86ldexp)
    SYMFUNC(xf86log)
    SYMFUNC(xf86log10)
    SYMFUNC(xf86lseek)
    SYMFUNC(xf86malloc)
    SYMFUNC(xf86memchr)
    SYMFUNC(xf86memcmp)
    SYMFUNC(xf86memcpy)
    /*
     * Some compilers generate calls to memcpy to handle structure copies
     * or run-time initializations.
     */
    SYMFUNCALIAS("memcpy", xf86memcpy)
    SYMFUNC(xf86memset)
    /*
     * Some compilers generate calls to memset to handle aggregate
     * initializations.
     */
    SYMFUNCALIAS("memset", xf86memset)
    SYMFUNC(xf86memmove)
    SYMFUNC(xf86mmap)
    SYMFUNC(xf86modf)
    SYMFUNC(xf86munmap)
    SYMFUNC(xf86open)
    SYMFUNC(xf86perror)
    SYMFUNC(xf86pow)
    SYMFUNC(xf86printf)
    SYMFUNC(xf86qsort)
    SYMFUNC(xf86read)
    SYMFUNC(xf86realloc)
    SYMFUNC(xf86remove)
    SYMFUNC(xf86rename)
    SYMFUNC(xf86rewind)
    SYMFUNC(xf86setbuf)
    SYMFUNC(xf86setvbuf)
    SYMFUNC(xf86sin)
    SYMFUNC(xf86snprintf)
    SYMFUNC(xf86sprintf)
    SYMFUNC(xf86sqrt)
    SYMFUNC(xf86sscanf)
    SYMFUNC(xf86strcat)
    SYMFUNC(xf86strcmp)
    SYMFUNC(xf86strcasecmp)
    SYMFUNC(xf86strcpy)
    SYMFUNC(xf86strcspn)
    SYMFUNC(xf86strerror)
    SYMFUNC(xf86strlcat)
    SYMFUNC(xf86strlcpy)
    SYMFUNC(xf86strlen)
    SYMFUNC(xf86strncasecmp)
    SYMFUNC(xf86strncat)
    SYMFUNC(xf86strncmp)
    SYMFUNC(xf86strncpy)
    SYMFUNC(xf86strpbrk)
    SYMFUNC(xf86strchr)
    SYMFUNC(xf86strrchr)
    SYMFUNC(xf86strspn)
    SYMFUNC(xf86strstr)
    SYMFUNC(xf86strtod)
    SYMFUNC(xf86strtok)
    SYMFUNC(xf86strtol)
    SYMFUNC(xf86strtoul)
    SYMFUNC(xf86tan)
    SYMFUNC(xf86tmpfile)
    SYMFUNC(xf86tolower)
    SYMFUNC(xf86toupper)
    SYMFUNC(xf86ungetc)
    SYMFUNC(xf86vfprintf)
    SYMFUNC(xf86vsnprintf)
    SYMFUNC(xf86vsprintf)
    SYMFUNC(xf86write)

    /* non-ANSI C functions */
    SYMFUNC(xf86opendir)
    SYMFUNC(xf86closedir)
    SYMFUNC(xf86readdir)
    SYMFUNC(xf86rewinddir)
    SYMFUNC(xf86ffs)
    SYMFUNC(xf86strdup)
    SYMFUNC(xf86bzero)
    SYMFUNC(xf86usleep)
    SYMFUNC(xf86execl)

    SYMFUNC(xf86getsecs)
    SYMFUNC(xf86fpossize) /* for returning sizeof(fpos_t) */

    /* Some of these were added for DRI support. */
    SYMFUNC(xf86stat)
    SYMFUNC(xf86fstat)
    SYMFUNC(xf86access)
    SYMFUNC(xf86geteuid)
    SYMFUNC(xf86getegid)
    SYMFUNC(xf86getpid)
    SYMFUNC(xf86mknod)
    SYMFUNC(xf86chmod)
    SYMFUNC(xf86chown)
    SYMFUNC(xf86sleep)
    SYMFUNC(xf86mkdir)
    SYMFUNC(xf86shmget)
    SYMFUNC(xf86shmat)
    SYMFUNC(xf86shmdt)
    SYMFUNC(xf86shmctl)
#ifdef HAS_GLIBC_SIGSETJMP
    SYMFUNC(xf86setjmp)
    SYMFUNC(xf86setjmp0)
#if defined(__GLIBC__) && (__GLIBC__ >= 2)
    SYMFUNCALIAS("xf86setjmp1", __sigsetjmp)
#else
    SYMFUNC(xf86setjmp1)	/* For libc5 */
#endif
#else
    SYMFUNCALIAS("xf86setjmp", setjmp)
    SYMFUNC(xf86setjmp0)
    SYMFUNC(xf86setjmp1)
#endif
    SYMFUNCALIAS("xf86longjmp", longjmp)
    SYMFUNC(xf86getjmptype)
    SYMFUNC(xf86setjmp1_arg2)
    SYMFUNC(xf86setjmperror)
#ifdef XF86DRI
    /*
     * These may have more general uses, but for now, they are only used
     * by the DRI.  Loading them only when the DRI is built may make porting
     * (the non-DRI portions of the X server) easier.
     */
    SYMFUNC(xf86InstallSIGIOHandler)
    SYMFUNC(xf86RemoveSIGIOHandler)
# if defined(__alpha__) && defined(linux)
    SYMFUNC(_bus_base)
# endif
#endif
    SYMFUNC(xf86BlockSIGIO)
    SYMFUNC(xf86UnblockSIGIO)

#if defined(__alpha__)
    SYMFUNC(__divl)
    SYMFUNC(__reml)
    SYMFUNC(__divlu)
    SYMFUNC(__remlu)
    SYMFUNC(__divq)
    SYMFUNC(__divqu)
    SYMFUNC(__remq)
    SYMFUNC(__remqu)

# ifdef linux
    SYMFUNC(_outw)
    SYMFUNC(_outb)
    SYMFUNC(_outl)
    SYMFUNC(_inb)
    SYMFUNC(_inw)
    SYMFUNC(_inl)
    SYMFUNC(_alpha_outw)
    SYMFUNC(_alpha_outb)
    SYMFUNC(_alpha_outl)
    SYMFUNC(_alpha_inb)
    SYMFUNC(_alpha_inw)
    SYMFUNC(_alpha_inl)
# else
    SYMFUNC(outw)
    SYMFUNC(outb)
    SYMFUNC(outl)
    SYMFUNC(inb)
    SYMFUNC(inw)
    SYMFUNC(inl)
# endif
    SYMFUNC(xf86ReadMmio32)
    SYMFUNC(xf86ReadMmio16)
    SYMFUNC(xf86ReadMmio8)
    SYMFUNC(xf86WriteMmio32)
    SYMFUNC(xf86WriteMmio16)
    SYMFUNC(xf86WriteMmio8)
    SYMFUNC(xf86WriteMmioNB32)
    SYMFUNC(xf86WriteMmioNB16)
    SYMFUNC(xf86WriteMmioNB8)
#endif
#if defined(sun) && defined(SVR4)
    SYMFUNC(inb)
    SYMFUNC(inw)
    SYMFUNC(inl)
    SYMFUNC(outb)
    SYMFUNC(outw)
    SYMFUNC(outl)
#endif
#if defined(__powerpc__) && !defined(__OpenBSD__)
    SYMFUNC(inb)
    SYMFUNC(inw)
    SYMFUNC(inl)
    SYMFUNC(outb)
    SYMFUNC(outw)
    SYMFUNC(outl)
# if defined(NO_INLINE) || defined(Lynx)
    SYMFUNC(mem_barrier)
    SYMFUNC(ldl_u)
    SYMFUNC(eieio)
    SYMFUNC(ldl_brx)
    SYMFUNC(ldw_brx)
    SYMFUNC(stl_brx)
    SYMFUNC(stw_brx)
    SYMFUNC(ldq_u)
    SYMFUNC(ldw_u)
    SYMFUNC(stl_u)
    SYMFUNC(stq_u)
    SYMFUNC(stw_u)
    SYMFUNC(write_mem_barrier)
# endif
# if defined(Lynx)
    SYMFUNC(_restf14)
    SYMFUNC(_restf17)
    SYMFUNC(_restf18)
    SYMFUNC(_restf19)
    SYMFUNC(_restf20)
    SYMFUNC(_restf22)
    SYMFUNC(_restf23)
    SYMFUNC(_restf24)
    SYMFUNC(_restf25)
    SYMFUNC(_restf26)
    SYMFUNC(_restf27)
    SYMFUNC(_restf28)
    SYMFUNC(_restf29)
    SYMFUNC(_savef14)
    SYMFUNC(_savef17)
    SYMFUNC(_savef18)
    SYMFUNC(_savef19)
    SYMFUNC(_savef20)
    SYMFUNC(_savef22)
    SYMFUNC(_savef23)
    SYMFUNC(_savef24)
    SYMFUNC(_savef25)
    SYMFUNC(_savef26)
    SYMFUNC(_savef27)
    SYMFUNC(_savef28)
    SYMFUNC(_savef29)
# endif
# if PPCIO_DEBUG
    SYMFUNC(debug_inb)
    SYMFUNC(debug_inw)
    SYMFUNC(debug_inl)
    SYMFUNC(debug_outb)
    SYMFUNC(debug_outw)
    SYMFUNC(debug_outl)
# endif
#endif
#if defined(__GNUC__)
#if !defined(__UNIXOS2__) && !defined(Lynx)
    SYMFUNC(__div64)
#endif
#if !defined(Lynx)	/* FIXME: test on others than x86 and !3.1.0a/x86 */
    SYMFUNC(__divdf3)
#endif
    SYMFUNC(__divdi3)
#if !defined(Lynx)
    SYMFUNC(__divsf3)
    SYMFUNC(__divsi3)
#endif
    SYMFUNC(__moddi3)
#if !defined(Lynx)
    SYMFUNC(__modsi3)
#endif
#if !defined(__UNIXOS2__) && !defined(Lynx)
    SYMFUNC(__mul64)
#endif
#if !defined(Lynx)
    SYMFUNC(__muldf3)
#endif
    SYMFUNC(__muldi3)
#if !defined(Lynx)
    SYMFUNC(__mulsf3)
    SYMFUNC(__mulsi3)
    SYMFUNC(__udivdi3)
    SYMFUNC(__udivsi3)
#endif
    SYMFUNC(__umoddi3)
#if !defined(Lynx)
    SYMFUNC(__umodsi3)
#endif
#endif
#if defined(__ia64__)
    SYMFUNC(outw)
    SYMFUNC(outb)
    SYMFUNC(outl)
    SYMFUNC(inb)
    SYMFUNC(inw)
    SYMFUNC(inl)
#endif
#if defined(__arm__)
    SYMFUNC(outw)
    SYMFUNC(outb)
    SYMFUNC(outl)
    SYMFUNC(inb)
    SYMFUNC(inw)
    SYMFUNC(inl)
#endif

#ifdef __FreeBSD__
#if defined(__sparc__)
    SYMFUNC(_Qp_qtos)
    SYMFUNC(_Qp_qtod)
    SYMFUNC(_Qp_qtoux)
    SYMFUNC(_Qp_uitoq)
    SYMFUNC(_Qp_dtoq)
#endif
#endif

    /* Some variables. */

    SYMVAR(xf86stdin)
    SYMVAR(xf86stdout)
    SYMVAR(xf86stderr)
    SYMVAR(xf86errno)
    SYMVAR(xf86HUGE_VAL)

    /* General variables (from xf86.h) */
    SYMVAR(xf86ScreenIndex)
    SYMVAR(xf86PixmapIndex)
    SYMVAR(xf86Screens)
    SYMVAR(byte_reversed)
    SYMVAR(xf86inSuspend)
    /* debugging variables */
#ifdef BUILDDEBUG
    SYMVAR(xf86p8bit)
    SYMVAR(xf86DummyVar1)
    SYMVAR(xf86DummyVar2)
    SYMVAR(xf86DummyVar3)
#endif

    /* predefined resource lists from xf86Bus.h */
    SYMVAR(resVgaExclusive)
    SYMVAR(resVgaShared)
    SYMVAR(resVgaMemShared)
    SYMVAR(resVgaIoShared)
    SYMVAR(resVgaUnusedExclusive)
    SYMVAR(resVgaUnusedShared)
    SYMVAR(resVgaSparseExclusive)
    SYMVAR(resVgaSparseShared)
    SYMVAR(res8514Exclusive)
    SYMVAR(res8514Shared)
    SYMVAR(PciAvoid)

#if defined(__powerpc__) && (!defined(NO_INLINE) || defined(Lynx))
    SYMVAR(ioBase)
#endif

    /* Globals from xf86Globals.c and xf86Priv.h */
    SYMVAR(xf86ConfigDRI)

    /* Globals from xf86Configure.c */
    SYMVAR(ConfiguredMonitor)

    /* Pci.c */
    SYMVAR(pciNumBuses)
};
