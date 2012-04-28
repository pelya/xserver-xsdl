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
 *    Eric Anholt <eric@anholt.net>
 *    Zhigang Gong <zhigang.gong@linux.intel.com>
 *    Junyan He <junyan.he@linux.intel.com>
 *
 */

/** @file glamor_render.c
 *
 * Render acceleration implementation
 */

#include "glamor_priv.h"

#ifdef RENDER
#include "mipict.h"
#include "fbpict.h"

struct shader_key {
	enum shader_source source;
	enum shader_mask mask;
	enum shader_in in;
};

struct blendinfo {
	Bool dest_alpha;
	Bool source_alpha;
	GLenum source_blend;
	GLenum dest_blend;
};

static struct blendinfo composite_op_info[] = {
	[PictOpClear] = {0, 0, GL_ZERO, GL_ZERO},
	[PictOpSrc] = {0, 0, GL_ONE, GL_ZERO},
	[PictOpDst] = {0, 0, GL_ZERO, GL_ONE},
	[PictOpOver] = {0, 1, GL_ONE, GL_ONE_MINUS_SRC_ALPHA},
	[PictOpOverReverse] = {1, 0, GL_ONE_MINUS_DST_ALPHA, GL_ONE},
	[PictOpIn] = {1, 0, GL_DST_ALPHA, GL_ZERO},
	[PictOpInReverse] = {0, 1, GL_ZERO, GL_SRC_ALPHA},
	[PictOpOut] = {1, 0, GL_ONE_MINUS_DST_ALPHA, GL_ZERO},
	[PictOpOutReverse] = {0, 1, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA},
	[PictOpAtop] = {1, 1, GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA},
	[PictOpAtopReverse] = {1, 1, GL_ONE_MINUS_DST_ALPHA, GL_SRC_ALPHA},
	[PictOpXor] =
	    {1, 1, GL_ONE_MINUS_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA},
	[PictOpAdd] = {0, 0, GL_ONE, GL_ONE},
};
#define RepeatFix			10
static GLuint
glamor_create_composite_fs(glamor_gl_dispatch * dispatch,
			   struct shader_key *key)
{
	const char *repeat_define =
	    "#define RepeatNone               	      0\n"
	    "#define RepeatNormal                     1\n"
	    "#define RepeatPad                        2\n"
	    "#define RepeatReflect                    3\n"
	    "#define RepeatFix		      	      10\n"
	    "uniform int 			source_repeat_mode;\n"
	    "uniform int 			mask_repeat_mode;\n";
	const char *relocate_texture =
	    GLAMOR_DEFAULT_PRECISION
	    "vec2 rel_tex_coord(vec2 texture, vec2 wh, int repeat) \n"
	    "{\n"
	    "   vec2 rel_tex; \n"
	    "   rel_tex = texture * wh; \n"
	    "	if (repeat == RepeatNone)\n"
	    "		return rel_tex; \n"
	    "   else if (repeat == RepeatNormal) \n"
	    "   	rel_tex = floor(rel_tex) + (fract(rel_tex) / wh); \n"
	    "   else if(repeat == RepeatPad) { \n"
	    "           if (rel_tex.x > 1.0) rel_tex.x = 1.0;		  \n"
	    "		else if(rel_tex.x < 0.0) rel_tex.x = 0.0;		  \n"
	    "           if (rel_tex.y > 1.0) rel_tex.y = 1.0;		  \n"
	    "		else if(rel_tex.y < 0.0) rel_tex.y = 0.0;	\n"
	    "   	rel_tex = rel_tex / wh; \n"
	    "    } \n"
	    "   else if(repeat == RepeatReflect) {\n"
	    "		if ((1.0 - mod(abs(floor(rel_tex.x)), 2.0)) < 0.001)\n"
	    "			rel_tex.x = 2.0 - (1.0 - fract(rel_tex.x))/wh.x;\n"
	    "		else \n"
	    "			rel_tex.x = fract(rel_tex.x)/wh.x;\n"
	    "		if ((1.0 - mod(abs(floor(rel_tex.y)), 2.0)) < 0.001)\n"
	    "			rel_tex.y = 2.0 - (1.0 - fract(rel_tex.y))/wh.y;\n"
	    "		else \n"
	    "			rel_tex.y = fract(rel_tex.y)/wh.y;\n"
	    "    } \n"
            "   return rel_tex; \n"
	    "}\n";
	/* The texture and the pixmap size is not match eaxctly, so can't sample it directly.
	 * rel_sampler will recalculate the texture coords.*/
	const char *rel_sampler =
	    " vec4 rel_sampler(sampler2D tex_image, vec2 tex, vec2 wh, int repeat, int set_alpha)\n"
	    "{\n"
	    "	tex = rel_tex_coord(tex, wh, repeat - RepeatFix);\n"
	    "   if (repeat == RepeatFix) {\n"
	    "		if (!(tex.x >= 0.0 && tex.x <= 1.0 \n"
	    "		    && tex.y >= 0.0 && tex.y <= 1.0))\n"
	    "			return vec4(0.0, 0.0, 0.0, set_alpha);\n"
	    "		tex = (fract(tex) / wh);\n"
	    "	}\n"
	    "	if (set_alpha != 1)\n"
	    "		return texture2D(tex_image, tex);\n"
	    "	else\n"
	    "		return vec4(texture2D(tex_image, tex).rgb, 1.0);\n"
	    "}\n";

	const char *source_solid_fetch =
	    GLAMOR_DEFAULT_PRECISION
	    "uniform vec4 source;\n"
	    "vec4 get_source()\n" "{\n" "	return source;\n" "}\n";
	const char *source_alpha_pixmap_fetch =
	    GLAMOR_DEFAULT_PRECISION
	    "varying vec2 source_texture;\n"
	    "uniform sampler2D source_sampler;\n"
	    "uniform vec2 source_wh;"
	    "vec4 get_source()\n"
	    "{\n"
	    "   if (source_repeat_mode < RepeatFix)\n"
	    "		return texture2D(source_sampler, source_texture);\n"
	    "   else \n"
	    "		return rel_sampler(source_sampler, source_texture,\n"
	    "				   source_wh, source_repeat_mode, 0);\n"
	    "}\n";
	const char *source_pixmap_fetch =
	    GLAMOR_DEFAULT_PRECISION "varying vec2 source_texture;\n"
	    "uniform sampler2D source_sampler;\n"
	    "uniform vec2 source_wh;\n"
	    "vec4 get_source()\n"
	    "{\n"
	    "   if (source_repeat_mode < RepeatFix) \n"
	    "		return vec4(texture2D(source_sampler, source_texture).rgb, 1);\n"
	    "	else \n"
	    "		return rel_sampler(source_sampler, source_texture,\n"
	    "				   source_wh, source_repeat_mode, 1);\n"
	    "}\n";
	const char *mask_solid_fetch =
	    GLAMOR_DEFAULT_PRECISION "uniform vec4 mask;\n"
	    "vec4 get_mask()\n" "{\n" "	return mask;\n" "}\n";
	const char *mask_alpha_pixmap_fetch =
	    GLAMOR_DEFAULT_PRECISION "varying vec2 mask_texture;\n"
	    "uniform sampler2D mask_sampler;\n"
	    "uniform vec2 mask_wh;\n"
	    "vec4 get_mask()\n"
	    "{\n"
	    "   if (mask_repeat_mode < RepeatFix) \n"
	    "		return texture2D(mask_sampler, mask_texture);\n"
	    "   else \n"
	    "		return rel_sampler(mask_sampler, mask_texture,\n"
	    "				   mask_wh, mask_repeat_mode, 0);\n"
	    "}\n";
	const char *mask_pixmap_fetch =
	    GLAMOR_DEFAULT_PRECISION "varying vec2 mask_texture;\n"
	    "uniform sampler2D mask_sampler;\n"
	    "uniform vec2 mask_wh;\n"
	    "vec4 get_mask()\n"
	    "{\n"
	    "   if (mask_repeat_mode < RepeatFix) \n"
	    "   	return vec4(texture2D(mask_sampler, mask_texture).rgb, 1);\n"
	    "   else \n"
	    "		return rel_sampler(mask_sampler, mask_texture,\n"
	    "				   mask_wh, mask_repeat_mode, 1);\n"
	    "}\n";
	const char *in_source_only =
	    GLAMOR_DEFAULT_PRECISION "void main()\n" "{\n"
	    "	gl_FragColor = get_source();\n" "}\n";
	const char *in_normal =
	    GLAMOR_DEFAULT_PRECISION "void main()\n" "{\n"
	    "	gl_FragColor = get_source() * get_mask().a;\n" "}\n";
	const char *in_ca_source =
	    GLAMOR_DEFAULT_PRECISION "void main()\n" "{\n"
	    "	gl_FragColor = get_source() * get_mask();\n" "}\n";
	const char *in_ca_alpha =
	    GLAMOR_DEFAULT_PRECISION "void main()\n" "{\n"
	    "	gl_FragColor = get_source().a * get_mask();\n" "}\n";
	char *source;
	const char *source_fetch;
	const char *mask_fetch = "";
	const char *in;
	GLuint prog;

	switch (key->source) {
	case SHADER_SOURCE_SOLID:
		source_fetch = source_solid_fetch;
		break;
	case SHADER_SOURCE_TEXTURE_ALPHA:
		source_fetch = source_alpha_pixmap_fetch;
		break;
	case SHADER_SOURCE_TEXTURE:
		source_fetch = source_pixmap_fetch;
		break;
	default:
		FatalError("Bad composite shader source");
	}

	switch (key->mask) {
	case SHADER_MASK_NONE:
		break;
	case SHADER_MASK_SOLID:
		mask_fetch = mask_solid_fetch;
		break;
	case SHADER_MASK_TEXTURE_ALPHA:
		mask_fetch = mask_alpha_pixmap_fetch;
		break;
	case SHADER_MASK_TEXTURE:
		mask_fetch = mask_pixmap_fetch;
		break;
	default:
		FatalError("Bad composite shader mask");
	}

	switch (key->in) {
	case SHADER_IN_SOURCE_ONLY:
		in = in_source_only;
		break;
	case SHADER_IN_NORMAL:
		in = in_normal;
		break;
	case SHADER_IN_CA_SOURCE:
		in = in_ca_source;
		break;
	case SHADER_IN_CA_ALPHA:
		in = in_ca_alpha;
		break;
	default:
		FatalError("Bad composite IN type");
	}

	XNFasprintf(&source, "%s%s%s%s%s%s", repeat_define, relocate_texture, rel_sampler,source_fetch, mask_fetch, in);


	prog = glamor_compile_glsl_prog(dispatch, GL_FRAGMENT_SHADER,
					source);
	free(source);

	return prog;
}

static GLuint
glamor_create_composite_vs(glamor_gl_dispatch * dispatch,
			   struct shader_key *key)
{
	const char *main_opening =
	    "attribute vec4 v_position;\n"
	    "attribute vec4 v_texcoord0;\n"
	    "attribute vec4 v_texcoord1;\n"
	    "varying vec2 source_texture;\n"
	    "varying vec2 mask_texture;\n"
	    "void main()\n" "{\n" "	gl_Position = v_position;\n";
	const char *source_coords =
	    "	source_texture = v_texcoord0.xy;\n";
	const char *mask_coords = "	mask_texture = v_texcoord1.xy;\n";
	const char *main_closing = "}\n";
	const char *source_coords_setup = "";
	const char *mask_coords_setup = "";
	char *source;
	GLuint prog;

	if (key->source != SHADER_SOURCE_SOLID)
		source_coords_setup = source_coords;

	if (key->mask != SHADER_MASK_NONE
	    && key->mask != SHADER_MASK_SOLID)
		mask_coords_setup = mask_coords;

	XNFasprintf(&source,
		    "%s%s%s%s",
		    main_opening,
		    source_coords_setup, mask_coords_setup, main_closing);

	prog =
	    glamor_compile_glsl_prog(dispatch, GL_VERTEX_SHADER, source);
	free(source);

	return prog;
}

static void
glamor_create_composite_shader(ScreenPtr screen, struct shader_key *key,
			       glamor_composite_shader * shader)
{
	GLuint vs, fs, prog;
	GLint source_sampler_uniform_location,
	    mask_sampler_uniform_location;
	glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
	glamor_gl_dispatch *dispatch;

	dispatch = glamor_get_dispatch(glamor_priv);
	vs = glamor_create_composite_vs(dispatch, key);
	if (vs == 0)
		goto out;
	fs = glamor_create_composite_fs(dispatch, key);
	if (fs == 0)
		goto out;

	prog = dispatch->glCreateProgram();
	dispatch->glAttachShader(prog, vs);
	dispatch->glAttachShader(prog, fs);

	dispatch->glBindAttribLocation(prog, GLAMOR_VERTEX_POS,
				       "v_position");
	dispatch->glBindAttribLocation(prog, GLAMOR_VERTEX_SOURCE,
				       "v_texcoord0");
	dispatch->glBindAttribLocation(prog, GLAMOR_VERTEX_MASK,
				       "v_texcoord1");

	glamor_link_glsl_prog(dispatch, prog);

	shader->prog = prog;

	dispatch->glUseProgram(prog);

	if (key->source == SHADER_SOURCE_SOLID) {
		shader->source_uniform_location =
		    dispatch->glGetUniformLocation(prog, "source");
	} else {
		source_sampler_uniform_location =
		    dispatch->glGetUniformLocation(prog, "source_sampler");
		dispatch->glUniform1i(source_sampler_uniform_location, 0);
		shader->source_wh = dispatch->glGetUniformLocation(prog, "source_wh");
		shader->source_repeat_mode = dispatch->glGetUniformLocation(prog, "source_repeat_mode");
	}

	if (key->mask != SHADER_MASK_NONE) {
		if (key->mask == SHADER_MASK_SOLID) {
			shader->mask_uniform_location =
			    dispatch->glGetUniformLocation(prog, "mask");
		} else {
			mask_sampler_uniform_location =
			    dispatch->glGetUniformLocation(prog,
							   "mask_sampler");
			dispatch->glUniform1i
			    (mask_sampler_uniform_location, 1);
			shader->mask_wh = dispatch->glGetUniformLocation(prog, "mask_wh");
			shader->mask_repeat_mode = dispatch->glGetUniformLocation(prog, "mask_repeat_mode");
		}
	}

out:
	glamor_put_dispatch(glamor_priv);
}

static glamor_composite_shader *
glamor_lookup_composite_shader(ScreenPtr screen, struct
			       shader_key
			       *key)
{
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(screen);
	glamor_composite_shader *shader;

	shader =
	    &glamor_priv->composite_shader[key->source][key->
							mask][key->in];
	if (shader->prog == 0)
		glamor_create_composite_shader(screen, key, shader);

	return shader;
}

#define GLAMOR_COMPOSITE_VBO_VERT_CNT 1024

static void
glamor_init_eb(unsigned short *eb, int vert_cnt)
{
	int i, j;
	for(i = 0, j = 0; j < vert_cnt; i += 6, j += 4)
	{
		eb[i] = j;
		eb[i + 1] = j + 1;
		eb[i + 2] = j + 2;
		eb[i + 3] = j;
		eb[i + 4] = j + 2;
		eb[i + 5] = j + 3;
	}
}

void
glamor_init_composite_shaders(ScreenPtr screen)
{
	glamor_screen_private *glamor_priv;
	glamor_gl_dispatch *dispatch;
	unsigned short *eb;
	float *vb = NULL;
	int eb_size;

	glamor_priv = glamor_get_screen_private(screen);
	dispatch = glamor_get_dispatch(glamor_priv);
	dispatch->glGenBuffers(1, &glamor_priv->vbo);
	dispatch->glGenBuffers(1, &glamor_priv->ebo);
	dispatch->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glamor_priv->ebo);

	eb_size = GLAMOR_COMPOSITE_VBO_VERT_CNT * sizeof(short) * 2;

	if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
		dispatch->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				       eb_size,
				       NULL, GL_DYNAMIC_DRAW);
		eb = dispatch->glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
	}
	else {
		vb = malloc(GLAMOR_COMPOSITE_VBO_VERT_CNT * sizeof(float) * 2);
		if (vb == NULL)
			FatalError("Failed to allocate vb memory.\n");
		eb = malloc(eb_size);
	}

	if (eb == NULL)
		FatalError("fatal error, fail to get element buffer. GL context may be not created correctly.\n");
	glamor_init_eb(eb, GLAMOR_COMPOSITE_VBO_VERT_CNT);

	if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
		dispatch->glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		dispatch->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	} else {
		dispatch->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				       eb_size,
				       eb, GL_DYNAMIC_DRAW);
		dispatch->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		dispatch->glBindBuffer(GL_ARRAY_BUFFER, glamor_priv->vbo);
		dispatch->glBufferData(GL_ARRAY_BUFFER,
				       GLAMOR_COMPOSITE_VBO_VERT_CNT * sizeof(float) * 2,
				       NULL, GL_DYNAMIC_DRAW);
		dispatch->glBindBuffer(GL_ARRAY_BUFFER, 0);

		free(eb);
		glamor_priv->vb = (char*)vb;
	}

	glamor_put_dispatch(glamor_priv);
}

