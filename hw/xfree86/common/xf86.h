/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86.h,v 3.47.2.8 1998/02/24 19:05:53 hohndel Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $Xorg: xf86.h,v 1.3 2000/08/17 19:50:28 cpqbld Exp $ */

#ifndef _XF86_H
#define _XF86_H

#include "misc.h"
#include "input.h"
#include "scrnintstr.h"

#include "xf86_Option.h"

/*
 * structure common for all modes
 */
typedef struct _DispM {
  struct _DispM	*prev,*next;
  char		*name;              /* identifier of this mode */
  /* These are the values that the user sees/provides */
  int		Clock;              /* pixel clock */
  int           HDisplay;           /* horizontal timing */
  int           HSyncStart;
  int           HSyncEnd;
  int           HTotal;
  int           HSkew;
  int           VDisplay;           /* vertical timing */
  int           VSyncStart;
  int           VSyncEnd;
  int           VTotal;
  int           Flags;
  /* These are the values the hardware uses */
  int		SynthClock;         /* Actual clock freq to be programmed */
  int		CrtcHDisplay;
  int		CrtcHSyncStart;
  int		CrtcHSyncEnd;
  int		CrtcHTotal;
  int		CrtcHSkew;
  int		CrtcVDisplay;
  int		CrtcVSyncStart;
  int		CrtcVSyncEnd;
  int		CrtcVTotal;
  Bool		CrtcHAdjusted;
  Bool		CrtcVAdjusted;
  int		PrivSize;
  INT32 	*Private;
} DisplayModeRec, *DisplayModePtr;

#define V_PHSYNC    0x0001
#define V_NHSYNC    0x0002
#define V_PVSYNC    0x0004
#define V_NVSYNC    0x0008
#define V_INTERLACE 0x0010
#define V_DBLSCAN   0x0020
#define V_CSYNC     0x0040
#define V_PCSYNC    0x0080
#define V_NCSYNC    0x0100
#define V_HSKEW     0x0200	/* hskew provided */
#define V_PIXMUX    0x1000
#define V_DBLCLK    0x2000
#define V_CLKDIV2   0x4000

/* The monitor description */

#define MAX_HSYNC 8
#define MAX_VREFRESH 8

typedef struct { float hi, lo; } range;

typedef struct {
   char *id;
   char *vendor;
   char *model;
   float EMPTY;
   int n_hsync;
   range hsync[MAX_HSYNC];       
   int n_vrefresh;                  
   range vrefresh[MAX_VREFRESH];
   DisplayModePtr Modes, Last; /* Start and end of monitor's mode list */
} MonRec, *MonPtr;

#define MAXCLOCKS   128
#define MAXDACSPEEDS  4  /* for <= 8, 16, 24, 32bpp */

/* Set default max allowed clock to 90MHz */
#define DEFAULT_MAX_CLOCK	90000

/*
 * the graphic device
 */
typedef struct {
  Bool           configured;
  int            tmpIndex;
  int            scrnIndex;
  Bool           (* Probe)(
#if NeedNestedPrototypes
    void
#endif
);
  Bool           (* Init)(
#if NeedNestedPrototypes
    int          scr_index,
    ScreenPtr    pScreen,
    int          argc,
    char         **argv
#endif
);
  int            (* ValidMode)(
#if NeedNestedPrototypes
    DisplayModePtr target,
    Bool verbose,
    int flag
#endif
);
  void           (* EnterLeaveVT)(
#if NeedNestedPrototypes
    int,
    int
#endif
);
  void           (* EnterLeaveMonitor)(
#if NeedNestedPrototypes
    int
#endif
);
  void           (* EnterLeaveCursor)(
#if NeedNestedPrototypes
    int
#endif
);
  void           (* AdjustFrame)(
#if NeedNestedPrototypes
    int x,
    int y
#endif
);
  Bool           (* SwitchMode)(
#if NeedNestedPrototypes
    DisplayModePtr modes
#endif
);
  void           (* DPMSSet)(
#if NeedNestedPrototypes
    int level
#endif
);
  void           (* PrintIdent)(
#if NeedNestedPrototypes
    void
#endif
);
  int            depth;
  xrgb		 weight;
  int            bitsPerPixel;
  int            defaultVisual;
  int            virtualX,virtualY; 
  int		 displayWidth;
  int            frameX0, frameY0, frameX1, frameY1;
  OFlagSet       options;
  OFlagSet       clockOptions;
  OFlagSet	 xconfigFlag;
  char           *chipset;
  char           *ramdac;
  int            dacSpeeds[MAXDACSPEEDS];
  int            dacSpeedBpp;
  int            clocks;
  int            clock[MAXCLOCKS];
  int            maxClock;
  int            videoRam;
  int            BIOSbase;                 /* Base address of video BIOS */
  unsigned long  MemBase;                  /* Frame buffer base address */
  int            width, height;            /* real display dimensions */
  unsigned long  speedup;                  /* Use SpeedUp code */
  DisplayModePtr modes;
  MonPtr         monitor;
  char           *clockprog;
  int            textclock;
  Bool           bankedMono;	  /* display supports banking for mono server */
  char           *name;
  xrgb           blackColour;
  xrgb           whiteColour;
  int            *validTokens;
  char           *patchLevel;
  unsigned int   IObase;          /* AGX - video card I/O reg base        */
  unsigned int   DACbase;         /* AGX - dac I/O reg base               */
  unsigned long  COPbase;         /* AGX - coprocessor memory base        */
  unsigned int   POSbase;         /* AGX - I/O address of POS regs        */
  int            instance;        /* AGX - XGA video card instance number */
  int            s3Madjust;
  int            s3Nadjust;
  int            s3MClk;
  int            chipID;
  int            chipRev;
  unsigned long  VGAbase;         /* AGX - 64K aperture memory address    */
  int            s3RefClk;
  int            s3BlankDelay;
  int            textClockFreq;
  char          *DCConfig;
  char          *DCOptions;
  int            MemClk;          /* General flag used for memory clocking */
  int            LCDClk;
#ifdef XFreeXDGA
  int            directMode;
  void           (*setBank)(
#if NeedNestedPrototypes
    int
#endif
  );
  unsigned long  physBase;
  int            physSize;
#endif
#ifdef XF86SETUP
  void           *device;	/* This should be GDevPtr, but it causes
				   problems with include file order */
#endif
} ScrnInfoRec, *ScrnInfoPtr;

