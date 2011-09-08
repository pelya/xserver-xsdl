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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <xf86.h>
#include <xf86drm.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#define EGL_DISPLAY_NO_X_MESA

#if GLAMOR_GLES2
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#include <GL/gl.h>
#endif

#define MESA_EGL_NO_X11_HEADERS
#include <EGL/egl.h>
#include <EGL/eglext.h>

#define GLAMOR_FOR_XORG

#include <glamor.h>
#include <glamor_gl_dispatch.h>

#define GLAMOR_VERSION_MAJOR 0
#define GLAMOR_VERSION_MINOR 1
#define GLAMOR_VERSION_PATCH 0

static const char glamor_name[] = "glamor";

static void
glamor_identify(int flags)
{
	xf86Msg(X_INFO, "%s: OpenGL accelerated X.org driver based.\n", glamor_name);
}

struct glamor_screen_private {
	EGLDisplay display;
	EGLContext context;
	EGLImageKHR root;
	EGLint major, minor;
        

	CreateScreenResourcesProcPtr CreateScreenResources;
	CloseScreenProcPtr CloseScreen;
	int fd;
        int front_buffer_handle;
	int cpp;

	PFNEGLCREATEDRMIMAGEMESA egl_create_drm_image_mesa;
	PFNEGLEXPORTDRMIMAGEMESA egl_export_drm_image_mesa;
        PFNEGLCREATEIMAGEKHRPROC egl_create_image_khr;
	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC egl_image_target_texture2d_oes;
        struct glamor_gl_dispatch *dispatch;
};

int xf86GlamorEGLPrivateIndex = -1;

static struct glamor_screen_private* glamor_get_egl_screen_private(ScrnInfoPtr scrn)
{
        return (struct glamor_screen_private *)  scrn->privates[xf86GlamorEGLPrivateIndex].ptr;
}

static Bool
_glamor_create_egl_screen_image(ScrnInfoPtr scrn, int width, int height, int stride)
{

	struct glamor_screen_private *glamor = glamor_get_egl_screen_private(scrn);
	ScreenPtr screen = screenInfo.screens[scrn->scrnIndex];
	EGLImageKHR image;
	GLuint texture;
	EGLint attribs[] = {
		EGL_WIDTH,	0,
		EGL_HEIGHT,	0,
                EGL_DRM_BUFFER_STRIDE_MESA, 0,
		EGL_DRM_BUFFER_FORMAT_MESA,	EGL_DRM_BUFFER_FORMAT_ARGB32_MESA,
		EGL_DRM_BUFFER_USE_MESA,	EGL_DRM_BUFFER_USE_SHARE_MESA |
					EGL_DRM_BUFFER_USE_SCANOUT_MESA,
		EGL_NONE
	};

        if (glamor->root) {
          eglDestroyImageKHR(glamor->display, glamor->root);
	  glamor->root = EGL_NO_IMAGE_KHR;
        }
          
	attribs[1] = width;
	attribs[3] = height;
        attribs[5] = stride / 4;
        image = glamor->egl_create_image_khr (glamor->display, 
                                              glamor->context,
                                              EGL_DRM_BUFFER_MESA,
                                              (void*)glamor->front_buffer_handle,
                                              attribs);   
	if (image == EGL_NO_IMAGE_KHR)
		return FALSE;

	glamor->dispatch->glGenTextures(1, &texture);
	glamor->dispatch->glBindTexture(GL_TEXTURE_2D, texture);
	glamor->dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glamor->dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	(glamor->egl_image_target_texture2d_oes)(GL_TEXTURE_2D, image); 

	glamor_set_screen_pixmap_texture(screen, width, height, texture);
	glamor->root = image;
	return TRUE;
}

Bool
glamor_create_egl_screen_image(ScreenPtr screen, int handle, int stride)
{
	ScrnInfoPtr scrn = xf86Screens[screen->myNum];
	struct glamor_screen_private *glamor = glamor_get_egl_screen_private(scrn);
        struct drm_gem_flink flink;
        flink.handle = handle;

        if (ioctl(glamor->fd, DRM_IOCTL_GEM_FLINK, &flink) < 0) {
           xf86DrvMsg(scrn->scrnIndex, X_ERROR,
	       "Couldn't flink front buffer handle\n");
           return FALSE;
        }

        glamor->front_buffer_handle = flink.name;

        return _glamor_create_egl_screen_image(scrn, scrn->virtualX, scrn->virtualY, stride);
}

