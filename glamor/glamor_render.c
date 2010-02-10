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

#include "glu3/glu3.h"

struct shader_key {
    Bool solid_source;
    Bool has_mask;
    Bool solid_mask;
};

struct blendinfo {
    Bool dest_alpha;
    Bool source_alpha;
    GLenum source_blend;
    GLenum dest_blend;
};

static struct blendinfo composite_op_info[] = {
    [PictOpClear] =       {0, 0, GL_ZERO,                GL_ZERO},
    [PictOpSrc] =         {0, 0, GL_ONE,                 GL_ZERO},
    [PictOpDst] =         {0, 0, GL_ZERO,                GL_ONE},
    [PictOpOver] =        {0, 1, GL_ONE,                 GL_ONE_MINUS_SRC_ALPHA},
    [PictOpOverReverse] = {1, 0, GL_ONE_MINUS_DST_ALPHA, GL_ONE},
    [PictOpIn] =          {1, 0, GL_DST_ALPHA,           GL_ZERO},
    [PictOpInReverse] =   {0, 1, GL_ZERO,                GL_SRC_ALPHA},
    [PictOpOut] =         {1, 0, GL_ONE_MINUS_DST_ALPHA, GL_ZERO},
    [PictOpOutReverse] =  {0, 1, GL_ZERO,                GL_ONE_MINUS_SRC_ALPHA},
    [PictOpAtop] =        {1, 1, GL_DST_ALPHA,           GL_ONE_MINUS_SRC_ALPHA},
    [PictOpAtopReverse] = {1, 1, GL_ONE_MINUS_DST_ALPHA, GL_SRC_ALPHA},
    [PictOpXor] =         {1, 1, GL_ONE_MINUS_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA},
    [PictOpAdd] =         {0, 0, GL_ONE,                 GL_ONE},
};

#define SOLID_SOURCE_INDEX	1
#define HAS_MASK_INDEX		2
#define SOLID_MASK_INDEX	3

static glamor_composite_shader *
glamor_lookup_composite_shader(ScreenPtr screen, struct shader_key *key)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    int index = 0;

    if (key->solid_source)
	index += SOLID_SOURCE_INDEX;
    if (key->has_mask) {
	index += HAS_MASK_INDEX;
	if (key->solid_mask)
	    index += SOLID_MASK_INDEX;
    }

    assert(index < ARRAY_SIZE(glamor_priv->composite_shader));

    return &glamor_priv->composite_shader[index];
}

static GLuint
glamor_create_composite_fs(struct shader_key *key)
{
    const char *source_pixmap_header =
	"uniform sampler2D source_sampler;\n";
    const char *source_solid_header =
	"uniform vec4 source;\n";
    const char *mask_pixmap_header =
	"uniform sampler2D mask_sampler;\n";
    const char *mask_solid_header =
	"uniform vec4 mask;\n";
    const char *main_opening =
	"void main()\n"
	"{\n";
    const char *source_pixmap_fetch =
	"	vec4 source = texture2D(source_sampler, gl_TexCoord[0].xy);\n";
    const char *mask_pixmap_fetch =
	"	vec4 mask = texture2D(mask_sampler, gl_TexCoord[1].xy);\n";
    const char *source_in_mask =
	"	gl_FragColor = source * mask.w;\n";
    const char *source_only =
	"	gl_FragColor = source;\n";
    const char *main_closing =
	"}\n";
    char *source;
    const char *source_setup = "";
    const char *source_fetch = "";
    const char *mask_setup = "";
    const char *mask_fetch = "";
    GLuint prog;

    if (key->solid_source) {
	source_setup = source_solid_header;
    } else {
	source_setup = source_pixmap_header;
	source_fetch = source_pixmap_fetch;
    }
    if (key->has_mask) {
	if (key->solid_mask) {
	    mask_setup = mask_solid_header;
	} else {
	    mask_setup = mask_pixmap_header;
	    mask_fetch = mask_pixmap_fetch;
	}
    }
    source = XNFprintf("%s%s%s%s%s%s%s",
		       source_setup,
		       mask_setup,
		       main_opening,
		       source_fetch,
		       mask_fetch,
		       key->has_mask ? source_in_mask : source_only,
		       main_closing);

    prog = glamor_compile_glsl_prog(GL_FRAGMENT_SHADER_ARB, source);
    xfree(source);

    return prog;
}

