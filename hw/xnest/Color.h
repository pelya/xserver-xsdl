/* $Xorg: Color.h,v 1.3 2000/08/17 19:53:28 cpqbld Exp $ */
/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/

#ifndef XNESTCOLOR_H
#define XNESTCOLOR_H

#define DUMB_WINDOW_MANAGERS

#define MAXCMAPS 1
#define MINCMAPS 1

typedef struct {
  Colormap colormap;
} xnestPrivColormap;

typedef struct {
  int numCmapIDs;
  Colormap *cmapIDs;
  int numWindows;
  Window *windows;
  int index;
} xnestInstalledColormapWindows;

#define xnestColormapPriv(pCmap) \
  ((xnestPrivColormap *)((pCmap)->devPriv))

#define xnestColormap(pCmap) (xnestColormapPriv(pCmap)->colormap)

#define xnestPixel(pixel) (pixel)

Bool xnestCreateColormap();
void xnestDestroyColormap ();
void xnestSetInstalledColormapWindows();
void xnestSetScreenSaverColormapWindow();
void xnestDirectInstallColormaps();
void xnestDirectUninstallColormaps();
void xnestInstallColormap();
void xnestUninstallColormap();
int xnestListInstalledColormaps();
void xnestStoreColors();
void xnestResolveColor();
Bool xnestCreateDefaultColormap();

#endif /* XNESTCOLOR_H */
