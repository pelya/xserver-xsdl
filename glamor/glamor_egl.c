/*
 * Copyright © 2010 Intel Corporation.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including
 * the next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Zhigang Gong <zhigang.gong@linux.intel.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define GLAMOR_FOR_XORG
#include <xorg-server.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <xf86.h>
#include <xf86drm.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#define EGL_DISPLAY_NO_X_MESA

#ifdef GLAMOR_HAS_GBM
#include <gbm.h>
#endif

#if GLAMOR_GLES2
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#include <GL/gl.h>
#endif

#define MESA_EGL_NO_X11_HEADERS
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "glamor.h"
#include "glamor_gl_dispatch.h"
#ifdef GLX_USE_SHARED_DISPATCH
#include "glapi.h"
#endif

static const char glamor_name[] = "glamor";

static DevPrivateKeyRec glamor_egl_pixmap_private_key_index;
DevPrivateKey glamor_egl_pixmap_private_key = &glamor_egl_pixmap_private_key_index;

static void
glamor_identify(int flags)
{
	xf86Msg(X_INFO, "%s: OpenGL accelerated X.org driver based.\n",
		glamor_name);
}

struct glamor_egl_screen_private {
	EGLDisplay display;
	EGLContext context;
	EGLint major, minor;

	CreateScreenResourcesProcPtr CreateScreenResources;
	CloseScreenProcPtr CloseScreen;
	int fd;
	EGLImageKHR front_image;
	PixmapPtr *back_pixmap;
	int cpp;
#ifdef GLAMOR_HAS_GBM
	struct gbm_device *gbm;
#endif
	int has_gem;
	void *gl_context, *old_context;
	int gl_context_depth;

	PFNEGLCREATEIMAGEKHRPROC egl_create_image_khr;
	PFNEGLDESTROYIMAGEKHRPROC egl_destroy_image_khr;
	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC egl_image_target_texture2d_oes;
	struct glamor_gl_dispatch *dispatch;
	CloseScreenProcPtr saved_close_screen;
	xf86FreeScreenProc *saved_free_screen;
};

int xf86GlamorEGLPrivateIndex = -1;

static struct glamor_egl_screen_private *
glamor_egl_get_screen_private(ScrnInfoPtr scrn)
{
	return (struct glamor_egl_screen_private *)
	    scrn->privates[xf86GlamorEGLPrivateIndex].ptr;
}
#ifdef GLX_USE_SHARED_DISPATCH
_X_EXPORT void
glamor_egl_make_current(ScreenPtr screen)
{
	ScrnInfoPtr scrn = xf86Screens[screen->myNum];
	struct glamor_egl_screen_private *glamor_egl =
	    glamor_egl_get_screen_private(scrn);

	if (glamor_egl->gl_context_depth++)
		return;

	GET_CURRENT_CONTEXT(current);
	glamor_egl->old_context = current;

	if (glamor_egl->gl_context != current) {
		eglMakeCurrent(glamor_egl->display, EGL_NO_SURFACE,
			       EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (!eglMakeCurrent(glamor_egl->display,
				    EGL_NO_SURFACE, EGL_NO_SURFACE,
				    glamor_egl->context)) {
			FatalError("Failed to make EGL context current\n");
		}
	}
}

_X_EXPORT void
glamor_egl_restore_context(ScreenPtr screen)
{
	ScrnInfoPtr scrn = xf86Screens[screen->myNum];
	struct glamor_egl_screen_private *glamor_egl =
	    glamor_egl_get_screen_private(scrn);

	if (--glamor_egl->gl_context_depth)
		return;

	if (glamor_egl->old_context &&
	    glamor_egl->gl_context != glamor_egl->old_context)
		SET_CURRENT_CONTEXT(glamor_egl->old_context);
}
#else
#define glamor_egl_make_current(x)
#define glamor_egl_restore_context(s)
#endif

static EGLImageKHR
_glamor_egl_create_image(struct glamor_egl_screen_private *glamor_egl,
			 int width, int height, int stride, int name, int depth)
{
	EGLImageKHR image;
	EGLint attribs[] = {
		EGL_WIDTH, 0,
		EGL_HEIGHT, 0,
		EGL_DRM_BUFFER_STRIDE_MESA, 0,
		EGL_DRM_BUFFER_FORMAT_MESA,
		EGL_DRM_BUFFER_FORMAT_ARGB32_MESA,
		EGL_DRM_BUFFER_USE_MESA,
		EGL_DRM_BUFFER_USE_SHARE_MESA |
		    EGL_DRM_BUFFER_USE_SCANOUT_MESA,
		EGL_NONE
	};
	attribs[1] = width;
	attribs[3] = height;
	attribs[5] = stride;
	if (depth != 32 && depth != 24)
		return EGL_NO_IMAGE_KHR;
	image = glamor_egl->egl_create_image_khr(glamor_egl->display,
						 glamor_egl->context,
						 EGL_DRM_BUFFER_MESA,
						 (void *) (uintptr_t)name, attribs);
	if (image == EGL_NO_IMAGE_KHR)
		return EGL_NO_IMAGE_KHR;


	return image;
}

static int
glamor_get_flink_name(int fd, int handle, int *name)
{
	struct drm_gem_flink flink;
	flink.handle = handle;
	if (ioctl(fd, DRM_IOCTL_GEM_FLINK, &flink) < 0)
		return FALSE;
	*name = flink.name;
	return TRUE;
}

static Bool
glamor_create_texture_from_image(struct glamor_egl_screen_private
				 *glamor_egl,
				 EGLImageKHR image, GLuint * texture)
{
	glamor_egl->dispatch->glGenTextures(1, texture);
	glamor_egl->dispatch->glBindTexture(GL_TEXTURE_2D, *texture);
	glamor_egl->dispatch->glTexParameteri(GL_TEXTURE_2D,
					      GL_TEXTURE_MIN_FILTER,
					      GL_NEAREST);
	glamor_egl->dispatch->glTexParameteri(GL_TEXTURE_2D,
					      GL_TEXTURE_MAG_FILTER,
					      GL_NEAREST);

	(glamor_egl->egl_image_target_texture2d_oes) (GL_TEXTURE_2D,
						      image);
	glamor_egl->dispatch->glBindTexture(GL_TEXTURE_2D, 0);
	return TRUE;
}

Bool
glamor_egl_create_textured_screen(ScreenPtr screen, int handle, int stride)
{
	ScrnInfoPtr scrn = xf86Screens[screen->myNum];
	struct glamor_egl_screen_private *glamor_egl;
	PixmapPtr	screen_pixmap;

	glamor_egl = glamor_egl_get_screen_private(scrn);
	screen_pixmap = screen->GetScreenPixmap(screen);

	if (!glamor_egl_create_textured_pixmap(screen_pixmap, handle, stride)) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR, "Failed to create textured screen.");
		return FALSE;
	}

	glamor_egl->front_image = dixLookupPrivate(&screen_pixmap->devPrivates,
						    glamor_egl_pixmap_private_key);
	glamor_set_screen_pixmap(screen_pixmap, glamor_egl->back_pixmap);
	return TRUE;
}

Bool
glamor_egl_create_textured_screen_ext(ScreenPtr screen,
				      int handle,
				      int stride,
				      PixmapPtr *back_pixmap)
{
	ScrnInfoPtr scrn = xf86Screens[screen->myNum];
	struct glamor_egl_screen_private *glamor_egl;

	glamor_egl = glamor_egl_get_screen_private(scrn);

	glamor_egl->back_pixmap = back_pixmap;
	if (!glamor_egl_create_textured_screen(screen, handle, stride))
		return FALSE;
	return TRUE;
}

Bool
glamor_egl_check_has_gem(int fd)
{
	struct drm_gem_flink flink;
	flink.handle = 0;
	int err;

	ioctl(fd, DRM_IOCTL_GEM_FLINK, &flink);
	if (errno == ENOENT || err == EINVAL)
		return TRUE;
	return FALSE;
}

Bool
glamor_egl_create_textured_pixmap(PixmapPtr pixmap, int handle, int stride)
{
	ScreenPtr screen = pixmap->drawable.pScreen;
	ScrnInfoPtr scrn = xf86Screens[screen->myNum];
	struct glamor_egl_screen_private *glamor_egl;
	EGLImageKHR image;
	GLuint texture;
	int name;
	Bool ret = FALSE;

	glamor_egl = glamor_egl_get_screen_private(scrn);

	glamor_egl_make_current(screen);
	if (glamor_egl->has_gem) {
		if (!glamor_get_flink_name(glamor_egl->fd, handle, &name)) {
			xf86DrvMsg(scrn->scrnIndex, X_ERROR,
				   "Couldn't flink pixmap handle\n");
			glamor_set_pixmap_type(pixmap, GLAMOR_DRM_ONLY);
			exit(1);
		}
	} else
		name = handle;

	image = _glamor_egl_create_image(glamor_egl,
					 pixmap->drawable.width,
					 pixmap->drawable.height,
					 ((stride * 8 + 7) / pixmap->drawable.bitsPerPixel),
					 name,
					 pixmap->drawable.depth);
	if (image == EGL_NO_IMAGE_KHR) {
		glamor_set_pixmap_type(pixmap, GLAMOR_DRM_ONLY);
		goto done;
	}
	glamor_create_texture_from_image(glamor_egl, image, &texture);
	glamor_set_pixmap_type(pixmap, GLAMOR_TEXTURE_DRM);
	glamor_set_pixmap_texture(pixmap, texture);
	dixSetPrivate(&pixmap->devPrivates, glamor_egl_pixmap_private_key,
		      image);
	ret = TRUE;

done:
	glamor_egl_restore_context(screen);
	return ret;
}

static void
_glamor_egl_destroy_pixmap_image(PixmapPtr pixmap)
{
	ScrnInfoPtr scrn = xf86Screens[pixmap->drawable.pScreen->myNum];
	EGLImageKHR image;
	struct glamor_egl_screen_private *glamor_egl =
	    glamor_egl_get_screen_private(scrn);

	image = dixLookupPrivate(&pixmap->devPrivates,
				 glamor_egl_pixmap_private_key);
	if (image != EGL_NO_IMAGE_KHR && image != NULL) {
		/* Before destroy an image which was attached to
		 * a texture. we must call glFlush to make sure the
		 * operation on that texture has been done.*/
		glamor_block_handler(pixmap->drawable.pScreen);
		glamor_egl->egl_destroy_image_khr(glamor_egl->display, image);
		dixSetPrivate(&pixmap->devPrivates, glamor_egl_pixmap_private_key, NULL);
	}
}