static GLuint
glamor_create_composite_vs(struct shader_key *key)
{
    const char *main_opening =
	"void main()\n"
	"{\n"
	"	gl_Position = gl_Vertex;\n"
	"	gl_TexCoord[0] = gl_MultiTexCoord0;\n";
    const char *mask_coords =
	"	gl_TexCoord[1] = gl_MultiTexCoord1;\n";
    const char *main_closing =
	"}\n";
    char *source;
    GLuint prog;
    Bool compute_mask_coords = key->has_mask && !key->solid_mask;

    source = XNFprintf("%s%s%s",
		       main_opening,
		       compute_mask_coords ? mask_coords : "",
		       main_closing);

    prog = glamor_compile_glsl_prog(GL_VERTEX_SHADER_ARB, source);
    xfree(source);

    return prog;
}

static void
glamor_create_composite_shader(ScreenPtr screen, struct shader_key *key)
{
    GLuint vs, fs, prog;
    GLint source_sampler_uniform_location, mask_sampler_uniform_location;
    glamor_composite_shader *shader;

    shader = glamor_lookup_composite_shader(screen, key);

    vs = glamor_create_composite_vs(key);
    if (vs == 0)
	return;
    fs = glamor_create_composite_fs(key);
    if (fs == 0)
	return;

    prog = glCreateProgramObjectARB();
    glAttachObjectARB(prog, vs);
    glAttachObjectARB(prog, fs);
    glamor_link_glsl_prog(prog);

    shader->prog = prog;

    glUseProgramObjectARB(prog);

    if (key->solid_source) {
	shader->source_uniform_location = glGetUniformLocationARB(prog,
								  "source");
    } else {
	source_sampler_uniform_location = glGetUniformLocationARB(prog,
								  "source_sampler");
	glUniform1i(source_sampler_uniform_location, 0);
    }

    if (key->has_mask) {
	if (key->solid_mask) {
	    shader->mask_uniform_location = glGetUniformLocationARB(prog,
								    "mask");
	} else {
	    mask_sampler_uniform_location = glGetUniformLocationARB(prog,
								    "mask_sampler");
	    glUniform1i(mask_sampler_uniform_location, 1);
	}
    }
}

void
glamor_init_composite_shaders(ScreenPtr screen)
{
    struct shader_key key;

    memset(&key, 0, sizeof(key));
    key.has_mask = FALSE;
    glamor_create_composite_shader(screen, &key);
    key.has_mask = TRUE;
    glamor_create_composite_shader(screen, &key);
    key.solid_mask = TRUE;
    glamor_create_composite_shader(screen, &key);

    memset(&key, 0, sizeof(key));
    key.solid_source = TRUE;
    key.has_mask = FALSE;
    glamor_create_composite_shader(screen, &key);
    key.has_mask = TRUE;
    glamor_create_composite_shader(screen, &key);
    key.solid_mask = TRUE;
    glamor_create_composite_shader(screen, &key);
}

