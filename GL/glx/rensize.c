/* $XFree86: xc/programs/Xserver/GL/glx/rensize.c,v 1.7 2004/01/28 18:11:53 alanh Exp $ */
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

#include <GL/gl.h>
#include "glxserver.h"
#include "GL/glxproto.h"
#include "unpack.h"
#include "impsize.h"

#define SWAPL(a) \
  (((a & 0xff000000U)>>24) | ((a & 0xff0000U)>>8) | \
   ((a & 0xff00U)<<8) | ((a & 0xffU)<<24))

int __glXCallListsReqSize(GLbyte *pc, Bool swap )
{
    GLsizei n = *(GLsizei *)(pc + 0);
    GLenum type = *(GLenum *)(pc + 4);

    if (swap) {
	n = SWAPL( n );
	type = SWAPL( type );
    }
    return __glCallLists_size( n, type );	/* defined in samplegl lib */
}

int __glXFogivReqSize(GLbyte *pc, Bool swap )
{
    GLenum pname = *(GLenum *)(pc + 0);
    if (swap) {
	pname = SWAPL( pname );
    }
    return 4 * __glFogiv_size( pname );		/* defined in samplegl lib */
}

int __glXFogfvReqSize(GLbyte *pc, Bool swap )
{
    return __glXFogivReqSize( pc, swap );
}

int __glXLightfvReqSize(GLbyte *pc, Bool swap )
{
    GLenum pname = *(GLenum *)(pc + 4);
    if (swap) {
	pname = SWAPL( pname );
    }
    return 4 * __glLightfv_size( pname );	/* defined in samplegl lib */
}

int __glXLightivReqSize(GLbyte *pc, Bool swap )
{
    return __glXLightfvReqSize( pc, swap );
}

int __glXLightModelfvReqSize(GLbyte *pc, Bool swap )
{
    GLenum pname = *(GLenum *)(pc + 0);
    if (swap) {
	pname = SWAPL( pname );
    }
    return 4 * __glLightModelfv_size( pname );	/* defined in samplegl lib */
}

int __glXLightModelivReqSize(GLbyte *pc, Bool swap )
{
    return __glXLightModelfvReqSize( pc, swap );
}

int __glXMaterialfvReqSize(GLbyte *pc, Bool swap )
{
    GLenum pname = *(GLenum *)(pc + 4);
    if (swap) {
	pname = SWAPL( pname );
    }
    return 4 * __glMaterialfv_size( pname );	/* defined in samplegl lib */
}

int __glXMaterialivReqSize(GLbyte *pc, Bool swap )
{
    return __glXMaterialfvReqSize( pc, swap );
}

int __glXTexGendvReqSize(GLbyte *pc, Bool swap )
{
    GLenum pname = *(GLenum *)(pc + 4);
    if (swap) {
	pname = SWAPL( pname );
    }
    return 8 * __glTexGendv_size( pname );	/* defined in samplegl lib */
}

int __glXTexGenfvReqSize(GLbyte *pc, Bool swap )
{
    GLenum pname = *(GLenum *)(pc + 4);
    if (swap) {
	pname = SWAPL( pname );
    }
    return 4 * __glTexGenfv_size( pname );	/* defined in samplegl lib */
}

int __glXTexGenivReqSize(GLbyte *pc, Bool swap )
{
    return __glXTexGenfvReqSize( pc, swap );
}

int __glXTexParameterfvReqSize(GLbyte *pc, Bool swap )
{
    GLenum pname = *(GLenum *)(pc + 4);
    if (swap) {
	pname = SWAPL( pname );
    }
    return 4 * __glTexParameterfv_size( pname ); /* defined in samplegl lib */
}

int __glXTexParameterivReqSize(GLbyte *pc, Bool swap )
{
    return __glXTexParameterfvReqSize( pc, swap );
}

int __glXTexEnvfvReqSize(GLbyte *pc, Bool swap )
{
    GLenum pname = *(GLenum *)(pc + 4);
    if (swap) {
	pname = SWAPL( pname );
    }
    return 4 * __glTexEnvfv_size( pname );	/* defined in samplegl lib */
}

int __glXTexEnvivReqSize(GLbyte *pc, Bool swap )
{
    return __glXTexEnvfvReqSize( pc, swap );
}

static int Map1Size( GLint k, GLint order)
{
    if (order <= 0 || k < 0) return -1;
    return k * order;
}

