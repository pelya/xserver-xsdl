/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86fbBus.c,v 1.2 2001/10/28 03:33:19 tsi Exp $ */

/*
 * Copyright (c) 2000 by The XFree86 Project, Inc.
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

Bool fbSlotClaimed = FALSE;

int
xf86ClaimFbSlot(DriverPtr drvp, int chipset, GDevPtr dev, Bool active)
{
    EntityPtr p;
    int num;
    
    num = xf86AllocateEntity();
    p = xf86Entities[num];
    p->driver = drvp;
    p->chipset = 0;
    p->busType = BUS_NONE;
    p->active = active;
    p->inUse = FALSE;
    xf86AddDevToEntity(num, dev);
    p->access = xnfcalloc(1,sizeof(EntityAccessRec));
    p->access->fallback = &AccessNULL;
    p->access->pAccess = &AccessNULL;
    p->busAcc = NULL;

    fbSlotClaimed = TRUE;
    return num;
}

/*
 * Get the list of FB "slots" claimed by a screen
 */
int
xf86GetFbInfoForScreen(int scrnIndex)
{
    int num = 0;
    int i;
    EntityPtr p;
    
    for (i = 0; i < xf86Screens[scrnIndex]->numEntities; i++) {
	p = xf86Entities[xf86Screens[scrnIndex]->entityList[i]];
  	if (p->busType == BUS_NONE) {
  	    num++;
  	}
    }
    return num;
}