typedef struct {
  int           token;                /* id of the token */
  char          *name;                /* pointer to the LOWERCASED name */
} SymTabRec, *SymTabPtr;

#define VGA_DRIVER  1
#define V256_DRIVER 2
#define WGA_DRIVER  3
#define XGA_DRIVER  4

#define ENTER       1
#define LEAVE       0

/* These are possible return values for xf86CheckMode() and ValidMode() */
#define MODE_OK	    0
#define MODE_HSYNC  1		/* hsync out of range */
#define MODE_VSYNC  2		/* vsync out of range */
#define MODE_BAD    255		/* unspecified reason */

/* These are the possible flags for ValidMode */
#define MODE_USED	1	/* this mode is really being used in the */
				/* modes line of the Display Subsection  */
#define MODE_SUGGESTED	2	/* this mode is included in the available*/
				/* modes in the Monitor Section */
#define MODE_VID	3	/* this is called from the VidMode extension */

/* Indicates the level of DPMS support */
typedef enum {
    DPMSSupportUnknown,
    DPMSNotSupported,
    DPMSFullSupport
} DPMSSupportStatus;

/* flags for xf86LookupMode */
#define LOOKUP_DEFAULT		0	/* Use default mode lookup method */
#define LOOKUP_BEST_REFRESH	1	/* Pick modes with best refresh */
#define LOOKUP_NO_INTERLACED	2	/* Ignore interlaced modes */
#define LOOKUP_FORCE_DEFAULT	4	/* Force default lookup method */

#define INTERLACE_REFRESH_WEIGHT	1.5

/* SpeedUp options */

#define SPEEDUP_FILLBOX		1
#define SPEEDUP_FILLRECT	2
#define	SPEEDUP_BITBLT		4
#define SPEEDUP_LINE		8
#define SPEEDUP_TEGBLT8      0x10
#define SPEEDUP_RECTSTIP     0x20

/*
 * This is the routines where SpeedUp is quicker than fXF86.  The problem is
 * that the SpeedUp fillbox is better for drawing vertical and horizontal
 * line segments, and the fXF86 version is significantly better for
 * more general lines
 */
#define SPEEDUP_BEST		(SPEEDUP_FILLRECT | SPEEDUP_BITBLT | \
				 SPEEDUP_LINE | SPEEDUP_TEGBLT8 | \
				 SPEEDUP_RECTSTIP)
/*
#define SPEEDUP_BEST		(SPEEDUP_FILLBOX | SPEEDUP_FILLRECT | \
				 SPEEDUP_BITBLT | SPEEDUP_LINE | \
                                 SPEEDUP_TEGBLT8 | SPEEDUP_RECTSTIP)
*/

/*
 * SpeedUp routines which are not dependent on the screen virtual resolution
 */
#ifndef SPEEDUP_ANYWIDTH
#define SPEEDUP_ANYWIDTH	(SPEEDUP_FILLRECT | SPEEDUP_BITBLT | \
                                 SPEEDUP_LINE | SPEEDUP_FILLBOX)
#endif

/*
 * SpeedUp routines which are not dependent on ET4000
 */
