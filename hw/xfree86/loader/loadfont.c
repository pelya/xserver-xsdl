/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/loadfont.c,v 1.2 1998/12/13 12:42:41 dawes Exp $ */

/* Maybe this file belongs in lib/font/fontfile/module/ ? */

#define LOADERDECLARATIONS
#include "loaderProcs.h"
#include "misc.h"
#include "xf86.h"

FontModule *FontModuleList = NULL;
static int numFontModules = 0;


static FontModule *
NewFontModule(void)
{
	FontModule *save = FontModuleList;
	int n;

	/* Sanity check */
	if (!FontModuleList)
		numFontModules = 0;

	n = numFontModules + 1;
	FontModuleList = xrealloc(FontModuleList, (n + 1) * sizeof(FontModule));
	if (FontModuleList == NULL) {
		FontModuleList = save;
		return NULL;
	} else {
		numFontModules++;
		FontModuleList[numFontModules].name = NULL;
		return FontModuleList + (numFontModules - 1);
	}
}

void
LoadFont(FontModule *f)
{
	FontModule *newfont;

	if (f == NULL)
		return;

	if (!(newfont = NewFontModule()))
		return;

	xf86MsgVerb(X_INFO, 2, "Loading font %s\n", f->name);

	newfont->name = f->name;
	newfont->initFunc = f->initFunc;
	newfont->module = f->module;
}

