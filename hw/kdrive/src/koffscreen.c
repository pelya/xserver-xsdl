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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "kdrive.h"

#define DEBUG_OFFSCREEN 0
#if DEBUG_OFFSCREEN
#define DBG_OFFSCREEN(a) ErrorF a
#else
#define DBG_OFFSCREEN(a)
#endif

typedef struct _RealOffscreenArea {
    KdOffscreenArea area;

    KdOffscreenSaveProc save;

    Bool locked;
    
    struct _RealOffscreenArea *prev;
    struct _RealOffscreenArea *next;
} RealOffscreenArea;

#if DEBUG_OFFSCREEN
static void
KdOffscreenValidate (ScreenPtr pScreen)
{
    KdScreenPriv (pScreen);
    RealOffscreenArea *prev = 0, *area;

    assert (pScreenPriv->screen->off_screen_areas->area.offset == 0);
    for (area = pScreenPriv->screen->off_screen_areas; area; area = area->next)
    {
	if (prev)
	    assert (prev->area.offset + prev->area.size == area->area.offset);
	    
	prev = area;
    }
    assert (prev->area.offset + prev->area.size == pScreenPriv->screen->memory_size);
}
#else
#define KdOffscreenValidate(s)
#endif

static void
KdOffscreenKickOut (KdOffscreenArea *area)
{
    RealOffscreenArea *real_area = (RealOffscreenArea *) area;
    if (real_area->save)
	(*real_area->save) (area);
    KdOffscreenFree (area);
}

KdOffscreenArea *
KdOffscreenAlloc (ScreenPtr pScreen, int size, int align,
		  Bool locked,
		  KdOffscreenSaveProc save,
		  pointer privData)
{
    RealOffscreenArea *area, **prev;
    KdScreenPriv (pScreen);
    int tmp, real_size;

    KdOffscreenValidate (pScreen);
    if (!align)
	align = 1;

    if (!size)
    {
	DBG_OFFSCREEN (("Alloc 0x%x -> EMPTY\n", size));
	return NULL;
    }

    /* throw out requests that cannot fit */
    if (size > (pScreenPriv->screen->memory_size - pScreenPriv->screen->off_screen_base))
    {
	DBG_OFFSCREEN (("Alloc 0x%x -> TOBIG\n", size));
	return NULL;
    }
    
retry:

    /* Go through the areas */
    for (area = pScreenPriv->screen->off_screen_areas; area; area = area->next)
    {
	/* skip allocated areas */
	if (area->area.screen)
	    continue;

	/* adjust size to match alignment requirement */
	real_size = size;
	tmp = area->area.offset % align;
	if (tmp)
	    real_size += (align - tmp);
	
	/* does it fit? */
	if (real_size <= area->area.size)
	{
	    RealOffscreenArea *new_area;

	    /* save extra space in new area */
	    if (real_size < area->area.size)
	    {
		new_area = xalloc (sizeof (RealOffscreenArea));
		if (!new_area)
		    return NULL;
		new_area->area.offset = area->area.offset + real_size;
		new_area->area.size = area->area.size - real_size;
		new_area->area.screen = 0;
		new_area->locked = FALSE;
		new_area->save = 0;
		if ((new_area->next = area->next))
		    new_area->next->prev = new_area;
		new_area->prev = area;
		area->next = new_area;
		area->area.size = real_size;
	    }
	    area->area.screen = pScreen;
	    area->area.privData = privData;
	    area->locked = locked;
	    area->save = save;

	    KdOffscreenValidate (pScreen);
	    
	    DBG_OFFSCREEN (("Alloc 0x%x -> 0x%x\n", size, area->area.offset));
	    return &area->area;
	}
    }
    
    /* 
     * Kick out existing users.  This is pretty simplistic; it just
     * keeps deleting areas until the first area is free and has enough room
     */
    
    prev = (RealOffscreenArea **) &pScreenPriv->screen->off_screen_areas;
    while ((area = *prev))
    {
	if (area->area.screen && !area->locked)
	{
	    KdOffscreenKickOut (&area->area);
	    continue;
	}
	/* adjust size to match alignment requirement */
	real_size = size;
	tmp = area->area.offset % align;
	if (tmp)
	    real_size += (align - tmp);
	
	/* does it fit? */
	if (real_size <= area->area.size)
	    goto retry;

	/* kick out the next area */
	area = area->next;
	if (!area)
	    break;
	/* skip over locked areas */
	if (area->locked)
	{
	    prev = &area->next;
	    continue;
	}
	assert (area->area.screen);
	KdOffscreenKickOut (&area->area);
    }
    
    DBG_OFFSCREEN (("Alloc 0x%x -> NOSPACE\n", size));
    /* Could not allocate memory */
    KdOffscreenValidate (pScreen);
    return NULL;
}

void
KdOffscreenSwapOut (ScreenPtr pScreen)
{
    KdScreenPriv (pScreen);

    KdOffscreenValidate (pScreen);
    /* loop until a single free area spans the space */
    for (;;)
    {
	RealOffscreenArea *area = pScreenPriv->screen->off_screen_areas;
	
	if (!area)
	    break;
	if (!area->area.screen)
	{
	    area = area->next;
	    if (!area)
		break;
	}
	assert (area->area.screen);
	KdOffscreenKickOut (&area->area);
	KdOffscreenValidate (pScreen);
    }    
    KdOffscreenValidate (pScreen);
    KdOffscreenFini (pScreen);
}

void
KdOffscreenSwapIn (ScreenPtr pScreen)
{
    KdOffscreenInit (pScreen);
}

/* merge the next free area into this one */
static void
KdOffscreenMerge (KdOffscreenArea *area)
{
    RealOffscreenArea	*real_area = (RealOffscreenArea *) area;
    RealOffscreenArea	*next = real_area->next;

    /* account for space */
    real_area->area.size += next->area.size;
    /* frob pointers */
    if ((real_area->next = next->next))
	real_area->next->prev = real_area;
    xfree (next);
}

void
KdOffscreenFree (KdOffscreenArea *area)
{
    RealOffscreenArea	*real_area = (RealOffscreenArea *) area;
    RealOffscreenArea	*next = real_area->next;
    RealOffscreenArea	*prev = real_area->prev;
    
    DBG_OFFSCREEN (("Free 0x%x -> 0x%x\n", area->size, area->offset));
    KdOffscreenValidate (pScreen);

    area->screen = NULL;

    /* link with next area if free */
    if (next && !next->area.screen)
	KdOffscreenMerge (&real_area->area);
    
    /* link with prev area if free */
    if (prev && !prev->area.screen)
	KdOffscreenMerge (&prev->area);

    KdOffscreenValidate (pScreen);
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
    area->area.size = pScreenPriv->screen->memory_size - area->area.offset;
    area->save = 0;
    area->locked = FALSE;
    area->next = NULL;
    area->prev = NULL;
    
    /* Add it to the free areas */
    pScreenPriv->screen->off_screen_areas = area;
    
    KdOffscreenValidate (pScreen);

    return TRUE;
}

void
KdOffscreenFini (ScreenPtr pScreen)
{
    KdScreenPriv (pScreen);
    RealOffscreenArea *area;
    
    /* just free all of the area records */
    while ((area = pScreenPriv->screen->off_screen_areas))
    {
	pScreenPriv->screen->off_screen_areas = area->next;
	xfree (area);
    }
}