void
glamor_fini_composite_shaders(ScreenPtr screen)
{
	glamor_screen_private *glamor_priv;
	glamor_gl_dispatch *dispatch;
	glamor_composite_shader *shader;
	int i,j,k;

	glamor_priv = glamor_get_screen_private(screen);
	dispatch = glamor_get_dispatch(glamor_priv);
	dispatch->glDeleteBuffers(1, &glamor_priv->vbo);
	dispatch->glDeleteBuffers(1, &glamor_priv->ebo);

	for(i = 0; i < SHADER_SOURCE_COUNT; i++)
		for(j = 0; j < SHADER_MASK_COUNT; j++)
			for(k = 0; k < SHADER_IN_COUNT; k++)
			{
				shader = &glamor_priv->composite_shader[i][j][k];
				if (shader->prog)
					dispatch->glDeleteProgram(shader->prog);
			}
	if (glamor_priv->gl_flavor != GLAMOR_GL_DESKTOP
	    && glamor_priv->vb)
		free(glamor_priv->vb);

	glamor_put_dispatch(glamor_priv);
}

static Bool
glamor_set_composite_op(ScreenPtr screen,
			CARD8 op, PicturePtr dest, PicturePtr mask)
{
	GLenum source_blend, dest_blend;
	struct blendinfo *op_info;
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(screen);
	glamor_gl_dispatch *dispatch;

	if (op >= ARRAY_SIZE(composite_op_info)) {
		glamor_fallback("unsupported render op %d \n", op);
		return GL_FALSE;
	}
	op_info = &composite_op_info[op];

	source_blend = op_info->source_blend;
	dest_blend = op_info->dest_blend;

	/* If there's no dst alpha channel, adjust the blend op so that we'll treat
	 * it as always 1.
	 */
	if (PICT_FORMAT_A(dest->format) == 0 && op_info->dest_alpha) {
		if (source_blend == GL_DST_ALPHA)
			source_blend = GL_ONE;
		else if (source_blend == GL_ONE_MINUS_DST_ALPHA)
			source_blend = GL_ZERO;
	}

	/* Set up the source alpha value for blending in component alpha mode. */
	if (mask && mask->componentAlpha
	    && PICT_FORMAT_RGB(mask->format) != 0 && op_info->source_alpha)
	{
		if (dest_blend == GL_SRC_ALPHA)
			dest_blend = GL_SRC_COLOR;
		else if (dest_blend == GL_ONE_MINUS_SRC_ALPHA)
			dest_blend = GL_ONE_MINUS_SRC_COLOR;
	}

	dispatch = glamor_get_dispatch(glamor_priv);
	if (source_blend == GL_ONE && dest_blend == GL_ZERO) {
		dispatch->glDisable(GL_BLEND);
	} else {
		dispatch->glEnable(GL_BLEND);
		dispatch->glBlendFunc(source_blend, dest_blend);
	}
	glamor_put_dispatch(glamor_priv);
	return TRUE;
}

static void
glamor_set_composite_texture(ScreenPtr screen, int unit,
			     PicturePtr picture,
			     glamor_pixmap_private * pixmap_priv,
			     GLuint wh_location, GLuint repeat_location)
{
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(screen);
	glamor_gl_dispatch *dispatch;
	float wh[2];
	int repeat_type;

	dispatch = glamor_get_dispatch(glamor_priv);
	dispatch->glActiveTexture(GL_TEXTURE0 + unit);
	dispatch->glBindTexture(GL_TEXTURE_2D, pixmap_priv->fbo->tex);
	repeat_type = picture->repeatType;
	switch (picture->repeatType) {
	case RepeatNone:
#ifndef GLAMOR_GLES2
		/* XXX  GLES2 doesn't support GL_CLAMP_TO_BORDER. */
		dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
					  GL_CLAMP_TO_BORDER);
		dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
					  GL_CLAMP_TO_BORDER);
#else
		dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
					  GL_CLAMP_TO_EDGE);
		dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
					  GL_CLAMP_TO_EDGE);
#endif
		break;
	case RepeatNormal:
		dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
					  GL_REPEAT);
		dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
					  GL_REPEAT);
		break;
	case RepeatPad:
		dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
					  GL_CLAMP_TO_EDGE);
		dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
					  GL_CLAMP_TO_EDGE);
		break;
	case RepeatReflect:
		dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
					  GL_MIRRORED_REPEAT);
		dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
					  GL_MIRRORED_REPEAT);
		break;
	}

	switch (picture->filter) {
	default:
	case PictFilterNearest:
		dispatch->glTexParameteri(GL_TEXTURE_2D,
					  GL_TEXTURE_MIN_FILTER,
					  GL_NEAREST);
		dispatch->glTexParameteri(GL_TEXTURE_2D,
					  GL_TEXTURE_MAG_FILTER,
					  GL_NEAREST);
		break;
	case PictFilterBilinear:
		dispatch->glTexParameteri(GL_TEXTURE_2D,
					  GL_TEXTURE_MIN_FILTER,
					  GL_LINEAR);
		dispatch->glTexParameteri(GL_TEXTURE_2D,
					  GL_TEXTURE_MAG_FILTER,
					  GL_LINEAR);
		break;
	}
#ifndef GLAMOR_GLES2
	dispatch->glEnable(GL_TEXTURE_2D);
#endif
	/* XXX may be we can eaxctly check whether we need to touch
	 * the out-of-box area then determine whether we need to fix.
	 * */
	if (repeat_type != RepeatNone)
		repeat_type += RepeatFix;
	else if (glamor_priv->gl_flavor == GLAMOR_GL_ES2) {
		if (picture->transform
		   || (GLAMOR_PIXMAP_FBO_NOT_EAXCT_SIZE(pixmap_priv)))
			repeat_type += RepeatFix;
	}

	if (repeat_type >= RepeatFix) {
		glamor_pixmap_fbo_fix_wh_ratio(wh, pixmap_priv);
		dispatch->glUniform2fv(wh_location, 1, wh);
	}
	dispatch->glUniform1i(repeat_location, repeat_type);
	glamor_put_dispatch(glamor_priv);
}

static void
glamor_set_composite_solid(glamor_gl_dispatch * dispatch, float *color,
			   GLint uniform_location)
{
	dispatch->glUniform4fv(uniform_location, 1, color);
}

static int
compatible_formats(CARD8 op, PicturePtr dst, PicturePtr src)
{
	if (op == PictOpSrc) {
		if (src->format == dst->format)
			return 1;

		if (src->format == PICT_a8r8g8b8
		    && dst->format == PICT_x8r8g8b8)
			return 1;

		if (src->format == PICT_a8b8g8r8
		    && dst->format == PICT_x8b8g8r8)
			return 1;
	} else if (op == PictOpOver) {
		if (src->alphaMap || dst->alphaMap)
			return 0;

		if (src->format != dst->format)
			return 0;

		if (src->format == PICT_x8r8g8b8
		    || src->format == PICT_x8b8g8r8)
			return 1;
	}

	return 0;
}

static char
glamor_get_picture_location(PicturePtr picture)
{
	if (picture == NULL)
		return ' ';

	if (picture->pDrawable == NULL) {
		switch (picture->pSourcePict->type) {
		case SourcePictTypeSolidFill:
			return 'c';
		case SourcePictTypeLinear:
			return 'l';
		case SourcePictTypeRadial:
			return 'r';
		default:
			return '?';
		}
	}
	return glamor_get_drawable_location(picture->pDrawable);
}

static Bool
glamor_composite_with_copy(CARD8 op,
			   PicturePtr source,
			   PicturePtr dest,
			   INT16 x_source,
			   INT16 y_source,
			   INT16 x_dest,
			   INT16 y_dest, CARD16 width, CARD16 height)
{
	RegionRec region;
	int ret = FALSE;

	if (!source->pDrawable)
		return FALSE;

	if (!compatible_formats(op, dest, source))
		return FALSE;

	if (source->repeat || source->transform)
		return FALSE;

	x_dest += dest->pDrawable->x;
	y_dest += dest->pDrawable->y;
	x_source += source->pDrawable->x;
	y_source += source->pDrawable->y;
	if (!miComputeCompositeRegion(&region,
				      source, NULL, dest,
				      x_source, y_source,
				      0, 0, x_dest, y_dest, width, height))
		return TRUE;

	if (PICT_FORMAT_A(source->format) == 0) {
		/* Fallback if we sample outside the source so that we
		 * swizzle the correct clear color for out-of-bounds texels.
		 */
		if (region.extents.x1 + x_source - x_dest < 0)
			goto cleanup_region;
		if (region.extents.x2 + x_source - x_dest > source->pDrawable->width)
			goto cleanup_region;

		if (region.extents.y1 + y_source - y_dest < 0)
			goto cleanup_region;
		if (region.extents.y2 + y_source - y_dest > source->pDrawable->height)
			goto cleanup_region;
	}

	ret = glamor_copy_n_to_n_nf(source->pDrawable,
				    dest->pDrawable, NULL,
				    REGION_RECTS(&region),
				    REGION_NUM_RECTS(&region),
				    x_source - x_dest, y_source - y_dest,
				    FALSE, FALSE, 0, NULL);
cleanup_region:
	REGION_UNINIT(dest->pDrawable->pScreen, &region);
	return ret;
}

static void
glamor_setup_composite_vbo(ScreenPtr screen, int n_verts)
{
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(screen);
	glamor_gl_dispatch *dispatch;

	glamor_priv->vbo_offset = 0;
	glamor_priv->vbo_offset = 0;
	glamor_priv->render_nr_verts = 0;
	glamor_priv->vbo_size = n_verts * sizeof(float) * 2;

	glamor_priv->vb_stride = 2 * sizeof(float);
	if (glamor_priv->has_source_coords)
		glamor_priv->vb_stride += 2 * sizeof(float);
	if (glamor_priv->has_mask_coords)
		glamor_priv->vb_stride += 2 * sizeof(float);

	dispatch = glamor_get_dispatch(glamor_priv);
	dispatch->glBindBuffer(GL_ARRAY_BUFFER, glamor_priv->vbo);
	if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
		dispatch->glBufferData(GL_ARRAY_BUFFER,
				       n_verts * sizeof(float) * 2,
				       NULL, GL_DYNAMIC_DRAW);
		glamor_priv->vb = dispatch->glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	}
	dispatch->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glamor_priv->ebo);

	dispatch->glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_FLOAT,
					GL_FALSE, glamor_priv->vb_stride,
					(void *) ((long)
						  glamor_priv->vbo_offset));
	dispatch->glEnableVertexAttribArray(GLAMOR_VERTEX_POS);

	if (glamor_priv->has_source_coords) {
		dispatch->glVertexAttribPointer(GLAMOR_VERTEX_SOURCE, 2,
						GL_FLOAT, GL_FALSE,
						glamor_priv->vb_stride,
						(void *) ((long)
							  glamor_priv->vbo_offset
							  +
							  2 *
							  sizeof(float)));
		dispatch->glEnableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
	}

	if (glamor_priv->has_mask_coords) {
		dispatch->glVertexAttribPointer(GLAMOR_VERTEX_MASK, 2,
						GL_FLOAT, GL_FALSE,
						glamor_priv->vb_stride,
						(void *) ((long)
							  glamor_priv->vbo_offset
							  +
							  (glamor_priv->has_source_coords
							   ? 4 : 2) *
							  sizeof(float)));
		dispatch->glEnableVertexAttribArray(GLAMOR_VERTEX_MASK);
	}
	glamor_put_dispatch(glamor_priv);
}

static void
glamor_emit_composite_vert(ScreenPtr screen,
			   const float *src_coords,
			   const float *mask_coords,
			   const float *dst_coords, int i)
{
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(screen);
	float *vb = (float *) (glamor_priv->vb + glamor_priv->vbo_offset);
	int j = 0;

	vb[j++] = dst_coords[i * 2 + 0];
	vb[j++] = dst_coords[i * 2 + 1];
	if (glamor_priv->has_source_coords) {
		vb[j++] = src_coords[i * 2 + 0];
		vb[j++] = src_coords[i * 2 + 1];
	}
	if (glamor_priv->has_mask_coords) {
		vb[j++] = mask_coords[i * 2 + 0];
		vb[j++] = mask_coords[i * 2 + 1];
	}

	glamor_priv->render_nr_verts++;
	glamor_priv->vbo_offset += glamor_priv->vb_stride;
}

static void
glamor_flush_composite_rects(ScreenPtr screen)
{
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(screen);
	glamor_gl_dispatch *dispatch;

	if (!glamor_priv->render_nr_verts)
		return;

	dispatch = glamor_get_dispatch(glamor_priv);
	if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP)
		dispatch->glUnmapBuffer(GL_ARRAY_BUFFER);
	else {

		dispatch->glBindBuffer(GL_ARRAY_BUFFER, glamor_priv->vbo);
		dispatch->glBufferData(GL_ARRAY_BUFFER,
				       glamor_priv->vbo_offset,
				       glamor_priv->vb, GL_DYNAMIC_DRAW);
	}

	dispatch->glDrawElements(GL_TRIANGLES, (glamor_priv->render_nr_verts * 3) / 2,
				 GL_UNSIGNED_SHORT, NULL);
	glamor_put_dispatch(glamor_priv);
}

static void
glamor_emit_composite_rect(ScreenPtr screen,
			   const float *src_coords,
			   const float *mask_coords,
			   const float *dst_coords)
{
	glamor_emit_composite_vert(screen, src_coords, mask_coords,
				   dst_coords, 0);
	glamor_emit_composite_vert(screen, src_coords, mask_coords,
				   dst_coords, 1);
	glamor_emit_composite_vert(screen, src_coords, mask_coords,
				   dst_coords, 2);
	glamor_emit_composite_vert(screen, src_coords, mask_coords,
				   dst_coords, 3);
}

int pict_format_combine_tab[][3] = {
	{PICT_TYPE_ARGB, PICT_TYPE_A, PICT_TYPE_ARGB},
	{PICT_TYPE_ABGR, PICT_TYPE_A, PICT_TYPE_ABGR},
};

static Bool
combine_pict_format(PictFormatShort * des, const PictFormatShort src,
		    const PictFormatShort mask, enum shader_in in_ca)
{
	PictFormatShort new_vis;
	int src_type, mask_type, src_bpp, mask_bpp;
	int i;
	if (src == mask) {
		*des = src;
		return TRUE;
	}
	src_bpp = PICT_FORMAT_BPP(src);
	mask_bpp = PICT_FORMAT_BPP(mask);

	assert(src_bpp == mask_bpp);

	new_vis = PICT_FORMAT_VIS(src) | PICT_FORMAT_VIS(mask);

	switch (in_ca) {
	case SHADER_IN_SOURCE_ONLY:
		return TRUE;
	case SHADER_IN_NORMAL:
		src_type = PICT_FORMAT_TYPE(src);
		mask_type = PICT_TYPE_A;
		break;
	case SHADER_IN_CA_SOURCE:
		src_type = PICT_FORMAT_TYPE(src);
		mask_type = PICT_FORMAT_TYPE(mask);
		break;
	case SHADER_IN_CA_ALPHA:
		src_type = PICT_TYPE_A;
		mask_type = PICT_FORMAT_TYPE(mask);
		break;
	default:
		return FALSE;
	}

	if (src_type == mask_type) {
		*des = PICT_VISFORMAT(src_bpp, src_type, new_vis);
		return TRUE;
	}

	for (i = 0;
	     i <
	     sizeof(pict_format_combine_tab) /
	     sizeof(pict_format_combine_tab[0]); i++) {
		if ((src_type == pict_format_combine_tab[i][0]
		     && mask_type == pict_format_combine_tab[i][1])
		    || (src_type == pict_format_combine_tab[i][1]
			&& mask_type == pict_format_combine_tab[i][0])) {
			*des = PICT_VISFORMAT(src_bpp,
					      pict_format_combine_tab[i]
					      [2], new_vis);
			return TRUE;
		}
	}
	return FALSE;
}

