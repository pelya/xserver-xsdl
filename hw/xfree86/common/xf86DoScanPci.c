/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86DoScanPci.c,v 1.15 2003/09/23 06:43:46 dawes Exp $ */
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
#include <pciaccess.h>
#include "os.h"
#ifdef XFree86LOADER
#include "loaderProcs.h"
#endif
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Pci.h"

static void ScanPciDisplayPCICardInfo( int verbosity );

void
ScanPciDisplayPCICardInfo(int verbosity)
{
    struct pci_id_match   match;
    pciConfigPtr pcrp, *pcrpp;
    int i;

    xf86EnableIO();
    pcrpp = xf86scanpci(0);

    if (pcrpp == NULL) {
        xf86MsgVerb(X_NONE,0,"No PCI info available\n");
	return;
    }
    xf86MsgVerb(X_NONE,0,"Probing for PCI devices (Bus:Device:Function)\n\n");
    for (i = 0; (pcrp = pcrpp[i]); i++) {
	const char *svendorname = NULL, *subsysname = NULL;
	const char *vendorname = NULL, *devicename = NULL;
	Bool noCard = FALSE;
	const char *prefix1 = "", *prefix2 = "";

	xf86MsgVerb(X_NONE, -verbosity, "(%d:%d:%d) ",
		    pcrp->busnum, pcrp->devnum, pcrp->funcnum);

	/*
	 * Lookup as much as we can about the device.
	 */
	match.vendor_id = pcrp->pci_vendor;
	match.device_id = pcrp->_pci_device;
	match.subvendor_id = (pcrp->pci_subsys_vendor != 0)
	  ? pcrp->pci_subsys_vendor : PCI_MATCH_ANY;
	match.subdevice_id = (pcrp->pci_subsys_card != 0)
	  ? pcrp->pci_subsys_card : PCI_MATCH_ANY;
	match.device_class = 0;
	match.device_class_mask = 0;

	pci_get_strings( & match, & vendorname, & devicename,
			 & svendorname, & subsysname);

	if (svendorname)
	    xf86MsgVerb(X_NONE, -verbosity, "%s ", svendorname);
	if (subsysname)
	    xf86MsgVerb(X_NONE, -verbosity, "%s ", subsysname);
	if (svendorname && !subsysname) {
	    if ( match.subdevice_id != PCI_MATCH_ANY ) {
		xf86MsgVerb(X_NONE, -verbosity, "unknown card (0x%04x) ",
			    match.subdevice_id);
	    } else {
		xf86MsgVerb(X_NONE, -verbosity, "card ");
	    }
	}
	if (!svendorname && !subsysname) {
	    /*
	     * We didn't find a text representation of the information 
	     * about the card.
	     */
	    if ( (match.subvendor_id != PCI_MATCH_ANY)
		 || (match.subdevice_id != PCI_MATCH_ANY) ) {
		/*
		 * If there was information and we just couldn't interpret
		 * it, print it out as unknown, anyway.
		 */
		xf86MsgVerb(X_NONE, -verbosity,
			    "unknown card (0x%04x/0x%04x) ",
			    match.subvendor_id, match.subdevice_id);
	    } else
		noCard = TRUE;
	}
	if (!noCard) {
	    prefix1 = "using a ";
	    prefix2 = "using an ";
	}
	if (vendorname && devicename) {
	    xf86MsgVerb(X_NONE, -verbosity,"%s%s %s\n", prefix1, vendorname,
			devicename);
	} else if (vendorname) {
	    xf86MsgVerb(X_NONE, -verbosity,
			"%sunknown chip (DeviceId 0x%04x) from %s\n",
			prefix2, match.device_id, vendorname);
	} else {
	    xf86MsgVerb(X_NONE, -verbosity,
			"%sunknown chipset(0x%04x/0x%04x)\n",
			prefix2, match.vendor_id, match.device_id);
	}
    }
}


void DoScanPci(int argc, char **argv, int i)
{
  int j,skip,globalVerbose;

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
  ScanPciDisplayPCICardInfo( 0 );

  /*
   * That's it; we really should clean things up, but a simple
   * exit seems to be all we need.
   */
  exit(0);
}
