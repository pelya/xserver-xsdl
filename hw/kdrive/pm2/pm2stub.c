#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "kdrive.h"

#include "pm2.h"

static const int PM2Cards[]={ PCI_CHIP_3DLABS_PERMEDIA2, PCI_CHIP_3DLABS_PERMEDIA2V };


#define numPM2Cards (sizeof(PM2Cards) / sizeof(PM2Cards[0]))

void
InitCard (char *name)
{
    KdCardAttr	attr;
    int		i;

    Bool LinuxFindPci(CARD16, CARD16, CARD32, KdCardAttr *); 

    for (i = 0; i < numPM2Cards; i++)
	if (LinuxFindPci (0x3d3d, PM2Cards[i], 0, &attr))
	    KdCardInfoAdd (&PM2Funcs, &attr, (void *) PM2Cards[i]);
}


void
InitOutput (ScreenInfo *pScreenInfo, int argc, char **argv)
{
    KdInitOutput (pScreenInfo, argc, argv);
}

void
InitInput (int argc, char **argv)
{
    KdInitInput (&LinuxMouseFuncs, &LinuxKeyboardFuncs);
}

void
ddxUseMsg (void)
{
    KdUseMsg();
    vesaUseMsg();
}

int
ddxProcessArgument (int argc, char **argv, int i)
{
    int	ret;
    
    if (!(ret = vesaProcessArgument (argc, argv, i)))
	ret = KdProcessArgument(argc, argv, i);
    return ret;
}
