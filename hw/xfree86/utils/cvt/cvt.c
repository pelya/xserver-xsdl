/*
 * Copyright (c) 2005 by Luc Verhaegen.
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

/*
 * Calculate VESA CVT standard timing modelines.
 *
 * My own copyright is added. This is only there for the standalone version.
 * I don't want to polute the xf86Mode.c license.
 * 
 */
#include "xf86.h"


/*
 * Grabbed from xf86Mode.c from the Xorg tree. Hence the copyright and license.
 *
 * Altered for stdout printing.
 */
static void
add(char **p, char *new)
{
    *p = xnfrealloc(*p, strlen(*p) + strlen(new) + 2);
    strcat(*p, " ");
    strcat(*p, new);
}

static void
PrintModeline(DisplayModePtr mode)
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

    printf("Modeline \"%s\"  %6.2f  %i %i %i %i  %i %i %i %i%s\n",
           mode->name, mode->Clock/1000., mode->HDisplay,
           mode->HSyncStart, mode->HSyncEnd, mode->HTotal,
           mode->VDisplay, mode->VSyncStart, mode->VSyncEnd,
           mode->VTotal, flags);
    xfree(flags);
}


/*
 * New code... Well, somewhat.
 */


/*
 * These calculations are stolen from the CVT calculation spreadsheet written
 * by Graham Loveridge. He seems to be claiming no copyright and there seems to
 * be no license attached to this. He apparently just wants to see his name
 * mentioned.
 *
 * This file can be found at http://www.vesa.org/Public/CVT/CVTd6r1.xls
 *
 * Comments and structure corresponds to the comments and structure of the xls.
 * This should ease importing of future changes to the standard (not very
 * likely though).
 *
 * About margins; i'm sure that they are to be the bit between HDisplay and
 * HBlankStart, HBlankEnd and HTotal, VDisplay and VBlankStart, VBlankEnd and 
 * VTotal, where the overscan colour is shown. FB seems to call _all_ blanking
 * outside sync "margin" for some reason. Since we prefer seeing proper
 * blanking instead of the overscan colour, and since the Crtc* values will
 * probably get altered after us, we will disable margins altogether. With
 * these calculations, Margins will plainly expand H/VDisplay, and we don't
 * want that.
 *
 */