static Bool
glamor_set_composite_op(ScreenPtr screen,
			CARD8 op, PicturePtr dest, PicturePtr mask)
{
    GLenum source_blend, dest_blend;
    struct blendinfo *op_info;

    if (op >= ARRAY_SIZE(composite_op_info)) {
	ErrorF("unsupported render op\n");
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
    if (mask && mask->componentAlpha && PICT_FORMAT_RGB(mask->format) != 0 &&
	op_info->source_alpha) {
	if (source_blend != GL_ZERO) {
	    ErrorF("Dual-source composite blending not supported\n");
	    return GL_FALSE;
	}
	if (dest_blend == GL_SRC_ALPHA)
	    dest_blend = GL_SRC_COLOR;
	else if (dest_blend == GL_ONE_MINUS_SRC_ALPHA)
	    dest_blend = GL_ONE_MINUS_SRC_COLOR;
    }

    if (source_blend == GL_ONE && dest_blend == GL_ZERO) {
	glDisable(GL_BLEND);
    } else {
	glEnable(GL_BLEND);
	glBlendFunc(source_blend, dest_blend);
    }
    return TRUE;
}

static void
glamor_set_composite_texture(ScreenPtr screen, int unit, PicturePtr picture,
			     glamor_pixmap_private *pixmap_priv)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, pixmap_priv->tex);
    switch (picture->repeatType) {
    case RepeatNone:
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	break;
    case RepeatNormal:
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	break;
    case RepeatPad:
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	break;
    case RepeatReflect:
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	break;
    }

    switch (picture->filter) {
    case PictFilterNearest:
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        break;
    case PictFilterBilinear:
    default:
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;
    }

    glEnable(GL_TEXTURE_2D);
}

static void
glamor_set_composite_solid(PicturePtr picture, GLint uniform_location)
{
    CARD32 c = picture->pSourcePict->solidFill.color; /* a8r8g8b8 */
    float color[4]; /* rgba */

    color[0] = ((c >> 16) & 0xff) / 255.0;
    color[1] = ((c >> 8) & 0xff) / 255.0;
    color[2] = ((c >> 0) & 0xff) / 255.0;
    color[3] = ((c >> 24) & 0xff) / 255.0;

    glUniform4fvARB(uniform_location, 1, color);
}

static int
compatible_formats (CARD8 op, PicturePtr dst, PicturePtr src)
{
    if (op == PictOpSrc) {
	if (src->format == dst->format)
	    return 1;

	if (src->format == PICT_a8r8g8b8 && dst->format == PICT_x8r8g8b8)
	    return 1;

	if (src->format == PICT_a8b8g8r8 && dst->format == PICT_x8b8g8r8)
	    return 1;
    } else if (op == PictOpOver) {
	if (src->alphaMap || dst->alphaMap)
	    return 0;

	if (src->format != dst->format)
	    return 0;

	if (src->format == PICT_x8r8g8b8 || src->format == PICT_x8b8g8r8)
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
			   INT16 y_dest,
			   CARD16 width,
			   CARD16 height)
{
    RegionRec region;

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
				  0, 0,
				  x_dest, y_dest,
				  width, height))
	return TRUE;

    glamor_copy_n_to_n(source->pDrawable,
		       dest->pDrawable, NULL,
		       REGION_RECTS(&region),
		       REGION_NUM_RECTS(&region),
		       x_source - x_dest, y_source - y_dest,
		       FALSE, FALSE, 0, NULL);
    REGION_UNINIT(dest->pDrawable->pScreen,
		  &region);
    return TRUE;
}

static Bool
good_source_format(PicturePtr picture)
{
    switch (picture->format) {
    case PICT_a8:
    case PICT_a8r8g8b8:
	return TRUE;
    default:
	glamor_fallback("Bad source format 0x%08x\n", picture->format);
	return FALSE;
    }
}

static Bool
good_mask_format(PicturePtr picture)
{
    switch (picture->format) {
    case PICT_a8:
    case PICT_a8r8g8b8:
	return TRUE;
    default:
	glamor_fallback("Bad mask format 0x%08x\n", picture->format);
	return FALSE;
    }
}

static Bool
good_dest_format(PicturePtr picture)
{
    switch (picture->format) {
    case PICT_a8:
    case PICT_a8r8g8b8:
    case PICT_x8r8g8b8:
	return TRUE;
    default:
	glamor_fallback("Bad dest format 0x%08x\n", picture->format);
	return FALSE;
    }
}

