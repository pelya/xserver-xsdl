/*
 * Copyright © 2007 Keith Packard
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
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "misc.h"
#include "scrnintstr.h"
#include "os.h"
#include "regionstr.h"
#include "validate.h"
#include "windowstr.h"
#include "input.h"
#include "resource.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "servermd.h"
#include "picturestr.h"

#define F(x)	IntToxFixed(x)

_X_EXPORT void
PictureTransformInitIdentity (PictTransformPtr matrix)
{
    int	i;
    memset (matrix, '\0', sizeof (PictTransform));
    for (i = 0; i < 3; i++)
	matrix->matrix[i][i] = F(1);
}

typedef xFixed_32_32	xFixed_34_30;

_X_EXPORT Bool
PictureTransformPoint3d (PictTransformPtr transform,
                         PictVectorPtr	vector)
{
    PictVector	    result;
    int		    i, j;
    xFixed_32_32    partial;
    xFixed_48_16    v;

    for (j = 0; j < 3; j++)
    {
	v = 0;
	for (i = 0; i < 3; i++)
	{
	    partial = ((xFixed_48_16) transform->matrix[j][i] *
		       (xFixed_48_16) vector->vector[i]);
	    v += partial >> 16;
	}
	if (v > MAX_FIXED_48_16 || v < MIN_FIXED_48_16)
	    return FALSE;
	result.vector[j] = (xFixed) v;
    }
    *vector = result;
    if (!result.vector[2])
	return FALSE;
    return TRUE;
}

_X_EXPORT Bool
PictureTransformPoint (PictTransformPtr transform,
		       PictVectorPtr	vector)
{
    PictVector	    result;
    int		    i, j;
    xFixed_32_32    partial;
    xFixed_34_30    v[3];
    xFixed_48_16    quo;

    for (j = 0; j < 3; j++)
    {
	v[j] = 0;
	for (i = 0; i < 3; i++)
	{
	    partial = ((xFixed_32_32) transform->matrix[j][i] * 
		       (xFixed_32_32) vector->vector[i]);
	    v[j] += partial >> 2;
	}
    }
    if (!v[2])
	return FALSE;
    for (j = 0; j < 2; j++)
    {
	quo = v[j] / (v[2] >> 16);
	if (quo > MAX_FIXED_48_16 || quo < MIN_FIXED_48_16)
	    return FALSE;
	vector->vector[j] = (xFixed) quo;
    }
    vector->vector[2] = xFixed1;
    return TRUE;
}

_X_EXPORT Bool
PictureTransformMultiply (PictTransformPtr dst, PictTransformPtr l, PictTransformPtr r)
{
    PictTransform   d;
    int		    dx, dy;
    int		    o;

    for (dy = 0; dy < 3; dy++)
	for (dx = 0; dx < 3; dx++)
	{
	    xFixed_48_16    v;
	    xFixed_32_32    partial;
	    v = 0;
	    for (o = 0; o < 3; o++)
	    {
		partial = (xFixed_32_32) l->matrix[dy][o] * (xFixed_32_32) r->matrix[o][dx];
		v += partial >> 16;
	    }
	    if (v > MAX_FIXED_48_16 || v < MIN_FIXED_48_16)
		return FALSE;
	    d.matrix[dy][dx] = (xFixed) v;
	}
    *dst = d;
    return TRUE;
}

_X_EXPORT void
PictureTransformInitScale (PictTransformPtr t, xFixed sx, xFixed sy)
{
    memset (t, '\0', sizeof (PictTransform));
    t->matrix[0][0] = sx;
    t->matrix[1][1] = sy;
    t->matrix[2][2] = F (1);
}

static xFixed
fixed_inverse (xFixed x)
{
    return (xFixed) ((((xFixed_48_16) F(1)) * F(1)) / x);
}

_X_EXPORT Bool
PictureTransformScale (PictTransformPtr forward,
		       PictTransformPtr reverse,
		       xFixed sx, xFixed sy)
{
    PictTransform   t;
    
    if (sx == 0 || sy == 0)
	return FALSE;

    PictureTransformInitScale (&t, sx, sy);
    if (!PictureTransformMultiply (forward, &t, forward))
	return FALSE;
    PictureTransformInitScale (&t, fixed_inverse (sx), fixed_inverse (sy));
    if (!PictureTransformMultiply (reverse, reverse, &t))
	return FALSE;
    return TRUE;
}

_X_EXPORT void
PictureTransformInitRotate (PictTransformPtr t, xFixed c, xFixed s)
{
    memset (t, '\0', sizeof (PictTransform));
    t->matrix[0][0] = c;
    t->matrix[0][1] = -s;
    t->matrix[1][0] = s;
    t->matrix[1][1] = c;
    t->matrix[2][2] = F (1);
}

_X_EXPORT Bool
PictureTransformRotate (PictTransformPtr forward,
			PictTransformPtr reverse,
			xFixed c, xFixed s)
{
    PictTransform   t;
    PictureTransformInitRotate (&t, c, s);
    if (!PictureTransformMultiply (forward, &t, forward))
	return FALSE;
    
    PictureTransformInitRotate (&t, c, -s);
    if (!PictureTransformMultiply (reverse, reverse, &t))
	return FALSE;
    return TRUE;
}

_X_EXPORT void
PictureTransformInitTranslate (PictTransformPtr t, xFixed tx, xFixed ty)
{
    memset (t, '\0', sizeof (PictTransform));
    t->matrix[0][0] = F (1);
    t->matrix[0][2] = tx;
    t->matrix[1][1] = F (1);
    t->matrix[1][2] = ty;
    t->matrix[2][2] = F (1);
}

_X_EXPORT Bool
PictureTransformTranslate (PictTransformPtr forward,
			   PictTransformPtr reverse,
			   xFixed tx, xFixed ty)
{
    PictTransform   t;
    PictureTransformInitTranslate (&t, tx, ty);
    if (!PictureTransformMultiply (forward, &t, forward))
	return FALSE;

    PictureTransformInitTranslate (&t, -tx, -ty);
    if (!PictureTransformMultiply (reverse, reverse, &t))
	return FALSE;
    return TRUE;
}

_X_EXPORT void
PictureTransformBounds (BoxPtr b, PictTransformPtr matrix)
{
    PictVector	v[4];
    int		i;
    int		x1, y1, x2, y2;

    v[0].vector[0] = F (b->x1);    v[0].vector[1] = F (b->y1);	v[0].vector[2] = F(1);
    v[1].vector[0] = F (b->x2);    v[1].vector[1] = F (b->y1);	v[1].vector[2] = F(1);
    v[2].vector[0] = F (b->x2);    v[2].vector[1] = F (b->y2);	v[2].vector[2] = F(1);
    v[3].vector[0] = F (b->x1);    v[3].vector[1] = F (b->y2);	v[3].vector[2] = F(1);
    for (i = 0; i < 4; i++)
    {
	PictureTransformPoint (matrix, &v[i]);
	x1 = xFixedToInt (v[i].vector[0]);
	y1 = xFixedToInt (v[i].vector[1]);
	x2 = xFixedToInt (xFixedCeil (v[i].vector[0]));
	y2 = xFixedToInt (xFixedCeil (v[i].vector[1]));
	if (i == 0)
	{
	    b->x1 = x1; b->y1 = y1;
	    b->x2 = x2; b->y2 = y2;
	}
	else
	{
	    if (x1 < b->x1) b->x1 = x1;
	    if (y1 < b->y1) b->y1 = y1;
	    if (x2 > b->x2) b->x2 = x2;
	    if (y2 > b->y2) b->y2 = y2;
	}
    }
}

_X_EXPORT Bool
PictureTransformInvert (PictTransformPtr dst, const PictTransformPtr src)
{
    struct pict_f_transform m, r;

    pict_f_transform_from_pixman_transform (&m, src);
    if (!pict_f_transform_invert (&r, &m))
	return FALSE;
    if (!pixman_transform_from_pict_f_transform (dst, &r))
	return FALSE;
    return TRUE;
}

static Bool
within_epsilon (xFixed a, xFixed b, xFixed epsilon)
{
    xFixed  t = a - b;
    if (t < 0) t = -t;
    return t <= epsilon;
}

#define epsilon	(xFixed) (2)

#define IsSame(a,b) (within_epsilon (a, b, epsilon))
#define IsZero(a)   (within_epsilon (a, 0, epsilon))
#define IsOne(a)    (within_epsilon (a, F(1), epsilon))
#define IsUnit(a)   (within_epsilon (a, F( 1), epsilon) || \
		     within_epsilon (a, F(-1), epsilon) || \
		     IsZero (a))
#define IsInt(a)    (IsZero (xFixedFrac(a)))
		     
_X_EXPORT Bool
PictureTransformIsIdentity(PictTransform *t)
{
    return (IsSame (t->matrix[0][0], t->matrix[1][1]) &&
	    IsSame (t->matrix[0][0], t->matrix[2][2]) &&
	    !IsZero (t->matrix[0][0]) &&
	    IsZero (t->matrix[0][1]) &&
	    IsZero (t->matrix[0][2]) &&
	    IsZero (t->matrix[1][0]) &&
	    IsZero (t->matrix[1][2]) &&
	    IsZero (t->matrix[2][0]) &&
	    IsZero (t->matrix[2][1]));
}

_X_EXPORT Bool
PictureTransformIsScale(PictTransform *t)
{
    return (!IsZero (t->matrix[0][0]) &&
	     IsZero (t->matrix[0][1]) &&
	     IsZero (t->matrix[0][2]) &&
	    
	     IsZero (t->matrix[1][0]) &&
	    !IsZero (t->matrix[1][1]) &&
	     IsZero (t->matrix[1][2]) &&
	    
	     IsZero (t->matrix[2][0]) &&
	     IsZero (t->matrix[2][1]) &&
	    !IsZero (t->matrix[2][2]));
}

_X_EXPORT Bool
PictureTransformIsIntTranslate(PictTransform *t)
{
    return ( IsOne  (t->matrix[0][0]) &&
	     IsZero (t->matrix[0][1]) &&
	     IsInt  (t->matrix[0][2]) &&
	    
	     IsZero (t->matrix[1][0]) &&
	     IsOne  (t->matrix[1][1]) &&
	     IsInt  (t->matrix[1][2]) &&
	    
	     IsZero (t->matrix[2][0]) &&
	     IsZero (t->matrix[2][1]) &&
	     IsOne  (t->matrix[2][2]));
}

_X_EXPORT Bool
PictureTransformIsInverse (PictTransform *a, PictTransform *b)
{
    PictTransform   t;

    PictureTransformMultiply (&t, a, b);
    return PictureTransformIsIdentity (&t);
}

_X_EXPORT void
PictTransform_from_xRenderTransform (PictTransformPtr pict,
				     xRenderTransform *render)
{
    pict->matrix[0][0] = render->matrix11;
    pict->matrix[0][1] = render->matrix12;
    pict->matrix[0][2] = render->matrix13;

    pict->matrix[1][0] = render->matrix21;
    pict->matrix[1][1] = render->matrix22;
    pict->matrix[1][2] = render->matrix23;

    pict->matrix[2][0] = render->matrix31;
    pict->matrix[2][1] = render->matrix32;
    pict->matrix[2][2] = render->matrix33;
}

void
xRenderTransform_from_PictTransform (xRenderTransform *render,
				     PictTransformPtr pict)
{
    render->matrix11 = pict->matrix[0][0];
    render->matrix12 = pict->matrix[0][1];
    render->matrix13 = pict->matrix[0][2];

    render->matrix21 = pict->matrix[1][0];
    render->matrix22 = pict->matrix[1][1];
    render->matrix23 = pict->matrix[1][2];

    render->matrix31 = pict->matrix[2][0];
    render->matrix32 = pict->matrix[2][1];
    render->matrix33 = pict->matrix[2][2];
}

/*
 * Floating point matrix interfaces
 */

