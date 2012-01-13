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
		return drawable->
		    pScreen->GetWindowPixmap((WindowPtr) drawable);
	else
		return (PixmapPtr) drawable;
}

_X_EXPORT void
glamor_set_pixmap_type(PixmapPtr pixmap, glamor_pixmap_type_t type)
{
	glamor_pixmap_private *pixmap_priv;
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(pixmap->drawable.pScreen);

	pixmap_priv = glamor_get_pixmap_private(pixmap);
	if (pixmap_priv == NULL) {
		pixmap_priv = calloc(sizeof(*pixmap_priv), 1);
		dixSetPrivate(&pixmap->devPrivates,
			      glamor_pixmap_private_key, pixmap_priv);
		pixmap_priv->container = pixmap;
		pixmap_priv->glamor_priv = glamor_priv;
	}
	pixmap_priv->type = type;
}


_X_EXPORT void
glamor_set_pixmap_texture(PixmapPtr pixmap, int w, int h, unsigned int tex)
{
	ScreenPtr screen = pixmap->drawable.pScreen;
	glamor_pixmap_private *pixmap_priv;
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(screen);
	glamor_gl_dispatch *dispatch = &glamor_priv->dispatch;

	pixmap_priv = glamor_get_pixmap_private(pixmap);
	if (pixmap_priv == NULL) {
		pixmap_priv = calloc(sizeof(*pixmap_priv), 1);
		dixSetPrivate(&pixmap->devPrivates,
			      glamor_pixmap_private_key, pixmap_priv);
		pixmap_priv->container = pixmap;
		pixmap_priv->glamor_priv = glamor_priv;
	}

	if (pixmap_priv->fb)
		dispatch->glDeleteFramebuffers(1, &pixmap_priv->fb);

	if (pixmap_priv->tex)
		dispatch->glDeleteTextures(1, &pixmap_priv->tex);

	pixmap_priv->tex = tex;

	/* Create a framebuffer object wrapping the texture so that we can render
	 * to it.
	 */
	pixmap_priv->gl_fbo = 1;
	if (tex != 0) {
		glamor_pixmap_ensure_fb(pixmap);
		pixmap_priv->gl_tex = 1;
	} else {
		pixmap_priv->fb = 0;
		pixmap_priv->gl_tex = 0;
	}

	if (pixmap->devKind == 0)
		screen->ModifyPixmapHeader(pixmap, w, h, 0, 0,
					   (((w *
					      pixmap->drawable.
					      bitsPerPixel + 7) / 8) +
					    3) & ~3, NULL);
	pixmap->devPrivate.ptr = NULL;
}

/* Set screen pixmap. If tex equal to 0, means it is called from ephyr. */
void
glamor_set_screen_pixmap_texture(ScreenPtr screen, int w, int h,
				 unsigned int tex)
{
	PixmapPtr pixmap = screen->GetScreenPixmap(screen);
	glamor_pixmap_private *pixmap_priv;
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(screen);

	glamor_set_pixmap_texture(pixmap, w, h, tex);
	pixmap_priv = glamor_get_pixmap_private(pixmap);
	glamor_priv->screen_fbo = pixmap_priv->fb;
}

PixmapPtr
glamor_create_pixmap(ScreenPtr screen, int w, int h, int depth,
		     unsigned int usage)
{
	PixmapPtr pixmap;
	GLenum format;
	GLuint tex;
	glamor_pixmap_type_t type = GLAMOR_TEXTURE_ONLY;
	glamor_pixmap_private *pixmap_priv;
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(screen);
	glamor_gl_dispatch *dispatch = &glamor_priv->dispatch;
	if (w > 32767 || h > 32767)
		return NullPixmap;

	if (!glamor_check_fbo_size(glamor_priv, w, h)
	    || !glamor_check_fbo_depth(depth)
	    || usage == GLAMOR_CREATE_PIXMAP_CPU) {
		/* MESA can only support upto MAX_WIDTH*MAX_HEIGHT fbo.
		   If we exceed such limitation, we have to use framebuffer. */
		type = GLAMOR_MEMORY;
		pixmap = fbCreatePixmap(screen, w, h, depth, usage);
	} else
		pixmap = fbCreatePixmap(screen, 0, 0, depth, usage);

	pixmap_priv = calloc(1, sizeof(*pixmap_priv));
	dixSetPrivate(&pixmap->devPrivates, glamor_pixmap_private_key,
		      pixmap_priv);
	pixmap_priv->container = pixmap;
	pixmap_priv->glamor_priv = glamor_priv;

	pixmap_priv->type = type;
	if (w == 0 || h == 0 || type == GLAMOR_MEMORY)
		return pixmap;

	gl_iformat_for_depth(depth, &format);
	/* Create the texture used to store the pixmap's data. */
	dispatch->glGenTextures(1, &tex);
	dispatch->glBindTexture(GL_TEXTURE_2D, tex);
	dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				  GL_NEAREST);
	dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
				  GL_NEAREST);
	dispatch->glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format,
			       GL_UNSIGNED_BYTE, NULL);

	glamor_set_pixmap_texture(pixmap, w, h, tex);

	return pixmap;
}

