/*
 * Copyright 2006 Luc Verhaegen.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "xf86DDC.h"
#include <X11/Xatom.h>
#include "property.h"
#include "propertyst.h"
#include "xf86DDC.h"

/*
 * xf86Mode.c should have a some more DisplayModePtr list handling.
 */
static DisplayModePtr
xf86ModesAdd(DisplayModePtr Modes, DisplayModePtr Additions)
{
    if (!Modes) {
        if (Additions)
            return Additions;
        else
            return NULL;
    }

    if (Additions) {
        DisplayModePtr Mode = Modes;

        while (Mode->next)
            Mode = Mode->next;
        
        Mode->next = Additions;
        Additions->prev = Mode;
    }

    return Modes;
}

static DisplayModePtr
xf86ModeCopy(DisplayModePtr Mode)
{
    DisplayModePtr New;

    if (!Mode)
        return NULL;

    New = xnfalloc(sizeof(DisplayModeRec));

    memcpy(New, Mode, sizeof(DisplayModeRec));
    
    New->name = xnfalloc(strlen(Mode->name) + 1);
    memcpy(New->name, Mode->name, strlen(Mode->name) + 1);

    /* We ignore privates as DDC code doesn't use it currently */
    return New;
}

/*
 * Temporary.
 */
static void
add(char **p, char *new)
{
    *p = xnfrealloc(*p, strlen(*p) + strlen(new) + 2);
    strcat(*p, " ");
    strcat(*p, new);
}

static void
PrintModeline(int scrnIndex,DisplayModePtr mode)
{
    char tmp[256];
    char *flags = xnfcalloc(1, 1);

    if (mode->HSkew) { 
	snprintf(tmp, 256, "hskew %i", mode->HSkew); 
	add(&flags, tmp);
    }
    if (mode->VScan) { 
	snprintf(tmp, 256, "vscan %i", mode->VScan); 
	add(&flags, tmp);
    }
    if (mode->Flags & V_INTERLACE) add(&flags, "interlace");
    if (mode->Flags & V_CSYNC) add(&flags, "composite");
    if (mode->Flags & V_DBLSCAN) add(&flags, "doublescan");
    if (mode->Flags & V_BCAST) add(&flags, "bcast");
    if (mode->Flags & V_PHSYNC) add(&flags, "+hsync");
    if (mode->Flags & V_NHSYNC) add(&flags, "-hsync");
    if (mode->Flags & V_PVSYNC) add(&flags, "+vsync");
    if (mode->Flags & V_NVSYNC) add(&flags, "-vsync");
    if (mode->Flags & V_PCSYNC) add(&flags, "+csync");
    if (mode->Flags & V_NCSYNC) add(&flags, "-csync");
#if 0
    if (mode->Flags & V_CLKDIV2) add(&flags, "vclk/2");
#endif
    xf86DrvMsgVerb(scrnIndex, X_INFO, 3,
		   "Modeline \"%s\"  %6.2f  %i %i %i %i  %i %i %i %i%s\n",
		   mode->name, mode->Clock/1000., mode->HDisplay,
		   mode->HSyncStart, mode->HSyncEnd, mode->HTotal,
		   mode->VDisplay, mode->VSyncStart, mode->VSyncEnd,
		   mode->VTotal, flags);
    xfree(flags);
}

/*
 * TODO:
 *  - for those with access to the VESA DMT standard; review please.
 */
#define MODEPREFIX(name) NULL, NULL, name, 0,M_T_DRIVER
#define MODESUFFIX   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,FALSE,FALSE,0,NULL,0,0.0,0.0

