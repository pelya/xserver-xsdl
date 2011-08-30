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

//#include "glu3/glu3.h"

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
    GLAMOR_DEFAULT_PRECISION
    "uniform vec4 source;\n"
    "vec4 get_source()\n"
    "{\n"
    "	return source;\n"
    "}\n";
  const char *source_alpha_pixmap_fetch =
    GLAMOR_DEFAULT_PRECISION
    "varying vec2 source_texture;\n"
    "uniform sampler2D source_sampler;\n"
    "vec4 get_source()\n"
    "{\n"
    "	return texture2D(source_sampler, source_texture);\n"
    "}\n";
  const char *source_pixmap_fetch =
    GLAMOR_DEFAULT_PRECISION
    "varying vec2 source_texture;\n"
    "uniform sampler2D source_sampler;\n"
    "vec4 get_source()\n"
    "{\n"
    "       return vec4(texture2D(source_sampler, source_texture).rgb, 1);\n"
    "}\n";
  const char *mask_solid_fetch =
    GLAMOR_DEFAULT_PRECISION
    "uniform vec4 mask;\n"
    "vec4 get_mask()\n"
    "{\n"
    "	return mask;\n"
    "}\n";
  const char *mask_alpha_pixmap_fetch =
    GLAMOR_DEFAULT_PRECISION
    "varying vec2 mask_texture;\n"
    "uniform sampler2D mask_sampler;\n"
    "vec4 get_mask()\n"
    "{\n"
    "	return texture2D(mask_sampler, mask_texture);\n"
    "}\n";
  const char *mask_pixmap_fetch =
    GLAMOR_DEFAULT_PRECISION
    "varying vec2 mask_texture;\n"
    "uniform sampler2D mask_sampler;\n"
    "vec4 get_mask()\n"
    "{\n"
    "       return vec4(texture2D(mask_sampler, mask_texture).rgb, 1);\n"
    "}\n";
  const char *in_source_only =
    GLAMOR_DEFAULT_PRECISION
    "void main()\n"
    "{\n"
    "	gl_FragColor = get_source();\n"
    "}\n";
  const char *in_normal =
    GLAMOR_DEFAULT_PRECISION
    "void main()\n"
    "{\n"
    "	gl_FragColor = get_source() * get_mask().a;\n"
    "}\n";
  const char *in_ca_source =
    GLAMOR_DEFAULT_PRECISION
    "void main()\n"
    "{\n"
    "	gl_FragColor = get_source() * get_mask();\n"
    "}\n";
  const char *in_ca_alpha =
    GLAMOR_DEFAULT_PRECISION
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
 

  prog = glamor_compile_glsl_prog(GL_FRAGMENT_SHADER, source);
  free(source);

  return prog;
}