int __glXMap1dReqSize(GLbyte *pc, Bool swap )
{
    GLenum target;
    GLint order, k;

    target = *(GLenum*) (pc + 16);
    order = *(GLint*) (pc + 20);
    if (swap) {
	target = SWAPL( target );
	order = SWAPL( order );
    }
    k = __glEvalComputeK( target );
    return 8 * Map1Size( k, order );
}

int __glXMap1fReqSize(GLbyte *pc, Bool swap )
{
    GLenum target;
    GLint order, k;

    target = *(GLenum *)(pc + 0);
    order = *(GLint *)(pc + 12);
    if (swap) {
	target = SWAPL( target );
	order = SWAPL( order );
    }
    k = __glEvalComputeK(target);
    return 4 * Map1Size(k, order);
}

static int Map2Size(int k, int majorOrder, int minorOrder)
{
    if (majorOrder <= 0 || minorOrder <= 0 || k < 0) return -1;
    return k * majorOrder * minorOrder;
}

int __glXMap2dReqSize(GLbyte *pc, Bool swap )
{
    GLenum target;
    GLint uorder, vorder, k;

    target = *(GLenum *)(pc + 32);
    uorder = *(GLint *)(pc + 36);
    vorder = *(GLint *)(pc + 40);
    if (swap) {
	target = SWAPL( target );
	uorder = SWAPL( uorder );
	vorder = SWAPL( vorder );
    }
    k = __glEvalComputeK( target );
    return 8 * Map2Size( k, uorder, vorder );
}

int __glXMap2fReqSize(GLbyte *pc, Bool swap )
{
    GLenum target;
    GLint uorder, vorder, k;

    target = *(GLenum *)(pc + 0);
    uorder = *(GLint *)(pc + 12);
    vorder = *(GLint *)(pc + 24);
    if (swap) {
	target = SWAPL( target );
	uorder = SWAPL( uorder );
	vorder = SWAPL( vorder );
    }
    k = __glEvalComputeK( target );
    return 4 * Map2Size( k, uorder, vorder );
}

int __glXPixelMapfvReqSize(GLbyte *pc, Bool swap )
{
    GLint mapsize;
    mapsize = *(GLint *)(pc + 4);
    if (swap) {
	mapsize = SWAPL( mapsize );
    }
    return 4 * mapsize;
}

int __glXPixelMapuivReqSize(GLbyte *pc, Bool swap )
{
    return __glXPixelMapfvReqSize( pc, swap );
}

int __glXPixelMapusvReqSize(GLbyte *pc, Bool swap )
{
    GLint mapsize;
    mapsize = *(GLint *)(pc + 4);
    if (swap) {
	mapsize = SWAPL( mapsize );
    }
    return 2 * mapsize;
}

int __glXImageSize( GLenum format, GLenum type, GLsizei w, GLsizei h,
	       GLint rowLength, GLint skipRows, GLint alignment )
{
    return __glXImage3DSize( format, type, w, h, 1, 0, rowLength,
			     0, skipRows, alignment );
}

/* XXX
 * This should be folded into __glXImageSize().
 */
