#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdlib.h>

#include "glamor_priv.h"

glamor_pixmap_fbo *
glamor_create_fbo_from_tex(glamor_screen_private *glamor_priv,
		  int w, int h, int depth, GLint tex, int flag)
{
	glamor_gl_dispatch *dispatch;
	glamor_pixmap_fbo *fbo;
	GLenum format;

	fbo = calloc(1, sizeof(*fbo));
	if (fbo == NULL)
		return NULL;

	gl_iformat_for_depth(depth, &format);

	fbo->tex = tex;
	fbo->width = w;
	fbo->height = h;
	fbo->format = format;
	fbo->glamor_priv = glamor_priv;

	glamor_pixmap_ensure_fb(fbo);

	return fbo;
}

/* Make sure already detached from any pixmap. */
void
glamor_destroy_fbo(glamor_pixmap_fbo *fbo)
{
	glamor_gl_dispatch *dispatch = &fbo->glamor_priv->dispatch;

	DEBUGF("Destroy fbo %p tex %d fb %d \n", fbo, fbo->tex, fbo->fb);
	if (fbo->fb)
		dispatch->glDeleteFramebuffers(1, &fbo->fb);
	if (fbo->tex)
		dispatch->glDeleteTextures(1, &fbo->tex);
	if (fbo->pbo)
		dispatch->glDeleteBuffers(1, &fbo->pbo);

	free(fbo);
}

glamor_pixmap_fbo *
glamor_create_fbo(glamor_screen_private *glamor_priv,
		  int w, int h, int depth, int flag)
{
	glamor_gl_dispatch *dispatch;
	glamor_pixmap_fbo *fbo;
	GLenum format;
	GLint tex;

	gl_iformat_for_depth(depth, &format);
	dispatch = &glamor_priv->dispatch;
	dispatch->glGenTextures(1, &tex);
	dispatch->glBindTexture(GL_TEXTURE_2D, tex);
	dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				  GL_NEAREST);
	dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
				  GL_NEAREST);
	dispatch->glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format,
			       GL_UNSIGNED_BYTE, NULL);

	fbo = glamor_create_fbo_from_tex(glamor_priv, w, h, depth, tex, flag);
	DEBUGF("Creat fbo %p tex %d width %d height %d \n", fbo, tex, w, h);

	return fbo;
}

glamor_pixmap_fbo *
glamor_pixmap_detach_fbo(glamor_pixmap_private *pixmap_priv)
{
	glamor_pixmap_fbo *fbo;

	if (pixmap_priv == NULL)
		return NULL;

	fbo = pixmap_priv->fbo;
	if (fbo == NULL)
		return NULL;

	pixmap_priv->fbo = NULL;
	return fbo;
}

/* The pixmap must not be attached to another fbo. */
void
glamor_pixmap_attach_fbo(PixmapPtr pixmap, glamor_pixmap_fbo *fbo)
{
	glamor_pixmap_private *pixmap_priv;

	pixmap_priv = glamor_get_pixmap_private(pixmap);

	if (pixmap_priv == NULL) {
		glamor_screen_private *glamor_priv;
		glamor_priv = glamor_get_screen_private(pixmap->drawable.pScreen);
		pixmap_priv = calloc(1, sizeof(*pixmap_priv));
		dixSetPrivate(&pixmap->devPrivates,
			      glamor_pixmap_private_key, pixmap_priv);
		pixmap_priv->container = pixmap;
		pixmap_priv->glamor_priv = glamor_priv;
		pixmap_priv->type = GLAMOR_MEMORY;
	}

	if (pixmap_priv->fbo)
		return;

	pixmap_priv->fbo = fbo;

	switch (pixmap_priv->type) {
	case GLAMOR_TEXTURE_ONLY:
	case GLAMOR_TEXTURE_DRM:
		pixmap_priv->gl_fbo = 1;
		if (fbo->tex != 0)
			pixmap_priv->gl_tex = 1;
		else {
			/* XXX For the Xephyr only, may be broken now.*/
			pixmap_priv->gl_tex = 0;
		}
		pixmap->devPrivate.ptr = NULL;
		break;
	default:
		break;
	}
}
