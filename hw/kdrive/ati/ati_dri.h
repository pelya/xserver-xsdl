/*
 * Copyright © 2003 Eric Anholt
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Eric Anholt not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Eric Anholt makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * ERIC ANHOLT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ERIC ANHOLT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _ATI_DRI_H_
#define _ATI_DRI_H_

typedef struct {
	/* DRI screen private data */
	int		deviceID;	/* PCI device ID */
	int		width;		/* Width in pixels of display */
	int		height;		/* Height in scanlines of display */
	int		depth;		/* Depth of display (8, 15, 16, 24) */
	int		bpp;		/* Bit depth of display (8, 16, 24, 32) */

	int		IsPCI;		/* Current card is a PCI card */
	int		AGPMode;

	int		frontOffset;	/* Start of front buffer */
	int		frontPitch;
	int		backOffset;	/* Start of shared back buffer */
	int		backPitch;
	int		depthOffset;	/* Start of shared depth buffer */
	int		depthPitch;
	int		spanOffset;	/* Start of scratch spanline */
	int		textureOffset;	/* Start of texture data in frame buffer */
	int		textureSize;
	int		log2TexGran;

	/* MMIO register data */
	drmHandle	registerHandle;
	drmSize		registerSize;

	/* CCE AGP Texture data */
	drmHandle	gartTexHandle;
	drmSize		gartTexMapSize;
	int		log2AGPTexGran;
	int		gartTexOffset;
	unsigned int sarea_priv_offset;
} R128DRIRec, *R128DRIPtr;

typedef struct {
	/* DRI screen private data */
	int		deviceID;	/* PCI device ID */
	int		width;		/* Width in pixels of display */
	int		height;		/* Height in scanlines of display */
	int		depth;		/* Depth of display (8, 15, 16, 24) */
	int		bpp;		/* Bit depth of display (8, 16, 24, 32) */

	int		IsPCI;		/* Current card is a PCI card */
	int		AGPMode;

	int		frontOffset;	/* Start of front buffer */
	int		frontPitch;
	int		backOffset;	/* Start of shared back buffer */
	int		backPitch;
	int		depthOffset;	/* Start of shared depth buffer */
	int		depthPitch;
	int		textureOffset;	/* Start of texture data in frame buffer */
	int		textureSize;
	int		log2TexGran;

	/* MMIO register data */
	drmHandle	registerHandle;
	drmSize		registerSize;

	/* CP in-memory status information */
	drmHandle	statusHandle;
	drmSize		statusSize;

	/* CP GART Texture data */
	drmHandle	gartTexHandle;
	drmSize		gartTexMapSize;
	int		log2GARTTexGran;
	int		gartTexOffset;
	unsigned int	sarea_priv_offset;
} RADEONDRIRec, *RADEONDRIPtr;

#endif /* _ATI_DRI_H_ */