int __glXImage3DSize( GLenum format, GLenum type, GLsizei w, GLsizei h,
		      GLsizei d, GLint imageHeight, GLint rowLength,
		      GLint skipImages, GLint skipRows, GLint alignment )
{
    GLint bytesPerElement, elementsPerGroup, groupsPerRow;
    GLint groupSize, rowSize, padding, imageSize;

    if (w < 0 || h < 0 || d < 0 ||
	(type == GL_BITMAP &&
	 (format != GL_COLOR_INDEX && format != GL_STENCIL_INDEX))) {
	return -1;
    }
    if (w==0 || h==0 || d == 0) return 0;

    if (type == GL_BITMAP) {
	if (rowLength > 0) {
	    groupsPerRow = rowLength;
	} else {
	    groupsPerRow = w;
	}
	rowSize = (groupsPerRow + 7) >> 3;
	padding = (rowSize % alignment);
	if (padding) {
	    rowSize += alignment - padding;
	}
	return ((h + skipRows) * rowSize);
    } else {
	switch(format) {
	  case GL_COLOR_INDEX:
	  case GL_STENCIL_INDEX:
	  case GL_DEPTH_COMPONENT:
	  case GL_RED:
	  case GL_GREEN:
	  case GL_BLUE:
	  case GL_ALPHA:
	  case GL_LUMINANCE:
	  case GL_INTENSITY:
	    elementsPerGroup = 1;
	    break;
	  case GL_422_EXT:
	  case GL_422_REV_EXT:
	  case GL_422_AVERAGE_EXT:
	  case GL_422_REV_AVERAGE_EXT:
	  case GL_DEPTH_STENCIL_NV:
	  case GL_DEPTH_STENCIL_MESA:
	  case GL_YCBCR_MESA:
	  case GL_LUMINANCE_ALPHA:
	    elementsPerGroup = 2;
	    break;
	  case GL_RGB:
	  case GL_BGR:
	    elementsPerGroup = 3;
	    break;
	  case GL_RGBA:
	  case GL_BGRA:
	  case GL_ABGR_EXT:
	    elementsPerGroup = 4;
	    break;
	  default:
	    return -1;
	}
	switch(type) {
	  case GL_UNSIGNED_BYTE:
	  case GL_BYTE:
	    bytesPerElement = 1;
	    break;
	  case GL_UNSIGNED_BYTE_3_3_2:
	  case GL_UNSIGNED_BYTE_2_3_3_REV:
	    bytesPerElement = 1;	    
	    elementsPerGroup = 1;
	    break;
	  case GL_UNSIGNED_SHORT:
	  case GL_SHORT:
	    bytesPerElement = 2;
	    break;
	  case GL_UNSIGNED_SHORT_5_6_5:
	  case GL_UNSIGNED_SHORT_5_6_5_REV:
	  case GL_UNSIGNED_SHORT_4_4_4_4:
 	  case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	  case GL_UNSIGNED_SHORT_5_5_5_1:
	  case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	  case GL_UNSIGNED_SHORT_8_8_APPLE:
	  case GL_UNSIGNED_SHORT_8_8_REV_APPLE:
	  case GL_UNSIGNED_SHORT_15_1_MESA:
	  case GL_UNSIGNED_SHORT_1_15_REV_MESA:
	    bytesPerElement = 2;
	    elementsPerGroup = 1;
	    break;
	  case GL_INT:
	  case GL_UNSIGNED_INT:
	  case GL_FLOAT:
	    bytesPerElement = 4;
	    break;
	  case GL_UNSIGNED_INT_8_8_8_8:
	  case GL_UNSIGNED_INT_8_8_8_8_REV:
	  case GL_UNSIGNED_INT_10_10_10_2:
	  case GL_UNSIGNED_INT_2_10_10_10_REV:
	  case GL_UNSIGNED_INT_24_8_NV:
	  case GL_UNSIGNED_INT_24_8_MESA:
	  case GL_UNSIGNED_INT_8_24_REV_MESA:
	    bytesPerElement = 4;
	    elementsPerGroup = 1;
	    break;
	  default:
	    return -1;
	}
	groupSize = bytesPerElement * elementsPerGroup;
	if (rowLength > 0) {
	    groupsPerRow = rowLength;
	} else {
	    groupsPerRow = w;
	}
	rowSize = groupsPerRow * groupSize;
	padding = (rowSize % alignment);
	if (padding) {
	    rowSize += alignment - padding;
	}
	if (imageHeight > 0) {
	    imageSize = (imageHeight + skipRows) * rowSize;
	} else {
	    imageSize = (h + skipRows) * rowSize;
	}
	return ((d + skipImages) * imageSize);
    }
}


int __glXDrawPixelsReqSize(GLbyte *pc, Bool swap )
{
    __GLXdispatchDrawPixelsHeader *hdr = (__GLXdispatchDrawPixelsHeader *) pc;
    GLenum format = hdr->format;
    GLenum type = hdr->type;
    GLint w = hdr->width;
    GLint h = hdr->height;
    GLint rowLength = hdr->rowLength;
    GLint skipRows = hdr->skipRows;
    GLint alignment = hdr->alignment;

    if (swap) {
	format = SWAPL( format );
	type = SWAPL( type );
	w = SWAPL( w );
	h = SWAPL( h );
	rowLength = SWAPL( rowLength );
	skipRows = SWAPL( skipRows );
	alignment = SWAPL( alignment );
    }
    return __glXImageSize( format, type, w, h, rowLength, skipRows, alignment );
}