static Bool
glamor_composite_with_shader(CARD8 op,
			     PicturePtr source,
			     PicturePtr mask,
			     PicturePtr dest,
			     int nrect, glamor_composite_rect_t * rects)
{
	ScreenPtr screen = dest->pDrawable->pScreen;
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(screen);
	glamor_gl_dispatch *dispatch;
	PixmapPtr dest_pixmap =
	    glamor_get_drawable_pixmap(dest->pDrawable);
	PixmapPtr source_pixmap = NULL, mask_pixmap = NULL;
	glamor_pixmap_private *source_pixmap_priv = NULL;
	glamor_pixmap_private *mask_pixmap_priv = NULL;
	glamor_pixmap_private *dest_pixmap_priv = NULL;
	GLfloat dst_xscale, dst_yscale;
	GLfloat mask_xscale = 1, mask_yscale = 1, src_xscale =
	    1, src_yscale = 1;
	struct shader_key key;
	glamor_composite_shader *shader;
	float vertices[8], source_texcoords[8], mask_texcoords[8];
	int dest_x_off, dest_y_off;
	int source_x_off, source_y_off;
	int mask_x_off, mask_y_off;
	enum glamor_pixmap_status source_status = GLAMOR_NONE;
	enum glamor_pixmap_status mask_status = GLAMOR_NONE;
	PictFormatShort saved_source_format = 0;
	float src_matrix[9], mask_matrix[9];
	GLfloat source_solid_color[4], mask_solid_color[4];
	dest_pixmap_priv = glamor_get_pixmap_private(dest_pixmap);
	int vert_stride = 4;
	int nrect_max;
	Bool ret = FALSE;

	if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(dest_pixmap_priv)) {
		glamor_fallback("dest has no fbo.\n");
		goto fail;
	}
	memset(&key, 0, sizeof(key));
	if (!source->pDrawable) {
		if (source->pSourcePict->type == SourcePictTypeSolidFill) {
			key.source = SHADER_SOURCE_SOLID;
			glamor_get_rgba_from_pixel(source->
						   pSourcePict->solidFill.
						   color,
						   &source_solid_color[0],
						   &source_solid_color[1],
						   &source_solid_color[2],
						   &source_solid_color[3],
						   PICT_a8r8g8b8);
		} else {
			glamor_fallback("gradient source\n");
			goto fail;
		}
	} else {
		key.source = SHADER_SOURCE_TEXTURE_ALPHA;
	}
	if (mask) {
		if (!mask->pDrawable) {
			if (mask->pSourcePict->type ==
			    SourcePictTypeSolidFill) {
				key.mask = SHADER_MASK_SOLID;
				glamor_get_rgba_from_pixel
				    (mask->pSourcePict->solidFill.color,
				     &mask_solid_color[0],
				     &mask_solid_color[1],
				     &mask_solid_color[2],
				     &mask_solid_color[3], PICT_a8r8g8b8);
			} else {
				glamor_fallback("gradient mask\n");
				goto fail;
			}
		} else {
			key.mask = SHADER_MASK_TEXTURE_ALPHA;
		}

		if (!mask->componentAlpha) {
			key.in = SHADER_IN_NORMAL;
		} else {
			if (op == PictOpClear)
				key.mask = SHADER_MASK_NONE;
			else if (op == PictOpSrc || op == PictOpAdd
				 || op == PictOpIn || op == PictOpOut
				 || op == PictOpOverReverse)
				key.in = SHADER_IN_CA_SOURCE;
			else if (op == PictOpOutReverse || op == PictOpInReverse) {
				key.in = SHADER_IN_CA_ALPHA;
			} else {
				glamor_fallback
				    ("Unsupported component alpha op: %d\n", op);
				goto fail;
			}
		}
	} else {
		key.mask = SHADER_MASK_NONE;
		key.in = SHADER_IN_SOURCE_ONLY;
	}

	if (source->alphaMap) {
		glamor_fallback("source alphaMap\n");
		goto fail;
	}
	if (mask && mask->alphaMap) {
		glamor_fallback("mask alphaMap\n");
		goto fail;
	}
	if (key.source == SHADER_SOURCE_TEXTURE ||
	    key.source == SHADER_SOURCE_TEXTURE_ALPHA) {
		source_pixmap =
		    glamor_get_drawable_pixmap(source->pDrawable);
		source_pixmap_priv =
		    glamor_get_pixmap_private(source_pixmap);
		if (source_pixmap == dest_pixmap) {
			glamor_fallback("source == dest\n");
			goto fail;
		}
		if (!source_pixmap_priv || source_pixmap_priv->gl_fbo == 0) {
			/* XXX in Xephyr, we may have gl_fbo equal to 1 but gl_tex 
			 * equal to zero when the pixmap is screen pixmap. Then we may
			 * refer the tex zero directly latter in the composition. 
			 * It seems that it works fine, but it may have potential problem*/
#ifdef GLAMOR_PIXMAP_DYNAMIC_UPLOAD
			source_status = GLAMOR_UPLOAD_PENDING;
#else
			glamor_fallback("no texture in source\n");
			goto fail;
#endif
		}
	}
	if (key.mask == SHADER_MASK_TEXTURE ||
	    key.mask == SHADER_MASK_TEXTURE_ALPHA) {
		mask_pixmap = glamor_get_drawable_pixmap(mask->pDrawable);
		mask_pixmap_priv = glamor_get_pixmap_private(mask_pixmap);
		if (mask_pixmap == dest_pixmap) {
			glamor_fallback("mask == dest\n");
			goto fail;
		}
		if (!mask_pixmap_priv || mask_pixmap_priv->gl_fbo == 0) {
#ifdef GLAMOR_PIXMAP_DYNAMIC_UPLOAD
			mask_status = GLAMOR_UPLOAD_PENDING;
#else
			glamor_fallback("no texture in mask\n");
			goto fail;
#endif
		}
	}
#ifdef GLAMOR_PIXMAP_DYNAMIC_UPLOAD
	if (source_status == GLAMOR_UPLOAD_PENDING
	    && mask_status == GLAMOR_UPLOAD_PENDING
	    && source_pixmap == mask_pixmap) {

		if (source->format != mask->format) {
			saved_source_format = source->format;

			if (!combine_pict_format
			    (&source->format, source->format,
			     mask->format, key.in)) {
				glamor_fallback
				    ("combine source %x mask %x failed.\n",
				     source->format, mask->format);
				goto fail;
			}

			if (source->format != saved_source_format) {
				glamor_picture_format_fixup(source,
							    source_pixmap_priv);
			}
			/* XXX  
			 * By default, glamor_upload_picture_to_texture will wire alpha to 1
			 * if one picture doesn't have alpha. So we don't do that again in 
			 * rendering function. But here is a special case, as source and
			 * mask share the same texture but may have different formats. For 
			 * example, source doesn't have alpha, but mask has alpha. Then the
			 * texture will have the alpha value for the mask. And will not wire
			 * to 1 for the source. In this case, we have to use different shader
			 * to wire the source's alpha to 1.
			 *
			 * But this may cause a potential problem if the source's repeat mode 
			 * is REPEAT_NONE, and if the source is smaller than the dest, then
			 * for the region not covered by the source may be painted incorrectly.
			 * because we wire the alpha to 1.
			 *
			 **/
			if (!PICT_FORMAT_A(saved_source_format)
			    && PICT_FORMAT_A(mask->format))
				key.source = SHADER_SOURCE_TEXTURE;

			if (!PICT_FORMAT_A(mask->format)
			    && PICT_FORMAT_A(saved_source_format))
				key.mask = SHADER_MASK_TEXTURE;

			mask_status = GLAMOR_NONE;
		}

		source_status = glamor_upload_picture_to_texture(source);
		if (source_status != GLAMOR_UPLOAD_DONE) {
			glamor_fallback
			    ("Failed to upload source texture.\n");
			goto fail;
		}
	} else {
		if (source_status == GLAMOR_UPLOAD_PENDING) {
			source_status = glamor_upload_picture_to_texture(source);
			if (source_status != GLAMOR_UPLOAD_DONE) {
				glamor_fallback
				    ("Failed to upload source texture.\n");
				goto fail;
			}
		}

		if (mask_status == GLAMOR_UPLOAD_PENDING) {
			mask_status = glamor_upload_picture_to_texture(mask);
			if (mask_status != GLAMOR_UPLOAD_DONE) {
				glamor_fallback
				    ("Failed to upload mask texture.\n");
				goto fail;
			}
		}
	}
#endif

	/*Before enter the rendering stage, we need to fixup
	 * transformed source and mask, if the transform is not int translate. */
	if (key.source != SHADER_SOURCE_SOLID
	    && source->transform
	    && !pixman_transform_is_int_translate(source->transform)) {
		if (!glamor_fixup_pixmap_priv(screen, source_pixmap_priv))
			goto fail;
	}
	if (key.mask != SHADER_MASK_NONE && key.mask != SHADER_MASK_SOLID
	    && mask->transform
	    && !pixman_transform_is_int_translate(mask->transform)) {
		if (!glamor_fixup_pixmap_priv(screen, mask_pixmap_priv))
			goto fail;
	}

	glamor_set_destination_pixmap_priv_nc(dest_pixmap_priv);

	if (!glamor_set_composite_op(screen, op, dest, mask)) {
		goto fail;
	}

	shader = glamor_lookup_composite_shader(screen, &key);
	if (shader->prog == 0) {
		glamor_fallback
		    ("no shader program for this render acccel mode\n");
		goto fail;
	}

	dispatch = glamor_get_dispatch(glamor_priv);
	dispatch->glUseProgram(shader->prog);

	if (key.source == SHADER_SOURCE_SOLID) {
		glamor_set_composite_solid(dispatch, source_solid_color,
					   shader->source_uniform_location);
	} else {
		glamor_set_composite_texture(screen, 0, source,
					     source_pixmap_priv, shader->source_wh,
					     shader->source_repeat_mode);
	}
	if (key.mask != SHADER_MASK_NONE) {
		if (key.mask == SHADER_MASK_SOLID) {
			glamor_set_composite_solid(dispatch,
						   mask_solid_color,
						   shader->mask_uniform_location);
		} else {
			glamor_set_composite_texture(screen, 1, mask,
						     mask_pixmap_priv, shader->mask_wh,
						     shader->mask_repeat_mode);
		}
	}

	glamor_priv->has_source_coords = key.source != SHADER_SOURCE_SOLID;
	glamor_priv->has_mask_coords = (key.mask != SHADER_MASK_NONE &&
					key.mask != SHADER_MASK_SOLID);

	glamor_get_drawable_deltas(dest->pDrawable, dest_pixmap,
				   &dest_x_off, &dest_y_off);
	pixmap_priv_get_scale(dest_pixmap_priv, &dst_xscale, &dst_yscale);

	if (glamor_priv->has_source_coords) {
		glamor_get_drawable_deltas(source->pDrawable,
					   source_pixmap, &source_x_off,
					   &source_y_off);
		pixmap_priv_get_scale(source_pixmap_priv, &src_xscale,
				      &src_yscale);
		glamor_picture_get_matrixf(source, src_matrix);
		vert_stride += 4;
	}

	if (glamor_priv->has_mask_coords) {
		glamor_get_drawable_deltas(mask->pDrawable, mask_pixmap,
					   &mask_x_off, &mask_y_off);
		pixmap_priv_get_scale(mask_pixmap_priv, &mask_xscale,
				      &mask_yscale);
		glamor_picture_get_matrixf(mask, mask_matrix);
		vert_stride += 4;
	}

	nrect_max = (vert_stride * nrect) > GLAMOR_COMPOSITE_VBO_VERT_CNT ?
			 (GLAMOR_COMPOSITE_VBO_VERT_CNT / vert_stride) : nrect;

	while(nrect) {
		int mrect, rect_processed;

		mrect = nrect > nrect_max ? nrect_max : nrect ;
		glamor_setup_composite_vbo(screen, mrect * vert_stride);
		rect_processed = mrect;

		while (mrect--) {
			INT16 x_source;
			INT16 y_source;
			INT16 x_mask;
			INT16 y_mask;
			INT16 x_dest;
			INT16 y_dest;
			CARD16 width;
			CARD16 height;

			x_dest = rects->x_dst + dest_x_off;
			y_dest = rects->y_dst + dest_y_off;
			x_source = rects->x_src + source_x_off;;
			y_source = rects->y_src + source_y_off;
			x_mask = rects->x_mask + mask_x_off;
			y_mask = rects->y_mask + mask_y_off;
			width = rects->width;
			height = rects->height;

			glamor_set_normalize_vcoords(dst_xscale,
						     dst_yscale,
						     x_dest, y_dest,
						     x_dest + width, y_dest + height,
						     glamor_priv->yInverted,
						     vertices);

			if (key.source != SHADER_SOURCE_SOLID) {
				if (source->transform)
					glamor_set_transformed_normalize_tcoords
						(src_matrix, src_xscale,
						 src_yscale, x_source, y_source,
						 x_source + width, y_source + height,
						 glamor_priv->yInverted,
						 source_texcoords);
				else
					glamor_set_normalize_tcoords
						(src_xscale, src_yscale,
						 x_source, y_source,
						 x_source + width, y_source + height,
						 glamor_priv->yInverted,
						 source_texcoords);
			}

			if (key.mask != SHADER_MASK_NONE
			    && key.mask != SHADER_MASK_SOLID) {
				if (mask->transform)
					glamor_set_transformed_normalize_tcoords
						(mask_matrix,
						 mask_xscale,
						 mask_yscale, x_mask, y_mask,
						 x_mask + width, y_mask + height,
						 glamor_priv->yInverted,
						 mask_texcoords);
				else
					glamor_set_normalize_tcoords
						(mask_xscale,
						 mask_yscale, x_mask, y_mask,
						 x_mask + width, y_mask + height,
						 glamor_priv->yInverted,
						 mask_texcoords);
			}
			glamor_emit_composite_rect(screen,
						   source_texcoords,
						   mask_texcoords,
						   vertices);
			rects++;
		}
		glamor_flush_composite_rects(screen);
		nrect -= rect_processed;
	}

	dispatch->glBindBuffer(GL_ARRAY_BUFFER, 0);
	dispatch->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_MASK);
	dispatch->glDisable(GL_BLEND);
#ifndef GLAMOR_GLES2
	dispatch->glActiveTexture(GL_TEXTURE0);
	dispatch->glDisable(GL_TEXTURE_2D);
	dispatch->glActiveTexture(GL_TEXTURE1);
	dispatch->glDisable(GL_TEXTURE_2D);
#endif
	dispatch->glUseProgram(0);
	if (saved_source_format)
		source->format = saved_source_format;
	glamor_put_dispatch(glamor_priv);

	ret = TRUE;
	goto done;

fail:
	if (saved_source_format)
		source->format = saved_source_format;
done:
	return ret;
}

