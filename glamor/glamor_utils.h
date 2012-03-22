#ifndef GLAMOR_PRIV_H
#error This file can only be included by glamor_priv.h
#endif

#ifndef __GLAMOR_UTILS_H__
#define __GLAMOR_UTILS_H__

#define v_from_x_coord_x(_xscale_, _x_)          ( 2 * (_x_) * (_xscale_) - 1.0)
#define v_from_x_coord_y(_yscale_, _y_)          (-2 * (_y_) * (_yscale_) + 1.0)
#define v_from_x_coord_y_inverted(_yscale_, _y_) (2 * (_y_) * (_yscale_) - 1.0)
#define t_from_x_coord_x(_xscale_, _x_)          ((_x_) * (_xscale_))
#define t_from_x_coord_y(_yscale_, _y_)          (1.0 - (_y_) * (_yscale_))
#define t_from_x_coord_y_inverted(_yscale_, _y_) ((_y_) * (_yscale_))

#define pixmap_priv_get_scale(_pixmap_priv_, _pxscale_, _pyscale_)	\
   do {									\
    *(_pxscale_) = 1.0 / (_pixmap_priv_)->fbo->width;			\
    *(_pyscale_) = 1.0 / (_pixmap_priv_)->fbo->height;			\
  } while(0)

#define xFixedToFloat(_val_) ((float)xFixedToInt(_val_)			\
			      + ((float)xFixedFrac(_val_) / 65536.0))

#define glamor_picture_get_matrixf(_picture_, _matrix_)			\
  do {									\
    int _i_;								\
    if ((_picture_)->transform)						\
      {									\
	for(_i_ = 0; _i_ < 3; _i_++)					\
	  {								\
	    (_matrix_)[_i_ * 3 + 0] =					\
	      xFixedToFloat((_picture_)->transform->matrix[_i_][0]);	\
	    (_matrix_)[_i_ * 3 + 1] =					\
	      xFixedToFloat((_picture_)->transform->matrix[_i_][1]);	\
	    (_matrix_)[_i_ * 3 + 2] = \
	      xFixedToFloat((_picture_)->transform->matrix[_i_][2]);	\
	  }								\
      }									\
  }  while(0)

#define glamor_set_transformed_point(matrix, xscale, yscale, texcoord,	\
                                     x, y, yInverted)			\
  do {									\
    float result[4];							\
    int i;								\
    float tx, ty;							\
									\
    for (i = 0; i < 3; i++) {						\
      result[i] = (matrix)[i * 3] * (x) + (matrix)[i * 3 + 1] * (y)	\
	+ (matrix)[i * 3 + 2];						\
    }									\
    tx = result[0] / result[2];						\
    ty = result[1] / result[2];						\
									\
    (texcoord)[0] = t_from_x_coord_x(xscale, tx);			\
    if (yInverted)							\
      (texcoord)[1] = t_from_x_coord_y_inverted(yscale, ty);		\
    else								\
      (texcoord)[1] = t_from_x_coord_y(yscale, ty);			\
  } while(0)


#define glamor_set_transformed_normalize_tcoords( matrix,		\
						  xscale,		\
						  yscale,		\
                                                  tx1, ty1, tx2, ty2,   \
                                                  yInverted, texcoords)	\
  do {									\
    glamor_set_transformed_point(matrix, xscale, yscale,		\
				 texcoords, tx1, ty1,			\
				 yInverted);				\
    glamor_set_transformed_point(matrix, xscale, yscale,		\
				 texcoords + 2, tx2, ty1,		\
				 yInverted);				\
    glamor_set_transformed_point(matrix, xscale, yscale,		\
				 texcoords + 4, tx2, ty2,		\
				 yInverted);				\
    glamor_set_transformed_point(matrix, xscale, yscale,		\
				 texcoords + 6, tx1, ty2,		\
				 yInverted);				\
  } while (0)

#define glamor_set_normalize_tcoords(xscale, yscale, x1, y1, x2, y2,	\
                                     yInverted, vertices)		\
  do {									\
    (vertices)[0] = t_from_x_coord_x(xscale, x1);			\
    (vertices)[2] = t_from_x_coord_x(xscale, x2);			\
    (vertices)[4] = (vertices)[2];					\
    (vertices)[6] = (vertices)[0];					\
    if (yInverted) {							\
      (vertices)[1] = t_from_x_coord_y_inverted(yscale, y1);		\
      (vertices)[5] = t_from_x_coord_y_inverted(yscale, y2);		\
    }									\
    else {								\
      (vertices)[1] = t_from_x_coord_y(yscale, y1);			\
      (vertices)[5] = t_from_x_coord_y(yscale, y2);			\
    }									\
    (vertices)[3] = (vertices)[1];					\
    (vertices)[7] = (vertices)[5];					\
  } while(0)