#ifndef SPEEDUP_ANYCHIPSET
#define SPEEDUP_ANYCHIPSET	(SPEEDUP_TEGBLT8 | SPEEDUP_RECTSTIP)
#endif

/* All SpeedUps */
#define SPEEDUP_ALL		(SPEEDUP_FILLBOX | SPEEDUP_FILLRECT | \
				 SPEEDUP_BITBLT | SPEEDUP_LINE | \
                                 SPEEDUP_TEGBLT8 | SPEEDUP_RECTSTIP)

/* SpeedUp flags used if SpeedUp is not in XF86Config */
#define SPEEDUP_DEFAULT		SPEEDUP_ALL

extern Bool        xf86VTSema;

/* Mouse device private record */

#define MSE_MAPTOX		(-1)
#define MSE_MAPTOY		(-2)
#define MSE_MAXBUTTONS		12
#define MSE_DFLTBUTTONS		3

typedef struct _MouseDevRec {
    DeviceProc    mseProc;              /* procedure for initializing */
    void          (* mseEvents)(
#if NeedNestedPrototypes
			struct _MouseDevRec *
#endif
				);      /* proc for processing events */
    DeviceIntPtr  device;
    int           mseFd;
    char          *mseDevice;
    int           mseType;
    int           mseModel;
    int           baudRate;
    int           oldBaudRate;
    int           sampleRate;
    int           lastButtons;
    int           threshold, num, den;  /* acceleration */
    int           buttons;		/* # of buttons */
    int           emulateState;         /* automata state for 2 button mode */
    Bool          emulate3Buttons;
    int           emulate3Timeout;      /* Timeout for 3 button emulation */
    Bool          chordMiddle;
    int           mouseFlags;		/* Flags to Clear after opening mouse dev */
    int		  truebuttons;		/* Arg to maintain before emulate3buttons timer callback */

    int           resolution;
    int           negativeZ;
    int           positiveZ;
#ifndef MOUSE_PROTOCOL_IN_KERNEL
    unsigned char protoPara[7];
#endif
    
#ifndef CSRG_BASED
    /* xque part */
    int           xquePending;		/* was xqueFd, but nothing uses that */
    int           xqueSema;
#endif
#ifdef XINPUT
    struct _LocalDeviceRec	*local;
#endif
} MouseDevRec, *MouseDevPtr;

#ifdef XINPUT
#define MOUSE_DEV(dev) (MouseDevPtr) PRIVATE(dev)
#else
#define MOUSE_DEV(dev) (MouseDevPtr) (dev)->public.devicePrivate
#endif

/* Global data */
/* xf86Init.c */
extern double xf86rGamma, xf86gGamma, xf86bGamma;

#ifdef XF86VIDMODE
extern Bool xf86VidModeEnabled;
extern Bool xf86VidModeAllowNonLocal;
#endif
#ifdef XF86MISC
extern Bool xf86MiscModInDevEnabled;
extern Bool xf86MiscModInDevAllowNonLocal;
#endif

/* PCI probe flags */


typedef enum {
    PCIProbe1 = 0,
    PCIProbe2,
    PCIForceConfig1,
    PCIForceConfig2
} PciProbeType;

extern PciProbeType xf86PCIFlags;

/* Function Prototypes */
#ifndef _NO_XF86_PROTOTYPES

/* xf86Init.c */
void InitOutput(
#if NeedFunctionPrototypes
    ScreenInfo *pScreenInfo,
    int argc,
    char **argv
#endif
);

void InitInput(
#if NeedFunctionPrototypes
    int argc,
    char **argv
#endif
);

void ddxGiveUp(
#if NeedFunctionPrototypes
    void
#endif
);

void AbortDDX(
#if NeedFunctionPrototypes
    void
#endif
);

int ddxProcessArgument(
#if NeedFunctionPrototypes
    int argc,
    char *argv[],
    int i
#endif
);

void ddxUseMsg(
#if NeedFunctionPrototypes
    void
#endif
);

/* xf86Config.c */
unsigned int StrToUL(
#if NeedFunctionPrototypes
    char *str
#endif
);

#ifndef CONFIG_RETURN_TYPE
#ifdef XF86SETUP
#define CONFIG_RETURN_TYPE int
#else
#define CONFIG_RETURN_TYPE void
#endif
#endif

CONFIG_RETURN_TYPE xf86Config(
#if NeedFunctionPrototypes
    int vtopen
#endif
);

CONFIG_RETURN_TYPE configPointerSection(
#if NeedFunctionPrototypes
    MouseDevPtr /*mouse_dev*/,
    int /*end_tag*/,
    char** /*devicename*/
#endif
);

Bool xf86LookupMode(
#if NeedFunctionPrototypes
    DisplayModePtr target,
    ScrnInfoPtr driver,
    int flags
#endif
);

void xf86VerifyOptions(
#if NeedFunctionPrototypes
    OFlagSet *allowedOptions,
    ScrnInfoPtr driver
#endif
);