#ifdef GLAMOR_GRADIENT_SHADER
static GLint
_glamor_create_getcolor_fs_program(ScreenPtr screen, int stops_count, int use_array)
{
	glamor_screen_private *glamor_priv;
	glamor_gl_dispatch *dispatch;

	char *gradient_fs = NULL;
	GLint fs_getcolor_prog;

	const char *gradient_fs_getcolor =
	    GLAMOR_DEFAULT_PRECISION
	    "uniform int n_stop;\n"
	    "uniform float stops[%d];\n"
	    "uniform vec4 stop_colors[%d];\n"
	    "vec4 get_color(float stop_len)\n"
	    "{\n"
	    "    int i = 0;\n"
	    "    float new_alpha; \n"
	    "    vec4 gradient_color;\n"
	    "    float percentage; \n"
	    "    for(i = 0; i < n_stop - 1; i++) {\n"
	    "        if(stop_len < stops[i])\n"
	    "            break; \n"
	    "    }\n"
	    "    \n"
	    "    percentage = (stop_len - stops[i-1])/(stops[i] - stops[i-1]);\n"
	    "    if(stops[i] - stops[i-1] > 2.0)\n"
	    "        percentage = 0.0;\n" //For comply with pixman, walker->stepper overflow.
	    "    new_alpha = percentage * stop_colors[i].a + \n"
	    "                       (1.0-percentage) * stop_colors[i-1].a; \n"
	    "    gradient_color = vec4((percentage * stop_colors[i].rgb \n"
	    "                          + (1.0-percentage) * stop_colors[i-1].rgb)*new_alpha, \n"
	    "                          new_alpha);\n"
	    "    \n"
	    "    return gradient_color;\n"
	    "}\n";

	/* Because the array access for shader is very slow, the performance is very low
	   if use array. So use global uniform to replace for it if the number of n_stops is small.*/
	const char *gradient_fs_getcolor_no_array =
	    GLAMOR_DEFAULT_PRECISION
	    "uniform int n_stop;\n"
	    "uniform float stop0;\n"
	    "uniform float stop1;\n"
	    "uniform float stop2;\n"
	    "uniform float stop3;\n"
	    "uniform float stop4;\n"
	    "uniform float stop5;\n"
	    "uniform float stop6;\n"
	    "uniform float stop7;\n"
	    "uniform vec4 stop_color0;\n"
	    "uniform vec4 stop_color1;\n"
	    "uniform vec4 stop_color2;\n"
	    "uniform vec4 stop_color3;\n"
	    "uniform vec4 stop_color4;\n"
	    "uniform vec4 stop_color5;\n"
	    "uniform vec4 stop_color6;\n"
	    "uniform vec4 stop_color7;\n"
	    "\n"
	    "vec4 get_color(float stop_len)\n"
	    "{\n"
	    "    float stop_after;\n"
	    "    float stop_before;\n"
	    "    vec4 stop_color_before;\n"
	    "    vec4 stop_color_after;\n"
	    "    float new_alpha; \n"
	    "    vec4 gradient_color;\n"
	    "    float percentage; \n"
	    "    \n"
	    "    if((stop_len < stop0) && (n_stop >= 1)) {\n"
	    "        stop_color_before = stop_color0;\n"
	    "        stop_color_after = stop_color0;\n"
	    "        stop_after = stop0;\n"
	    "        stop_before = stop0;\n"
	    "        percentage = 0.0;\n"
	    "    } else if((stop_len < stop1) && (n_stop >= 2)) {\n"
	    "        stop_color_before = stop_color0;\n"
	    "        stop_color_after = stop_color1;\n"
	    "        stop_after = stop1;\n"
	    "        stop_before = stop0;\n"
	    "        percentage = (stop_len - stop0)/(stop1 - stop0);\n"
	    "    } else if((stop_len < stop2) && (n_stop >= 3)) {\n"
	    "        stop_color_before = stop_color1;\n"
	    "        stop_color_after = stop_color2;\n"
	    "        stop_after = stop2;\n"
	    "        stop_before = stop1;\n"
	    "        percentage = (stop_len - stop1)/(stop2 - stop1);\n"
	    "    } else if((stop_len < stop3) && (n_stop >= 4)){\n"
	    "        stop_color_before = stop_color2;\n"
	    "        stop_color_after = stop_color3;\n"
	    "        stop_after = stop3;\n"
	    "        stop_before = stop2;\n"
	    "        percentage = (stop_len - stop2)/(stop3 - stop2);\n"
	    "    } else if((stop_len < stop4) && (n_stop >= 5)){\n"
	    "        stop_color_before = stop_color3;\n"
	    "        stop_color_after = stop_color4;\n"
	    "        stop_after = stop4;\n"
	    "        stop_before = stop3;\n"
	    "        percentage = (stop_len - stop3)/(stop4 - stop3);\n"
	    "    } else if((stop_len < stop5) && (n_stop >= 6)){\n"
	    "        stop_color_before = stop_color4;\n"
	    "        stop_color_after = stop_color5;\n"
	    "        stop_after = stop5;\n"
	    "        stop_before = stop4;\n"
	    "        percentage = (stop_len - stop4)/(stop5 - stop4);\n"
	    "    } else if((stop_len < stop6) && (n_stop >= 7)){\n"
	    "        stop_color_before = stop_color5;\n"
	    "        stop_color_after = stop_color6;\n"
	    "        stop_after = stop6;\n"
	    "        stop_before = stop5;\n"
	    "        percentage = (stop_len - stop5)/(stop6 - stop5);\n"
	    "    } else if((stop_len < stop7) && (n_stop >= 8)){\n"
	    "        stop_color_before = stop_color6;\n"
	    "        stop_color_after = stop_color7;\n"
	    "        stop_after = stop7;\n"
	    "        stop_before = stop6;\n"
	    "        percentage = (stop_len - stop6)/(stop7 - stop6);\n"
	    "    } else {\n"
	    "        stop_color_before = stop_color7;\n"
	    "        stop_color_after = stop_color7;\n"
	    "        stop_after = stop7;\n"
	    "        stop_before = stop7;\n"
	    "        percentage = 0.0;\n"
	    "    }\n"
	    "    if(stop_after - stop_before > 2.0)\n"
	    "        percentage = 0.0;\n"//For comply with pixman, walker->stepper overflow.
	    "    new_alpha = percentage * stop_color_after.a + \n"
	    "                       (1.0-percentage) * stop_color_before.a; \n"
	    "    gradient_color = vec4((percentage * stop_color_after.rgb \n"
	    "                          + (1.0-percentage) * stop_color_before.rgb)*new_alpha, \n"
	    "                          new_alpha);\n"
	    "    \n"
	    "    return gradient_color;\n"
	    "}\n";

	glamor_priv = glamor_get_screen_private(screen);
	dispatch = glamor_get_dispatch(glamor_priv);

	if(use_array) {
		XNFasprintf(&gradient_fs,
		    gradient_fs_getcolor, stops_count, stops_count);
		fs_getcolor_prog = glamor_compile_glsl_prog(dispatch, GL_FRAGMENT_SHADER,
		                                            gradient_fs);
		free(gradient_fs);
	} else {
		fs_getcolor_prog = glamor_compile_glsl_prog(dispatch, GL_FRAGMENT_SHADER,
		                                            gradient_fs_getcolor_no_array);
	}

	return fs_getcolor_prog;
}

static void
_glamor_create_radial_gradient_program(ScreenPtr screen, int stops_count, int dyn_gen)
{
	glamor_screen_private *glamor_priv;
	glamor_gl_dispatch *dispatch;
	int index;

	GLint gradient_prog = 0;
	char *gradient_fs = NULL;
	GLint fs_main_prog, fs_getcolor_prog, vs_prog;

	const char *gradient_vs =
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
	 *     Refer to pixman radial gradient.
	 *
	 *     The problem is given the two circles of c1 and c2 with the radius of r1 and
	 *     r1, we need to caculate the t, which is used to do interpolate with stops,
	 *     using the fomula:
	 *     length((1-t)*c1 + t*c2 - p) = (1-t)*r1 + t*r2
	 *     expand the fomula with xy coond, get the following:
	 *     sqrt(sqr((1-t)*c1.x + t*c2.x - p.x) + sqr((1-t)*c1.y + t*c2.y - p.y))
	 *           = (1-t)r1 + t*r2
	 *     <====> At*t- 2Bt + C = 0
	 *     where A = sqr(c2.x - c1.x) + sqr(c2.y - c1.y) - sqr(r2 -r1)
	 *           B = (p.x - c1.x)*(c2.x - c1.x) + (p.y - c1.y)*(c2.y - c1.y) + r1*(r2 -r1)
	 *           C = sqr(p.x - c1.x) + sqr(p.y - c1.y) - r1*r1
	 *
	 *     solve the fomula and we get the result of
	 *     t = (B + sqrt(B*B - A*C)) / A  or
	 *     t = (B - sqrt(B*B - A*C)) / A  (quadratic equation have two solutions)
	 *
	 *     The solution we are going to prefer is the bigger one, unless the
	 *     radius associated to it is negative (or it falls outside the valid t range)
	 */

	const char *gradient_fs_template =
	    GLAMOR_DEFAULT_PRECISION
	    "uniform mat3 transform_mat;\n"
	    "uniform int repeat_type;\n"
	    "uniform float A_value;\n"
	    "uniform vec2 c1;\n"
	    "uniform float r1;\n"
	    "uniform vec2 c2;\n"
	    "uniform float r2;\n"
	    "varying vec2 source_texture;\n"
	    "\n"
	    "vec4 get_color(float stop_len);\n"
	    "\n"
	    "int t_invalid;\n"
	    "\n"
	    "float get_stop_len()\n"
	    "{\n"
	    "    float t = 0.0;\n"
	    "    float sqrt_value;\n"
	    "    int revserse = 0;\n"
	    "    t_invalid = 0;\n"
	    "    \n"
	    "    vec3 tmp = vec3(source_texture.x, source_texture.y, 1.0);\n"
	    "    vec3 source_texture_trans = transform_mat * tmp;\n"
	    "    source_texture_trans.xy = source_texture_trans.xy/source_texture_trans.z;\n"
	    "    float B_value = (source_texture_trans.x - c1.x) * (c2.x - c1.x)\n"
	    "                     + (source_texture_trans.y - c1.y) * (c2.y - c1.y)\n"
	    "                     + r1 * (r2 - r1);\n"
	    "    float C_value = (source_texture_trans.x - c1.x) * (source_texture_trans.x - c1.x)\n"
	    "                     + (source_texture_trans.y - c1.y) * (source_texture_trans.y - c1.y)\n"
	    "                     - r1*r1;\n"
	    "    if(abs(A_value) < 0.00001) {\n"
	    "        if(B_value == 0.0) {\n"
	    "            t_invalid = 1;\n"
	    "            return t;\n"
	    "        }\n"
	    "        t = 0.5 * C_value / B_value;"
	    "    } else {\n"
	    "        sqrt_value = B_value * B_value - A_value * C_value;\n"
	    "        if(sqrt_value < 0.0) {\n"
	    "            t_invalid = 1;\n"
	    "            return t;\n"
	    "        }\n"
	    "        sqrt_value = sqrt(sqrt_value);\n"
	    "        t = (B_value + sqrt_value) / A_value;\n"
	    "    }\n"
	    "    if(repeat_type == %d) {\n" // RepeatNone case.
	    "        if((t <= 0.0) || (t > 1.0))\n"
	    //           try another if first one invalid
	    "            t = (B_value - sqrt_value) / A_value;\n"
	    "        \n"
	    "        if((t <= 0.0) || (t > 1.0)) {\n" //still invalid, return.
	    "            t_invalid = 1;\n"
	    "            return t;\n"
	    "        }\n"
	    "    } else {\n"
	    "        if(t * (r2 - r1) <= -1.0 * r1)\n"
	    //           try another if first one invalid
	    "            t = (B_value - sqrt_value) / A_value;\n"
	    "        \n"
	    "        if(t * (r2 -r1) <= -1.0 * r1) {\n" //still invalid, return.
	    "            t_invalid = 1;\n"
	    "            return t;\n"
	    "        }\n"
	    "    }\n"
	    "    \n"
	    "    if(repeat_type == %d){\n" // repeat normal
	    "        while(t > 1.0) \n"
	    "            t = t - 1.0; \n"
	    "        while(t < 0.0) \n"
	    "            t = t + 1.0; \n"
	    "    }\n"
	    "    \n"
	    "    if(repeat_type == %d) {\n" // repeat reflect
	    "        while(t > 1.0) {\n"
	    "            t = t - 1.0; \n"
	    "            if(revserse == 0)\n"
	    "                revserse = 1;\n"
	    "            else\n"
	    "                revserse = 0;\n"
	    "        }\n"
	    "        while(t < 0.0) {\n"
	    "            t = t + 1.0; \n"
	    "            if(revserse == 0)\n"
	    "                revserse = 1;\n"
	    "            else\n"
	    "                revserse = 0;\n"
	    "        }\n"
	    "        if(revserse == 1) {\n"
	    "            t = 1.0 - t; \n"
	    "        }\n"
	    "    }\n"
	    "    \n"
	    "    return t;\n"
	    "}\n"
	    "\n"
	    "void main()\n"
	    "{\n"
	    "    float stop_len = get_stop_len();\n"
	    "    if(t_invalid == 1) {\n"
	    "        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n"
	    "    } else {\n"
	    "        gl_FragColor = get_color(stop_len);\n"
	    "    }\n"
	    "}\n";

	glamor_priv = glamor_get_screen_private(screen);

	if ((glamor_priv->radial_max_nstops >= stops_count) && (dyn_gen)) {
		/* Very Good, not to generate again. */
		return;
	}

	dispatch = glamor_get_dispatch(glamor_priv);

	if (dyn_gen && glamor_priv->gradient_prog[SHADER_GRADIENT_RADIAL][2]) {
		dispatch->glDeleteShader(
		    glamor_priv->radial_gradient_shaders[SHADER_GRADIENT_VS_PROG][2]);
		glamor_priv->radial_gradient_shaders[SHADER_GRADIENT_VS_PROG][2] = 0;

		dispatch->glDeleteShader(
		    glamor_priv->radial_gradient_shaders[SHADER_GRADIENT_FS_MAIN_PROG][2]);
		glamor_priv->radial_gradient_shaders[SHADER_GRADIENT_FS_MAIN_PROG][2] = 0;

		dispatch->glDeleteShader(
		    glamor_priv->radial_gradient_shaders[SHADER_GRADIENT_FS_GETCOLOR_PROG][2]);
		glamor_priv->radial_gradient_shaders[SHADER_GRADIENT_FS_GETCOLOR_PROG][2] = 0;

		dispatch->glDeleteProgram(glamor_priv->gradient_prog[SHADER_GRADIENT_RADIAL][2]);
		glamor_priv->gradient_prog[SHADER_GRADIENT_RADIAL][2] = 0;
	}

	gradient_prog = dispatch->glCreateProgram();

	vs_prog = glamor_compile_glsl_prog(dispatch,
	                                   GL_VERTEX_SHADER, gradient_vs);

	XNFasprintf(&gradient_fs,
	            gradient_fs_template,
	            PIXMAN_REPEAT_NONE, PIXMAN_REPEAT_NORMAL, PIXMAN_REPEAT_REFLECT);

	fs_main_prog = glamor_compile_glsl_prog(dispatch,
	                                        GL_FRAGMENT_SHADER, gradient_fs);

	free(gradient_fs);

	fs_getcolor_prog =
	    _glamor_create_getcolor_fs_program(screen, stops_count, (stops_count > 0));

	dispatch->glAttachShader(gradient_prog, vs_prog);
	dispatch->glAttachShader(gradient_prog, fs_getcolor_prog);
	dispatch->glAttachShader(gradient_prog, fs_main_prog);

	dispatch->glBindAttribLocation(gradient_prog, GLAMOR_VERTEX_POS, "v_positionsition");
	dispatch->glBindAttribLocation(gradient_prog, GLAMOR_VERTEX_SOURCE, "v_texcoord");

	glamor_link_glsl_prog(dispatch, gradient_prog);

	dispatch->glUseProgram(0);

	if (dyn_gen) {
		index = 2;
		glamor_priv->radial_max_nstops = stops_count;
	} else if (stops_count) {
		index = 1;
	} else {
		index = 0;
	}

	glamor_priv->gradient_prog[SHADER_GRADIENT_RADIAL][index] = gradient_prog;
	glamor_priv->radial_gradient_shaders[SHADER_GRADIENT_VS_PROG][index] = vs_prog;
	glamor_priv->radial_gradient_shaders[SHADER_GRADIENT_FS_MAIN_PROG][index] = fs_main_prog;
	glamor_priv->radial_gradient_shaders[SHADER_GRADIENT_FS_GETCOLOR_PROG][index] = fs_getcolor_prog;

	glamor_put_dispatch(glamor_priv);
}

