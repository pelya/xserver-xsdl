#include <stdlib.h>

#include "glamor_priv.h"

/**
 * Clip the boxes regards to each pixmap's block array.
 *
 * Should translate the region to relative coords to the pixmap,
 * start at (0,0).
 */
#if 0
//#define DEBUGF(str, ...)  do {} while(0)
#define DEBUGF(str, ...) ErrorF(str, ##__VA_ARGS__)
//#define DEBUGRegionPrint(x) do {} while (0)
#define DEBUGRegionPrint RegionPrint
#endif

static glamor_pixmap_clipped_regions *
__glamor_compute_clipped_regions(int block_w,
			       int block_h,
			       int block_stride,
			       int x, int y,
			       int w, int h,
                               RegionPtr region,
                               int *n_region,
                               int repeat)
{
	glamor_pixmap_clipped_regions * clipped_regions;
	BoxPtr extent;
	int start_x, start_y, end_x, end_y;
	int start_block_x, start_block_y;
	int end_block_x, end_block_y;
	int i, j;
	int width, height;
	RegionRec temp_region;
	RegionPtr current_region;
	int block_idx;
	int k = 0;
	int temp_block_idx;

	extent = RegionExtents(region);
	start_x = MAX(x, extent->x1);
	start_y = MAX(y, extent->y1);
	end_x = MIN(x + w, extent->x2);
	end_y = MIN(y + h, extent->y2);

	DEBUGF("start compute clipped regions:\n");
	DEBUGF("block w %d h %d  x %d y %d w %d h %d, block_stride %d \n",
		block_w, block_h, x, y, w, h, block_stride);
	DEBUGRegionPrint(region);

	DEBUGF("start_x %d start_y %d end_x %d end_y %d \n", start_x, start_y, end_x, end_y);

	if (start_x >= end_x || start_y >= end_y) {
		*n_region = 0;
		return NULL;
	}

	width = end_x - start_x;
	height = end_y - start_y;
	start_block_x = (start_x  - x)/ block_w;
	start_block_y = (start_y - y)/ block_h;
	end_block_x = (end_x - x)/ block_w;
	end_block_y = (end_y - y)/ block_h;

	clipped_regions = calloc((end_block_x - start_block_x + 1)
				 * (end_block_y - start_block_y + 1),
				 sizeof(*clipped_regions));

	block_idx = (start_block_y - 1) * block_stride;

	DEBUGF("startx %d starty %d endx %d endy %d \n",
		start_x, start_y, end_x, end_y);
	DEBUGF("start_block_x %d end_block_x %d \n", start_block_x, end_block_x);
	DEBUGF("start_block_y %d end_block_y %d \n", start_block_y, end_block_y);

	for(j = start_block_y; j <= end_block_y; j++)
	{
		block_idx += block_stride;
		temp_block_idx = block_idx + start_block_x;
		for(i = start_block_x;
		    i <= end_block_x; i++, temp_block_idx++)
		{
			BoxRec temp_box;
			temp_box.x1 = x + i * block_w;
			temp_box.y1 = y + j * block_h;
			temp_box.x2 = MIN(temp_box.x1 + block_w, end_x);
			temp_box.y2 = MIN(temp_box.y1 + block_h, end_y);
			RegionInitBoxes(&temp_region, &temp_box, 1);
			DEBUGF("block idx %d \n",temp_block_idx);
			DEBUGRegionPrint(&temp_region);
			current_region = RegionCreate(NULL, 4);
			RegionIntersect(current_region, &temp_region, region);
			DEBUGF("i %d j %d  region: \n",i ,j);
			DEBUGRegionPrint(current_region);
			if (RegionNumRects(current_region)) {
				clipped_regions[k].region = current_region;
				clipped_regions[k].block_idx = temp_block_idx;
				k++;
			} else
				RegionDestroy(current_region);
			RegionUninit(&temp_region);
		}
	}

	*n_region = k;
	return clipped_regions;
}

