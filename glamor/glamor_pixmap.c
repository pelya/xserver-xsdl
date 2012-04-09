#include <stdlib.h>

#include "glamor_priv.h"
/**
 * Sets the offsets to add to coordinates to make them address the same bits in
 * the backing drawable. These coordinates are nonzero only for redirected
 * windows.
 */
void
glamor_get_drawable_deltas(DrawablePtr drawable, PixmapPtr pixmap,
			   int *x, int *y)
{
#ifdef COMPOSITE
	if (drawable->type == DRAWABLE_WINDOW) {
		*x = -pixmap->screen_x;
		*y = -pixmap->screen_y;
		return;
	}
#endif

	*x = 0;
	*y = 0;
}


static void
_glamor_pixmap_validate_filling(glamor_screen_private * glamor_priv,
				glamor_pixmap_private * pixmap_priv)
{
	glamor_gl_dispatch *dispatch = glamor_get_dispatch(glamor_priv);
	GLfloat vertices[8];
	dispatch->glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_FLOAT,
					GL_FALSE, 2 * sizeof(float),
					vertices);
	dispatch->glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
	dispatch->glUseProgram(glamor_priv->solid_prog);
	dispatch->glUniform4fv(glamor_priv->solid_color_uniform_location,
			       1, pixmap_priv->pending_op.fill.color4fv);
	vertices[0] = -1;
	vertices[1] = -1;
	vertices[2] = 1;
	vertices[3] = -1;
	vertices[4] = 1;
	vertices[5] = 1;
	vertices[6] = -1;
	vertices[7] = 1;
	dispatch->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
	dispatch->glUseProgram(0);
	pixmap_priv->pending_op.type = GLAMOR_PENDING_NONE;
	glamor_put_dispatch(glamor_priv);
}


glamor_pixmap_validate_function_t pixmap_validate_funcs[] = {
	NULL,
	_glamor_pixmap_validate_filling
};

void
glamor_pixmap_init(ScreenPtr screen)
{
	glamor_screen_private *glamor_priv;

	glamor_priv = glamor_get_screen_private(screen);
	glamor_priv->pixmap_validate_funcs = pixmap_validate_funcs;
}

void
glamor_pixmap_fini(ScreenPtr screen)
{
}

void
glamor_validate_pixmap(PixmapPtr pixmap)
{
	glamor_pixmap_validate_function_t validate_op;
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(pixmap->drawable.pScreen);
	glamor_pixmap_private *pixmap_priv =
	    glamor_get_pixmap_private(pixmap);

	validate_op =
	    glamor_priv->pixmap_validate_funcs[pixmap_priv->
					       pending_op.type];
	if (validate_op) {
		(*validate_op) (glamor_priv, pixmap_priv);
	}
}

void
glamor_set_destination_pixmap_fbo(glamor_pixmap_fbo * fbo)
{
	glamor_gl_dispatch *dispatch = glamor_get_dispatch(fbo->glamor_priv);
	dispatch->glBindFramebuffer(GL_FRAMEBUFFER, fbo->fb);
#ifndef GLAMOR_GLES2
	dispatch->glMatrixMode(GL_PROJECTION);
	dispatch->glLoadIdentity();
	dispatch->glMatrixMode(GL_MODELVIEW);
	dispatch->glLoadIdentity();
#endif
	dispatch->glViewport(0, 0,
			     fbo->width,
			     fbo->height);

	glamor_put_dispatch(fbo->glamor_priv);
}

void
glamor_set_destination_pixmap_priv_nc(glamor_pixmap_private * pixmap_priv)
{
	glamor_set_destination_pixmap_fbo(pixmap_priv->fbo);
}

int
glamor_set_destination_pixmap_priv(glamor_pixmap_private * pixmap_priv)
{
	if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
		return -1;

	glamor_set_destination_pixmap_priv_nc(pixmap_priv);
	return 0;
}

int
glamor_set_destination_pixmap(PixmapPtr pixmap)
{
	int err;
	glamor_pixmap_private *pixmap_priv =
	    glamor_get_pixmap_private(pixmap);

	err = glamor_set_destination_pixmap_priv(pixmap_priv);
	return err;
}

Bool
glamor_set_planemask(PixmapPtr pixmap, unsigned long planemask)
{
	if (glamor_pm_is_solid(&pixmap->drawable, planemask)) {
		return GL_TRUE;
	}

	glamor_fallback("unsupported planemask %lx\n", planemask);
	return GL_FALSE;
}