#define glamor_set_tcoords(x1, y1, x2, y2, yInverted, vertices)	    \
    do {                                                            \
      (vertices)[0] = (x1);                                         \
      (vertices)[2] = (x2);                                         \
      (vertices)[4] = (vertices)[2];                                \
      (vertices)[6] = (vertices)[0];                                \
      if (yInverted) {                                              \
          (vertices)[1] = (y1);                                     \
          (vertices)[5] = (y2);                                     \
      }                                                             \
      else {                                                        \
          (vertices)[1] = (y2);                                     \
          (vertices)[5] = (y1);                                     \
      }                                                             \
      (vertices)[3] = (vertices)[1];                \
      (vertices)[7] = (vertices)[5];                \
    } while(0)


#define glamor_set_normalize_vcoords(xscale, yscale, x1, y1, x2, y2,	\
                                     yInverted, vertices)		\
  do {									\
    (vertices)[0] = v_from_x_coord_x(xscale, x1);			\
    (vertices)[2] = v_from_x_coord_x(xscale, x2);			\
    (vertices)[4] = (vertices)[2];					\
    (vertices)[6] = (vertices)[0];					\
    if (yInverted) {							\
      (vertices)[1] = v_from_x_coord_y_inverted(yscale, y1);		\
      (vertices)[5] = v_from_x_coord_y_inverted(yscale, y2);		\
    }									\
    else {								\
      (vertices)[1] = v_from_x_coord_y(yscale, y1);			\
      (vertices)[5] = v_from_x_coord_y(yscale, y2);			\
    }									\
    (vertices)[3] = (vertices)[1];					\
    (vertices)[7] = (vertices)[5];					\
  } while(0)

#define glamor_set_normalize_pt(xscale, yscale, x, x_start, y, y_start,     \
                                yInverted, pt)                              \
    do {                                                                    \
        (pt)[0] = t_from_x_coord_x(xscale, x - x_start);                    \
        if (yInverted) {                                                    \
            (pt)[1] = t_from_x_coord_y_inverted(yscale, y - y_start);       \
        } else {                                                            \
            (pt)[1] = t_from_x_coord_y(yscale, y - y_start);                \
        }                                                                   \
        (pt)[2] = (pt)[3] = 0.0;                                            \
    } while(0)

inline static void
glamor_calculate_boxes_bound(BoxPtr bound, BoxPtr boxes, int nbox)
{
	int x_min, y_min;
	int x_max, y_max;
	int i;
	x_min = y_min = MAXSHORT;
	x_max = y_max = MINSHORT;
	for (i = 0; i < nbox; i++) {
		if (x_min > boxes[i].x1)
			x_min = boxes[i].x1;
		if (y_min > boxes[i].y1)
			y_min = boxes[i].y1;

		if (x_max < boxes[i].x2)
			x_max = boxes[i].x2;
		if (y_max < boxes[i].y2)
			y_max = boxes[i].y2;
	}
	bound->x1 = x_min;
	bound->y1 = y_min;
	bound->x2 = x_max;
	bound->y2 = y_max;
}

inline static void
glamor_transform_boxes(BoxPtr boxes, int nbox, int dx, int dy)
{
	int i;
	for (i = 0; i < nbox; i++) {
		boxes[i].x1 += dx;
		boxes[i].y1 += dy;
		boxes[i].x2 += dx;
		boxes[i].y2 += dy;
	}
}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#define ALIGN(i,m)	(((i) + (m) - 1) & ~((m) - 1))
#define MIN(a,b)	((a) < (b) ? (a) : (b))

#define glamor_check_fbo_size(_glamor_,_w_, _h_)    ((_w_) > 0 && (_h_) > 0 \
                                                    && (_w_) < _glamor_->max_fbo_size  \
                                                    && (_h_) < _glamor_->max_fbo_size)

#define glamor_check_fbo_depth(_depth_) (			\
                                         _depth_ == 8		\
	                                 || _depth_ == 15	\
                                         || _depth_ == 16	\
                                         || _depth_ == 24	\
                                         || _depth_ == 30	\
                                         || _depth_ == 32)


