#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdlib.h>

#include "glamor_priv.h"

#define GLAMOR_CACHE_EXPIRE_MAX 100

#define GLAMOR_CACHE_DEFAULT    0
#define GLAMOR_CACHE_EXACT_SIZE 1
#define GLAMOR_CACHE_TEXTURE	2

/* Loop from the tail to the head. */
#define list_for_each_entry_reverse(pos, head, member)                  \
    for (pos = __container_of((head)->prev, pos, member);               \
         &pos->member != (head);                                        \
         pos = __container_of(pos->member.prev, pos, member))


#define list_for_each_entry_safe_reverse(pos, tmp, head, member)        \
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
inline static int cache_format(GLenum format)
{
	switch (format) {
#if 0
	case GL_ALPHA:
		return 1;
#endif
	case GL_RGBA:
	default:
		return 0;
	}
}

glamor_pixmap_fbo *
glamor_pixmap_fbo_cache_get(glamor_screen_private *glamor_priv,
			    int w, int h, GLenum format, int flag)
{
	struct list *cache;
	glamor_pixmap_fbo *fbo_entry;
	int size;

	if (!(flag & GLAMOR_CACHE_TEXTURE))
		cache = &glamor_priv->fbo_cache[cache_format(format)]
					       [cache_wbucket(w)]
					       [cache_hbucket(h)];
	else
		cache = &glamor_priv->tex_cache[cache_format(format)]
					       [cache_wbucket(w)]
					       [cache_hbucket(h)];
	if (!(flag & GLAMOR_CACHE_EXACT_SIZE)) {
		list_for_each_entry(fbo_entry, cache, list) {
			if (fbo_entry->width >= w && fbo_entry->height >= h) {

				DEBUGF("Request w %d h %d \n", w, h);
				DEBUGF("got cache entry %p w %d h %d fbo %d tex %d\n",
					fbo_entry, fbo_entry->width, fbo_entry->height,
					fbo_entry->fb, fbo_entry->tex);
				list_del(&fbo_entry->list);
				return fbo_entry;
			}
		}
	}
	else {
		list_for_each_entry(fbo_entry, cache, list) {
			if (fbo_entry->width == w && fbo_entry->height == h) {

				DEBUGF("Request w %d h %d \n", w, h);
				DEBUGF("got cache entry %p w %d h %d fbo %d tex %d\n",
					fbo_entry, fbo_entry->width, fbo_entry->height,
					fbo_entry->fb, fbo_entry->tex);
				list_del(&fbo_entry->list);
				return fbo_entry;
			}
		}
	}

	return NULL;
}

void
glamor_purge_fbo(glamor_pixmap_fbo *fbo)
{
	GLAMOR_DEFINE_CONTEXT;

	GLAMOR_SET_CONTEXT(fbo->glamor_priv);
	glamor_gl_dispatch *dispatch = &fbo->glamor_priv->dispatch;
	if (fbo->fb)
		dispatch->glDeleteFramebuffers(1, &fbo->fb);
	if (fbo->tex)
		dispatch->glDeleteTextures(1, &fbo->tex);
	if (fbo->pbo)
		dispatch->glDeleteBuffers(1, &fbo->pbo);
	GLAMOR_RESTORE_CONTEXT(fbo->glamor_priv);

	free(fbo);
}


void
glamor_pixmap_fbo_cache_put(glamor_pixmap_fbo *fbo)
{
	struct list *cache;

	if (fbo->fb == 0) {
		glamor_purge_fbo(fbo);
		return;
	}

	if (fbo->fb)
		cache = &fbo->glamor_priv->fbo_cache[cache_format(fbo->format)]
						    [cache_wbucket(fbo->width)]
						    [cache_hbucket(fbo->height)];
	else
		cache = &fbo->glamor_priv->tex_cache[cache_format(fbo->format)]
						    [cache_wbucket(fbo->width)]
						    [cache_hbucket(fbo->height)];
	DEBUGF("Put cache entry %p to cache %p w %d h %d format %x fbo %d tex %d \n", fbo, cache,
		fbo->width, fbo->height, fbo->format, fbo->fb, fbo->tex);
	list_add(&fbo->list, cache);
	fbo->expire = fbo->glamor_priv->tick + GLAMOR_CACHE_EXPIRE_MAX;
}

glamor_pixmap_fbo *
glamor_create_fbo_from_tex(glamor_screen_private *glamor_priv,
		  int w, int h, int depth, GLint tex, int flag)
{
	glamor_pixmap_fbo *fbo;
	GLenum format;

	fbo = calloc(1, sizeof(*fbo));
	if (fbo == NULL)
		return NULL;

	list_init(&fbo->list);
	gl_iformat_for_depth(depth, &format);

	fbo->tex = tex;
	fbo->width = w;
	fbo->height = h;
	fbo->format = format;
	fbo->glamor_priv = glamor_priv;

	if (flag != GLAMOR_CREATE_FBO_NO_FBO)
		glamor_pixmap_ensure_fb(fbo);

	return fbo;
}