extern void glamor_pixmap_exchange_fbos(PixmapPtr front, PixmapPtr back);

_X_EXPORT void
glamor_egl_exchange_buffers(PixmapPtr front, PixmapPtr back)
{
	ScrnInfoPtr scrn = xf86Screens[front->drawable.pScreen->myNum];
	struct glamor_egl_screen_private *glamor_egl =
	    glamor_egl_get_screen_private(scrn);
	EGLImageKHR old_front_image;
	EGLImageKHR new_front_image;

	glamor_pixmap_exchange_fbos(front, back);
	new_front_image = dixLookupPrivate(&back->devPrivates, glamor_egl_pixmap_private_key);
	old_front_image = dixLookupPrivate(&front->devPrivates, glamor_egl_pixmap_private_key);
	dixSetPrivate(&front->devPrivates, glamor_egl_pixmap_private_key, new_front_image);
	dixSetPrivate(&back->devPrivates, glamor_egl_pixmap_private_key, old_front_image);
	glamor_set_pixmap_type(front, GLAMOR_TEXTURE_DRM);
	glamor_set_pixmap_type(back, GLAMOR_TEXTURE_DRM);
	glamor_egl->front_image = new_front_image;

}

void
glamor_egl_destroy_textured_pixmap(PixmapPtr pixmap)
{
	if (pixmap->refcnt == 1)
		_glamor_egl_destroy_pixmap_image(pixmap);
	glamor_destroy_textured_pixmap(pixmap);
}

