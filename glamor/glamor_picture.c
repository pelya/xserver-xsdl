#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdlib.h>

#include "glamor_priv.h"

/* Upload picture to texture.  We may need to flip the y axis or
 * wire alpha to 1. So we may conditional create fbo for the picture.
 * */
enum glamor_pixmap_status 
glamor_upload_picture_to_texture(PicturePtr picture)
{
  PixmapPtr pixmap;
  glamor_pixmap_private *pixmap_priv;
  assert(picture->pDrawable);
  pixmap = glamor_get_drawable_pixmap(picture->pDrawable);
  pixmap_priv = glamor_get_pixmap_private(pixmap);

  assert(GLAMOR_PIXMAP_PRIV_IS_PICTURE(pixmap_priv) == 1);
  return glamor_upload_pixmap_to_texture(pixmap);
}


Bool
glamor_prepare_access_picture(PicturePtr picture, glamor_access_t access)
{
  if (!picture || !picture->pDrawable)
    return TRUE;
    
  return glamor_prepare_access(picture->pDrawable, access);
}

void
glamor_finish_access_picture(PicturePtr picture)
{
  if (!picture || !picture->pDrawable)
    return;
    
  glamor_finish_access(picture->pDrawable);
}

/* 
 * We should already has drawable attached to it, if it has one.
 * Then set the attached pixmap to is_picture format, and set
 * the pict format.
 * */
int
glamor_create_picture(PicturePtr picture)
{
  PixmapPtr pixmap;
  glamor_pixmap_private *pixmap_priv;
  glamor_screen_private *glamor_priv;

  if (!picture || !picture->pDrawable)
    return 0;

  glamor_priv = glamor_get_screen_private(picture->pDrawable->pScreen);
  pixmap = glamor_get_drawable_pixmap(picture->pDrawable);
  pixmap_priv = glamor_get_pixmap_private(pixmap);
  assert(pixmap_priv);

  pixmap_priv->is_picture = 1;
  pixmap_priv->pict_format = picture->format;
  return glamor_priv->saved_create_picture(picture);
}

void
glamor_destroy_picture(PicturePtr picture)
{
  PixmapPtr pixmap;
  glamor_pixmap_private *pixmap_priv;
  glamor_screen_private *glamor_priv;

  if (!picture || !picture->pDrawable)
    return;

  glamor_priv = glamor_get_screen_private(picture->pDrawable->pScreen);
  pixmap = glamor_get_drawable_pixmap(picture->pDrawable);
  pixmap_priv = glamor_get_pixmap_private(pixmap);
  assert(pixmap_priv);

  pixmap_priv->is_picture = 0;
  pixmap_priv->pict_format = 0;
  glamor_priv->saved_destroy_picture(picture);
}

void glamor_picture_format_fixup(PicturePtr picture, glamor_pixmap_private *pixmap_priv)
{
  pixmap_priv->pict_format = picture->format;
}

