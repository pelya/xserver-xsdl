/*
 * $Id$
 *
 * Copyright © 2003 Anders Carlsson
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Anders Carlsson not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Anders Carlsson makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * ANDERS CARLSSON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ANDERS CARLSSON BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $Header$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "kdrive.h"

typedef struct _RealOffscreenArea {
    KdOffscreenArea area;

    KdOffscreenMoveDataProc moveIn;
    KdOffscreenMoveDataProc moveOut;

    Bool locked;
    
    struct _RealOffscreenArea *next;
    struct _RealOffscreenArea *prev;
} RealOffscreenArea;

KdOffscreenArea *
KdOffscreenAlloc (ScreenPtr pScreen, int size, int align,
		  Bool locked,
		  KdOffscreenMoveDataProc moveIn,
		  KdOffscreenMoveDataProc moveOut,
		  pointer privData)
{
    RealOffscreenArea *area;
    KdScreenPriv (pScreen);
    int tmp, real_size;


    if (!align)
	align = 1;

    /* Go through the areas */
    area = pScreenPriv->screen->off_screen_areas;
    while (area != NULL)
    {
	if (area->area.screen != NULL)
	{
	    area = area->next;
	    continue;
	}

	real_size = size;
	tmp = (area->area.offset + area->area.size - size) % align;

	if (tmp)
	    real_size += (align - tmp);
	
	if (real_size <= area->area.size)
	{
	    RealOffscreenArea *new_area;

	    if (real_size == area->area.size)
	    {
		area->area.screen = pScreen;
		area->area.privData = privData;
		area->area.swappedOut = FALSE;		
		area->locked = locked;
		area->moveIn = moveIn;
		area->moveOut = moveOut;

		return (KdOffscreenArea *)area;
	    }
	    
	    /* Create a new area */
	    new_area = xalloc (sizeof (RealOffscreenArea));
	    new_area->area.offset = area->area.offset + area->area.size - real_size;
	    new_area->area.size = real_size;
	    new_area->area.screen = pScreen;
	    new_area->area.swappedOut = FALSE;
	    new_area->locked = locked;
	    new_area->moveIn = moveIn;
	    new_area->moveOut = moveOut;
	    
	    area->area.size -= real_size;

	    new_area->prev = area;
	    new_area->next = area->next;

	    if (area->next)
		area->next->prev = new_area;
	    area->next = new_area;

	    return (KdOffscreenArea *)new_area;
	}
	
	area = area->next;
    }

    /* Could not allocate memory */
    return NULL;
}

void
KdOffscreenSwapOut (ScreenPtr pScreen)
{
    KdScreenPriv (pScreen);
    RealOffscreenArea *area = pScreenPriv->screen->off_screen_areas;

    while (area)
    {
	if (area->area.screen && area->moveOut)
	    (*area->moveOut) ((KdOffscreenArea *)area);

	area->area.swappedOut = TRUE;

	area = area->next;
    }    
}

void
KdOffscreenSwapIn (ScreenPtr pScreen)
{
    KdScreenPriv (pScreen);
    RealOffscreenArea *area = pScreenPriv->screen->off_screen_areas;    

    while (area)
    {
	if (area->area.screen && area->moveIn)
	    (*area->moveIn) ((KdOffscreenArea *)area);

	area->area.swappedOut = FALSE;
	area = area->next;
    }    
}

void
KdOffscreenFree (KdOffscreenArea *area)
{
    RealOffscreenArea *real_area = (RealOffscreenArea *)area;
    
    real_area->area.screen = NULL;

    if (real_area && real_area->next && !real_area->next->area.screen)
    {
	RealOffscreenArea *tmp;
	
	real_area->next->prev = real_area->prev;
	if (real_area->prev)
	    real_area->prev->next = real_area->next;

	real_area->next->area.size += real_area->area.size;
	real_area->next->area.offset = real_area->area.offset;

	tmp = real_area->next;
	xfree (real_area);
	real_area = tmp;
    }
    
    if (real_area->prev && !real_area->prev->area.screen)
    {
	real_area->prev->next = real_area->next;
	if (real_area->next)
	    real_area->next->prev = real_area->prev;
	
	real_area->prev->area.size += real_area->area.size;
	
	xfree (real_area);
    }
}

Bool
KdOffscreenInit (ScreenPtr pScreen)
{
    KdScreenPriv (pScreen);
    RealOffscreenArea *area;

    /* Allocate a big free area */
    area = xalloc (sizeof (RealOffscreenArea));

    if (!area)
	return FALSE;

    area->area.screen = NULL;
    area->area.offset = pScreenPriv->screen->off_screen_base;
    area->area.size = pScreenPriv->screen->off_screen_size;
    area->area.swappedOut = FALSE;
    area->next = NULL;
    area->prev = NULL;
    
    /* Add it to the free areas */
    pScreenPriv->screen->off_screen_areas = area;
    
    return TRUE;
}

