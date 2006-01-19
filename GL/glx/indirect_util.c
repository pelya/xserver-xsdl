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

#include <X11/Xmd.h>
#include <GL/gl.h>
#include <GL/glxproto.h>
#ifdef __linux__
#include <byteswap.h>
#else
#include <sys/endian.h>
#define bswap_16 bswap16
#define bswap_32 bswap32
#define bswap_64 bswap64
#endif
#include <inttypes.h>
#include "indirect_size.h"
#include "indirect_size_get.h"
#include "indirect_dispatch.h"
#include "glxserver.h"
#include "singlesize.h"
#include "glapitable.h"
#include "glapi.h"
#include "glthread.h"
#include "dispatch.h"
#include "glxext.h"
#include "indirect_util.h"


#define __GLX_PAD(a) (((a)+3)&~3)

extern xGLXSingleReply __glXReply;


GLint
__glGetBooleanv_variable_size( GLenum e )
{
    if ( e == GL_COMPRESSED_TEXTURE_FORMATS ) {
	GLint temp;

	CALL_GetIntegerv( GET_DISPATCH(),
			  (GL_NUM_COMPRESSED_TEXTURE_FORMATS, & temp) );
	return temp;
    }
    else {
	return 0;
    }
}


/**
 * Get a properly aligned buffer to hold reply data.
 *
 * \warning
 * This function assumes that \c local_buffer is already properly aligned.
 * It also assumes that \c alignment is a power of two.
 */
void *
__glXGetAnswerBuffer( __GLXclientState * cl, size_t required_size,
    void * local_buffer, size_t local_size, unsigned alignment )
{
    void * buffer = local_buffer;
    const unsigned mask = alignment - 1;

    if ( local_size < required_size ) {
        const size_t worst_case_size = required_size + alignment;
        intptr_t  temp_buf;

        if ( cl->returnBufSize < worst_case_size ) {
	    void * temp = xrealloc( cl->returnBuf, worst_case_size );
	    
	    if ( temp == NULL ) {
	        return NULL;
	    }
	    
	    cl->returnBuf = temp;
	    cl->returnBufSize = worst_case_size;
	}
	
	temp_buf = (intptr_t) cl->returnBuf;
	temp_buf = (temp_buf + mask) & ~mask;
	buffer = (void *) temp_buf;
    }

    return buffer;
}


/**
 * Send a GLX reply to the client.
 *
 * Technically speaking, there are several different ways to encode a GLX
 * reply.  The primary difference is whether or not certain fields (e.g.,
 * retval, size, and "pad3") are set.  This function gets around that by
 * always setting all of the fields to "reasonable" values.  This does no
 * harm to clients, but it does make the server-side code much more compact.
 */
void
__glXSendReply( ClientPtr client, const void * data, size_t elements,
    size_t element_size, GLboolean always_array, CARD32 retval )
{
    size_t reply_ints = 0;

    if ( __glXErrorOccured() ) {
        elements = 0;
    }
    else if ( (elements > 1) || always_array ) {
        reply_ints = ((elements * element_size) + 3) >> 2;
    }

    __glXReply.length =         reply_ints;
    __glXReply.type =           X_Reply;
    __glXReply.sequenceNumber = client->sequence;
    __glXReply.size =           elements;
    __glXReply.retval =         retval;


    /* It is faster on almost always every architecture to just copy the 8
     * bytes, even when not necessary, than check to see of the value of
     * elements requires it.  Copying the data when not needed will do no
     * harm.
     */

    (void) memcpy( & __glXReply.pad3, data, 8 );
    WriteToClient( client, sz_xGLXSingleReply, (char *) & __glXReply );

    if ( reply_ints != 0 ) {
        WriteToClient( client, reply_ints * 4, (char *) data );
    }
}


/**
 * Send a GLX reply to the client.
 *
 * Technically speaking, there are several different ways to encode a GLX
 * reply.  The primary difference is whether or not certain fields (e.g.,
 * retval, size, and "pad3") are set.  This function gets around that by
 * always setting all of the fields to "reasonable" values.  This does no
 * harm to clients, but it does make the server-side code much more compact.
 *
 * \warning
 * This function assumes that values stored in \c data will be byte-swapped
 * by the caller if necessary.
 */
void
__glXSendReplySwap( ClientPtr client, const void * data, size_t elements,
    size_t element_size, GLboolean always_array, CARD32 retval )
{
    size_t reply_ints = 0;

    if ( __glXErrorOccured() ) {
        elements = 0;
    }
    else if ( (elements > 1) || always_array ) {
        reply_ints = ((elements * element_size) + 3) >> 2;
    }

    __glXReply.length =         bswap_32( reply_ints );
    __glXReply.type =           bswap_32( X_Reply );
    __glXReply.sequenceNumber = bswap_32( client->sequence );
    __glXReply.size =           bswap_32( elements );
    __glXReply.retval =         bswap_32( retval );


    /* It is faster on almost always every architecture to just copy the 8
     * bytes, even when not necessary, than check to see of the value of
     * elements requires it.  Copying the data when not needed will do no
     * harm.
     */

    (void) memcpy( & __glXReply.pad3, data, 8 );
    WriteToClient( client, sz_xGLXSingleReply, (char *) & __glXReply );

    if ( reply_ints != 0 ) {
        WriteToClient( client, reply_ints * 4, (char *) data );
    }
}