int __glXBitmapReqSize(GLbyte *pc, Bool swap )
{
    __GLXdispatchBitmapHeader *hdr = (__GLXdispatchBitmapHeader *) pc;
    GLint w = hdr->width;
    GLint h = hdr->height;
    GLint rowLength = hdr->rowLength;
    GLint skipRows = hdr->skipRows;
    GLint alignment = hdr->alignment;

    if (swap) {
	w = SWAPL( w );
	h = SWAPL( h );
	rowLength = SWAPL( rowLength );
	skipRows = SWAPL( skipRows );
	alignment = SWAPL( alignment );
    }
    return __glXImageSize( GL_COLOR_INDEX, GL_BITMAP, w, h,
		      rowLength, skipRows, alignment );
}

int __glXTexImage1DReqSize(GLbyte *pc, Bool swap )
{
    __GLXdispatchTexImageHeader *hdr = (__GLXdispatchTexImageHeader *) pc;
    GLenum target = hdr->target;
    GLenum format = hdr->format;
    GLenum type = hdr->type;
    GLint w = hdr->width;
    GLint rowLength = hdr->rowLength;
    GLint skipRows = hdr->skipRows;
    GLint alignment = hdr->alignment;

    if (swap) {
	target = SWAPL( target );
	format = SWAPL( format );
	type = SWAPL( type );
	w = SWAPL( w );
	rowLength = SWAPL( rowLength );
	skipRows = SWAPL( skipRows );
	alignment = SWAPL( alignment );
    }
    if (target == GL_PROXY_TEXTURE_1D) {
	return 0;
    } else if (format == GL_STENCIL_INDEX || format == GL_DEPTH_COMPONENT) {
	return -1;
    }
    return __glXImageSize( format, type, w, 1, rowLength, skipRows, alignment );
}

int __glXTexImage2DReqSize(GLbyte *pc, Bool swap )
{
    __GLXdispatchTexImageHeader *hdr = (__GLXdispatchTexImageHeader *) pc;
    GLenum target = hdr->target;
    GLenum format = hdr->format;
    GLenum type = hdr->type;
    GLint w = hdr->width;
    GLint h = hdr->height;
    GLint rowLength = hdr->rowLength;
    GLint skipRows = hdr->skipRows;
    GLint alignment = hdr->alignment;

    if (swap) {
	target = SWAPL( target );
	format = SWAPL( format );
	type = SWAPL( type );
	w = SWAPL( w );
	h = SWAPL( h );
	rowLength = SWAPL( rowLength );
	skipRows = SWAPL( skipRows );
	alignment = SWAPL( alignment );
    }
    if (target == GL_PROXY_TEXTURE_2D || target == GL_PROXY_TEXTURE_CUBE_MAP_ARB) {
	return 0;
    } else if (format == GL_STENCIL_INDEX || format == GL_DEPTH_COMPONENT) {
	return -1;
    }
    return __glXImageSize( format, type, w, h, rowLength, skipRows, alignment );
}

/* XXX this is used elsewhere - should it be exported from glxserver.h? */
int __glXTypeSize(GLenum enm)
{
  switch(enm) {
    case GL_BYTE:		return sizeof(GLbyte);
    case GL_UNSIGNED_BYTE:	return sizeof(GLubyte);
    case GL_SHORT:		return sizeof(GLshort);
    case GL_UNSIGNED_SHORT:	return sizeof(GLushort);
    case GL_INT:		return sizeof(GLint);
    case GL_UNSIGNED_INT:	return sizeof(GLint);
    case GL_FLOAT:		return sizeof(GLfloat);
    case GL_DOUBLE:		return sizeof(GLdouble);
    default:			return -1;
  }
}

