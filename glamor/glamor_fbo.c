/*
 * Copyright © 2009 Intel Corporation
 * Copyright © 1998 Keith Packard
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
 *    Zhigang Gong <zhigang.gong@gmail.com>
 *
 */

#include <stdlib.h>

#include "glamor_priv.h"

#define GLAMOR_CACHE_EXPIRE_MAX 100

#define GLAMOR_CACHE_DEFAULT    0
#define GLAMOR_CACHE_EXACT_SIZE 1

//#define NO_FBO_CACHE 1
#define FBO_CACHE_THRESHOLD  (256*1024*1024)

/* Loop from the tail to the head. */
#define xorg_list_for_each_entry_reverse(pos, head, member)             \
    for (pos = __container_of((head)->prev, pos, member);               \
         &pos->member != (head);                                        \
         pos = __container_of(pos->member.prev, pos, member))


#define xorg_list_for_each_entry_safe_reverse(pos, tmp, head, member)   \
    for (pos = __container_of((head)->prev, pos, member),               \
         tmp = __container_of(pos->member.prev, pos, member);           \
         &pos->member != (head);                                        \
         pos = tmp, tmp = __container_of(pos->member.prev, tmp, member))

#ifdef __i386__
static inline unsigned long __fls(unsigned long x)
{
        asm("bsr %1,%0"
            : "=r" (x)
            : "rm" (x));
        return x;
}
#else
static inline unsigned long __fls(unsigned long x)
{
   int n;

   if (x == 0) return(0);
   n = 0;
   if (x <= 0x0000FFFF) {n = n +16; x = x <<16;}
   if (x <= 0x00FFFFFF) {n = n + 8; x = x << 8;}
   if (x <= 0x0FFFFFFF) {n = n + 4; x = x << 4;}
   if (x <= 0x3FFFFFFF) {n = n + 2; x = x << 2;}
   if (x <= 0x7FFFFFFF) {n = n + 1;}
   return 31 - n;
}
#endif

inline static int cache_wbucket(int size)
{
	int order = __fls(size / 32);
	if (order >= CACHE_BUCKET_WCOUNT)
		order = CACHE_BUCKET_WCOUNT - 1;
	return order;
}

inline static int cache_hbucket(int size)
{
	int order = __fls(size / 32);
	if (order >= CACHE_BUCKET_HCOUNT)
		order = CACHE_BUCKET_HCOUNT - 1;
	return order;
}

static glamor_pixmap_fbo *
glamor_pixmap_fbo_cache_get(glamor_screen_private *glamor_priv,
			    int w, int h, GLenum format, int flag)
{
	struct xorg_list *cache;
	glamor_pixmap_fbo *fbo_entry, *ret_fbo = NULL;
	int n_format;
#ifdef NO_FBO_CACHE
	return NULL;
#else
	n_format = cache_format(format);
	if (n_format == -1)
		return NULL;
	cache = &glamor_priv->fbo_cache[n_format]
				       [cache_wbucket(w)]
				       [cache_hbucket(h)];
	if (!(flag & GLAMOR_CACHE_EXACT_SIZE)) {
		xorg_list_for_each_entry(fbo_entry, cache, list) {
			if (fbo_entry->width >= w && fbo_entry->height >= h) {

				DEBUGF("Request w %d h %d format %x \n", w, h, format);
				DEBUGF("got cache entry %p w %d h %d fbo %d tex %d format %x\n",
					fbo_entry, fbo_entry->width, fbo_entry->height,
					fbo_entry->fb, fbo_entry->tex);
				xorg_list_del(&fbo_entry->list);
				ret_fbo = fbo_entry;
				break;
			}
		}
	}
	else {
		xorg_list_for_each_entry(fbo_entry, cache, list) {
			if (fbo_entry->width == w && fbo_entry->height == h) {

				DEBUGF("Request w %d h %d format %x \n", w, h, format);
				DEBUGF("got cache entry %p w %d h %d fbo %d tex %d format %x\n",
					fbo_entry, fbo_entry->width, fbo_entry->height,
					fbo_entry->fb, fbo_entry->tex, fbo_entry->format);
				assert(format == fbo_entry->format);
				xorg_list_del(&fbo_entry->list);
				ret_fbo = fbo_entry;
				break;
			}
		}
	}

	if (ret_fbo)
		glamor_priv->fbo_cache_watermark -= ret_fbo->width * ret_fbo->height;

	assert(glamor_priv->fbo_cache_watermark >= 0);

	return ret_fbo;
#endif
}