static Bool
glamor_egl_close_screen(int idx, ScreenPtr screen)
{
	ScrnInfoPtr scrn;
	struct glamor_egl_screen_private *glamor_egl;
	PixmapPtr screen_pixmap;
	EGLImageKHR back_image;

	scrn = xf86Screens[screen->myNum];
	glamor_egl = glamor_egl_get_screen_private(scrn);
	screen_pixmap = screen->GetScreenPixmap(screen);

	glamor_egl->egl_destroy_image_khr(glamor_egl->display, glamor_egl->front_image);
	dixSetPrivate(&screen_pixmap->devPrivates, glamor_egl_pixmap_private_key, NULL);
	glamor_egl->front_image = NULL;
	if (glamor_egl->back_pixmap && *glamor_egl->back_pixmap) {
		back_image = dixLookupPrivate(&(*glamor_egl->back_pixmap)->devPrivates,
					       glamor_egl_pixmap_private_key);
		if (back_image != NULL && back_image != EGL_NO_IMAGE_KHR) {
			glamor_egl->egl_destroy_image_khr(glamor_egl->display, back_image);
			dixSetPrivate(&(*glamor_egl->back_pixmap)->devPrivates,
				      glamor_egl_pixmap_private_key, NULL);
		}
	}

	screen->CloseScreen = glamor_egl->saved_close_screen;

	return screen->CloseScreen(idx, screen);
}