DisplayModeRec DDCEstablishedModes[17] = {
    { MODEPREFIX("800x600"),    40000,  800,  840,  968, 1056, 0,  600,  601,  605,  628, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX }, /* 800x600@60Hz */
    { MODEPREFIX("800x600"),    36000,  800,  824,  896, 1024, 0,  600,  601,  603,  625, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX }, /* 800x600@56Hz */
    { MODEPREFIX("640x480"),    31500,  640,  656,  720,  840, 0,  480,  481,  484,  500, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX }, /* 640x480@75Hz */
    { MODEPREFIX("640x480"),    31500,  640,  664,  704,  832, 0,  480,  489,  491,  520, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX }, /* 640x480@72Hz */
    { MODEPREFIX("640x480"),    30240,  640,  704,  768,  864, 0,  480,  483,  486,  525, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX }, /* 640x480@67Hz */
    { MODEPREFIX("640x480"),    25200,  640,  656,  752,  800, 0,  480,  490,  492,  525, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX }, /* 640x480@60Hz */
    { MODEPREFIX("720x400"),    35500,  720,  738,  846,  900, 0,  400,  421,  423,  449, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX }, /* 720x400@88Hz */
    { MODEPREFIX("720x400"),    28320,  720,  738,  846,  900, 0,  400,  412,  414,  449, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX }, /* 720x400@70Hz */
    { MODEPREFIX("1280x1024"), 135000, 1280, 1296, 1440, 1688, 0, 1024, 1025, 1028, 1066, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX }, /* 1280x1024@75Hz */
    { MODEPREFIX("1024x768"),   78800, 1024, 1040, 1136, 1312, 0,  768,  769,  772,  800, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX }, /* 1024x768@75Hz */
    { MODEPREFIX("1024x768"),   75000, 1024, 1048, 1184, 1328, 0,  768,  771,  777,  806, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX }, /* 1024x768@70Hz */
    { MODEPREFIX("1024x768"),   65000, 1024, 1048, 1184, 1344, 0,  768,  771,  777,  806, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX }, /* 1024x768@60Hz */
    { MODEPREFIX("1024x768"),   44900, 1024, 1032, 1208, 1264, 0,  768,  768,  776,  817, 0, V_PHSYNC | V_PVSYNC | V_INTERLACE, MODESUFFIX }, /* 1024x768@43Hz */
    { MODEPREFIX("832x624"),    57284,  832,  864,  928, 1152, 0,  624,  625,  628,  667, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX }, /* 832x624@75Hz */
    { MODEPREFIX("800x600"),    49500,  800,  816,  896, 1056, 0,  600,  601,  604,  625, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX }, /* 800x600@75Hz */
    { MODEPREFIX("800x600"),    50000,  800,  856,  976, 1040, 0,  600,  637,  643,  666, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX }, /* 800x600@72Hz */
    { MODEPREFIX("1152x864"),  108000, 1152, 1216, 1344, 1600, 0,  864,  865,  868,  900, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX }, /* 1152x864@75Hz */
};

static DisplayModePtr
DDCModesFromEstablished(int scrnIndex, struct established_timings *timing)
{
    DisplayModePtr Modes = NULL, Mode = NULL;
    CARD32 bits = (timing->t1) | (timing->t2 << 8) |
        ((timing->t_manu & 0x80) << 9);
    int i;

    for (i = 0; i < 17; i++)
        if (bits & (0x01 << i)) {
            Mode = xf86ModeCopy(&(DDCEstablishedModes[i]));
            Modes = xf86ModesAdd(Modes, Mode);
        }

    return Modes;
}

/*
 *
 */
static DisplayModePtr
DDCModesFromStandardTiming(int scrnIndex, struct std_timings *timing)
{
    DisplayModePtr Modes = NULL, Mode = NULL;
    int i;
    
    for (i = 0; i < STD_TIMINGS; i++)
        if (timing[i].hsize && timing[i].vsize && timing[i].refresh) {
            Mode =  xf86CVTMode(timing[i].hsize, timing[i].vsize,
                                timing[i].refresh, FALSE, FALSE);
	    Mode->type = M_T_DRIVER;
            Modes = xf86ModesAdd(Modes, Mode);
        }

    return Modes;
}

/*
 *
 */
