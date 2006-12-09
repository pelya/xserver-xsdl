/* DO NOT EDIT - This file generated automatically by glX_proto_size.py (from Mesa) script */

/*
 * (C) Copyright IBM Corporation 2005
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <GL/gl.h>
#include "glxserver.h"
#include "indirect_size.h"
#include "indirect_reqsize.h"

#if defined(__linux__) || defined (__GLIBC__) || defined(__GNU__)
#  include <byteswap.h>
#  define SWAP_32(v)  do { (v) = bswap_32(v); } while(0)
#else
#  define SWAP_32(v)  do { char tmp; swapl(&v, tmp); } while(0)
#endif

#define __GLX_PAD(x)  (((x) + 3) & ~3)

#if defined(__CYGWIN__) || defined(__MINGW32__)
#  undef HAVE_ALIAS
#endif
#ifdef HAVE_ALIAS
#  define ALIAS2(from,to) \
    GLint __glX ## from ## ReqSize( const GLbyte * pc, Bool swap ) \
        __attribute__ ((alias( # to )));
#  define ALIAS(from,to) ALIAS2( from, __glX ## to ## ReqSize )
#else
#  define ALIAS(from,to) \
    GLint __glX ## from ## ReqSize( const GLbyte * pc, Bool swap ) \
    { return __glX ## to ## ReqSize( pc, swap ); }
#endif


int
__glXCallListsReqSize( const GLbyte * pc, Bool swap )
{
    GLsizei n          = *(GLsizei *)(pc + 0);
    GLenum type        = * (GLenum *)(pc + 4);
    GLsizei compsize;

    if (swap) {
        SWAP_32( n );
        SWAP_32( type );
    }

    compsize = __glCallLists_size(type);
    return __GLX_PAD((compsize * n));
}

int
__glXBitmapReqSize( const GLbyte * pc, Bool swap )
{
    GLint row_length   = *  (GLint *)(pc +  4);
    GLint image_height = 0;
    GLint skip_images  = 0;
    GLint skip_rows    = *  (GLint *)(pc +  8);
    GLint alignment    = *  (GLint *)(pc + 16);
    GLsizei width      = *(GLsizei *)(pc + 20);
    GLsizei height     = *(GLsizei *)(pc + 24);

    if (swap) {
        SWAP_32( row_length );
        SWAP_32( skip_rows );
        SWAP_32( alignment );
        SWAP_32( width );
        SWAP_32( height );
    }

    return __glXImageSize(GL_COLOR_INDEX, GL_BITMAP, 0, width, height, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXFogfvReqSize( const GLbyte * pc, Bool swap )
{
    GLenum pname       = * (GLenum *)(pc + 0);
    GLsizei compsize;

    if (swap) {
        SWAP_32( pname );
    }

    compsize = __glFogfv_size(pname);
    return __GLX_PAD((compsize * 4));
}

int
__glXLightfvReqSize( const GLbyte * pc, Bool swap )
{
    GLenum pname       = * (GLenum *)(pc + 4);
    GLsizei compsize;

    if (swap) {
        SWAP_32( pname );
    }

    compsize = __glLightfv_size(pname);
    return __GLX_PAD((compsize * 4));
}

int
__glXLightModelfvReqSize( const GLbyte * pc, Bool swap )
{
    GLenum pname       = * (GLenum *)(pc + 0);
    GLsizei compsize;

    if (swap) {
        SWAP_32( pname );
    }

    compsize = __glLightModelfv_size(pname);
    return __GLX_PAD((compsize * 4));
}

int
__glXMaterialfvReqSize( const GLbyte * pc, Bool swap )
{
    GLenum pname       = * (GLenum *)(pc + 4);
    GLsizei compsize;

    if (swap) {
        SWAP_32( pname );
    }

    compsize = __glMaterialfv_size(pname);
    return __GLX_PAD((compsize * 4));
}

int
__glXPolygonStippleReqSize( const GLbyte * pc, Bool swap )
{
    GLint row_length   = *  (GLint *)(pc +  4);
    GLint image_height = 0;
    GLint skip_images  = 0;
    GLint skip_rows    = *  (GLint *)(pc +  8);
    GLint alignment    = *  (GLint *)(pc + 16);

    if (swap) {
        SWAP_32( row_length );
        SWAP_32( skip_rows );
        SWAP_32( alignment );
    }

    return __glXImageSize(GL_COLOR_INDEX, GL_BITMAP, 0, 32, 32, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXTexParameterfvReqSize( const GLbyte * pc, Bool swap )
{
    GLenum pname       = * (GLenum *)(pc + 4);
    GLsizei compsize;

    if (swap) {
        SWAP_32( pname );
    }

    compsize = __glTexParameterfv_size(pname);
    return __GLX_PAD((compsize * 4));
}

int
__glXTexImage1DReqSize( const GLbyte * pc, Bool swap )
{
    GLint row_length   = *  (GLint *)(pc +  4);
    GLint image_height = 0;
    GLint skip_images  = 0;
    GLint skip_rows    = *  (GLint *)(pc +  8);
    GLint alignment    = *  (GLint *)(pc + 16);
    GLenum target      = * (GLenum *)(pc + 20);
    GLsizei width      = *(GLsizei *)(pc + 32);
    GLenum format      = * (GLenum *)(pc + 44);
    GLenum type        = * (GLenum *)(pc + 48);

    if (swap) {
        SWAP_32( row_length );
        SWAP_32( skip_rows );
        SWAP_32( alignment );
        SWAP_32( target );
        SWAP_32( width );
        SWAP_32( format );
        SWAP_32( type );
    }

    return __glXImageSize(format, type, target, width, 1, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXTexImage2DReqSize( const GLbyte * pc, Bool swap )
{
    GLint row_length   = *  (GLint *)(pc +  4);
    GLint image_height = 0;
    GLint skip_images  = 0;
    GLint skip_rows    = *  (GLint *)(pc +  8);
    GLint alignment    = *  (GLint *)(pc + 16);
    GLenum target      = * (GLenum *)(pc + 20);
    GLsizei width      = *(GLsizei *)(pc + 32);
    GLsizei height     = *(GLsizei *)(pc + 36);
    GLenum format      = * (GLenum *)(pc + 44);
    GLenum type        = * (GLenum *)(pc + 48);

    if (swap) {
        SWAP_32( row_length );
        SWAP_32( skip_rows );
        SWAP_32( alignment );
        SWAP_32( target );
        SWAP_32( width );
        SWAP_32( height );
        SWAP_32( format );
        SWAP_32( type );
    }

    return __glXImageSize(format, type, target, width, height, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXTexEnvfvReqSize( const GLbyte * pc, Bool swap )
{
    GLenum pname       = * (GLenum *)(pc + 4);
    GLsizei compsize;

    if (swap) {
        SWAP_32( pname );
    }

    compsize = __glTexEnvfv_size(pname);
    return __GLX_PAD((compsize * 4));
}

int
__glXTexGendvReqSize( const GLbyte * pc, Bool swap )
{
    GLenum pname       = * (GLenum *)(pc + 4);
    GLsizei compsize;

    if (swap) {
        SWAP_32( pname );
    }

    compsize = __glTexGendv_size(pname);
    return __GLX_PAD((compsize * 8));
}

int
__glXTexGenfvReqSize( const GLbyte * pc, Bool swap )
{
    GLenum pname       = * (GLenum *)(pc + 4);
    GLsizei compsize;

    if (swap) {
        SWAP_32( pname );
    }

    compsize = __glTexGenfv_size(pname);
    return __GLX_PAD((compsize * 4));
}

int
__glXPixelMapfvReqSize( const GLbyte * pc, Bool swap )
{
    GLsizei mapsize    = *(GLsizei *)(pc + 4);

    if (swap) {
        SWAP_32( mapsize );
    }

    return __GLX_PAD((mapsize * 4));
}

int
__glXPixelMapusvReqSize( const GLbyte * pc, Bool swap )
{
    GLsizei mapsize    = *(GLsizei *)(pc + 4);

    if (swap) {
        SWAP_32( mapsize );
    }

    return __GLX_PAD((mapsize * 2));
}

int
__glXDrawPixelsReqSize( const GLbyte * pc, Bool swap )
{
    GLint row_length   = *  (GLint *)(pc +  4);
    GLint image_height = 0;
    GLint skip_images  = 0;
    GLint skip_rows    = *  (GLint *)(pc +  8);
    GLint alignment    = *  (GLint *)(pc + 16);
    GLsizei width      = *(GLsizei *)(pc + 20);
    GLsizei height     = *(GLsizei *)(pc + 24);
    GLenum format      = * (GLenum *)(pc + 28);
    GLenum type        = * (GLenum *)(pc + 32);

    if (swap) {
        SWAP_32( row_length );
        SWAP_32( skip_rows );
        SWAP_32( alignment );
        SWAP_32( width );
        SWAP_32( height );
        SWAP_32( format );
        SWAP_32( type );
    }

    return __glXImageSize(format, type, 0, width, height, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXPrioritizeTexturesReqSize( const GLbyte * pc, Bool swap )
{
    GLsizei n          = *(GLsizei *)(pc + 0);

    if (swap) {
        SWAP_32( n );
    }

    return __GLX_PAD((n * 4) + (n * 4));
}

int
__glXTexSubImage1DReqSize( const GLbyte * pc, Bool swap )
{
    GLint row_length   = *  (GLint *)(pc +  4);
    GLint image_height = 0;
    GLint skip_images  = 0;
    GLint skip_rows    = *  (GLint *)(pc +  8);
    GLint alignment    = *  (GLint *)(pc + 16);
    GLenum target      = * (GLenum *)(pc + 20);
    GLsizei width      = *(GLsizei *)(pc + 36);
    GLenum format      = * (GLenum *)(pc + 44);
    GLenum type        = * (GLenum *)(pc + 48);

    if (swap) {
        SWAP_32( row_length );
        SWAP_32( skip_rows );
        SWAP_32( alignment );
        SWAP_32( target );
        SWAP_32( width );
        SWAP_32( format );
        SWAP_32( type );
    }

    return __glXImageSize(format, type, target, width, 1, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXTexSubImage2DReqSize( const GLbyte * pc, Bool swap )
{
    GLint row_length   = *  (GLint *)(pc +  4);
    GLint image_height = 0;
    GLint skip_images  = 0;
    GLint skip_rows    = *  (GLint *)(pc +  8);
    GLint alignment    = *  (GLint *)(pc + 16);
    GLenum target      = * (GLenum *)(pc + 20);
    GLsizei width      = *(GLsizei *)(pc + 36);
    GLsizei height     = *(GLsizei *)(pc + 40);
    GLenum format      = * (GLenum *)(pc + 44);
    GLenum type        = * (GLenum *)(pc + 48);

    if (swap) {
        SWAP_32( row_length );
        SWAP_32( skip_rows );
        SWAP_32( alignment );
        SWAP_32( target );
        SWAP_32( width );
        SWAP_32( height );
        SWAP_32( format );
        SWAP_32( type );
    }

    return __glXImageSize(format, type, target, width, height, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXColorTableReqSize( const GLbyte * pc, Bool swap )
{
    GLint row_length   = *  (GLint *)(pc +  4);
    GLint image_height = 0;
    GLint skip_images  = 0;
    GLint skip_rows    = *  (GLint *)(pc +  8);
    GLint alignment    = *  (GLint *)(pc + 16);
    GLenum target      = * (GLenum *)(pc + 20);
    GLsizei width      = *(GLsizei *)(pc + 28);
    GLenum format      = * (GLenum *)(pc + 32);
    GLenum type        = * (GLenum *)(pc + 36);

    if (swap) {
        SWAP_32( row_length );
        SWAP_32( skip_rows );
        SWAP_32( alignment );
        SWAP_32( target );
        SWAP_32( width );
        SWAP_32( format );
        SWAP_32( type );
    }

    return __glXImageSize(format, type, target, width, 1, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXColorTableParameterfvReqSize( const GLbyte * pc, Bool swap )
{
    GLenum pname       = * (GLenum *)(pc + 4);
    GLsizei compsize;

    if (swap) {
        SWAP_32( pname );
    }

    compsize = __glColorTableParameterfv_size(pname);
    return __GLX_PAD((compsize * 4));
}

int
__glXColorSubTableReqSize( const GLbyte * pc, Bool swap )
{
    GLint row_length   = *  (GLint *)(pc +  4);
    GLint image_height = 0;
    GLint skip_images  = 0;
    GLint skip_rows    = *  (GLint *)(pc +  8);
    GLint alignment    = *  (GLint *)(pc + 16);
    GLenum target      = * (GLenum *)(pc + 20);
    GLsizei count      = *(GLsizei *)(pc + 28);
    GLenum format      = * (GLenum *)(pc + 32);
    GLenum type        = * (GLenum *)(pc + 36);

    if (swap) {
        SWAP_32( row_length );
        SWAP_32( skip_rows );
        SWAP_32( alignment );
        SWAP_32( target );
        SWAP_32( count );
        SWAP_32( format );
        SWAP_32( type );
    }

    return __glXImageSize(format, type, target, count, 1, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXConvolutionFilter1DReqSize( const GLbyte * pc, Bool swap )
{
    GLint row_length   = *  (GLint *)(pc +  4);
    GLint image_height = 0;
    GLint skip_images  = 0;
    GLint skip_rows    = *  (GLint *)(pc +  8);
    GLint alignment    = *  (GLint *)(pc + 16);
    GLenum target      = * (GLenum *)(pc + 20);
    GLsizei width      = *(GLsizei *)(pc + 28);
    GLenum format      = * (GLenum *)(pc + 36);
    GLenum type        = * (GLenum *)(pc + 40);

    if (swap) {
        SWAP_32( row_length );
        SWAP_32( skip_rows );
        SWAP_32( alignment );
        SWAP_32( target );
        SWAP_32( width );
        SWAP_32( format );
        SWAP_32( type );
    }

    return __glXImageSize(format, type, target, width, 1, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXConvolutionFilter2DReqSize( const GLbyte * pc, Bool swap )
{
    GLint row_length   = *  (GLint *)(pc +  4);
    GLint image_height = 0;
    GLint skip_images  = 0;
    GLint skip_rows    = *  (GLint *)(pc +  8);
    GLint alignment    = *  (GLint *)(pc + 16);
    GLenum target      = * (GLenum *)(pc + 20);
    GLsizei width      = *(GLsizei *)(pc + 28);
    GLsizei height     = *(GLsizei *)(pc + 32);
    GLenum format      = * (GLenum *)(pc + 36);
    GLenum type        = * (GLenum *)(pc + 40);

    if (swap) {
        SWAP_32( row_length );
        SWAP_32( skip_rows );
        SWAP_32( alignment );
        SWAP_32( target );
        SWAP_32( width );
        SWAP_32( height );
        SWAP_32( format );
        SWAP_32( type );
    }

    return __glXImageSize(format, type, target, width, height, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXConvolutionParameterfvReqSize( const GLbyte * pc, Bool swap )
{
    GLenum pname       = * (GLenum *)(pc + 4);
    GLsizei compsize;

    if (swap) {
        SWAP_32( pname );
    }

    compsize = __glConvolutionParameterfv_size(pname);
    return __GLX_PAD((compsize * 4));
}

int
__glXTexImage3DReqSize( const GLbyte * pc, Bool swap )
{
    GLint row_length   = *  (GLint *)(pc +  4);
    GLint image_height = *  (GLint *)(pc +  8);
    GLint skip_rows    = *  (GLint *)(pc + 16);
    GLint skip_images  = *  (GLint *)(pc + 20);
    GLint alignment    = *  (GLint *)(pc + 32);
    GLenum target      = * (GLenum *)(pc + 36);
    GLsizei width      = *(GLsizei *)(pc + 48);
    GLsizei height     = *(GLsizei *)(pc + 52);
    GLsizei depth      = *(GLsizei *)(pc + 56);
    GLenum format      = * (GLenum *)(pc + 68);
    GLenum type        = * (GLenum *)(pc + 72);

    if (swap) {
        SWAP_32( row_length );
        SWAP_32( image_height );
        SWAP_32( skip_rows );
        SWAP_32( skip_images );
        SWAP_32( alignment );
        SWAP_32( target );
        SWAP_32( width );
        SWAP_32( height );
        SWAP_32( depth );
        SWAP_32( format );
        SWAP_32( type );
    }

    return __glXImageSize(format, type, target, width, height, depth,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXTexSubImage3DReqSize( const GLbyte * pc, Bool swap )
{
    GLint row_length   = *  (GLint *)(pc +  4);
    GLint image_height = *  (GLint *)(pc +  8);
    GLint skip_rows    = *  (GLint *)(pc + 16);
    GLint skip_images  = *  (GLint *)(pc + 20);
    GLint alignment    = *  (GLint *)(pc + 32);
    GLenum target      = * (GLenum *)(pc + 36);
    GLsizei width      = *(GLsizei *)(pc + 60);
    GLsizei height     = *(GLsizei *)(pc + 64);
    GLsizei depth      = *(GLsizei *)(pc + 68);
    GLenum format      = * (GLenum *)(pc + 76);
    GLenum type        = * (GLenum *)(pc + 80);

    if (swap) {
        SWAP_32( row_length );
        SWAP_32( image_height );
        SWAP_32( skip_rows );
        SWAP_32( skip_images );
        SWAP_32( alignment );
        SWAP_32( target );
        SWAP_32( width );
        SWAP_32( height );
        SWAP_32( depth );
        SWAP_32( format );
        SWAP_32( type );
    }

    return __glXImageSize(format, type, target, width, height, depth,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXDrawBuffersARBReqSize( const GLbyte * pc, Bool swap )
{
    GLsizei n          = *(GLsizei *)(pc + 0);

    if (swap) {
        SWAP_32( n );
    }

    return __GLX_PAD((n * 4));
}

int
__glXPointParameterfvEXTReqSize( const GLbyte * pc, Bool swap )
{
    GLenum pname       = * (GLenum *)(pc + 0);
    GLsizei compsize;

    if (swap) {
        SWAP_32( pname );
    }

    compsize = __glPointParameterfvEXT_size(pname);
    return __GLX_PAD((compsize * 4));
}

int
__glXCompressedTexImage3DARBReqSize( const GLbyte * pc, Bool swap )
{
    GLsizei imageSize  = *(GLsizei *)(pc + 28);

    if (swap) {
        SWAP_32( imageSize );
    }

    return __GLX_PAD(imageSize);
}

int
__glXCompressedTexImage2DARBReqSize( const GLbyte * pc, Bool swap )
{
    GLsizei imageSize  = *(GLsizei *)(pc + 24);

    if (swap) {
        SWAP_32( imageSize );
    }

    return __GLX_PAD(imageSize);
}

int
__glXCompressedTexImage1DARBReqSize( const GLbyte * pc, Bool swap )
{
    GLsizei imageSize  = *(GLsizei *)(pc + 20);

    if (swap) {
        SWAP_32( imageSize );
    }

    return __GLX_PAD(imageSize);
}

int
__glXCompressedTexSubImage3DARBReqSize( const GLbyte * pc, Bool swap )
{
    GLsizei imageSize  = *(GLsizei *)(pc + 36);

    if (swap) {
        SWAP_32( imageSize );
    }

    return __GLX_PAD(imageSize);
}

int
__glXLoadProgramNVReqSize( const GLbyte * pc, Bool swap )
{
    GLsizei len        = *(GLsizei *)(pc + 8);

    if (swap) {
        SWAP_32( len );
    }

    return __GLX_PAD(len);
}

int
__glXProgramParameters4dvNVReqSize( const GLbyte * pc, Bool swap )
{
    GLuint num         = * (GLuint *)(pc + 8);

    if (swap) {
        SWAP_32( num );
    }

    return __GLX_PAD((num * 32));
}

int
__glXProgramParameters4fvNVReqSize( const GLbyte * pc, Bool swap )
{
    GLuint num         = * (GLuint *)(pc + 8);

    if (swap) {
        SWAP_32( num );
    }

    return __GLX_PAD((num * 16));
}

int
__glXVertexAttribs1dvNVReqSize( const GLbyte * pc, Bool swap )
{
    GLsizei n          = *(GLsizei *)(pc + 4);

    if (swap) {
        SWAP_32( n );
    }

    return __GLX_PAD((n * 8));
}

int
__glXVertexAttribs2dvNVReqSize( const GLbyte * pc, Bool swap )
{
    GLsizei n          = *(GLsizei *)(pc + 4);

    if (swap) {
        SWAP_32( n );
    }

    return __GLX_PAD((n * 16));
}

int
__glXVertexAttribs3dvNVReqSize( const GLbyte * pc, Bool swap )
{
    GLsizei n          = *(GLsizei *)(pc + 4);

    if (swap) {
        SWAP_32( n );
    }

    return __GLX_PAD((n * 24));
}

int
__glXVertexAttribs3fvNVReqSize( const GLbyte * pc, Bool swap )
{
    GLsizei n          = *(GLsizei *)(pc + 4);

    if (swap) {
        SWAP_32( n );
    }

    return __GLX_PAD((n * 12));
}

int
__glXVertexAttribs3svNVReqSize( const GLbyte * pc, Bool swap )
{
    GLsizei n          = *(GLsizei *)(pc + 4);

    if (swap) {
        SWAP_32( n );
    }

    return __GLX_PAD((n * 6));
}

int
__glXVertexAttribs4dvNVReqSize( const GLbyte * pc, Bool swap )
{
    GLsizei n          = *(GLsizei *)(pc + 4);

    if (swap) {
        SWAP_32( n );
    }

    return __GLX_PAD((n * 32));
}

int
__glXProgramNamedParameter4fvNVReqSize( const GLbyte * pc, Bool swap )
{
    GLsizei len        = *(GLsizei *)(pc + 4);

    if (swap) {
        SWAP_32( len );
    }

    return __GLX_PAD(len);
}

ALIAS( Fogiv, Fogfv )
ALIAS( Lightiv, Lightfv )
ALIAS( LightModeliv, LightModelfv )
ALIAS( Materialiv, Materialfv )
ALIAS( TexParameteriv, TexParameterfv )
ALIAS( TexEnviv, TexEnvfv )
ALIAS( TexGeniv, TexGenfv )
ALIAS( PixelMapuiv, PixelMapfv )
ALIAS( ColorTableParameteriv, ColorTableParameterfv )
ALIAS( ConvolutionParameteriv, ConvolutionParameterfv )
ALIAS( CompressedTexSubImage2DARB, CompressedTexImage3DARB )
ALIAS( CompressedTexSubImage1DARB, CompressedTexImage1DARB )
ALIAS( RequestResidentProgramsNV, DrawBuffersARB )
ALIAS( VertexAttribs1fvNV, PixelMapfv )
ALIAS( VertexAttribs1svNV, PixelMapusv )
ALIAS( VertexAttribs2fvNV, VertexAttribs1dvNV )
ALIAS( VertexAttribs2svNV, PixelMapfv )
ALIAS( VertexAttribs4fvNV, VertexAttribs2dvNV )
ALIAS( VertexAttribs4svNV, VertexAttribs1dvNV )
ALIAS( VertexAttribs4ubvNV, PixelMapfv )
ALIAS( PointParameterivNV, PointParameterfvEXT )
ALIAS( ProgramStringARB, LoadProgramNV )
ALIAS( ProgramNamedParameter4dvNV, CompressedTexSubImage3DARB )
ALIAS( DeleteRenderbuffersEXT, DrawBuffersARB )
ALIAS( DeleteFramebuffersEXT, DrawBuffersARB )