#define GLAMOR_PIXMAP_PRIV_IS_PICTURE(pixmap_priv) (pixmap_priv && pixmap_priv->is_picture == 1)
#define GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv)    (pixmap_priv && pixmap_priv->gl_fbo == GLAMOR_FBO_NORMAL)
#define GLAMOR_PIXMAP_PRIV_HAS_FBO_DOWNLOADED(pixmap_priv)    (pixmap_priv && (pixmap_priv->gl_fbo == GLAMOR_FBO_DOWNLOADED))

#define GLAMOR_PIXMAP_PRIV_NEED_VALIDATE(pixmap_priv)  \
	(GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv) \
	&& (pixmap_priv->pending_op.type != GLAMOR_PENDING_NONE))

#define GLAMOR_PIXMAP_PRIV_NO_PENDING(pixmap_priv)   \
	(pixmap_priv->pending_op.type == GLAMOR_PENDING_NONE)

#define GLAMOR_CHECK_PENDING_FILL(_dispatch_, _glamor_priv_, _pixmap_priv_) do \
  { \
      if (_pixmap_priv_->pending_op.type == GLAMOR_PENDING_FILL) { \
        _dispatch_->glUseProgram(_glamor_priv_->solid_prog); \
        _dispatch_->glUniform4fv(_glamor_priv_->solid_color_uniform_location, 1,  \
                        _pixmap_priv_->pending_op.fill.color4fv); \
      } \
  } while(0)


/**
 * Borrow from uxa.
 */
static inline CARD32
format_for_depth(int depth)
{
	switch (depth) {
	case 1:
		return PICT_a1;
	case 4:
		return PICT_a4;
	case 8:
		return PICT_a8;
	case 15:
		return PICT_x1r5g5b5;
	case 16:
		return PICT_r5g6b5;
	default:
	case 24:
		return PICT_x8r8g8b8;
#if XORG_VERSION_CURRENT >= 10699900
	case 30:
		return PICT_x2r10g10b10;
#endif
	case 32:
		return PICT_a8r8g8b8;
	}
}

static inline void
gl_iformat_for_depth(int depth, GLenum * format)
{
	switch (depth) {
#if 0
	case 8:
		*format = GL_ALPHA;
		break;
	case 24:
		*format = GL_RGB;
		break;
#endif
	default:
		*format = GL_RGBA;
		break;
       }
}

static inline CARD32
format_for_pixmap(PixmapPtr pixmap)
{
	glamor_pixmap_private *pixmap_priv;
	PictFormatShort pict_format;

	pixmap_priv = glamor_get_pixmap_private(pixmap);
	if (GLAMOR_PIXMAP_PRIV_IS_PICTURE(pixmap_priv))
		pict_format = pixmap_priv->pict_format;
	else
		pict_format = format_for_depth(pixmap->drawable.depth);

	return pict_format;
}

/*
 * Map picture's format to the correct gl texture format and type.
 * no_alpha is used to indicate whehter we need to wire alpha to 1. 
 *
 * Return 0 if find a matched texture type. Otherwise return -1.
 **/