int xf86CheckMode(
#if NeedFunctionPrototypes
    ScrnInfoPtr scrp,
    DisplayModePtr dispmp,
    MonPtr monp,
    int verbose
#endif
);

int xf86GetNearestClock(
#if NeedFunctionPrototypes
    ScrnInfoPtr Screen,
    int Frequency
#endif
);

/* xf86Cursor.c */
void xf86InitViewport(
#if NeedFunctionPrototypes
    ScrnInfoPtr pScr
#endif
);

void xf86SetViewport(
#if NeedFunctionPrototypes
    ScreenPtr pScreen,
    int x,
    int y
#endif
);

void xf86LockZoom(
#if NeedFunctionPrototypes
    ScreenPtr pScreen,
    int lock
#endif
);

void xf86ZoomViewport(
#if NeedFunctionPrototypes
    ScreenPtr pScreen,
    int zoom
#endif
);

/* xf86Dl.c */
void*
xf86LoadModule(
#if NeedFunctionPrototypes
	const char *	file,
	const char *	path
#endif
);

/* xf86Events.c */
int TimeSinceLastInputEvent(
#if NeedFunctionPrototypes
    void
#endif
);

void SetTimeSinceLastInputEvent(
#if NeedFunctionPrototypes
    void
#endif
);

void ProcessInputEvents(
#if NeedFunctionPrototypes
    void
#endif
);

void xf86PostKbdEvent(
#if NeedFunctionPrototypes
    unsigned key
#endif
);

void xf86PostMseEvent(
#if NeedFunctionPrototypes
    DeviceIntPtr device,
    int buttons,
    int dx,
    int dy
#endif
);

void xf86Block(
#if NeedFunctionPrototypes
    pointer blockData,
    OSTimePtr pTimeout,
    pointer pReadmask
#endif
);

void xf86Wakeup(
#if NeedFunctionPrototypes
    pointer blockData,
    int err,
    pointer pReadmask
#endif
);

void xf86SigHandler(
#if NeedFunctionPrototypes
    int signo
#endif
);

/* xf86Io.c */
void xf86KbdBell(
#if NeedFunctionPrototypes
    int percent,
    DeviceIntPtr pKeyboard,
    pointer ctrl,
    int unused
#endif
);

void xf86KbdLeds(
#if NeedFunctionPrototypes
    void
#endif
);

void xf86KbdCtrl(
#if NeedFunctionPrototypes
    DevicePtr pKeyboard,
    KeybdCtrl *ctrl
#endif
);

void xf86InitKBD(
#if NeedFunctionPrototypes
    Bool init
#endif
);

int xf86KbdProc(
#if NeedFunctionPrototypes
    DeviceIntPtr pKeyboard,
    int what
#endif
);

void xf86MseCtrl(
#if NeedFunctionPrototypes
    DevicePtr pPointer,
    PtrCtrl *ctrl
#endif
);

int GetMotionEvents(
#if NeedFunctionPrototypes
    DeviceIntPtr,
    xTimecoord *,
    unsigned long,
    unsigned long,
    ScreenPtr
#endif
);

int xf86MseProc(
#if NeedFunctionPrototypes
    DeviceIntPtr pPointer,
    int what
#endif
);

void xf86MseEvents(
#if NeedFunctionPrototypes
        MouseDevPtr mouse
#endif
);

CARD32 GetTimeInMillis(
#if NeedFunctionPrototypes
    void
#endif
);

void OsVendorInit(
#if NeedFunctionPrototypes
    void
#endif
);

/* xf86_Mouse.c */
Bool xf86MouseSupported(
#if NeedFunctionPrototypes
    int mousetype
#endif
);

void xf86SetupMouse(
#if NeedFunctionPrototypes
    MouseDevPtr mouse
#endif
);

void xf86MouseProtocol(
#if NeedFunctionPrototypes
    DeviceIntPtr device,
    unsigned char *rBuf,
    int nBytes
#endif
);

#ifdef XINPUT
void xf86MouseCtrl(
#if NeedFunctionPrototypes
     DeviceIntPtr device,
     PtrCtrl   *ctrl
#endif
);
#endif

/* xf86_PnPMouse.c */
int xf86GetPnPMouseProtocol(
#if NeedFunctionPrototypes
    MouseDevPtr mouse
#endif
);

/* xf86Kbd.c */
Bool LegalModifier(
#if NeedFunctionPrototypes
    unsigned int key,
    DevicePtr pDev
#endif
);

void xf86KbdGetMapping(
#if NeedFunctionPrototypes
    KeySymsPtr pKeySyms,
    CARD8 *pModMap
#endif
);
#endif /* _NO_XF86_PROTOTYPES */

/* End of Prototypes */

#endif /* _XF86_H */


