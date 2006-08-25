/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
**
*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <string.h>
#include <windowstr.h>
#include <os.h>

#include "glxserver.h"
#include "glxutil.h"
#include "glxext.h"

const char GLServerVersion[] = "1.4";
static const char GLServerExtensions[] = 
			"GL_ARB_depth_texture "
			"GL_ARB_draw_buffers "
			"GL_ARB_fragment_program "
			"GL_ARB_fragment_program_shadow "
			"GL_ARB_imaging "
			"GL_ARB_multisample "
			"GL_ARB_multitexture "
			"GL_ARB_occlusion_query "
			"GL_ARB_point_parameters "
			"GL_ARB_point_sprite "
			"GL_ARB_shadow "
			"GL_ARB_shadow_ambient "
			"GL_ARB_texture_border_clamp "
			"GL_ARB_texture_compression "
			"GL_ARB_texture_cube_map "
			"GL_ARB_texture_env_add "
			"GL_ARB_texture_env_combine "
			"GL_ARB_texture_env_crossbar "
			"GL_ARB_texture_env_dot3 "
			"GL_ARB_texture_mirrored_repeat "
			"GL_ARB_texture_non_power_of_two "
			"GL_ARB_transpose_matrix "
			"GL_ARB_vertex_program "
			"GL_ARB_window_pos "
			"GL_EXT_abgr "
			"GL_EXT_bgra "
 			"GL_EXT_blend_color "
			"GL_EXT_blend_equation_separate "
			"GL_EXT_blend_func_separate "
			"GL_EXT_blend_logic_op "
 			"GL_EXT_blend_minmax "
 			"GL_EXT_blend_subtract "
			"GL_EXT_clip_volume_hint "
			"GL_EXT_copy_texture "
			"GL_EXT_draw_range_elements "
			"GL_EXT_fog_coord "
			"GL_EXT_framebuffer_object "
			"GL_EXT_multi_draw_arrays "
			"GL_EXT_packed_pixels "
			"GL_EXT_paletted_texture "
			"GL_EXT_point_parameters "
			"GL_EXT_polygon_offset "
			"GL_EXT_rescale_normal "
			"GL_EXT_secondary_color "
			"GL_EXT_separate_specular_color "
			"GL_EXT_shadow_funcs "
			"GL_EXT_shared_texture_palette "
 			"GL_EXT_stencil_two_side "
			"GL_EXT_stencil_wrap "
			"GL_EXT_subtexture "
			"GL_EXT_texture "
			"GL_EXT_texture3D "
			"GL_EXT_texture_compression_dxt1 "
			"GL_EXT_texture_compression_s3tc "
			"GL_EXT_texture_edge_clamp "
 			"GL_EXT_texture_env_add "
 			"GL_EXT_texture_env_combine "
 			"GL_EXT_texture_env_dot3 "
 			"GL_EXT_texture_filter_ansiotropic "
			"GL_EXT_texture_lod "
 			"GL_EXT_texture_lod_bias "
 			"GL_EXT_texture_mirror_clamp "
			"GL_EXT_texture_object "
			"GL_EXT_texture_rectangle "
			"GL_EXT_vertex_array "
			"GL_3DFX_texture_compression_FXT1 "
			"GL_APPLE_packed_pixels "
			"GL_ATI_draw_buffers "
			"GL_ATI_texture_env_combine3 "
			"GL_ATI_texture_mirror_once "
 			"GL_HP_occlusion_test "
			"GL_IBM_texture_mirrored_repeat "
			"GL_INGR_blend_func_separate "
			"GL_MESA_pack_invert "
			"GL_MESA_ycbcr_texture "
			"GL_NV_blend_square "
			"GL_NV_depth_clamp "
			"GL_NV_fog_distance "
			"GL_NV_fragment_program "
			"GL_NV_fragment_program_option "
			"GL_NV_fragment_program2 "
			"GL_NV_light_max_exponent "
			"GL_NV_multisample_filter_hint "
			"GL_NV_point_sprite "
			"GL_NV_texgen_reflection "
			"GL_NV_texture_compression_vtc "
			"GL_NV_texture_env_combine4 "
			"GL_NV_texture_expand_normal "
			"GL_NV_texture_rectangle "
			"GL_NV_vertex_program "
			"GL_NV_vertex_program1_1 "
			"GL_NV_vertex_program2 "
			"GL_NV_vertex_program2_option "
			"GL_NV_vertex_program3 "
			"GL_OES_compressed_paletted_texture "
			"GL_SGI_color_matrix "
			"GL_SGI_color_table "
			"GL_SGIS_generate_mipmap "
			"GL_SGIS_multisample "
			"GL_SGIS_point_parameters "
			"GL_SGIS_texture_border_clamp "
			"GL_SGIS_texture_edge_clamp "
			"GL_SGIS_texture_lod "
			"GL_SGIX_depth_texture "
			"GL_SGIX_shadow "
			"GL_SGIX_shadow_ambient "
			"GL_SUN_slice_accum "
			;

