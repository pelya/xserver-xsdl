/*
 * Copyright © 2009 Intel Corporation
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
 *    Junyan He <junyan.he@linux.intel.com>
 *
 */

/** @file glamor_trapezoid.c
 *
 * Trapezoid acceleration implementation
 */

#include "glamor_priv.h"

#ifdef RENDER
#include "mipict.h"
#include "fbpict.h"

#ifdef GLAMOR_TRAPEZOID_SHADER
void
glamor_init_trapezoid_shader(ScreenPtr screen)
{
	glamor_screen_private *glamor_priv;
	glamor_gl_dispatch *dispatch;
	GLint fs_prog, vs_prog;

	const char *trapezoid_vs =
	    GLAMOR_DEFAULT_PRECISION
	    "attribute vec4 v_position;\n"
	    "attribute vec4 v_texcoord;\n"
	    "varying vec2 source_texture;\n"
	    "\n"
	    "void main()\n"
	    "{\n"
	    "    gl_Position = v_position;\n"
	    "    source_texture = v_texcoord.xy;\n"
	    "}\n";

	/*
	 * Because some GL fill function do not support the MultSample
	 * anti-alias, we need to do the MSAA here. This manner like
	 * pixman, will caculate the value of area in trapezoid dividing
	 * the totol area for each pixel, as follow:
	     |
	 ----+------------------------------------------------------>
	     |
	     |              -------------
	     |             /             \
	     |            /               \
	     |           /                 \
	     |          /              +----------------+
	     |         /               |.....\          |
	     |        /                |......\         |
	     |       /                 |.......\        |
	     |      /                  |........\       |
	     |     /-------------------+---------\      |
	     |                         |                |
	     |                         |                |
	     |                         +----------------+
	     |
	    \|/

	 */
	const char *trapezoid_fs =
	    GLAMOR_DEFAULT_PRECISION
	    "varying vec2 source_texture;  \n"
	    "uniform float x_per_pix;  \n"
	    "uniform float y_per_pix;  \n"
	    "uniform float trap_top;  \n"
	    "uniform float trap_bottom;  \n"
	    "uniform float trap_left_x;  \n"
	    "uniform float trap_left_y;  \n"
	    "uniform float trap_left_slope;  \n"
	    "uniform int trap_left_vertical;  \n"
	    "uniform float trap_right_x;  \n"
	    "uniform float trap_right_y;  \n"
	    "uniform float trap_right_slope;  \n"
	    "uniform int trap_right_vertical;  \n"
	    "\n"
	    "float get_alpha_val() \n"
	    "{  \n"
	    "    float x_up_cut_left;  \n"
	    "    float x_bottom_cut_left;  \n"
	    "    float x_up_cut_right;  \n"
	    "    float x_bottom_cut_right;  \n"
	    "    \n"
	    "    if(trap_left_vertical == 1) {  \n"
	    "        x_up_cut_left = trap_left_x;  \n"
	    "        x_bottom_cut_left = trap_left_x;  \n"
	    "    } else {  \n"
	    "        x_up_cut_left = trap_left_x  \n"
	    "            + (source_texture.y - y_per_pix/2.0 - trap_left_y)  \n"
	    "              / trap_left_slope;  \n"
	    "        x_bottom_cut_left = trap_left_x  \n"
	    "            + (source_texture.y + y_per_pix/2.0 - trap_left_y)  \n"
	    "              / trap_left_slope;  \n"
	    "    }  \n"
	    "    \n"
	    "    if(trap_right_vertical == 1) {  \n"
	    "        x_up_cut_right = trap_right_x;  \n"
	    "        x_bottom_cut_right = trap_right_x;  \n"
	    "    } else {  \n"
	    "        x_up_cut_right = trap_right_x  \n"
	    "            + (source_texture.y - y_per_pix/2.0 - trap_right_y)  \n"
	    "              / trap_right_slope;  \n"
	    "        x_bottom_cut_right = trap_right_x  \n"
	    "            + (source_texture.y + y_per_pix/2.0 - trap_right_y)  \n"
	    "              / trap_right_slope;  \n"
	    "    }  \n"
	    "    \n"
	    "    if((x_up_cut_left <= source_texture.x - x_per_pix/2.0) &&  \n"
	    "          (x_bottom_cut_left <= source_texture.x - x_per_pix/2.0) &&  \n"
	    "          (x_up_cut_right >= source_texture.x + x_per_pix/2.0) &&  \n"
	    "          (x_bottom_cut_right >= source_texture.x + x_per_pix/2.0) &&  \n"
	    "          (trap_top <= source_texture.y - y_per_pix/2.0) &&  \n"
	    "          (trap_bottom >= source_texture.y + y_per_pix/2.0)) {  \n"
	    //       The complete inside case.
	    "        return 1.0;  \n"
	    "    } else if((trap_top > source_texture.y + y_per_pix/2.0) ||  \n"
	    "                (trap_bottom < source_texture.y - y_per_pix/2.0)) {  \n"
	    //       The complete outside. Above the top or Below the bottom.
	    "        return 0.0;  \n"
	    "    } else {  \n"
	    "        if((x_up_cut_right < source_texture.x - x_per_pix/2.0 &&  \n"
	    "                   x_bottom_cut_right < source_texture.x - x_per_pix/2.0)  \n"
	    "            || (x_up_cut_left > source_texture.x + x_per_pix/2.0  &&  \n"
	    "                   x_bottom_cut_left > source_texture.x + x_per_pix/2.0)) {  \n"
	    //            The complete outside. At Left or Right of the trapezoide.
	    "             return 0.0;  \n"
	    "        }  \n"
	    "    }  \n"
	    //   Get here, the pix is partly inside the trapezoid.
	    "    {  \n"
	    "        float percent = 0.0;  \n"
	    "        float up = (source_texture.y - y_per_pix/2.0) >= trap_top ?  \n"
	    "                       (source_texture.y - y_per_pix/2.0) : trap_top;  \n"
	    "        float bottom = (source_texture.y + y_per_pix/2.0) <= trap_bottom ?  \n"
	    "                       (source_texture.y + y_per_pix/2.0) : trap_bottom;  \n"
	    "        float left = source_texture.x - x_per_pix/2.0;  \n"
	    "        float right = source_texture.x + x_per_pix/2.0;  \n"
	    "        \n"
	    "        percent = (bottom - up) / y_per_pix;  \n"
	    "        \n"
	    "	     if(trap_left_vertical == 1) {  \n"
	    "            if(trap_left_x > source_texture.x - x_per_pix/2.0 &&  \n"
	    "                     trap_left_x < source_texture.x + x_per_pix/2.0)  \n"
	    "                left = trap_left_x;  \n"
	    "        }  \n"
	    "	     if(trap_right_vertical == 1) {  \n"
	    "            if(trap_right_x > source_texture.x - x_per_pix/2.0 &&  \n"
	    "                     trap_right_x < source_texture.x + x_per_pix/2.0)  \n"
	    "                right = trap_right_x;  \n"
	    "        }  \n"
	    "        if((up >= bottom) || (left >= right))  \n"
	    "            return 0.0;  \n"
	    "        \n"
	    "        percent = percent * ((right - left)/x_per_pix);  \n"
	    "        if(trap_left_vertical == 1 && trap_right_vertical == 1)  \n"
	    "            return percent;  \n"
	    "        \n"
	    "        if(trap_left_vertical != 1) {  \n"
	    "            float area;  \n"
	    //           the slope should never be 0.0 here
	    "            float up_x = trap_left_x + (up - trap_left_y)/trap_left_slope;  \n"
	    "            float bottom_x = trap_left_x + (bottom - trap_left_y)/trap_left_slope;  \n"
	    "            if(trap_left_slope < 0.0 && up_x > left) {  \n"
	    /*   case 1
	         |
	     ----+------------------------------------->
	         |                 /
	         |                /
	         |           +---/--------+
	         |           |  /.........|
	         |           | /..........|
	         |           |/...........|
	         |           /............|
	         |          /|............|
	         |           +------------+
	         |
	        \|/
	    */
	    "                float left_y = trap_left_y + trap_left_slope*(left - trap_left_x);  \n"
	    "                if((up_x > left) && (left_y > up)) {  \n"
	    "                    area = 0.5 * (up_x - left) * (left_y - up);  \n"
	    "                    if(up_x > right) {  \n"
	    "                        float right_y = trap_left_y  \n"
	    "                                  + trap_left_slope*(right - trap_left_x);  \n"
	    "                        area = area - 0.5 * (up_x - right) * (right_y - up);  \n"
	    "                    }  \n"
	    "                    if(left_y > bottom) {  \n"
	    "                        area = area - 0.5 * (bottom_x - left) * (left_y - bottom);  \n"
	    "                    }  \n"
	    "                } else {  \n"
	    "                    area = 0.0;  \n"
	    "                }  \n"
	    "                percent = percent * (1.0 - (area/((right-left)*(bottom-up))));  \n"
	    "            } else if(trap_left_slope > 0.0 && bottom_x > left) {  \n"
	    /*   case 2
	         |
	     ----+------------------------------------->
	         |          \
	         |           \
	         |           +\-----------+
	         |           | \..........|
	         |           |  \.........|
	         |           |   \........|
	         |           |    \.......|
	         |           |     \......|
	         |           +------\-----+
	         |                   \
	         |                    \
	        \|/
	    */
	    "                float right_y = trap_left_y + trap_left_slope*(right - trap_left_x);  \n"
	    "                if((up_x < right) && (right_y > up)) {  \n"
	    "                    area = 0.5 * (right - up_x) * (right_y - up);  \n"
	    "                    if(up_x < left) {  \n"
	    "                        float left_y = trap_left_y  \n"
	    "                                  + trap_left_slope*(left - trap_left_x);  \n"
	    "                        area = area - 0.5 * (left - up_x) * (left_y - up);  \n"
	    "                    }  \n"
	    "                    if(right_y > bottom) {  \n"
	    "                        area = area - 0.5 * (right - bottom_x) * (right_y - bottom);  \n"
	    "                    }  \n"
	    "                } else {  \n"
	    "                    area = 0.0;  \n"
	    "                }  \n"
	    "                percent = percent * (area/((right-left)*(bottom-up)));  \n"
	    "            }  \n"
	    "        }  \n"
	    "        \n"
	    "        if(trap_right_vertical != 1) {  \n"
	    "            float area;  \n"
	    //           the slope should never be 0.0 here
	    "            float up_x = trap_right_x + (up - trap_right_y)/trap_right_slope;  \n"
	    "            float bottom_x = trap_right_x + (bottom - trap_right_y)/trap_right_slope;  \n"
	    "            if(trap_right_slope < 0.0 && bottom_x < right) {  \n"
	    /*   case 3
	         |
	     ----+------------------------------------->
	         |                     /
	         |           +--------/---+
	         |           |......./    |
	         |           |....../     |
	         |           |...../      |
	         |           |..../       |
	         |           |.../        |
	         |           +--/---------+
	         |             /
	         |
	        \|/
	    */
	    "                float left_y = trap_right_y + trap_right_slope*(left - trap_right_x);  \n"
	    "                if((up_x > left) && (left_y > up)) {  \n"
	    "                    area = 0.5 * (up_x - left) * (left_y - up);  \n"
	    "                    if(up_x > right) {  \n"
	    "                        float right_y = trap_right_y  \n"
	    "                                  + trap_right_slope*(right - trap_right_x);  \n"
	    "                        area = area - 0.5 * (up_x - right) * (right_y - up);  \n"
	    "                    }  \n"
	    "                    if(left_y > bottom) {  \n"
	    "                        area = area - 0.5 * (bottom_x - left) * (left_y - bottom);  \n"
	    "                    }  \n"
	    "                } else {  \n"
	    "                    area = 0.0;  \n"
	    "                }  \n"
	    "                percent = percent * (area/((right-left)*(bottom-up)));  \n"
	    "            } else if(trap_right_slope > 0.0 && up_x < right) {  \n"
	    /*   case 4
	         |
	     ----+------------------------------------->
	         |                   \
	         |           +--------\---+
	         |           |.........\  |
	         |           |..........\ |
	         |           |...........\|
	         |           |............\
	         |           |............|\
	         |           +------------+ \
	         |                           \
	         |
	        \|/
	    */
	    "                float right_y = trap_right_y + trap_right_slope*(right - trap_right_x);  \n"
	    "                if((up_x < right) && (right_y > up)) {  \n"
	    "                    area = 0.5 * (right - up_x) * (right_y - up);  \n"
	    "                    if(up_x < left) {  \n"
	    "                        float left_y = trap_right_y  \n"
	    "                                  + trap_right_slope*(left - trap_right_x);  \n"
	    "                        area = area - 0.5 * (left - up_x) * (left_y - up);  \n"
	    "                    }  \n"
	    "                    if(right_y > bottom) {  \n"
	    "                        area = area - 0.5 * (right - bottom_x) * (right_y - bottom);  \n"
	    "                    }  \n"
	    "                } else {  \n"
	    "                    area = 0.0;  \n"
	    "                }  \n"
	    "                percent = percent * (1.0 - (area/((right-left)*(bottom-up))));  \n"
	    "            }  \n"
	    "        }  \n"
	    "        \n"
	    "        return percent;  \n"
	    "    }  \n"
	    "}  \n"
	    "\n"
	    "void main()  \n"
	    "{  \n"
	    "    float alpha_val = get_alpha_val();  \n"
	    "    gl_FragColor = vec4(0.0, 0.0, 0.0, alpha_val);  \n"
	    "}\n";

	glamor_priv = glamor_get_screen_private(screen);
	dispatch = glamor_get_dispatch(glamor_priv);

	glamor_priv->trapezoid_prog = dispatch->glCreateProgram();

	vs_prog = glamor_compile_glsl_prog(dispatch,
	                  GL_VERTEX_SHADER, trapezoid_vs);
	fs_prog = glamor_compile_glsl_prog(dispatch,
	                  GL_FRAGMENT_SHADER, trapezoid_fs);

	dispatch->glAttachShader(glamor_priv->trapezoid_prog, vs_prog);
	dispatch->glAttachShader(glamor_priv->trapezoid_prog, fs_prog);

	dispatch->glBindAttribLocation(glamor_priv->trapezoid_prog,
	                               GLAMOR_VERTEX_POS, "v_positionsition");
	dispatch->glBindAttribLocation(glamor_priv->trapezoid_prog,
	                               GLAMOR_VERTEX_SOURCE, "v_texcoord");

	glamor_link_glsl_prog(dispatch, glamor_priv->trapezoid_prog);

	dispatch->glUseProgram(0);

	glamor_put_dispatch(glamor_priv);
}