#ifndef GLAMOR_GLES2
static inline int
glamor_get_tex_format_type_from_pictformat(PictFormatShort format,
					   GLenum * tex_format,
					   GLenum * tex_type,
					   int *no_alpha, int *no_revert)
{
	*no_alpha = 0;
	*no_revert = 1;
	switch (format) {
	case PICT_a1:
		*tex_format = GL_COLOR_INDEX;
		*tex_type = GL_BITMAP;
		break;
	case PICT_b8g8r8x8:
		*no_alpha = 1;
	case PICT_b8g8r8a8:
		*tex_format = GL_BGRA;
		*tex_type = GL_UNSIGNED_INT_8_8_8_8;
		break;

	case PICT_x8r8g8b8:
		*no_alpha = 1;
	case PICT_a8r8g8b8:
		*tex_format = GL_BGRA;
		*tex_type = GL_UNSIGNED_INT_8_8_8_8_REV;
		break;
	case PICT_x8b8g8r8:
		*no_alpha = 1;
	case PICT_a8b8g8r8:
		*tex_format = GL_RGBA;
		*tex_type = GL_UNSIGNED_INT_8_8_8_8_REV;
		break;
	case PICT_x2r10g10b10:
		*no_alpha = 1;
	case PICT_a2r10g10b10:
		*tex_format = GL_BGRA;
		*tex_type = GL_UNSIGNED_INT_2_10_10_10_REV;
		break;
	case PICT_x2b10g10r10:
		*no_alpha = 1;
	case PICT_a2b10g10r10:
		*tex_format = GL_RGBA;
		*tex_type = GL_UNSIGNED_INT_2_10_10_10_REV;
		break;

	case PICT_r5g6b5:
		*tex_format = GL_RGB;
		*tex_type = GL_UNSIGNED_SHORT_5_6_5;
		break;
	case PICT_b5g6r5:
		*tex_format = GL_RGB;
		*tex_type = GL_UNSIGNED_SHORT_5_6_5_REV;
		break;
	case PICT_x1b5g5r5:
		*no_alpha = 1;
	case PICT_a1b5g5r5:
		*tex_format = GL_RGBA;
		*tex_type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		break;

	case PICT_x1r5g5b5:
		*no_alpha = 1;
	case PICT_a1r5g5b5:
		*tex_format = GL_BGRA;
		*tex_type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		break;
	case PICT_a8:
		*tex_format = GL_ALPHA;
		*tex_type = GL_UNSIGNED_BYTE;
		break;
	case PICT_x4r4g4b4:
		*no_alpha = 1;
	case PICT_a4r4g4b4:
		*tex_format = GL_BGRA;
		*tex_type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
		break;

	case PICT_x4b4g4r4:
		*no_alpha = 1;
	case PICT_a4b4g4r4:
		*tex_format = GL_RGBA;
		*tex_type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
		break;

	default:
		LogMessageVerb(X_INFO, 0,
			       "fail to get matched format for %x \n",
			       format);
		return -1;
	}
	return 0;
}
#else
#define IS_LITTLE_ENDIAN  (IMAGE_BYTE_ORDER == LSBFirst)

static inline int
glamor_get_tex_format_type_from_pictformat(PictFormatShort format,
					   GLenum * tex_format,
					   GLenum * tex_type,
					   int *no_alpha, int *no_revert)
{
	*no_alpha = 0;
	*no_revert = IS_LITTLE_ENDIAN;

	switch (format) {
	case PICT_b8g8r8x8:
		*no_alpha = 1;
	case PICT_b8g8r8a8:
		*tex_format = GL_BGRA;
		*tex_type = GL_UNSIGNED_BYTE;
		*no_revert = !IS_LITTLE_ENDIAN;
		break;

	case PICT_x8r8g8b8:
		*no_alpha = 1;
	case PICT_a8r8g8b8:
		*tex_format = GL_BGRA;
		*tex_type = GL_UNSIGNED_BYTE;
		break;

	case PICT_x8b8g8r8:
		*no_alpha = 1;
	case PICT_a8b8g8r8:
		*tex_format = GL_RGBA;
		*tex_type = GL_UNSIGNED_BYTE;
		break;

	case PICT_x2r10g10b10:
		*no_alpha = 1;
	case PICT_a2r10g10b10:
		*tex_format = GL_BGRA;
		*tex_type = GL_UNSIGNED_INT_10_10_10_2;
		*no_revert = TRUE;
		break;

	case PICT_x2b10g10r10:
		*no_alpha = 1;
	case PICT_a2b10g10r10:
		*tex_format = GL_RGBA;
		*tex_type = GL_UNSIGNED_INT_10_10_10_2;
		*no_revert = TRUE;
		break;

	case PICT_r5g6b5:
		*tex_format = GL_RGB;
		*tex_type = GL_UNSIGNED_SHORT_5_6_5;
		*no_revert = TRUE;
		break;

	case PICT_b5g6r5:
		*tex_format = GL_RGB;
		*tex_type = GL_UNSIGNED_SHORT_5_6_5;
		*no_revert = FALSE;
		break;

	case PICT_x1b5g5r5:
		*no_alpha = 1;
	case PICT_a1b5g5r5:
		*tex_format = GL_RGBA;
		*tex_type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		*no_revert = TRUE;
		break;

	case PICT_x1r5g5b5:
		*no_alpha = 1;
	case PICT_a1r5g5b5:
		*tex_format = GL_BGRA;
		*tex_type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		*no_revert = TRUE;
		break;

	case PICT_a8:
		*tex_format = GL_ALPHA;
		*tex_type = GL_UNSIGNED_BYTE;
		*no_revert = TRUE;
		break;

	case PICT_x4r4g4b4:
		*no_alpha = 1;
	case PICT_a4r4g4b4:
		*tex_format = GL_BGRA;
		*tex_type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
		*no_revert = TRUE;
		break;

	case PICT_x4b4g4r4:
		*no_alpha = 1;
	case PICT_a4b4g4r4:
		*tex_format = GL_RGBA;
		*tex_type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
		*no_revert = TRUE;
		break;

	default:
		LogMessageVerb(X_INFO, 0,
			       "fail to get matched format for %x \n",
			       format);
		return -1;
	}
	return 0;
}


