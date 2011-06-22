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

static GLuint
glamor_create_composite_fs(struct shader_key *key)
{
  const char *source_solid_fetch =
    "uniform vec4 source;\n"
    "vec4 get_source()\n"
    "{\n"
    "	return source;\n"
    "}\n";
  const char *source_alpha_pixmap_fetch =
    "uniform sampler2D source_sampler;\n"
    "vec4 get_source()\n"
    "{\n"
    "	return texture2D(source_sampler, gl_TexCoord[0].xy);\n"
    "}\n";
  const char *source_pixmap_fetch =
    "uniform sampler2D source_sampler;\n"
    "vec4 get_source()\n"
    "{\n"
    "       return vec4(texture2D(source_sampler, gl_TexCoord[0].xy).rgb, 1);\n"
    "}\n";
  const char *mask_solid_fetch =
    "uniform vec4 mask;\n"
    "vec4 get_mask()\n"
    "{\n"
    "	return mask;\n"
    "}\n";
  const char *mask_alpha_pixmap_fetch =
    "uniform sampler2D mask_sampler;\n"
    "vec4 get_mask()\n"
    "{\n"
    "	return texture2D(mask_sampler, gl_TexCoord[1].xy);\n"
    "}\n";
  const char *mask_pixmap_fetch =
    "uniform sampler2D mask_sampler;\n"
    "vec4 get_mask()\n"
    "{\n"
    "       return vec4(texture2D(mask_sampler, gl_TexCoord[1].xy).rgb, 1);\n"
    "}\n";
  const char *in_source_only =
    "void main()\n"
    "{\n"
    "	gl_FragColor = get_source();\n"
    "}\n";
  const char *in_normal =
    "void main()\n"
    "{\n"
    "	gl_FragColor = get_source() * get_mask().a;\n"
    "}\n";
  const char *in_ca_source =
    "void main()\n"
    "{\n"
    "	gl_FragColor = get_source() * get_mask();\n"
    "}\n";
  const char *in_ca_alpha =
    "void main()\n"
    "{\n"
    "	gl_FragColor = get_source().a * get_mask();\n"
    "}\n";
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

  XNFasprintf(&source,
	      "%s%s%s",
	      source_fetch,
	      mask_fetch,
	      in);
 

  prog = glamor_compile_glsl_prog(GL_FRAGMENT_SHADER_ARB, source);
  free(source);

  return prog;
}

static GLuint
glamor_create_composite_vs(struct shader_key *key)
{
  const char *main_opening =
    "void main()\n"
    "{\n"
    "	gl_Position = gl_Vertex;\n";
  const char *source_coords =
    "	gl_TexCoord[0] = gl_MultiTexCoord0;\n";
  const char *mask_coords =
    "	gl_TexCoord[1] = gl_MultiTexCoord1;\n";
  const char *main_closing =
    "}\n";
  const char *source_coords_setup = "";
  const char *mask_coords_setup = "";
  char *source;
  GLuint prog;

  if (key->source != SHADER_SOURCE_SOLID)
    source_coords_setup = source_coords;

  if (key->mask != SHADER_MASK_NONE && key->mask != SHADER_MASK_SOLID)
    mask_coords_setup = mask_coords;

  XNFasprintf(&source,
	      "%s%s%s%s",
	      main_opening,
	      source_coords_setup,
	      mask_coords_setup,
	      main_closing);

  prog = glamor_compile_glsl_prog(GL_VERTEX_SHADER_ARB, source);
  free(source);

  return prog;
}

