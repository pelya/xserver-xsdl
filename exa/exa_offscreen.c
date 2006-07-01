/*
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

/** @file
 * This allocator allocates blocks of memory by maintaining a list of areas
 * and a score for each area.  As an area is marked used, its score is
 * incremented, and periodically all of the areas have their scores decayed by
 * a fraction.  When allocating, the contiguous block of areas with the minimum
 * score is found and evicted in order to make room for the new allocation.
 */

#include "exa_priv.h"

#include <limits.h>
#include <assert.h>
#include <stdlib.h>

#if DEBUG_OFFSCREEN
#define DBG_OFFSCREEN(a) ErrorF a
#else
#define DBG_OFFSCREEN(a)
#endif

#if DEBUG_OFFSCREEN
static void
ExaOffscreenValidate (ScreenPtr pScreen)
{
    ExaScreenPriv (pScreen);
    ExaOffscreenArea *prev = 0, *area;

    assert (pExaScr->info->offScreenAreas->base_offset == 
	    pExaScr->info->offScreenBase);
    for (area = pExaScr->info->offScreenAreas; area; area = area->next)
    {
	assert (area->offset >= area->base_offset &&
		area->offset < (area->base_offset + area->size));
	if (prev)
	    assert (prev->base_offset + prev->area.size == area->base_offset);
	prev = area;
    }
    assert (prev->base_offset + prev->size == pExaScr->info->memorySize);
}
#else
#define ExaOffscreenValidate(s)
#endif

static ExaOffscreenArea *
ExaOffscreenKickOut (ScreenPtr pScreen, ExaOffscreenArea *area)
{
    if (area->save)
	(*area->save) (pScreen, area);
    return exaOffscreenFree (pScreen, area);
}

/**
 * exaOffscreenAlloc allocates offscreen memory
 *
 * @param pScreen current screen
 * @param size size in bytes of the allocation
 * @param align byte alignment requirement for the offset of the allocated area
 * @param locked whether the allocated area is locked and can't be kicked out
 * @param save callback for when the area is evicted from memory
 * @param privdata private data for the save callback.
 *
 * Allocates offscreen memory from the device associated with pScreen.  size and
 * align deteremine where and how large the allocated area is, and locked will
 * mark whether it should be held in card memory.  privdata may be any pointer
 * for the save callback when the area is removed.
 *
 * Note that locked areas do get evicted on VT switch, because during that time
 * all offscreen memory becomes inaccessible.  This may change in the future,
 * but drivers should be aware of this and provide a callback to mark that their
 * locked allocation was evicted, and then restore it if necessary on EnterVT.
 */