static void
_glamor_create_linear_gradient_program(ScreenPtr screen, int stops_count, int dyn_gen)
{
	glamor_screen_private *glamor_priv;
	glamor_gl_dispatch *dispatch;

	int index = 0;
	GLint gradient_prog = 0;
	char *gradient_fs = NULL;
	GLint fs_main_prog, fs_getcolor_prog, vs_prog;

	const char *gradient_vs =
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
	 *                                      |
	 *                                      |\
	 *                                      | \
	 *                                      |  \
	 *                                      |   \
	 *                                      |\   \
	 *                                      | \   \
	 *     cos_val =                        |\ p1d \   /
	 *      sqrt(1/(slope*slope+1.0))  ------>\ \   \ /
	 *                                      |  \ \   \
	 *                                      |   \ \ / \
	 *                                      |    \ *Pt1\
	 *         *p1                          |     \     \     *P
	 *          \                           |    / \     \   /
	 *           \                          |   /   \     \ /
	 *            \                         |       pd     \
	 *             \                        |         \   / \
	 *            p2*                       |          \ /   \       /
	 *        slope = (p2.y - p1.y) /       |           /     p2d   /
	 *                    (p2.x - p1.x)     |          /       \   /
	 *                                      |         /         \ /
	 *                                      |        /           /
	 *                                      |       /           /
	 *                                      |      /           *Pt2
	 *                                      |                 /
	 *                                      |                /
	 *                                      |               /
	 *                                      |              /
	 *                                      |             /
	 *                               -------+---------------------------------
	 *                                     O|
	 *                                      |
	 *                                      |
	 *
	 *	step 1: compute the distance of p, pt1 and pt2 in the slope direction.
	 *		Caculate the distance on Y axis first and multiply cos_val to
	 *		get the value on slope direction(pd, p1d and p2d represent the
	 *		distance of p, pt1, and pt2 respectively).
	 *
	 *	step 2: caculate the percentage of (pd - p1d)/(p2d - p1d).
	 *		If (pd - p1d) > (p2d - p1d) or < 0, then sub or add (p2d - p1d)
	 *		to make it in the range of [0, (p2d - p1d)].
	 *
	 *	step 3: compare the percentage to every stop and find the stpos just
	 *		before and after it. Use the interpolation fomula to compute RGBA.
	 */

	const char *gradient_fs_template =
	    GLAMOR_DEFAULT_PRECISION
	    "uniform mat3 transform_mat;\n"
	    "uniform int repeat_type;\n"
	    "uniform int hor_ver;\n"
	    "uniform vec4 pt1;\n"
	    "uniform vec4 pt2;\n"
	    "uniform float pt_slope;\n"
	    "uniform float cos_val;\n"
	    "uniform float p1_distance;\n"
	    "uniform float pt_distance;\n"
	    "varying vec2 source_texture;\n"
	    "\n"
	    "vec4 get_color(float stop_len);\n"
	    "\n"
	    "float get_stop_len()\n"
	    "{\n"
	    "    vec3 tmp = vec3(source_texture.x, source_texture.y, 1.0);\n"
	    "    float len_percentage;\n"
	    "    float distance;\n"
	    "    float _p1_distance;\n"
	    "    float _pt_distance;\n"
	    "    float y_dist;\n"
	    "    float stop_after;\n"
	    "    float stop_before;\n"
	    "    vec4 stop_color_before;\n"
	    "    vec4 stop_color_after;\n"
	    "    float new_alpha; \n"
	    "    int revserse = 0;\n"
	    "    vec4 gradient_color;\n"
	    "    float percentage; \n"
	    "    vec3 source_texture_trans = transform_mat * tmp;\n"
	    "    \n"
	    "    if(hor_ver == 0) { \n" //Normal case.
	    "        y_dist = source_texture_trans.y - source_texture_trans.x*pt_slope;\n"
	    "        distance = y_dist * cos_val;\n"
	    "        _p1_distance = p1_distance * source_texture_trans.z;\n"
	    "        _pt_distance = pt_distance * source_texture_trans.z;\n"
	    "        \n"
	    "    } else if (hor_ver == 1) {\n"//horizontal case.
	    "        distance = source_texture_trans.x;\n"
	    "        _p1_distance = p1_distance * source_texture_trans.z;\n"
	    "        _pt_distance = pt_distance * source_texture_trans.z;\n"
	    "    } else if (hor_ver == 2) {\n"//vertical case.
	    "        distance = source_texture_trans.y;\n"
	    "        _p1_distance = p1_distance * source_texture_trans.z;\n"
	    "        _pt_distance = pt_distance * source_texture_trans.z;\n"
	    "    } \n"
	    "    \n"
	    "    distance = distance - _p1_distance; \n"
	    "    \n"
	    "    if(repeat_type == %d){\n" // repeat normal
	    "        while(distance > _pt_distance) \n"
	    "            distance = distance - (_pt_distance); \n"
	    "        while(distance < 0.0) \n"
	    "            distance = distance + (_pt_distance); \n"
	    "    }\n"
	    "    \n"
	    "    if(repeat_type == %d) {\n" // repeat reflect
	    "        while(distance > _pt_distance) {\n"
	    "            distance = distance - (_pt_distance); \n"
	    "            if(revserse == 0)\n"
	    "                revserse = 1;\n"
	    "            else\n"
	    "                revserse = 0;\n"
	    "        }\n"
	    "        while(distance < 0.0) {\n"
	    "            distance = distance + (_pt_distance); \n"
	    "            if(revserse == 0)\n"
	    "                revserse = 1;\n"
	    "            else\n"
	    "                revserse = 0;\n"
	    "        }\n"
	    "        if(revserse == 1) {\n"
	    "            distance = (_pt_distance) - distance; \n"
	    "        }\n"
	    "    }\n"
	    "    \n"
	    "    len_percentage = distance/(_pt_distance);\n"
	    "    \n"
	    "    return len_percentage;\n"
	    "}\n"
	    "\n"
	    "void main()\n"
	    "{\n"
	    "    float stop_len = get_stop_len();\n"
	    "    gl_FragColor = get_color(stop_len);\n"
	    "}\n";


	glamor_priv = glamor_get_screen_private(screen);

	if ((glamor_priv->linear_max_nstops >= stops_count) && (dyn_gen)) {
		/* Very Good, not to generate again. */
		return;
	}

	dispatch = glamor_get_dispatch(glamor_priv);
	if (dyn_gen && glamor_priv->gradient_prog[SHADER_GRADIENT_LINEAR][2]) {
		dispatch->glDeleteShader(
		    glamor_priv->linear_gradient_shaders[SHADER_GRADIENT_VS_PROG][2]);
		glamor_priv->linear_gradient_shaders[SHADER_GRADIENT_VS_PROG][2] = 0;

		dispatch->glDeleteShader(
		    glamor_priv->linear_gradient_shaders[SHADER_GRADIENT_FS_MAIN_PROG][2]);
		glamor_priv->linear_gradient_shaders[SHADER_GRADIENT_FS_MAIN_PROG][2] = 0;

		dispatch->glDeleteShader(
		    glamor_priv->linear_gradient_shaders[SHADER_GRADIENT_FS_GETCOLOR_PROG][2]);
		glamor_priv->linear_gradient_shaders[SHADER_GRADIENT_FS_GETCOLOR_PROG][2] = 0;

		dispatch->glDeleteProgram(glamor_priv->gradient_prog[SHADER_GRADIENT_LINEAR][2]);
		glamor_priv->gradient_prog[SHADER_GRADIENT_LINEAR][2] = 0;
	}

	gradient_prog = dispatch->glCreateProgram();

	vs_prog = glamor_compile_glsl_prog(dispatch,
	                                   GL_VERTEX_SHADER, gradient_vs);

	XNFasprintf(&gradient_fs,
	            gradient_fs_template,
	            PIXMAN_REPEAT_NORMAL, PIXMAN_REPEAT_REFLECT);

	fs_main_prog = glamor_compile_glsl_prog(dispatch,
	                                        GL_FRAGMENT_SHADER, gradient_fs);
	free(gradient_fs);

	fs_getcolor_prog =
	    _glamor_create_getcolor_fs_program(screen, stops_count, (stops_count > 0));

	dispatch->glAttachShader(gradient_prog, vs_prog);
	dispatch->glAttachShader(gradient_prog, fs_getcolor_prog);
	dispatch->glAttachShader(gradient_prog, fs_main_prog);

	dispatch->glBindAttribLocation(gradient_prog, GLAMOR_VERTEX_POS, "v_position");
	dispatch->glBindAttribLocation(gradient_prog, GLAMOR_VERTEX_SOURCE, "v_texcoord");

	glamor_link_glsl_prog(dispatch, gradient_prog);

	dispatch->glUseProgram(0);

	if (dyn_gen) {
		index = 2;
		glamor_priv->linear_max_nstops = stops_count;
	} else if (stops_count) {
		index = 1;
	} else {
		index = 0;
	}

	glamor_priv->gradient_prog[SHADER_GRADIENT_LINEAR][index] = gradient_prog;
	glamor_priv->linear_gradient_shaders[SHADER_GRADIENT_VS_PROG][index] = vs_prog;
	glamor_priv->linear_gradient_shaders[SHADER_GRADIENT_FS_MAIN_PROG][index] = fs_main_prog;
	glamor_priv->linear_gradient_shaders[SHADER_GRADIENT_FS_GETCOLOR_PROG][index] = fs_getcolor_prog;

	glamor_put_dispatch(glamor_priv);
}

#define LINEAR_SMALL_STOPS 6 + 2
#define LINEAR_LARGE_STOPS 16 + 2

#define RADIAL_SMALL_STOPS 6 + 2
#define RADIAL_LARGE_STOPS 16 + 2

void
glamor_init_gradient_shader(ScreenPtr screen)
{
	glamor_screen_private *glamor_priv;
	int i;

	glamor_priv = glamor_get_screen_private(screen);

	for (i = 0; i < 3; i++) {
		glamor_priv->gradient_prog[SHADER_GRADIENT_LINEAR][i] = 0;
		glamor_priv->linear_gradient_shaders[SHADER_GRADIENT_VS_PROG][i] = 0;
		glamor_priv->linear_gradient_shaders[SHADER_GRADIENT_FS_MAIN_PROG][i] = 0;
		glamor_priv->linear_gradient_shaders[SHADER_GRADIENT_FS_GETCOLOR_PROG][i] = 0;

		glamor_priv->gradient_prog[SHADER_GRADIENT_RADIAL][i] = 0;
		glamor_priv->radial_gradient_shaders[SHADER_GRADIENT_VS_PROG][i] = 0;
		glamor_priv->radial_gradient_shaders[SHADER_GRADIENT_FS_MAIN_PROG][i] = 0;
		glamor_priv->radial_gradient_shaders[SHADER_GRADIENT_FS_GETCOLOR_PROG][i] = 0;
	}
	glamor_priv->linear_max_nstops = 0;
	glamor_priv->radial_max_nstops = 0;

	_glamor_create_linear_gradient_program(screen, 0, 0);
	_glamor_create_linear_gradient_program(screen, LINEAR_LARGE_STOPS, 0);

	_glamor_create_radial_gradient_program(screen, 0, 0);
	_glamor_create_radial_gradient_program(screen, RADIAL_LARGE_STOPS, 0);
}

void
glamor_fini_gradient_shader(ScreenPtr screen)
{
	glamor_screen_private *glamor_priv;
	glamor_gl_dispatch *dispatch;
	int i = 0;

	glamor_priv = glamor_get_screen_private(screen);
	dispatch = glamor_get_dispatch(glamor_priv);

	for (i = 0; i < 3; i++) {
		/* Linear Gradient */
		if (glamor_priv->linear_gradient_shaders[SHADER_GRADIENT_VS_PROG][i])
			dispatch->glDeleteShader(
			    glamor_priv->linear_gradient_shaders[SHADER_GRADIENT_VS_PROG][i]);

		if (glamor_priv->linear_gradient_shaders[SHADER_GRADIENT_FS_MAIN_PROG][i])
			dispatch->glDeleteShader(
			    glamor_priv->linear_gradient_shaders[SHADER_GRADIENT_FS_MAIN_PROG][i]);

		if (glamor_priv->linear_gradient_shaders[SHADER_GRADIENT_FS_GETCOLOR_PROG][i])
			dispatch->glDeleteShader(
			    glamor_priv->linear_gradient_shaders[SHADER_GRADIENT_FS_GETCOLOR_PROG][i]);

		if (glamor_priv->gradient_prog[SHADER_GRADIENT_LINEAR][i])
			dispatch->glDeleteProgram(glamor_priv->gradient_prog[SHADER_GRADIENT_LINEAR][i]);

		/* Radial Gradient */
		if (glamor_priv->radial_gradient_shaders[SHADER_GRADIENT_VS_PROG][i])
			dispatch->glDeleteShader(
			    glamor_priv->radial_gradient_shaders[SHADER_GRADIENT_VS_PROG][i]);

		if (glamor_priv->radial_gradient_shaders[SHADER_GRADIENT_FS_MAIN_PROG][i])
			dispatch->glDeleteShader(
			    glamor_priv->radial_gradient_shaders[SHADER_GRADIENT_FS_MAIN_PROG][i]);

		if (glamor_priv->radial_gradient_shaders[SHADER_GRADIENT_FS_GETCOLOR_PROG][i])
			dispatch->glDeleteShader(
			    glamor_priv->radial_gradient_shaders[SHADER_GRADIENT_FS_GETCOLOR_PROG][i]);

		if (glamor_priv->gradient_prog[SHADER_GRADIENT_RADIAL][i])
			dispatch->glDeleteProgram(glamor_priv->gradient_prog[SHADER_GRADIENT_RADIAL][i]);
	}

	glamor_put_dispatch(glamor_priv);
}

static void
_glamor_gradient_convert_trans_matrix(PictTransform *from, float to[3][3],
				      int width, int height, int normalize)
{
	/*
	 * Because in the shader program, we normalize all the pixel cood to [0, 1],
	 * so with the transform matrix, the correct logic should be:
	 * v_s = A*T*v
	 * v_s: point vector in shader after normalized.
	 * A: The transition matrix from   width X height --> 1.0 X 1.0
	 * T: The transform matrix.
	 * v: point vector in width X height space.
	 *
	 * result is OK if we use this fomula. But for every point in width X height space,
	 * we can just use their normalized point vector in shader, namely we can just
	 * use the result of A*v in shader. So we have no chance to insert T in A*v.
	 * We can just convert v_s = A*T*v to v_s = A*T*inv(A)*A*v, where inv(A) is the
	 * inverse matrix of A. Now, v_s = (A*T*inv(A)) * (A*v)
	 * So, to get the correct v_s, we need to cacula1 the matrix: (A*T*inv(A)), and
	 * we name this matrix T_s.
	 *
	 * Firstly, because A is for the scale convertion, we find
	 *      --         --
	 *      |1/w  0   0 |
	 * A =  | 0  1/h  0 |
	 *      | 0   0  1.0|
	 *      --         --
	 * so T_s = A*T*inv(a) and result
	 *
	 *       --                      --
	 *       | t11      h*t12/w  t13/w|
	 * T_s = | w*t21/h  t22      t23/h|
	 *       | w*t31    h*t32    t33  |
	 *       --                      --
	 */

	to[0][0] = (float)pixman_fixed_to_double(from->matrix[0][0]);
	to[0][1] = (float)pixman_fixed_to_double(from->matrix[0][1])
	                        * (normalize ? (((float)height) / ((float)width)) : 1.0);
	to[0][2] = (float)pixman_fixed_to_double(from->matrix[0][2])
	                        / (normalize ? ((float)width) : 1.0);

	to[1][0] = (float)pixman_fixed_to_double(from->matrix[1][0])
	                        * (normalize ? (((float)width) / ((float)height)) : 1.0);
	to[1][1] = (float)pixman_fixed_to_double(from->matrix[1][1]);
	to[1][2] = (float)pixman_fixed_to_double(from->matrix[1][2])
	                        / (normalize ? ((float)height) : 1.0);

	to[2][0] = (float)pixman_fixed_to_double(from->matrix[2][0])
	                        * (normalize ? ((float)width) : 1.0);
	to[2][1] = (float)pixman_fixed_to_double(from->matrix[2][1])
	                        * (normalize ? ((float)height) : 1.0);
	to[2][2] = (float)pixman_fixed_to_double(from->matrix[2][2]);

	DEBUGF("the transform matrix is:\n%f\t%f\t%f\n%f\t%f\t%f\n%f\t%f\t%f\n",
	       to[0][0], to[0][1], to[0][2],
	       to[1][0], to[1][1], to[1][2],
	       to[2][0], to[2][1], to[2][2]);
}

static int
_glamor_gradient_set_pixmap_destination(ScreenPtr screen,
                                        glamor_screen_private *glamor_priv,
                                        PicturePtr dst_picture,
                                        GLfloat *xscale, GLfloat *yscale,
                                        int x_source, int y_source,
                                        float vertices[8],
                                        float tex_vertices[8],
					int tex_normalize)
{
	glamor_pixmap_private *pixmap_priv;
	PixmapPtr pixmap = NULL;

	pixmap = glamor_get_drawable_pixmap(dst_picture->pDrawable);
	pixmap_priv = glamor_get_pixmap_private(pixmap);

	if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv)) { /* should always have here. */
		return 0;
	}

	glamor_set_destination_pixmap_priv_nc(pixmap_priv);

	pixmap_priv_get_scale(pixmap_priv, xscale, yscale);

	glamor_priv->has_source_coords = 1;
	glamor_priv->has_mask_coords = 0;
	glamor_setup_composite_vbo(screen, 4*2);

	DEBUGF("xscale = %f, yscale = %f,"
	       " x_source = %d, y_source = %d, width = %d, height = %d\n",
	       *xscale, *yscale, x_source, y_source,
	       dst_picture->pDrawable->width, dst_picture->pDrawable->height);

	glamor_set_normalize_vcoords(*xscale, *yscale,
	                             0, 0,
	                             (INT16)(dst_picture->pDrawable->width),
	                             (INT16)(dst_picture->pDrawable->height),
	                             glamor_priv->yInverted, vertices);

	if (tex_normalize) {
		glamor_set_normalize_tcoords(*xscale, *yscale,
		                             0, 0,
		                             (INT16)(dst_picture->pDrawable->width),
		                             (INT16)(dst_picture->pDrawable->height),
		                             glamor_priv->yInverted, tex_vertices);
	} else {
		glamor_set_tcoords(0, 0,
		                   (INT16)(dst_picture->pDrawable->width),
		                   (INT16)(dst_picture->pDrawable->height),
		                   glamor_priv->yInverted, tex_vertices);
	}

	DEBUGF("vertices --> leftup : %f X %f, rightup: %f X %f,"
	       "rightbottom: %f X %f, leftbottom : %f X %f\n",
	       vertices[0], vertices[1], vertices[2], vertices[3],
	       vertices[4], vertices[5], vertices[6], vertices[7]);
	DEBUGF("tex_vertices --> leftup : %f X %f, rightup: %f X %f,"
	       "rightbottom: %f X %f, leftbottom : %f X %f\n",
	       tex_vertices[0], tex_vertices[1], tex_vertices[2], tex_vertices[3],
	       tex_vertices[4], tex_vertices[5], tex_vertices[6], tex_vertices[7]);

	return 1;
}