void
glamor_destroy_textured_pixmap(PixmapPtr pixmap)
{
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(pixmap->drawable.pScreen);
	glamor_gl_dispatch *dispatch = &glamor_priv->dispatch;
	if (pixmap->refcnt == 1) {
		glamor_pixmap_private *pixmap_priv =
		    glamor_get_pixmap_private(pixmap);
		if (pixmap_priv != NULL) {
			if (pixmap_priv->fb)
				dispatch->glDeleteFramebuffers(1,
							       &pixmap_priv->fb);
			if (pixmap_priv->tex)
				dispatch->glDeleteTextures(1,
							   &pixmap_priv->tex);
			if (pixmap_priv->pbo)
				dispatch->glDeleteBuffers(1,
							  &pixmap_priv->pbo);
			free(pixmap_priv);
		}
	}
}

Bool
glamor_destroy_pixmap(PixmapPtr pixmap)
{
	glamor_destroy_textured_pixmap(pixmap);
	return fbDestroyPixmap(pixmap);
}

void
glamor_block_handler(ScreenPtr screen)
{
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(screen);
	glamor_gl_dispatch *dispatch = &glamor_priv->dispatch;

	dispatch->glFlush();
	dispatch->glFinish();
}

static void
_glamor_block_handler(void *data, OSTimePtr timeout,
		      void *last_select_mask)
{
	glamor_gl_dispatch *dispatch = data;
	dispatch->glFlush();
}

static void
_glamor_wakeup_handler(void *data, int result, void *last_select_mask)
{
}

static void
glamor_set_debug_level(int *debug_level)
{
	char *debug_level_string;
	debug_level_string = getenv("GLAMOR_DEBUG");
	if (debug_level_string
	    && sscanf(debug_level_string, "%d", debug_level) == 1)
		return;
	*debug_level = 0;
}

int glamor_debug_level;

/** Set up glamor for an already-configured GL context. */
Bool
glamor_init(ScreenPtr screen, unsigned int flags)
{
	glamor_screen_private *glamor_priv;
	int gl_version;

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

	if (!dixRegisterPrivateKey
	    (glamor_screen_private_key, PRIVATE_SCREEN, 0)) {
		LogMessage(X_WARNING,
			   "glamor%d: Failed to allocate screen private\n",
			   screen->myNum);
		goto fail;
	}

	dixSetPrivate(&screen->devPrivates, glamor_screen_private_key,
		      glamor_priv);

	if (!dixRegisterPrivateKey
	    (glamor_pixmap_private_key, PRIVATE_PIXMAP, 0)) {
		LogMessage(X_WARNING,
			   "glamor%d: Failed to allocate pixmap private\n",
			   screen->myNum);
		goto fail;;
	}

	gl_version = glamor_gl_get_version();

#ifndef GLAMOR_GLES2
	if (gl_version < GLAMOR_GL_VERSION_ENCODE(1, 3)) {
		ErrorF("Require OpenGL version 1.3 or latter.\n");
		goto fail;
	}
#else
	if (gl_version < GLAMOR_GL_VERSION_ENCODE(2, 0)) {
		ErrorF("Require Open GLES2.0 or latter.\n");
		goto fail;
	}
#endif

	glamor_gl_dispatch_init(screen, &glamor_priv->dispatch,
				gl_version);

#ifdef GLAMOR_GLES2
	if (!glamor_gl_has_extension("GL_EXT_texture_format_BGRA8888")) {
		ErrorF("GL_EXT_texture_format_BGRA8888 required\n");
		goto fail;
	}
#endif

	glamor_priv->has_pack_invert =
	    glamor_gl_has_extension("GL_MESA_pack_invert");
	glamor_priv->has_fbo_blit =
	    glamor_gl_has_extension("GL_EXT_framebuffer_blit");
	glamor_priv->dispatch.glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE,
					    &glamor_priv->max_fbo_size);

	glamor_set_debug_level(&glamor_debug_level);

	if (flags & GLAMOR_USE_SCREEN) {
		if (!RegisterBlockAndWakeupHandlers(_glamor_block_handler,
						    _glamor_wakeup_handler,
						    (void *)
						    &glamor_priv->dispatch)) {
			goto fail;
		}

		glamor_priv->saved_procs.close_screen = screen->CloseScreen;
		screen->CloseScreen = glamor_close_screen;

		glamor_priv->saved_procs.create_gc = screen->CreateGC;
		screen->CreateGC = glamor_create_gc;

		glamor_priv->saved_procs.create_pixmap = screen->CreatePixmap;
		screen->CreatePixmap = glamor_create_pixmap;

		glamor_priv->saved_procs.destroy_pixmap = screen->DestroyPixmap;
		screen->DestroyPixmap = glamor_destroy_pixmap;

		glamor_priv->saved_procs.get_spans = screen->GetSpans;
		screen->GetSpans = glamor_get_spans;

		glamor_priv->saved_procs.get_image = screen->GetImage;
		screen->GetImage = glamor_get_image;

		glamor_priv->saved_procs.change_window_attributes =
		    screen->ChangeWindowAttributes;
		screen->ChangeWindowAttributes =
		    glamor_change_window_attributes;

		glamor_priv->saved_procs.copy_window = screen->CopyWindow;
		screen->CopyWindow = glamor_copy_window;

		glamor_priv->saved_procs.bitmap_to_region =
		    screen->BitmapToRegion;
		screen->BitmapToRegion = glamor_bitmap_to_region;
	}
