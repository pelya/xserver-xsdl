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

typedef struct _rrMode {
    struct _rrMode  *next;
    int		    id;
    Bool	    referenced;
    Bool	    oldReferenced;
    xRRMonitorMode  mode;
} RRMode, *RRModePtr;

typedef struct _rrMonitor {
    struct _rrMonitor	*next;
    ScreenPtr	    pScreen;
    RRModePtr	    pModes;
    void	    *identifier;    /* made unique by DDX */
    Bool	    referenced;
    
    /*
     * Current state
     */
    int		    mode;
    int		    x, y;
    Rotation	    rotation;
} RRMonitor, *RRMonitorPtr;

typedef Bool (*RRSetScreenSizeProcPtr) (ScreenPtr	pScreen,
					CARD16		width,
					CARD16		height,
					CARD32		widthInMM,
					CARD32		heightInMM);
					
typedef Bool (*RRSetModeProcPtr) (ScreenPtr		pScreen,
				  int			monitor,
				  RRModePtr		pMode,
				  int			x,
				  int			y,
				  Rotation		rotation);

typedef Bool (*RRGetInfoProcPtr) (ScreenPtr pScreen, Rotation *rotations);
typedef Bool (*RRCloseScreenProcPtr) ( int i, ScreenPtr pscreen);


#ifdef RANDR_SCREEN_INTERFACE

typedef void *RRScreenSizePtr;

typedef Bool (*RRSetConfigProcPtr) (ScreenPtr		pScreen,
				    Rotation		rotation,
				    int			rate,
				    RRScreenSizePtr	pSize);

#endif
	

typedef struct _rrScrPriv {
    RRSetModeProcPtr	    rrSetMode;
    RRGetInfoProcPtr	    rrGetInfo;
    RRCloseScreenProcPtr    CloseScreen;
    
    TimeStamp		    lastSetTime;	/* last changed by client */
    TimeStamp		    lastConfigTime;	/* possible configs changed */

    /*
     * monitor data
     */
    RRMonitorPtr	    pMonitors;

#ifdef RANDR_SCREEN_INTERFACE
    /*
     * Configuration information
     */
    Rotation		    rotations;
    
    RRSetConfigProcPtr	    rrSetConfig;

    Rotation		    rotation;
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
 * Then, register a monitor with the screen
 */

RRMonitorPtr
RRRegisterMonitor (ScreenPtr		pScreen,
		   void			*identifier,
		   Rotation		rotations);

/*
 * Next, register the list of modes with the monitor
 */

RRModePtr
RRRegisterMode (RRMonitorPtr	pMonitor,
		xRRMonitorMode	*pMode,
		char		*name,
		int		nameLength);

/*
 * Finally, set the current configuration of each monitor
 */

void
RRSetCurrentMode (RRMonitorPtr	pMonitor,
		  RRModePtr	pMode,
		  Rotation	rotation);

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
miRRSetMode (ScreenPtr	pScreen,
	     int	monitor,
	     RRModePtr	pMode,
	     Rotation	rotation);

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

Bool RRRegisterRotation (ScreenPtr	pScreen,
			 Rotation	rotation);

/*
 * Finally, set the current configuration of the screen
 */

void
RRSetCurrentConfig (ScreenPtr		pScreen,
		    Rotation		rotation,
		    int			rate,
		    RRScreenSizePtr	pSize);

int
RRSetScreenConfig (ScreenPtr		pScreen,
		   Rotation		rotation,
		   int			rate,
		   RRScreenSizePtr	pSize);

Bool
miRRSetConfig (ScreenPtr	pScreen,
	       Rotation		rotation,
	       int		rate,
	       RRScreenSizePtr	size);

#endif					
#endif /* _RANDRSTR_H_ */