static Bool
glamor_egl_has_extension(struct glamor_egl_screen_private *glamor_egl,
			 char *extension)
{
	const char *egl_extensions;
	char *pext;
	int ext_len;
	ext_len = strlen(extension);

	egl_extensions =
	    (const char *) eglQueryString(glamor_egl->display,
					  EGL_EXTENSIONS);
	pext = (char *) egl_extensions;

	if (pext == NULL || extension == NULL)
		return FALSE;
	while ((pext = strstr(pext, extension)) != NULL) {
		if (pext[ext_len] == ' ' || pext[ext_len] == '\0')
			return TRUE;
		pext += ext_len;
	}
	return FALSE;
}

void
glamor_egl_screen_init(ScreenPtr screen)
{
	ScrnInfoPtr scrn = xf86Screens[screen->myNum];
	struct glamor_egl_screen_private *glamor_egl =
	    glamor_egl_get_screen_private(scrn);

	glamor_egl->saved_close_screen = screen->CloseScreen;
	screen->CloseScreen = glamor_egl_close_screen;
}

static void
glamor_egl_free_screen(int scrnIndex, int flags)
{
	ScrnInfoPtr scrn = xf86Screens[scrnIndex];
	struct glamor_egl_screen_private *glamor_egl;

	glamor_egl = glamor_egl_get_screen_private(scrn);
	if (glamor_egl != NULL) {

		eglMakeCurrent(glamor_egl->display,
			       EGL_NO_SURFACE, EGL_NO_SURFACE,
			       EGL_NO_CONTEXT);
#ifdef GLAMOR_HAS_GBM
		if (glamor_egl->gbm)
			gbm_device_destroy(glamor_egl->gbm);
#endif
		scrn->FreeScreen = glamor_egl->saved_free_screen;
		free(glamor_egl);
		scrn->FreeScreen(scrnIndex, flags);
	}
}

Bool
glamor_egl_init(ScrnInfoPtr scrn, int fd)
{
	struct glamor_egl_screen_private *glamor_egl;
	const char *version;
	EGLint config_attribs[] = {
#ifdef GLAMOR_GLES2
		EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
		EGL_NONE
	};

	glamor_identify(0);
	glamor_egl = calloc(sizeof(*glamor_egl), 1);
	if (xf86GlamorEGLPrivateIndex == -1)
		xf86GlamorEGLPrivateIndex =
		    xf86AllocateScrnInfoPrivateIndex();

	scrn->privates[xf86GlamorEGLPrivateIndex].ptr = glamor_egl;
	glamor_egl->fd = fd;
#ifdef GLAMOR_HAS_GBM
	glamor_egl->gbm = gbm_create_device(glamor_egl->fd);
	if (glamor_egl->gbm == NULL) {
		ErrorF("couldn't get display device\n");
		return FALSE;
	}
	glamor_egl->display = eglGetDisplay(glamor_egl->gbm);
#else
	glamor_egl->display = eglGetDisplay((EGLNativeDisplayType)fd);
#endif

	glamor_egl->has_gem = glamor_egl_check_has_gem(fd);

#ifndef GLAMOR_GLES2
	eglBindAPI(EGL_OPENGL_API);
#else
	eglBindAPI(EGL_OPENGL_ES_API);
#endif
	if (!eglInitialize
	    (glamor_egl->display, &glamor_egl->major, &glamor_egl->minor))
	{
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "eglInitialize() failed\n");
		return FALSE;
	}

	version = eglQueryString(glamor_egl->display, EGL_VERSION);
	xf86Msg(X_INFO, "%s: EGL version %s:\n", glamor_name, version);

