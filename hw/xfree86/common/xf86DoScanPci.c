
/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86DoScanPci.c,v 1.12 2002/07/15 20:46:01 dawes Exp $ */
/*
 * Finish setting up the server.
 * Call the functions from the scanpci module.
 *
 * Copyright 1999-2002 by The XFree86 Project, Inc.
 *
 */

#include <ctype.h>
#include <stdlib.h>
#include "X.h"
#include "Xmd.h"
#include "os.h"
#ifdef XFree86LOADER
#include "loaderProcs.h"
#endif
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Pci.h"
#include "xf86ScanPci.h"


void DoScanPci(int argc, char **argv, int i)
{
  int j,skip,globalVerbose,scanpciVerbose;
  ScanPciSetupProcPtr PciSetup;
  ScanPciDisplayCardInfoProcPtr DisplayPCICardInfo;
#ifdef XFree86LOADER
  int errmaj, errmin;
#endif

  /*
   * first we need to finish setup of the OS so that we can call other
   * functions in the server
   */
  OsInit();

  /*
   * now we decrease verbosity and remember the value, in case a later
   * -verbose on the command line increases it, because that is a 
   * verbose flag for scanpci...
   */
  globalVerbose = --xf86Verbose;
  /*
   * next we process the arguments that are remaining on the command line,
   * so that things like the module path can be set there
   */
  for ( j = i+1; j < argc; j++ ) {
    if ((skip = ddxProcessArgument(argc, argv, j)))
	j += (skip - 1);
  } 
  /*
   * was the verbosity level increased?
   */
  if( (globalVerbose == 0) && (xf86Verbose > 0) )
    scanpciVerbose = xf86Verbose - globalVerbose -1;
  else
    scanpciVerbose = xf86Verbose - globalVerbose;
  xf86Verbose = globalVerbose;
  /*
   * now get the loader set up and load the scanpci module
   */
#ifdef XFree86LOADER
  /* Initialise the loader */
  LoaderInit();
  /* Tell the loader the default module search path */
  LoaderSetPath(xf86ModulePath);

  if (!LoadModule("scanpci", NULL, NULL, NULL, NULL, NULL,
                  &errmaj, &errmin)) {
    LoaderErrorMsg(NULL, "scanpci", errmaj, errmin);
    exit(1);
  }
  if (LoaderCheckUnresolved(LD_RESOLV_IFDONE)) {
      /* For now, just a warning */
      xf86Msg(X_WARNING, "Some symbols could not be resolved!\n");
  }
  PciSetup = (ScanPciSetupProcPtr)LoaderSymbol("ScanPciSetupPciIds");
  DisplayPCICardInfo =
    (ScanPciDisplayCardInfoProcPtr)LoaderSymbol("ScanPciDisplayPCICardInfo");
#else
  PciSetup = ScanPciSetupPciIds;
  DisplayPCICardInfo = ScanPciDisplayPCICardInfo;
#endif

  if (!(*PciSetup)())
    FatalError("ScanPciSetupPciIds() failed\n");
  (*DisplayPCICardInfo)(scanpciVerbose);

  /*
   * That's it; we really should clean things up, but a simple
   * exit seems to be all we need.
   */
  exit(0);
}