Bool
glamor_set_alu(struct glamor_gl_dispatch *dispatch, unsigned char alu)
{
#ifndef GLAMOR_GLES2
	if (alu == GXcopy) {
		dispatch->glDisable(GL_COLOR_LOGIC_OP);
		return TRUE;
	}
	dispatch->glEnable(GL_COLOR_LOGIC_OP);
	switch (alu) {
	case GXclear:
		dispatch->glLogicOp(GL_CLEAR);
		break;
	case GXand:
		dispatch->glLogicOp(GL_AND);
		break;
	case GXandReverse:
		dispatch->glLogicOp(GL_AND_REVERSE);
		break;
	case GXandInverted:
		dispatch->glLogicOp(GL_AND_INVERTED);
		break;
	case GXnoop:
		dispatch->glLogicOp(GL_NOOP);
		break;
	case GXxor:
		dispatch->glLogicOp(GL_XOR);
		break;
	case GXor:
		dispatch->glLogicOp(GL_OR);
		break;
	case GXnor:
		dispatch->glLogicOp(GL_NOR);
		break;
	case GXequiv:
		dispatch->glLogicOp(GL_EQUIV);
		break;
	case GXinvert:
		dispatch->glLogicOp(GL_INVERT);
		break;
	case GXorReverse:
		dispatch->glLogicOp(GL_OR_REVERSE);
		break;
	case GXcopyInverted:
		dispatch->glLogicOp(GL_COPY_INVERTED);
		break;
	case GXorInverted:
		dispatch->glLogicOp(GL_OR_INVERTED);
		break;
	case GXnand:
		dispatch->glLogicOp(GL_NAND);
		break;
	case GXset:
		dispatch->glLogicOp(GL_SET);
		break;
	default:
		glamor_fallback("unsupported alu %x\n", alu);
		return FALSE;
	}
#else
	if (alu != GXcopy)
		return FALSE;
#endif
	return TRUE;
}

void *
_glamor_color_convert_a1_a8(void *src_bits, void *dst_bits, int w, int h, int stride, int revert)
{
	void *bits;
	PictFormatShort dst_format, src_format;
	pixman_image_t *dst_image;
	pixman_image_t *src_image;

	if (revert == REVERT_UPLOADING_A1) {
		src_format = PICT_a1;
		dst_format = PICT_a8;
	} else {
		dst_format = PICT_a1;
		src_format = PICT_a8;
	}

	dst_image = pixman_image_create_bits(dst_format,
					     w, h,
					     dst_bits,
					     stride);
	if (dst_image == NULL) {
		free(bits);
		return NULL;
	}

	src_image = pixman_image_create_bits(src_format,
					     w, h,
					     src_bits,
					     stride);

	if (src_image == NULL) {
		pixman_image_unref(dst_image);
		free(bits);
		return NULL;
	}

	pixman_image_composite(PictOpSrc, src_image, NULL, dst_image,
			       0, 0, 0, 0, 0, 0,
			       w,h);

	pixman_image_unref(src_image);
	pixman_image_unref(dst_image);
	return dst_bits;
}

#define ADJUST_BITS(d, src_bits, dst_bits)	(((dst_bits) == (src_bits)) ? (d) : 				\
							(((dst_bits) > (src_bits)) ? 				\
							  (((d) << ((dst_bits) - (src_bits))) 			\
								   + (( 1 << ((dst_bits) - (src_bits))) >> 1))	\
								:  ((d) >> ((src_bits) - (dst_bits)))))

#define GLAMOR_DO_CONVERT(src, dst, no_alpha, swap,		\
			  a_shift_src, a_bits_src,		\
			  b_shift_src, b_bits_src,		\
			  g_shift_src, g_bits_src,		\
			  r_shift_src, r_bits_src,		\
			  a_shift, a_bits,			\
			  b_shift, b_bits,			\
			  g_shift, g_bits,			\
			  r_shift, r_bits)			\
	{								\
		typeof(src) a,b,g,r;					\
		typeof(src) a_mask_src, b_mask_src, g_mask_src, r_mask_src;\
		a_mask_src = (((1 << (a_bits_src)) - 1) << a_shift_src);\
		b_mask_src = (((1 << (b_bits_src)) - 1) << b_shift_src);\
		g_mask_src = (((1 << (g_bits_src)) - 1) << g_shift_src);\
		r_mask_src = (((1 << (r_bits_src)) - 1) << r_shift_src);\
		if (no_alpha)						\
			a = (a_mask_src) >> (a_shift_src);			\
		else							\
			a = ((src) & (a_mask_src)) >> (a_shift_src);	\
		b = ((src) & (b_mask_src)) >> (b_shift_src);		\
		g = ((src) & (g_mask_src)) >> (g_shift_src);		\
		r = ((src) & (r_mask_src)) >> (r_shift_src);		\
		a = ADJUST_BITS(a, a_bits_src, a_bits);			\
		b = ADJUST_BITS(b, b_bits_src, b_bits);			\
		g = ADJUST_BITS(g, g_bits_src, g_bits);			\
		r = ADJUST_BITS(r, r_bits_src, r_bits);			\
		if (swap == 0)						\
			(*dst) = ((a) << (a_shift)) | ((b) << (b_shift)) | ((g) << (g_shift)) | ((r) << (r_shift)); \
		else 												    \
			(*dst) = ((a) << (a_shift)) | ((r) << (b_shift)) | ((g) << (g_shift)) | ((b) << (r_shift)); \
	}