int __glXDrawArraysSize( GLbyte *pc, Bool swap )
{
    __GLXdispatchDrawArraysHeader *hdr = (__GLXdispatchDrawArraysHeader *) pc;
    __GLXdispatchDrawArraysComponentHeader *compHeader;
    GLint numVertexes = hdr->numVertexes;
    GLint numComponents = hdr->numComponents;
    GLint arrayElementSize = 0;
    int i;

    if (swap) {
	numVertexes = SWAPL( numVertexes );
	numComponents = SWAPL( numComponents );
    }

    pc += sizeof(__GLXdispatchDrawArraysHeader);
    compHeader = (__GLXdispatchDrawArraysComponentHeader *) pc;

    for (i=0; i<numComponents; i++) {
	GLenum datatype = compHeader[i].datatype;
	GLint numVals = compHeader[i].numVals;
	GLint component = compHeader[i].component;

	if (swap) {
	    datatype = SWAPL( datatype );
	    numVals = SWAPL( numVals );
	    component = SWAPL( component );
	}

	switch (component) {
	  case GL_VERTEX_ARRAY:
	  case GL_COLOR_ARRAY:
	  case GL_TEXTURE_COORD_ARRAY:
	    break;
	  case GL_SECONDARY_COLOR_ARRAY:
	  case GL_NORMAL_ARRAY:
	    if (numVals != 3) {
		/* bad size */
		return -1;
	    }
	    break;
	  case GL_FOG_COORDINATE_ARRAY:
	  case GL_INDEX_ARRAY:
	    if (numVals != 1) {
		/* bad size */
		return -1;
	    }
	    break;
	  case GL_EDGE_FLAG_ARRAY:
	    if ((numVals != 1) && (datatype != GL_UNSIGNED_BYTE)) {
		/* bad size or bad type */
		return -1;
	    }
	    break;
	  default:
	    /* unknown component type */
	    return -1;
	}

	arrayElementSize += __GLX_PAD(numVals * __glXTypeSize(datatype));

	pc += sizeof(__GLXdispatchDrawArraysComponentHeader);
    }

    return ((numComponents * sizeof(__GLXdispatchDrawArraysComponentHeader)) +
	    (numVertexes * arrayElementSize));
}

int __glXPrioritizeTexturesReqSize(GLbyte *pc, Bool swap )
{
    GLint n = *(GLsizei *)(pc + 0);
    if (swap) n = SWAPL(n);
    return(8*n); /* 4*n for textures, 4*n for priorities */
}

int __glXTexSubImage1DReqSize(GLbyte *pc, Bool swap )
{
    __GLXdispatchTexSubImageHeader *hdr = (__GLXdispatchTexSubImageHeader *) pc;
    GLenum format = hdr->format;
    GLenum type = hdr->type;
    GLint w = hdr->width;
    GLint rowLength = hdr->rowLength;
    GLint skipRows = hdr->skipRows;
    GLint alignment = hdr->alignment;

    if (swap) {
	format = SWAPL( format );
	type = SWAPL( type );
	w = SWAPL( w );
	rowLength = SWAPL( rowLength );
	skipRows = SWAPL( skipRows );
	alignment = SWAPL( alignment );
    }
    return __glXImageSize( format, type, w, 1, rowLength, skipRows, alignment );
}

int __glXTexSubImage2DReqSize(GLbyte *pc, Bool swap )
{
    __GLXdispatchTexSubImageHeader *hdr = (__GLXdispatchTexSubImageHeader *) pc;
    GLenum format = hdr->format;
    GLenum type = hdr->type;
    GLint w = hdr->width;
    GLint h = hdr->height;
    GLint rowLength = hdr->rowLength;
    GLint skipRows = hdr->skipRows;
    GLint alignment = hdr->alignment;

    if (swap) {
	format = SWAPL( format );
	type = SWAPL( type );
	w = SWAPL( w );
	h = SWAPL( h );
	rowLength = SWAPL( rowLength );
	skipRows = SWAPL( skipRows );
	alignment = SWAPL( alignment );
    }
    return __glXImageSize( format, type, w, h, rowLength, skipRows, alignment );
}

int __glXTexImage3DReqSize(GLbyte *pc, Bool swap )
{
    __GLXdispatchTexImage3DHeader *hdr = (__GLXdispatchTexImage3DHeader *) pc;
    GLenum target = hdr->target;
    GLenum format = hdr->format;
    GLenum type = hdr->type;
    GLint w = hdr->width;
    GLint h = hdr->height;
    GLint d = hdr->depth;
    GLint imageHeight = hdr->imageHeight;
    GLint rowLength = hdr->rowLength;
    GLint skipImages = hdr->skipImages;
    GLint skipRows = hdr->skipRows;
    GLint alignment = hdr->alignment;
    GLint nullImage = hdr->nullimage;

    if (swap) {
	target = SWAPL( target );
	format = SWAPL( format );
	type = SWAPL( type );
	w = SWAPL( w );
	h = SWAPL( h );
	d = SWAPL( d );
	imageHeight = SWAPL( imageHeight );
	rowLength = SWAPL( rowLength );
	skipImages = SWAPL( skipImages );
	skipRows = SWAPL( skipRows );
	alignment = SWAPL( alignment );
    }
    if (target == GL_PROXY_TEXTURE_3D || nullImage) {
	return 0;
    } else {
	return __glXImage3DSize( format, type, w, h, d, imageHeight,
				 rowLength, skipImages, skipRows,
				 alignment);
    }
}

