/* $XFree86: xc/programs/Xserver/hw/xfree86/ddc/print_vdif.c,v 1.5 2003/11/06 18:37:58 tsi Exp $ */

#include "vdif.h"
#include "misc.h"
#include "xf86DDC.h"

static void print_vdif(xf86VdifPtr l, char *s);
static void print_timings(xf86VdifTimingPtr *pt);
static void print_limits(xf86VdifLimitsPtr *pl);
static void print_gamma(xf86VdifGammaPtr *pg);
static void print_type(CARD8 c);
static void print_polarity(CARD8 c);

void
xf86print_vdif(xf86vdifPtr v)
{
    print_vdif(v->vdif,v->strings);
    print_limits(v->limits);
    print_timings(v->timings);
    print_gamma(v->gamma);
}

static void
print_vdif(xf86VdifPtr l, char *s)
{
    ErrorF("Version %i.%i",l->VDIFVersion,l->VDIFRevision);
    ErrorF(" Date: %i/%i/%i, Manufactured: %i/%i/%i\n",l->Date[0],
	   l->Date[1],l->Date[2],l->DateManufactured[0],
	   l->DateManufactured[1],l->DateManufactured[2]);
    ErrorF("File Revision: %i",l->FileRevision);
    ErrorF("Manufacturer: %s\n",s + l->Manufacturer);
    ErrorF("ModelNumber: %s\n",s + l->ModelNumber);
    ErrorF("VDIFIndex: %s\n",s +l->MinVDIFIndex);
    ErrorF("Version: %s\n",s + l->Version);
    ErrorF("SerialNumber %s\n",s + l->SerialNumber);
    ErrorF("MonitorType: ");
    switch (l->MonitorType) {
    case VDIF_MONITOR_MONOCHROME:
	ErrorF("Mono\n");
	break;
    case VDIF_MONITOR_COLOR:
	ErrorF("Color\n");
	break;
    }
    ErrorF("CRT Size: %i inches\n",l->CRTSize);
    switch (l->MonitorType) {
    case VDIF_MONITOR_MONOCHROME:
	ErrorF("Border:  %i percent\n",
	       l->BorderRed);
	ErrorF("Phosphor Decay: 1: %i,",l->RedPhosphorDecay);
	if (l->GreenPhosphorDecay !=0)
	    ErrorF(" 2: %i,",l->GreenPhosphorDecay);
	if (l->BluePhosphorDecay !=0)
	    ErrorF(" 3: %i",l->BluePhosphorDecay);
	ErrorF(" ms\n");
	if (l->RedChromaticity_x)
	    ErrorF("Chromaticity: 1: x:%f, y:%f;  ",
		   l->RedChromaticity_x/1000.0,l->RedChromaticity_y/1000.0);
	if (l->GreenChromaticity_x)
	    ErrorF("Chromaticity: 2: x:%f, y:%f;  ",
		   l->GreenChromaticity_x/1000.0,l->GreenChromaticity_y/1000.0);
	if (l->BlueChromaticity_x)
	    ErrorF("Chromaticity: 3: x:%f, y:%f  ",
		   l->BlueChromaticity_x/1000.0,l->BlueChromaticity_y/1000.0);
	ErrorF("\n");
	ErrorF("Gamma: %f\n",l->RedGamma/1000.0);
	break;
    case VDIF_MONITOR_COLOR:
	ErrorF("Border: Red: %i Green: %i Blue: %i percent\n",
	       l->BorderRed,l->BorderGreen,l->BorderBlue);
	ErrorF("Phosphor Decay: Red: %i, Green: %i, Blue: %i ms\n",
	       l->RedPhosphorDecay,l->GreenPhosphorDecay,l->BluePhosphorDecay);
	ErrorF("Chromaticity: Red: x:%f, y:%f;  Green: x:%f, y:%f;  "
	       "Blue: x:%f, y:%f\n",
	       l->RedChromaticity_x/1000.0,l->RedChromaticity_y/1000.0,
	       l->GreenChromaticity_x/1000.0,l->GreenChromaticity_y/1000.0,
	       l->BlueChromaticity_x/1000.0,l->BlueChromaticity_y/1000.0);
	ErrorF("Gamma: Red:%f, Green:%f, Blue:%f\n",l->RedGamma/1000.0,
	       l->GreenGamma/1000.0,l->BlueGamma/1000.0);
	break;
    }
    ErrorF("White Point: x: %f y: %f Y: %f\n",l->WhitePoint_x/1000.0,
	   l->WhitePoint_y/1000.0,l->WhitePoint_Y/1000.0);
}