void *
_glamor_color_revert_x2b10g10r10(void *src_bits, void *dst_bits, int w, int h, int stride, int no_alpha, int revert, int swap_rb)
{
	int x,y;
	unsigned int *words, *saved_words, *source_words;
	int swap = !(swap_rb == SWAP_NONE_DOWNLOADING || swap_rb == SWAP_NONE_UPLOADING);

	source_words = src_bits;
	words = dst_bits;
	saved_words = words;

	for (y = 0; y < h; y++)
	{
		DEBUGF("Line %d :  ", y);
		for (x = 0; x < w; x++)
		{
			unsigned int pixel = source_words[x];

			if (revert == REVERT_DOWNLOADING_2_10_10_10)
				GLAMOR_DO_CONVERT(pixel, &words[x], no_alpha, swap,
						  24, 8, 16, 8, 8, 8, 0, 8,
						  30, 2, 20, 10, 10, 10, 0, 10)
			else
				GLAMOR_DO_CONVERT(pixel, &words[x], no_alpha, swap,
						  30, 2, 20, 10, 10, 10, 0, 10,
						  24, 8, 16, 8, 8, 8, 0, 8);
			DEBUGF("%x:%x ", pixel, words[x]);
		}
		DEBUGF("\n");
		words += stride / sizeof(*words);
		source_words += stride / sizeof(*words);
	}
	DEBUGF("\n");
	return saved_words;

}

void *
_glamor_color_revert_x1b5g5r5(void *src_bits, void *dst_bits, int w, int h, int stride, int no_alpha, int revert, int swap_rb)
{
	int x,y;
	unsigned short *words, *saved_words, *source_words;
	int swap = !(swap_rb == SWAP_NONE_DOWNLOADING || swap_rb == SWAP_NONE_UPLOADING);

	words = dst_bits;
	source_words = src_bits;
	saved_words = words;

	for (y = 0; y < h; y++)
	{
		DEBUGF("Line %d :  ", y);
		for (x = 0; x < w; x++)
		{
			unsigned short pixel = source_words[x];

			if (revert == REVERT_DOWNLOADING_1_5_5_5)
				GLAMOR_DO_CONVERT(pixel, &words[x], no_alpha, swap,
						  0, 1, 1, 5, 6, 5, 11, 5,
						  15, 1, 10, 5, 5, 5, 0, 5)
			else
				GLAMOR_DO_CONVERT(pixel, &words[x], no_alpha, swap,
						  15, 1, 10, 5, 5, 5, 0, 5,
						  0, 1, 1, 5, 6, 5, 11, 5);
			DEBUGF("%04x:%04x ", pixel, words[x]);
		}
		DEBUGF("\n");
		words += stride / sizeof(*words);
		source_words += stride / sizeof(*words);
	}
	DEBUGF("\n");
	return saved_words;
}

/*
 * This function is to convert an unsupported color format to/from a
 * supported GL format.
 * Here are the current scenarios:
 *
 * @no_alpha:
 * 	If it is set, then we need to wire the alpha value to 1.
 * @revert:
	REVERT_DOWNLOADING_A1		: convert an Alpha8 buffer to a A1 buffer.
	REVERT_UPLOADING_A1		: convert an A1 buffer to an Alpha8 buffer
	REVERT_DOWNLOADING_2_10_10_10 	: convert r10G10b10X2 to X2B10G10R10
	REVERT_UPLOADING_2_10_10_10 	: convert X2B10G10R10 to R10G10B10X2
	REVERT_DOWNLOADING_1_5_5_5  	: convert B5G5R5X1 to X1R5G5B5
	REVERT_UPLOADING_1_5_5_5    	: convert X1R5G5B5 to B5G5R5X1
   @swap_rb: if we have the swap_rb set, then we need to swap the R and B's position.
 *
 */

void *
glamor_color_convert_to_bits(void *src_bits, void *dst_bits, int w, int h, int stride, int no_alpha, int revert, int swap_rb)
{
	if (revert == REVERT_DOWNLOADING_A1 || revert == REVERT_UPLOADING_A1) {
		return _glamor_color_convert_a1_a8(src_bits, dst_bits, w, h, stride, revert);
	} else if (revert == REVERT_DOWNLOADING_2_10_10_10 || revert == REVERT_UPLOADING_2_10_10_10) {
		return _glamor_color_revert_x2b10g10r10(src_bits, dst_bits, w, h, stride, no_alpha, revert, swap_rb);
	} else if (revert == REVERT_DOWNLOADING_1_5_5_5 || revert == REVERT_UPLOADING_1_5_5_5) {
		return _glamor_color_revert_x1b5g5r5(src_bits, dst_bits, w, h, stride, no_alpha, revert, swap_rb);
	} else
		ErrorF("convert a non-supported mode %x.\n", revert);

	return NULL;
}

/**
 * Upload pixmap to a specified texture.
 * This texture may not be the one attached to it.
 **/
