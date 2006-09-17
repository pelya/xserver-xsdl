/*
 * Copyright © 2000 Compaq Computer Corporation
 * Copyright © 2002 Hewlett-Packard Company
 * Copyright © 2006 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * Author:  Jim Gettys, Hewlett-Packard Company, Inc.
 *	    Keith Packard, Intel Corporation
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _RANDRSTR_H_
#define _RANDRSTR_H_

#include <X11/extensions/randrproto.h>

/* required for ABI compatibility for now */
#define RANDR_SCREEN_INTERFACE 1

typedef XID	RRMode;
typedef XID	RROutput;
typedef XID	RRCrtc;

/*
 * Modeline for a monitor. Name follows directly after this struct
 */

#define RRModeName(pMode) ((char *) (pMode + 1))
typedef struct _rrMode	    RRModeRec, *RRModePtr;
typedef struct _rrCrtc	    RRCrtcRec, *RRCrtcPtr;
typedef struct _rrOutput    RROutputRec, *RROutputPtr;

struct _rrMode {
    RRMode	    id;
    int		    refcnt;
    xRRModeInfo	    mode;
    char	    *name;
};

struct _rrCrtc {
    RRCrtc	    id;
    ScreenPtr	    pScreen;
    RRModePtr	    mode;
    int		    x, y;
    Rotation	    rotation;
    Rotation	    rotations;
    int		    numPossibleOutputs;
    RROutputPtr	    *possibleOutputs;
    Bool	    changed;
    void	    *devPrivate;
};

struct _rrOutput {
    RROutput	    id;
    ScreenPtr	    pScreen;
    char	    *name;
    int		    nameLength;
    CARD8	    connection;
    CARD8	    subpixelOrder;
    RRCrtcPtr	    crtc;
    int		    numCrtcs;
    RRCrtcPtr	    *crtcs;
    int		    numClones;
    RROutputPtr	    *outputs;
    int		    numModes;
    RRModePtr	    *modes;
    Bool	    changed;
    void	    *devPrivate;
};

typedef Bool (*RRScreenSetSizeProcPtr) (ScreenPtr	pScreen,
					CARD16		width,
					CARD16		height,
					CARD32		widthInMM,
					CARD32		heightInMM);
					
typedef Bool (*RRCrtcSetProcPtr) (ScreenPtr		pScreen,
				  RRCrtcPtr		crtc,
				  RRModePtr		mode,
				  int			x,
				  int			y,
				  Rotation		rotation,
				  int			numOutput,
				  RROutputPtr		*outputs);

typedef Bool (*RRGetInfoProcPtr) (ScreenPtr pScreen, Rotation *rotations);
typedef Bool (*RRCloseScreenProcPtr) ( int i, ScreenPtr pscreen);

/* These are for 1.0 compatibility */
 
typedef struct _rrRefresh {
    CARD16	    rate;
    RRModePtr	    mode;
} RRScreenRate, *RRScreenRatePtr;

typedef struct _rrScreenSize {
    int		    id;
    short	    width, height;
    short	    mmWidth, mmHeight;
    int		    nRates;
    RRScreenRatePtr pRates;
} RRScreenSize, *RRScreenSizePtr;

#ifdef RANDR_SCREEN_INTERFACE

typedef Bool (*RRSetConfigProcPtr) (ScreenPtr		pScreen,
				    Rotation		rotation,
				    int			rate,
				    RRScreenSizePtr	pSize);

#endif
	

typedef struct _rrScrPriv {
    /*
     * 'public' part of the structure; DDXen fill this in
     * as they initialize
     */
#ifdef RANDR_SCREEN_INTERFACE
    RRSetConfigProcPtr	    rrSetConfig;
#endif
    RRGetInfoProcPtr	    rrGetInfo;
    RRScreenSetSizeProcPtr  rrScreenSetSize;
    RRCrtcSetProcPtr	    rrCrtcSet;
    
    /*
     * Private part of the structure; not considered part of the ABI
     */
    TimeStamp		    lastSetTime;	/* last changed by client */
    TimeStamp		    lastConfigTime;	/* possible configs changed */
    RRCloseScreenProcPtr    CloseScreen;
    Bool		    changed;
    CARD16		    minWidth, minHeight;
    CARD16		    maxWidth, maxHeight;

    /* modes, outputs and crtcs */
    int			    numModes;
    RRModePtr		    *modes;

    int			    numOutputs;
    RROutputPtr		    *outputs;

    int			    numCrtcs;
    RRCrtcPtr		    *crtcs;

#ifdef RANDR_SCREEN_INTERFACE
    /*
     * Configuration information
     */
    Rotation		    rotations;
    CARD16		    reqWidth, reqHeight;
    
    int			    nSizes;
    RRScreenSizePtr	    pSizes;
    
    RRScreenSizePtr	    pSize;
    Rotation		    rotation;
    int			    rate;
    int			    size;
#endif
} rrScrPrivRec, *rrScrPrivPtr;