static DisplayModePtr
DDCModeFromDetailedTiming(int scrnIndex, struct detailed_timings *timing,
			  int preferred)
{
    DisplayModePtr Mode;

    /* We don't do stereo */
    if (timing->stereo) {
        xf86DrvMsg(scrnIndex, X_INFO, "%s: Ignoring: We don't handle stereo.\n",
                   __func__);
        return NULL;
    }
    
    /* We only do seperate sync currently */
    if (timing->sync != 0x03) {
         xf86DrvMsg(scrnIndex, X_INFO, "%s: %dx%d Warning: We only handle seperate"
                    " sync.\n", __func__, timing->h_active, timing->v_active);
    }
    
    Mode = xnfalloc(sizeof(DisplayModeRec));
    memset(Mode, 0, sizeof(DisplayModeRec));
    
    Mode->name = xnfalloc(10); /* "1234x1234" */
    xf86snprintf(Mode->name, 20, "%dx%d", timing->h_active,
                 timing->v_active);
    
    Mode->type = M_T_DRIVER;
    if (preferred)
	Mode->type |= M_T_PREFERRED;
    
    Mode->Clock = timing->clock / 1000.0;
    
    Mode->HDisplay = timing->h_active;
    Mode->HSyncStart = timing->h_active + timing->h_sync_off;
    Mode->HSyncEnd = Mode->HSyncStart + timing->h_sync_width;
    Mode->HTotal = timing->h_active + timing->h_blanking;
    
    Mode->VDisplay = timing->v_active;
    Mode->VSyncStart = timing->v_active + timing->v_sync_off;
    Mode->VSyncEnd = Mode->VSyncStart + timing->v_sync_width;
    Mode->VTotal = timing->v_active + timing->v_blanking;
    
    /* We ignore h/v_size and h/v_border for now. */
    
    if (timing->interlaced)
        Mode->Flags |= V_INTERLACE;
    
    if (timing->misc & 0x02)
        Mode->Flags |= V_PHSYNC;
    else
        Mode->Flags |= V_NHSYNC;
    
    if (timing->misc & 0x01)
        Mode->Flags |= V_PVSYNC;
    else
        Mode->Flags |= V_NVSYNC;
 
    return Mode;
}

/*
 *
 */
static void
DDCGuessRangesFromModes(int scrnIndex, MonPtr Monitor, DisplayModePtr Modes)
{
    DisplayModePtr Mode = Modes;

    if (!Monitor || !Modes)
        return;

    /* set up the ranges for scanning through the modes */
    Monitor->nHsync = 1;
    Monitor->hsync[0].lo = 1024.0;
    Monitor->hsync[0].hi = 0.0;

    Monitor->nVrefresh = 1;
    Monitor->vrefresh[0].lo = 1024.0;
    Monitor->vrefresh[0].hi = 0.0;

    while (Mode) {
        if (!Mode->HSync)
            Mode->HSync = ((float) Mode->Clock ) / ((float) Mode->HTotal);

        if (!Mode->VRefresh)
            Mode->VRefresh = (1000.0 * ((float) Mode->Clock)) / 
                ((float) (Mode->HTotal * Mode->VTotal));

        if (Mode->HSync < Monitor->hsync[0].lo)
            Monitor->hsync[0].lo = Mode->HSync;

        if (Mode->HSync > Monitor->hsync[0].hi)
            Monitor->hsync[0].hi = Mode->HSync;

        if (Mode->VRefresh < Monitor->vrefresh[0].lo)
            Monitor->vrefresh[0].lo = Mode->VRefresh;

        if (Mode->VRefresh > Monitor->vrefresh[0].hi)
            Monitor->vrefresh[0].hi = Mode->VRefresh;
        
        Mode = Mode->next;
    }
}

/*
 * Fill out MonPtr with xf86MonPtr information.
 */