void
glamor_fini_trapezoid_shader(ScreenPtr screen)
{
	glamor_screen_private *glamor_priv;
	glamor_gl_dispatch *dispatch;

	glamor_priv = glamor_get_screen_private(screen);
	dispatch = glamor_get_dispatch(glamor_priv);
	dispatch->glDeleteProgram(glamor_priv->trapezoid_prog);
	glamor_put_dispatch(glamor_priv);
}

static Bool
_glamor_generate_trapezoid_with_shader(ScreenPtr screen, PicturePtr picture,
        xTrapezoid * traps, int ntrap, BoxRec *bounds)
{
	glamor_screen_private *glamor_priv;
	glamor_gl_dispatch *dispatch;
	glamor_pixmap_private *pixmap_priv;
	PixmapPtr pixmap = NULL;
	GLint x_per_pix_uniform_location;
	GLint y_per_pix_uniform_location;
	GLint trap_top_uniform_location;
	GLint trap_bottom_uniform_location;
	GLint trap_left_x_uniform_location;
	GLint trap_left_y_uniform_location;
	GLint trap_left_slope_uniform_location;
	GLint trap_right_x_uniform_location;
	GLint trap_right_y_uniform_location;
	GLint trap_right_slope_uniform_location;
	GLint trap_left_vertical_uniform_location;
	GLint trap_right_vertical_uniform_location;
	GLint trapezoid_prog;
	float width, height;
	xFixed width_fix, height_fix;
	GLfloat xscale, yscale;
	float left_slope, right_slope;
	xTrapezoid *ptrap;
	BoxRec one_trap_bound;
	float vertices[8];
	float tex_vertices[8];
	int i;

	glamor_priv = glamor_get_screen_private(screen);
	trapezoid_prog = glamor_priv->trapezoid_prog;

	pixmap = glamor_get_drawable_pixmap(picture->pDrawable);
	pixmap_priv = glamor_get_pixmap_private(pixmap);

	if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv)) { /* should always have here. */
		DEBUGF("GLAMOR_PIXMAP_PRIV_HAS_FBO check failed, fallback\n");
		return FALSE;
	}

	/* First, clear all to zero */
	if (!glamor_solid(pixmap, 0, 0, pixmap_priv->base.pixmap->drawable.width,
	                  pixmap_priv->base.pixmap->drawable.height,
	                  GXclear, 0xFFFFFFFF, 0)) {
		DEBUGF("glamor_solid failed, fallback\n");
		return FALSE;
	}

	dispatch = glamor_get_dispatch(glamor_priv);

	/* Bind all the uniform vars .*/
	x_per_pix_uniform_location =
	    dispatch->glGetUniformLocation(trapezoid_prog, "x_per_pix");
	y_per_pix_uniform_location =
	    dispatch->glGetUniformLocation(trapezoid_prog, "y_per_pix");
	trap_top_uniform_location =
	    dispatch->glGetUniformLocation(trapezoid_prog, "trap_top");
	trap_bottom_uniform_location =
	    dispatch->glGetUniformLocation(trapezoid_prog, "trap_bottom");
	trap_left_x_uniform_location =
	    dispatch->glGetUniformLocation(trapezoid_prog, "trap_left_x");
	trap_left_y_uniform_location =
	    dispatch->glGetUniformLocation(trapezoid_prog, "trap_left_y");
	trap_left_slope_uniform_location =
	    dispatch->glGetUniformLocation(trapezoid_prog, "trap_left_slope");
	trap_left_vertical_uniform_location =
	    dispatch->glGetUniformLocation(trapezoid_prog, "trap_left_vertical");
	trap_right_x_uniform_location =
	    dispatch->glGetUniformLocation(trapezoid_prog, "trap_right_x");
	trap_right_y_uniform_location =
	    dispatch->glGetUniformLocation(trapezoid_prog, "trap_right_y");
	trap_right_slope_uniform_location =
	    dispatch->glGetUniformLocation(trapezoid_prog, "trap_right_slope");
	trap_right_vertical_uniform_location =
	    dispatch->glGetUniformLocation(trapezoid_prog, "trap_right_vertical");

	glamor_set_destination_pixmap_priv_nc(pixmap_priv);

	pixmap_priv_get_dest_scale(pixmap_priv, (&xscale), (&yscale));

	width = (float)(bounds->x2 - bounds->x1);
	height = (float)(bounds->y2 - bounds->y1);
	width_fix = IntToxFixed((bounds->x2 - bounds->x1));
	height_fix = IntToxFixed((bounds->y2 - bounds->y1));

	dispatch->glBindBuffer(GL_ARRAY_BUFFER, 0);
	dispatch->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);

	/* Now draw the Trapezoid mask. */
	dispatch->glUseProgram(trapezoid_prog);
	dispatch->glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_FLOAT,
	        GL_FALSE, 0, vertices);
	dispatch->glVertexAttribPointer(GLAMOR_VERTEX_SOURCE, 2, GL_FLOAT,
	        GL_FALSE, 0, tex_vertices);
	dispatch->glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
	dispatch->glEnableVertexAttribArray(GLAMOR_VERTEX_SOURCE);

	dispatch->glEnable(GL_BLEND);
	dispatch->glBlendFunc(GL_ONE, GL_ONE);

	for (i = 0; i < ntrap; i++) {
		ptrap = traps + i;

		DEBUGF("--- The parameter of xTrapezoid is:\ntop: %d  0x%x\tbottom: %d  0x%x\n"
		       "left:  p1 (%d   0x%x, %d   0x%x)\tp2 (%d   0x%x, %d   0x%x)\n"
		       "right: p1 (%d   0x%x, %d   0x%x)\tp2 (%d   0x%x, %d   0x%x)\n",
		       xFixedToInt(ptrap->top), ptrap->top,
		       xFixedToInt(ptrap->bottom), ptrap->bottom,
		       xFixedToInt(ptrap->left.p1.x), ptrap->left.p1.x,
		       xFixedToInt(ptrap->left.p1.y), ptrap->left.p1.y,
		       xFixedToInt(ptrap->left.p2.x), ptrap->left.p2.x,
		       xFixedToInt(ptrap->left.p2.y), ptrap->left.p2.y,
		       xFixedToInt(ptrap->right.p1.x), ptrap->right.p1.x,
		       xFixedToInt(ptrap->right.p1.y), ptrap->right.p1.y,
		       xFixedToInt(ptrap->right.p2.x), ptrap->right.p2.x,
		       xFixedToInt(ptrap->right.p2.y), ptrap->right.p2.y);

		miTrapezoidBounds(1, ptrap, &one_trap_bound);
		glamor_set_tcoords_tri_strip((pixmap_priv->base.pixmap->drawable.width),
		                             (pixmap_priv->base.pixmap->drawable.height),
		                             (one_trap_bound.x1),
		                             (one_trap_bound.y1),
		                             (one_trap_bound.x2),
		                             (one_trap_bound.y2),
		                             glamor_priv->yInverted, tex_vertices);

		/* Need to rebase. */
		one_trap_bound.x1 -= bounds->x1;
		one_trap_bound.x2 -= bounds->x1;
		one_trap_bound.y1 -= bounds->y1;
		one_trap_bound.y2 -= bounds->y1;
		glamor_set_normalize_vcoords_tri_strip(xscale, yscale,
		        one_trap_bound.x1, one_trap_bound.y1,
		        one_trap_bound.x2, one_trap_bound.y2,
		        glamor_priv->yInverted, vertices);

		DEBUGF("vertices --> leftup : %f X %f, rightup: %f X %f,"
		       "rightbottom: %f X %f, leftbottom : %f X %f\n",
		       vertices[0], vertices[1], vertices[2], vertices[3],
		       vertices[4], vertices[5], vertices[6], vertices[7]);
		DEBUGF("tex_vertices --> leftup : %f X %f, rightup: %f X %f,"
		       "rightbottom: %f X %f, leftbottom : %f X %f\n",
		       tex_vertices[0], tex_vertices[1], tex_vertices[2], tex_vertices[3],
		       tex_vertices[4], tex_vertices[5], tex_vertices[6], tex_vertices[7]);

		if (ptrap->left.p1.x == ptrap->left.p2.x) {
			left_slope = 0.0;
			dispatch->glUniform1i(trap_left_vertical_uniform_location, 1);
		} else {
			left_slope = ((float)(ptrap->left.p1.y - ptrap->left.p2.y))
			             / ((float)(ptrap->left.p1.x - ptrap->left.p2.x));
			dispatch->glUniform1i(trap_left_vertical_uniform_location, 0);
		}
		dispatch->glUniform1f(trap_left_slope_uniform_location, left_slope);

		if (ptrap->right.p1.x == ptrap->right.p2.x) {
			right_slope = 0.0;
			dispatch->glUniform1i(trap_right_vertical_uniform_location, 1);
		} else {
			right_slope = ((float)(ptrap->right.p1.y - ptrap->right.p2.y))
			              / ((float)(ptrap->right.p1.x - ptrap->right.p2.x));
			dispatch->glUniform1i(trap_right_vertical_uniform_location, 0);
		}
		dispatch->glUniform1f(trap_right_slope_uniform_location, right_slope);

		dispatch->glUniform1f(x_per_pix_uniform_location,
		        ((float)width_fix) / (65536 * width));
		dispatch->glUniform1f(y_per_pix_uniform_location,
		        ((float)height_fix) / (65536 * height));

		dispatch->glUniform1f(trap_top_uniform_location,
		        ((float)ptrap->top) / 65536);
		dispatch->glUniform1f(trap_bottom_uniform_location,
		        ((float)ptrap->bottom) / 65536);

		dispatch->glUniform1f(trap_left_x_uniform_location,
		        ((float)ptrap->left.p1.x) / 65536);
		dispatch->glUniform1f(trap_left_y_uniform_location,
		        ((float)ptrap->left.p1.y) / 65536);
		dispatch->glUniform1f(trap_right_x_uniform_location,
		        ((float)ptrap->right.p1.x) / 65536);
		dispatch->glUniform1f(trap_right_y_uniform_location,
		        ((float)ptrap->right.p1.y) / 65536);

		DEBUGF("x_per_pix = %f, y_per_pix = %f, trap_top = %f, trap_bottom = %f, "
		       "trap_left_x = %f, trap_left_y = %f, left_slope = %f, "
		       "trap_right_x = %f, trap_right_y = %f, right_slope = %f\n",
		       ((float)width_fix) / (65536*width), ((float)height_fix) / (65536*height),
		       ((float)ptrap->top) / 65536, ((float)ptrap->bottom) / 65536,
		       ((float)ptrap->left.p1.x) / 65536, ((float)ptrap->left.p1.y) / 65536,
		       left_slope,
		       ((float)ptrap->right.p1.x) / 65536, ((float)ptrap->right.p1.y) / 65536,
		       right_slope);

		/* Now rendering. */
		dispatch->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	dispatch->glBindBuffer(GL_ARRAY_BUFFER, 0);
	dispatch->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	dispatch->glBlendFunc(GL_ONE, GL_ZERO);
	dispatch->glDisable(GL_BLEND);
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
	dispatch->glUseProgram(0);
	glamor_put_dispatch(glamor_priv);
	return TRUE;
}