static void
glamor_create_composite_shader(ScreenPtr screen, struct shader_key *key,
			       glamor_composite_shader *shader)
{
  GLuint vs, fs, prog;
  GLint source_sampler_uniform_location, mask_sampler_uniform_location;

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

  if (key->source == SHADER_SOURCE_SOLID) {
    shader->source_uniform_location = glGetUniformLocationARB(prog,
							      "source");
  } else {
    source_sampler_uniform_location = glGetUniformLocationARB(prog,
							      "source_sampler");
    glUniform1i(source_sampler_uniform_location, 0);
  }

  if (key->mask != SHADER_MASK_NONE) {
    if (key->mask == SHADER_MASK_SOLID) {
      shader->mask_uniform_location = glGetUniformLocationARB(prog,
							      "mask");
    } else {
      mask_sampler_uniform_location = glGetUniformLocationARB(prog,
							      "mask_sampler");
      glUniform1i(mask_sampler_uniform_location, 1);
    }
  }
}

static glamor_composite_shader *
glamor_lookup_composite_shader(ScreenPtr screen, struct shader_key *key)
{
  glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
  glamor_composite_shader *shader;

  shader = &glamor_priv->composite_shader[key->source][key->mask][key->in];
  if (shader->prog == 0)
    glamor_create_composite_shader(screen, key, shader);

  return shader;
}

void
glamor_init_composite_shaders(ScreenPtr screen)
{
}

static Bool
glamor_set_composite_op(ScreenPtr screen,
			CARD8 op, PicturePtr dest, PicturePtr mask)
{
  GLenum source_blend, dest_blend;
  struct blendinfo *op_info;

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
  if (mask && mask->componentAlpha && PICT_FORMAT_RGB(mask->format) != 0 &&
      op_info->source_alpha) {
    if (source_blend != GL_ZERO) {
      glamor_fallback("Dual-source composite blending not supported\n");
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
  case PICT_a1:
  case PICT_a8:
  case PICT_a8r8g8b8:
  case PICT_x8r8g8b8:
    return TRUE;
  default:
    return TRUE;
    glamor_fallback("Bad source format 0x%08x\n", picture->format);
    return FALSE;
  }
}

static Bool
good_mask_format(PicturePtr picture)
{
  switch (picture->format) {
  case PICT_a1:
  case PICT_a8:
  case PICT_a8r8g8b8:
  case PICT_x8r8g8b8:
    return TRUE;
  default:
    return TRUE;
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
    return TRUE;
    glamor_fallback("Bad dest format 0x%08x\n", picture->format);
    return FALSE;
  }
}

static void
glamor_setup_composite_vbo(ScreenPtr screen)
{
  glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

  glamor_priv->vb_stride = 2 * sizeof(float);
  if (glamor_priv->has_source_coords)
    glamor_priv->vb_stride += 2 * sizeof(float);
  if (glamor_priv->has_mask_coords)
    glamor_priv->vb_stride += 2 * sizeof(float);

  glBindBufferARB(GL_ARRAY_BUFFER_ARB, glamor_priv->vbo);
  glVertexPointer(2, GL_FLOAT, glamor_priv->vb_stride,
		  (void *)((long)glamor_priv->vbo_offset));
  glEnableClientState(GL_VERTEX_ARRAY);

  if (glamor_priv->has_source_coords) {
    glClientActiveTexture(GL_TEXTURE0);
    glTexCoordPointer(2, GL_FLOAT, glamor_priv->vb_stride,
		      (void *)(glamor_priv->vbo_offset + 2 * sizeof(float)));
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  }

  if (glamor_priv->has_mask_coords) {
    glClientActiveTexture(GL_TEXTURE1);
    glTexCoordPointer(2, GL_FLOAT, glamor_priv->vb_stride,
		      (void *)(glamor_priv->vbo_offset +
			       (glamor_priv->has_source_coords ? 4 : 2) *
			       sizeof(float)));
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  }
}

static void
glamor_emit_composite_vert(ScreenPtr screen,
			   const float *src_coords,
			   const float *mask_coords,
			   const float *dst_coords,
			   int i)
{
  glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
  float *vb = (float *)(glamor_priv->vb + glamor_priv->vbo_offset);
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
  glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

  if (!glamor_priv->render_nr_verts)
    return;

  glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
  glamor_priv->vb = NULL;

  glDrawArrays(GL_QUADS, 0, glamor_priv->render_nr_verts);
  glamor_priv->render_nr_verts = 0;
  glamor_priv->vbo_size = 0;
}

static void
glamor_emit_composite_rect(ScreenPtr screen,
			   const float *src_coords,
			   const float *mask_coords,
			   const float *dst_coords)
{
  glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

  if (glamor_priv->vbo_offset + 4 * glamor_priv->vb_stride >
      glamor_priv->vbo_size)
    {
      glamor_flush_composite_rects(screen);
    }

  if (glamor_priv->vbo_size == 0) {
    if (glamor_priv->vbo == 0)
      glGenBuffersARB(1, &glamor_priv->vbo);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, glamor_priv->vbo);

    glamor_priv->vbo_size = 4096;
    glBufferDataARB(GL_ARRAY_BUFFER_ARB, glamor_priv->vbo_size, NULL,
		    GL_STREAM_DRAW_ARB);
    glamor_priv->vbo_offset = 0;
    glamor_priv->vb = glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    glamor_setup_composite_vbo(screen);
  }

  glamor_emit_composite_vert(screen, src_coords, mask_coords, dst_coords, 0);
  glamor_emit_composite_vert(screen, src_coords, mask_coords, dst_coords, 1);
  glamor_emit_composite_vert(screen, src_coords, mask_coords, dst_coords, 2);
  glamor_emit_composite_vert(screen, src_coords, mask_coords, dst_coords, 3);
}