_X_EXPORT void
pict_f_transform_from_pixman_transform (struct pict_f_transform *ft,
					struct pixman_transform	*t)
{
    int	i, j;

    for (j = 0; j < 3; j++)
	for (i = 0; i < 3; i++)
	    ft->m[j][i] = pixman_fixed_to_double (t->matrix[j][i]);
}

_X_EXPORT Bool
pixman_transform_from_pict_f_transform (struct pixman_transform	*t,
					struct pict_f_transform	*ft)
{
    int	i, j;

    for (j = 0; j < 3; j++)
	for (i = 0; i < 3; i++)
	{
	    double  d = ft->m[j][i];
	    if (d < -32767.0 || d > 32767.0)
		return FALSE;
	    d = d * 65536.0 + 0.5;
	    t->matrix[j][i] = (xFixed) floor (d);
	}
    return TRUE;
}

static const int	a[3] = { 3, 3, 2 };
static const int	b[3] = { 2, 1, 1 };

_X_EXPORT Bool
pict_f_transform_invert (struct pict_f_transform *r,
			 struct pict_f_transform *m)
{
    double  det;
    int	    i, j;
    static int	a[3] = { 2, 2, 1 };
    static int	b[3] = { 1, 0, 0 };

    det = 0;
    for (i = 0; i < 3; i++) {
	double	p;
	int	ai = a[i];
	int	bi = b[i];
	p = m->m[i][0] * (m->m[ai][2] * m->m[bi][1] - m->m[ai][1] * m->m[bi][2]);
	if (i == 1)
	    p = -p;
	det += p;
    }
    if (det == 0)
	return FALSE;
    det = 1/det;
    for (j = 0; j < 3; j++) {
	for (i = 0; i < 3; i++) {
	    double  p;
	    int	    ai = a[i];
	    int	    aj = a[j];
	    int	    bi = b[i];
	    int	    bj = b[j];

	    p = m->m[ai][aj] * m->m[bi][bj] - m->m[ai][bj] * m->m[bi][aj];
	    if (((i + j) & 1) != 0)
		p = -p;
	    r->m[j][i] = det * p;
	}
    }
    return TRUE;
}