static DisplayModeRec *
xf86CVTMode(int HDisplay, int VDisplay, float VRefresh, Bool Reduced,
            Bool Interlaced)
{
    DisplayModeRec  *Mode = xnfalloc(sizeof(DisplayModeRec));

    /* 1) top/bottom margin size (% of height) - default: 1.8 */
#define CVT_MARGIN_PERCENTAGE 1.8    

    /* 2) character cell horizontal granularity (pixels) - default 8 */
#define CVT_H_GRANULARITY 8

    /* 4) Minimum vertical porch (lines) - default 3 */
#define CVT_MIN_V_PORCH 3

    /* 4) Minimum number of vertical back porch lines - default 6 */
#define CVT_MIN_V_BPORCH 6

    /* Pixel Clock step (kHz) */
#define CVT_CLOCK_STEP 250

    Bool Margins = FALSE;
    float  VFieldRate, HPeriod;
    int  HDisplayRnd, HMargin;
    int  VDisplayRnd, VMargin, VSync;
    float  Interlace; /* Please rename this */

    /* CVT default is 60.0Hz */
    if (!VRefresh)
        VRefresh = 60.0;

    /* 1. Required field rate */
    if (Interlaced)
        VFieldRate = VRefresh * 2;
    else
        VFieldRate = VRefresh;

    /* 2. Horizontal pixels */
    HDisplayRnd = HDisplay - (HDisplay % CVT_H_GRANULARITY);

    /* 3. Determine left and right borders */
    if (Margins) {
        /* right margin is actually exactly the same as left */
        HMargin = (((float) HDisplayRnd) * CVT_MARGIN_PERCENTAGE / 100.0);
        HMargin -= HMargin % CVT_H_GRANULARITY;
    } else
        HMargin = 0;

    /* 4. Find total active pixels */
    Mode->HDisplay = HDisplayRnd + 2*HMargin;

    /* 5. Find number of lines per field */
    if (Interlaced)
        VDisplayRnd = VDisplay / 2;
    else
        VDisplayRnd = VDisplay;

    /* 6. Find top and bottom margins */
    /* nope. */
    if (Margins)
        /* top and bottom margins are equal again. */
        VMargin = (((float) VDisplayRnd) * CVT_MARGIN_PERCENTAGE / 100.0);
    else
        VMargin = 0;

    Mode->VDisplay = VDisplay + 2*VMargin;

    /* 7. Interlace */
    if (Interlaced)
        Interlace = 0.5;
    else
        Interlace = 0.0;

    /* Determine VSync Width from aspect ratio */
    if (!(VDisplay % 3) && ((VDisplay * 4 / 3) == HDisplay))
        VSync = 4;
    else if (!(VDisplay % 9) && ((VDisplay * 16 / 9) == HDisplay))
        VSync = 5;
    else if (!(VDisplay % 10) && ((VDisplay * 16 / 10) == HDisplay))
        VSync = 6;
    else if (!(VDisplay % 4) && ((VDisplay * 5 / 4) == HDisplay))
        VSync = 7;
    else if (!(VDisplay % 9) && ((VDisplay * 15 / 9) == HDisplay))
        VSync = 7;
    else /* Custom */
        VSync = 10;

    if (!Reduced) { /* simplified GTF calculation */

        /* 4) Minimum time of vertical sync + back porch interval (µs) 
         * default 550.0 */
#define CVT_MIN_VSYNC_BP 550.0

        /* 3) Nominal HSync width (% of line period) - default 8 */
#define CVT_HSYNC_PERCENTAGE 8

        float  HBlankPercentage;
        int  VSyncAndBackPorch, VBackPorch;
        int  HBlank;

        /* 8. Estimated Horizontal period */
        HPeriod = ((float) (1000000.0 / VFieldRate - CVT_MIN_VSYNC_BP)) / 
            (VDisplayRnd + 2 * VMargin + CVT_MIN_V_PORCH + Interlace);

        /* 9. Find number of lines in sync + backporch */
        if (((int)(CVT_MIN_VSYNC_BP / HPeriod) + 1) < (VSync + CVT_MIN_V_PORCH))
            VSyncAndBackPorch = VSync + CVT_MIN_V_PORCH;
        else
            VSyncAndBackPorch = (int)(CVT_MIN_VSYNC_BP / HPeriod) + 1;

        /* 10. Find number of lines in back porch */
        VBackPorch = VSyncAndBackPorch - VSync;

        /* 11. Find total number of lines in vertical field */
        Mode->VTotal = VDisplayRnd + 2 * VMargin + VSyncAndBackPorch + Interlace
            + CVT_MIN_V_PORCH;

        /* 5) Definition of Horizontal blanking time limitation */
        /* Gradient (%/kHz) - default 600 */
#define CVT_M_FACTOR 600

        /* Offset (%) - default 40 */
#define CVT_C_FACTOR 40

        /* Blanking time scaling factor - default 128 */
#define CVT_K_FACTOR 128

        /* Scaling factor weighting - default 20 */
#define CVT_J_FACTOR 20

#define CVT_M_PRIME CVT_M_FACTOR * CVT_K_FACTOR / 256
#define CVT_C_PRIME (CVT_C_FACTOR - CVT_J_FACTOR) * CVT_K_FACTOR / 256 + \
        CVT_J_FACTOR

        /* 12. Find ideal blanking duty cycle from formula */
        HBlankPercentage = CVT_C_PRIME - CVT_M_PRIME * HPeriod/1000.0;

        /* 13. Blanking time */
        if (HBlankPercentage < 20)
            HBlankPercentage = 20;

        HBlank = Mode->HDisplay * HBlankPercentage/(100.0 - HBlankPercentage);
        HBlank -= HBlank % (2*CVT_H_GRANULARITY);
        
        /* 14. Find total number of pixels in a line. */
        Mode->HTotal = Mode->HDisplay + HBlank;

        /* Fill in HSync values */
        Mode->HSyncEnd = Mode->HDisplay + HBlank / 2;

        Mode->HSyncStart = Mode->HSyncEnd - 
            (Mode->HTotal * CVT_HSYNC_PERCENTAGE) / 100;
        Mode->HSyncStart += CVT_H_GRANULARITY - 
            Mode->HSyncStart % CVT_H_GRANULARITY;

        /* Fill in VSync values */
        Mode->VSyncStart = Mode->VDisplay + CVT_MIN_V_PORCH;
        Mode->VSyncEnd = Mode->VSyncStart + VSync;

    } else { /* Reduced blanking */
        /* Minimum vertical blanking interval time (µs) - default 460 */
#define CVT_RB_MIN_VBLANK 460.0

        /* Fixed number of clocks for horizontal sync */
#define CVT_RB_H_SYNC 32.0

        /* Fixed number of clocks for horizontal blanking */
#define CVT_RB_H_BLANK 160.0

        /* Fixed number of lines for vertical front porch - default 3 */
#define CVT_RB_VFPORCH 3

        int  VBILines;

        /* 8. Estimate Horizontal period. */
        HPeriod = ((float) (1000000.0 / VFieldRate - CVT_RB_MIN_VBLANK)) / 
            (VDisplayRnd + 2*VMargin);

        /* 9. Find number of lines in vertical blanking */
        VBILines = ((float) CVT_RB_MIN_VBLANK) / HPeriod + 1;

        /* 10. Check if vertical blanking is sufficient */
        if (VBILines < (CVT_RB_VFPORCH + VSync + CVT_MIN_V_BPORCH))
            VBILines = CVT_RB_VFPORCH + VSync + CVT_MIN_V_BPORCH;
        
        /* 11. Find total number of lines in vertical field */
        Mode->VTotal = VDisplayRnd + 2 * VMargin + Interlace + VBILines;

        /* 12. Find total number of pixels in a line */
        Mode->HTotal = Mode->HDisplay + CVT_RB_H_BLANK;

        /* Fill in HSync values */
        Mode->HSyncEnd = Mode->HDisplay + CVT_RB_H_BLANK / 2;
        Mode->HSyncStart = Mode->HSyncEnd - CVT_RB_H_SYNC;

        /* Fill in VSync values */
        Mode->VSyncStart = Mode->VDisplay + CVT_RB_VFPORCH;
        Mode->VSyncEnd = Mode->VSyncStart + VSync;
    }

    /* 15/13. Find pixel clock frequency (kHz for xf86) */
    Mode->Clock = Mode->HTotal * 1000.0 / HPeriod;
    Mode->Clock -= Mode->Clock % CVT_CLOCK_STEP;

    /* 16/14. Find actual Horizontal Frequency (kHz) */
    Mode->HSync = ((float) Mode->Clock) / ((float) Mode->HTotal);

    /* 17/15. Find actual Field rate */
    Mode->VRefresh = (1000.0 * ((float) Mode->Clock)) / 
        ((float) (Mode->HTotal * Mode->VTotal));

    /* 18/16. Find actual vertical frame frequency */
    /* ignore - just set the mode flag for interlaced */
    if (Interlaced)
        Mode->VTotal *= 2;

    /* Fill in Mode parameters */
    {
        char  Name[256];
        Name[0] = 0;
        if (Reduced)
            snprintf(Name, 256, "%dx%dR", HDisplay, VDisplay);
        else
            snprintf(Name, 256, "%dx%d_%.2f", HDisplay, VDisplay, VRefresh);
        Mode->name = xnfalloc(strlen(Name) + 1);
        memcpy(Mode->name, Name, strlen(Name));
    }

    if (Reduced)
        Mode->Flags |= V_PHSYNC | V_NVSYNC;
    else
        Mode->Flags |= V_NHSYNC | V_PVSYNC;

    if (Interlaced)
        Mode->Flags |= V_INTERLACE;

    return Mode;
}


