/*
 * Copyright © 1999 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "sis.h"
#include <stdio.h>

#define FREF		    14318180
#define MIN_VCO		    FREF
#define MAX_VCO		    230000000
#define MAX_PSN		    0 /* no pre scaler for this chip */
#define TOLERANCE	    0.01  /* search smallest M and N in this tolerance */
#define max_VLD 1

/*
 * Compute clock values given target frequency
 */
void 
sisGetClock (unsigned long clock, SisCrtc *crtc)
{
    unsigned char   reg7, reg13, reg2a, reg2b;
    int		    M, N, P, VLD;

    int		    bestM, bestN, bestP, bestPSN, bestVLD;
    double	    bestError, abest = 42.0, bestFout;

    double	    Fvco, Fout;
    double	    error, aerror;

    double	    target = (double) clock;

    int		    M_min = 2;
    int		    M_max = 128;

    int		    low_N = 2;
    int		    high_N = 32;
    int		    PSN = 1;

    /*
     *  fd = fref*(Numerator/Denumerator)*(Divider/PostScaler)
     *
     *  M       = Numerator [1:128]
     *  N       = DeNumerator [1:32]
     *  VLD     = Divider (Vco Loop Divider) : divide by 1, 2
     *  P       = Post Scaler : divide by 1, 2, 3, 4
     *  PSN     = Pre Scaler (Reference Divisor Select)
     *
     * result in vclk[]
     */

    P = 1;
    if (target < MAX_VCO / 2)
	P = 2;
    if (target < MAX_VCO / 3)
	P = 3;
    if (target < MAX_VCO / 4)
	P = 4;
    if (target < MAX_VCO / 6)
	P = 6;
    if (target < MAX_VCO / 8)
	P = 8;

    Fvco = P * target;

    for (N = low_N; N <= high_N; N++)
    {
	double M_desired = Fvco / FREF * N;

	if (M_desired > M_max * max_VLD)
	    continue;

	if ( M_desired > M_max ) 
	{
	    M = (int)(M_desired / 2 + 0.5);
	    VLD = 2;
	}
	else 
	{
	    M = (int)(M_desired + 0.5);
	    VLD = 1;
	}

	Fout = (double)FREF * (M * VLD)/(N * P);
	error = (target - Fout) / target;
	aerror = (error < 0) ? -error : error;
	if (aerror < abest) 
	{
	    abest = aerror;
	    bestError = error;
	    bestM = M;
	    bestN = N;
	    bestP = P;
	    bestPSN = PSN;
	    bestVLD = VLD;
	    bestFout = Fout;
	}
    }

    crtc->vclk_numerator = bestM - 1;
    crtc->vclk_divide_by_2 = bestVLD == 2;

    crtc->vclk_denominator = bestN - 1;
    switch (bestP) {
    case 1:
	crtc->vclk_post_scale = SIS_VCLK_POST_SCALE_1;
	crtc->vclk_post_scale_2 = 0;
	break;
    case 2:
	crtc->vclk_post_scale = SIS_VCLK_POST_SCALE_2;
	crtc->vclk_post_scale_2 = 0;
	break;
    case 3:
	crtc->vclk_post_scale = SIS_VCLK_POST_SCALE_3;
	crtc->vclk_post_scale_2 = 0;
	break;
    case 4:
	crtc->vclk_post_scale = SIS_VCLK_POST_SCALE_4;
	crtc->vclk_post_scale_2 = 0;
	break;
    case 6:
	crtc->vclk_post_scale = SIS_VCLK_POST_SCALE_3;
	crtc->vclk_post_scale_2 = 1;
	break;
    case 8:
	crtc->vclk_post_scale = SIS_VCLK_POST_SCALE_4;
	crtc->vclk_post_scale_2 = 1;
	break;
    }

    crtc->vclk_vco_gain = 1;

    /*
     * Don't know how to set mclk for local frame buffer; for
     * shared frame buffer, mclk is hardwired to bus speed (100MHz)?
     */
}

sisCalcMclk (SisCrtc *crtc)
{
    int mclk, Numer, DeNumer;
    double Divider, Scalar;

    Numer = crtc->mclk_numerator;
    DeNumer = crtc->mclk_denominator;
    Divider = crtc->mclk_divide_by_2 ? 2.0 : 1.0;
    Scalar = 1.0;
    if (crtc->mclk_post_scale_2)
    {
	switch (crtc->mclk_post_scale) {
	case 2:
	    Scalar = 6.0;
	    break;
	case 3:
	    Scalar = 8.0;
	    break;
	}
    }
    else
    {
	switch (crtc->mclk_post_scale) {
	case 0:
	    Scalar = 1.0;
	    break;
	case 1:
	    Scalar = 2.0;
	    break;
	case 2:
	    Scalar = 3.0;
	    break;
	case 3:
	    Scalar = 4.0;
	    break;
	}
    }

    mclk = (int)(FREF*((double)(Numer+1)/(double)(DeNumer+1))*(Divider/Scalar));

    return(mclk);
}

#define UMA_FACTOR      60
#define LFB_FACTOR      30      // Only if local frame buffer
#define SIS_SAYS_SO     0x1F    // But how is the performance??
#define CRT_ENG_THRESH  0x0F    // But how is the performance??
#define BUS_WIDTH       64
#define DFP_BUS_WIDTH   32      // rumour has it for digital flat panel ??
#define MEGAHZ          (1<<20)

void
sisEngThresh (SisCrtc *crtc, unsigned long vclk, int bpp)
{
    int threshlow, mclk;

    mclk = sisCalcMclk(crtc) / 1000000;
    vclk = vclk / 1000000;
    threshlow = ((((UMA_FACTOR*vclk*bpp)/
		   (mclk*BUS_WIDTH))+1)/2)+4;
    
    crtc->crt_cpu_threshold_low_0_3 = threshlow;
    crtc->crt_cpu_threshold_low_4 = threshlow >> 4;
    
    crtc->crt_cpu_threshold_high_0_3 = (SIS_SAYS_SO & 0xf);
    crtc->crt_cpu_threshold_high_4 = 0;
    
    crtc->crt_engine_threshold_high_0_3 = CRT_ENG_THRESH;
    crtc->crt_engine_threshold_high_4 = 1;
    
    crtc->ascii_attribute_threshold_0_2 = (SIS_SAYS_SO >> 4);
    
    crtc->crt_threshold_full_control = SIS_CRT_64_STAGE_THRESHOLD;
}