_X_EXPORT Bool
pict_f_transform_point (struct pict_f_transform *t,
			struct pict_f_vector	*v)
{
    struct pict_f_vector    result;
    int			    i, j;
    double		    a;

    for (j = 0; j < 3; j++)
    {
	a = 0;
	for (i = 0; i < 3; i++)
	    a += t->m[j][i] * v->v[i];
	result.v[j] = a;
    }
    if (!result.v[2])
	return FALSE;
    for (j = 0; j < 2; j++)
	v->v[j] = result.v[j] / result.v[2];
    v->v[2] = 1;
    return TRUE;
}

_X_EXPORT void
pict_f_transform_point_3d (struct pict_f_transform *t,
			   struct pict_f_vector	*v)
{
    struct pict_f_vector    result;
    int			    i, j;
    double		    a;

    for (j = 0; j < 3; j++)
    {
	a = 0;
	for (i = 0; i < 3; i++)
	    a += t->m[j][i] * v->v[i];
	result.v[j] = a;
    }
    *v = result;
}

_X_EXPORT void
pict_f_transform_multiply (struct pict_f_transform *dst,
			   struct pict_f_transform *l, struct pict_f_transform *r)
{
    struct pict_f_transform d;
    int			    dx, dy;
    int			    o;

    for (dy = 0; dy < 3; dy++)
	for (dx = 0; dx < 3; dx++)
	{
	    double v = 0;
	    for (o = 0; o < 3; o++)
		v += l->m[dy][o] * r->m[o][dx];
	    d.m[dy][dx] = v;
	}
    *dst = d;
}