Bool
glamor_close_egl_screen(ScreenPtr screen)
{
	ScrnInfoPtr scrn = xf86Screens[screen->myNum];
	struct glamor_screen_private *glamor = glamor_get_egl_screen_private(scrn);
	glamor_fini(screen);

        eglDestroyImageKHR(glamor->display, glamor->root);


	glamor->root = EGL_NO_IMAGE_KHR;

	return TRUE;
}



static Bool
glamor_egl_has_extension(struct glamor_screen_private *glamor, char *extension)
{
  const char *egl_extensions;
  char *pext;
  int  ext_len;
  ext_len = strlen(extension);
 
  egl_extensions = (const char*)eglQueryString(glamor->display, EGL_EXTENSIONS);
  pext = (char*)egl_extensions;
 
  if (pext == NULL || extension == NULL)
    return FALSE;
  while((pext = strstr(pext, extension)) != NULL) {
    if (pext[ext_len] == ' ' || pext[ext_len] == '\0')
      return TRUE;
    pext += ext_len;
  }
  return FALSE;
}


Bool glamor_egl_init(ScrnInfoPtr scrn, int fd)
{
	struct glamor_screen_private *glamor;
	const char *version;
        EGLint config_attribs[] = {
#ifdef GLAMOR_GLES2
          EGL_CONTEXT_CLIENT_VERSION, 2, 
#endif
          EGL_NONE
        };

        glamor_identify(0);
        glamor = calloc(sizeof(*glamor), 1);
        if (xf86GlamorEGLPrivateIndex == -1) 
           xf86GlamorEGLPrivateIndex = xf86AllocateScrnInfoPrivateIndex();      

        scrn->privates[xf86GlamorEGLPrivateIndex].ptr = glamor;

        glamor->fd = fd;
	glamor->display = eglGetDRMDisplayMESA(glamor->fd);
#ifndef GLAMOR_GLES2
	eglBindAPI(EGL_OPENGL_API);
#else
	eglBindAPI(EGL_OPENGL_ES_API);
#endif
	if (!eglInitialize(glamor->display, &glamor->major, &glamor->minor)) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "eglInitialize() failed\n");
		return FALSE;
	}

	version = eglQueryString(glamor->display, EGL_VERSION);
	xf86Msg(X_INFO, "%s: EGL version %s:\n", glamor_name, version);

#define GLAMOR_CHECK_EGL_EXTENSION(EXT)  \
        if (!glamor_egl_has_extension(glamor, "EGL_" #EXT)) {  \
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

	glamor->egl_export_drm_image_mesa = (PFNEGLEXPORTDRMIMAGEMESA)
                                            eglGetProcAddress("eglExportDRMImageMESA");
        glamor->egl_create_image_khr = (PFNEGLCREATEIMAGEKHRPROC)
                                       eglGetProcAddress("eglCreateImageKHR");

	glamor->egl_image_target_texture2d_oes = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)
                                                 eglGetProcAddress("glEGLImageTargetTexture2DOES");

	if (!glamor->egl_create_image_khr 
            || !glamor->egl_export_drm_image_mesa
            || !glamor->egl_image_target_texture2d_oes) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "eglGetProcAddress() failed\n");
		return FALSE;
	}

	glamor->context = eglCreateContext(glamor->display,
					   NULL, EGL_NO_CONTEXT, config_attribs);
	if (glamor->context == EGL_NO_CONTEXT) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "Failed to create EGL context\n");
		return FALSE;
	}

	if (!eglMakeCurrent(glamor->display,
			    EGL_NO_SURFACE, EGL_NO_SURFACE, glamor->context)) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "Failed to make EGL context current\n");
		return FALSE;
	}

        return TRUE;
}

void
glamor_free_egl_screen(int scrnIndex, int flags)
{
	ScrnInfoPtr scrn = xf86Screens[scrnIndex];
	struct glamor_screen_private *glamor = glamor_get_egl_screen_private(scrn);

	if (glamor != NULL)
	{
	  if (!(flags & GLAMOR_EGL_EXTERNAL_BUFFER)) {
            eglMakeCurrent(glamor->display,
                         EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	    eglTerminate(glamor->display);
          }
	  free(glamor);
	}
}

Bool
glamor_gl_dispatch_init(ScreenPtr screen, struct glamor_gl_dispatch *dispatch, int gl_version)
{
	ScrnInfoPtr scrn = xf86Screens[screen->myNum];
	struct glamor_screen_private *glamor_egl = glamor_get_egl_screen_private(scrn);
        if (!glamor_gl_dispatch_init_impl(dispatch, gl_version, eglGetProcAddress))
          return FALSE;
        glamor_egl->dispatch = dispatch; 
        return TRUE;
}