/*
 * Quickly check wether this is a CVT standard mode.
 */
static Bool
xf86CVTCheckStandard(int HDisplay, int VDisplay, float VRefresh, Bool Reduced,
                     Bool Verbose)
{
    Bool  IsCVT = TRUE;

    if ((!(VDisplay % 3) && ((VDisplay * 4 / 3) == HDisplay)) ||
        (!(VDisplay % 9) && ((VDisplay * 16 / 9) == HDisplay)) ||
        (!(VDisplay % 10) && ((VDisplay * 16 / 10) == HDisplay)) ||
        (!(VDisplay % 4) && ((VDisplay * 5 / 4) == HDisplay)) ||
        (!(VDisplay % 9) && ((VDisplay * 15 / 9) == HDisplay)))
        ;
    else {
        if (Verbose)
            fprintf(stderr, "Warning: Aspect Ratio is not CVT standard.\n");
        IsCVT = FALSE;
    }
    
    if ((VRefresh != 50.0) && (VRefresh != 60.0) &&
        (VRefresh != 75.0) && (VRefresh != 85.0)) {
        if (Verbose)
            fprintf(stderr, "Warning: Refresh Rate is not CVT standard "
                    "(50, 60, 75 or 85Hz).\n");
        IsCVT = FALSE;
    }
    
    return IsCVT;
}


/*
 * I'm not documenting --interlaced for obvious reasons, even though I did
 * implement it. I also can't deny having looked at gtf here.
 */