/*
** We have made the simplifying assuption that the same extensions are 
** supported across all screens in a multi-screen system.
*/
static char GLXServerVendorName[] = "SGI";
static char GLXServerVersion[] = "1.2";
static char GLXServerExtensions[] =
			"GLX_ARB_multisample "
			"GLX_EXT_visual_info "
			"GLX_EXT_visual_rating "
			"GLX_EXT_import_context "
                        "GLX_EXT_texture_from_pixmap "
			"GLX_OML_swap_method "
			"GLX_SGI_make_current_read "
#ifndef __DARWIN__
			"GLX_SGIS_multisample "
                        "GLX_SGIX_hyperpipe "
                        "GLX_SGIX_swap_barrier "
#endif
			"GLX_SGIX_fbconfig "
			"GLX_MESA_copy_sub_buffer "
			;

__GLXscreen **__glXActiveScreens;

__GLXSwapBarrierExtensionFuncs *__glXSwapBarrierFuncs = NULL;
static int __glXNumSwapBarrierFuncs = 0;
__GLXHyperpipeExtensionFuncs *__glXHyperpipeFuncs = NULL;
static int __glXNumHyperpipeFuncs = 0;

__GLXscreen *__glXgetActiveScreen(int num) {
	return __glXActiveScreens[num];
}


/*
** This hook gets called when a window moves or changes size.
*/
static Bool PositionWindow(WindowPtr pWin, int x, int y)
{
    ScreenPtr pScreen;
    __GLXcontext *glxc;
    __GLXdrawable *glxPriv;
    Bool ret;

    /*
    ** Call wrapped position window routine
    */
    pScreen = pWin->drawable.pScreen;
    pScreen->PositionWindow =
	__glXActiveScreens[pScreen->myNum]->WrappedPositionWindow;
    ret = (*pScreen->PositionWindow)(pWin, x, y);
    pScreen->PositionWindow = PositionWindow;

    /*
    ** Tell all contexts rendering into this window that the window size
    ** has changed.
    */
    glxPriv = (__GLXdrawable *) LookupIDByType(pWin->drawable.id,
					       __glXDrawableRes);
    if (glxPriv == NULL) {
	/*
	** This window is not being used by the OpenGL.
	*/
	return ret;
    }

    /*
    ** resize the drawable
    */
    /* first change the drawable size */
    if (glxPriv->resize(glxPriv) == GL_FALSE) {
	/* resize failed! */
	/* XXX: what can we possibly do here? */
	ret = False;
    }

    /* mark contexts as needing resize */

    for (glxc = glxPriv->drawGlxc; glxc; glxc = glxc->nextDrawPriv) {
	glxc->pendingState |= __GLX_PENDING_RESIZE;
    }

    for (glxc = glxPriv->readGlxc; glxc; glxc = glxc->nextReadPriv) {
	glxc->pendingState |= __GLX_PENDING_RESIZE;
    }

    return ret;
}

/*
 * If your DDX driver wants to register support for swap barriers or hyperpipe
 * topology, it should call __glXHyperpipeInit() or __glXSwapBarrierInit()
 * with a dispatch table of functions to handle the requests.   In the XFree86
 * DDX, for example, you would call these near the bottom of the driver's
 * ScreenInit method, after DRI has been initialized.
 *
 * This should be replaced with a better method when we teach the server how
 * to load DRI drivers.
 */

