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
/* $Xorg: miinitext.c,v 1.4 2001/02/09 02:05:21 xorgcvs Exp $ */

#include "misc.h"

#ifdef NOPEXEXT /* sleaze for Solaris cpp building XsunMono */
#undef PEXEXT
#endif
#ifdef PANORAMIX
extern Bool noPanoramiXExtension;
#endif
extern Bool noTestExtensions;
#ifdef XKB
extern Bool noXkbExtension;
#endif

#ifdef BEZIER
extern void BezierExtensionInit();
#endif
#ifdef XTESTEXT1
extern void XTestExtension1Init();
#endif
#ifdef SHAPE
extern void ShapeExtensionInit();
#endif
#ifdef EVI
extern void EVIExtensionInit();
#endif
#ifdef MITSHM
extern void ShmExtensionInit();
#endif
#ifdef PEXEXT
extern void PexExtensionInit();
#endif
#ifdef MULTIBUFFER
extern void MultibufferExtensionInit();
#endif
#ifdef PANORAMIX
extern void PanoramiXExtensionInit();
#endif
#ifdef XINPUT
extern void XInputExtensionInit();
#endif
#ifdef XTEST
extern void XTestExtensionInit();
#endif
#ifdef BIGREQS
extern void BigReqExtensionInit();
#endif
#ifdef MITMISC
extern void MITMiscExtensionInit();
#endif
#ifdef XIDLE
extern void XIdleExtensionInit();
#endif
#ifdef XTRAP
extern void DEC_XTRAPInit();
#endif
#ifdef SCREENSAVER
extern void ScreenSaverExtensionInit ();
#endif
#ifdef XV
extern void XvExtensionInit();
#endif
#ifdef XIE
extern void XieInit();
#endif
#ifdef XSYNC
extern void SyncExtensionInit();
#endif
#ifdef XKB
extern void XkbExtensionInit();
#endif
#ifdef XCMISC
extern void XCMiscExtensionInit();
#endif
#ifdef XRECORD
extern void XRecordExtensionInit();
#endif
#ifdef LBX
extern void LbxExtensionInit();
#endif
#ifdef DBE
extern void DbeExtensionInit();
#endif
#ifdef XAPPGROUP
extern void XagExtensionInit();
#endif
#ifdef XF86VIDMODE
extern void XFree86VidModeExtensionInit();
#endif
#ifdef XCSECURITY
extern void SecurityExtensionInit();
#endif
#ifdef XPRINT
extern void XpExtensionInit();
#endif
#ifdef TOGCUP
extern void XcupExtensionInit();
#endif
#ifdef DPMSExtension
extern void DPMSExtensionInit();
#endif


/*ARGSUSED*/
void
InitExtensions(argc, argv)
    int		argc;
    char	*argv[];
{
#ifdef PANORAMIX
#if !defined(PRINT_ONLY_SERVER) && !defined(NO_PANORAMIX)
  if (!noPanoramiXExtension) PanoramiXExtensionInit();
#endif
#endif
#ifdef BEZIER
    BezierExtensionInit();
#endif
#ifdef XTESTEXT1
    if (!noTestExtensions) XTestExtension1Init();
#endif
#ifdef SHAPE
    ShapeExtensionInit();
#endif
#ifdef MITSHM
    ShmExtensionInit();
#endif
#ifdef EVI
    EVIExtensionInit();
#endif
#ifdef PEXEXT
    PexExtensionInit();
#endif
#ifdef MULTIBUFFER
    MultibufferExtensionInit();
#endif
#ifdef XINPUT
    XInputExtensionInit();
#endif
#ifdef XTEST
    if (!noTestExtensions) XTestExtensionInit();
#endif
#ifdef BIGREQS
    BigReqExtensionInit();
#endif
#ifdef MITMISC
    MITMiscExtensionInit();
#endif
#ifdef XIDLE
    XIdleExtensionInit();
#endif
#ifdef XTRAP
    if (!noTestExtensions) DEC_XTRAPInit();
#endif
#if defined(SCREENSAVER) && !defined(PRINT_ONLY_SERVER)
    ScreenSaverExtensionInit ();
#endif
#ifdef XV
    XvExtensionInit();
#endif
#ifdef XIE
    XieInit();
#endif
#ifdef XSYNC
    SyncExtensionInit();
#endif
#if defined(XKB) && !defined(PRINT_ONLY_SERVER)
    if (!noXkbExtension) XkbExtensionInit();
#endif
#ifdef XCMISC
    XCMiscExtensionInit();
#endif
#ifdef XRECORD
    if (!noTestExtensions) RecordExtensionInit(); 
#endif
#ifdef LBX
    LbxExtensionInit();
#endif
#ifdef DBE
    DbeExtensionInit();
#endif
#ifdef XAPPGROUP
    XagExtensionInit();
#endif
#if defined(XF86VIDMODE) && !defined(PRINT_ONLY_SERVER)
    XFree86VidModeExtensionInit();
#endif
#ifdef XCSECURITY
    SecurityExtensionInit();
#endif
#ifdef XPRINT
    XpExtensionInit();
#endif
#ifdef TOGCUP
    XcupExtensionInit();
#endif
#if defined(DPMSExtension) && !defined(PRINT_ONLY_SERVER)
    DPMSExtensionInit();
#endif
}