void
xf86DDCMonitorSet(int scrnIndex, MonPtr Monitor, xf86MonPtr DDC)
{
    DisplayModePtr Modes = NULL, Mode;
    int i, clock;
    Bool have_hsync = FALSE, have_vrefresh = FALSE;
    int preferred;
    
    if (!Monitor || !DDC)
        return;

    Monitor->DDC = DDC;

    preferred = PREFERRED_TIMING_MODE(DDC->features.msc);

    Monitor->widthmm = 10 * DDC->features.hsize;
    Monitor->heightmm = 10 * DDC->features.vsize;
    
    /* If this is a digital display, then we can use reduced blanking */
    if (DDC->features.input_type) 
        Monitor->reducedblanking = TRUE;
    /* Allow the user to also enable this through config */

    /* Add established timings */
    Mode = DDCModesFromEstablished(scrnIndex, &DDC->timings1);
    Modes = xf86ModesAdd(Modes, Mode);

    /* Add standard timings */
    Mode = DDCModesFromStandardTiming(scrnIndex, DDC->timings2);
    Modes = xf86ModesAdd(Modes, Mode);

    /* Skip EDID ranges if they were specified in the config file */
    have_hsync = (Monitor->nHsync != 0);
    have_vrefresh = (Monitor->nVrefresh != 0);

    /* Go through the detailed monitor sections */
    for (i = 0; i < DET_TIMINGS; i++)
        switch (DDC->det_mon[i].type) {
        case DS_RANGES:
	    if (!have_hsync) {
		if (!Monitor->nHsync)
		    xf86DrvMsg(scrnIndex, X_INFO,
			    "Using EDID range info for horizontal sync\n");
		Monitor->hsync[Monitor->nHsync].lo =
		    DDC->det_mon[i].section.ranges.min_h;
		Monitor->hsync[Monitor->nHsync].hi =
		    DDC->det_mon[i].section.ranges.max_h;
		Monitor->nHsync++;
	    } else {
		xf86DrvMsg(scrnIndex, X_INFO,
			"Using hsync ranges from config file\n");
	    }

	    if (!have_vrefresh) {
		if (!Monitor->nVrefresh)
		    xf86DrvMsg(scrnIndex, X_INFO,
			    "Using EDID range info for vertical refresh\n");
		Monitor->vrefresh[Monitor->nVrefresh].lo =
		    DDC->det_mon[i].section.ranges.min_v;
		Monitor->vrefresh[Monitor->nVrefresh].hi =
		    DDC->det_mon[i].section.ranges.max_v;
		Monitor->nVrefresh++;
	    } else {
		xf86DrvMsg(scrnIndex, X_INFO,
			"Using vrefresh ranges from config file\n");
	    }

	    clock = DDC->det_mon[i].section.ranges.max_clock * 1000;
	    if (clock > Monitor->maxPixClock)
		Monitor->maxPixClock = clock;

            break;
        case DT:
            Mode = DDCModeFromDetailedTiming(scrnIndex, 
                                             &DDC->det_mon[i].section.d_timings,
					     preferred);
	    preferred = 0;
            Modes = xf86ModesAdd(Modes, Mode);
            break;
        case DS_STD_TIMINGS:
            Mode = DDCModesFromStandardTiming(scrnIndex,
                                             DDC->det_mon[i].section.std_t);
            Modes = xf86ModesAdd(Modes, Mode);
            break;
        default:
            break;
        }

    if (Modes) {
        /* Print Modes */
        xf86DrvMsg(scrnIndex, X_INFO, "Printing DDC gathered Modelines:\n");

        Mode = Modes;
        while (Mode) {
            PrintModeline(scrnIndex, Mode);
            Mode = Mode->next;
        }

        /* Do we still need ranges to be filled in? */
        if (!Monitor->nHsync || !Monitor->nVrefresh)
            DDCGuessRangesFromModes(scrnIndex, Monitor, Modes);

        /* look for last Mode */
        Mode = Modes;

        while (Mode->next)
            Mode = Mode->next;
        
        /* add to MonPtr */
        if (Monitor->Modes) {
            Monitor->Last->next = Modes;
            Modes->prev = Monitor->Last;
            Monitor->Last = Mode;
        } else {
            Monitor->Modes = Modes;
            Monitor->Last = Mode;
        }
    }
}

#define EDID1_ATOM_NAME         "XFree86_DDC_EDID1_RAWDATA"
#define EDID2_ATOM_NAME         "XFree86_DDC_EDID2_RAWDATA"
#define VDIF_ATOM_NAME          "XFree86_DDC_VDIF_RAWDATA"

