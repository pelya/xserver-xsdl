/* $Xorg: Args.h,v 1.3 2000/08/17 19:53:27 cpqbld Exp $ */
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
/* $XFree86: xc/programs/Xserver/hw/xnest/Args.h,v 1.2 2003/11/16 05:05:20 dawes Exp $ */

#ifndef XNESTARGC_H
#define XNESTARGS_H

extern char *xnestDisplayName;           
extern Bool xnestSynchronize;
extern Bool xnestFullGeneration;
extern int xnestDefaultClass;                   
extern Bool xnestUserDefaultClass;
extern int xnestDefaultDepth;                   
extern Bool xnestUserDefaultDepth;
extern Bool xnestSoftwareScreenSaver;
extern int xnestX;                       
extern int xnestY;                       
extern unsigned int xnestWidth;          
extern unsigned int xnestHeight;         
extern int xnestUserGeometry;
extern int xnestBorderWidth;    
extern Bool xnestUserBorderWidth;
extern char *xnestWindowName;           
extern int xnestNumScreens;
extern Bool xnestDoDirectColormaps;
extern Window xnestParentWindow;

#endif /* XNESTARGS_H */
