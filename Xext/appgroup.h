/* $XFree86: xc/programs/Xserver/Xext/appgroup.h,v 1.1 2003/07/16 01:38:28 dawes Exp $ */

void XagClientStateChange(
    CallbackListPtr* pcbl,
    pointer nulldata,
    pointer calldata);
int ProcXagCreate (
    register ClientPtr client);
int ProcXagDestroy(
    register ClientPtr client);
