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
_glamor_pixmap_validate_filling(glamor_screen_private *glamor_priv, 
				 glamor_pixmap_private *pixmap_priv)
{
    GLfloat vertices[8];
#if 0
    glVertexPointer(2, GL_FLOAT, sizeof(float) * 2, vertices);
    glEnableClientState(GL_VERTEX_ARRAY);
#else
    glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_FLOAT, GL_FALSE,
                          2 * sizeof(float), vertices);
    glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
#endif
    glUseProgram(glamor_priv->solid_prog);
    glUniform4fv(glamor_priv->solid_color_uniform_location, 
      1, pixmap_priv->pending_op.fill.color4fv);
    vertices[0] = -1;
    vertices[1] = -1;
    vertices[2] = 1;
    vertices[3] = -1;
    vertices[4] = 1;
    vertices[5] = 1;
    vertices[6] = -1;
    vertices[7] = 1;
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
#if 0
    glDisableClientState(GL_VERTEX_ARRAY);
#else
    glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
#endif
    glUseProgram(0);
    pixmap_priv->pending_op.type = GLAMOR_PENDING_NONE;
}


glamor_pixmap_validate_function_t pixmap_validate_funcs[] = {
  NULL,
  _glamor_pixmap_validate_filling
};

void
glamor_pixmap_init(ScreenPtr screen)
{
  glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
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

  validate_op = glamor_priv->pixmap_validate_funcs[pixmap_priv->pending_op.type];
  if (validate_op) { 
    (*validate_op)(glamor_priv, pixmap_priv);
  }
}

void
glamor_set_destination_pixmap_priv_nc(glamor_pixmap_private *pixmap_priv)
{
  glBindFramebuffer(GL_FRAMEBUFFER, pixmap_priv->fb);
#ifndef GLAMOR_GLES2
  glMatrixMode(GL_PROJECTION);                                                                                                                                                                  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);                                                                                                                                                                   glLoadIdentity();                                        
#endif

  glViewport(0, 0,
	     pixmap_priv->container->drawable.width,
	     pixmap_priv->container->drawable.height);

}

int
glamor_set_destination_pixmap_priv(glamor_pixmap_private *pixmap_priv)
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
  glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);

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
glamor_set_alu(unsigned char alu)
{
#ifndef GLAMOR_GLES2
  if (alu == GXcopy) {
    glDisable(GL_COLOR_LOGIC_OP);
    return;
  }
  glEnable(GL_COLOR_LOGIC_OP);
  switch (alu) {
  case GXclear:
    glLogicOp(GL_CLEAR);
    break;
  case GXand:
    glLogicOp(GL_AND);
    break;
  case GXandReverse:
    glLogicOp(GL_AND_REVERSE);
    break;
  case GXandInverted:
    glLogicOp(GL_AND_INVERTED);
    break;
  case GXnoop:
    glLogicOp(GL_NOOP);
    break;
  case GXxor:
    glLogicOp(GL_XOR);
    break;
  case GXor:
    glLogicOp(GL_OR);
    break;
  case GXnor:
    glLogicOp(GL_NOR);
    break;
  case GXequiv:
    glLogicOp(GL_EQUIV);
    break;
  case GXinvert:
    glLogicOp(GL_INVERT);
    break;
  case GXorReverse:
    glLogicOp(GL_OR_REVERSE);
    break;
  case GXcopyInverted:
    glLogicOp(GL_COPY_INVERTED);
    break;
  case GXorInverted:
    glLogicOp(GL_OR_INVERTED);
    break;
  case GXnand:
    glLogicOp(GL_NAND);
    break;
  case GXset:
    glLogicOp(GL_SET);
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

static void 
__glamor_upload_pixmap_to_texture(PixmapPtr pixmap, GLenum format, GLenum type, GLuint tex)
{
  glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
  glamor_screen_private *glamor_priv = glamor_get_screen_private(pixmap->drawable.pScreen);
  unsigned int stride, row_length;
  void *texels;
  GLenum iformat;

  switch (pixmap->drawable.depth) {
#if 0
    case 8:
        iformat = GL_ALPHA;
        break;
#endif
    case 24:
        iformat = GL_RGB;
        break; 
    default:
        iformat = GL_RGBA;
        break;
    }

  if (glamor_priv->gl_flavor == GLAMOR_GL_ES2) {
    iformat = format;
    type = GL_UNSIGNED_BYTE;
  }

  stride = pixmap->devKind;
  row_length = (stride * 8) / pixmap->drawable.bitsPerPixel;

  glBindTexture(GL_TEXTURE_2D, tex);

  if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, row_length);
  } 
  else {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  }

  if (pixmap_priv->pbo && pixmap_priv->pbo_valid) {
    texels = NULL;
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixmap_priv->pbo);
  }
  else
    texels = pixmap->devPrivate.ptr;

  glTexImage2D(GL_TEXTURE_2D, 
	       0, 
	       iformat,
	       pixmap->drawable.width, 
	       pixmap->drawable.height, 0,
	       format, type, texels);
}


