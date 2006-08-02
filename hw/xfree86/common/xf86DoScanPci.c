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
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Pci.h"
#include "Pci.h"
#include "xf86_OSproc.h"

static void ScanPciDisplayPCICardInfo(void);

void
ScanPciDisplayPCICardInfo(void)
{
    struct pci_id_match   match;
    struct pci_device_iterator *iter;
    const struct pci_device *dev;

    xf86EnableIO();

    if (! xf86scanpci()) {
        xf86MsgVerb(X_NONE, 0, "No PCI info available\n");
	return;
    }
    xf86MsgVerb(X_NONE, 0,
		"Probing for PCI devices (Bus:Device:Function)\n\n");

    iter = pci_id_match_iterator_create(NULL);
    while ((dev = pci_device_next(iter)) != NULL) {
	const char *svendorname = NULL, *subsysname = NULL;
	const char *vendorname = NULL, *devicename = NULL;


	xf86MsgVerb(X_NONE, 0, "(%d:%d:%d) ",
		    PCI_MAKE_BUS(dev->domain, dev->bus), dev->dev, dev->func);

	/*
	 * Lookup as much as we can about the device.
	 */
	match.vendor_id = dev->vendor_id;
	match.device_id = dev->device_id;
	match.subvendor_id = (dev->subvendor_id != 0)
	  ? dev->subvendor_id : PCI_MATCH_ANY;
	match.subdevice_id = (dev->subdevice_id != 0)
	  ? dev->subdevice_id : PCI_MATCH_ANY;
	match.device_class = 0;
	match.device_class_mask = 0;

	pci_get_strings(& match, & vendorname, & devicename,
			& svendorname, & subsysname);

	if ((dev->subvendor_id != 0) || (dev->subdevice_id != 0)) {
	    xf86MsgVerb(X_NONE, 0, "%s %s (0x%04x / 0x%04x) using ",
			(svendorname == NULL) ? "unknown vendor" : svendorname,
			(subsysname == NULL) ? "unknown card" : subsysname,
			dev->subvendor_id, dev->subdevice_id);
	}

	xf86MsgVerb(X_NONE, 0, "%s %s (0x%04x / 0x%04x)\n",
		    (vendorname == NULL) ? "unknown vendor" : vendorname,
		    (devicename == NULL) ? "unknown chip" : devicename,
		    dev->vendor_id, dev->device_id);
    }

    pci_iterator_destroy(iter);
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

  ScanPciDisplayPCICardInfo();

  /*
   * That's it; we really should clean things up, but a simple
   * exit seems to be all we need.
   */
  exit(0);
}
