/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86isaBus.c,v 3.5 2000/12/06 15:35:11 eich Exp $ */

/*
 * Copyright (c) 1997-1999 by The XFree86 Project, Inc.
 */

/*
 * This file contains the interfaces to the bus-specific code
 */

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Resources.h"

#include "xf86Bus.h"

#define XF86_OS_PRIVS
#define NEED_OS_RAC_PROTOS
#include "xf86_OSproc.h"

#include "xf86RAC.h"

Bool isaSlotClaimed = FALSE;

/*
 * If the slot requested is already in use, return FALSE.
 * Otherwise, claim the slot for the screen requesting it.
 */

int
xf86ClaimIsaSlot(DriverPtr drvp, int chipset, GDevPtr dev, Bool active)
{
    EntityPtr p;
    BusAccPtr pbap = xf86BusAccInfo;
    int num;
    
    num = xf86AllocateEntity();
    p = xf86Entities[num];
    p->driver = drvp;
    p->chipset = chipset;
    p->busType = BUS_ISA;
    p->active = active;
    p->inUse = FALSE;
    xf86AddDevToEntity(num, dev);
    p->access = xnfcalloc(1,sizeof(EntityAccessRec));
    p->access->fallback = &AccessNULL;
    p->access->pAccess = &AccessNULL;
    p->busAcc = NULL;
    while (pbap) {
	if (pbap->type == BUS_ISA)
	    p->busAcc = pbap;
	pbap = pbap->next;
    }
    isaSlotClaimed = TRUE;
    return num;
}

/*
 * Get the list of ISA "slots" claimed by a screen
 *
 * Note: The ISA implementation here assumes that only one ISA "slot" type
 * can be claimed by any one screen.  That means a return value other than
 * 0 or 1 isn't useful.
 */
int
xf86GetIsaInfoForScreen(int scrnIndex)
{
    int num = 0;
    int i;
    EntityPtr p;
    
    for (i = 0; i < xf86Screens[scrnIndex]->numEntities; i++) {
	p = xf86Entities[xf86Screens[scrnIndex]->entityList[i]];
  	if (p->busType == BUS_ISA) {
  	    num++;
  	}
    }
    return num;
}

/*
 * Parse a BUS ID string, and return True if it is a ISA bus id.
 */

Bool
xf86ParseIsaBusString(const char *busID)
{
    /*
     * The format assumed to be "isa" or "isa:"
     */
    return (StringToBusType(busID,NULL) == BUS_ISA);
}


/*
 * xf86IsPrimaryIsa() -- return TRUE if primary device
 * is ISA.
 */
 
Bool
xf86IsPrimaryIsa(void)
{
    return ( primaryBus.type == BUS_ISA );
}

void
isaConvertRange2Host(resRange *pRange)
{
    return;
}
