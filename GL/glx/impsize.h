/* $XFree86: xc/programs/Xserver/GL/glx/impsize.h,v 1.5 2004/01/28 18:11:50 alanh Exp $ */
#ifndef _impsize_h_
#define _impsize_h_

/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
**
*/

/*
** These are defined in libsampleGL.a. They are not technically part of
** the defined interface between libdixGL.a and libsampleGL.a (that interface
** being the functions in the __glXScreenInfo structure, plus the OpenGL API
** itself), but we thought it was better to call these routines than to
** replicate the code in here.
*/
extern int __glCallLists_size(GLsizei n, GLenum type);
extern int __glColorTableParameterfv_size(GLenum pname);
extern int __glColorTableParameteriv_size(GLenum pname);
extern int __glConvolutionParameterfv_size(GLenum pname);
extern int __glConvolutionParameteriv_size(GLenum pname);
extern int __glDrawPixels_size(GLenum format, GLenum type, GLsizei w,GLsizei h);
extern int __glFogfv_size(GLenum pname);
extern int __glFogiv_size(GLenum pname);
extern int __glLightModelfv_size(GLenum pname);
extern int __glLightModeliv_size(GLenum pname);
extern int __glLightfv_size(GLenum pname);
extern int __glLightiv_size(GLenum pname);
extern int __glMaterialfv_size(GLenum pname);
extern int __glMaterialiv_size(GLenum pname);
extern int __glTexEnvfv_size(GLenum e);
extern int __glTexEnviv_size(GLenum e);
extern int __glTexGendv_size(GLenum e);
extern int __glTexGenfv_size(GLenum e);
extern int __glTexGeniv_size(GLenum pname);
extern int __glTexParameterfv_size(GLenum e);
extern int __glTexParameteriv_size(GLenum e);
extern int __glEvalComputeK(GLenum target);

extern int __glPointParameterfvARB_size(GLenum e);
extern int __glPointParameteriv_size(GLenum e);

#endif /* _impsize_h_ */