static void
addRootWindowProperties(ScrnInfoPtr pScrn, xf86MonPtr DDC)
{
    Atom EDID1Atom=-1, EDID2Atom=-1;
    CARD8 *EDID1rawdata = NULL;
    CARD8 *EDID2rawdata = NULL;
    int i, scrnIndex = pScrn->scrnIndex;
    Bool makeEDID1prop = FALSE;
    Bool makeEDID2prop = FALSE;

    if (DDC->ver.version == 1) {
	makeEDID1prop = TRUE;
    } else if (DDC->ver.version == 2) {
	int checksum1;
	int checksum2;
	makeEDID2prop = TRUE;

	/* Some monitors (eg Panasonic PanaSync4)
	 * report version==2 because they used EDID v2 spec document,
	 * although they use EDID v1 data structure :-(
	 *
	 * Try using checksum to determine when we have such a monitor.
	 */
	checksum2 = 0;
	for (i = 0; i < 256; i++)
	    checksum2 += DDC->rawData[i];
	if (checksum2 % 256) {
	    xf86DrvMsg(scrnIndex, X_INFO, "Monitor EDID v2 checksum failed\n");
	    xf86DrvMsg(scrnIndex, X_INFO,
		    "XFree86_DDC_EDID2_RAWDATA property may be bad\n");
	    checksum1 = 0;
	    for (i = 0; i < 128; i++)
		checksum1 += DDC->rawData[i];
	    if (!(checksum1 % 256)) {
		xf86DrvMsg(scrnIndex, X_INFO,
			"Monitor EDID v1 checksum passed,\n");
		xf86DrvMsg(scrnIndex, X_INFO,
			"XFree86_DDC_EDID1_RAWDATA property created\n");
		makeEDID1prop = TRUE;
	    }
	}
    } else {
	xf86DrvMsg(scrnIndex, X_PROBED, "unexpected EDID version %d.%d\n",
		DDC->ver.version, DDC->ver.revision);
	return;
    }

    if (makeEDID1prop) {
	if ((EDID1rawdata = xalloc(128*sizeof(CARD8)))==NULL)
	    return;

	EDID1Atom = MakeAtom(EDID1_ATOM_NAME, sizeof(EDID1_ATOM_NAME), TRUE);
	memcpy(EDID1rawdata, DDC->rawData, 128);
	xf86RegisterRootWindowProperty(scrnIndex, EDID1Atom, XA_INTEGER, 8,
		128, (unsigned char *)EDID1rawdata);
    } 

    if (makeEDID2prop) {
	if ((EDID2rawdata = xalloc(256*sizeof(CARD8)))==NULL)
	    return;

	memcpy(EDID2rawdata, DDC->rawData, 256);
	EDID2Atom = MakeAtom(EDID2_ATOM_NAME, sizeof(EDID2_ATOM_NAME), TRUE);
	xf86RegisterRootWindowProperty(scrnIndex, EDID2Atom, XA_INTEGER, 8,
		256, (unsigned char *)EDID2rawdata);
    }

#if 0
    if (DDC->vdif) {
#define VDIF_DUMMY_STRING "setting dummy VDIF property - please insert correct values\n"

	VDIFAtom = MakeAtom(VDIF_ATOM_NAME, sizeof(VDIF_ATOM_NAME), TRUE);
	xf86RegisterRootWindowProperty(scrnIndex, VDIFAtom, XA_STRING, 8,
		strlen(VDIF_DUMMY_STRING), VDIF_DUMMY_STRING);
    }
#endif
}

Bool
xf86SetDDCproperties(ScrnInfoPtr pScrn, xf86MonPtr DDC)
{
    if (!pScrn || !pScrn->monitor || !DDC)
        return FALSE;

    xf86DDCMonitorSet(pScrn->scrnIndex, pScrn->monitor, DDC);

    addRootWindowProperties(pScrn, DDC);

    return TRUE;
}