_X_EXPORT void
pict_f_transform_init_scale (struct pict_f_transform *t, double sx, double sy)
{
    t->m[0][0] = sx;	t->m[0][1] = 0;	    t->m[0][2] = 0;
    t->m[1][0] = 0;	t->m[1][1] = sy;    t->m[1][2] = 0;
    t->m[2][0] = 0;	t->m[2][1] = 0;	    t->m[2][2] = 1;
}

_X_EXPORT Bool
pict_f_transform_scale (struct pict_f_transform *forward,
			struct pict_f_transform *reverse,
			double sx, double sy)
{
    struct pict_f_transform t;

    if (sx == 0 || sy == 0)
	return FALSE;

    pict_f_transform_init_scale (&t, sx, sy);
    pict_f_transform_multiply (forward, &t, forward);
    pict_f_transform_init_scale (&t, 1/sx, 1/sy);
    pict_f_transform_multiply (reverse, reverse, &t);
    return TRUE;
}

_X_EXPORT void
pict_f_transform_init_rotate (struct pict_f_transform *t, double c, double s)
{
    t->m[0][0] = c;	t->m[0][1] = -s;    t->m[0][2] = 0;
    t->m[1][0] = s;	t->m[1][1] = c;	    t->m[1][2] = 0;
    t->m[2][0] = 0;	t->m[2][1] = 0;	    t->m[2][2] = 1;
}

