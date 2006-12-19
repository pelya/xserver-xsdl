/*
 * Copyright (c) 1999-2002 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/*
 * Finish setting up the server.
 * Call the functions from the scanpci module.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <ctype.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xmd.h>
#include "os.h"
#include "loaderProcs.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Pci.h"
#include "xf86ScanPci.h"


void DoScanPci(int argc, char **argv, int i)
{
  int j,skip,globalVerbose,scanpciVerbose;
  ScanPciSetupProcPtr PciSetup;
  ScanPciDisplayCardInfoProcPtr DisplayPCICardInfo;
  int errmaj, errmin;

  /*
   * first we need to finish setup of the OS so that we can call other
   * functions in the server
   */
  OsInit();

  /*
   * The old verbosity processing that was here isn't useful anymore, but
   * for compatibility purposes, ignore verbosity changes after the -scanpci
   * flag.
   */
  globalVerbose = xf86Verbose;

  /*
   * next we process the arguments that are remaining on the command line,
   * so that things like the module path can be set there
   */
  for ( j = i+1; j < argc; j++ ) {
    if ((skip = ddxProcessArgument(argc, argv, j)))
	j += (skip - 1);
  } 

  /*
   * Was the verbosity level increased?  If so, set it back.
   */
  if (xf86Verbose > globalVerbose)
    xf86SetVerbosity(globalVerbose);

  /*
   * Setting scanpciVerbose to 0 will ensure that the output will go to
   * stderr for all reasonable default stderr verbosity levels.
   */
  scanpciVerbose = 0;

  /*
   * now get the loader set up and load the scanpci module
   */
  /* Initialise the loader */
  LoaderInit();
  /* Tell the loader the default module search path */
  LoaderSetPath(xf86ModulePath);

  if (!LoadModule("scanpci", NULL, NULL, NULL, NULL, NULL,
                  &errmaj, &errmin)) {
    LoaderErrorMsg(NULL, "scanpci", errmaj, errmin);
    exit(1);
  }
  PciSetup = (ScanPciSetupProcPtr)LoaderSymbol("ScanPciSetupPciIds");
  DisplayPCICardInfo =
    (ScanPciDisplayCardInfoProcPtr)LoaderSymbol("ScanPciDisplayPCICardInfo");

  if (!(*PciSetup)())
    FatalError("ScanPciSetupPciIds() failed\n");
  (*DisplayPCICardInfo)(scanpciVerbose);

  /*
   * That's it; we really should clean things up, but a simple
   * exit seems to be all we need.
   */
  exit(0);
}