#endif


static inline int
glamor_get_tex_format_type_from_pixmap(PixmapPtr pixmap,
				       GLenum * format,
				       GLenum * type,
				       int *no_alpha, int *no_revert)
{
	glamor_pixmap_private *pixmap_priv;
	PictFormatShort pict_format;

	pixmap_priv = glamor_get_pixmap_private(pixmap);
	if (GLAMOR_PIXMAP_PRIV_IS_PICTURE(pixmap_priv))
		pict_format = pixmap_priv->pict_format;
	else
		pict_format = format_for_depth(pixmap->drawable.depth);

	return glamor_get_tex_format_type_from_pictformat(pict_format,
							  format, type,
							  no_alpha,
							  no_revert);
}


/* borrowed from uxa */
static inline Bool
glamor_get_rgba_from_pixel(CARD32 pixel,
			   float *red,
			   float *green,
			   float *blue, float *alpha, CARD32 format)
{
	int rbits, bbits, gbits, abits;
	int rshift, bshift, gshift, ashift;

	rbits = PICT_FORMAT_R(format);
	gbits = PICT_FORMAT_G(format);
	bbits = PICT_FORMAT_B(format);
	abits = PICT_FORMAT_A(format);

	if (PICT_FORMAT_TYPE(format) == PICT_TYPE_A) {
		rshift = gshift = bshift = ashift = 0;
	} else if (PICT_FORMAT_TYPE(format) == PICT_TYPE_ARGB) {
		bshift = 0;
		gshift = bbits;
		rshift = gshift + gbits;
		ashift = rshift + rbits;
	} else if (PICT_FORMAT_TYPE(format) == PICT_TYPE_ABGR) {
		rshift = 0;
		gshift = rbits;
		bshift = gshift + gbits;
		ashift = bshift + bbits;
#if XORG_VERSION_CURRENT >= 10699900
	} else if (PICT_FORMAT_TYPE(format) == PICT_TYPE_BGRA) {
		ashift = 0;
		rshift = abits;
		if (abits == 0)
			rshift = PICT_FORMAT_BPP(format) - (rbits + gbits +
							    bbits);
		gshift = rshift + rbits;
		bshift = gshift + gbits;
#endif
	} else {
		return FALSE;
	}
#define COLOR_INT_TO_FLOAT(_fc_, _p_, _s_, _bits_)	\
  *_fc_ = (((_p_) >> (_s_)) & (( 1 << (_bits_)) - 1))	\
    / (float)((1<<(_bits_)) - 1)

	if (rbits)
		COLOR_INT_TO_FLOAT(red, pixel, rshift, rbits);
	else
		*red = 0;

	if (gbits)
		COLOR_INT_TO_FLOAT(green, pixel, gshift, gbits);
	else
		*green = 0;

	if (bbits)
		COLOR_INT_TO_FLOAT(blue, pixel, bshift, bbits);
	else
		*blue = 0;

	if (abits)
		COLOR_INT_TO_FLOAT(alpha, pixel, ashift, abits);
	else
		*alpha = 1;

	return TRUE;
}

inline static Bool glamor_pict_format_is_compatible(PictFormatShort pict_format, int depth)
{
	GLenum iformat;

	gl_iformat_for_depth(depth, &iformat);
	switch (iformat) {
		case GL_RGBA:
			return (pict_format == PICT_a8r8g8b8 || pict_format == PICT_x8r8g8b8);
		case GL_ALPHA:
			return (pict_format == PICT_a8);
		default:
			return FALSE;
	}
}

