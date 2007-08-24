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
#include "s3.h"

/*
 * Clock synthesis:
 *
 *  f_out = f_ref * ((M + 2) / ((N + 2) * (1 << R)))
 *
 *  Constraints:
 *
 *  1.	135MHz <= f_ref * ((M + 2) / (N + 2)) <= 270 MHz
 *  2.	N >= 1
 *
 *  Vertical refresh rate = clock / ((hsize + hblank) * (vsize + vblank))
 *  Horizontal refresh rate = clock / (hsize + hblank)
 */
 
/* all in kHz */
#define MIN_VCO	180000.0
#define MAX_VCO 360000.0

void
s3GetClock (int target, int *Mp, int *Np, int *Rp, int maxM, int maxN, int maxR)
{
    int	    M, N, R, bestM, bestN;
    int	    f_vco, f_out;
    int	    err, abserr, besterr;

    /*
     * Compute correct R value to keep VCO in range
     */
    for (R = 0; R <= maxR; R++)
    {
	f_vco = target * (1 << R);
	if (MIN_VCO <= f_vco && f_vco < MAX_VCO)
	    break;
    }

    /* M = f_out / f_ref * ((N + 2) * (1 << R)); */
    besterr = target;
    for (N = 0; N <= maxN; N++)
    {
	M = (target * (N + 2) * (1 << R) + (S3_CLOCK_REF/2)) / S3_CLOCK_REF - 2;
	if (0 <= M && M <= maxM)
	{
	    f_out = S3_CLOCK(M,N,R);
	    err = target - f_out;
	    if (err < 0)
		err = -err;
	    if (err < besterr)
	    {
		besterr = err;
		bestM = M;
		bestN = N;
	    }
	}
    }
    *Mp = bestM;
    *Np = bestN;
    *Rp = R;
}