#endif                           /*GLAMOR_TRAPEZOID_SHADER */

/**
 * Creates an appropriate picture for temp mask use.
 */
static PicturePtr
glamor_create_mask_picture(ScreenPtr screen,
        PicturePtr dst,
        PictFormatPtr pict_format,
        CARD16 width, CARD16 height, int gpu)
{
	PixmapPtr pixmap;
	PicturePtr picture;
	int error;

	if (!pict_format) {
		if (dst->polyEdge == PolyEdgeSharp)
			pict_format =
			    PictureMatchFormat(screen, 1, PICT_a1);
		else
			pict_format =
			    PictureMatchFormat(screen, 8, PICT_a8);
		if (!pict_format)
			return 0;
	}

	if (gpu) {
		pixmap = glamor_create_pixmap(screen, width, height,
		         pict_format->depth, 0);
	} else {
		pixmap = glamor_create_pixmap(screen, 0, 0,
		         pict_format->depth,
		         GLAMOR_CREATE_PIXMAP_CPU);
	}

	if (!pixmap)
		return 0;
	picture = CreatePicture(0, &pixmap->drawable, pict_format,
	                        0, 0, serverClient, &error);
	glamor_destroy_pixmap(pixmap);
	return picture;
}

/**
 * glamor_trapezoids will first try to create a trapezoid mask using shader,
 * if failed, miTrapezoids will generate trapezoid mask accumulating in
 * system memory.
 */