extern int rrPrivIndex;

#define rrGetScrPriv(pScr)  ((rrScrPrivPtr) (pScr)->devPrivates[rrPrivIndex].ptr)
#define rrScrPriv(pScr)	rrScrPrivPtr    pScrPriv = rrGetScrPriv(pScr)
#define SetRRScreen(s,p) ((s)->devPrivates[rrPrivIndex].ptr = (pointer) (p))

/* Initialize the extension */
void
RRExtensionInit (void);

/*
 * Set the range of sizes for the screen
 */
void
RRScreenSetSizeRange (ScreenPtr	pScreen,
		      CARD16	minWidth,
		      CARD16	minHeight,
		      CARD16	maxWidth,
		      CARD16	maxHeight);

/*
 * Create a CRTC
 */
RRCrtcPtr
RRCrtcCreate (ScreenPtr	pScreen,
	      void	*devPrivate);


/*
 * Use this value for any num parameter to indicate that
 * the related data are unchanged
 */
#define RR_NUM_UNCHANGED    -1

/*
 * Notify the extension that the Crtc has been reconfigured
 */
Bool
RRCrtcSet (RRCrtcPtr	crtc,
	   RRModePtr	mode,
	   int		x,
	   int		y,
	   Rotation	rotation,
	   int		numOutput,
	   RROutputPtr	*outputs);

/*
 * Destroy a Crtc at shutdown
 */
void
RRCrtcDestroy (RRCrtcPtr crtc);

/*
 * Find, and if necessary, create a mode
 */

RRModePtr
RRModeGet (ScreenPtr	pScreen,
	   xRRModeInfo	*modeInfo,
	   char		*name);

/*
 * Destroy a mode.
 */

void
RRModeDestroy (RRModePtr mode);

/*
 * Create an output
 */

RROutputPtr
RROutputCreate (ScreenPtr   pScreen,
		char	    *name,
		int	    nameLength,
		void	    *devPrivate);

/*
 * Notify extension that output parameters have been changed
 */
Bool
RROutputSet (RROutputPtr    output,
	     RROutputPtr    *clones,
	     int	    numClones,
	     RRModePtr	    *modes,
	     int	    numModes,
	     RRCrtcPtr	    *crtcs,
	     int	    numCrtcs,
	     CARD8	    connection);

void
RROutputDestroy (RROutputPtr	output);

void
RRTellChanged (ScreenPtr pScreen);

Bool RRScreenInit(ScreenPtr pScreen);

Rotation
RRGetRotation (ScreenPtr pScreen);

Bool
miRandRInit (ScreenPtr pScreen);

Bool
miRRGetInfo (ScreenPtr pScreen, Rotation *rotations);

Bool
miRRGetScreenInfo (ScreenPtr pScreen);

Bool
miRRCrtcSet (ScreenPtr	pScreen,
	     RRCrtcPtr	crtc,
	     RRModePtr	mode,
	     int	x,
	     int	y,
	     Rotation	rotation,
	     int	numOutput,
	     RROutputPtr    *outputs);

#ifdef RANDR_SCREEN_INTERFACE					
/*
 * This is the old interface, deprecated but left
 * around for compatibility
 */

/*
 * Then, register the specific size with the screen
 */

RRScreenSizePtr
RRRegisterSize (ScreenPtr		pScreen,
		short			width, 
		short			height,
		short			mmWidth,
		short			mmHeight);

Bool RRRegisterRate (ScreenPtr		pScreen,
		     RRScreenSizePtr	pSize,
		     int		rate);

/*
 * Finally, set the current configuration of the screen
 */

void
RRSetCurrentConfig (ScreenPtr		pScreen,
		    Rotation		rotation,
		    int			rate,
		    RRScreenSizePtr	pSize);

Bool RRScreenInit (ScreenPtr pScreen);

Rotation
RRGetRotation (ScreenPtr pScreen);

int
RRSetScreenConfig (ScreenPtr		pScreen,
		   Rotation		rotation,
		   int			rate,
		   RRScreenSizePtr	pSize);

#endif					
#endif /* _RANDRSTR_H_ */