ExaOffscreenArea *
exaOffscreenAlloc (ScreenPtr pScreen, int size, int align,
                   Bool locked,
                   ExaOffscreenSaveProc save,
                   pointer privData)
{
    ExaOffscreenArea *area, *begin, *best;
    ExaScreenPriv (pScreen);
    int tmp, real_size = 0, best_score;
#if DEBUG_OFFSCREEN
    static int number = 0;
    ErrorF("================= ============ allocating a new pixmap %d\n", ++number);
#endif

    ExaOffscreenValidate (pScreen);
    if (!align)
	align = 1;

    if (!size)
    {
	DBG_OFFSCREEN (("Alloc 0x%x -> EMPTY\n", size));
	return NULL;
    }

    /* throw out requests that cannot fit */
    if (size > (pExaScr->info->memorySize - pExaScr->info->offScreenBase))
    {
	DBG_OFFSCREEN (("Alloc 0x%x vs (0x%lx) -> TOBIG\n", size,
			pExaScr->info->memorySize -
			pExaScr->info->offScreenBase));
	return NULL;
    }

    /* Try to find a free space that'll fit. */
    for (area = pExaScr->info->offScreenAreas; area; area = area->next)
    {
	/* skip allocated areas */
	if (area->state != ExaOffscreenAvail)
	    continue;

	/* adjust size to match alignment requirement */
	real_size = size;
	tmp = area->base_offset % align;
	if (tmp)
	    real_size += (align - tmp);

	/* does it fit? */
	if (real_size <= area->size)
	    break;
    }

    if (!area)
    {
	/*
	 * Kick out existing users to make space.
	 *
	 * First, locate a region which can hold the desired object.
	 */

	/* prev points at the first object to boot */
	best = NULL;
	best_score = INT_MAX;
	for (begin = pExaScr->info->offScreenAreas; begin != NULL;
	     begin = begin->next)
	{
	    int avail, score;
	    ExaOffscreenArea *scan;

	    if (begin->state == ExaOffscreenLocked)
		continue;

	    /* adjust size needed to account for alignment loss for this area */
	    real_size = size;
	    tmp = begin->base_offset % align;
	    if (tmp)
		real_size += (align - tmp);

	    avail = 0;
	    score = 0;
	    /* now see if we can make room here, and how "costly" it'll be. */
	    for (scan = begin; scan != NULL; scan = scan->next)
	    {
		if (scan->state == ExaOffscreenLocked) {
		    /* Can't make room here, start after this locked area. */
		    begin = scan;
		    break;
		}
		/* Score should only be non-zero for ExaOffscreenRemovable */
		score += scan->score;
		avail += scan->size;
		if (avail >= real_size)
		    break;
	    }
	    /* Is it the best option we've found so far? */
	    if (avail >= real_size && score < best_score) {
		best = begin;
		best_score = score;
	    }
	}
	area = best;
	if (!area)
	{
	    DBG_OFFSCREEN (("Alloc 0x%x -> NOSPACE\n", size));
	    /* Could not allocate memory */
	    ExaOffscreenValidate (pScreen);
	    return NULL;
	}

	/* adjust size needed to account for alignment loss for this area */
	real_size = size;
	tmp = area->base_offset % align;
	if (tmp)
	    real_size += (align - tmp);

	/*
	 * Kick out first area if in use
	 */
	if (area->state != ExaOffscreenAvail)
	    area = ExaOffscreenKickOut (pScreen, area);
	/*
	 * Now get the system to merge the other needed areas together
	 */
	while (area->size < real_size)
	{
	    assert (area->next && area->next->state == ExaOffscreenRemovable);
	    (void) ExaOffscreenKickOut (pScreen, area->next);
	}
    }

    /* save extra space in new area */
    if (real_size < area->size)
    {
	ExaOffscreenArea   *new_area = xalloc (sizeof (ExaOffscreenArea));
	if (!new_area)
	    return NULL;
	new_area->base_offset = area->base_offset + real_size;
	new_area->offset = new_area->base_offset;
	new_area->size = area->size - real_size;
	new_area->state = ExaOffscreenAvail;
	new_area->save = NULL;
	new_area->score = 0;
	new_area->next = area->next;
	area->next = new_area;
	area->size = real_size;
    }
    /*
     * Mark this area as in use
     */
    if (locked)
	area->state = ExaOffscreenLocked;
    else
	area->state = ExaOffscreenRemovable;
    area->privData = privData;
    area->save = save;
    area->score = 0;
    area->offset = (area->base_offset + align - 1);
    area->offset -= area->offset % align;

    ExaOffscreenValidate (pScreen);

    DBG_OFFSCREEN (("Alloc 0x%x -> 0x%x (0x%x)\n", size,
		    area->base_offset, area->offset));
    return area;
}

void
ExaOffscreenSwapOut (ScreenPtr pScreen)
{
    ExaScreenPriv (pScreen);

    ExaOffscreenValidate (pScreen);
    /* loop until a single free area spans the space */
    for (;;)
    {
	ExaOffscreenArea *area = pExaScr->info->offScreenAreas;

	if (!area)
	    break;
	if (area->state == ExaOffscreenAvail)
	{
	    area = area->next;
	    if (!area)
		break;
	}
	assert (area->state != ExaOffscreenAvail);
	(void) ExaOffscreenKickOut (pScreen, area);
	ExaOffscreenValidate (pScreen);
    }
    ExaOffscreenValidate (pScreen);
    ExaOffscreenFini (pScreen);
}

void
ExaOffscreenSwapIn (ScreenPtr pScreen)
{
    exaOffscreenInit (pScreen);
}

