/* $Xorg: attributes.h,v 1.4 2001/03/14 18:42:44 pookie Exp $ */
/*
(c) Copyright 1996 Hewlett-Packard Company
(c) Copyright 1996 International Business Machines Corp.
(c) Copyright 1996 Sun Microsystems, Inc.
(c) Copyright 1996 Novell, Inc.
(c) Copyright 1996 Digital Equipment Corp.
(c) Copyright 1996 Fujitsu Limited
(c) Copyright 1996 Hitachi, Ltd.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the copyright holders shall
not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from said
copyright holders.
*/

#include "Oid.h"

/*
 * attributes.c
 */
void XpInitAttributes(XpContextPtr pContext);
char *XpGetOneAttribute(XpContextPtr pContext,
			XPAttributes class,
			char *attributeName);
void XpPutOneAttribute(XpContextPtr pContext,
		       XPAttributes class,
		       const char* attributeName,
		       const char* value);
char *XpGetAttributes(XpContextPtr pContext,
		      XPAttributes class);
int XpAugmentAttributes(XpContextPtr pContext,
			 XPAttributes class,
			 char *attributes);
int XpSetAttributes(XpContextPtr pContext,
		     XPAttributes class,
		     char *attributes);
const char *XpGetPrinterAttribute(const char *printerName,
				  const char *attribute);
void XpGetTrayMediumFromContext(XpContextPtr pCon,
				char **medium,
				char **tray);

/*
 * mediaSizes.c
 */
int XpGetResolution(XpContextPtr pContext);
XpOid XpGetContentOrientation(XpContextPtr pContext);
XpOid XpGetAvailableCompression(XpContextPtr pContext);
XpOid XpGetPlex(XpContextPtr pContext);
XpOid XpGetPageSize(XpContextPtr pContext,
		    XpOid* pTray,
		    const XpOidMediumSS* msss);
void XpGetMediumMillimeters(XpOid page_size,
			    float *width,
			    float *height);
void XpGetMediumDimensions(XpContextPtr pContext,
			   unsigned short *width,
			   unsigned short *height);
void XpGetReproductionArea(XpContextPtr pContext,
			   xRectangle *pRect);