int __glXTexSubImage3DReqSize(GLbyte *pc, Bool swap )
{
    __GLXdispatchTexSubImage3DHeader *hdr =
					(__GLXdispatchTexSubImage3DHeader *) pc;
    GLenum target = hdr->target;
    GLenum format = hdr->format;
    GLenum type = hdr->type;
    GLint w = hdr->width;
    GLint h = hdr->height;
    GLint d = hdr->depth;
    GLint imageHeight = hdr->imageHeight;
    GLint rowLength = hdr->rowLength;
    GLint skipImages = hdr->skipImages;
    GLint skipRows = hdr->skipRows;
    GLint alignment = hdr->alignment;

    if (swap) {
	target = SWAPL( target );
	format = SWAPL( format );
	type = SWAPL( type );
	w = SWAPL( w );
	h = SWAPL( h );
	d = SWAPL( d );
	imageHeight = SWAPL( imageHeight );
	rowLength = SWAPL( rowLength );
	skipImages = SWAPL( skipImages );
	skipRows = SWAPL( skipRows );
	alignment = SWAPL( alignment );
    }
    if (target == GL_PROXY_TEXTURE_3D) {
	return 0;
    } else {
	return __glXImage3DSize( format, type, w, h, d, imageHeight,
				 rowLength, skipImages, skipRows,
				 alignment);
    }
}

int __glXConvolutionFilter1DReqSize(GLbyte *pc, Bool swap )
{
    __GLXdispatchConvolutionFilterHeader *hdr =
			(__GLXdispatchConvolutionFilterHeader *) pc;

    GLenum format = hdr->format;
    GLenum type = hdr->type;
    GLint w = hdr->width;
    GLint rowLength = hdr->rowLength;
    GLint alignment = hdr->alignment;

    if (swap) {
	format = SWAPL( format );
	type = SWAPL( type );
	w = SWAPL( w );
	rowLength = SWAPL( rowLength );
	alignment = SWAPL( alignment );
    }

    return __glXImageSize ( format, type, w, 1, rowLength, 0, alignment );
}

int __glXConvolutionFilter2DReqSize(GLbyte *pc, Bool swap )
{
    __GLXdispatchConvolutionFilterHeader *hdr =
			(__GLXdispatchConvolutionFilterHeader *) pc;

    GLenum format = hdr->format;
    GLenum type = hdr->type;
    GLint w = hdr->width;
    GLint h = hdr->height;
    GLint rowLength = hdr->rowLength;
    GLint skipRows = hdr->skipRows;
    GLint alignment = hdr->alignment;

    if (swap) {
	format = SWAPL( format );
	type = SWAPL( type );
	w = SWAPL( w );
	h = SWAPL( h );
	rowLength = SWAPL( rowLength );
	skipRows = SWAPL( skipRows );
	alignment = SWAPL( alignment );
    }

    return __glXImageSize ( format, type, w, h, rowLength, skipRows, alignment );
}

int __glXConvolutionParameterivSize(GLenum pname)
{
    switch (pname) {
      case GL_CONVOLUTION_BORDER_COLOR:
      case GL_CONVOLUTION_FILTER_SCALE:
      case GL_CONVOLUTION_FILTER_BIAS:
	return 4;
      case GL_CONVOLUTION_BORDER_MODE:
	return 1;
      default:
	return -1;
    }
}

int __glXConvolutionParameterfvSize(GLenum pname)
{
    return __glXConvolutionParameterivSize(pname);
}

int __glXConvolutionParameterivReqSize(GLbyte *pc, Bool swap )
{
    GLenum pname = *(GLenum *)(pc + 4);
    if (swap) {
	pname = SWAPL( pname );
    }
    return 4 * __glXConvolutionParameterivSize( pname );
}