int in_restore = 0;
static void
__glamor_upload_pixmap_to_texture(PixmapPtr pixmap, GLenum format,
				  GLenum type, GLuint tex, int sub,
				  void *bits)
{
	glamor_pixmap_private *pixmap_priv =
	    glamor_get_pixmap_private(pixmap);
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(pixmap->drawable.pScreen);
	glamor_gl_dispatch *dispatch;
	unsigned int stride, row_length;
	GLenum iformat;

	if (glamor_priv->gl_flavor == GLAMOR_GL_ES2)
		iformat = format;
	else
		gl_iformat_for_depth(pixmap->drawable.depth, &iformat);

	stride = pixmap->devKind;
	row_length = (stride * 8) / pixmap->drawable.bitsPerPixel;

	dispatch = glamor_get_dispatch(glamor_priv);
	dispatch->glBindTexture(GL_TEXTURE_2D, tex);
	dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				  GL_NEAREST);
	dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
				  GL_NEAREST);

	if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
		dispatch->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		dispatch->glPixelStorei(GL_UNPACK_ROW_LENGTH, row_length);
	} else {
		dispatch->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}

	if (bits == NULL)
		dispatch->glBindBuffer(GL_PIXEL_UNPACK_BUFFER,
				       pixmap_priv->fbo->pbo);

	if (sub)
		dispatch->glTexSubImage2D(GL_TEXTURE_2D,
				       0,0,0,
				       pixmap->drawable.width,
				       pixmap->drawable.height, format, type,
				       bits);
	else
		dispatch->glTexImage2D(GL_TEXTURE_2D,
				       0,
				       iformat,
				       pixmap->drawable.width,
				       pixmap->drawable.height, 0, format, type,
				       bits);

	if (bits == NULL)
		dispatch->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glamor_put_dispatch(glamor_priv);
}

Bool
glamor_upload_bits_to_pixmap_texture(PixmapPtr pixmap, GLenum format, GLenum type,
				     int no_alpha, int revert, int swap_rb, void *bits)
{
	glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
	glamor_screen_private *glamor_priv = glamor_get_screen_private(pixmap->drawable.pScreen);
	glamor_gl_dispatch *dispatch;
	static float vertices[8];
	static float texcoords[8] = { 0, 1,
		1, 1,
		1, 0,
		0, 0
	};
	static float texcoords_inv[8] = { 0, 0,
		1, 0,
		1, 1,
		0, 1
	};
	float *ptexcoords;
	float dst_xscale, dst_yscale;
	GLuint tex;
	int need_flip;
	int need_free_bits = 0;

	need_flip = !glamor_priv->yInverted;

	if (bits == NULL)
		goto ready_to_upload;

	if (glamor_priv->gl_flavor == GLAMOR_GL_ES2
	    &&  revert > REVERT_NORMAL) {
		/* XXX if we are restoring the pixmap, then we may not need to allocate
		 * new buffer */
		void *converted_bits = malloc(pixmap->drawable.height * pixmap->devKind);
		if (converted_bits == NULL)
			return FALSE;
		bits = glamor_color_convert_to_bits(bits, converted_bits, pixmap->drawable.width,
						    pixmap->drawable.height,
						    pixmap->devKind,
						    no_alpha, revert, swap_rb);
		if (bits == NULL) {
			ErrorF("Failed to convert pixmap no_alpha %d, revert mode %d, swap mode %d\n", swap_rb);
			return FALSE;
		}
		no_alpha = 0;
		revert = REVERT_NONE;
		swap_rb = SWAP_NONE_UPLOADING;
		need_free_bits = TRUE;
	}

ready_to_upload:
	/* Try fast path firstly, upload the pixmap to the texture attached
	 * to the fbo directly. */
	if (no_alpha == 0
	    && revert == REVERT_NONE
	    && swap_rb == SWAP_NONE_UPLOADING
	    && !need_flip) {
		__glamor_upload_pixmap_to_texture(pixmap, format, type,
						  pixmap_priv->fbo->tex, 1,
						  bits);
		return TRUE;
	}

	if (need_flip)
		ptexcoords = texcoords;
	else
		ptexcoords = texcoords_inv;

	pixmap_priv_get_scale(pixmap_priv, &dst_xscale, &dst_yscale);
	glamor_set_normalize_vcoords(dst_xscale,
				     dst_yscale,
				     0, 0,
				     pixmap->drawable.width, pixmap->drawable.height,
				     glamor_priv->yInverted,
				     vertices);

	/* Slow path, we need to flip y or wire alpha to 1. */
	dispatch = glamor_get_dispatch(glamor_priv);
	dispatch->glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_FLOAT,
					GL_FALSE, 2 * sizeof(float),
					vertices);
	dispatch->glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
	dispatch->glVertexAttribPointer(GLAMOR_VERTEX_SOURCE, 2, GL_FLOAT,
					GL_FALSE, 2 * sizeof(float),
					ptexcoords);
	dispatch->glEnableVertexAttribArray(GLAMOR_VERTEX_SOURCE);

	glamor_set_destination_pixmap_priv_nc(pixmap_priv);
	dispatch->glGenTextures(1, &tex);

	__glamor_upload_pixmap_to_texture(pixmap, format, type, tex, 0, bits);
	dispatch->glActiveTexture(GL_TEXTURE0);
	dispatch->glBindTexture(GL_TEXTURE_2D, tex);

	dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				  GL_NEAREST);
	dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
				  GL_NEAREST);
