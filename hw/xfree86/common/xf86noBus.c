/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86noBus.c,v 1.1 2002/09/18 08:54:55 eich Exp $ */

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

int
xf86ClaimNoSlot(DriverPtr drvp, int chipset, GDevPtr dev, Bool active)
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

    return num;
}
