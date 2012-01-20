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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif
#define GLAMOR_FOR_XORG
#include "xorg-server.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <xf86.h>
#include <xf86drm.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#define EGL_DISPLAY_NO_X_MESA

#include <gbm.h>

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
	int front_buffer_handle;
	int cpp;
	struct gbm_device *gbm;

	PFNEGLCREATEIMAGEKHRPROC egl_create_image_khr;
	PFNEGLDESTROYIMAGEKHRPROC egl_destroy_image_khr;
	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC egl_image_target_texture2d_oes;
	struct glamor_gl_dispatch *dispatch;
};

int xf86GlamorEGLPrivateIndex = -1;

static struct glamor_egl_screen_private
*
glamor_egl_get_screen_private(ScrnInfoPtr scrn)
{
	return (struct glamor_egl_screen_private *)
	    scrn->privates[xf86GlamorEGLPrivateIndex].ptr;
}

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

	glamor_set_screen_pixmap(screen_pixmap);
	return TRUE;
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

	glamor_egl = glamor_egl_get_screen_private(scrn);

	if (!glamor_get_flink_name(glamor_egl->fd, handle, &name)) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "Couldn't flink pixmap handle\n");
		glamor_set_pixmap_type(pixmap, GLAMOR_DRM_ONLY);
		exit(1);
	}
	image = _glamor_egl_create_image(glamor_egl,
					 pixmap->drawable.width,
					 pixmap->drawable.height,
					 ((stride * 8 + 7) / pixmap->drawable.bitsPerPixel),
					 name,
					 pixmap->drawable.depth);
	if (image == EGL_NO_IMAGE_KHR) 
		return FALSE;

	glamor_create_texture_from_image(glamor_egl, image, &texture);
	glamor_set_pixmap_type(pixmap, GLAMOR_TEXTURE_DRM);
	glamor_set_pixmap_texture(pixmap, texture);
	dixSetPrivate(&pixmap->devPrivates, glamor_egl_pixmap_private_key,
		      image);

	return TRUE;
}

void
glamor_egl_destroy_textured_pixmap(PixmapPtr pixmap)
{
	EGLImageKHR image;
	ScrnInfoPtr scrn = xf86Screens[pixmap->drawable.pScreen->myNum];
	struct glamor_egl_screen_private *glamor_egl =
	    glamor_egl_get_screen_private(scrn);

	if (pixmap->refcnt == 1) {
		image = dixLookupPrivate(&pixmap->devPrivates,
					 glamor_egl_pixmap_private_key);
		if (image != EGL_NO_IMAGE_KHR && image != NULL) {
			/* Before destroy an image which was attached to 
 			 * a texture. we must call glFlush to make sure the 
 			 * operation on that texture has been done.*/
			glamor_block_handler(pixmap->drawable.pScreen);
			glamor_egl->egl_destroy_image_khr(glamor_egl->display, image);
		}
	}
	glamor_destroy_textured_pixmap(pixmap);
}

Bool
glamor_egl_close_screen(ScreenPtr screen)
{
	ScrnInfoPtr scrn = xf86Screens[screen->myNum];
	struct glamor_egl_screen_private *glamor_egl =
	    glamor_egl_get_screen_private(scrn);
	glamor_fini(screen);

	return TRUE;
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
	glamor_egl->gbm = gbm_create_device(glamor_egl->fd);
	if (glamor_egl->gbm == NULL) {
		ErrorF("couldn't get display device\n");
		return FALSE;
	}
	glamor_egl->display = eglGetDisplay(glamor_egl->gbm);

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

void
glamor_egl_free_screen(int scrnIndex, int flags)
{
	ScrnInfoPtr scrn = xf86Screens[scrnIndex];
	struct glamor_egl_screen_private *glamor_egl =
	    glamor_egl_get_screen_private(scrn);

	if (glamor_egl != NULL) {
		if (!(flags & GLAMOR_EGL_EXTERNAL_BUFFER)) {
			eglMakeCurrent(glamor_egl->display,
				       EGL_NO_SURFACE, EGL_NO_SURFACE,
				       EGL_NO_CONTEXT);

			eglTerminate(glamor_egl->display);
		}
		free(glamor_egl);
	}

	glamor_close_screen(scrn->scrnIndex, scrn->pScreen);
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