#ifndef GLAMOR_GLES2
	dispatch->glEnable(GL_TEXTURE_2D);
#endif
	dispatch->glUseProgram(glamor_priv->finish_access_prog[no_alpha]);
	dispatch->glUniform1i(glamor_priv->
			      finish_access_revert[no_alpha],
			      revert);
	dispatch->glUniform1i(glamor_priv->finish_access_swap_rb[no_alpha],
			      swap_rb);

	dispatch->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

#ifndef GLAMOR_GLES2
	dispatch->glDisable(GL_TEXTURE_2D);
#endif
	dispatch->glUseProgram(0);
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
	dispatch->glDeleteTextures(1, &tex);
	dispatch->glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glamor_put_dispatch(glamor_priv);

	if (need_free_bits)
		free(bits);
	return TRUE;
}

/*
 * Load texture from the pixmap's data pointer and then
 * draw the texture to the fbo, and flip the y axis.
 * */

static Bool
_glamor_upload_pixmap_to_texture(PixmapPtr pixmap, GLenum format,
				 GLenum type, int no_alpha, int revert,
				 int swap_rb)
{
	glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
	glamor_screen_private *glamor_priv = glamor_get_screen_private(pixmap->drawable.pScreen);
	void *bits;
	int need_free_bits = 0;

	if (!pixmap_priv)
		return TRUE;

	glamor_debug_output(GLAMOR_DEBUG_TEXTURE_DYNAMIC_UPLOAD,
			    "Uploading pixmap %p  %dx%d depth%d.\n",
			    pixmap,
			    pixmap->drawable.width,
			    pixmap->drawable.height,
			    pixmap->drawable.depth);

	if (pixmap_priv->fbo->pbo && pixmap_priv->fbo->pbo_valid)
		bits = NULL;

	return glamor_upload_bits_to_pixmap_texture(pixmap, format, type, no_alpha,
						    revert, swap_rb,
						    pixmap->devPrivate.ptr);
}

void
glamor_pixmap_ensure_fb(glamor_pixmap_fbo *fbo)
{
	glamor_gl_dispatch *dispatch;
	int status;

	dispatch = glamor_get_dispatch(fbo->glamor_priv);

	if (fbo->fb == 0)
		dispatch->glGenFramebuffers(1, &fbo->fb);
	assert(fbo->tex != 0);
	dispatch->glBindFramebuffer(GL_FRAMEBUFFER, fbo->fb);
	dispatch->glFramebufferTexture2D(GL_FRAMEBUFFER,
					 GL_COLOR_ATTACHMENT0,
					 GL_TEXTURE_2D, fbo->tex,
					 0);
	status = dispatch->glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		const char *str;
		switch (status) {
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			str = "incomplete attachment";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			str = "incomplete/missing attachment";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			str = "incomplete draw buffer";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			str = "incomplete read buffer";
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			str = "unsupported";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			str = "incomplete multiple";
			break;
		default:
			str = "unknown error";
			break;
		}

		FatalError("destination is framebuffer incomplete: %s [%#x]\n",
			   str, status);
	}
	glamor_put_dispatch(fbo->glamor_priv);
}

/*
 * Prepare to upload a pixmap to texture memory.
 * no_alpha equals 1 means the format needs to wire alpha to 1.
 * Two condtion need to setup a fbo for a pixmap
 * 1. !yInverted, we need to do flip if we are not yInverted.
 * 2. no_alpha != 0, we need to wire the alpha.
 * */
static int
glamor_pixmap_upload_prepare(PixmapPtr pixmap, GLenum format, int no_alpha, int revert, int swap_rb)
{
	int flag;
	glamor_pixmap_private *pixmap_priv;
	glamor_screen_private *glamor_priv;
	glamor_pixmap_fbo *fbo;
	GLenum iformat;

	pixmap_priv = glamor_get_pixmap_private(pixmap);
	glamor_priv = glamor_get_screen_private(pixmap->drawable.pScreen);

	if (!(no_alpha
	      || (revert != REVERT_NONE)
	      || (swap_rb != SWAP_NONE_UPLOADING)
	      || !glamor_priv->yInverted)) {
		/* We don't need a fbo, a simple texture uploading should work. */

		if (pixmap_priv && pixmap_priv->fbo)
			return 0;
		flag = GLAMOR_CREATE_FBO_NO_FBO;
	} else {

		if (GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
			return 0;
		flag = 0;
	}

	if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP)
		gl_iformat_for_depth(pixmap->drawable.depth, &iformat);
	else
		iformat = format;


	fbo = glamor_create_fbo(glamor_priv, pixmap->drawable.width,
				pixmap->drawable.height,
				iformat,
				flag);
	if (fbo == NULL) {
		glamor_fallback
		    ("upload failed, depth %d x %d @depth %d \n",
		     pixmap->drawable.width, pixmap->drawable.height,
		     pixmap->drawable.depth);
		return -1;
	}

	glamor_pixmap_attach_fbo(pixmap, fbo);

	return 0;
}