/* return TRUE if we can access this pixmap at DDX driver. */
inline static Bool glamor_ddx_fallback_check_pixmap(DrawablePtr drawable)
{
	PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
	glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
	return (!pixmap_priv 
		|| (pixmap_priv->type == GLAMOR_TEXTURE_DRM
		    || pixmap_priv->type == GLAMOR_MEMORY
		    || pixmap_priv->type == GLAMOR_DRM_ONLY));
}

inline static Bool glamor_ddx_fallback_check_gc(GCPtr gc)
{
	PixmapPtr pixmap;
	if (!gc)
		return TRUE;
        switch (gc->fillStyle) {
        case FillStippled:
        case FillOpaqueStippled:
                pixmap = gc->stipple;
                break;
        case FillTiled:
                pixmap = gc->tile.pixmap;
                break;
	default:
		pixmap = NULL;
        }
	return (!pixmap || glamor_ddx_fallback_check_pixmap(&pixmap->drawable));
}

inline static Bool glamor_tex_format_is_readable(GLenum format)
{
	return ((format == GL_RGBA || format == GL_RGB || format == GL_ALPHA));

}

static inline void _glamor_dump_pixmap_byte(PixmapPtr pixmap, int x, int y, int w, int h)
{
	int i,j;
	unsigned char * p = pixmap->devPrivate.ptr;
	int stride = pixmap->devKind;

	p = p + y * stride + x;
	ErrorF("devKind %d, x %d y %d w %d h %d width %d height %d\n", stride, x, y, w, h, pixmap->drawable.width, pixmap->drawable.height);

	for (i = 0; i < h; i++)
	{
		ErrorF("line %3d: ", i);
		for(j = 0; j < w; j++)
			ErrorF("%2x ", p[j]);
		p += stride;
		ErrorF("\n");
	}
}

static inline void _glamor_dump_pixmap_word(PixmapPtr pixmap, int x, int y, int w, int h)
{
	int i,j;
	unsigned int * p = pixmap->devPrivate.ptr;
	int stride = pixmap->devKind / 4;

	p = p + y * stride + x;

	for (i = 0; i < h; i++)
	{
		ErrorF("line %3d: ", i);
		for(j = 0; j < w; j++)
			ErrorF("%2x ", p[j]);
		p += stride;
		ErrorF("\n");
	}
}

static inline void glamor_dump_pixmap(PixmapPtr pixmap, int x, int y, int w, int h)
{
	glamor_prepare_access(&pixmap->drawable, GLAMOR_ACCESS_RO);
	switch (pixmap->drawable.depth) {
	case 8:
		_glamor_dump_pixmap_byte(pixmap, x, y, w, h);
		break;
	case 24:
	case 32:
		_glamor_dump_pixmap_word(pixmap, x, y, w, h);
		break;
	default:
		assert(0);
	}
	glamor_finish_access(&pixmap->drawable, GLAMOR_ACCESS_RO);
}