static int
_glamor_gradient_set_stops(PicturePtr src_picture, PictGradient * pgradient,
        GLfloat *stop_colors, GLfloat *n_stops)
{
	int i;
	int count = 1;

	for (i = 0; i < pgradient->nstops; i++) {
		/* We find some gradient picture set the stops at the same percentage, which
		   will cause the shader problem because the (stops[i] - stops[i-1]) will
		   be used as divisor. We just keep the later one if stops[i] == stops[i-1] */
		if (i < pgradient->nstops - 1
		         && pgradient->stops[i].x == pgradient->stops[i+1].x)
			continue;

		stop_colors[count*4] = pixman_fixed_to_double(
		                                pgradient->stops[i].color.red);
		stop_colors[count*4+1] = pixman_fixed_to_double(
		                                pgradient->stops[i].color.green);
		stop_colors[count*4+2] = pixman_fixed_to_double(
		                                pgradient->stops[i].color.blue);
		stop_colors[count*4+3] = pixman_fixed_to_double(
		                                pgradient->stops[i].color.alpha);

		n_stops[count] = (GLfloat)pixman_fixed_to_double(
		                                pgradient->stops[i].x);
		count++;
	}

	/* for the end stop. */
	count++;

	switch (src_picture->repeatType) {
#define REPEAT_FILL_STOPS(m, n) \
			stop_colors[(m)*4 + 0] = stop_colors[(n)*4 + 0]; \
			stop_colors[(m)*4 + 1] = stop_colors[(n)*4 + 1]; \
			stop_colors[(m)*4 + 2] = stop_colors[(n)*4 + 2]; \
			stop_colors[(m)*4 + 3] = stop_colors[(n)*4 + 3];

		default:
		case PIXMAN_REPEAT_NONE:
			stop_colors[0] = 0.0;	   //R
			stop_colors[1] = 0.0;	   //G
			stop_colors[2] = 0.0;	   //B
			stop_colors[3] = 0.0;	   //Alpha
			n_stops[0] = -(float)INT_MAX;  //should be small enough.

			stop_colors[0 + (count-1)*4] = 0.0;	 //R
			stop_colors[1 + (count-1)*4] = 0.0;	 //G
			stop_colors[2 + (count-1)*4] = 0.0;	 //B
			stop_colors[3 + (count-1)*4] = 0.0;	 //Alpha
			n_stops[count-1] = (float)INT_MAX;  //should be large enough.
			break;
		case PIXMAN_REPEAT_NORMAL:
			REPEAT_FILL_STOPS(0, count - 2);
			n_stops[0] = n_stops[count-2] - 1.0;

			REPEAT_FILL_STOPS(count - 1, 1);
			n_stops[count-1] = n_stops[1] + 1.0;
			break;
		case PIXMAN_REPEAT_REFLECT:
			REPEAT_FILL_STOPS(0, 1);
			n_stops[0] = -n_stops[1];

			REPEAT_FILL_STOPS(count - 1, count - 2);
			n_stops[count-1] = 1.0 + 1.0 - n_stops[count-2];
			break;
		case PIXMAN_REPEAT_PAD:
			REPEAT_FILL_STOPS(0, 1);
			n_stops[0] = -(float)INT_MAX;

			REPEAT_FILL_STOPS(count - 1, count - 2);
			n_stops[count-1] = (float)INT_MAX;
			break;
#undef REPEAT_FILL_STOPS
	}

	for (i = 0; i < count; i++) {
		DEBUGF("n_stops[%d] = %f, color = r:%f g:%f b:%f a:%f\n",
		       i, n_stops[i],
		       stop_colors[i*4], stop_colors[i*4+1],
		       stop_colors[i*4+2], stop_colors[i*4+3]);
	}

	return count;
}

static PicturePtr
_glamor_generate_radial_gradient_picture(ScreenPtr screen,
                                         PicturePtr src_picture,
                                         int x_source, int y_source,
                                         int width, int height,
                                         PictFormatShort format)
{
	glamor_screen_private *glamor_priv;
	glamor_gl_dispatch *dispatch;
	PicturePtr dst_picture = NULL;
	PixmapPtr pixmap = NULL;
	GLint gradient_prog = 0;
	int error;
	float tex_vertices[8];
	int stops_count;
	int count = 0;
	GLfloat *stop_colors = NULL;
	GLfloat *n_stops = NULL;
	GLfloat xscale, yscale;
	float vertices[8];
	float transform_mat[3][3];
	static const float identity_mat[3][3] = {{1.0, 0.0, 0.0},
	                                         {0.0, 1.0, 0.0},
	                                         {0.0, 0.0, 1.0}};
	GLfloat stop_colors_st[RADIAL_SMALL_STOPS*4];
	GLfloat n_stops_st[RADIAL_SMALL_STOPS];
	GLfloat A_value;
	GLfloat cxy[4];
	float c1x, c1y, c2x, c2y, r1, r2;

	GLint transform_mat_uniform_location;
	GLint repeat_type_uniform_location;
	GLint n_stop_uniform_location;
	GLint stops_uniform_location;
	GLint stop_colors_uniform_location;
	GLint stop0_uniform_location;
	GLint stop1_uniform_location;
	GLint stop2_uniform_location;
	GLint stop3_uniform_location;
	GLint stop4_uniform_location;
	GLint stop5_uniform_location;
	GLint stop6_uniform_location;
	GLint stop7_uniform_location;
	GLint stop_color0_uniform_location;
	GLint stop_color1_uniform_location;
	GLint stop_color2_uniform_location;
	GLint stop_color3_uniform_location;
	GLint stop_color4_uniform_location;
	GLint stop_color5_uniform_location;
	GLint stop_color6_uniform_location;
	GLint stop_color7_uniform_location;
	GLint A_value_uniform_location;
	GLint c1_uniform_location;
	GLint r1_uniform_location;
	GLint c2_uniform_location;
	GLint r2_uniform_location;

	glamor_priv = glamor_get_screen_private(screen);
	dispatch = glamor_get_dispatch(glamor_priv);

	/* Create a pixmap with VBO. */
	pixmap = glamor_create_pixmap(screen,
	                              width, height,
	                              PIXMAN_FORMAT_DEPTH(format),
	                              0);
	if (!pixmap)
		goto GRADIENT_FAIL;

	dst_picture = CreatePicture(0, &pixmap->drawable,
	                            PictureMatchFormat(screen,
	                                 PIXMAN_FORMAT_DEPTH(format), format),
	                            0, 0, serverClient, &error);

	/* Release the reference, picture will hold the last one. */
	glamor_destroy_pixmap(pixmap);

	if (!dst_picture)
		goto GRADIENT_FAIL;

	ValidatePicture(dst_picture);

	stops_count = src_picture->pSourcePict->radial.nstops + 2;

	/* Because the max value of nstops is unkown, so create a program
	   when nstops > LINEAR_LARGE_STOPS.*/
	if (stops_count <= RADIAL_SMALL_STOPS) {
		gradient_prog = glamor_priv->gradient_prog[SHADER_GRADIENT_RADIAL][0];
	} else if (stops_count <= RADIAL_LARGE_STOPS) {
		gradient_prog = glamor_priv->gradient_prog[SHADER_GRADIENT_RADIAL][1];
	} else {
		_glamor_create_radial_gradient_program(screen, src_picture->pSourcePict->linear.nstops + 2, 1);
		gradient_prog = glamor_priv->gradient_prog[SHADER_GRADIENT_RADIAL][2];
	}

	/* Bind all the uniform vars .*/
	transform_mat_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "transform_mat");
	repeat_type_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "repeat_type");
	n_stop_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "n_stop");
	A_value_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "A_value");
	repeat_type_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "repeat_type");
	c1_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "c1");
	r1_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "r1");
	c2_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "c2");
	r2_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "r2");

	if (src_picture->pSourcePict->radial.nstops + 2 <= RADIAL_SMALL_STOPS) {
		stop0_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop0");
		stop1_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop1");
		stop2_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop2");
		stop3_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop3");
		stop4_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop4");
		stop5_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop5");
		stop6_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop6");
		stop7_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop7");

		stop_color0_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop_color0");
		stop_color1_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop_color1");
		stop_color2_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop_color2");
		stop_color3_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop_color3");
		stop_color4_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop_color4");
		stop_color5_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop_color5");
		stop_color6_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop_color6");
		stop_color7_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop_color7");
	} else {
		stops_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stops");
		stop_colors_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop_colors");
	}

	dispatch->glUseProgram(gradient_prog);

	dispatch->glUniform1i(repeat_type_uniform_location, src_picture->repeatType);


	if (src_picture->transform) {
		_glamor_gradient_convert_trans_matrix(src_picture->transform,
		                                      transform_mat,
		                                      width, height, 0);
		dispatch->glUniformMatrix3fv(transform_mat_uniform_location,
		                             1, 1, &transform_mat[0][0]);
	} else {
		dispatch->glUniformMatrix3fv(transform_mat_uniform_location,
		                             1, 1, &identity_mat[0][0]);
	}

	if (!_glamor_gradient_set_pixmap_destination(screen, glamor_priv, dst_picture,
	                                             &xscale, &yscale, x_source, y_source,
	                                             vertices, tex_vertices, 0))
		goto GRADIENT_FAIL;

	/* Set all the stops and colors to shader. */
	if (stops_count > RADIAL_SMALL_STOPS) {
		stop_colors = malloc(4 * stops_count * sizeof(float));
		if (stop_colors == NULL) {
			ErrorF("Failed to allocate stop_colors memory.\n");
			goto GRADIENT_FAIL;
		}

		n_stops = malloc(stops_count * sizeof(float));
		if (n_stops == NULL) {
			ErrorF("Failed to allocate n_stops memory.\n");
			goto GRADIENT_FAIL;
		}
	} else {
		stop_colors = stop_colors_st;
		n_stops = n_stops_st;
	}

	count = _glamor_gradient_set_stops(src_picture, &src_picture->pSourcePict->gradient,
	                                   stop_colors, n_stops);

	if (src_picture->pSourcePict->linear.nstops + 2 <= RADIAL_SMALL_STOPS) {
		int j = 0;
		dispatch->glUniform4f(stop_color0_uniform_location, stop_colors[4*j+0], stop_colors[4*j+1],
		                      stop_colors[4*j+2], stop_colors[4*j+3]);
		j++;
		dispatch->glUniform4f(stop_color1_uniform_location, stop_colors[4*j+0], stop_colors[4*j+1],
		                      stop_colors[4*j+2], stop_colors[4*j+3]);
		j++;
		dispatch->glUniform4f(stop_color2_uniform_location, stop_colors[4*j+0], stop_colors[4*j+1],
		                      stop_colors[4*j+2], stop_colors[4*j+3]);
		j++;
		dispatch->glUniform4f(stop_color3_uniform_location, stop_colors[4*j+0], stop_colors[4*j+1],
		                      stop_colors[4*j+2], stop_colors[4*j+3]);
		j++;
		dispatch->glUniform4f(stop_color4_uniform_location, stop_colors[4*j+0], stop_colors[4*j+1],
		                      stop_colors[4*j+2], stop_colors[4*j+3]);
		j++;
		dispatch->glUniform4f(stop_color5_uniform_location, stop_colors[4*j+0], stop_colors[4*j+1],
		                      stop_colors[4*j+2], stop_colors[4*j+3]);
		j++;
		dispatch->glUniform4f(stop_color6_uniform_location, stop_colors[4*j+0], stop_colors[4*j+1],
		                      stop_colors[4*j+2], stop_colors[4*j+3]);
		j++;
		dispatch->glUniform4f(stop_color7_uniform_location, stop_colors[4*j+0], stop_colors[4*j+1],
		                      stop_colors[4*j+2], stop_colors[4*j+3]);

		j = 0;
		dispatch->glUniform1f(stop0_uniform_location, n_stops[j++]);
		dispatch->glUniform1f(stop1_uniform_location, n_stops[j++]);
		dispatch->glUniform1f(stop2_uniform_location, n_stops[j++]);
		dispatch->glUniform1f(stop3_uniform_location, n_stops[j++]);
		dispatch->glUniform1f(stop4_uniform_location, n_stops[j++]);
		dispatch->glUniform1f(stop5_uniform_location, n_stops[j++]);
		dispatch->glUniform1f(stop6_uniform_location, n_stops[j++]);
		dispatch->glUniform1f(stop7_uniform_location, n_stops[j++]);
		dispatch->glUniform1i(n_stop_uniform_location, count);
	} else {
		dispatch->glUniform4fv(stop_colors_uniform_location, count, stop_colors);
		dispatch->glUniform1fv(stops_uniform_location, count, n_stops);
		dispatch->glUniform1i(n_stop_uniform_location, count);
	}

	c1x = (float)pixman_fixed_to_double(src_picture->pSourcePict->radial.c1.x);
	c1y = (float)pixman_fixed_to_double(src_picture->pSourcePict->radial.c1.y);
	c2x = (float)pixman_fixed_to_double(src_picture->pSourcePict->radial.c2.x);
	c2y = (float)pixman_fixed_to_double(src_picture->pSourcePict->radial.c2.y);

	r1 = (float)pixman_fixed_to_double(src_picture->pSourcePict->radial.c1.radius);
	r2 = (float)pixman_fixed_to_double(src_picture->pSourcePict->radial.c2.radius);


	cxy[0] = c1x;
	cxy[1] = c1y;
	dispatch->glUniform2fv(c1_uniform_location, 1, cxy);
	dispatch->glUniform1f(r1_uniform_location, r1);

	cxy[0] = c2x;
	cxy[1] = c2y;
	dispatch->glUniform2fv(c2_uniform_location, 1, cxy);
	dispatch->glUniform1f(r2_uniform_location, r2);

	A_value = (c2x - c1x) * (c2x - c1x) + (c2y - c1y) * (c2y - c1y) - (r2 - r1) * (r2 - r1);
	dispatch->glUniform1f(A_value_uniform_location, A_value);

	DEBUGF("C1:(%f, %f) R1:%f\nC2:(%f, %f) R2:%f\nA = %f\n",
	       c1x, c1y, r1, c2x, c2y, r2, A_value);

	glamor_emit_composite_rect(screen, tex_vertices, NULL, vertices);

	if (glamor_priv->render_nr_verts) {
		if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP)
			dispatch->glUnmapBuffer(GL_ARRAY_BUFFER);
		else {

			dispatch->glBindBuffer(GL_ARRAY_BUFFER, glamor_priv->vbo);
			dispatch->glBufferData(GL_ARRAY_BUFFER,
			                       glamor_priv->vbo_offset,
			                       glamor_priv->vb, GL_DYNAMIC_DRAW);
		}

		dispatch->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, NULL);
	}


	/* Do the clear logic.*/
	if (stops_count > RADIAL_SMALL_STOPS) {
		free(n_stops);
		free(stop_colors);
	}

	dispatch->glBindBuffer(GL_ARRAY_BUFFER, 0);
	dispatch->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
	dispatch->glUseProgram(0);

	glamor_put_dispatch(glamor_priv);
	return dst_picture;

GRADIENT_FAIL:
	if (dst_picture) {
		FreePicture(dst_picture, 0);
	}

	if (stops_count > RADIAL_SMALL_STOPS) {
		if (n_stops)
			free(n_stops);
		if (stop_colors)
			free(stop_colors);
	}

	dispatch->glBindBuffer(GL_ARRAY_BUFFER, 0);
	dispatch->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
	dispatch->glUseProgram(0);
	glamor_put_dispatch(glamor_priv);
	return NULL;
}