/**
 * Do a two round clipping,
 * first is to clip the region regard to current pixmap's
 * block array. Then for each clipped region, do a inner
 * block clipping. This is to make sure the final result
 * will be shapped by inner_block_w and inner_block_h, and
 * the final region also will not cross the pixmap's block
 * boundary.
 *
 * This is mainly used by transformation support when do
 * compositing.
 */

glamor_pixmap_clipped_regions *
glamor_compute_clipped_regions_ext(glamor_pixmap_private *pixmap_priv,
				   RegionPtr region,
				   int *n_region,
				   int inner_block_w, int inner_block_h)
{
	glamor_pixmap_clipped_regions * clipped_regions, *inner_regions, *result_regions;
	int i, j, x, y, k, inner_n_regions;
	int width, height;
	glamor_pixmap_private_large_t *priv;
	priv = &pixmap_priv->large;

	DEBUGF("ext called \n");

	if (pixmap_priv->type != GLAMOR_TEXTURE_LARGE) {
		clipped_regions = calloc(1, sizeof(*clipped_regions));
		if (clipped_regions == NULL) {
			*n_region = 0;
			return NULL;
		}
		clipped_regions[0].region = RegionCreate(NULL, 1);
		clipped_regions[0].block_idx = 0;
		RegionCopy(clipped_regions[0].region, region);
		*n_region = 1;
		priv->block_w = priv->base.pixmap->drawable.width;
		priv->block_h = priv->base.pixmap->drawable.height;
		priv->box_array = &priv->box;
		priv->box.x1 = priv->box.y1 = 0;
		priv->box.x2 = priv->block_w;
		priv->box.y2 = priv->block_h;
	} else {
		clipped_regions =  __glamor_compute_clipped_regions(priv->block_w,
					priv->block_h,
					priv->block_wcnt,
					0, 0,
					priv->base.pixmap->drawable.width,
					priv->base.pixmap->drawable.height,
					region, n_region, 0
					);

		if (clipped_regions == NULL) {
			*n_region = 0;
			return NULL;
		}
	}
	if (inner_block_w >= priv->block_w
	    && inner_block_h >= priv->block_h)
		return clipped_regions;
	result_regions = calloc(*n_region
				* ((priv->block_w + inner_block_w - 1)/inner_block_w)
				* ((priv->block_h + inner_block_h - 1)/ inner_block_h),
				sizeof(*result_regions));
	k = 0;
	for(i = 0; i < *n_region; i++)
	{
		x = priv->box_array[clipped_regions[i].block_idx].x1;
		y = priv->box_array[clipped_regions[i].block_idx].y1;
		width = priv->box_array[clipped_regions[i].block_idx].x2 - x;
		height = priv->box_array[clipped_regions[i].block_idx].y2 - y;
		inner_regions = __glamor_compute_clipped_regions(inner_block_w,
					inner_block_h,
					0, x, y,
					width,
					height,
					clipped_regions[i].region,
					&inner_n_regions, 0);
		for(j = 0; j < inner_n_regions; j++)
		{
			result_regions[k].region = inner_regions[j].region;
			result_regions[k].block_idx = clipped_regions[i].block_idx;
			k++;
		}
		free(inner_regions);
	}
	*n_region = k;
	free(clipped_regions);
	return result_regions;
}

/**
 * Clip the boxes regards to each pixmap's block array.
 *
 * Should translate the region to relative coords to the pixmap,
 * start at (0,0).
 *
 * @is_transform: if it is set, it has a transform matrix.
 *
 * XXX Not support repeatPad currently.
 */