_X_EXPORT Bool
pict_f_transform_rotate (struct pict_f_transform *forward,
			 struct pict_f_transform *reverse,
			 double c, double s)
{
    struct pict_f_transform t;

    pict_f_transform_init_rotate (&t, c, s);
    pict_f_transform_multiply (forward, &t, forward);
    pict_f_transform_init_rotate (&t, c, -s);
    pict_f_transform_multiply (reverse, reverse, &t);
    return TRUE;
}

_X_EXPORT void
pict_f_transform_init_translate (struct pict_f_transform *t, double tx, double ty)
{
    t->m[0][0] = 1;	t->m[0][1] = 0;	    t->m[0][2] = tx;
    t->m[1][0] = 0;	t->m[1][1] = 1;	    t->m[1][2] = ty;
    t->m[2][0] = 0;	t->m[2][1] = 0;	    t->m[2][2] = 1;
}

_X_EXPORT Bool
pict_f_transform_translate (struct pict_f_transform *forward,
			    struct pict_f_transform *reverse,
			    double tx, double ty)
{
    struct pict_f_transform t;

    pict_f_transform_init_translate (&t, tx, ty);
    pict_f_transform_multiply (forward, &t, forward);
    pict_f_transform_init_translate (&t, -tx, -ty);
    pict_f_transform_multiply (reverse, reverse, &t);
    return TRUE;
}

_X_EXPORT Bool
pict_f_transform_bounds (struct pict_f_transform *t, BoxPtr b)
{
    struct pict_f_vector    v[4];
    int			    i;
    int			    x1, y1, x2, y2;

    v[0].v[0] = b->x1;    v[0].v[1] = b->y1;	v[0].v[2] = 1;
    v[1].v[0] = b->x2;    v[1].v[1] = b->y1;	v[1].v[2] = 1;
    v[2].v[0] = b->x2;    v[2].v[1] = b->y2;	v[2].v[2] = 1;
    v[3].v[0] = b->x1;    v[3].v[1] = b->y2;	v[3].v[2] = 1;
    for (i = 0; i < 4; i++)
    {
	if (!pict_f_transform_point (t, &v[i]))
	    return FALSE;
	x1 = floor (v[i].v[0]);
	y1 = floor (v[i].v[1]);
	x2 = ceil (v[i].v[0]);
	y2 = ceil (v[i].v[1]);
	if (i == 0)
	{
	    b->x1 = x1; b->y1 = y1;
	    b->x2 = x2; b->y2 = y2;
	}
	else
	{
	    if (x1 < b->x1) b->x1 = x1;
	    if (y1 < b->y1) b->y1 = y1;
	    if (x2 > b->x2) b->x2 = x2;
	    if (y2 > b->y2) b->y2 = y2;
	}
    }
    return TRUE;
}

_X_EXPORT void
pict_f_transform_init_identity (struct pict_f_transform *t)
{
    int	i, j;

    for (j = 0; j < 3; j++)
	for (i = 0; i < 3; i++)
	    t->m[j][i] = i == j ? 1 : 0;
}