int __glXConvolutionParameterfvReqSize(GLbyte *pc, Bool swap )
{
    return __glXConvolutionParameterivReqSize( pc, swap );
}

int __glXSeparableFilter2DReqSize(GLbyte *pc, Bool swap )
{
    __GLXdispatchConvolutionFilterHeader *hdr =
			(__GLXdispatchConvolutionFilterHeader *) pc;

    GLint image1size, image2size;
    GLenum format = hdr->format;
    GLenum type = hdr->type;
    GLint w = hdr->width;
    GLint h = hdr->height;
    GLint rowLength = hdr->rowLength;
    GLint alignment = hdr->alignment;

    if (swap) {
	format = SWAPL( format );
	type = SWAPL( type );
	w = SWAPL( w );
	h = SWAPL( h );
	rowLength = SWAPL( rowLength );
	alignment = SWAPL( alignment );
    }

    /* XXX Should rowLength be used for either or both image? */
    image1size = __glXImageSize ( format, type, w, 1, rowLength, 0, alignment );
    image1size = __GLX_PAD(image1size);
    image2size = __glXImageSize ( format, type, h, 1, rowLength, 0, alignment );
    return image1size + image2size;

}

int __glXColorTableParameterfvSize(GLenum pname)
{
    /* currently, only scale and bias are supported; return RGBA */
    switch(pname) {
    case GL_COLOR_TABLE_SCALE:
    case GL_COLOR_TABLE_BIAS:
	return 4;
    default:
	return 0;
    }
}

int __glXColorTableParameterivSize(GLenum pname)
{
    /* fv and iv are the same in this context */
    return __glXColorTableParameterfvSize(pname);
}

int __glXColorTableReqSize(GLbyte *pc, Bool swap )
{
    __GLXdispatchColorTableHeader *hdr =
			(__GLXdispatchColorTableHeader *) pc;

    GLenum target = hdr->target;
    GLenum format = hdr->format;
    GLenum type = hdr->type;
    GLint w = hdr->width;
    GLint rowLength = hdr->rowLength;
    GLint alignment = hdr->alignment;

    switch (target) {
      case GL_PROXY_TEXTURE_1D:
      case GL_PROXY_TEXTURE_2D:
      case GL_PROXY_TEXTURE_3D:
      case GL_PROXY_COLOR_TABLE:
      case GL_PROXY_POST_CONVOLUTION_COLOR_TABLE:
      case GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE:
      case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
          return 0;
    }

    if (swap) {
	format = SWAPL( format );
	type = SWAPL( type );
	w = SWAPL( w );
	rowLength = SWAPL( rowLength );
	alignment = SWAPL( alignment );
    }

    return __glXImageSize ( format, type, w, 1, rowLength, 0, alignment );
}

int __glXColorSubTableReqSize(GLbyte *pc, Bool swap )
{
    __GLXdispatchColorSubTableHeader *hdr =
			(__GLXdispatchColorSubTableHeader *) pc;

    GLenum format = hdr->format;
    GLenum type = hdr->type;
    GLint count = hdr->count;
    GLint rowLength = hdr->rowLength;
    GLint alignment = hdr->alignment;

    if (swap) {
	format = SWAPL( format );
	type = SWAPL( type );
	count = SWAPL( count );
	rowLength = SWAPL( rowLength );
	alignment = SWAPL( alignment );
    }

    return __glXImageSize ( format, type, count, 1, rowLength, 0, alignment );
}

int __glXColorTableParameterfvReqSize(GLbyte *pc, Bool swap )
{
    GLenum pname = *(GLenum *)(pc + 4);
    if (swap) {
	pname = SWAPL( pname );
    }
    return 4 * __glXColorTableParameterfvSize(pname);
}

int __glXColorTableParameterivReqSize(GLbyte *pc, Bool swap )
{
    /* no difference between fv and iv versions */
    return __glXColorTableParameterfvReqSize(pc, swap);
}

int __glXPointParameterfvARBReqSize(GLbyte *pc, Bool swap )
{
    GLenum pname = *(GLenum *)(pc + 0);
    if (swap) {
	pname = SWAPL( pname );
    }
    return 4 * __glPointParameterfvARB_size( pname );
}

int __glXPointParameterivReqSize(GLbyte *pc, Bool swap )
{
    /* no difference between fv and iv versions */
    return __glXPointParameterfvARBReqSize(pc, swap);
}
