/* $Xorg: PsAttr.c,v 1.4 2001/02/09 02:04:35 xorgcvs Exp $ */
/*

Copyright 1996, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
/*
 * (c) Copyright 1996 Hewlett-Packard Company
 * (c) Copyright 1996 International Business Machines Corp.
 * (c) Copyright 1996 Sun Microsystems, Inc.
 * (c) Copyright 1996 Novell, Inc.
 * (c) Copyright 1996 Digital Equipment Corp.
 * (c) Copyright 1996 Fujitsu Limited
 * (c) Copyright 1996 Hitachi, Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the names of the copyright holders
 * shall not be used in advertising or otherwise to promote the sale, use
 * or other dealings in this Software without prior written authorization
 * from said copyright holders.
 */
/* $XFree86: xc/programs/Xserver/Xprint/ps/PsAttr.c,v 1.5 2001/12/14 19:59:15 dawes Exp $ */

/*******************************************************************
**
**    *********************************************************
**    *
**    *  File:		PsAttr.c
**    *
**    *  Contents:	Attribute-handling functions for the PS driver
**    *
**    *  Created By:	Roger Helmendach (Liberty Systems)
**    *
**    *  Copyright:	Copyright 1996 The Open Group, Inc.
**    *
**    *********************************************************
** 
********************************************************************/

#include "Ps.h"

char *
PsGetAttributes(
  XpContextPtr pCon,
  XPAttributes pool)
{
  return XpGetAttributes(pCon, pool);
}

char *
PsGetOneAttribute(
  XpContextPtr  pCon,
  XPAttributes  pool,
  char         *attr)
{
  return XpGetOneAttribute(pCon, pool, attr);
}

int
PsAugmentAttributes(
  XpContextPtr  pCon,
  XPAttributes  pool,
  char         *attrs)
{
  return XpAugmentAttributes(pCon, pool, attrs);
}

int
PsSetAttributes(
  XpContextPtr  pCon,
  XPAttributes  pool,
  char         *attrs)
{
  return XpSetAttributes(pCon, pool, attrs);
}