void
glamor_fbo_expire(glamor_screen_private *glamor_priv)
{
	struct list *cache;
	glamor_pixmap_fbo *fbo_entry, *tmp;
	int i,j,k;
	int empty_cache = TRUE;

	for(i = 0; i < CACHE_FORMAT_COUNT; i++)
		for(j = 0; j < CACHE_BUCKET_WCOUNT; j++)
			for(k = 0; k < CACHE_BUCKET_HCOUNT; k++) {
				cache = &glamor_priv->fbo_cache[i][j][k];
				list_for_each_entry_safe_reverse(fbo_entry, tmp, cache, list) {
					if (GLAMOR_TICK_AFTER(fbo_entry->expire, glamor_priv->tick)) {
						empty_cache = FALSE;
						break;
					}
					list_del(&fbo_entry->list);
					DEBUGF("cache %p fbo %p expired %d current %d \n", cache, fbo_entry,
						fbo_entry->expire, glamor_priv->tick);
					glamor_purge_fbo(fbo_entry);
				}
#if 0
				cache = &glamor_priv->tex_cache[i][j][k];
				list_for_each_entry_safe_reverse(fbo_entry, tmp, cache, list) {
					if (GLAMOR_TICK_AFTER(fbo_entry->expire, glamor_priv->tick)) {
						empty_cache = FALSE;
						break;
					}
					list_del(&fbo_entry->list);
					DEBUGF("cache %p fbo %p expired %d current %d \n", cache, fbo_entry,
						fbo_entry->expire, glamor_priv->tick);
					glamor_purge_fbo(fbo_entry);
				}
#endif
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
				list_init(&glamor_priv->fbo_cache[i][j][k]);
				list_init(&glamor_priv->tex_cache[i][j][k]);
			}
}

void
glamor_fini_pixmap_fbo(ScreenPtr screen)
{
	struct list *cache;
	glamor_screen_private *glamor_priv;
	glamor_pixmap_fbo *fbo_entry, *tmp;
	int i,j,k;

	glamor_priv = glamor_get_screen_private(screen);
	for(i = 0; i < CACHE_FORMAT_COUNT; i++)
		for(j = 0; j < CACHE_BUCKET_WCOUNT; j++)
			for(k = 0; k < CACHE_BUCKET_HCOUNT; k++)
			{
				cache = &glamor_priv->fbo_cache[i][j][k];
				list_for_each_entry_safe_reverse(fbo_entry, tmp, cache, list) {
					list_del(&fbo_entry->list);
					glamor_purge_fbo(fbo_entry);
				}
#if 0
				cache = &glamor_priv->tex_cache[i][j][k];
				list_for_each_entry_safe_reverse(fbo_entry, tmp, cache, list) {
					list_del(&fbo_entry->list);
					glamor_purge_fbo(fbo_entry);
				}
#endif
			}
}

void
glamor_destroy_fbo(glamor_pixmap_fbo *fbo)
{
	list_del(&fbo->list);
	glamor_pixmap_fbo_cache_put(fbo);

}

glamor_pixmap_fbo *
glamor_create_tex_obj(glamor_screen_private *glamor_priv,
		      int w, int h, GLenum format, int flag)
{
	glamor_gl_dispatch *dispatch;
	glamor_pixmap_fbo *fbo;
	int cache_flag = GLAMOR_CACHE_TEXTURE;
	GLuint tex;
	GLAMOR_DEFINE_CONTEXT;

	if (flag == GLAMOR_CREATE_TEXTURE_EXACT_SIZE)
		cache_flag |= GLAMOR_CACHE_EXACT_SIZE;

	fbo = glamor_pixmap_fbo_cache_get(glamor_priv, w, h,
					  format, cache_flag);
	if (fbo)
		return fbo;
	fbo = calloc(1, sizeof(*fbo));
	if (fbo == NULL)
		return NULL;

	GLAMOR_SET_CONTEXT(glamor_priv);
	list_init(&fbo->list);

	dispatch = &glamor_priv->dispatch;
	dispatch->glGenTextures(1, &tex);
	dispatch->glBindTexture(GL_TEXTURE_2D, tex);
	dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				  GL_NEAREST);
	dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
				  GL_NEAREST);
	dispatch->glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format,
			       GL_UNSIGNED_BYTE, NULL);
	fbo->tex = tex;
	fbo->width = w;
	fbo->height = h;
	fbo->format = format;
	fbo->glamor_priv = glamor_priv;
	GLAMOR_RESTORE_CONTEXT(glamor_priv);

	return fbo;
}

void
glamor_destroy_tex_obj(glamor_pixmap_fbo * tex_obj)
{
	assert(tex_obj->fb == 0);
	list_del(&tex_obj->list);
	glamor_pixmap_fbo_cache_put(tex_obj);
}

glamor_pixmap_fbo *
glamor_create_fbo(glamor_screen_private *glamor_priv,
		  int w, int h, int depth, int flag)
{
	glamor_gl_dispatch *dispatch;
	glamor_pixmap_fbo *fbo;
	GLenum format;
	GLint tex;
	int cache_flag;
	GLAMOR_DEFINE_CONTEXT;

	if (!glamor_check_fbo_size(glamor_priv, w, h)
	    || !glamor_check_fbo_depth(depth))
		return NULL;

	if (flag == GLAMOR_CREATE_FBO_NO_FBO)
		goto new_fbo;

	if (flag == GLAMOR_CREATE_PIXMAP_FIXUP)
		cache_flag = GLAMOR_CACHE_EXACT_SIZE;
	else
		cache_flag = 0;

	gl_iformat_for_depth(depth, &format);
	fbo = glamor_pixmap_fbo_cache_get(glamor_priv, w, h,
					  format, cache_flag);
	if (fbo)
		return fbo;
new_fbo:

	GLAMOR_SET_CONTEXT(glamor_priv);
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
	GLAMOR_RESTORE_CONTEXT(glamor_priv);

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