static glamor_pixmap_clipped_regions *
_glamor_compute_clipped_regions(glamor_pixmap_private *pixmap_priv,
				RegionPtr region, int *n_region,
				int repeat_type, int is_transform)
{
	glamor_pixmap_clipped_regions * clipped_regions;
	BoxPtr extent;
	int i, j;
	int width, height;
	RegionPtr current_region;
	int pixmap_width, pixmap_height;
	int m;
	BoxRec repeat_box;
	RegionRec repeat_region;
	int right_shift = 0;
	int down_shift = 0;
	int x_center_shift = 0, y_center_shift = 0;
	glamor_pixmap_private_large_t *priv;
	priv = &pixmap_priv->large;

	DEBUGRegionPrint(region);
	if (pixmap_priv->type != GLAMOR_TEXTURE_LARGE) {
		clipped_regions = calloc(1, sizeof(*clipped_regions));
		clipped_regions[0].region = RegionCreate(NULL, 1);
		clipped_regions[0].block_idx = 0;
		RegionCopy(clipped_regions[0].region, region);
		*n_region = 1;
		return clipped_regions;
	}

	pixmap_width = priv->base.pixmap->drawable.width;
	pixmap_height = priv->base.pixmap->drawable.height;
	if (repeat_type == 0) {
		clipped_regions = __glamor_compute_clipped_regions(priv->block_w,
							priv->block_h,
							priv->block_wcnt,
							0, 0,
							priv->base.pixmap->drawable.width,
							priv->base.pixmap->drawable.height,
							region, n_region, 0
							);
		return clipped_regions;
	} else if (repeat_type != RepeatNormal) {
		*n_region = 0;
		return NULL;
	}
	extent = RegionExtents(region);

	x_center_shift = extent->x1 / pixmap_width;
	if (x_center_shift < 0)
		x_center_shift--;
	if (abs(x_center_shift) & 1)
		x_center_shift++;
	y_center_shift = extent->y1 / pixmap_height;
	if (y_center_shift < 0)
		y_center_shift--;
	if (abs(y_center_shift) & 1)
		y_center_shift++;

	if (extent->x1 < 0)
		right_shift = ((-extent->x1 + pixmap_width - 1) / pixmap_width );
	if (extent->y1 < 0)
		down_shift = ((-extent->y1 + pixmap_height - 1) / pixmap_height );

	if (right_shift != 0 || down_shift != 0) {
		if (repeat_type == RepeatReflect) {
			right_shift = (right_shift + 1)&~1;
			down_shift = (down_shift + 1)&~1;
		}
		RegionTranslate(region, right_shift * pixmap_width, down_shift * pixmap_height);
	}

	extent = RegionExtents(region);
	width = extent->x2 - extent->x1;
	height = extent->y2 - extent->y1;
	/* Tile a large pixmap to another large pixmap.
	 * We can't use the target large pixmap as the
	 * loop variable, instead we need to loop for all
	 * the blocks in the tile pixmap.
	 *
	 * simulate repeat each single block to cover the
	 * target's blocks. Two special case:
	 * a block_wcnt == 1 or block_hcnt ==1, then we
	 * only need to loop one direction as the other
	 * direction is fully included in the first block.
	 *
	 * For the other cases, just need to start
	 * from a proper shiftx/shifty, and then increase
	 * y by tile_height each time to walk trhough the
	 * target block and then walk trhough the target
	 * at x direction by increate tile_width each time.
	 *
	 * This way, we can consolidate all the sub blocks
	 * of the target boxes into one tile source's block.
	 *
	 * */
	m = 0;
	clipped_regions = calloc(priv->block_wcnt * priv->block_hcnt,
				 sizeof(*clipped_regions));
	if (clipped_regions == NULL) {
		*n_region = 0;
		return NULL;
	}
	if (right_shift != 0 || down_shift != 0) {
		DEBUGF("region to be repeated shifted \n");
		DEBUGRegionPrint(region);
	}
	DEBUGF("repeat pixmap width %d height %d \n", pixmap_width, pixmap_height);
	DEBUGF("extent x1 %d y1 %d x2 %d y2 %d \n", extent->x1, extent->y1, extent->x2, extent->y2);
	for(j = 0; j < priv->block_hcnt; j++)
	{
		for(i = 0; i < priv->block_wcnt; i++)
		{
			int dx = pixmap_width;
			int dy = pixmap_height;
			int idx;
			int shift_x;
			int shift_y;
			int saved_y1, saved_y2;
			int x_idx = 0, y_idx = 0;
			RegionRec temp_region;

			shift_x = (extent->x1 / pixmap_width) * pixmap_width;
			shift_y = (extent->y1 / pixmap_height) * pixmap_height;
			idx = j * priv->block_wcnt + i;
			if (repeat_type == RepeatReflect) {
				x_idx = (extent->x1 / pixmap_width);
				y_idx = (extent->y1 / pixmap_height);
			}

			/* Construct a rect to clip the target region. */
			repeat_box.x1 = shift_x + priv->box_array[idx].x1;
			repeat_box.y1 = shift_y + priv->box_array[idx].y1;
			if (priv->block_wcnt == 1)
				repeat_box.x2 = extent->x2;
			else
				repeat_box.x2 = shift_x + priv->box_array[idx].x2;
			if (priv->block_hcnt == 1)
				repeat_box.y2 = extent->y2;
			else
				repeat_box.y2 = shift_y + priv->box_array[idx].y2;

			current_region = RegionCreate(NULL, 4);
			RegionInit(&temp_region, NULL, 4);
			DEBUGF("init repeat box %d %d %d %d \n",
				repeat_box.x1, repeat_box.y1, repeat_box.x2, repeat_box.y2);

			if (repeat_type == RepeatNormal) {
				saved_y1 = repeat_box.y1;
				saved_y2 = repeat_box.y2;
				for(; repeat_box.x1 < extent->x2;
				      repeat_box.x1 += dx, repeat_box.x2 += dx)
				{
					repeat_box.y1 = saved_y1;
					repeat_box.y2 = saved_y2;
					for( repeat_box.y1 = saved_y1, repeat_box.y2 = saved_y2;
					     repeat_box.y1 < extent->y2;
					     repeat_box.y1 += dy, repeat_box.y2 += dy)
					{

						RegionInitBoxes(&repeat_region, &repeat_box, 1);
						DEBUGF("Start to clip repeat region: \n");
						DEBUGRegionPrint(&repeat_region);
						RegionIntersect(&temp_region, &repeat_region, region);
						DEBUGF("clip result:\n");
						DEBUGRegionPrint(&temp_region);
						RegionAppend(current_region, &temp_region);
						RegionUninit(&repeat_region);
					}
				}
			}
			DEBUGF("dx %d dy %d \n", dx, dy);

			if (RegionNumRects(current_region)) {

				if ((right_shift != 0 || down_shift != 0))
					RegionTranslate(current_region,
							-right_shift * pixmap_width,
							-down_shift * pixmap_height);
				clipped_regions[m].region = current_region;
				clipped_regions[m].block_idx = idx;
				m++;
			} else
				RegionDestroy(current_region);
			RegionUninit(&temp_region);
		}
	}

	if (right_shift != 0 || down_shift != 0)
		RegionTranslate(region, -right_shift * pixmap_width, -down_shift * pixmap_height);
	*n_region = m;

	return clipped_regions;
}

