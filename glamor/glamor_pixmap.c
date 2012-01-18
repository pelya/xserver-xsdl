#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

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
	glamor_gl_dispatch *dispatch = &glamor_priv->dispatch;
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
}


glamor_pixmap_validate_function_t pixmap_validate_funcs[] = {
	NULL,
	_glamor_pixmap_validate_filling
};

void
glamor_pixmap_init(ScreenPtr screen)
{
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(screen);
	glamor_priv->pixmap_validate_funcs = pixmap_validate_funcs;
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
glamor_set_destination_pixmap_priv_nc(glamor_pixmap_private * pixmap_priv)
{
	glamor_gl_dispatch *dispatch = &pixmap_priv->glamor_priv->dispatch;
	dispatch->glBindFramebuffer(GL_FRAMEBUFFER, pixmap_priv->fbo->fb);
#ifndef GLAMOR_GLES2
	dispatch->glMatrixMode(GL_PROJECTION);
	dispatch->glLoadIdentity();
	dispatch->glMatrixMode(GL_MODELVIEW);
	dispatch->glLoadIdentity();
#endif
	dispatch->glViewport(0, 0,
			     pixmap_priv->fbo->width,
			     pixmap_priv->fbo->height);

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



void
glamor_set_alu(struct glamor_gl_dispatch *dispatch, unsigned char alu)
{
#ifndef GLAMOR_GLES2
	if (alu == GXcopy) {
		dispatch->glDisable(GL_COLOR_LOGIC_OP);
		return;
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
		FatalError("unknown logic op\n");
	}
#else
	if (alu != GXcopy)
		ErrorF("unsupported alu %x \n", alu);
#endif
}




/**
 * Upload pixmap to a specified texture.
 * This texture may not be the one attached to it.
 **/
int in_restore = 0;
static void
__glamor_upload_pixmap_to_texture(PixmapPtr pixmap, GLenum format,
				  GLenum type, GLuint tex)
{
	glamor_pixmap_private *pixmap_priv =
	    glamor_get_pixmap_private(pixmap);
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(pixmap->drawable.pScreen);
	glamor_gl_dispatch *dispatch = &glamor_priv->dispatch;
	unsigned int stride, row_length;
	void *texels;
	GLenum iformat;

	if (glamor_priv->gl_flavor == GLAMOR_GL_ES2)
		iformat = format;
	else
		gl_iformat_for_depth(pixmap->drawable.depth, &iformat);

	stride = pixmap->devKind;
	row_length = (stride * 8) / pixmap->drawable.bitsPerPixel;

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

	if (pixmap_priv->fbo->pbo && pixmap_priv->fbo->pbo_valid) {
		texels = NULL;
		dispatch->glBindBuffer(GL_PIXEL_UNPACK_BUFFER,
				       pixmap_priv->fbo->pbo);
	} else
		texels = pixmap->devPrivate.ptr;

	dispatch->glTexImage2D(GL_TEXTURE_2D,
			       0,
			       iformat,
			       pixmap->drawable.width,
			       pixmap->drawable.height, 0, format, type,
			       texels);
}


/* 
 * Load texture from the pixmap's data pointer and then
 * draw the texture to the fbo, and flip the y axis.
 * */

static void
_glamor_upload_pixmap_to_texture(PixmapPtr pixmap, GLenum format,
				 GLenum type, int no_alpha, int no_revert,
				 int flip)
{

	glamor_pixmap_private *pixmap_priv =
	    glamor_get_pixmap_private(pixmap);
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(pixmap->drawable.pScreen);
	glamor_gl_dispatch *dispatch = &glamor_priv->dispatch;
	static float vertices[8] = { -1, -1,
		1, -1,
		1, 1,
		-1, 1
	};
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

	GLuint tex;
	int need_flip;

	if (!pixmap_priv)
		return;
	need_flip = (flip && !glamor_priv->yInverted);

	glamor_debug_output(GLAMOR_DEBUG_TEXTURE_DYNAMIC_UPLOAD,
			    "Uploading pixmap %p  %dx%d depth%d.\n",
			    pixmap,
			    pixmap->drawable.width,
			    pixmap->drawable.height,
			    pixmap->drawable.depth);

	/* Try fast path firstly, upload the pixmap to the texture attached
	 * to the fbo directly. */
	if (no_alpha == 0 && no_revert == 1 && !need_flip) {
		__glamor_upload_pixmap_to_texture(pixmap, format, type,
						  pixmap_priv->fbo->tex);
		return;
	}


	if (need_flip)
		ptexcoords = texcoords;
	else
		ptexcoords = texcoords_inv;

	/* Slow path, we need to flip y or wire alpha to 1. */
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

	__glamor_upload_pixmap_to_texture(pixmap, format, type, tex);
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
			      finish_access_no_revert[no_alpha],
			      no_revert);
	dispatch->glUniform1i(glamor_priv->finish_access_swap_rb[no_alpha],
			      0);

	dispatch->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

#ifndef GLAMOR_GLES2
	dispatch->glDisable(GL_TEXTURE_2D);
#endif
	dispatch->glUseProgram(0);
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
	dispatch->glDeleteTextures(1, &tex);
	dispatch->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void
glamor_pixmap_ensure_fb(glamor_pixmap_fbo *fbo)
{
	int status;
	glamor_gl_dispatch *dispatch = &fbo->glamor_priv->dispatch;

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

		LogMessageVerb(X_INFO, 0,
			       "destination is framebuffer incomplete: %s [%#x]\n",
			       str, status);
		assert(0);
	}
}

/*  
 * Prepare to upload a pixmap to texture memory.
 * no_alpha equals 1 means the format needs to wire alpha to 1.
 * Two condtion need to setup a fbo for a pixmap
 * 1. !yInverted, we need to do flip if we are not yInverted.
 * 2. no_alpha != 0, we need to wire the alpha.
 * */
static int
glamor_pixmap_upload_prepare(PixmapPtr pixmap, int no_alpha, int no_revert)
{
	int need_fbo;
	glamor_pixmap_private *pixmap_priv;
	glamor_screen_private *glamor_priv;
	GLenum format;
	glamor_pixmap_fbo *fbo;

	pixmap_priv = glamor_get_pixmap_private(pixmap);
	glamor_priv = glamor_get_screen_private(pixmap->drawable.pScreen);

	if (!glamor_check_fbo_size
	    (glamor_priv, pixmap->drawable.width, pixmap->drawable.height)
	    || !glamor_check_fbo_depth(pixmap->drawable.depth)) {
		glamor_fallback
		    ("upload failed reason: bad size or depth %d x %d @depth %d \n",
		     pixmap->drawable.width, pixmap->drawable.height,
		     pixmap->drawable.depth);
		return -1;
	}

	if (GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
		return 0;

	fbo = glamor_create_fbo(glamor_priv, pixmap->drawable.width,
				pixmap->drawable.height,
				pixmap->drawable.depth,
				0);
	if (fbo == NULL)
		return -1;

	glamor_pixmap_attach_fbo(pixmap, fbo);

	return 0;
}

enum glamor_pixmap_status
glamor_upload_pixmap_to_texture(PixmapPtr pixmap)
{
	GLenum format, type;
	int no_alpha, no_revert;

	if (glamor_get_tex_format_type_from_pixmap(pixmap,
						   &format,
						   &type, &no_alpha,
						   &no_revert)) {
		glamor_fallback("Unknown pixmap depth %d.\n",
				pixmap->drawable.depth);
		return GLAMOR_UPLOAD_FAILED;
	}
	if (glamor_pixmap_upload_prepare(pixmap, no_alpha, no_revert))
		return GLAMOR_UPLOAD_FAILED;
	_glamor_upload_pixmap_to_texture(pixmap, format, type, no_alpha,
					 no_revert, 1);
	return GLAMOR_UPLOAD_DONE;
}

#if 0
enum glamor_pixmap_status
glamor_upload_pixmap_to_texure_from_data(PixmapPtr pixmap, void *data)
{
	enum glamor_pixmap_status upload_status;
	glamor_pixmap_private *pixmap_priv =
	    glamor_get_pixmap_private(pixmap);

	assert(pixmap_priv->pbo_valid == 0);
	assert(pixmap->devPrivate.ptr == NULL);
	pixmap->devPrivate.ptr = data;
	upload_status = glamor_upload_pixmap_to_texture(pixmap);
	pixmap->devPrivate.ptr = NULL;
	return upload_status;
}
#endif

void
glamor_restore_pixmap_to_texture(PixmapPtr pixmap)
{
	GLenum format, type;
	int no_alpha, no_revert;

	if (glamor_get_tex_format_type_from_pixmap(pixmap,
						   &format,
						   &type, &no_alpha,
						   &no_revert)) {
		ErrorF("Unknown pixmap depth %d.\n",
		       pixmap->drawable.depth);
		assert(0);
	}

	in_restore = 1;
	_glamor_upload_pixmap_to_texture(pixmap, format, type, no_alpha,
					 no_revert, 1);
	in_restore = 0;
}

/* 
 * as gles2 only support a very small set of color format and
 * type when do glReadPixel,  
 * Before we use glReadPixels to get back a textured pixmap, 
 * Use shader to convert it to a supported format and thus
 * get a new temporary pixmap returned.
 * */

PixmapPtr
glamor_es2_pixmap_read_prepare(PixmapPtr source, GLenum * format,
			       GLenum * type, int no_alpha, int no_revert)
{
	glamor_pixmap_private *source_priv;
	glamor_screen_private *glamor_priv;
	ScreenPtr screen;
	PixmapPtr temp_pixmap;
	glamor_pixmap_private *temp_pixmap_priv;
	glamor_gl_dispatch *dispatch;
	static float vertices[8] = { -1, -1,
		1, -1,
		1, 1,
		-1, 1
	};
	static float texcoords[8] = { 0, 0,
		1, 0,
		1, 1,
		0, 1
	};

	int swap_rb = 0;

	screen = source->drawable.pScreen;

	glamor_priv = glamor_get_screen_private(screen);
	source_priv = glamor_get_pixmap_private(source);
	dispatch = &glamor_priv->dispatch;
	if (*format == GL_BGRA) {
		*format = GL_RGBA;
		swap_rb = 1;
	}


	temp_pixmap = glamor_create_pixmap (screen,
					    source->drawable.width,
					    source->drawable.height,
					    source->drawable.depth, 0);

	temp_pixmap_priv = glamor_get_pixmap_private(temp_pixmap);

	dispatch->glBindTexture(GL_TEXTURE_2D, temp_pixmap_priv->fbo->tex);
	dispatch->glTexParameteri(GL_TEXTURE_2D,
				  GL_TEXTURE_MIN_FILTER,
				  GL_NEAREST);
	dispatch->glTexParameteri(GL_TEXTURE_2D,
				  GL_TEXTURE_MAG_FILTER,
				  GL_NEAREST);

	dispatch->glTexImage2D(GL_TEXTURE_2D, 0, *format,
			       source->drawable.width,
			       source->drawable.height, 0, *format, *type,
			       NULL);

	dispatch->glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_FLOAT,
					GL_FALSE, 2 * sizeof(float),
					vertices);
	dispatch->glEnableVertexAttribArray(GLAMOR_VERTEX_POS);

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

	glamor_set_destination_pixmap_priv_nc(temp_pixmap_priv);

	dispatch->glUseProgram(glamor_priv->finish_access_prog[no_alpha]);
	dispatch->glUniform1i(glamor_priv->
			      finish_access_no_revert[no_alpha],
			      no_revert);
	dispatch->glUniform1i(glamor_priv->finish_access_swap_rb[no_alpha],
			      swap_rb);

	dispatch->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
	dispatch->glUseProgram(0);
	return temp_pixmap;
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
	int no_alpha, no_revert;
	uint8_t *data = NULL, *read;
	PixmapPtr temp_pixmap = NULL;
	ScreenPtr screen;
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(pixmap->drawable.pScreen);
	glamor_gl_dispatch *dispatch = &glamor_priv->dispatch;

	screen = pixmap->drawable.pScreen;
	if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
		return TRUE;
	if (glamor_get_tex_format_type_from_pixmap(pixmap,
						   &format,
						   &type, &no_alpha,
						   &no_revert)) {
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

	if (glamor_priv->gl_flavor == GLAMOR_GL_ES2
	    &&
	    ((format != GL_RGBA && format != GL_RGB && format != GL_ALPHA)
	     || no_revert != 1)) {

		temp_pixmap =
		    glamor_es2_pixmap_read_prepare(pixmap, &format,
						   &type, no_alpha,
						   no_revert);

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
			if (type == GL_UNSIGNED_SHORT_1_5_5_5_REV)
				type = GL_UNSIGNED_SHORT_5_5_5_1;
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
      done:
	pixmap->devPrivate.ptr = data;

	if (temp_pixmap)
		glamor_destroy_pixmap(temp_pixmap);

	return TRUE;
}