void
glamor_purge_fbo(glamor_pixmap_fbo *fbo)
{
	glamor_gl_dispatch *dispatch = glamor_get_dispatch(fbo->glamor_priv);
	if (fbo->fb)
		dispatch->glDeleteFramebuffers(1, &fbo->fb);
	if (fbo->tex)
		dispatch->glDeleteTextures(1, &fbo->tex);
	if (fbo->pbo)
		dispatch->glDeleteBuffers(1, &fbo->pbo);
	glamor_put_dispatch(fbo->glamor_priv);

	free(fbo);
}

static void
glamor_pixmap_fbo_cache_put(glamor_pixmap_fbo *fbo)
{
	struct xorg_list *cache;
	int n_format;

#ifdef NO_FBO_CACHE
	glamor_purge_fbo(fbo);
	return;
#else
	n_format = cache_format(fbo->format);

	if (fbo->fb == 0 || n_format == -1
	   || fbo->glamor_priv->fbo_cache_watermark >= FBO_CACHE_THRESHOLD) {
		fbo->glamor_priv->tick ++;
		glamor_purge_fbo(fbo);
		return;
	}

	cache = &fbo->glamor_priv->fbo_cache[n_format]
					    [cache_wbucket(fbo->width)]
					    [cache_hbucket(fbo->height)];
	DEBUGF("Put cache entry %p to cache %p w %d h %d format %x fbo %d tex %d \n", fbo, cache,
		fbo->width, fbo->height, fbo->format, fbo->fb, fbo->tex);

	fbo->glamor_priv->fbo_cache_watermark += fbo->width * fbo->height;
	xorg_list_add(&fbo->list, cache);
	fbo->expire = fbo->glamor_priv->tick + GLAMOR_CACHE_EXPIRE_MAX;
#endif
}

static void
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

glamor_pixmap_fbo *
glamor_create_fbo_from_tex(glamor_screen_private *glamor_priv,
		  int w, int h, GLenum format, GLint tex, int flag)
{
	glamor_pixmap_fbo *fbo;

	fbo = calloc(1, sizeof(*fbo));
	if (fbo == NULL)
		return NULL;

	xorg_list_init(&fbo->list);

	fbo->tex = tex;
	fbo->width = w;
	fbo->height = h;
	fbo->format = format;
	fbo->glamor_priv = glamor_priv;

	if (flag == GLAMOR_CREATE_PIXMAP_MAP) {
		glamor_gl_dispatch *dispatch;
		dispatch = glamor_get_dispatch(glamor_priv);
		dispatch->glGenBuffers(1, &fbo->pbo);
		glamor_put_dispatch(glamor_priv);
		goto done;
	}

	if (flag != GLAMOR_CREATE_FBO_NO_FBO)
		glamor_pixmap_ensure_fb(fbo);

done:
	return fbo;
}


void
glamor_fbo_expire(glamor_screen_private *glamor_priv)
{
	struct xorg_list *cache;
	glamor_pixmap_fbo *fbo_entry, *tmp;
	int i,j,k;
	int empty_cache = TRUE;

	for(i = 0; i < CACHE_FORMAT_COUNT; i++)
		for(j = 0; j < CACHE_BUCKET_WCOUNT; j++)
			for(k = 0; k < CACHE_BUCKET_HCOUNT; k++) {
				cache = &glamor_priv->fbo_cache[i][j][k];
				xorg_list_for_each_entry_safe_reverse(fbo_entry, tmp, cache, list) {
					if (GLAMOR_TICK_AFTER(fbo_entry->expire, glamor_priv->tick)) {
						empty_cache = FALSE;
						break;
					}

					glamor_priv->fbo_cache_watermark -= fbo_entry->width * fbo_entry->height;
					xorg_list_del(&fbo_entry->list);
					DEBUGF("cache %p fbo %p expired %d current %d \n", cache, fbo_entry,
						fbo_entry->expire, glamor_priv->tick);
					glamor_purge_fbo(fbo_entry);
				}
			}

}

void
glamor_init_pixmap_fbo(ScreenPtr screen)
{
	glamor_screen_private *glamor_priv;
	int i,j,k;

	glamor_priv = glamor_get_screen_private(screen);
	for(i = 0; i < CACHE_FORMAT_COUNT; i++)
		for(j = 0; j < CACHE_BUCKET_WCOUNT; j++)
			for(k = 0; k < CACHE_BUCKET_HCOUNT; k++)
			{
				xorg_list_init(&glamor_priv->fbo_cache[i][j][k]);
			}
	glamor_priv->fbo_cache_watermark = 0;
}