#ifdef RENDER
	if (flags & GLAMOR_USE_PICTURE_SCREEN) {
		glamor_priv->saved_procs.composite = ps->Composite;
		ps->Composite = glamor_composite;

		glamor_priv->saved_procs.trapezoids = ps->Trapezoids;
		ps->Trapezoids = glamor_trapezoids;

		glamor_priv->saved_procs.glyphs = ps->Glyphs;
		ps->Glyphs = glamor_glyphs;

		glamor_priv->saved_procs.triangles = ps->Triangles;
		ps->Triangles = glamor_triangles;

		glamor_priv->saved_procs.addtraps = ps->AddTraps;
		ps->AddTraps = glamor_add_traps;

		glamor_priv->saved_procs.unrealize_glyph = ps->UnrealizeGlyph;
		ps->UnrealizeGlyph = glamor_glyph_unrealize;
	}
	glamor_priv->saved_procs.create_picture = ps->CreatePicture;
	ps->CreatePicture = glamor_create_picture;

	glamor_priv->saved_procs.destroy_picture = ps->DestroyPicture;
	ps->DestroyPicture = glamor_destroy_picture;
	glamor_init_composite_shaders(screen);
#endif
	glamor_init_solid_shader(screen);
	glamor_init_tile_shader(screen);
	glamor_init_putimage_shaders(screen);
	glamor_init_finish_access_shaders(screen);
	glamor_pixmap_init(screen);

#ifdef GLAMOR_GLES2
	glamor_priv->gl_flavor = GLAMOR_GL_ES2;
#else
	glamor_priv->gl_flavor = GLAMOR_GL_DESKTOP;
#endif

	return TRUE;

      fail:
	free(glamor_priv);
	dixSetPrivate(&screen->devPrivates, glamor_screen_private_key,
		      NULL);
	return FALSE;
}

Bool
glamor_close_screen(int idx, ScreenPtr screen)
{
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(screen);
#ifdef RENDER
	PictureScreenPtr ps = GetPictureScreenIfSet(screen);
#endif
	glamor_glyphs_fini(screen);
	screen->CloseScreen = glamor_priv->saved_procs.close_screen;
	screen->CreateGC = glamor_priv->saved_procs.create_gc;
	screen->CreatePixmap = glamor_priv->saved_procs.create_pixmap;
	screen->DestroyPixmap = glamor_priv->saved_procs.destroy_pixmap;
	screen->GetSpans = glamor_priv->saved_procs.get_spans;
	screen->ChangeWindowAttributes =
	    glamor_priv->saved_procs.change_window_attributes;
	screen->CopyWindow = glamor_priv->saved_procs.copy_window;
	screen->BitmapToRegion = glamor_priv->saved_procs.bitmap_to_region;
#ifdef RENDER
	if (ps) {
		ps->Composite = glamor_priv->saved_procs.composite;
		ps->Trapezoids = glamor_priv->saved_procs.trapezoids;
		ps->Glyphs = glamor_priv->saved_procs.glyphs;
		ps->Triangles = glamor_priv->saved_procs.triangles;
		ps->CreatePicture = glamor_priv->saved_procs.create_picture;
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
