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
	"uniform sampler2D source_sampler;\n"
	"varying vec4 source_coords;\n";
    const char *source_solid_header =
	"uniform vec4 source;\n";
    const char *mask_pixmap_header =
	"uniform sampler2D mask_sampler;\n"
	"varying vec4 mask_coords;\n";
    const char *mask_solid_header =
	"uniform vec4 mask;\n";
    const char *main_opening =
	"void main()\n"
	"{\n";
    const char *source_pixmap_fetch =
	"	vec4 source = texture2DProj(source_sampler, "
	"				    source_coords.xyw);\n";
    const char *mask_pixmap_fetch =
	"	vec4 mask = texture2DProj(mask_sampler, mask_coords.xyw);\n";
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
    const char *header =
	"uniform mat4 dest_to_dest;\n"
	"uniform mat4 dest_to_source;\n"
	"varying vec4 source_coords;\n";
    const char *mask_header =
	"uniform mat4 dest_to_mask;\n"
	"varying vec4 mask_coords;\n";
    const char *main_opening =
	"void main()\n"
	"{\n"
	"	vec4 incoming_dest_coords = vec4(gl_Vertex.xy, 0, 1);\n"
	"	gl_Position = dest_to_dest * incoming_dest_coords;\n"
	"	source_coords = dest_to_source * incoming_dest_coords;\n";
    const char *mask_coords =
	"	mask_coords = dest_to_mask * incoming_dest_coords;\n";
    const char *main_closing =
	"}\n";
    char *source;
    GLuint prog;
    Bool compute_mask_coords = key->has_mask && !key->solid_mask;

    source = XNFprintf("%s%s%s%s%s",
		       header,
		       compute_mask_coords ? mask_header : "",
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
    shader->dest_to_dest_uniform_location =
	glGetUniformLocationARB(prog, "dest_to_dest");

    if (key->solid_source) {
	shader->source_uniform_location = glGetUniformLocationARB(prog,
								  "source");
    } else {
	source_sampler_uniform_location = glGetUniformLocationARB(prog,
								  "source_sampler");
	shader->dest_to_source_uniform_location =
	    glGetUniformLocationARB(prog, "dest_to_source");
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
	    shader->dest_to_mask_uniform_location =
		glGetUniformLocationARB(prog, "dest_to_mask");
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

    key.solid_source = TRUE;
    key.has_mask = FALSE;
    glamor_create_composite_shader(screen, &key);
    key.has_mask = TRUE;
    glamor_create_composite_shader(screen, &key);
    key.solid_mask = TRUE;
    glamor_create_composite_shader(screen, &key);
}

static void
glamor_set_composite_transform_matrix(GLUmat4 *m,
				      PicturePtr picture,
				      float x_source,
				      float y_source)
{
    GLUmat4 temp;
    DrawablePtr drawable = picture->pDrawable;
    GLUvec4 scale = {{1.0f / drawable->width,
		      1.0f / drawable->height,
		      1.0,
		      1.0}};

    gluTranslate3f(m, -x_source, -y_source, 0.0);
    gluScale4v(&temp, &scale);
    gluMult4m_4m(m, &temp, m);
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

    if (mask && mask->componentAlpha && PICT_FORMAT_RGB(mask->format) != 0 &&
	op_info->source_alpha && source_blend != GL_ZERO) {
    }

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
			     glamor_pixmap_private *pixmap_priv,
			     GLint transform_uniform_location,
			     int x_translate, int y_translate)
{
    GLUmat4 transform;

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

    glamor_set_composite_transform_matrix(&transform,
					  picture,
					  x_translate,
					  y_translate);
    glUniformMatrix4fvARB(transform_uniform_location, 1, 0,
			  (float *)&transform);
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
    ScreenPtr screen = dest->pDrawable->pScreen;
    PixmapPtr dest_pixmap = glamor_get_drawable_pixmap(dest->pDrawable);
    PixmapPtr source_pixmap, mask_pixmap = NULL;
    glamor_pixmap_private *source_pixmap_priv = NULL;
    glamor_pixmap_private *mask_pixmap_priv = NULL;
    struct shader_key key;
    glamor_composite_shader *shader;
    GLUmat4 dest_to_dest;
    RegionRec region;
    int i;

    /* Do two-pass PictOpOver componentAlpha, until we enable
     * dual source color blending.
     */
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
	    ErrorF("source == dest\n");
	    goto fail;
	}
	if (!source_pixmap_priv || source_pixmap_priv->tex == 0) {
	    ErrorF("no FBO in source\n");
	    goto fail;
	}
    }
    if (mask && !key.solid_mask) {
	mask_pixmap = glamor_get_drawable_pixmap(mask->pDrawable);
	mask_pixmap_priv = glamor_get_pixmap_private(mask_pixmap);
	if (mask_pixmap == dest_pixmap) {
	    ErrorF("mask == dest\n");
	    goto fail;
	}
	if (!mask_pixmap_priv || mask_pixmap_priv->tex == 0) {
	    ErrorF("no FBO in mask\n");
	    goto fail;
	}
    }

    shader = glamor_lookup_composite_shader(screen, &key);
    if (shader->prog == 0) {
	ErrorF("No program compiled for this render accel mode\n");
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

    gluOrtho6f(&dest_to_dest,
	       dest_pixmap->screen_x, dest_pixmap->screen_x + width,
	       dest_pixmap->screen_y, dest_pixmap->screen_y + height,
	       -1, 1);
    glUniformMatrix4fvARB(shader->dest_to_dest_uniform_location, 1, 0,
			  (float *)&dest_to_dest);

    if (key.solid_source) {
	glamor_set_composite_solid(source, shader->source_uniform_location);
    } else {
	glamor_set_composite_texture(screen, 0, source, source_pixmap_priv,
				     shader->dest_to_source_uniform_location,
				     x_source - x_dest, y_source - y_dest);
    }
    if (key.has_mask) {
	if (key.solid_mask) {
	    glamor_set_composite_solid(mask, shader->mask_uniform_location);
	} else {
	    glamor_set_composite_texture(screen, 1, mask, mask_pixmap_priv,
					 shader->dest_to_mask_uniform_location,
					 x_mask - x_dest, y_mask - y_dest);
	}
    }

    if (!miComputeCompositeRegion(&region,
				  source, mask, dest,
				  x_source, y_source,
				  x_mask, y_mask,
				  x_dest, y_dest,
				  width, height))
	return;

    glBegin(GL_QUADS);
    for (i = 0; i < REGION_NUM_RECTS(&region); i++) {
	BoxPtr box = &REGION_RECTS(&region)[i];
	glVertex2i(box->x1, box->y1);
	glVertex2i(box->x2, box->y1);
	glVertex2i(box->x2, box->y2);
	glVertex2i(box->x1, box->y2);
    }
    glEnd();

    glDisable(GL_BLEND);
    glUseProgramObjectARB(0);
    REGION_UNINIT(pDst->pDrawable->pScreen, &region);

    return;

fail:
    glamor_set_composite_op(screen, PictOpSrc, dest, mask);
    glUseProgramObjectARB(0);
    glamor_solid_fail_region(dest_pixmap, x_dest, y_dest, width, height);
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

#endif /* RENDER */
