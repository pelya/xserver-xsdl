/*
 * Copyright © 2008 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

/** @file glamor.c
 * This file covers the initialization and teardown of glamor, and has various
 * functions not responsible for performing rendering.
 */

#include <stdlib.h>

#include "glamor_priv.h"

static DevPrivateKeyRec glamor_screen_private_key_index;
DevPrivateKey glamor_screen_private_key = &glamor_screen_private_key_index;
static DevPrivateKeyRec glamor_pixmap_private_key_index;
DevPrivateKey glamor_pixmap_private_key = &glamor_pixmap_private_key_index;

/**
 * glamor_get_drawable_pixmap() returns a backing pixmap for a given drawable.
 *
 * @param drawable the drawable being requested.
 *
 * This function returns the backing pixmap for a drawable, whether it is a
 * redirected window, unredirected window, or already a pixmap.  Note that
 * coordinate translation is needed when drawing to the backing pixmap of a
 * redirected window, and the translation coordinates are provided by calling
 * exaGetOffscreenPixmap() on the drawable.
 */
PixmapPtr
glamor_get_drawable_pixmap(DrawablePtr drawable)
{
     if (drawable->type == DRAWABLE_WINDOW)
	return drawable->pScreen->GetWindowPixmap((WindowPtr)drawable);
     else
	return (PixmapPtr)drawable;
}

static void
glamor_set_pixmap_texture(PixmapPtr pixmap, int w, int h, unsigned int tex)
{
    ScreenPtr screen = pixmap->drawable.pScreen;
    glamor_pixmap_private *pixmap_priv;

    pixmap_priv = glamor_get_pixmap_private(pixmap);
    assert(pixmap_priv);
    pixmap_priv->tex = tex;

    /* Create a framebuffer object wrapping the texture so that we can render
     * to it.
     */
    pixmap_priv->gl_fbo = 1;
    pixmap_priv->gl_tex = 1;
    glGenFramebuffers(1, &pixmap_priv->fb);
    glBindFramebuffer(GL_FRAMEBUFFER, pixmap_priv->fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                             GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D,
                             pixmap_priv->tex,
                             0);

    screen->ModifyPixmapHeader(pixmap, w, h, 0, 0,
                              (((w * pixmap->drawable.bitsPerPixel +
                                 7) / 8) + 3) & ~3,
                              NULL);
}

/* Set screen pixmap. If tex equal to 0, means it is called from ephyr. */
void
glamor_set_screen_pixmap_texture(ScreenPtr screen, int w, int h, unsigned int tex)
{
  PixmapPtr pixmap = screen->GetScreenPixmap(screen);
  glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
  glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

  glamor_set_pixmap_texture(pixmap, w, h, tex);
  glamor_priv->screen_fbo = pixmap_priv->fb;
  pixmap_priv->pending_op.type = GLAMOR_PENDING_NONE;
}



#define GLAMOR_PIXMAP_MEMORY 0 
#define GLAMOR_PIXMAP_TEXTURE 1 



static PixmapPtr
glamor_create_pixmap(ScreenPtr screen, int w, int h, int depth,
		     unsigned int usage)
{
    PixmapPtr pixmap;
    GLenum format;
    GLuint tex;
    int type = GLAMOR_PIXMAP_TEXTURE;
    glamor_pixmap_private *pixmap_priv;
    if (w > 32767 || h > 32767)
	return NullPixmap;

    if (!glamor_check_fbo_width_height(w,h)
	|| !glamor_check_fbo_depth(depth) 
	|| usage == GLAMOR_CREATE_PIXMAP_CPU) {
	/* MESA can only support upto MAX_WIDTH*MAX_HEIGHT fbo.
 	   If we exceed such limitation, we have to use framebuffer.*/
      type = GLAMOR_PIXMAP_MEMORY;
      pixmap = fbCreatePixmap (screen, w, h, depth, usage);
      screen->ModifyPixmapHeader(pixmap, w, h, 0, 0,
                              (((w * pixmap->drawable.bitsPerPixel +
                                 7) / 8) + 3) & ~3,
                              NULL);
#if 0
      if (usage != GLAMOR_CREATE_PIXMAP_CPU)
	glamor_fallback("choose cpu memory for pixmap %p ,"
			" %d x %d depth %d\n", pixmap, w, h, depth);
#endif
   } else 
      pixmap = fbCreatePixmap (screen, 0, 0, depth, usage);

    if (dixAllocatePrivates(&pixmap->devPrivates, PRIVATE_PIXMAP) != TRUE) {
        fbDestroyPixmap(pixmap);
	ErrorF("Fail to allocate privates for PIXMAP.\n");
	return NullPixmap;
    }	 

    pixmap_priv = glamor_get_pixmap_private(pixmap);
    pixmap_priv->container = pixmap;

    if (w == 0 || h == 0 || type == GLAMOR_PIXMAP_MEMORY)
	return pixmap;

    switch (depth) {
    case 8:
        format = GL_ALPHA;
        break;
    case 24:
        format = GL_RGB;
        break; 
    default:
        format = GL_RGBA;
        break;
    }

    /* Create the texture used to store the pixmap's data. */
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0,
                 format, GL_UNSIGNED_BYTE, NULL);

    glamor_set_pixmap_texture(pixmap, w, h, tex);
    return pixmap;
}