enum glamor_pixmap_status
glamor_upload_pixmap_to_texture(PixmapPtr pixmap)
{
	GLenum format, type;
	int no_alpha, revert, swap_rb;

	if (glamor_get_tex_format_type_from_pixmap(pixmap,
						   &format,
						   &type,
						   &no_alpha,
						   &revert,
						   &swap_rb, 1)) {
		glamor_fallback("Unknown pixmap depth %d.\n",
				pixmap->drawable.depth);
		return GLAMOR_UPLOAD_FAILED;
	}

	if (glamor_pixmap_upload_prepare(pixmap, format, no_alpha, revert, swap_rb))
		return GLAMOR_UPLOAD_FAILED;

	if (_glamor_upload_pixmap_to_texture(pixmap, format, type, no_alpha,
					     revert, swap_rb))
		return GLAMOR_UPLOAD_DONE;
	else
		return GLAMOR_UPLOAD_FAILED;

	return GLAMOR_UPLOAD_DONE;
}

void
glamor_restore_pixmap_to_texture(PixmapPtr pixmap)
{
	GLenum format, type;
	int no_alpha, revert, swap_rb;

	if (glamor_get_tex_format_type_from_pixmap(pixmap,
						   &format,
						   &type, &no_alpha,
						   &revert, &swap_rb, 1)) {
		ErrorF("Unknown pixmap depth %d.\n",
		       pixmap->drawable.depth);
		assert(0);
	}

	if (!_glamor_upload_pixmap_to_texture(pixmap, format, type, no_alpha,
					      revert, swap_rb))
		LogMessage(X_WARNING, "Failed to restore pixmap to texture.\n",
			   pixmap->drawable.pScreen->myNum);
}

/*
 * as gles2 only support a very small set of color format and
 * type when do glReadPixel,
 * Before we use glReadPixels to get back a textured pixmap,
 * Use shader to convert it to a supported format and thus
 * get a new temporary pixmap returned.
 * */

glamor_pixmap_fbo *
glamor_es2_pixmap_read_prepare(PixmapPtr source, GLenum format,
			       GLenum type, int no_alpha, int revert, int swap_rb)

{
	glamor_pixmap_private *source_priv;
	glamor_screen_private *glamor_priv;
	ScreenPtr screen;
	glamor_pixmap_fbo *temp_fbo;
	glamor_gl_dispatch *dispatch;
	float temp_xscale, temp_yscale, source_xscale, source_yscale;
	static float vertices[8];
	static float texcoords[8];

	screen = source->drawable.pScreen;

	glamor_priv = glamor_get_screen_private(screen);
	source_priv = glamor_get_pixmap_private(source);
	temp_fbo = glamor_create_fbo(glamor_priv,
				     source->drawable.width,
				     source->drawable.height,
				     format,
				     0);
	if (temp_fbo == NULL)
		return NULL;

	dispatch = glamor_get_dispatch(glamor_priv);
	temp_xscale = 1.0 / temp_fbo->width;
	temp_yscale = 1.0 / temp_fbo->height;

	glamor_set_normalize_vcoords(temp_xscale,
				     temp_yscale,
				     0, 0,
				     source->drawable.width, source->drawable.height,
				     glamor_priv->yInverted,
				     vertices);

	dispatch->glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_FLOAT,
					GL_FALSE, 2 * sizeof(float),
					vertices);
	dispatch->glEnableVertexAttribArray(GLAMOR_VERTEX_POS);

	pixmap_priv_get_scale(source_priv, &source_xscale, &source_yscale);
	glamor_set_normalize_tcoords(source_xscale,
				     source_yscale,
				     0, 0,
				     source->drawable.width, source->drawable.height,
				     glamor_priv->yInverted,
				     texcoords);

	dispatch->glVertexAttribPointer(GLAMOR_VERTEX_SOURCE, 2, GL_FLOAT,
					GL_FALSE, 2 * sizeof(float),
					texcoords);
	dispatch->glEnableVertexAttribArray(GLAMOR_VERTEX_SOURCE);

	dispatch->glActiveTexture(GL_TEXTURE0);
	dispatch->glBindTexture(GL_TEXTURE_2D, source_priv->fbo->tex);
	dispatch->glTexParameteri(GL_TEXTURE_2D,
				  GL_TEXTURE_MIN_FILTER,
				  GL_NEAREST);
	dispatch->glTexParameteri(GL_TEXTURE_2D,
				  GL_TEXTURE_MAG_FILTER,
				  GL_NEAREST);

	glamor_set_destination_pixmap_fbo(temp_fbo);
	dispatch->glUseProgram(glamor_priv->finish_access_prog[no_alpha]);
	dispatch->glUniform1i(glamor_priv->
			      finish_access_revert[no_alpha],
			      revert);
	dispatch->glUniform1i(glamor_priv->finish_access_swap_rb[no_alpha],
			      swap_rb);

	dispatch->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
	dispatch->glUseProgram(0);
	glamor_put_dispatch(glamor_priv);
	return temp_fbo;
}