static Bool
_glamor_trapezoids(CARD8 op,
                   PicturePtr src, PicturePtr dst,
                   PictFormatPtr mask_format, INT16 x_src, INT16 y_src,
                   int ntrap, xTrapezoid * traps, Bool fallback)
{
	ScreenPtr screen = dst->pDrawable->pScreen;
	BoxRec bounds;
	PicturePtr picture;
	INT16 x_dst, y_dst;
	INT16 x_rel, y_rel;
	int width, height, stride;
	PixmapPtr pixmap;
	pixman_image_t *image = NULL;
	int ret = 0;

	/* If a mask format wasn't provided, we get to choose, but behavior should
	 * be as if there was no temporary mask the traps were accumulated into.
	 */
	if (!mask_format) {
		if (dst->polyEdge == PolyEdgeSharp)
			mask_format =
			    PictureMatchFormat(screen, 1, PICT_a1);
		else
			mask_format =
			    PictureMatchFormat(screen, 8, PICT_a8);
		for (; ntrap; ntrap--, traps++)
			glamor_trapezoids(op, src, dst, mask_format, x_src,
			                  y_src, 1, traps);
		return TRUE;
	}

	miTrapezoidBounds(ntrap, traps, &bounds);
	DEBUGF("The bounds for all traps is: bounds.x1 = %d, bounds.x2 = %d, "
	       "bounds.y1 = %d, bounds.y2 = %d\n", bounds.x1, bounds.x2,
	       bounds.y1, bounds.y2);

	if (bounds.y1 >= bounds.y2 || bounds.x1 >= bounds.x2)
		return TRUE;

	x_dst = traps[0].left.p1.x >> 16;
	y_dst = traps[0].left.p1.y >> 16;

	width = bounds.x2 - bounds.x1;
	height = bounds.y2 - bounds.y1;
	stride = PixmapBytePad(width, mask_format->depth);

#ifdef GLAMOR_TRAPEZOID_SHADER
	picture = glamor_create_mask_picture(screen, dst, mask_format,
	          width, height, 1);
	if (!picture)
		return TRUE;

	ret = _glamor_generate_trapezoid_with_shader(screen, picture, traps, ntrap, &bounds);

	if (!ret)
		FreePicture(picture, 0);
#endif

	if (!ret) {
		DEBUGF("Fallback to sw rasterize of trapezoid\n");

		picture = glamor_create_mask_picture(screen, dst, mask_format,
		          width, height, 0);
		if (!picture)
			return TRUE;

		image = pixman_image_create_bits(picture->format,
		        width, height, NULL, stride);
		if (!image) {
			FreePicture(picture, 0);
			return TRUE;
		}

		for (; ntrap; ntrap--, traps++)
			pixman_rasterize_trapezoid(image,
			        (pixman_trapezoid_t *) traps,
			        -bounds.x1, -bounds.y1);

		pixmap = glamor_get_drawable_pixmap(picture->pDrawable);

		screen->ModifyPixmapHeader(pixmap, width, height,
		        mask_format->depth,
		        BitsPerPixel(mask_format->depth),
		        PixmapBytePad(width,
		                      mask_format->depth),
		        pixman_image_get_data(image));
	}

	x_rel = bounds.x1 + x_src - x_dst;
	y_rel = bounds.y1 + y_src - y_dst;
	DEBUGF("x_src = %d, y_src = %d, x_dst = %d, y_dst = %d, "
	       "x_rel = %d, y_rel = %d\n", x_src, y_src, x_dst,
	       y_dst, x_rel, y_rel);

	CompositePicture(op, src, picture, dst,
	                 x_rel, y_rel,
	                 0, 0,
	                 bounds.x1, bounds.y1,
	                 bounds.x2 - bounds.x1, bounds.y2 - bounds.y1);

	if (image)
		pixman_image_unref(image);

	FreePicture(picture, 0);
	return TRUE;
}

void
glamor_trapezoids(CARD8 op,
                  PicturePtr src, PicturePtr dst,
                  PictFormatPtr mask_format, INT16 x_src, INT16 y_src,
                  int ntrap, xTrapezoid * traps)
{
	DEBUGF("x_src = %d, y_src = %d, ntrap = %d\n", x_src, y_src, ntrap);

	_glamor_trapezoids(op, src, dst, mask_format, x_src,
	                   y_src, ntrap, traps, TRUE);
}

Bool
glamor_trapezoids_nf(CARD8 op,
        PicturePtr src, PicturePtr dst,
        PictFormatPtr mask_format, INT16 x_src, INT16 y_src,
        int ntrap, xTrapezoid * traps)
{
	DEBUGF("x_src = %d, y_src = %d, ntrap = %d\n", x_src, y_src, ntrap);

	return _glamor_trapezoids(op, src, dst, mask_format, x_src,
	        y_src, ntrap, traps, FALSE);
}

#endif				/* RENDER */

