#ifndef GLAMOR_DDX_H
#define GLAMOR_DDX_H

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

#include "glamor_gl_dispatch.h"
struct glamor_ddx_screen_private {
	EGLDisplay display;
	EGLContext context;
	EGLImageKHR root;
	EGLint major, minor;

	CreateScreenResourcesProcPtr CreateScreenResources;
	CloseScreenProcPtr CloseScreen;
	int fd;
	int cpp;

	PFNEGLCREATEDRMIMAGEMESA egl_create_drm_image_mesa;
	PFNEGLEXPORTDRMIMAGEMESA egl_export_drm_image_mesa;
	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC egl_image_target_texture2d_oes;
        struct glamor_gl_dispatch *dispatch;
};

inline struct glamor_ddx_screen_private *
glamor_ddx_get_screen_private(ScrnInfoPtr scrn)
{
	return (struct glamor_ddx_screen_private *) (scrn->driverPrivate);
}

Bool glamor_resize(ScrnInfoPtr scrn, int width, int height);
void glamor_frontbuffer_handle(ScrnInfoPtr scrn,
			       uint32_t *handle, uint32_t *pitch);
Bool glamor_load_cursor(ScrnInfoPtr scrn,
			CARD32 *image, int width, int height);

void glamor_cursor_handle(ScrnInfoPtr scrn, EGLImageKHR image, uint32_t *handle, uint32_t *pitch);
EGLImageKHR glamor_create_cursor_argb(ScrnInfoPtr scrn, int width, int height);
void glamor_destroy_cursor(ScrnInfoPtr scrn, EGLImageKHR cursor);

Bool drmmode_pre_init(ScrnInfoPtr scrn, int fd, int cpp);
void drmmode_closefb(ScrnInfoPtr scrn);

#endif