/**
 * Move a pixmap to CPU memory.
 * The input data is the pixmap's fbo.
 * The output data is at pixmap->devPrivate.ptr. We always use pbo
 * to read the fbo and then map it to va. If possible, we will use
 * it directly as devPrivate.ptr.
 * If successfully download a fbo to cpu then return TRUE.
 * Otherwise return FALSE.
 **/
Bool
glamor_download_pixmap_to_cpu(PixmapPtr pixmap, glamor_access_t access)
{
	glamor_pixmap_private *pixmap_priv =
	    glamor_get_pixmap_private(pixmap);
	unsigned int stride, row_length, y;
	GLenum format, type, gl_access, gl_usage;
	int no_alpha, revert, swap_rb;
	uint8_t *data = NULL, *read;
	ScreenPtr screen;
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(pixmap->drawable.pScreen);
	glamor_gl_dispatch *dispatch;
	glamor_pixmap_fbo *temp_fbo = NULL;
	int need_post_conversion = 0;

	screen = pixmap->drawable.pScreen;
	if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
		return TRUE;
	if (glamor_get_tex_format_type_from_pixmap(pixmap,
						   &format,
						   &type,
						   &no_alpha,
						   &revert,
						   &swap_rb, 0)) {
		ErrorF("Unknown pixmap depth %d.\n",
		       pixmap->drawable.depth);
		assert(0);	// Should never happen.
		return FALSE;
	}

	glamor_debug_output(GLAMOR_DEBUG_TEXTURE_DOWNLOAD,
			    "Downloading pixmap %p  %dx%d depth%d\n",
			    pixmap,
			    pixmap->drawable.width,
			    pixmap->drawable.height,
			    pixmap->drawable.depth);

	stride = pixmap->devKind;

	glamor_set_destination_pixmap_priv_nc(pixmap_priv);
	/* XXX we may don't need to validate it on GPU here,
	 * we can just validate it on CPU. */
	glamor_validate_pixmap(pixmap);


	need_post_conversion = (revert > REVERT_NORMAL);

	if (glamor_priv->gl_flavor == GLAMOR_GL_ES2
	    && !need_post_conversion
	    && (swap_rb != SWAP_NONE_DOWNLOADING || revert != REVERT_NONE)) {
		 if (!(temp_fbo = glamor_es2_pixmap_read_prepare(pixmap, format,
								 type, no_alpha,
								 revert, swap_rb)))
			return FALSE;
	}
	switch (access) {
	case GLAMOR_ACCESS_RO:
		gl_access = GL_READ_ONLY;
		gl_usage = GL_STREAM_READ;
		break;
	case GLAMOR_ACCESS_WO:
		data = malloc(stride * pixmap->drawable.height);
		goto done;
		break;
	case GLAMOR_ACCESS_RW:
		gl_access = GL_READ_WRITE;
		gl_usage = GL_DYNAMIC_DRAW;
		break;
	default:
		ErrorF("Glamor: Invalid access code. %d\n", access);
		assert(0);
	}
	if (glamor_priv->gl_flavor == GLAMOR_GL_ES2) {
		data = malloc(stride * pixmap->drawable.height);
	}
	row_length = (stride * 8) / pixmap->drawable.bitsPerPixel;

	dispatch = glamor_get_dispatch(glamor_priv);
	if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
		dispatch->glPixelStorei(GL_PACK_ALIGNMENT, 1);
		dispatch->glPixelStorei(GL_PACK_ROW_LENGTH, row_length);
	} else {
		dispatch->glPixelStorei(GL_PACK_ALIGNMENT, 4);
		//  dispatch->glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	}
	if (glamor_priv->has_pack_invert || glamor_priv->yInverted) {

		if (!glamor_priv->yInverted) {
			assert(glamor_priv->gl_flavor ==
			       GLAMOR_GL_DESKTOP);
			dispatch->glPixelStorei(GL_PACK_INVERT_MESA, 1);
		}

		if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
			if (pixmap_priv->fbo->pbo == 0)
				dispatch->glGenBuffers(1,
						       &pixmap_priv->fbo->pbo);
			dispatch->glBindBuffer(GL_PIXEL_PACK_BUFFER,
					       pixmap_priv->fbo->pbo);
			dispatch->glBufferData(GL_PIXEL_PACK_BUFFER,
					       stride *
					       pixmap->drawable.height,
					       NULL, gl_usage);
			dispatch->glReadPixels(0, 0, row_length,
					       pixmap->drawable.height,
					       format, type, 0);
			data = dispatch->glMapBuffer(GL_PIXEL_PACK_BUFFER,
						     gl_access);
			pixmap_priv->fbo->pbo_valid = TRUE;

			if (!glamor_priv->yInverted) {
				assert(glamor_priv->gl_flavor ==
				       GLAMOR_GL_DESKTOP);
				dispatch->glPixelStorei
				    (GL_PACK_INVERT_MESA, 0);
			}
			dispatch->glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

		} else {
			dispatch->glReadPixels(0, 0,
					       pixmap->drawable.width,
					       pixmap->drawable.height,
					       format, type, data);
		}
	} else {
		data = malloc(stride * pixmap->drawable.height);
		assert(data);
		if (access != GLAMOR_ACCESS_WO) {
			if (pixmap_priv->fbo->pbo == 0)
				dispatch->glGenBuffers(1,
						       &pixmap_priv->fbo->pbo);
			dispatch->glBindBuffer(GL_PIXEL_PACK_BUFFER,
					       pixmap_priv->fbo->pbo);
			dispatch->glBufferData(GL_PIXEL_PACK_BUFFER,
					       stride *
					       pixmap->drawable.height,
					       NULL, GL_STREAM_READ);
			dispatch->glReadPixels(0, 0, row_length,
					       pixmap->drawable.height,
					       format, type, 0);
			read = dispatch->glMapBuffer(GL_PIXEL_PACK_BUFFER,
						     GL_READ_ONLY);

			for (y = 0; y < pixmap->drawable.height; y++)
				memcpy(data + y * stride,
				       read + (pixmap->drawable.height -
					       y - 1) * stride, stride);
			dispatch->glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
			dispatch->glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
			pixmap_priv->fbo->pbo_valid = FALSE;
			dispatch->glDeleteBuffers(1, &pixmap_priv->fbo->pbo);
			pixmap_priv->fbo->pbo = 0;
		}
	}

	dispatch->glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glamor_put_dispatch(glamor_priv);

	if (need_post_conversion) {
		/* As OpenGL desktop version never enters here.
		 * Don't need to consider if the pbo is valid.*/
		int stride;
		assert(pixmap_priv->fbo->pbo_valid == 0);

		/* Only A1 <--> A8 conversion need to adjust the stride value. */
		if (pixmap->drawable.depth == 1)
			stride = (((pixmap->drawable.width * 8 + 7) / 8) + 3) & ~3;
		else
			stride = pixmap->devKind;
		glamor_color_convert_to_bits(data, data, pixmap->drawable.width,
					     pixmap->drawable.height,
					     stride, no_alpha,
					     revert, swap_rb);
	}

      done:

	pixmap_priv->gl_fbo = GLAMOR_FBO_DOWNLOADED;
	pixmap->devPrivate.ptr = data;

	if (temp_fbo != NULL)
		glamor_destroy_fbo(temp_fbo);

	return TRUE;
}