int pict_format_combine_tab[][3] = 
  {
    {PICT_TYPE_ARGB, PICT_TYPE_A, PICT_TYPE_ARGB},
    {PICT_TYPE_ABGR, PICT_TYPE_A, PICT_TYPE_ABGR},
  };

static Bool 
combine_pict_format(PictFormatShort *des, const PictFormatShort src, 
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

  switch(in_ca) {
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

  for(i = 0; 
      i < sizeof(pict_format_combine_tab)/sizeof(pict_format_combine_tab[0]);
      i++) 
    {
      if ((src_type == pict_format_combine_tab[i][0] 
	   && mask_type == pict_format_combine_tab[i][1])
	  ||(src_type == pict_format_combine_tab[i][1]
	     && mask_type == pict_format_combine_tab[i][0])) {
        *des = PICT_VISFORMAT(src_bpp, pict_format_combine_tab[i][2], new_vis);
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
			     int nrect,
			     glamor_composite_rect_t *rects)
{
  ScreenPtr screen = dest->pDrawable->pScreen;
  glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
  PixmapPtr dest_pixmap = glamor_get_drawable_pixmap(dest->pDrawable);
  PixmapPtr source_pixmap = NULL, mask_pixmap = NULL;
  glamor_pixmap_private *source_pixmap_priv = NULL;
  glamor_pixmap_private *mask_pixmap_priv = NULL;
  glamor_pixmap_private *dest_pixmap_priv = NULL;
  GLfloat dst_xscale, dst_yscale;
  GLfloat mask_xscale = 1, mask_yscale = 1, src_xscale = 1, src_yscale = 1;
  struct shader_key key;
  glamor_composite_shader *shader;
  RegionRec region;
  float vertices[8], source_texcoords[8], mask_texcoords[8];
  int i;
  BoxPtr box;
  int dest_x_off, dest_y_off;
  int source_x_off, source_y_off;
  int mask_x_off, mask_y_off;
  enum glamor_pixmap_status source_status = GLAMOR_NONE;
  enum glamor_pixmap_status mask_status = GLAMOR_NONE;
  PictFormatShort saved_source_format = 0;
  float src_matrix[9], mask_matrix[9];

  dest_pixmap_priv = glamor_get_pixmap_private(dest_pixmap);

  if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(dest_pixmap_priv)) {
    glamor_fallback("dest has no fbo.\n");
    goto fail;
  }
  memset(&key, 0, sizeof(key));
  if (!source->pDrawable) {
    if (source->pSourcePict->type == SourcePictTypeSolidFill) {
      key.source = SHADER_SOURCE_SOLID;
    } else {
      glamor_fallback("gradient source\n");
      goto fail;
    }
  } else {
    key.source = SHADER_SOURCE_TEXTURE_ALPHA;
  }
  if (mask) {
    if (!mask->pDrawable) {
      if (mask->pSourcePict->type == SourcePictTypeSolidFill) {
	key.mask = SHADER_MASK_SOLID;
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
      /* We only handle two CA modes. */
      if (op == PictOpAdd)
	key.in = SHADER_IN_CA_SOURCE;
      else if (op == PictOpOutReverse) {
	key.in = SHADER_IN_CA_ALPHA;
      } else {
	glamor_fallback("Unsupported component alpha op: %d\n", op);
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
    source_pixmap = glamor_get_drawable_pixmap(source->pDrawable);
    source_pixmap_priv = glamor_get_pixmap_private(source_pixmap);
    if (source_pixmap == dest_pixmap) {
      glamor_fallback("source == dest\n");
      goto fail;
    }
    if (!source_pixmap_priv || source_pixmap_priv->gl_tex == 0) {
#ifdef GLAMOR_PIXMAP_DYNAMIC_UPLOAD
      source_status = GLAMOR_UPLOAD_PENDING;
#else
      glamor_fallback("no texture in source\n");
      goto fail;
#endif
    }
    if ((source_status != GLAMOR_UPLOAD_PENDING) 
	&& !good_source_format(source)) {
      goto fail;
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
    if (!mask_pixmap_priv || mask_pixmap_priv->gl_tex == 0) {
#ifdef GLAMOR_PIXMAP_DYNAMIC_UPLOAD
      mask_status = GLAMOR_UPLOAD_PENDING;
#else
      glamor_fallback("no texture in mask\n");
      goto fail;
#endif
    }
    if ((mask_status != GLAMOR_UPLOAD_PENDING) 
	&& !good_mask_format(mask)) {
      goto fail;
    }
  }
  if (!good_dest_format(dest)) {
    goto fail;
  }
  if (!glamor_set_composite_op(screen, op, dest, mask)) {
    goto fail;
  }

#ifdef GLAMOR_PIXMAP_DYNAMIC_UPLOAD
  if (source_status == GLAMOR_UPLOAD_PENDING 
      && mask_status == GLAMOR_UPLOAD_PENDING 
      && source_pixmap == mask_pixmap) {

    if (source->format != mask->format) {
      saved_source_format = source->format;
      /* XXX
       * when need to flip the texture and mask and source share the same pixmap,
       * there is a bug, need to be fixed. *
       */
      if (!glamor_priv->yInverted)
	goto fail;
      if (!combine_pict_format(&source->format, source->format, mask->format, key.in)) {
	glamor_fallback("combine source %x mask %x failed.\n", 
			source->format, mask->format);
	goto fail;
      }
      if (source->format != saved_source_format) {
	glamor_picture_format_fixup(source, source_pixmap_priv);
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
      if (!PICT_FORMAT_A(saved_source_format) && PICT_FORMAT_A(mask->format))
	key.source = SHADER_SOURCE_TEXTURE;
    
      if (!PICT_FORMAT_A(mask->format) && PICT_FORMAT_A(saved_source_format))
	key.mask = SHADER_MASK_TEXTURE;

      mask_status = GLAMOR_NONE;
    }
    source_status = glamor_upload_picture_to_texture(source);
     
    if (source_status != GLAMOR_UPLOAD_DONE) {
      glamor_fallback("Failed to upload source texture.\n");
      goto fail;
    }
  } 
  else {

    if (source_status == GLAMOR_UPLOAD_PENDING) {
      source_status = glamor_upload_picture_to_texture(source);
      if (source_status != GLAMOR_UPLOAD_DONE) {
	glamor_fallback("Failed to upload source texture.\n");
	goto fail;
      }
    }

    if (mask_status == GLAMOR_UPLOAD_PENDING) {
      mask_status = glamor_upload_picture_to_texture(mask);
      if (mask_status != GLAMOR_UPLOAD_DONE) {
	glamor_fallback("Failed to upload mask texture.\n");
	goto fail;
      }
    }
  }
#endif

  glamor_set_destination_pixmap_priv_nc(dest_pixmap_priv);

  shader = glamor_lookup_composite_shader(screen, &key);
  if (shader->prog == 0) {
    glamor_fallback("no shader program for this render acccel mode\n");
    goto fail;
  }

  glUseProgramObjectARB(shader->prog);
  if (key.source == SHADER_SOURCE_SOLID) {
    glamor_set_composite_solid(source, shader->source_uniform_location);
  } else {
    glamor_set_composite_texture(screen, 0, source, source_pixmap_priv);
  }
  if (key.mask != SHADER_MASK_NONE) {
    if (key.mask == SHADER_MASK_SOLID) {
      glamor_set_composite_solid(mask, shader->mask_uniform_location);
    } else {
      glamor_set_composite_texture(screen, 1, mask, mask_pixmap_priv);
    }
  }

  glamor_priv->has_source_coords = key.source != SHADER_SOURCE_SOLID;
  glamor_priv->has_mask_coords = (key.mask != SHADER_MASK_NONE &&
				  key.mask != SHADER_MASK_SOLID);

  glamor_get_drawable_deltas(dest->pDrawable, dest_pixmap,
			     &dest_x_off, &dest_y_off);
  pixmap_priv_get_scale(dest_pixmap_priv, &dst_xscale, &dst_yscale);



  if (source_pixmap) {
    glamor_get_drawable_deltas(source->pDrawable, source_pixmap,
			       &source_x_off, &source_y_off);
    pixmap_priv_get_scale(source_pixmap_priv, &src_xscale, &src_yscale);
    glamor_picture_get_matrixf(source, src_matrix);
  }

  if (mask_pixmap) {
    glamor_get_drawable_deltas(mask->pDrawable, mask_pixmap,
			       &mask_x_off, &mask_y_off);
    pixmap_priv_get_scale(mask_pixmap_priv, &mask_xscale, &mask_yscale);
    glamor_picture_get_matrixf(mask, mask_matrix);
  }

  while (nrect--) {
    INT16 x_source;
    INT16 y_source;
    INT16 x_mask;
    INT16 y_mask;
    INT16 x_dest;
    INT16 y_dest;
    CARD16 width;
    CARD16 height;

    x_dest = rects->x_dst;
    y_dest = rects->y_dst;
    x_source = rects->x_src;
    y_source = rects->y_src;
    x_mask = rects->x_mask;
    y_mask = rects->y_mask;
    width = rects->width;
    height = rects->height;

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

    if (!miComputeCompositeRegion(&region,
				  source, mask, dest,
				  x_source, y_source,
				  x_mask, y_mask,
				  x_dest, y_dest,
				  width, height))
      continue;

    x_source += source_x_off;
    y_source += source_y_off;
    x_mask += mask_x_off;
    y_mask += mask_y_off;

    box = REGION_RECTS(&region);
    for (i = 0; i < REGION_NUM_RECTS(&region); i++) {
      int vx1 = box[i].x1 + dest_x_off;
      int vx2 = box[i].x2 + dest_x_off;
      int vy1 = box[i].y1 + dest_y_off;
      int vy2 = box[i].y2 + dest_y_off;
      glamor_set_normalize_vcoords(dst_xscale, dst_yscale, vx1, vy1, vx2, vy2, 
				   glamor_priv->yInverted, vertices);

      if (key.source != SHADER_SOURCE_SOLID) {
	int tx1 = box[i].x1 + x_source - x_dest;
	int ty1 = box[i].y1 + y_source - y_dest;
	int tx2 = box[i].x2 + x_source - x_dest;
	int ty2 = box[i].y2 + y_source - y_dest;
	if (source->transform)
	  glamor_set_transformed_normalize_tcoords(src_matrix, src_xscale, src_yscale, 
						   tx1, ty1, tx2, ty2,
						   glamor_priv->yInverted, 
						   source_texcoords);
	else
	  glamor_set_normalize_tcoords(src_xscale, src_yscale, tx1, ty1, tx2, ty2,
				       glamor_priv->yInverted, source_texcoords);
      }   

      if (key.mask != SHADER_MASK_NONE && key.mask != SHADER_MASK_SOLID) {
	float tx1 = box[i].x1 + x_mask - x_dest;
	float ty1 = box[i].y1 + y_mask - y_dest;
	float tx2 = box[i].x2 + x_mask - x_dest;
	float ty2 = box[i].y2 + y_mask - y_dest;
	if (mask->transform)
	  glamor_set_transformed_normalize_tcoords(mask_matrix, mask_xscale, mask_yscale, 
						   tx1, ty1, tx2, ty2,
						   glamor_priv->yInverted, 
						   mask_texcoords);
	else
	  glamor_set_normalize_tcoords(mask_xscale, mask_yscale, tx1, ty1, tx2, ty2,
				       glamor_priv->yInverted, mask_texcoords);
      }
      glamor_emit_composite_rect(screen, source_texcoords,
				 mask_texcoords, vertices);
    }
    rects++;
  }
  glamor_flush_composite_rects(screen);

  glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
  glClientActiveTexture(GL_TEXTURE0);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glClientActiveTexture(GL_TEXTURE1);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);

  REGION_UNINIT(dst->pDrawable->pScreen, &region);
  glDisable(GL_BLEND);
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);
  glActiveTexture(GL_TEXTURE1);
  glDisable(GL_TEXTURE_2D);
  glUseProgramObjectARB(0);
  if (saved_source_format) 
    source->format = saved_source_format;
  return TRUE;

 fail:
  if (saved_source_format) 
    source->format = saved_source_format;

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
  glamor_composite_rect_t rect;

  /* Do two-pass PictOpOver componentAlpha, until we enable
   * dual source color blending.
   */
  if (mask && mask->componentAlpha) {
    if (op == PictOpOver) {
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
    } else if (op != PictOpAdd && op != PictOpOutReverse) {
      glamor_fallback("glamor_composite(): component alpha\n");
      goto fail;
    }
  }

  if (!mask) {
    if (glamor_composite_with_copy(op, source, dest,
				   x_source, y_source,
				   x_dest, y_dest,
				   width, height))
      return;
  }

  rect.x_src = x_source;
  rect.y_src = y_source;
  rect.x_mask = x_mask;
  rect.y_mask = y_mask;
  rect.x_dst = x_dest;
  rect.y_dst = y_dest;
  rect.width = width;
  rect.height = height;
  if (glamor_composite_with_shader(op, source, mask, dest, 1, &rect))
    return;

 fail:
  glamor_fallback("glamor_composite(): "
		  "from picts %p/%p(%c,%c) to pict %p (%c)\n",
		  source, mask,
		  glamor_get_picture_location(source),
		  glamor_get_picture_location(mask),
		  dest,
		  glamor_get_picture_location(dest));

  glUseProgramObjectARB(0);
  glDisable(GL_BLEND);
  if (glamor_prepare_access_picture(dest, GLAMOR_ACCESS_RW)) {
    if (glamor_prepare_access_picture(source, GLAMOR_ACCESS_RO))
      {
	if (!mask ||
	    glamor_prepare_access_picture(mask, GLAMOR_ACCESS_RO))
	  {
	    fbComposite(op,
			source, mask, dest,
			x_source, y_source,
			x_mask, y_mask,
			x_dest, y_dest,
			width, height);
	    if (mask)
	      glamor_finish_access_picture(mask);
	  }
	glamor_finish_access_picture(source);
      }
    glamor_finish_access_picture(dest);
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

  if (glamor_composite_with_shader(op, src, NULL, dst, nrect, rects))
    return;

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