#define GLAMOR_CHECK_EGL_EXTENSION(EXT)  \
	if (!glamor_egl_has_extension(glamor_egl, "EGL_" #EXT)) {  \
		ErrorF("EGL_" #EXT "required.\n");  \
		return FALSE;  \
	}

	GLAMOR_CHECK_EGL_EXTENSION(MESA_drm_image);
	GLAMOR_CHECK_EGL_EXTENSION(KHR_gl_renderbuffer_image);
#ifdef GLAMOR_GLES2
	GLAMOR_CHECK_EGL_EXTENSION(KHR_surfaceless_gles2);
#else
	GLAMOR_CHECK_EGL_EXTENSION(KHR_surfaceless_opengl);
#endif

	glamor_egl->egl_create_image_khr = (PFNEGLCREATEIMAGEKHRPROC)
	    eglGetProcAddress("eglCreateImageKHR");

	glamor_egl->egl_destroy_image_khr = (PFNEGLDESTROYIMAGEKHRPROC)
	    eglGetProcAddress("eglDestroyImageKHR");

	glamor_egl->egl_image_target_texture2d_oes =
	    (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)
	    eglGetProcAddress("glEGLImageTargetTexture2DOES");

	if (!glamor_egl->egl_create_image_khr
	    || !glamor_egl->egl_image_target_texture2d_oes) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "eglGetProcAddress() failed\n");
		return FALSE;
	}

	glamor_egl->context = eglCreateContext(glamor_egl->display,
					       NULL, EGL_NO_CONTEXT,
					       config_attribs);
	if (glamor_egl->context == EGL_NO_CONTEXT) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "Failed to create EGL context\n");
		return FALSE;
	}

	if (!eglMakeCurrent(glamor_egl->display,
			    EGL_NO_SURFACE, EGL_NO_SURFACE,
			    glamor_egl->context)) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "Failed to make EGL context current\n");
		return FALSE;
	}
#ifdef GLX_USE_SHARED_DISPATCH
	GET_CURRENT_CONTEXT(current);
	glamor_egl->gl_context = current;
#endif
	glamor_egl->saved_free_screen = scrn->FreeScreen;
	scrn->FreeScreen = glamor_egl_free_screen;
#ifdef GLAMOR_GLES2
	xf86DrvMsg(scrn->scrnIndex, X_INFO, "Using GLES2.\n");
#ifdef GLX_USE_SHARED_DISPATCH
	xf86DrvMsg(scrn->scrnIndex, X_WARNING, "Glamor is using GLES2 but GLX needs GL. "
					       "Indirect GLX may not work correctly.\n");
#endif
#endif
	return TRUE;
}

Bool
glamor_egl_init_textured_pixmap(ScreenPtr screen)
{
	if (!dixRegisterPrivateKey
	    (glamor_egl_pixmap_private_key, PRIVATE_PIXMAP, 0)) {
		LogMessage(X_WARNING,
			   "glamor%d: Failed to allocate egl pixmap private\n",
			   screen->myNum);
		return FALSE;
	}
	return TRUE;
}

Bool
glamor_gl_dispatch_init(ScreenPtr screen,
			struct glamor_gl_dispatch *dispatch,
			int gl_version)
{
	ScrnInfoPtr scrn = xf86Screens[screen->myNum];
	struct glamor_egl_screen_private *glamor_egl =
	    glamor_egl_get_screen_private(scrn);
	if (!glamor_gl_dispatch_init_impl
	    (dispatch, gl_version, (get_proc_address_t)eglGetProcAddress))
		return FALSE;
	glamor_egl->dispatch = dispatch;
	return TRUE;
}