/* 
 * Load texture from the pixmap's data pointer and then
 * draw the texture to the fbo, and flip the y axis.
 * */

static void
_glamor_upload_pixmap_to_texture(PixmapPtr pixmap, GLenum format, GLenum type, int ax, int flip)
{

  glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
  glamor_screen_private *glamor_priv =
    glamor_get_screen_private(pixmap->drawable.pScreen);
  static float vertices[8] = {-1, -1,
			      1, -1,
			      1,  1,
			      -1,  1};
  static float texcoords[8] = {0, 1,
			       1, 1,
			       1, 0,
			       0, 0};
  static float texcoords_inv[8] = {0, 0,
				   1, 0,
				   1, 1,
				   0, 1};
  float *ptexcoords;

  GLuint tex;
  int need_flip;
  need_flip = (flip && !glamor_priv->yInverted);

  /* Try fast path firstly, upload the pixmap to the texture attached
   * to the fbo directly. */

  if (ax == 0 && !need_flip) {
    __glamor_upload_pixmap_to_texture(pixmap, format, type, pixmap_priv->tex);
    return;
  }


  if (need_flip)
    ptexcoords = texcoords;
  else
    ptexcoords = texcoords_inv;

  /* Slow path, we need to flip y or wire alpha to 1. */
#if 0
  glVertexPointer(2, GL_FLOAT, sizeof(float) * 2, vertices);
  glEnableClientState(GL_VERTEX_ARRAY);

  glClientActiveTexture(GL_TEXTURE0);
  glTexCoordPointer(2, GL_FLOAT, sizeof(float) * 2, ptexcoords);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#else
  glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_FLOAT, GL_FALSE,
                        2 * sizeof(float),
                        vertices);
  glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
  glVertexAttribPointer(GLAMOR_VERTEX_SOURCE, 2, GL_FLOAT, GL_FALSE,
                        2 * sizeof(float),
                        ptexcoords);
  glEnableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
#endif
  glBindFramebuffer(GL_FRAMEBUFFER, pixmap_priv->fb);
  glViewport(0, 0, pixmap->drawable.width, pixmap->drawable.height);

  glGenTextures(1, &tex);

  __glamor_upload_pixmap_to_texture(pixmap, format, type, tex);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glEnable(GL_TEXTURE_2D);
#if 0
  glUseProgram(glamor_priv->finish_access_prog[ax]);
#else
  glUseProgram(glamor_priv->finish_access_prog[ax + 2]);
#endif

  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  glDisable(GL_TEXTURE_2D);
  glUseProgram(0);
#if 0
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#else
  glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
  glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
#endif
  glDeleteTextures(1, &tex);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void