void
exaEnableDisableFBAccess (int index, Bool enable)
{
    ScreenPtr pScreen = screenInfo.screens[index];
    ExaScreenPriv (pScreen);

    if (!enable) {
	ExaOffscreenSwapOut (pScreen);
	pExaScr->swappedOut = TRUE;
    } else {
	ExaOffscreenSwapIn (pScreen);
	pExaScr->swappedOut = FALSE;
    }
}

/* merge the next free area into this one */
static void
ExaOffscreenMerge (ExaOffscreenArea *area)
{
    ExaOffscreenArea	*next = area->next;

    /* account for space */
    area->size += next->size;
    /* frob pointer */
    area->next = next->next;
    xfree (next);
}

/**
 * exaOffscreenFree frees an allocation.
 *
 * @param pScreen current screen
 * @param area offscreen area to free
 *
 * exaOffscreenFree frees an allocation created by exaOffscreenAlloc.  Note that
 * the save callback of the area is not called, and it is up to the driver to
 * do any cleanup necessary as a result.
 *
 * @return pointer to the newly freed area. This behavior should not be relied
 * on.
 */
ExaOffscreenArea *
exaOffscreenFree (ScreenPtr pScreen, ExaOffscreenArea *area)
{
    ExaScreenPriv(pScreen);
    ExaOffscreenArea	*next = area->next;
    ExaOffscreenArea	*prev;

    DBG_OFFSCREEN (("Free 0x%x -> 0x%x (0x%x)\n", area->size,
		    area->base_offset, area->offset));
    ExaOffscreenValidate (pScreen);

    area->state = ExaOffscreenAvail;
    area->save = NULL;
    area->score = 0;
    /*
     * Find previous area
     */
    if (area == pExaScr->info->offScreenAreas)
	prev = NULL;
    else
	for (prev = pExaScr->info->offScreenAreas; prev; prev = prev->next)
	    if (prev->next == area)
		break;

    /* link with next area if free */
    if (next && next->state == ExaOffscreenAvail)
	ExaOffscreenMerge (area);

    /* link with prev area if free */
    if (prev && prev->state == ExaOffscreenAvail)
    {
	area = prev;
	ExaOffscreenMerge (area);
    }

    ExaOffscreenValidate (pScreen);
    DBG_OFFSCREEN(("\tdone freeing\n"));
    return area;
}

void
ExaOffscreenMarkUsed (PixmapPtr pPixmap)
{
    ExaPixmapPriv (pPixmap);
    ExaScreenPriv (pPixmap->drawable.pScreen);
    static int iter = 0;

    if (!pExaPixmap->area)
	return;

    /* The numbers here are arbitrary.  We may want to tune these. */
    pExaPixmap->area->score += 100;
    if (++iter == 10) {
	ExaOffscreenArea *area;
	for (area = pExaScr->info->offScreenAreas; area != NULL;
	     area = area->next)
	{
	    if (area->state == ExaOffscreenRemovable)
		area->score = (area->score * 7) / 8;
	}
    }
}

/**
 * exaOffscreenInit initializes the offscreen memory manager.
 *
 * @param pScreen current screen
 *
 * exaOffscreenInit is called by exaDriverInit to set up the memory manager for
 * the screen, if any offscreen memory is available.
 */
Bool
exaOffscreenInit (ScreenPtr pScreen)
{
    ExaScreenPriv (pScreen);
    ExaOffscreenArea *area;

    /* Allocate a big free area */
    area = xalloc (sizeof (ExaOffscreenArea));

    if (!area)
	return FALSE;

    area->state = ExaOffscreenAvail;
    area->base_offset = pExaScr->info->offScreenBase;
    area->offset = area->base_offset;
    area->size = pExaScr->info->memorySize - area->base_offset;
    area->save = NULL;
    area->next = NULL;
    area->score = 0;

    /* Add it to the free areas */
    pExaScr->info->offScreenAreas = area;

    ExaOffscreenValidate (pScreen);

    return TRUE;
}

void
ExaOffscreenFini (ScreenPtr pScreen)
{
    ExaScreenPriv (pScreen);
    ExaOffscreenArea *area;

    /* just free all of the area records */
    while ((area = pExaScr->info->offScreenAreas))
    {
	pExaScr->info->offScreenAreas = area->next;
	xfree (area);
    }
}