static inline void _glamor_compare_pixmaps(PixmapPtr pixmap1, PixmapPtr pixmap2,
                                           int x, int y, int w, int h,
                                           PictFormatShort short_format,
                                           int all, int diffs)
{
	int i, j;
	unsigned char * p1 = pixmap1->devPrivate.ptr;
	unsigned char * p2 = pixmap2->devPrivate.ptr;
	int line_need_printed = 0;
	int test_code = 0xAABBCCDD;
	int little_endian = 0;
	unsigned char *p_test;
	int bpp = pixmap1->drawable.depth == 8 ? 1 : 4;

	assert(pixmap1->devKind == pixmap2->devKind);
	int stride = pixmap1->devKind;

	ErrorF("stride:%d, width:%d, height:%d\n", stride, w, h);

	p1 = p1 + y * stride + x;
	p2 = p2 + y * stride + x;

	if (all) {
		for (i = 0; i < h; i++) {
			ErrorF("line %3d: ", i);

			for (j = 0; j < stride; j++) {
				if (j % bpp == 0)
					ErrorF("[%d]%2x:%2x ", j / bpp, p1[j], p2[j]);
				else
					ErrorF("%2x:%2x ", p1[j], p2[j]);
			}

			p1 += stride;
			p2 += stride;
			ErrorF("\n");
		}
	} else {
		if (short_format == PICT_a8r8g8b8) {
			p_test = (unsigned char *) & test_code;
			little_endian = (*p_test == 0xDD);
			bpp = 4;

			for (i = 0; i < h; i++) {
				line_need_printed = 0;

				for (j = 0; j < stride; j++) {
					if (p1[j] != p2[j] && (p1[j] - p2[j] > diffs || p2[j] - p1[j] > diffs)) {
						if (line_need_printed) {
							if (little_endian) {
								switch (j % 4) {
									case 2:
										ErrorF("[%d]RED:%2x:%2x ", j / bpp, p1[j], p2[j]);
										break;
									case 1:
										ErrorF("[%d]GREEN:%2x:%2x ", j / bpp, p1[j], p2[j]);
										break;
									case 0:
										ErrorF("[%d]BLUE:%2x:%2x ", j / bpp, p1[j], p2[j]);
										break;
									case 3:
										ErrorF("[%d]Alpha:%2x:%2x ", j / bpp, p1[j], p2[j]);
										break;
								}
							} else {
								switch (j % 4) {
									case 1:
										ErrorF("[%d]RED:%2x:%2x ", j / bpp, p1[j], p2[j]);
										break;
									case 2:
										ErrorF("[%d]GREEN:%2x:%2x ", j / bpp, p1[j], p2[j]);
										break;
									case 3:
										ErrorF("[%d]BLUE:%2x:%2x ", j / bpp, p1[j], p2[j]);
										break;
									case 0:
										ErrorF("[%d]Alpha:%2x:%2x ", j / bpp, p1[j], p2[j]);
										break;
								}
							}
						} else {
							line_need_printed = 1;
							j = -1;
							ErrorF("line %3d: ", i);
							continue;
						}
					}
				}

				p1 += stride;
				p2 += stride;
				ErrorF("\n");
			}
		} //more format can be added here.
		else { // the default format, just print.
			for (i = 0; i < h; i++) {
				line_need_printed = 0;

				for (j = 0; j < stride; j++) {
					if (p1[j] != p2[j]) {
						if (line_need_printed) {
							ErrorF("[%d]%2x:%2x ", j / bpp, p1[j], p2[j]);
						} else {
							line_need_printed = 1;
							j = -1;
							ErrorF("line %3d: ", i);
							continue;
						}
					}
				}

				p1 += stride;
				p2 += stride;
				ErrorF("\n");
			}
		}
	}
}

static inline void glamor_compare_pixmaps(PixmapPtr pixmap1, PixmapPtr pixmap2,
                                          int x, int y, int w, int h, int all, int diffs)
{
	assert(pixmap1->drawable.depth == pixmap2->drawable.depth);

	glamor_prepare_access(&pixmap1->drawable, GLAMOR_ACCESS_RO);
	glamor_prepare_access(&pixmap2->drawable, GLAMOR_ACCESS_RO);

	_glamor_compare_pixmaps(pixmap1, pixmap2, x, y, w, h, -1, all, diffs);

	glamor_finish_access(&pixmap1->drawable, GLAMOR_ACCESS_RO);
	glamor_finish_access(&pixmap2->drawable, GLAMOR_ACCESS_RO);
}

/* This function is used to compare two pictures.
   If the picture has no drawable, we use fb functions to generate it. */