glamor_pixmap_ensure_fb(PixmapPtr pixmap)
{
  int status;
  glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
  if (pixmap_priv->fb == 0)
    glGenFramebuffers(1, &pixmap_priv->fb);
  assert(pixmap_priv->tex != 0);
  glBindFramebuffer(GL_FRAMEBUFFER, pixmap_priv->fb);
  glFramebufferTexture2D(GL_FRAMEBUFFER,
                         GL_COLOR_ATTACHMENT0,
			 GL_TEXTURE_2D,
			 pixmap_priv->tex,
			 0);
    status = glCheckFramebufferStatus (GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        const char *str;
        switch (status) {
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: str= "incomplete attachment"; break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: str= "incomplete/missing attachment"; break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: str= "incomplete draw buffer"; break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: str= "incomplete read buffer"; break;
        case GL_FRAMEBUFFER_UNSUPPORTED: str= "unsupported"; break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: str= "incomplete multiple"; break;
        default: str = "unknown error"; break;
        }

        LogMessageVerb(X_INFO, 0,
                 "destination is framebuffer incomplete: %s [%#x]\n",
                 str, status);
        assert(0);
    }
}

/*  
 * Prepare to upload a pixmap to texture memory.
 * ax 1 means the format needs to wire alpha to 1.
 * Two condtion need to setup a fbo for a pixmap
 * 1. !yInverted, we need to do flip if we are not yInverted.
 * 2. ax != 0, we need to wire the alpha.
 * */
static int
glamor_pixmap_upload_prepare(PixmapPtr pixmap, int ax)
{
  int need_fbo;
  glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
  glamor_screen_private *glamor_priv = glamor_get_screen_private(pixmap->drawable.pScreen);

  if (!glamor_check_fbo_size(glamor_priv, pixmap->drawable.width , pixmap->drawable.height) 
      || !glamor_check_fbo_depth(pixmap->drawable.depth)) {
    glamor_fallback("upload failed reason: bad size or depth %d x %d @depth %d \n",
		    pixmap->drawable.width, pixmap->drawable.height, pixmap->drawable.depth);
    return -1;
  }

  if (GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv)) 
    return 0; 

  if (ax != 0 || !glamor_priv->yInverted)
    need_fbo = 1;
  else
    need_fbo = 0;

  if (pixmap_priv->tex == 0) 
    glGenTextures(1, &pixmap_priv->tex);

  if (need_fbo) {
    glBindTexture(GL_TEXTURE_2D, pixmap_priv->tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixmap->drawable.width, 
		 pixmap->drawable.height, 0,
		 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glamor_pixmap_ensure_fb(pixmap);
  }
 
  return 0;
}
 
enum glamor_pixmap_status 
glamor_upload_pixmap_to_texture(PixmapPtr pixmap)
{
  GLenum format, type;
  int ax;

  if (glamor_get_tex_format_type_from_pixmap(pixmap, 
					     &format, 
					     &type, 
					     &ax)) {
    glamor_fallback("Unknown pixmap depth %d.\n", pixmap->drawable.depth);
    return GLAMOR_UPLOAD_FAILED;
  }
  if (glamor_pixmap_upload_prepare(pixmap, ax))
    return GLAMOR_UPLOAD_FAILED;
  glamor_debug_output(GLAMOR_DEBUG_TEXTURE_DYNAMIC_UPLOAD,
		      "Uploading pixmap %p  %dx%d depth%d.\n", 
		      pixmap, 
		      pixmap->drawable.width, 
		      pixmap->drawable.height,
		      pixmap->drawable.depth);
  _glamor_upload_pixmap_to_texture(pixmap, format, type, ax, 1);
  return GLAMOR_UPLOAD_DONE;
}

