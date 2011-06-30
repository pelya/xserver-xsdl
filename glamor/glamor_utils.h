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
        x_min = boxes[i].y1;

      if (x_max < boxes[i].x2)
        x_max = boxes[i].x2;
      if (y_max > boxes[i].y2)
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
#endif