static void 
print_limits(xf86VdifLimitsPtr *pl)
{
    int i = 0;
    xf86VdifLimitsPtr l;

    while((l = pl[i]) != NULL) {
	ErrorF("Max display resolution: %i x %i pixel\n",l->MaxHorPixel,
	       l->MaxVerPixel);
	ErrorF("Size of active area: %i x %i millimeters\n",l->MaxHorActiveLength,
	       l->MaxVerActiveHeight);
	ErrorF("Video Type: ");
	print_type(l->VideoType);
	ErrorF("Sync Type: ");
	print_type(l->SyncType);
	ErrorF("Sync Configuration ");
	switch (l->SyncConfiguration) {
	case VDIF_SYNC_SEPARATE:
	    ErrorF("separate\n");
	    break;
	case VDIF_SYNC_C:
	    ErrorF("composite C\n");
	    break;
	case VDIF_SYNC_CP:
	    ErrorF("composite CP\n");
	    break;
	case VDIF_SYNC_G:
	    ErrorF("composite G\n");
	    break;
	case VDIF_SYNC_GP:
	    ErrorF("composite GP\n");
	    break;
	case VDIF_SYNC_OTHER:
	    ErrorF("other\n");
	    break;
	}
	ErrorF("Termination Resistance: %i\n",l->TerminationResistance);
	ErrorF("Levels: white: %i, black: %i, blank: %i, sync: %i mV\n",
	       l->WhiteLevel,l->BlackLevel,l->BlankLevel,l->SyncLevel);
	ErrorF("Max. Pixel Clock: %f MHz\n",l->MaxPixelClock/1000.0);
	ErrorF("Freq. Range: Hor.: %f - %f kHz, Ver.: %f - %f Hz\n",
	       l->MaxHorFrequency/1000.0,l->MinHorFrequency/1000.0,
	       l->MaxVerFrequency/1000.0,l->MinVerFrequency/1000.0);
	ErrorF("Retrace time: Hor: %f us,  Ver: %f ms\n",l->MinHorRetrace/1000.0,
	       l->MinVerRetrace/1000.0);
    }
}

static void 
print_timings(xf86VdifTimingPtr *pt)
{
    int i = 0;
    xf86VdifTimingPtr t;

    while((t = pt[i]) != NULL) {
	ErrorF("SVGA / SVPMI mode number: %i\n",t->PreadjustedTimingName);
	ErrorF("Mode %i x %i\n",t->HorPixel,t->VerPixel);
	ErrorF("Size: %i x %i mm\n",t->HorAddrLength,t->VerAddrHeight);
	ErrorF("Ratios: %i/%i\n",t->PixelWidthRatio,t->PixelHeightRatio);
	ErrorF("Character width: %i",t->CharacterWidth);
	ErrorF("Clock: %f MHz HFreq.: %f kHz, VFreq: %f Hz\n",t->PixelClock/1000.0,
	       t->HorFrequency/1000.0,t->VerFrequency/1000.0);
	ErrorF("Htotal: %f us, Vtotal %f ms\n", t->HorTotalTime/1000.0,
	       t->VerTotalTime/1000.0);
	ErrorF("HDisp: %f, HBlankStart: %f, HBlankLength: %f, "
	       "HSyncStart: %f HSyncEnd: %f us\n",t->HorAddrTime/1000.0,
	       t->HorBlankStart/1000.0,t->HorBlankTime/1000.0,
	       t->HorSyncStart/1000.0,t->HorSyncTime/1000.0);
	ErrorF("VDisp: %f, VBlankStart: %f, VBlankLength: %f, "
	       "VSyncStart: %f VSyncEnd: %f us\n",t->VerAddrTime/1000.0,
	       t->VerBlankStart/1000.0,t->VerBlankTime/1000.0,
	       t->VerSyncStart/1000.0,t->VerSyncTime/1000.0);
	ErrorF("Scan Type: ");
	switch (t->ScanType) {
	case VDIF_SCAN_INTERLACED:
	    ErrorF("interlaced   ");
	    break;
	case VDIF_SCAN_NONINTERLACED:
	    ErrorF("non interlaced   ");
	    break;
	case VDIF_SCAN_OTHER:
	    ErrorF("other   ");
	    break;
	}
	ErrorF("Polarity: H: ");
	print_polarity(t->HorSyncPolarity);
	ErrorF("V: ");
	print_polarity(t->VerSyncPolarity);
	ErrorF("\n");
    }
}

static void
print_gamma(xf86VdifGammaPtr *pg)
{
    int i = 0;
    xf86VdifGammaPtr g;

    while((g = pg[i]) != NULL) {
	ErrorF("Gamma Table Entries: %i\n",g->GammaTableEntries);
    }
}

static void
print_type(CARD8 c)
{
    switch (c) {
    case VDIF_VIDEO_TTL :
	ErrorF("TTL\n");
	break;
    case VDIF_VIDEO_ANALOG :
	ErrorF("Analog\n");
	break;
    case VDIF_VIDEO_ECL:
	ErrorF("ECL\n");
	break;
    case VDIF_VIDEO_DECL:
	ErrorF("DECL\n");
	break;
    case VDIF_VIDEO_OTHER:
	ErrorF("other\n");
	break;
    }
}

static void
print_polarity(CARD8 c)
{
    switch (c) {
    case VDIF_POLARITY_NEGATIVE:
	ErrorF(" Neg.");
	break;
    case VDIF_POLARITY_POSITIVE:
	ErrorF(" Pos.");
	break;
    }
}