/* fixup a fbo to the exact size as the pixmap. */
Bool
glamor_fixup_pixmap_priv(ScreenPtr screen, glamor_pixmap_private *pixmap_priv)
{
	glamor_screen_private *glamor_priv;
	glamor_pixmap_fbo *old_fbo;
	glamor_pixmap_fbo *new_fbo = NULL;
	PixmapPtr scratch = NULL;
	glamor_pixmap_private *scratch_priv;
	DrawablePtr drawable;
	GCPtr gc = NULL;
	int ret = FALSE;

	drawable = &pixmap_priv->container->drawable;

	if (pixmap_priv->container->drawable.width == pixmap_priv->fbo->width
	    && pixmap_priv->container->drawable.height == pixmap_priv->fbo->height)
		return	TRUE;

	old_fbo = pixmap_priv->fbo;
	glamor_priv = pixmap_priv->glamor_priv;

	if (!old_fbo)
		return FALSE;

	gc = GetScratchGC(drawable->depth, screen);
	if (!gc)
		goto fail;

	scratch = glamor_create_pixmap(screen, drawable->width, drawable->height,
				       drawable->depth,
				       GLAMOR_CREATE_PIXMAP_FIXUP);

	scratch_priv = glamor_get_pixmap_private(scratch);

	if (!scratch_priv || !scratch_priv->fbo)
		goto fail;

	ValidateGC(&scratch->drawable, gc);
	glamor_copy_area(drawable,
			 &scratch->drawable,
			 gc, 0, 0,
			 drawable->width, drawable->height,
			 0, 0);
	old_fbo = glamor_pixmap_detach_fbo(pixmap_priv);
	new_fbo = glamor_pixmap_detach_fbo(scratch_priv);
	glamor_pixmap_attach_fbo(pixmap_priv->container, new_fbo);
	glamor_pixmap_attach_fbo(scratch, old_fbo);

	DEBUGF("old %dx%d type %d\n",
		drawable->width, drawable->height, pixmap_priv->type);
	DEBUGF("copy tex %d  %dx%d to tex %d %dx%d \n",
		old_fbo->tex, old_fbo->width, old_fbo->height, new_fbo->tex, new_fbo->width, new_fbo->height);
	ret = TRUE;
fail:
	if (gc)
		FreeScratchGC(gc);
	if (scratch)
		glamor_destroy_pixmap(scratch);

	return ret;
}