/**
 * For Xephyr use only. set up the screen pixmap to correct state. 
 **/
static PixmapPtr
glamor_create_screen_pixmap(ScreenPtr screen, int w, int h, int depth,
		     unsigned int usage)
{
    PixmapPtr pixmap;
    glamor_pixmap_private *pixmap_priv;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    assert(w ==0 && h == 0);

    glamor_priv->screen_fbo = 0; 
    pixmap = fbCreatePixmap (screen, 0, 0, depth, usage);

    if (dixAllocatePrivates(&pixmap->devPrivates, PRIVATE_PIXMAP) != TRUE) {
        fbDestroyPixmap(pixmap);
	ErrorF("Fail to allocate privates for PIXMAP.\n");
	return NullPixmap;
    }	 

    pixmap_priv = glamor_get_pixmap_private(pixmap);
    pixmap_priv->tex = 0; 
    pixmap_priv->gl_fbo = 1;
    pixmap_priv->gl_tex = 0;
    pixmap_priv->container = pixmap;
    pixmap_priv->pending_op.type = GLAMOR_PENDING_NONE;
    
    screen->CreatePixmap = glamor_create_pixmap;
    return pixmap;
}




static Bool
glamor_destroy_pixmap(PixmapPtr pixmap)
{
    if (pixmap->refcnt == 1) {
	glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
        if (pixmap_priv->fb)
	  glDeleteFramebuffers(1, &pixmap_priv->fb);
        if (pixmap_priv->tex)
	  glDeleteTextures(1, &pixmap_priv->tex);
        if (pixmap_priv->pbo)
          glDeleteBuffers(1, &pixmap_priv->pbo);
        dixFreePrivates(pixmap->devPrivates, PRIVATE_PIXMAP);
    }

    return fbDestroyPixmap(pixmap);
}

static void
glamor_block_handler(void *data, OSTimePtr timeout, void *last_select_mask)
{
    glFlush();
}

static void
glamor_wakeup_handler(void *data, int result, void *last_select_mask)
{
}

static void 
glamor_set_debug_level(int *debug_level)
{
  char *debug_level_string;
  debug_level_string = getenv("GLAMOR_DEBUG");
  if (debug_level_string && sscanf(debug_level_string, "%d", debug_level) == 1)
     return;
  *debug_level = 0;
}

int glamor_debug_level;