void
glamor_fini_pixmap_fbo(ScreenPtr screen)
{
	struct xorg_list *cache;
	glamor_screen_private *glamor_priv;
	glamor_pixmap_fbo *fbo_entry, *tmp;
	int i,j,k;

	glamor_priv = glamor_get_screen_private(screen);
	for(i = 0; i < CACHE_FORMAT_COUNT; i++)
		for(j = 0; j < CACHE_BUCKET_WCOUNT; j++)
			for(k = 0; k < CACHE_BUCKET_HCOUNT; k++)
			{
				cache = &glamor_priv->fbo_cache[i][j][k];
				xorg_list_for_each_entry_safe_reverse(fbo_entry, tmp, cache, list) {
					xorg_list_del(&fbo_entry->list);
					glamor_purge_fbo(fbo_entry);
				}
			}
}

void
glamor_destroy_fbo(glamor_pixmap_fbo *fbo)
{
	xorg_list_del(&fbo->list);
	glamor_pixmap_fbo_cache_put(fbo);

}

static int
_glamor_create_tex(glamor_screen_private *glamor_priv,
		   int w, int h, GLenum format)
{
	glamor_gl_dispatch *dispatch;
	unsigned int tex;

	dispatch = glamor_get_dispatch(glamor_priv);
	dispatch->glGenTextures(1, &tex);
	dispatch->glBindTexture(GL_TEXTURE_2D, tex);
	dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				  GL_NEAREST);
	dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
				  GL_NEAREST);
	dispatch->glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format,
			       GL_UNSIGNED_BYTE, NULL);
	glamor_put_dispatch(glamor_priv);
	return tex;
}

glamor_pixmap_fbo *
glamor_create_fbo(glamor_screen_private *glamor_priv,
		  int w, int h,
		  GLenum format,
		  int flag)
{
	glamor_pixmap_fbo *fbo;
	GLint tex = 0;
	int cache_flag;

	if (!glamor_check_fbo_size(glamor_priv, w, h))
		return NULL;

	if (flag == GLAMOR_CREATE_FBO_NO_FBO)
		goto new_fbo;

	if (flag == GLAMOR_CREATE_PIXMAP_MAP)
		goto no_tex;

	if (flag == GLAMOR_CREATE_PIXMAP_FIXUP)
		cache_flag = GLAMOR_CACHE_EXACT_SIZE;
	else
		cache_flag = 0;

	fbo = glamor_pixmap_fbo_cache_get(glamor_priv, w, h,
					  format, cache_flag);
	if (fbo)
		return fbo;
new_fbo:
	tex = _glamor_create_tex(glamor_priv, w, h, format);
no_tex:
	fbo = glamor_create_fbo_from_tex(glamor_priv, w, h, format, tex, flag);

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
	case GLAMOR_MEMORY_MAP:
		pixmap->devPrivate.ptr = NULL;
		break;
	default:
		break;
	}
}


Bool
glamor_pixmap_ensure_fbo(PixmapPtr pixmap, GLenum format, int flag)
{
	glamor_screen_private *glamor_priv;
	glamor_pixmap_private *pixmap_priv;
	glamor_pixmap_fbo *fbo;

	glamor_priv = glamor_get_screen_private(pixmap->drawable.pScreen);
	pixmap_priv = glamor_get_pixmap_private(pixmap);
	if (pixmap_priv == NULL || pixmap_priv->fbo == NULL) {

		fbo = glamor_create_fbo(glamor_priv, pixmap->drawable.width,
					pixmap->drawable.height,
					format,
					flag);
		if (fbo == NULL)
			return FALSE;

		glamor_pixmap_attach_fbo(pixmap, fbo);
	} else {
		/* We do have a fbo, but it may lack of fb or tex. */
		if (!pixmap_priv->fbo->tex)
			pixmap_priv->fbo->tex = _glamor_create_tex(glamor_priv, pixmap->drawable.width,
								   pixmap->drawable.height, format);

		if (flag != GLAMOR_CREATE_FBO_NO_FBO && pixmap_priv->fbo->fb == 0)
			glamor_pixmap_ensure_fb(pixmap_priv->fbo);
	}

	pixmap_priv = glamor_get_pixmap_private(pixmap);
	return TRUE;
}

_X_EXPORT void
glamor_pixmap_exchange_fbos(PixmapPtr front, PixmapPtr back)
{
	glamor_pixmap_private *front_priv, *back_priv;
	glamor_pixmap_fbo *temp_fbo;

	front_priv = glamor_get_pixmap_private(front);
	back_priv = glamor_get_pixmap_private(back);
	temp_fbo = front_priv->fbo;
	front_priv->fbo = back_priv->fbo;
	back_priv->fbo = temp_fbo;
}