static PicturePtr
_glamor_generate_linear_gradient_picture(ScreenPtr screen,
                                         PicturePtr src_picture,
                                         int x_source, int y_source,
                                         int width, int height,
                                         PictFormatShort format)
{
	glamor_screen_private *glamor_priv;
	glamor_gl_dispatch *dispatch;
	PicturePtr dst_picture = NULL;
	PixmapPtr pixmap = NULL;
	GLint gradient_prog = 0;
	int error;
	float pt_distance;
	float p1_distance;
	GLfloat cos_val;
	float tex_vertices[8];
	int stops_count;
	GLfloat *stop_colors = NULL;
	GLfloat *n_stops = NULL;
	int count = 0;
	float slope;
	GLfloat xscale, yscale;
	GLfloat pt1[4], pt2[4];
	float vertices[8];
	float transform_mat[3][3];
	static const float identity_mat[3][3] = {{1.0, 0.0, 0.0},
	                                         {0.0, 1.0, 0.0},
	                                         {0.0, 0.0, 1.0}};
	GLfloat stop_colors_st[LINEAR_SMALL_STOPS*4];
	GLfloat n_stops_st[LINEAR_SMALL_STOPS];

	GLint transform_mat_uniform_location;
	GLint pt1_uniform_location;
	GLint pt2_uniform_location;
	GLint n_stop_uniform_location;
	GLint stops_uniform_location;
	GLint stop0_uniform_location;
	GLint stop1_uniform_location;
	GLint stop2_uniform_location;
	GLint stop3_uniform_location;
	GLint stop4_uniform_location;
	GLint stop5_uniform_location;
	GLint stop6_uniform_location;
	GLint stop7_uniform_location;
	GLint stop_colors_uniform_location;
	GLint stop_color0_uniform_location;
	GLint stop_color1_uniform_location;
	GLint stop_color2_uniform_location;
	GLint stop_color3_uniform_location;
	GLint stop_color4_uniform_location;
	GLint stop_color5_uniform_location;
	GLint stop_color6_uniform_location;
	GLint stop_color7_uniform_location;
	GLint pt_slope_uniform_location;
	GLint repeat_type_uniform_location;
	GLint hor_ver_uniform_location;
	GLint cos_val_uniform_location;
	GLint p1_distance_uniform_location;
	GLint pt_distance_uniform_location;

	glamor_priv = glamor_get_screen_private(screen);
	dispatch = glamor_get_dispatch(glamor_priv);

	/* Create a pixmap with VBO. */
	pixmap = glamor_create_pixmap(screen,
	                              width, height,
	                              PIXMAN_FORMAT_DEPTH(format),
	                              0);

	if (!pixmap)
		goto GRADIENT_FAIL;

	dst_picture = CreatePicture(0, &pixmap->drawable,
	                            PictureMatchFormat(screen,
	                                    PIXMAN_FORMAT_DEPTH(format), format),
	                            0, 0, serverClient, &error);

	/* Release the reference, picture will hold the last one. */
	glamor_destroy_pixmap(pixmap);

	if (!dst_picture)
		goto GRADIENT_FAIL;

	ValidatePicture(dst_picture);

	stops_count = src_picture->pSourcePict->linear.nstops + 2;

	/* Because the max value of nstops is unkown, so create a program
	   when nstops > LINEAR_LARGE_STOPS.*/
	if (stops_count <= LINEAR_SMALL_STOPS) {
		gradient_prog = glamor_priv->gradient_prog[SHADER_GRADIENT_LINEAR][0];
	} else if (stops_count <= LINEAR_LARGE_STOPS) {
		gradient_prog = glamor_priv->gradient_prog[SHADER_GRADIENT_LINEAR][1];
	} else {
		_glamor_create_linear_gradient_program(screen,
		        src_picture->pSourcePict->linear.nstops + 2, 1);
		gradient_prog = glamor_priv->gradient_prog[SHADER_GRADIENT_LINEAR][2];
	}

	/* Bind all the uniform vars .*/
	pt1_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "pt1");
	pt2_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "pt2");
	n_stop_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "n_stop");
	pt_slope_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "pt_slope");
	repeat_type_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "repeat_type");
	hor_ver_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "hor_ver");
	transform_mat_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "transform_mat");
	cos_val_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "cos_val");
	p1_distance_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "p1_distance");
	pt_distance_uniform_location =
	    dispatch->glGetUniformLocation(gradient_prog, "pt_distance");

	if (src_picture->pSourcePict->linear.nstops + 2 <= LINEAR_SMALL_STOPS) {
		stop0_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop0");
		stop1_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop1");
		stop2_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop2");
		stop3_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop3");
		stop4_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop4");
		stop5_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop5");
		stop6_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop6");
		stop7_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop7");

		stop_color0_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop_color0");
		stop_color1_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop_color1");
		stop_color2_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop_color2");
		stop_color3_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop_color3");
		stop_color4_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop_color4");
		stop_color5_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop_color5");
		stop_color6_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop_color6");
		stop_color7_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop_color7");
	} else {
		stops_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stops");
		stop_colors_uniform_location =
		    dispatch->glGetUniformLocation(gradient_prog, "stop_colors");
	}

	dispatch->glUseProgram(gradient_prog);

	dispatch->glUniform1i(repeat_type_uniform_location, src_picture->repeatType);

	if (src_picture->transform) {
		_glamor_gradient_convert_trans_matrix(src_picture->transform,
		                                      transform_mat,
		                                      width, height, 1);
		dispatch->glUniformMatrix3fv(transform_mat_uniform_location,
		                             1, 1, &transform_mat[0][0]);
	} else {
		dispatch->glUniformMatrix3fv(transform_mat_uniform_location,
		                             1, 1, &identity_mat[0][0]);
	}

	if (!_glamor_gradient_set_pixmap_destination(screen, glamor_priv, dst_picture,
	                                             &xscale, &yscale, x_source, y_source,
	                                             vertices, tex_vertices, 1))
		goto GRADIENT_FAIL;

	/* Normalize the PTs. */
	glamor_set_normalize_pt(xscale, yscale,
	                        pixman_fixed_to_int(src_picture->pSourcePict->linear.p1.x),
	                        x_source,
	                        pixman_fixed_to_int(src_picture->pSourcePict->linear.p1.y),
	                        y_source,
	                        glamor_priv->yInverted,
	                        pt1);
	dispatch->glUniform4fv(pt1_uniform_location, 1, pt1);
	DEBUGF("pt1:(%f %f)\n", pt1[0], pt1[1]);

	glamor_set_normalize_pt(xscale, yscale,
	                        pixman_fixed_to_int(src_picture->pSourcePict->linear.p2.x),
	                        x_source,
	                        pixman_fixed_to_int(src_picture->pSourcePict->linear.p2.y),
	                        y_source,
	                        glamor_priv->yInverted,
	                        pt2);
	dispatch->glUniform4fv(pt2_uniform_location, 1, pt2);
	DEBUGF("pt2:(%f %f)\n", pt2[0], pt2[1]);

	/* Set all the stops and colors to shader. */
	if (stops_count > LINEAR_SMALL_STOPS) {
		stop_colors = malloc(4 * stops_count * sizeof(float));
		if (stop_colors == NULL) {
			ErrorF("Failed to allocate stop_colors memory.\n");
			goto GRADIENT_FAIL;
		}

		n_stops = malloc(stops_count * sizeof(float));
		if (n_stops == NULL) {
			ErrorF("Failed to allocate n_stops memory.\n");
			goto GRADIENT_FAIL;
		}
	} else {
		stop_colors = stop_colors_st;
		n_stops = n_stops_st;
	}

	count = _glamor_gradient_set_stops(src_picture, &src_picture->pSourcePict->gradient,
	                                   stop_colors, n_stops);

	if (src_picture->pSourcePict->linear.nstops + 2 <= LINEAR_SMALL_STOPS) {
		int j = 0;
		dispatch->glUniform4f(stop_color0_uniform_location, stop_colors[4*j+0], stop_colors[4*j+1],
		                      stop_colors[4*j+2], stop_colors[4*j+3]);
		j++;
		dispatch->glUniform4f(stop_color1_uniform_location, stop_colors[4*j+0], stop_colors[4*j+1],
		                      stop_colors[4*j+2], stop_colors[4*j+3]);
		j++;
		dispatch->glUniform4f(stop_color2_uniform_location, stop_colors[4*j+0], stop_colors[4*j+1],
		                      stop_colors[4*j+2], stop_colors[4*j+3]);
		j++;
		dispatch->glUniform4f(stop_color3_uniform_location, stop_colors[4*j+0], stop_colors[4*j+1],
		                      stop_colors[4*j+2], stop_colors[4*j+3]);
		j++;
		dispatch->glUniform4f(stop_color4_uniform_location, stop_colors[4*j+0], stop_colors[4*j+1],
		                      stop_colors[4*j+2], stop_colors[4*j+3]);
		j++;
		dispatch->glUniform4f(stop_color5_uniform_location, stop_colors[4*j+0], stop_colors[4*j+1],
		                      stop_colors[4*j+2], stop_colors[4*j+3]);
		j++;
		dispatch->glUniform4f(stop_color6_uniform_location, stop_colors[4*j+0], stop_colors[4*j+1],
		                      stop_colors[4*j+2], stop_colors[4*j+3]);
		j++;
		dispatch->glUniform4f(stop_color7_uniform_location, stop_colors[4*j+0], stop_colors[4*j+1],
		                      stop_colors[4*j+2], stop_colors[4*j+3]);

		j = 0;
		dispatch->glUniform1f(stop0_uniform_location, n_stops[j++]);
		dispatch->glUniform1f(stop1_uniform_location, n_stops[j++]);
		dispatch->glUniform1f(stop2_uniform_location, n_stops[j++]);
		dispatch->glUniform1f(stop3_uniform_location, n_stops[j++]);
		dispatch->glUniform1f(stop4_uniform_location, n_stops[j++]);
		dispatch->glUniform1f(stop5_uniform_location, n_stops[j++]);
		dispatch->glUniform1f(stop6_uniform_location, n_stops[j++]);
		dispatch->glUniform1f(stop7_uniform_location, n_stops[j++]);

		dispatch->glUniform1i(n_stop_uniform_location, count);
	} else {
		dispatch->glUniform4fv(stop_colors_uniform_location, count, stop_colors);
		dispatch->glUniform1fv(stops_uniform_location, count, n_stops);
		dispatch->glUniform1i(n_stop_uniform_location, count);
	}

	if ((pt2[1] - pt1[1]) / yscale < 1.0) { // The horizontal case.
		dispatch->glUniform1i(hor_ver_uniform_location, 1);
		DEBUGF("p1.x: %f, p2.x: %f, enter the horizontal case\n", pt1[1], pt2[1]);

		p1_distance = pt1[0];
		pt_distance = (pt2[0] - p1_distance);
		dispatch->glUniform1f(p1_distance_uniform_location, p1_distance);
		dispatch->glUniform1f(pt_distance_uniform_location, pt_distance);
	} else if ((pt2[0] - pt1[0]) / xscale < 1.0) { //The vertical case.
		dispatch->glUniform1i(hor_ver_uniform_location, 2);
		DEBUGF("p1.y: %f, p2.y: %f, enter the vertical case\n", pt1[0], pt2[0]);

		p1_distance = pt1[1];
		pt_distance = (pt2[1] - p1_distance);
		dispatch->glUniform1f(p1_distance_uniform_location, p1_distance);
		dispatch->glUniform1f(pt_distance_uniform_location, pt_distance);
	} else {
		/* The slope need to compute here. In shader, the viewport set will change
		   the orginal slope and the slope which is vertical to it will not be correct.*/
		slope = - (float)(src_picture->pSourcePict->linear.p2.x - src_picture->pSourcePict->linear.p1.x) /
		        (float)(src_picture->pSourcePict->linear.p2.y - src_picture->pSourcePict->linear.p1.y);
		slope = slope * yscale / xscale;
		dispatch->glUniform1f(pt_slope_uniform_location, slope);
		dispatch->glUniform1i(hor_ver_uniform_location, 0);

		cos_val = sqrt(1.0 / (slope * slope + 1.0));
		dispatch->glUniform1f(cos_val_uniform_location, cos_val);

		p1_distance = (pt1[1] - pt1[0] * slope) * cos_val;
		pt_distance = (pt2[1] - pt2[0] * slope) * cos_val - p1_distance;
		dispatch->glUniform1f(p1_distance_uniform_location, p1_distance);
		dispatch->glUniform1f(pt_distance_uniform_location, pt_distance);
	}

	/* set the transform matrix. */	/* Now rendering. */
	glamor_emit_composite_rect(screen, tex_vertices, NULL, vertices);

	if (glamor_priv->render_nr_verts) {
		if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP)
			dispatch->glUnmapBuffer(GL_ARRAY_BUFFER);
		else {

			dispatch->glBindBuffer(GL_ARRAY_BUFFER, glamor_priv->vbo);
			dispatch->glBufferData(GL_ARRAY_BUFFER,
			                       glamor_priv->vbo_offset,
			                       glamor_priv->vb, GL_DYNAMIC_DRAW);
		}

		dispatch->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, NULL);
	}

	/* Do the clear logic.*/
	if (stops_count > LINEAR_SMALL_STOPS) {
		free(n_stops);
		free(stop_colors);
	}

	dispatch->glBindBuffer(GL_ARRAY_BUFFER, 0);
	dispatch->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
	dispatch->glUseProgram(0);

	glamor_put_dispatch(glamor_priv);
	return dst_picture;

GRADIENT_FAIL:
	if (dst_picture) {
		FreePicture(dst_picture, 0);
	}

	if (stops_count > LINEAR_SMALL_STOPS) {
		if (n_stops)
			free(n_stops);
		if (stop_colors)
			free(stop_colors);
	}

	dispatch->glBindBuffer(GL_ARRAY_BUFFER, 0);
	dispatch->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
	dispatch->glUseProgram(0);
	glamor_put_dispatch(glamor_priv);
	return NULL;
}
#undef LINEAR_DEFAULT_STOPS
#endif

static PicturePtr
glamor_convert_gradient_picture(ScreenPtr screen,
                                PicturePtr source,
                                int x_source,
                                int y_source, int width, int height)
{
	PixmapPtr pixmap;
	PicturePtr dst = NULL;
	int error;
	PictFormatShort format;
	if (!source->pDrawable)
		format = PICT_a8r8g8b8;
	else
		format = source->format;
#ifdef GLAMOR_GRADIENT_SHADER
	if (!source->pDrawable) {
		if (source->pSourcePict->type == SourcePictTypeLinear) {
			dst = _glamor_generate_linear_gradient_picture(screen,
				source, x_source, y_source, width, height, format);
		} else if (source->pSourcePict->type == SourcePictTypeRadial) {
			dst = _glamor_generate_radial_gradient_picture(screen,
		                  source, x_source, y_source, width, height, format);
		}

		if (dst) {
#if 0			/* Debug to compare it to pixman, Enable it if needed. */
			glamor_compare_pictures(screen, source,
					dst, x_source, y_source, width, height,
					0, 3);
#endif
			return dst;
		}
	}
#endif
	pixmap = glamor_create_pixmap(screen,
				      width,
				      height,
				      PIXMAN_FORMAT_DEPTH(format),
				      GLAMOR_CREATE_PIXMAP_CPU);

	if (!pixmap)
		return NULL;

	dst = CreatePicture(0,
	                    &pixmap->drawable,
	                    PictureMatchFormat(screen,
	                                       PIXMAN_FORMAT_DEPTH(format),
	                                       format),
	                    0, 0, serverClient, &error);
	glamor_destroy_pixmap(pixmap);
	if (!dst)
		return NULL;

	ValidatePicture(dst);

	fbComposite(PictOpSrc, source, NULL, dst, x_source, y_source,
	            0, 0, 0, 0, width, height);
	return dst;
}