/** Set up glamor for an already-configured GL context. */
Bool
glamor_init(ScreenPtr screen, unsigned int flags)
{
    glamor_screen_private *glamor_priv;

#ifdef RENDER
    PictureScreenPtr ps = GetPictureScreenIfSet(screen);
#endif
    if (flags & ~GLAMOR_VALID_FLAGS) {
      ErrorF("glamor_init: Invalid flags %x\n", flags);
      return FALSE;
    }
    glamor_priv = calloc(1, sizeof(*glamor_priv));
    if (glamor_priv == NULL)
	return FALSE;

    if (flags & GLAMOR_INVERTED_Y_AXIS) {
	glamor_priv->yInverted = 1;
    } else
	glamor_priv->yInverted = 0;

    if (!dixRegisterPrivateKey(glamor_screen_private_key,PRIVATE_SCREEN,
			   0)) {
	LogMessage(X_WARNING,
		   "glamor%d: Failed to allocate screen private\n",
		   screen->myNum);
        return FALSE;
    }

    dixSetPrivate(&screen->devPrivates, glamor_screen_private_key, glamor_priv);

    if (!dixRegisterPrivateKey(glamor_pixmap_private_key,PRIVATE_PIXMAP,
                           sizeof(glamor_pixmap_private))) {
        LogMessage(X_WARNING,
                   "glamor%d: Failed to allocate pixmap private\n",
                   screen->myNum);
        return FALSE;
    }

    glewInit();

#ifndef GLAMOR_GLES2
    if (!GLEW_EXT_framebuffer_object) {
	ErrorF("GL_EXT_framebuffer_object required\n");
	goto fail;
    }
    if (!GLEW_ARB_shader_objects) {
	ErrorF("GL_ARB_shader_objects required\n");
	goto fail;
    }
    if (!GLEW_ARB_vertex_shader) {
	ErrorF("GL_ARB_vertex_shader required\n");
	goto fail;
    }
    if (!GLEW_EXT_bgra) {
	ErrorF("GL_EXT_bgra required\n");
	goto fail;
    }
#endif

    if (!GLEW_ARB_pixel_buffer_object) {
	ErrorF("GL_ARB_pixel_buffer_object required\n");
	goto fail;
    }
 
    if (!RegisterBlockAndWakeupHandlers(glamor_block_handler,
					glamor_wakeup_handler,
					NULL)) {
	goto fail;
    }

    glamor_set_debug_level(&glamor_debug_level); 
    glamor_priv->saved_close_screen = screen->CloseScreen;
    screen->CloseScreen = glamor_close_screen;

    glamor_priv->saved_create_gc = screen->CreateGC;
    screen->CreateGC = glamor_create_gc;

    glamor_priv->saved_create_pixmap = screen->CreatePixmap;

    if (flags & GLAMOR_HOSTX)
      screen->CreatePixmap = glamor_create_screen_pixmap;
    else
      screen->CreatePixmap = glamor_create_pixmap;

    glamor_priv->saved_destroy_pixmap = screen->DestroyPixmap;
    screen->DestroyPixmap = glamor_destroy_pixmap;

    glamor_priv->saved_get_spans = screen->GetSpans;
    screen->GetSpans = glamor_get_spans;

    glamor_priv->saved_get_image = screen->GetImage;
    screen->GetImage = miGetImage;

    glamor_priv->saved_change_window_attributes = screen->ChangeWindowAttributes;
    screen->ChangeWindowAttributes = glamor_change_window_attributes;

    glamor_priv->saved_copy_window = screen->CopyWindow;
    screen->CopyWindow = glamor_copy_window;

    glamor_priv->saved_bitmap_to_region = screen->BitmapToRegion;
    screen->BitmapToRegion = glamor_bitmap_to_region;

#ifdef RENDER
    glamor_priv->saved_composite = ps->Composite;
    ps->Composite = glamor_composite;
    glamor_priv->saved_trapezoids = ps->Trapezoids;
    ps->Trapezoids = glamor_trapezoids;
    glamor_priv->saved_glyphs = ps->Glyphs;
    ps->Glyphs = glamor_glyphs;
    glamor_priv->saved_triangles = ps->Triangles;
    ps->Triangles = glamor_triangles;
    glamor_init_composite_shaders(screen);
    glamor_priv->saved_create_picture = ps->CreatePicture; 
    ps->CreatePicture = glamor_create_picture;
    glamor_priv->saved_destroy_picture = ps->DestroyPicture; 
    ps->DestroyPicture = glamor_destroy_picture;
#endif
    glamor_init_solid_shader(screen);
    glamor_init_tile_shader(screen);
    glamor_init_putimage_shaders(screen);
    glamor_init_finish_access_shaders(screen);
    glamor_glyphs_init(screen);
    glamor_pixmap_init(screen);

    return TRUE;

fail:
    free(glamor_priv);
    dixSetPrivate(&screen->devPrivates, glamor_screen_private_key, NULL);
    return FALSE;
}

Bool 
glamor_close_screen(int idx, ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
#ifdef RENDER
    PictureScreenPtr	ps = GetPictureScreenIfSet(screen);
#endif
    glamor_glyphs_fini(screen);
    screen->CloseScreen = glamor_priv->saved_close_screen;
    screen->CreateGC = glamor_priv->saved_create_gc;
    screen->CreatePixmap = glamor_priv->saved_create_pixmap;
    screen->DestroyPixmap = glamor_priv->saved_destroy_pixmap;
    screen->GetSpans = glamor_priv->saved_get_spans;
    screen->ChangeWindowAttributes = glamor_priv->saved_change_window_attributes;
    screen->CopyWindow = glamor_priv->saved_copy_window;
    screen->BitmapToRegion = glamor_priv->saved_bitmap_to_region;
#ifdef RENDER
    if (ps) {
	ps->Composite = glamor_priv->saved_composite;
	ps->Trapezoids = glamor_priv->saved_trapezoids;
	ps->Glyphs = glamor_priv->saved_glyphs;
        ps->Triangles = glamor_priv->saved_triangles;
        ps->CreatePicture = glamor_priv->saved_create_picture;
    }
#endif
    if (glamor_priv->vb)
      free(glamor_priv->vb);
    free(glamor_priv);
    return screen->CloseScreen(idx, screen);   
    
}

void
glamor_fini(ScreenPtr screen)
{
/* Do nothing currently. */
}