static Bool
glamor_composite_with_shader(CARD8 op,
			     PicturePtr source,
			     PicturePtr mask,
			     PicturePtr dest,
			     INT16 x_source,
			     INT16 y_source,
			     INT16 x_mask,
			     INT16 y_mask,
			     INT16 x_dest,
			     INT16 y_dest,
			     CARD16 width,
			     CARD16 height)
{
    ScreenPtr screen = dest->pDrawable->pScreen;
    PixmapPtr dest_pixmap = glamor_get_drawable_pixmap(dest->pDrawable);
    PixmapPtr source_pixmap, mask_pixmap = NULL;
    glamor_pixmap_private *source_pixmap_priv = NULL;
    glamor_pixmap_private *mask_pixmap_priv = NULL;
    struct shader_key key;
    glamor_composite_shader *shader;
    RegionRec region;
    float vertices[4][2], source_texcoords[4][2], mask_texcoords[4][2];
    int i;
    BoxPtr box;

    memset(&key, 0, sizeof(key));
    key.has_mask = (mask != NULL);
    if (!source->pDrawable) {
	if (source->pSourcePict->type == SourcePictTypeSolidFill) {
	    key.solid_source = TRUE;
	} else {
	    ErrorF("gradient source\n");
	    goto fail;
	}
    }
    if (mask && !mask->pDrawable) {
	if (mask->pSourcePict->type == SourcePictTypeSolidFill) {
	    key.solid_mask = TRUE;
	} else {
	    ErrorF("gradient mask\n");
	    goto fail;
	}
    }
    if (source->alphaMap) {
	ErrorF("source alphaMap\n");
	goto fail;
    }
    if (mask && mask->alphaMap) {
	ErrorF("mask alphaMap\n");
	goto fail;
    }

    if (!key.solid_source) {
	source_pixmap = glamor_get_drawable_pixmap(source->pDrawable);
	source_pixmap_priv = glamor_get_pixmap_private(source_pixmap);
	if (source_pixmap == dest_pixmap) {
	    glamor_fallback("glamor_composite(): source == dest\n");
	    goto fail;
	}
	if (!source_pixmap_priv || source_pixmap_priv->tex == 0) {
	    glamor_fallback("glamor_composite(): no FBO in source\n");
	    goto fail;
	}
	if (!good_source_format(source))
	    goto fail;
    }
    if (mask && !key.solid_mask) {
	mask_pixmap = glamor_get_drawable_pixmap(mask->pDrawable);
	mask_pixmap_priv = glamor_get_pixmap_private(mask_pixmap);
	if (mask_pixmap == dest_pixmap) {
	    glamor_fallback("glamor_composite(): mask == dest\n");
	    goto fail;
	}
	if (!mask_pixmap_priv || mask_pixmap_priv->tex == 0) {
	    glamor_fallback("glamor_composite(): no FBO in mask\n");
	    goto fail;
	}
	if (!good_mask_format(mask))
	    goto fail;
    }
    if (!good_dest_format(dest))
	goto fail;

    shader = glamor_lookup_composite_shader(screen, &key);
    if (shader->prog == 0) {
	glamor_fallback("glamor_composite(): "
			"no shader program for this render acccel mode\n");
	goto fail;
    }

    glUseProgramObjectARB(shader->prog);

    if (!glamor_set_destination_pixmap(dest_pixmap))
	goto fail;

    if (!glamor_set_composite_op(screen, op, dest, mask)) {
	goto fail;
    }

    x_dest += dest->pDrawable->x;
    y_dest += dest->pDrawable->y;
    if (source->pDrawable) {
	x_source += source->pDrawable->x;
	y_source += source->pDrawable->y;
    }
    if (mask && mask->pDrawable) {
	x_mask += mask->pDrawable->x;
	y_mask += mask->pDrawable->y;
    }

    if (key.solid_source) {
	glamor_set_composite_solid(source, shader->source_uniform_location);
    } else {
	glamor_set_composite_texture(screen, 0, source, source_pixmap_priv);
    }
    if (key.has_mask) {
	if (key.solid_mask) {
	    glamor_set_composite_solid(mask, shader->mask_uniform_location);
	} else {
	    glamor_set_composite_texture(screen, 1, mask, mask_pixmap_priv);
	}
    }

    if (!miComputeCompositeRegion(&region,
				  source, mask, dest,
				  x_source, y_source,
				  x_mask, y_mask,
				  x_dest, y_dest,
				  width, height))
	goto done;


    glVertexPointer(2, GL_FLOAT, sizeof(float) * 2, vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    if (!key.solid_source) {
	glClientActiveTexture(GL_TEXTURE0);
	glTexCoordPointer(2, GL_FLOAT, sizeof(float) * 2, source_texcoords);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    if (key.has_mask && !key.solid_mask) {
	glClientActiveTexture(GL_TEXTURE1);
	glTexCoordPointer(2, GL_FLOAT, sizeof(float) * 2, mask_texcoords);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    box = REGION_RECTS(&region);
    for (i = 0; i < REGION_NUM_RECTS(&region); i++) {
	vertices[0][0] = v_from_x_coord_x(dest_pixmap, box[i].x1);
	vertices[0][1] = v_from_x_coord_y(dest_pixmap, box[i].y1);
	vertices[1][0] = v_from_x_coord_x(dest_pixmap, box[i].x2);
	vertices[1][1] = v_from_x_coord_y(dest_pixmap, box[i].y1);
	vertices[2][0] = v_from_x_coord_x(dest_pixmap, box[i].x2);
	vertices[2][1] = v_from_x_coord_y(dest_pixmap, box[i].y2);
	vertices[3][0] = v_from_x_coord_x(dest_pixmap, box[i].x1);
	vertices[3][1] = v_from_x_coord_y(dest_pixmap, box[i].y2);

	if (!key.solid_source) {
	    int tx1 = box[i].x1 + x_source - x_dest;
	    int ty1 = box[i].y1 + y_source - y_dest;
	    int tx2 = box[i].x2 + x_source - x_dest;
	    int ty2 = box[i].y2 + y_source - y_dest;
	    source_texcoords[0][0] = t_from_x_coord_x(source_pixmap, tx1);
	    source_texcoords[0][1] = t_from_x_coord_y(source_pixmap, ty1);
	    source_texcoords[1][0] = t_from_x_coord_x(source_pixmap, tx2);
	    source_texcoords[1][1] = t_from_x_coord_y(source_pixmap, ty1);
	    source_texcoords[2][0] = t_from_x_coord_x(source_pixmap, tx2);
	    source_texcoords[2][1] = t_from_x_coord_y(source_pixmap, ty2);
	    source_texcoords[3][0] = t_from_x_coord_x(source_pixmap, tx1);
	    source_texcoords[3][1] = t_from_x_coord_y(source_pixmap, ty2);
	}

	if (key.has_mask && !key.solid_mask) {
	    int tx1 = box[i].x1 + x_mask - x_dest;
	    int ty1 = box[i].y1 + y_mask - y_dest;
	    int tx2 = box[i].x2 + x_mask - x_dest;
	    int ty2 = box[i].y2 + y_mask - y_dest;
	    mask_texcoords[0][0] = t_from_x_coord_x(mask_pixmap, tx1);
	    mask_texcoords[0][1] = t_from_x_coord_y(mask_pixmap, ty1);
	    mask_texcoords[1][0] = t_from_x_coord_x(mask_pixmap, tx2);
	    mask_texcoords[1][1] = t_from_x_coord_y(mask_pixmap, ty1);
	    mask_texcoords[2][0] = t_from_x_coord_x(mask_pixmap, tx2);
	    mask_texcoords[2][1] = t_from_x_coord_y(mask_pixmap, ty2);
	    mask_texcoords[3][0] = t_from_x_coord_x(mask_pixmap, tx1);
	    mask_texcoords[3][1] = t_from_x_coord_y(mask_pixmap, ty2);
	}
#if 0
 else memset(mask_texcoords, 0, sizeof(mask_texcoords));
	for (i = 0; i < 4; i++) {
	    ErrorF("%d: (%04.4f, %04.4f) (%04.4f, %04.4f) (%04.4f, %04.4f)\n",
		   i,
		   source_texcoords[i][0], source_texcoords[i][1],
		   mask_texcoords[i][0], mask_texcoords[i][1],
		   vertices[i][0], vertices[i][1]);
	}
#endif

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }

    glClientActiveTexture(GL_TEXTURE0);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glClientActiveTexture(GL_TEXTURE1);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

done:
    REGION_UNINIT(dst->pDrawable->pScreen, &region);
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);
    glUseProgramObjectARB(0);
    return TRUE;

fail:
    glDisable(GL_BLEND);
    glUseProgramObjectARB(0);
    return FALSE;
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
		 INT16 x_dest,
		 INT16 y_dest,
		 CARD16 width,
		 CARD16 height)
{
    /* Do two-pass PictOpOver componentAlpha, until we enable
     * dual source color blending.
     */
    if (mask && mask->componentAlpha)
	goto fail;
    if (mask && mask->componentAlpha && op == PictOpOver) {
	glamor_composite(PictOpOutReverse,
			 source, mask, dest,
			 x_source, y_source,
			 x_mask, y_mask,
			 x_dest, y_dest,
			 width, height);
	glamor_composite(PictOpAdd,
			 source, mask, dest,
			 x_source, y_source,
			 x_mask, y_mask,
			 x_dest, y_dest,
			 width, height);
	return;
    }

    if (!mask) {
	if (glamor_composite_with_copy(op, source, dest,
				       x_source, y_source,
				       x_dest, y_dest,
				       width, height))
	    return;
    }

    if (glamor_composite_with_shader(op, source, mask, dest,
				     x_source, y_source,
				     x_mask, y_mask,
				     x_dest, y_dest,
				     width, height))
	return;

    glamor_fallback("glamor_composite(): "
		    "from picts %p/%p(%c,%c) to pict %p (%c)\n",
		    source, mask,
		    glamor_get_picture_location(source),
		    glamor_get_picture_location(mask),
		    dest,
		    glamor_get_picture_location(dest));
fail:
    glUseProgramObjectARB(0);
    glDisable(GL_BLEND);
    if (glamor_prepare_access(dest->pDrawable, GLAMOR_ACCESS_RW)) {
	if (source->pDrawable == NULL ||
	    glamor_prepare_access(source->pDrawable, GLAMOR_ACCESS_RO))
	{
	    if (!mask || mask->pDrawable == NULL ||
		glamor_prepare_access(mask->pDrawable, GLAMOR_ACCESS_RO))
	    {
		fbComposite(op,
			    source, mask, dest,
			    x_source, y_source,
			    x_mask, y_mask,
			    x_dest, y_dest,
			    width, height);
		if (mask && mask->pDrawable != NULL)
		    glamor_finish_access(mask->pDrawable);
	    }
	    if (source->pDrawable != NULL)
		glamor_finish_access(source->pDrawable);
	}
	glamor_finish_access(dest->pDrawable);
    }
}


/**
 * Creates an appropriate picture to upload our alpha mask into (which
 * we calculated in system memory)
 */
static PicturePtr
glamor_create_mask_picture(ScreenPtr screen,
			   PicturePtr dst,
			   PictFormatPtr pict_format,
			   CARD16 width,
			   CARD16 height)
{
    PixmapPtr pixmap;
    PicturePtr picture;
    int	error;

    if (!pict_format) {
	if (dst->polyEdge == PolyEdgeSharp)
	    pict_format = PictureMatchFormat(screen, 1, PICT_a1);
	else
	    pict_format = PictureMatchFormat(screen, 8, PICT_a8);
	if (!pict_format)
	    return 0;
    }

    pixmap = screen->CreatePixmap(screen, width, height,
				  pict_format->depth,
				  0);
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
void
glamor_trapezoids(CARD8 op,
		  PicturePtr src, PicturePtr dst,
		  PictFormatPtr mask_format, INT16 x_src, INT16 y_src,
		  int ntrap, xTrapezoid *traps)
{
    ScreenPtr screen = dst->pDrawable->pScreen;
    BoxRec bounds;
    PicturePtr picture;
    INT16 x_dst, y_dst;
    INT16 x_rel, y_rel;
    int width, height, stride;
    PixmapPtr pixmap;
    GCPtr gc;
    pixman_image_t *image;

    /* If a mask format wasn't provided, we get to choose, but behavior should
     * be as if there was no temporary mask the traps were accumulated into.
     */
    if (!mask_format) {
	if (dst->polyEdge == PolyEdgeSharp)
	    mask_format = PictureMatchFormat(screen, 1, PICT_a1);
	else
	    mask_format = PictureMatchFormat(screen, 8, PICT_a8);
	for (; ntrap; ntrap--, traps++)
	    glamor_trapezoids(op, src, dst, mask_format, x_src, y_src,
			      1, traps);
	return;
    }

    miTrapezoidBounds(ntrap, traps, &bounds);

    if (bounds.y1 >= bounds.y2 || bounds.x1 >= bounds.x2)
	return;

    x_dst = traps[0].left.p1.x >> 16;
    y_dst = traps[0].left.p1.y >> 16;

    width = bounds.x2 - bounds.x1;
    height = bounds.y2 - bounds.y1;
    stride = (width * BitsPerPixel(mask_format->depth) + 7) / 8;

    picture = glamor_create_mask_picture(screen, dst, mask_format,
					 width, height);
    if (!picture)
	return;

    image = pixman_image_create_bits(picture->format,
				     width, height,
				     NULL, stride);
    if (!image) {
	FreePicture(picture, 0);
	return;
    }

    for (; ntrap; ntrap--, traps++)
	pixman_rasterize_trapezoid(image, (pixman_trapezoid_t *) traps,
				   -bounds.x1, -bounds.y1);

    pixmap = GetScratchPixmapHeader(screen, width, height,
				    mask_format->depth,
				    BitsPerPixel(mask_format->depth),
				    PixmapBytePad(width, mask_format->depth),
				    pixman_image_get_data(image));
    if (!pixmap) {
	FreePicture(picture, 0);
	pixman_image_unref(image);
	return;
    }

    gc = GetScratchGC(picture->pDrawable->depth, screen);
    if (!gc) {
	FreeScratchPixmapHeader(pixmap);
	pixman_image_unref (image);
	FreePicture(picture, 0);
	return;
    }
    ValidateGC(picture->pDrawable, gc);

    gc->ops->CopyArea(&pixmap->drawable, picture->pDrawable,
		      gc, 0, 0, width, height, 0, 0);

    FreeScratchGC(gc);
    FreeScratchPixmapHeader(pixmap);
    pixman_image_unref(image);

    x_rel = bounds.x1 + x_src - x_dst;
    y_rel = bounds.y1 + y_src - y_dst;
    CompositePicture(op, src, picture, dst,
		     x_rel, y_rel,
		     0, 0,
		     bounds.x1, bounds.y1,
		     bounds.x2 - bounds.x1, bounds.y2 - bounds.y1);
    FreePicture(picture, 0);
}

void
glamor_composite_rects(CARD8 op,
		       PicturePtr src, PicturePtr dst,
		       int nrect, glamor_composite_rect_t *rects)
{
    int n;
    glamor_composite_rect_t *r;

    ValidatePicture(src);
    ValidatePicture(dst);

    n = nrect;
    r = rects;

    while (n--) {
	CompositePicture(op,
			 src,
			 NULL,
			 dst,
			 r->x_src, r->y_src,
			 0, 0,
			 r->x_dst, r->y_dst,
			 r->width, r->height);
	r++;
    }
}

#endif /* RENDER */
