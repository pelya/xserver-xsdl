/*
 * Minimal implementation of PanoramiX/Xinerama
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/quartz/pseudoramiX.h,v 1.2 2003/04/30 23:15:39 torrey Exp $ */

extern int noPseudoramiXExtension;

void PseudoramiXAddScreen(int x, int y, int w, int h);
void PseudoramiXExtensionInit(int argc, char *argv[]);
