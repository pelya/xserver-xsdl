/*
 * (C) Copyright IBM Corporation 2006, 2007
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
 * THE COPYRIGHT HOLDERS, THE AUTHORS, AND/OR THEIR SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file glxbyteorder.h
 * Platform glue for handling byte-ordering issues in GLX protocol.
 * 
 * \author Ian Romanick <idr@us.ibm.com>
 */
#if !defined(__GLXBYTEORDER_H__)
#define __GLXBYTEORDER_H__

#if defined(__linux__) || defined(__GLIBC__) || defined(__GNU__)
#include <byteswap.h>
#elif defined(__OpenBSD__)
#include <sys/endian.h>
#define bswap_16 __swap16
#define bswap_32 __swap32
#define bswap_64 __swap64
#else
#include <sys/endian.h>
#define bswap_16 bswap16
#define bswap_32 bswap32
#define bswap_64 bswap64
#endif

#endif /* !defined(__GLXBYTEORDER_H__) */