void __glXHyperpipeInit(int screen, __GLXHyperpipeExtensionFuncs *funcs)
{
    if (__glXNumHyperpipeFuncs < screen + 1) {
        __glXHyperpipeFuncs = xrealloc(__glXHyperpipeFuncs,
                                           (screen+1) * sizeof(__GLXHyperpipeExtensionFuncs));
        __glXNumHyperpipeFuncs = screen + 1;
    }

    __glXHyperpipeFuncs[screen].queryHyperpipeNetworkFunc =
        *funcs->queryHyperpipeNetworkFunc;
    __glXHyperpipeFuncs[screen].queryHyperpipeConfigFunc =
        *funcs->queryHyperpipeConfigFunc;
    __glXHyperpipeFuncs[screen].destroyHyperpipeConfigFunc =
        *funcs->destroyHyperpipeConfigFunc;
    __glXHyperpipeFuncs[screen].hyperpipeConfigFunc =
        *funcs->hyperpipeConfigFunc;
}

void __glXSwapBarrierInit(int screen, __GLXSwapBarrierExtensionFuncs *funcs)
{
    if (__glXNumSwapBarrierFuncs < screen + 1) {
        __glXSwapBarrierFuncs = xrealloc(__glXSwapBarrierFuncs,
                                           (screen+1) * sizeof(__GLXSwapBarrierExtensionFuncs));
        __glXNumSwapBarrierFuncs = screen + 1;
    }

    __glXSwapBarrierFuncs[screen].bindSwapBarrierFunc =
        funcs->bindSwapBarrierFunc;
    __glXSwapBarrierFuncs[screen].queryMaxSwapBarriersFunc =
        funcs->queryMaxSwapBarriersFunc;
}

static __GLXprovider *__glXProviderStack;

void GlxPushProvider(__GLXprovider *provider)
{
    provider->next = __glXProviderStack;
    __glXProviderStack = provider;
}

void __glXScreenInit(__GLXscreen *screen, ScreenPtr pScreen)
{
    screen->pScreen       = pScreen;
    screen->GLextensions  = xstrdup(GLServerExtensions);
    screen->GLXvendor     = xstrdup(GLXServerVendorName);
    screen->GLXversion    = xstrdup(GLXServerVersion);
    screen->GLXextensions = xstrdup(GLXServerExtensions);

    screen->WrappedPositionWindow = pScreen->PositionWindow;
    pScreen->PositionWindow = PositionWindow;

    __glXScreenInitVisuals(screen);
}

void
__glXScreenDestroy(__GLXscreen *screen)
{
    xfree(screen->GLXvendor);
    xfree(screen->GLXversion);
    xfree(screen->GLXextensions);
    xfree(screen->GLextensions);
}

void __glXInitScreens(void)
{
    GLint i;
    ScreenPtr pScreen;
    __GLXprovider *p;
    size_t size;

    /*
    ** This alloc has to work or else the server might as well core dump.
    */
    size = screenInfo.numScreens * sizeof(__GLXscreen *);
    __glXActiveScreens = xalloc(size);
    memset(__glXActiveScreens, 0, size);
    
    for (i = 0; i < screenInfo.numScreens; i++) {
	pScreen = screenInfo.screens[i];

	for (p = __glXProviderStack; p != NULL; p = p->next) {
	    __glXActiveScreens[i] = p->screenProbe(pScreen);
	    if (__glXActiveScreens[i] != NULL) {
		LogMessage(X_INFO,
			   "GLX: Initialized %s GL provider for screen %d\n",
			   p->name, i);
	        break;
	    }
	}
    }
}

void __glXResetScreens(void)
{
  int i;

  for (i = 0; i < screenInfo.numScreens; i++)
      if (__glXActiveScreens[i])
	  __glXActiveScreens[i]->destroy(__glXActiveScreens[i]);

    xfree(__glXActiveScreens);
    xfree(__glXHyperpipeFuncs);
    xfree(__glXSwapBarrierFuncs);
    __glXNumHyperpipeFuncs = 0;
    __glXNumSwapBarrierFuncs = 0;
    __glXHyperpipeFuncs = NULL;
    __glXSwapBarrierFuncs = NULL;
    __glXActiveScreens = NULL;
}