static void
PrintUsage(char *Name)
{
    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s [-v|--verbose] [-r|--reduced] X Y [refresh]\n",
            Name);
    fprintf(stderr, "\n");
    fprintf(stderr, " -v|--verbose : Warn about CVT standard adherance.\n");
    fprintf(stderr, " -r|--reduced : Create a mode with reduced blanking "
            "(default: normal blanking).\n");
    fprintf(stderr, "            X : Desired horizontal resolution "
            "(multiple of 8, required).\n");
    fprintf(stderr, "            Y : Desired vertical resolution (required).\n");
    fprintf(stderr, "      refresh : Desired refresh rate (default: 60.0Hz).\n");
    fprintf(stderr, "\n");

    fprintf(stderr, "Calculates VESA CVT (Common Video Timing) modelines for"
            " use with X.\n");
}


/*
 *
 */
static void
PrintComment(DisplayModeRec *Mode, Bool CVT, Bool Reduced)
{
    printf("# %dx%d %.2f Hz ", Mode->HDisplay, Mode->VDisplay, Mode->VRefresh);

    if (CVT) {
        printf("(CVT %.2fM",
               ((float) Mode->HDisplay * Mode->VDisplay) / 1000000.0);

        if (!(Mode->VDisplay % 3) &&
            ((Mode->VDisplay * 4 / 3) == Mode->HDisplay))
            printf("3");
        else if (!(Mode->VDisplay % 9) &&
                 ((Mode->VDisplay * 16 / 9) == Mode->HDisplay))
            printf("9");
        else if (!(Mode->VDisplay % 10) &&
                 ((Mode->VDisplay * 16 / 10) == Mode->HDisplay))
            printf("A");
        else if (!(Mode->VDisplay % 4) &&
                 ((Mode->VDisplay * 5 / 4) == Mode->HDisplay))
            printf("4");
        else if (!(Mode->VDisplay % 9) &&
                 ((Mode->VDisplay * 15 / 9) == Mode->HDisplay))
            printf("9");

        if (Reduced)
            printf("-R");

        printf(") ");
    } else
        printf("(CVT) ");

    printf("hsync: %.2f kHz; ", Mode->HSync);
    printf("pclk: %.2f MHz", ((float ) Mode->Clock) / 1000.0);

    printf("\n");
}


/*
 *
 */
int
main (int argc, char *argv[])
{
    DisplayModeRec  *Mode;
    int  HDisplay = 0, VDisplay = 0;
    float  VRefresh = 0.0;
    Bool  Reduced = FALSE, Verbose = FALSE, IsCVT;
    Bool  Interlaced = FALSE;
    int  n;

    if ((argc < 3) || (argc > 7)) {
        PrintUsage(argv[0]);
        return 0;
    }

    /* This doesn't filter out bad flags properly. Bad flags get passed down
     * to atoi/atof, which then return 0, so that these variables can get
     * filled next time round. So this is just a cosmetic problem.
     */
    for (n = 1; n < argc; n++) {
        if (!strcmp(argv[n], "-r") || !strcmp(argv[n], "--reduced"))
            Reduced = TRUE;
        else if (!strcmp(argv[n], "-i") || !strcmp(argv[n], "--interlaced"))
            Interlaced = TRUE;
        else if (!strcmp(argv[n], "-v") || !strcmp(argv[n], "--verbose"))
            Verbose = TRUE;
        else if (!strcmp(argv[n], "-h") || !strcmp(argv[n], "--help")) {
            PrintUsage(argv[0]);
            return 0;
        } else if (!HDisplay)
            HDisplay = atoi(argv[n]);
        else if (!VDisplay)
            VDisplay = atoi(argv[n]);
        else if (!VRefresh)
            VRefresh = atof(argv[n]);
        else {
            PrintUsage(argv[0]);
            return 0;
        }
    }

    if (!HDisplay || !VDisplay) {
        PrintUsage(argv[0]);
        return 0;
    }

    /* Default to 60.0Hz */
    if (!VRefresh)
        VRefresh = 60.0;

    /* Enforce hard limitation */
    if (HDisplay % 8) {
        fprintf(stderr, "\nERROR: Horizontal Resolution needs to be a "
                "multiple of 8.\n");
        PrintUsage(argv[0]);
        return 0;
    }

    if (Reduced && (VRefresh != 60.0)) {
        fprintf(stderr, "\nERROR: 60Hz refresh rate required for reduced"
                " blanking.\n");
        PrintUsage(argv[0]);
        return 0;
    }

    IsCVT = xf86CVTCheckStandard(HDisplay, VDisplay, VRefresh, Reduced, Verbose);

    Mode = xf86CVTMode(HDisplay, VDisplay, VRefresh, Reduced, Interlaced);

    PrintComment(Mode, IsCVT, Reduced);
    PrintModeline(Mode);
    
    return 0;
}