#if 0
enum glamor_pixmap_status 
glamor_upload_pixmap_to_texure_from_data(PixmapPtr pixmap, void *data)
{
  enum glamor_pixmap_status upload_status;
  glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);

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
  int ax;

  if (glamor_get_tex_format_type_from_pixmap(pixmap, 
					     &format, 
					     &type, 
					     &ax)) {
    ErrorF("Unknown pixmap depth %d.\n", pixmap->drawable.depth);
    assert(0);
  }
  _glamor_upload_pixmap_to_texture(pixmap, format, type, ax, 1);
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
  glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
  unsigned int stride, row_length, y;
  GLenum format, type, gl_access, gl_usage;
  int ax;
  uint8_t *data, *read;
  glamor_screen_private *glamor_priv =
    glamor_get_screen_private(pixmap->drawable.pScreen);


  if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
    return TRUE;
  /* XXX we may don't need to validate it on GPU here,
   * we can just validate it on CPU. */
  if (glamor_get_tex_format_type_from_pixmap(pixmap, 
					     &format,
					     &type, 
					     &ax)) {
    ErrorF("Unknown pixmap depth %d.\n", pixmap->drawable.depth);
    assert(0);  // Should never happen.
    return FALSE;
  }

  pixmap_priv->access_mode = access;
  glamor_debug_output(GLAMOR_DEBUG_TEXTURE_DOWNLOAD,
		      "Downloading pixmap %p  %dx%d depth%d\n", 
		      pixmap, 
		      pixmap->drawable.width, 
		      pixmap->drawable.height,
		      pixmap->drawable.depth);
 
  stride = pixmap->devKind;
  glamor_set_destination_pixmap_priv_nc(pixmap_priv);
  glamor_validate_pixmap(pixmap); 
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
  if (glamor_priv->gl_flavor == GLAMOR_GL_ES2)
    data = malloc(stride * pixmap->drawable.height);
  row_length = (stride * 8) / pixmap->drawable.bitsPerPixel;
  if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, row_length);
  }
  else {
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
  //  glPixelStorei(GL_PACK_ROW_LENGTH, 0);
  }

  if (glamor_priv->has_pack_invert || glamor_priv->yInverted) {

    if (!glamor_priv->yInverted) {
      assert(glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP); 
      glPixelStorei(GL_PACK_INVERT_MESA, 1);
    }
    
    if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
    if (pixmap_priv->pbo == 0)
      glGenBuffers (1, &pixmap_priv->pbo);
    glBindBuffer (GL_PIXEL_PACK_BUFFER, pixmap_priv->pbo);
    glBufferData (GL_PIXEL_PACK_BUFFER,
		     stride * pixmap->drawable.height,
		     NULL, gl_usage);
    glReadPixels (0, 0,
                    row_length, pixmap->drawable.height,
                    format, type, 0);
    data = glMapBuffer (GL_PIXEL_PACK_BUFFER, gl_access);
    pixmap_priv->pbo_valid = TRUE;

    if (!glamor_priv->yInverted) {
      assert(glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP);
      glPixelStorei(GL_PACK_INVERT_MESA, 0);
    }
    glBindBuffer (GL_PIXEL_PACK_BUFFER, 0);
    } else {
    glReadPixels (0, 0,
                    row_length, pixmap->drawable.height,
                    format, type, data);
    }
  } else {
    data = malloc(stride * pixmap->drawable.height);
    assert(data);
    if (access != GLAMOR_ACCESS_WO) {
      if (pixmap_priv->pbo == 0)
	glGenBuffers(1, &pixmap_priv->pbo);
      glBindBuffer(GL_PIXEL_PACK_BUFFER, pixmap_priv->pbo);
      glBufferData(GL_PIXEL_PACK_BUFFER,
		      stride * pixmap->drawable.height,
		      NULL, GL_STREAM_READ);
      glReadPixels (0, 0, row_length, pixmap->drawable.height,
		    format, type, 0);
      read = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	  
      for (y = 0; y < pixmap->drawable.height; y++)
	memcpy(data + y * stride,
	       read + (pixmap->drawable.height - y - 1) * stride, stride);
      glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
      glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
      pixmap_priv->pbo_valid = FALSE;
      glDeleteBuffers(1, &pixmap_priv->pbo);
      pixmap_priv->pbo = 0;
    }
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
done:
  pixmap->devPrivate.ptr = data;
  return TRUE;
}



static void 
_glamor_destroy_upload_pixmap(PixmapPtr pixmap)
{
  glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);

  assert(pixmap_priv->gl_fbo == 0);
  if (pixmap_priv->fb)
    glDeleteFramebuffers(1, &pixmap_priv->fb);
  if (pixmap_priv->tex)
    glDeleteTextures(1, &pixmap_priv->tex);
  if (pixmap_priv->pbo)
    glDeleteBuffers(1, &pixmap_priv->pbo);
  pixmap_priv->fb = pixmap_priv->tex = pixmap_priv->pbo = 0;

}

void glamor_destroy_upload_pixmap(PixmapPtr pixmap)
{
  _glamor_destroy_upload_pixmap(pixmap);
}