static inline void glamor_compare_pictures( ScreenPtr screen,
                                            PicturePtr fst_picture,
                                            PicturePtr snd_picture,
                                            int x_source, int y_source,
                                            int width, int height,
                                            int all, int diffs)
{
	PixmapPtr fst_pixmap;
	PixmapPtr snd_pixmap;
	int fst_generated, snd_generated;
	glamor_pixmap_private *fst_pixmap_priv;
	glamor_pixmap_private *snd_pixmap_priv;
	int error;
	int fst_type = -1;
	int snd_type = -1; // -1 represent has drawable.

	if (fst_picture->format != snd_picture->format) {
		ErrorF("Different picture format can not compare!\n");
		return;
	}

	if (!fst_picture->pDrawable) {
		fst_type = fst_picture->pSourcePict->type;
	}

	if (!snd_picture->pDrawable) {
		snd_type = snd_picture->pSourcePict->type;
	}

	if ((fst_type != -1) && (snd_type != -1) && (fst_type != snd_type)) {
		ErrorF("Different picture type will never be same!\n");
		return;
	}

	fst_generated = snd_generated = 0;

	if (!fst_picture->pDrawable) {
		PicturePtr pixman_pic;
		PixmapPtr pixmap = NULL;
		PictFormatShort format;

		format = fst_picture->format;

		pixmap = glamor_create_pixmap(screen,
		                              width, height,
		                              PIXMAN_FORMAT_DEPTH(format),
		                              GLAMOR_CREATE_PIXMAP_CPU);

		pixman_pic = CreatePicture(0,
		                           &pixmap->drawable,
		                           PictureMatchFormat(screen,
		                               PIXMAN_FORMAT_DEPTH(format), format),
		                           0, 0, serverClient, &error);

		fbComposite(PictOpSrc, fst_picture, NULL, pixman_pic,
		            x_source, y_source,
		            0, 0,
		            0, 0,
		            width, height);

		glamor_destroy_pixmap(pixmap);

		fst_picture = pixman_pic;
		fst_generated = 1;
	}

	if (!snd_picture->pDrawable) {
		PicturePtr pixman_pic;
		PixmapPtr pixmap = NULL;
		PictFormatShort format;

		format = snd_picture->format;

		pixmap = glamor_create_pixmap(screen,
		                              width, height,
		                              PIXMAN_FORMAT_DEPTH(format),
		                              GLAMOR_CREATE_PIXMAP_CPU);

		pixman_pic = CreatePicture(0,
		                           &pixmap->drawable,
		                           PictureMatchFormat(screen,
		                               PIXMAN_FORMAT_DEPTH(format), format),
		                           0, 0, serverClient, &error);

		fbComposite(PictOpSrc, snd_picture, NULL, pixman_pic,
		            x_source, y_source,
		            0, 0,
		            0, 0,
		            width, height);

		glamor_destroy_pixmap(pixmap);

		snd_picture = pixman_pic;
		snd_generated = 1;
	}

	fst_pixmap = glamor_get_drawable_pixmap(fst_picture->pDrawable);
	snd_pixmap = glamor_get_drawable_pixmap(snd_picture->pDrawable);

	if (fst_pixmap->drawable.depth != snd_pixmap->drawable.depth) {
		if (fst_generated)
			glamor_destroy_picture(fst_picture);
		if (snd_generated)
			glamor_destroy_picture(snd_picture);

		ErrorF("Different pixmap depth can not compare!\n");
		return;
	}

	glamor_prepare_access(&fst_pixmap->drawable, GLAMOR_ACCESS_RO);
	glamor_prepare_access(&snd_pixmap->drawable, GLAMOR_ACCESS_RO);

	if ((fst_type == SourcePictTypeLinear) ||
	     (fst_type == SourcePictTypeRadial) ||
	     (fst_type == SourcePictTypeConical) ||
	     (snd_type == SourcePictTypeLinear) ||
	     (snd_type == SourcePictTypeRadial) ||
	     (snd_type == SourcePictTypeConical)) {
		x_source = y_source = 0;
	}

	_glamor_compare_pixmaps(fst_pixmap, snd_pixmap,
	                        x_source, y_source,
	                        width, height,
	                        fst_picture->format, all, diffs);

	glamor_finish_access(&fst_pixmap->drawable, GLAMOR_ACCESS_RO);
	glamor_finish_access(&snd_pixmap->drawable, GLAMOR_ACCESS_RO);

	if (fst_generated)
		glamor_destroy_picture(fst_picture);
	if (snd_generated)
		glamor_destroy_picture(snd_picture);

	return;
}

static inline void glamor_make_current(ScreenPtr screen)
{
	glamor_egl_make_current(screen);
}

static inline void glamor_restore_current(ScreenPtr screen)
{
	glamor_egl_restore_context(screen);
}

#ifdef GLX_USE_SHARED_DISPATCH
static inline glamor_gl_dispatch *
glamor_get_dispatch(glamor_screen_private *glamor_priv)
{
	if (glamor_priv->flags & GLAMOR_USE_EGL_SCREEN)
		glamor_make_current(glamor_priv->screen);

	return &glamor_priv->_dispatch;
}

static inline void
glamor_put_dispatch(glamor_screen_private *glamor_priv)
{
	if (glamor_priv->flags & GLAMOR_USE_EGL_SCREEN)
		glamor_restore_current(glamor_priv->screen);
}
#else
#warning "Indirect GLX may be broken, need to implement context switch."
static inline glamor_gl_dispatch *
glamor_get_dispatch(glamor_screen_private *glamor_priv)
{
	return &glamor_priv->_dispatch;
}

static inline void
glamor_put_dispatch(glamor_screen_private *glamor_priv)
{
}

#endif

#endif
