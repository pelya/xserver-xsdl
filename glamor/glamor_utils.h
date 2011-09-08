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
    *(_pxscale_) = 1.0 / (_pixmap_priv_)->container->drawable.width;	\
    *(_pyscale_) = 1.0 / (_pixmap_priv_)->container->drawable.height;	\
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


inline static void
glamor_calculate_boxes_bound(BoxPtr bound, BoxPtr boxes, int nbox)
{
  int x_min, y_min;
  int x_max, y_max;
  int i;
  x_min = y_min = MAXSHORT;
  x_max = y_max = MINSHORT;
  for(i = 0; i < nbox; i++)
    {
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
  for (i = 0; i < nbox; i++)
    {
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


#define GLAMOR_PIXMAP_PRIV_IS_PICTURE(pixmap_priv) (pixmap_priv->is_picture == 1)
#define GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv)    (pixmap_priv->gl_fbo == 1)

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
  case 1: return PICT_a1;
  case 4: return PICT_a4;
  case 8: return PICT_a8;
  case 15: return PICT_x1r5g5b5;
  case 16: return PICT_r5g6b5;
  default:
  case 24: return PICT_x8r8g8b8;
#if XORG_VERSION_CURRENT >= 10699900
  case 30: return PICT_x2r10g10b10;
#endif
  case 32: return PICT_a8r8g8b8;
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
					   GLenum *tex_format, 
					   GLenum *tex_type,
					   int *no_alpha,
                                           int *no_revert)
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
    LogMessageVerb(X_INFO, 0, "fail to get matched format for %x \n", format);
    return -1;
  }
  return 0;
}
#else
#define IS_LITTLE_ENDIAN  (IMAGE_BYTE_ORDER == LSBFirst)

static inline int 
glamor_get_tex_format_type_from_pictformat(PictFormatShort format,
					   GLenum *tex_format, 
					   GLenum *tex_type,
					   int *no_alpha,
                                           int *no_revert)
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
    LogMessageVerb(X_INFO, 0, "fail to get matched format for %x \n", format);
    return -1;
  }
  return 0;
}


#endif


static inline int 
glamor_get_tex_format_type_from_pixmap(PixmapPtr pixmap,
                                       GLenum *format, 
                                       GLenum *type, 
                                       int *no_alpha,
                                       int *no_revert)
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
                                                    no_alpha, no_revert);  
}


/* borrowed from uxa */
static inline Bool
glamor_get_rgba_from_pixel(CARD32 pixel,
			   float * red,
			   float * green,
			   float * blue,
			   float * alpha,
			   CARD32 format)
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
      rshift = PICT_FORMAT_BPP(format) - (rbits+gbits+bbits);
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






#endif