static Bool
_glamor_composite(CARD8 op,
		  PicturePtr source,
		  PicturePtr mask,
		  PicturePtr dest,
		  INT16 x_source,
		  INT16 y_source,
		  INT16 x_mask,
		  INT16 y_mask,
		  INT16 x_dest, INT16 y_dest, 
		  CARD16 width, CARD16 height, Bool fallback)
{
	ScreenPtr screen = dest->pDrawable->pScreen;
	glamor_pixmap_private *dest_pixmap_priv;
	glamor_pixmap_private *source_pixmap_priv =
	    NULL, *mask_pixmap_priv = NULL;
	PixmapPtr dest_pixmap =
	    glamor_get_drawable_pixmap(dest->pDrawable);
	PixmapPtr source_pixmap = NULL, mask_pixmap = NULL;
	PicturePtr temp_src = source, temp_mask = mask;
	int x_temp_src, y_temp_src, x_temp_mask, y_temp_mask;
	glamor_composite_rect_t rect[10];
	glamor_composite_rect_t *prect = rect;
	int prect_size = ARRAY_SIZE(rect);
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(screen);
	Bool ret = TRUE;
	RegionRec region;
	BoxPtr box;
	int nbox, i, ok = FALSE;
	PixmapPtr sub_dest_pixmap = NULL;
	PixmapPtr sub_source_pixmap = NULL;
	PixmapPtr sub_mask_pixmap = NULL;
	int dest_x_off, dest_y_off, saved_dest_x = 0, saved_dest_y = 0;
	int source_x_off, source_y_off, saved_source_x = 0, saved_source_y = 0;
	int mask_x_off, mask_y_off, saved_mask_x = 0, saved_mask_y = 0;
	DrawablePtr saved_dest_drawable = NULL;
	DrawablePtr saved_source_drawable = NULL;
	DrawablePtr saved_mask_drawable = NULL;

	x_temp_src = x_source;
	y_temp_src = y_source;
	x_temp_mask = x_mask;
	y_temp_mask = y_mask;

	dest_pixmap_priv = glamor_get_pixmap_private(dest_pixmap);
	/* Currently. Always fallback to cpu if destination is in CPU memory. */

	if (source->pDrawable) {
		source_pixmap = glamor_get_drawable_pixmap(source->pDrawable);
		source_pixmap_priv = glamor_get_pixmap_private(source_pixmap);
		if (source_pixmap_priv && source_pixmap_priv->type == GLAMOR_DRM_ONLY)
			goto fail;
	}

	if (mask && mask->pDrawable) {
		mask_pixmap = glamor_get_drawable_pixmap(mask->pDrawable);
		mask_pixmap_priv = glamor_get_pixmap_private(mask_pixmap);
		if (mask_pixmap_priv && mask_pixmap_priv->type == GLAMOR_DRM_ONLY)
			goto fail;
	}

	if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(dest_pixmap_priv)) {
		goto fail;
	}

	if (op >= ARRAY_SIZE(composite_op_info))
		goto fail;

	if ((!source->pDrawable
	     && (source->pSourcePict->type != SourcePictTypeSolidFill))
	    || (source->pDrawable
		&& !GLAMOR_PIXMAP_PRIV_HAS_FBO(source_pixmap_priv)
		&&
		((width * height * 4 <
		  (source_pixmap->drawable.width *
		   source_pixmap->drawable.height))
		 ||
		 !(glamor_check_fbo_size
		   (glamor_priv, source_pixmap->drawable.width,
		    source_pixmap->drawable.height))))) {
		temp_src =
		    glamor_convert_gradient_picture(screen, source,
						    x_source, y_source,
						    width, height);
		if (!temp_src) {
			temp_src = source;
			goto fail;
		}
		x_temp_src = y_temp_src = 0;
	}

	if (mask
	    &&
	    ((!mask->pDrawable
	      && (mask->pSourcePict->type != SourcePictTypeSolidFill))
	     || (mask->pDrawable
		 && (!GLAMOR_PIXMAP_PRIV_HAS_FBO(mask_pixmap_priv))
		 &&
		 ((width * height * 4 <
		   (mask_pixmap->drawable.width *
		    mask_pixmap->drawable.height))
		  ||
		  !(glamor_check_fbo_size
		    (glamor_priv, mask_pixmap->drawable.width,
		     mask_pixmap->drawable.height)))))) {
		/* XXX if mask->pDrawable is the same as source->pDrawable, we have an opportunity
		 * to do reduce one convertion. */
		temp_mask =
		    glamor_convert_gradient_picture(screen, mask,
						    x_mask, y_mask,
						    width, height);
		if (!temp_mask) {
			temp_mask = mask;
			goto fail;
		}
		x_temp_mask = y_temp_mask = 0;
	}
	/* Do two-pass PictOpOver componentAlpha, until we enable
	 * dual source color blending.
	 */

	if (mask && mask->componentAlpha) {
		if (op == PictOpOver) {
			glamor_composite(PictOpOutReverse,
					 temp_src, temp_mask, dest,
					 x_temp_src, y_temp_src,
					 x_temp_mask, y_temp_mask,
					 x_dest, y_dest, width, height);
			glamor_composite(PictOpAdd,
					 temp_src, temp_mask, dest,
					 x_temp_src, y_temp_src,
					 x_temp_mask, y_temp_mask,
					 x_dest, y_dest, width, height);
			goto done;

		} else if (op == PictOpAtop
			   || op == PictOpAtopReverse
			   || op == PictOpXor
			   || op >= PictOpSaturate) {
				glamor_fallback
					("glamor_composite(): component alpha op %x\n", op);
				goto fail;
		}
	}

	if (!mask) {
		if (glamor_composite_with_copy(op, temp_src, dest,
					       x_temp_src, y_temp_src,
					       x_dest, y_dest, width,
					       height))
			goto done;
	}

	/*XXXXX, maybe we can make a copy of dest pixmap.*/
	if (source_pixmap == dest_pixmap)
		goto full_fallback;

	x_dest += dest->pDrawable->x;
	y_dest += dest->pDrawable->y;
	if (temp_src->pDrawable) {
		x_temp_src += temp_src->pDrawable->x;
		y_temp_src += temp_src->pDrawable->y;
	}
	if (temp_mask && temp_mask->pDrawable) {
		x_temp_mask += temp_mask->pDrawable->x;
		y_temp_mask += temp_mask->pDrawable->y;
	}
	if (!miComputeCompositeRegion(&region,
				      temp_src, temp_mask, dest,
				      x_temp_src, y_temp_src,
				      x_temp_mask, y_temp_mask,
				      x_dest, y_dest, width,
				      height))
		goto done;

	box = REGION_RECTS(&region);
	nbox = REGION_NUM_RECTS(&region);

	if (nbox > ARRAY_SIZE(rect)) {
		prect = calloc(nbox, sizeof(*prect));
		if (prect)
			prect_size = nbox;
		else {
			prect = rect;
			prect_size = ARRAY_SIZE(rect);
		}
	}
	while(nbox) {
		int box_cnt;
		box_cnt = nbox > prect_size ? prect_size : nbox;
		for (i = 0; i < box_cnt; i++) {
			prect[i].x_src = box[i].x1 + x_temp_src - x_dest;
			prect[i].y_src = box[i].y1 + y_temp_src - y_dest;
			prect[i].x_mask = box[i].x1 + x_temp_mask - x_dest;
			prect[i].y_mask = box[i].y1 + y_temp_mask - y_dest;
			prect[i].x_dst = box[i].x1;
			prect[i].y_dst = box[i].y1;
			prect[i].width = box[i].x2 - box[i].x1;
			prect[i].height = box[i].y2 - box[i].y1;
		}
		ok = glamor_composite_with_shader(op, temp_src, temp_mask,
						  dest, box_cnt, prect);
		if (!ok)
			break;
		nbox -= box_cnt;
		box += box_cnt;
	}

	REGION_UNINIT(dest->pDrawable->pScreen, &region);
	if (ok)
		goto done;

fail:

	if (!fallback
	    && glamor_ddx_fallback_check_pixmap(&dest_pixmap->drawable)
	    && (!source_pixmap 
		|| glamor_ddx_fallback_check_pixmap(&source_pixmap->drawable))
	    && (!mask_pixmap
		|| glamor_ddx_fallback_check_pixmap(&mask_pixmap->drawable))) {
		ret = FALSE;
		goto done;
	}

	glamor_fallback
	    ("from picts %p:%p %dx%d / %p:%p %d x %d (%c,%c)  to pict %p:%p %dx%d (%c)\n",
	     source, source->pDrawable,
	     source->pDrawable ? source->pDrawable->width : 0,
	     source->pDrawable ? source->pDrawable->height : 0, mask,
	     (!mask) ? NULL : mask->pDrawable, (!mask
						|| !mask->pDrawable) ? 0 :
	     mask->pDrawable->width, (!mask
				      || !mask->
				      pDrawable) ? 0 : mask->pDrawable->
	     height, glamor_get_picture_location(source),
	     glamor_get_picture_location(mask), dest, dest->pDrawable,
	     dest->pDrawable->width, dest->pDrawable->height,
	     glamor_get_picture_location(dest));

#define GET_SUB_PICTURE(p, access)		do {					\
	glamor_get_drawable_deltas(p->pDrawable, p ##_pixmap,				\
				   & p ##_x_off, & p ##_y_off);				\
	sub_ ##p ##_pixmap = glamor_get_sub_pixmap(p ##_pixmap,				\
					      x_ ##p + p ##_x_off + p->pDrawable->x,	\
					      y_ ##p + p ##_y_off + p->pDrawable->y,	\
					      width, height, access);			\
	if (sub_ ##p ##_pixmap != NULL) {						\
		saved_ ##p ##_drawable = p->pDrawable;					\
		p->pDrawable = &sub_ ##p ##_pixmap->drawable;				\
		saved_ ##p ##_x = x_ ##p;						\
		saved_ ##p ##_y = y_ ##p;						\
		if (p->pCompositeClip)							\
			pixman_region_translate (p->pCompositeClip,			\
						 -p->pDrawable->x - x_ ##p,		\
						 -p->pDrawable->y - y_ ##p);		\
		x_ ##p = 0;								\
		y_ ##p = 0;								\
	} } while(0)

	GET_SUB_PICTURE(dest, GLAMOR_ACCESS_RW);
	if (source->pDrawable)
		GET_SUB_PICTURE(source, GLAMOR_ACCESS_RO);
	if (mask && mask->pDrawable)
		GET_SUB_PICTURE(mask, GLAMOR_ACCESS_RO);

full_fallback:
	if (glamor_prepare_access_picture(dest, GLAMOR_ACCESS_RW)) {
		if (source_pixmap == dest_pixmap || glamor_prepare_access_picture
		    (source, GLAMOR_ACCESS_RO)) {
			if (!mask
			    || glamor_prepare_access_picture(mask,
							     GLAMOR_ACCESS_RO))
			{
				fbComposite(op,
					    source, mask, dest,
					    x_source, y_source,
					    x_mask, y_mask, x_dest,
					    y_dest, width, height);
				if (mask)
					glamor_finish_access_picture(mask, GLAMOR_ACCESS_RO);
			}
			if (source_pixmap != dest_pixmap)
				glamor_finish_access_picture(source, GLAMOR_ACCESS_RO);
		}
		glamor_finish_access_picture(dest, GLAMOR_ACCESS_RW);
	}

#define PUT_SUB_PICTURE(p, access)		do {				\
	if (sub_ ##p ##_pixmap != NULL) {					\
		x_ ##p = saved_ ##p ##_x;					\
		y_ ##p = saved_ ##p ##_y;					\
		if (p->pCompositeClip)						\
			pixman_region_translate (p->pCompositeClip,		\
						 p->pDrawable->x + x_ ##p,	\
						 p->pDrawable->y + y_ ##p);	\
		p->pDrawable = saved_ ##p ##_drawable;				\
		glamor_put_sub_pixmap(sub_ ##p ##_pixmap, p ##_pixmap,		\
				      x_ ##p + p ##_x_off + p->pDrawable->x,	\
				      y_ ##p + p ##_y_off + p->pDrawable->y,	\
				      width, height, access);			\
	}} while(0)
	if (mask && mask->pDrawable)
		PUT_SUB_PICTURE(mask, GLAMOR_ACCESS_RO);
	if (source->pDrawable)
		PUT_SUB_PICTURE(source, GLAMOR_ACCESS_RO);
	PUT_SUB_PICTURE(dest, GLAMOR_ACCESS_RW);
      done:
	if (temp_src != source)
		FreePicture(temp_src, 0);
	if (temp_mask != mask)
		FreePicture(temp_mask, 0);
	if (prect != rect)
		free(prect);
	return ret;
}

void
glamor_composite(CARD8 op,
		 PicturePtr source,
		 PicturePtr mask,
		 PicturePtr dest,
		 INT16 x_source,
		 INT16 y_source,
		 INT16 x_mask,
		 INT16 y_mask,
		 INT16 x_dest, INT16 y_dest,
		 CARD16 width, CARD16 height)
{
	_glamor_composite(op, source, mask, dest, x_source, y_source,
			  x_mask, y_mask, x_dest, y_dest, width, height,
			  TRUE);
}

Bool
glamor_composite_nf(CARD8 op,
		    PicturePtr source,
		    PicturePtr mask,
		    PicturePtr dest,
		    INT16 x_source,
		    INT16 y_source,
		    INT16 x_mask,
		    INT16 y_mask,
		    INT16 x_dest, INT16 y_dest,
		    CARD16 width, CARD16 height)
{
	return _glamor_composite(op, source, mask, dest, x_source, y_source,
				 x_mask, y_mask, x_dest, y_dest, width, height,
				 FALSE);
}




/**
 * Creates an appropriate picture to upload our alpha mask into (which
 * we calculated in system memory)
 */
static PicturePtr
glamor_create_mask_picture(ScreenPtr screen,
			   PicturePtr dst,
			   PictFormatPtr pict_format,
			   CARD16 width, CARD16 height)
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

	pixmap = glamor_create_pixmap(screen, 0, 0,
				      pict_format->depth,
				      GLAMOR_CREATE_PIXMAP_CPU);
	if (!pixmap)
		return 0;
	picture = CreatePicture(0, &pixmap->drawable, pict_format,
				0, 0, serverClient, &error);
	screen->DestroyPixmap(pixmap);
	return picture;
}

/**
 * glamor_trapezoids is a copy of miTrapezoids that does all the trapezoid
 * accumulation in system memory.
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
	pixman_image_t *image;

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

	if (bounds.y1 >= bounds.y2 || bounds.x1 >= bounds.x2)
		return TRUE;

	x_dst = traps[0].left.p1.x >> 16;
	y_dst = traps[0].left.p1.y >> 16;

	width = bounds.x2 - bounds.x1;
	height = bounds.y2 - bounds.y1;
	stride = PixmapBytePad(width, mask_format->depth);
	picture = glamor_create_mask_picture(screen, dst, mask_format,
					     width, height);
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

	x_rel = bounds.x1 + x_src - x_dst;
	y_rel = bounds.y1 + y_src - y_dst;
	CompositePicture(op, src, picture, dst,
			 x_rel, y_rel,
			 0, 0,
			 bounds.x1, bounds.y1,
			 bounds.x2 - bounds.x1, bounds.y2 - bounds.y1);

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
	_glamor_trapezoids(op, src, dst, mask_format, x_src, 
			   y_src, ntrap, traps, TRUE);
}

Bool
glamor_trapezoids_nf(CARD8 op,
		     PicturePtr src, PicturePtr dst,
		     PictFormatPtr mask_format, INT16 x_src, INT16 y_src,
		     int ntrap, xTrapezoid * traps)
{
	return _glamor_trapezoids(op, src, dst, mask_format, x_src, 
				  y_src, ntrap, traps, FALSE);
}



void
glamor_composite_glyph_rects(CARD8 op,
			     PicturePtr src, PicturePtr mask, PicturePtr dst,
			     int nrect, glamor_composite_rect_t * rects)
{
	int n;
	glamor_composite_rect_t *r;

	ValidatePicture(src);
	ValidatePicture(dst);

	if (glamor_composite_with_shader(op, src, mask, dst, nrect, rects))
		return;

	n = nrect;
	r = rects;

	while (n--) {
		CompositePicture(op,
				 src,
				 mask,
				 dst,
				 r->x_src, r->y_src,
				 r->x_mask, r->y_mask,
				 r->x_dst, r->y_dst, r->width, r->height);
		r++;
	}
}

static Bool
_glamor_composite_rects (CARD8         op,
			      PicturePtr    pDst,
			      xRenderColor  *color,
			      int           nRect,
			      xRectangle    *rects,
			      Bool	    fallback)
{
	miCompositeRects(op, pDst, color, nRect, rects);
	return TRUE;
}

void
glamor_composite_rects (CARD8         op,
			PicturePtr    pDst,
			xRenderColor  *color,
			int           nRect,
			xRectangle    *rects)
{
	_glamor_composite_rects(op, pDst, color, nRect, rects, TRUE);
}

Bool
glamor_composite_rects_nf (CARD8         op,
			   PicturePtr    pDst,
			   xRenderColor  *color,
			   int           nRect,
			   xRectangle    *rects)
{
	return _glamor_composite_rects(op, pDst, color, nRect, rects, FALSE);
}

#endif				/* RENDER */