static GLuint
glamor_create_composite_vs(struct shader_key *key)
{
  const char *main_opening =
    "attribute vec4 v_position;\n"
    "attribute vec4 v_texcoord0;\n"
    "attribute vec4 v_texcoord1;\n"
    "varying vec2 source_texture;\n"
    "varying vec2 mask_texture;\n"
    "void main()\n"
    "{\n"
    "	gl_Position = v_position;\n";
  const char *source_coords =
    "	source_texture = v_texcoord0.xy;\n";
  const char *mask_coords =
    "	mask_texture = v_texcoord1.xy;\n";
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

  prog = glamor_compile_glsl_prog(GL_VERTEX_SHADER, source);
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

  prog = glCreateProgram();
  glAttachShader(prog, vs);
  glAttachShader(prog, fs);

  glBindAttribLocation(prog, GLAMOR_VERTEX_POS, "v_position");
  glBindAttribLocation(prog, GLAMOR_VERTEX_SOURCE, "v_texcoord0");
  glBindAttribLocation(prog, GLAMOR_VERTEX_MASK, "v_texcoord1");

  glamor_link_glsl_prog(prog);

  shader->prog = prog;

  glUseProgram(prog);

  if (key->source == SHADER_SOURCE_SOLID) {
    shader->source_uniform_location = glGetUniformLocation(prog,
							      "source");
  } else {
    source_sampler_uniform_location = glGetUniformLocation(prog,
							      "source_sampler");
    glUniform1i(source_sampler_uniform_location, 0);
  }

  if (key->mask != SHADER_MASK_NONE) {
    if (key->mask == SHADER_MASK_SOLID) {
      shader->mask_uniform_location = glGetUniformLocation(prog,
							      "mask");
    } else {
      mask_sampler_uniform_location = glGetUniformLocation(prog,
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
#define GLAMOR_COMPOSITE_VBO_SIZE 8192

static void
glamor_reset_composite_vbo(ScreenPtr screen)
{
  glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
  glamor_priv->vbo_offset = 0;
  glamor_priv->vbo_size = GLAMOR_COMPOSITE_VBO_SIZE;
  glamor_priv->render_nr_verts = 0;
}


void
glamor_init_composite_shaders(ScreenPtr screen)
{
  glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
  glamor_priv->vb = malloc(GLAMOR_COMPOSITE_VBO_SIZE);
  assert(glamor_priv->vb != NULL);
  glamor_reset_composite_vbo(screen);
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
#ifndef GLAMOR_GLES2
    /* XXX  GLES2 doesn't support GL_CLAMP_TO_BORDER. */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
#endif
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
#ifndef GLAMOR_GLES2
  glEnable(GL_TEXTURE_2D);
#endif
}

static void
glamor_set_composite_solid(float *color, GLint uniform_location)
{
  glUniform4fv(uniform_location, 1, color);
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

static void
glamor_setup_composite_vbo(ScreenPtr screen)
{
  glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

  glamor_priv->vb_stride = 2 * sizeof(float);
  if (glamor_priv->has_source_coords)
    glamor_priv->vb_stride += 2 * sizeof(float);
  if (glamor_priv->has_mask_coords)
    glamor_priv->vb_stride += 2 * sizeof(float);

  glBindBuffer(GL_ARRAY_BUFFER, glamor_priv->vbo);

  glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_FLOAT, GL_FALSE, 
                        glamor_priv->vb_stride,
                       (void *)((long)glamor_priv->vbo_offset));
  glEnableVertexAttribArray(GLAMOR_VERTEX_POS);

  if (glamor_priv->has_source_coords) {
    glVertexAttribPointer(GLAMOR_VERTEX_SOURCE, 2, GL_FLOAT, GL_FALSE, 
                        glamor_priv->vb_stride,
                       (void *)((long)glamor_priv->vbo_offset + 2 * sizeof(float)));
    glEnableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
  }

  if (glamor_priv->has_mask_coords) {
    glVertexAttribPointer(GLAMOR_VERTEX_MASK, 2, GL_FLOAT, GL_FALSE, 
                        glamor_priv->vb_stride,
                       (void *)((long)glamor_priv->vbo_offset + 
			       (glamor_priv->has_source_coords ? 4 : 2) *
                               sizeof(float)));
    glEnableVertexAttribArray(GLAMOR_VERTEX_MASK);
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
  glBufferData(GL_ARRAY_BUFFER, glamor_priv->vbo_offset, glamor_priv->vb,
	    GL_STREAM_DRAW);

  glDrawArrays(GL_TRIANGLES, 0, glamor_priv->render_nr_verts);
  glamor_reset_composite_vbo(screen);
}

static void
glamor_emit_composite_rect(ScreenPtr screen,
			   const float *src_coords,
			   const float *mask_coords,
			   const float *dst_coords)
{
  glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

  if (glamor_priv->vbo_offset + 6 * glamor_priv->vb_stride >
      glamor_priv->vbo_size)
    {
      glamor_flush_composite_rects(screen);
    }

  if (glamor_priv->vbo_offset == 0) {
    if (glamor_priv->vbo == 0)
      glGenBuffers(1, &glamor_priv->vbo);

    glamor_setup_composite_vbo(screen);
  }

  glamor_emit_composite_vert(screen, src_coords, mask_coords, dst_coords, 0);
  glamor_emit_composite_vert(screen, src_coords, mask_coords, dst_coords, 1);
  glamor_emit_composite_vert(screen, src_coords, mask_coords, dst_coords, 2);
  glamor_emit_composite_vert(screen, src_coords, mask_coords, dst_coords, 0);
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
  GLfloat source_solid_color[4], mask_solid_color[4];

  dest_pixmap_priv = glamor_get_pixmap_private(dest_pixmap);

  if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(dest_pixmap_priv)) {
    glamor_fallback("dest has no fbo.\n");
    goto fail;
  }
  memset(&key, 0, sizeof(key));
  if (!source->pDrawable) {
    if (source->pSourcePict->type == SourcePictTypeSolidFill) {
      key.source = SHADER_SOURCE_SOLID;
      glamor_get_rgba_from_pixel(source->pSourcePict->solidFill.color,
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
      if (mask->pSourcePict->type == SourcePictTypeSolidFill) {
	key.mask = SHADER_MASK_SOLID;
        glamor_get_rgba_from_pixel(mask->pSourcePict->solidFill.color,
				 &mask_solid_color[0], 
				 &mask_solid_color[1], 
				 &mask_solid_color[2], 
				 &mask_solid_color[3], 
				 PICT_a8r8g8b8);
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
    else if (source_pixmap_priv->pending_op.type == GLAMOR_PENDING_FILL) {
      key.source = SHADER_SOURCE_SOLID;
      memcpy(source_solid_color, source_pixmap_priv->pending_op.fill.color4fv, 4 * sizeof(float));
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
    else if (mask_pixmap_priv->pending_op.type == GLAMOR_PENDING_FILL) {
      key.mask = SHADER_MASK_SOLID;
      memcpy(mask_solid_color, mask_pixmap_priv->pending_op.fill.color4fv, 4 * sizeof(float));
    }
  }
#ifdef GLAMOR_PIXMAP_DYNAMIC_UPLOAD
  if (source_status == GLAMOR_UPLOAD_PENDING 
      && mask_status == GLAMOR_UPLOAD_PENDING 
      && source_pixmap == mask_pixmap) {

    if (source->format != mask->format) {
      saved_source_format = source->format;

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
  glamor_validate_pixmap(dest_pixmap);
  if (!glamor_set_composite_op(screen, op, dest, mask)) {
    goto fail;
  }

  shader = glamor_lookup_composite_shader(screen, &key);
  if (shader->prog == 0) {
    glamor_fallback("no shader program for this render acccel mode\n");
    goto fail;
  }

  glUseProgram(shader->prog);


  if (key.source == SHADER_SOURCE_SOLID) {
    glamor_set_composite_solid(source_solid_color, shader->source_uniform_location);
  } else {
    glamor_set_composite_texture(screen, 0, source, source_pixmap_priv);
  }
  if (key.mask != SHADER_MASK_NONE) {
    if (key.mask == SHADER_MASK_SOLID) {
      glamor_set_composite_solid(mask_solid_color, shader->mask_uniform_location);
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



  if (glamor_priv->has_source_coords) {
    glamor_get_drawable_deltas(source->pDrawable, source_pixmap,
			       &source_x_off, &source_y_off);
    pixmap_priv_get_scale(source_pixmap_priv, &src_xscale, &src_yscale);
    glamor_picture_get_matrixf(source, src_matrix);
  }

  if (glamor_priv->has_mask_coords) {
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

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
  glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
  glDisableVertexAttribArray(GLAMOR_VERTEX_MASK);
  REGION_UNINIT(dst->pDrawable->pScreen, &region);
  glDisable(GL_BLEND);
#ifndef GLAMOR_GLES2
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);
  glActiveTexture(GL_TEXTURE1);
  glDisable(GL_TEXTURE_2D);
#endif
  glUseProgram(0);
  if (saved_source_format) 
    source->format = saved_source_format;
  return TRUE;

 fail:
  if (saved_source_format) 
    source->format = saved_source_format;

  glDisable(GL_BLEND);
  glUseProgram(0);
  return FALSE;
}

static PicturePtr
glamor_convert_gradient_picture(ScreenPtr screen,
                                PicturePtr source,
                                int x_source,
                                int y_source,
                                int width,
                                int height)
{
  PixmapPtr pixmap;
  PicturePtr dst;
  int error;
  PictFormatShort format;

  if (!source->pDrawable)
    format = PICT_a8r8g8b8;
  else
    format = source->format;

  pixmap = screen->CreatePixmap(screen, 
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
                      0, 
                      0, 
                      serverClient, 
                      &error);
  screen->DestroyPixmap(pixmap);
  if (!dst)
     return NULL;

  ValidatePicture(dst);

  fbComposite(PictOpSrc, source, NULL, dst, x_source, y_source,
              0, 0, 0, 0, width, height);
  return dst;
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
  glamor_pixmap_private *dest_pixmap_priv;
  glamor_pixmap_private *source_pixmap_priv = NULL, *mask_pixmap_priv = NULL;
  PixmapPtr dest_pixmap = glamor_get_drawable_pixmap(dest->pDrawable);
  PixmapPtr source_pixmap = NULL, mask_pixmap = NULL;
  PicturePtr temp_src = source, temp_mask = mask;
  int x_temp_src, y_temp_src, x_temp_mask, y_temp_mask;
  glamor_composite_rect_t rect;
  glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

  x_temp_src = x_source;
  y_temp_src = y_source;
  x_temp_mask = x_mask;
  y_temp_mask = y_mask;

  dest_pixmap_priv = glamor_get_pixmap_private(dest_pixmap);
  /* Currently. Always fallback to cpu if destination is in CPU memory. */
  if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(dest_pixmap_priv)) {
    goto fail;
  }

  if (source->pDrawable) {
    source_pixmap = glamor_get_drawable_pixmap(source->pDrawable);
    source_pixmap_priv = glamor_get_pixmap_private(source_pixmap);
  }

  if (mask && mask->pDrawable) {
    mask_pixmap = glamor_get_drawable_pixmap(mask->pDrawable);
    mask_pixmap_priv = glamor_get_pixmap_private(mask_pixmap);
  }

  if ((!source->pDrawable && (source->pSourcePict->type != SourcePictTypeSolidFill))
      || (source->pDrawable 
	  && !GLAMOR_PIXMAP_PRIV_HAS_FBO(source_pixmap_priv) 
	  && ((width * height * 4 
	       < (source_pixmap->drawable.width * source_pixmap->drawable.height))
	      || !(glamor_check_fbo_size(glamor_priv, source_pixmap->drawable.width,
						 source_pixmap->drawable.height))))) {
    temp_src = glamor_convert_gradient_picture(screen, source, x_source, y_source, width, height);
    if (!temp_src) {
      temp_src = source;
      goto fail; 
    }
    x_temp_src = y_temp_src = 0;
  }

  if (mask 
      && ((!mask->pDrawable && (mask->pSourcePict->type != SourcePictTypeSolidFill))
	  || (mask->pDrawable 
	      && (!GLAMOR_PIXMAP_PRIV_HAS_FBO(mask_pixmap_priv))
	      && ((width * height * 4 
		   < (mask_pixmap->drawable.width * mask_pixmap->drawable.height))
		  || !(glamor_check_fbo_size(glamor_priv, mask_pixmap->drawable.width,
						     mask_pixmap->drawable.height)))))) {
    /* XXX if mask->pDrawable is the same as source->pDrawable, we have an opportunity
     * to do reduce one convertion. */
    temp_mask = glamor_convert_gradient_picture(screen, mask, x_mask, y_mask, width, height);
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
		       x_dest, y_dest,
		       width, height);
      glamor_composite(PictOpAdd,
		       temp_src, temp_mask, dest,
		       x_temp_src, y_temp_src,
		       x_temp_mask, y_temp_mask,
		       x_dest, y_dest,
		       width, height);
      goto done;

    } else if (op != PictOpAdd && op != PictOpOutReverse) {
      glamor_fallback("glamor_composite(): component alpha\n");
      goto fail;
    }
  }

  if (!mask) {
    if (glamor_composite_with_copy(op, temp_src, dest,
				   x_temp_src, y_temp_src,
				   x_dest, y_dest,
				   width, height))
      goto done;
  }

  rect.x_src = x_temp_src;
  rect.y_src = y_temp_src;
  rect.x_mask = x_temp_mask;
  rect.y_mask = y_temp_mask;
  rect.x_dst = x_dest;
  rect.y_dst = y_dest;
  rect.width = width;
  rect.height = height;
  if (glamor_composite_with_shader(op, temp_src, temp_mask, dest, 1, &rect))
    goto done;

fail:

  glamor_fallback(
		  "from picts %p:%p %dx%d / %p:%p %d x %d (%c,%c)  to pict %p:%p %dx%d (%c)\n",
		  source, 
                  source->pDrawable,
		  source->pDrawable ? source->pDrawable->width : 0,
		  source->pDrawable ? source->pDrawable->height : 0,
		  mask, 
                  (!mask) ? NULL : mask->pDrawable,
		  (!mask || !mask->pDrawable)? 0 : mask->pDrawable->width,
		  (!mask || !mask->pDrawable)? 0 : mask->pDrawable->height,
		  glamor_get_picture_location(source),
		  glamor_get_picture_location(mask),
		  dest,
		  dest->pDrawable,
		  dest->pDrawable->width,
		  dest->pDrawable->height,
		  glamor_get_picture_location(dest));

  glUseProgram(0);
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
done:
    if (temp_src != source)
      FreePicture(temp_src, 0);
    if (temp_mask != mask)
      FreePicture(temp_mask, 0);
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

  pixmap = screen->CreatePixmap(screen, 0, 0,
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
  stride = PixmapBytePad(width, mask_format->depth);
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

  pixmap = glamor_get_drawable_pixmap(picture->pDrawable);
 
  screen->ModifyPixmapHeader(pixmap, width, height, 
                             mask_format->depth, 
	                     BitsPerPixel(mask_format->depth),
			     PixmapBytePad(width, mask_format->depth),
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
}

void
glamor_composite_rects(CARD8 op,
		       PicturePtr src, PicturePtr mask, PicturePtr dst,
		       int nrect, glamor_composite_rect_t *rects)
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
		     r->x_dst, r->y_dst,
		     r->width, r->height);
    r++;
  }
}

#endif /* RENDER */
