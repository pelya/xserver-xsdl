/* $XFree86: xc/programs/Xserver/hw/xfree86/ddc/interpret_vdif.c,v 1.6 2000/01/21 02:30:00 dawes Exp $ */

#include "Xarch.h"
#include "xf86DDC.h"
#include "vdif.h"

static xf86VdifLimitsPtr* get_limits(CARD8 *c);
static xf86VdifGammaPtr* get_gamma(CARD8 *c);
static xf86VdifTimingPtr* get_timings(CARD8 *c);
#if X_BYTE_ORDER == X_BIG_ENDIAN
static CARD32 swap_byte_order(CARD32 c);
#endif

xf86vdifPtr
xf86InterpretVdif(CARD8 *c)
{
    xf86VdifPtr p = (xf86VdifPtr)c;
    xf86vdifPtr vdif;
    int i;
#if X_BYTE_ORDER == X_BIG_ENDIAN
    int length;
#endif
    unsigned long l = 0;

    if (c == NULL) return NULL;
#if X_BYTE_ORDER == X_BIG_ENDIAN
    length = swap_byte_order(p->FileLength);
    for (i = 0; i < (length >>2); i++) 
	((CARD32*)c)[i] = swap_byte_order(((CARD32*)c)[i]) ;
#endif  
    if (p->VDIFId[0] != 'V' || p->VDIFId[1] != 'D' || p->VDIFId[2] != 'I'
	|| p->VDIFId[3] != 'F') return NULL;
    for ( i = 12; i < p->FileLength; i++) 
	l += c[i];
    if ( l != p->Checksum) return NULL;
    vdif = xalloc(sizeof(xf86vdif));  
    vdif->vdif = p;
    vdif->limits = get_limits(c);
    vdif->timings = get_timings(c);
    vdif->gamma = get_gamma(c);
    vdif->strings = VDIF_STRING(((xf86VdifPtr)c),0);
    xfree(c);
    return vdif;
}

static xf86VdifLimitsPtr*
get_limits(CARD8 *c)
{ 
    int num, i, j;
    xf86VdifLimitsPtr *pp;
    xf86VdifLimitsPtr p;

    num = ((xf86VdifPtr)c)->NumberOperationalLimits;
    pp = xalloc(sizeof(xf86VdifLimitsPtr) * (num+1));
    p = VDIF_OPERATIONAL_LIMITS(((xf86VdifPtr)c));
    j = 0;
    for ( i = 0; i<num; i++) {
	if (p->Header.ScnTag == VDIF_OPERATIONAL_LIMITS_TAG)
	    pp[j++] = p;
	VDIF_NEXT_OPERATIONAL_LIMITS(p);
    }
    pp[j] = NULL;
    return pp;
}

static xf86VdifGammaPtr*
get_gamma(CARD8 *c)
{ 
    int num, i, j;
    xf86VdifGammaPtr *pp;
    xf86VdifGammaPtr p;

    num = ((xf86VdifPtr)c)->NumberOptions;
    pp = xalloc(sizeof(xf86VdifGammaPtr) * (num+1));
    p = (xf86VdifGammaPtr)VDIF_OPTIONS(((xf86VdifPtr)c));
    j = 0;
    for ( i = 0; i<num; i++)
    {
	if (p->Header.ScnTag == VDIF_GAMMA_TABLE_TAG)
	    pp[j++] = p;
	VDIF_NEXT_OPTIONS(p);
    }
    pp[j] = NULL;
    return pp;
}

static xf86VdifTimingPtr*
get_timings(CARD8 *c)
{
    int num, num_limits;
    int i,j,k;
    xf86VdifLimitsPtr lp;
    xf86VdifTimingPtr *pp;
    xf86VdifTimingPtr p;
  
    num = ((xf86VdifPtr)c)->NumberOperationalLimits;
    lp = VDIF_OPERATIONAL_LIMITS(((xf86VdifPtr)c));
    num_limits = 0;
    for (i = 0; i < num; i++) {
	if (lp->Header.ScnTag == VDIF_OPERATIONAL_LIMITS_TAG)
	    num_limits += lp->NumberPreadjustedTimings;
	VDIF_NEXT_OPERATIONAL_LIMITS(lp);
    }
    pp = xalloc(sizeof(xf86VdifTimingPtr) 
				      * (num_limits+1));
    j = 0;
    lp = VDIF_OPERATIONAL_LIMITS(((xf86VdifPtr) c));
    for (i = 0; i < num; i++) {
	p = VDIF_PREADJUSTED_TIMING(lp);
	for (k = 0; k < lp->NumberPreadjustedTimings; k++) {
	    if (p->Header.ScnTag == VDIF_PREADJUSTED_TIMING_TAG)
		pp[j++] = p;
	    VDIF_NEXT_PREADJUSTED_TIMING(p);
	}
	VDIF_NEXT_OPERATIONAL_LIMITS(lp);
    }
    pp[j] = NULL;
    return pp;
}

#if X_BYTE_ORDER == X_BIG_ENDIAN
static CARD32
swap_byte_order(CARD32 c)
{
    return ((c & 0xFF000000) >> 24) | ((c & 0xFF0000) >> 8)
	| ((c & 0xFF00) << 8) | ((c & 0xFF) << 24);
}

#endif
