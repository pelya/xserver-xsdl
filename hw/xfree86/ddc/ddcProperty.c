/* ddcProperty.c: Make the DDC monitor information available to clients
 * as properties on the root window
 * 
 * Copyright 1999 by Andrew C Aitchison <A.C.Aitchison@dpmms.cam.ac.uk>
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/ddc/ddcProperty.c,v 1.10 2003/11/03 05:11:04 tsi Exp $ */

#include "misc.h"
#include "xf86.h"
/* #include "xf86_ansic.h" */
/* #include "xf86_OSproc.h" */
#include "Xatom.h"
#include "property.h"
#include "propertyst.h"
#include "xf86DDC.h"

#define EDID1_ATOM_NAME         "XFree86_DDC_EDID1_RAWDATA"
#define EDID2_ATOM_NAME         "XFree86_DDC_EDID2_RAWDATA"
#define VDIF_ATOM_NAME          "XFree86_DDC_VDIF_RAWDATA"

Bool
xf86SetDDCproperties(ScrnInfoPtr pScrnInfo, xf86MonPtr DDC)
{
    Atom EDID1Atom=-1, EDID2Atom=-1, VDIFAtom=-1;
    CARD8 *EDID1rawdata = NULL;
    CARD8 *EDID2rawdata = NULL;
    int  i, ret;
    Bool  makeEDID1prop = FALSE;
    Bool  makeEDID2prop = FALSE;

#ifdef DEBUG
    ErrorF("xf86SetDDCproperties(%p, %p)\n", pScrnInfo, DDC);
#endif

    if (pScrnInfo==NULL || pScrnInfo->monitor==NULL || DDC==NULL) {
      return FALSE;
    }

#ifdef DEBUG
    ErrorF("pScrnInfo->scrnIndex %d\n", pScrnInfo->scrnIndex);

    ErrorF("pScrnInfo->monitor was %p\n", pScrnInfo->monitor);
#endif

    pScrnInfo->monitor->DDC = DDC;

    if (DDC->ver.version == 1) {
      makeEDID1prop = TRUE;
    } else if (DDC->ver.version == 2) {
      int checksum1;
      int checksum2;
      makeEDID2prop = TRUE;

      /* Some monitors (eg Panasonic PanaSync4)
       * report version==2 because they used EDID v2 spec document,
       * although they use EDID v1 data structure :-(
       *
       * Try using checksum to determine when we have such a monitor.
       */
      checksum2 = 0;
      for (i=0; i<256; i++) { checksum2 += DDC->rawData[i]; }
      if ( (checksum2 % 256) != 0 ) {
	xf86DrvMsg(pScrnInfo->scrnIndex,X_INFO, "Monitor EDID v2 checksum failed\n");
	xf86DrvMsg(pScrnInfo->scrnIndex,X_INFO, "XFree86_DDC_EDID2_RAWDATA property may be bad\n");
	checksum1 = 0;
	for (i=0; i<128; i++) { checksum1 += DDC->rawData[i]; }
	if ( (checksum1 % 256) == 0 ) {
	  xf86DrvMsg(pScrnInfo->scrnIndex,X_INFO, "Monitor EDID v1 checksum passed,\n");
	  xf86DrvMsg(pScrnInfo->scrnIndex,X_INFO, "XFree86_DDC_EDID1_RAWDATA property created\n");
	  makeEDID1prop = TRUE;
	}
      }
    } else {
     xf86DrvMsg(pScrnInfo->scrnIndex, X_PROBED,
		"unexpected EDID version %d revision %d\n",
		DDC->ver.version, DDC->ver.revision );      
    }

    if (makeEDID1prop) {
      if ( (EDID1rawdata = xalloc(128*sizeof(CARD8)))==NULL ) {
	return FALSE;
      }

      EDID1Atom = MakeAtom(EDID1_ATOM_NAME, sizeof(EDID1_ATOM_NAME), TRUE);


      for (i=0; i<128; i++) {
	EDID1rawdata[i] = DDC->rawData[i];
      }

#ifdef DEBUG
      ErrorF("xf86RegisterRootWindowProperty %p(%d,%d,%d,%d,%d,%p)\n",
	     xf86RegisterRootWindowProperty,
	     pScrnInfo->scrnIndex,
	     EDID1Atom, XA_INTEGER, 8,
	     128, (unsigned char *)EDID1rawdata  );
#endif

      ret = xf86RegisterRootWindowProperty(pScrnInfo->scrnIndex,
					   EDID1Atom, XA_INTEGER, 8, 
					   128, (unsigned char *)EDID1rawdata
					   );
      if (ret != Success)
      ErrorF("xf86RegisterRootWindowProperty returns %d\n", ret );
    } 

    if (makeEDID2prop) {
      if ( (EDID2rawdata = xalloc(256*sizeof(CARD8)))==NULL ) {
	return FALSE;
      }
      for (i=0; i<256; i++) {
	EDID2rawdata[i] = DDC->rawData[i];
      }

      EDID2Atom = MakeAtom(EDID2_ATOM_NAME, sizeof(EDID2_ATOM_NAME), TRUE);

#ifdef DEBUG
      ErrorF("xf86RegisterRootWindowProperty %p(%d,%d,%d,%d,%d,%p)\n",
	     xf86RegisterRootWindowProperty,
	     pScrnInfo->scrnIndex,
	     EDID2Atom, XA_INTEGER, 8,
	     256, (unsigned char *)EDID2rawdata  );
#endif
      ret = xf86RegisterRootWindowProperty(pScrnInfo->scrnIndex,
					   EDID2Atom, XA_INTEGER, 8, 
					   256, (unsigned char *)EDID2rawdata
					   );
      if (ret != Success)
      ErrorF("xf86RegisterRootWindowProperty returns %d\n", ret );
    }

    if (DDC->vdif) {
#define VDIF_DUMMY_STRING "setting dummy VDIF property - please insert correct values\n"
#ifdef DEBUG
      ErrorF("xf86RegisterRootWindowProperty %p(%d,%d,%d,%d,%d,%p)\n",
	     xf86RegisterRootWindowProperty,
	     pScrnInfo->scrnIndex,
	     VDIFAtom, XA_STRING, 8,
	     strlen(VDIF_DUMMY_STRING), VDIF_DUMMY_STRING 
	     );
#endif


      VDIFAtom = MakeAtom(VDIF_ATOM_NAME, sizeof(VDIF_ATOM_NAME), TRUE);

      ret = xf86RegisterRootWindowProperty(pScrnInfo->scrnIndex,
					   VDIFAtom, XA_STRING, 8, 
					   strlen(VDIF_DUMMY_STRING),
					   VDIF_DUMMY_STRING 
					   );
      if (ret != Success)
      ErrorF("xf86RegisterRootWindowProperty returns %d\n", ret );
    }

    return TRUE;
}