glamor_pixmap_clipped_regions *
glamor_compute_clipped_regions(glamor_pixmap_private *priv, RegionPtr region, int *n_region, int repeat_type)
{
	return _glamor_compute_clipped_regions(priv, region, n_region, repeat_type, 0);
}

Bool
glamor_composite_largepixmap_region(CARD8 op,
			  PicturePtr source,
			  PicturePtr mask,
			  PicturePtr dest,
			  glamor_pixmap_private * source_pixmap_priv,
			  glamor_pixmap_private * mask_pixmap_priv,
			  glamor_pixmap_private * dest_pixmap_priv,
			  RegionPtr region, Bool force_clip,
			  INT16 x_source,
			  INT16 y_source,
			  INT16 x_mask,
			  INT16 y_mask,
			  INT16 x_dest, INT16 y_dest,
			  CARD16 width, CARD16 height)
{
	glamor_screen_private *glamor_priv;
	glamor_pixmap_clipped_regions *clipped_dest_regions;
	glamor_pixmap_clipped_regions *clipped_source_regions;
	glamor_pixmap_clipped_regions *clipped_mask_regions;
	int n_dest_regions;
	int n_mask_regions;
	int n_source_regions;
	int i,j,k;
	int need_clean_source_fbo = 0;
	int need_clean_mask_fbo = 0;
	int is_normal_source_fbo = 0;
	int is_normal_mask_fbo = 0;
	int fixed_block_width, fixed_block_height;
	int null_source, null_mask;
	glamor_pixmap_private * need_free_source_pixmap_priv = NULL;
	glamor_pixmap_private * need_free_mask_pixmap_priv = NULL;
	int source_repeat_type = 0, mask_repeat_type = 0;
	PixmapPtr source_pixmap = NULL;
	PixmapPtr mask_pixmap = NULL;
	int ok = TRUE;

	if (source_pixmap_priv) {
		source_pixmap = source_pixmap_priv->base.pixmap;
		if (source_pixmap_priv->type == GLAMOR_TEXTURE_LARGE
		    && (source->transform
			|| source->repeatType != RepeatNone))
			return FALSE;
	}

	if (mask_pixmap_priv) {
		mask_pixmap = mask_pixmap_priv->base.pixmap;
		if (mask_pixmap_priv->type == GLAMOR_TEXTURE_LARGE
		    && (mask->transform
		         || mask->repeatType != RepeatNone))
			return FALSE;
	}

	if (source->repeat)
		source_repeat_type = source->repeatType;
	else
		source_repeat_type = RepeatNone;

	if (mask && mask->repeat)
		mask_repeat_type = mask->repeatType;
	else
		mask_repeat_type = RepeatNone;

	glamor_priv = dest_pixmap_priv->base.glamor_priv;
	fixed_block_width = glamor_priv->max_fbo_size;
	fixed_block_height = glamor_priv->max_fbo_size;
	RegionTranslate(region, -dest->pDrawable->x,
			-dest->pDrawable->y);

	if (force_clip)
		clipped_dest_regions = glamor_compute_clipped_regions_ext(dest_pixmap_priv,
									  region,
									  &n_dest_regions,
									  fixed_block_width,
									  fixed_block_height);
	else
		clipped_dest_regions = glamor_compute_clipped_regions(dest_pixmap_priv,
								      region,
								      &n_dest_regions,
								      0);
	DEBUGF("dest clipped result %d region: \n", n_dest_regions);
	for(i = 0; i < n_dest_regions; i++)
	{
		DEBUGF("dest region %d  idx %d\n", i, clipped_dest_regions[i].block_idx);
		DEBUGRegionPrint(clipped_dest_regions[i].region);
		SET_PIXMAP_FBO_CURRENT(dest_pixmap_priv, clipped_dest_regions[i].block_idx);
		if ( source_pixmap_priv && source_pixmap_priv->type == GLAMOR_TEXTURE_LARGE) {
				RegionTranslate(clipped_dest_regions[i].region,
						x_source - x_dest,
						y_source - y_dest);
				clipped_source_regions = glamor_compute_clipped_regions(source_pixmap_priv,
										        clipped_dest_regions[i].region,
										        &n_source_regions, source_repeat_type);
			DEBUGF("source clipped result %d region: \n", n_source_regions);
			for(j = 0; j < n_source_regions; j++)
			{
				SET_PIXMAP_FBO_CURRENT(source_pixmap_priv,
						       clipped_source_regions[j].block_idx);

				if (mask_pixmap_priv && mask_pixmap_priv->type == GLAMOR_TEXTURE_LARGE) {
					DEBUGF("source region %d  idx %d\n", j, clipped_source_regions[j].block_idx);
					DEBUGRegionPrint(clipped_source_regions[j].region);
					RegionTranslate(clipped_source_regions[j].region,
							- x_source + x_mask,
							- y_source + y_mask);
					clipped_mask_regions = glamor_compute_clipped_regions(mask_pixmap_priv,
										     clipped_source_regions[j].region,
										     &n_mask_regions, mask_repeat_type);
					DEBUGF("mask clipped result %d region: \n", n_mask_regions);

#define COMPOSITE_REGION(region) do {				\
	if (!glamor_composite_clipped_region(op,		\
			 source,				\
			 mask, dest, region,			\
			 x_source, y_source, x_mask, y_mask,	\
			 x_dest, y_dest)) {			\
		assert(0);					\
	}							\
   } while(0)
					for(k = 0; k < n_mask_regions; k++)
					{
						DEBUGF("mask region %d  idx %d\n", k, clipped_mask_regions[k].block_idx);
						DEBUGRegionPrint(clipped_mask_regions[k].region);
						SET_PIXMAP_FBO_CURRENT(mask_pixmap_priv,
							       clipped_mask_regions[k].block_idx);
						DEBUGF("mask fbo off %d %d \n",
							mask_pixmap_priv->large.box.x1,
							mask_pixmap_priv->large.box.y1);
						DEBUGF("start composite mask hasn't transform.\n");
						RegionTranslate(clipped_mask_regions[k].region,
								x_dest - x_mask + dest->pDrawable->x,
								y_dest - y_mask + dest->pDrawable->y);
						COMPOSITE_REGION(clipped_mask_regions[k].region);
						RegionDestroy(clipped_mask_regions[k].region);
					}
					free(clipped_mask_regions);
				} else {
					RegionTranslate(clipped_source_regions[j].region,
							-x_source + x_dest + dest->pDrawable->x,
							-y_source + y_dest + dest->pDrawable->y);
					COMPOSITE_REGION(clipped_source_regions[j].region);
				}
				if (clipped_source_regions && clipped_source_regions[j].region)
					RegionDestroy(clipped_source_regions[j].region);
			}
			free(clipped_source_regions);
		}
		else {
			if (mask_pixmap_priv && mask_pixmap_priv->type == GLAMOR_TEXTURE_LARGE) {
				RegionTranslate(clipped_dest_regions[i].region,
						x_mask - x_dest,
						y_mask - y_dest);
				clipped_mask_regions = glamor_compute_clipped_regions(mask_pixmap_priv,
							        clipped_dest_regions[i].region,
							        &n_mask_regions, mask_repeat_type);
				for(k = 0; k < n_mask_regions; k++)
				{
					DEBUGF("mask region %d  idx %d\n", k, clipped_mask_regions[k].block_idx);
					DEBUGRegionPrint(clipped_mask_regions[k].region);
					SET_PIXMAP_FBO_CURRENT(mask_pixmap_priv,
							       clipped_mask_regions[k].block_idx);
					RegionTranslate(clipped_mask_regions[k].region,
							x_dest - x_mask + dest->pDrawable->x,
							y_dest - y_mask + dest->pDrawable->y);
					COMPOSITE_REGION(clipped_mask_regions[k].region);
					RegionDestroy(clipped_mask_regions[k].region);
				}
				free(clipped_mask_regions);
			}
			else {
				RegionTranslate(clipped_dest_regions[i].region,
						dest->pDrawable->x,
						dest->pDrawable->y);
				COMPOSITE_REGION(clipped_dest_regions[i].region);
			}
		}
		RegionDestroy(clipped_dest_regions[i].region);
	}
	free(clipped_dest_regions);
	ok = TRUE;
	return ok;
}
