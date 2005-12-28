/*
 * Copyright © 2005 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include "xgl.h"
#include "micmap.h"
#include "mipointer.h"
#include "fb.h"

#define DEAFULT_DDX_MODULE_NAME "xglx"

static char *ddxModuleName = DEAFULT_DDX_MODULE_NAME;

xglScreenInfoRec xglScreenInfo = {
    NULL, 0, 0, 0, 0,
    DEFAULT_GEOMETRY_DATA_TYPE,
    DEFAULT_GEOMETRY_USAGE,
    FALSE,
    XGL_DEFAULT_PBO_MASK,
    FALSE,
    FALSE
};

#ifdef GLXEXT
static Bool loadGlx = TRUE;
#endif

typedef struct _xglDDXFunc {
    void (*initOutput)	       (ScreenInfo   *pScreenInfo,
				int	     argc,
				char	     **argv);
    void (*initInput)	       (int	     argc,
				char	     **argv);
    Bool (*legalModifier)      (unsigned int key,
				DevicePtr    pDev);
    void (*processInputEvents) (void);
    void (*useMsg)	       (void);
    int  (*processArgument)    (int	     argc,
				char	     **argv,
				int	     i);
    void (*abort)	       (void);
    void (*giveUp)	       (void);
    void (*osVendorInit)       (void);
} xglDDXFuncRec;

static xglDDXFuncRec __ddxFunc;

#define SYMFUNC(name) ((void *) (name))
#define SYMVAR(name)  ((void *) &(name))

/*
 * The following table is used to make sure that all symbols required by
 * dynamically loaded modules are present in the main program. Add more symbols
 * as needed.
 */

void *symTab[] = {
    SYMFUNC (xglKbdCtrl),
    SYMFUNC (xglSetPixmapFormats),
    SYMVAR  (xglVisuals),

    SYMFUNC (mieqEnqueue),
    SYMFUNC (mieqInit),
    SYMFUNC (mieqProcessInputEvents),
    SYMFUNC (miPointerAbsoluteCursor),
    SYMFUNC (miRegisterPointerDevice),
    SYMFUNC (miPointerWarpCursor),
    SYMFUNC (miDCInitialize),
    SYMFUNC (miPointerAbsoluteCursor),
    SYMFUNC (miPointerUpdate),
    SYMFUNC (miRegisterRedirectBorderClipProc)
};

#define SYM(ptr, name) { (void **) &(ptr), (name) }

static Bool
xglEnsureDDXModule (void)
{

#ifdef XGL_MODULAR
    static void *ddxHandle = 0;
    static Bool status = TRUE;

    if (!status)
	return FALSE;

    if (!ddxHandle)
    {
	xglSymbolRec sym[] = {
	    SYM (__ddxFunc.initOutput,	       "InitOutput"),
	    SYM (__ddxFunc.initInput,	       "InitInput"),
	    SYM (__ddxFunc.legalModifier,      "LegalModifier"),
	    SYM (__ddxFunc.processInputEvents, "ProcessInputEvents"),
	    SYM (__ddxFunc.useMsg,	       "ddxUseMsg"),
	    SYM (__ddxFunc.processArgument,    "ddxProcessArgument"),
	    SYM (__ddxFunc.abort,	       "AbortDDX"),
	    SYM (__ddxFunc.giveUp,	       "ddxGiveUp"),
	    SYM (__ddxFunc.osVendorInit,       "OsVendorInit")
	};

	ddxHandle = xglLoadModule (ddxModuleName);
	if (!ddxHandle)
	    return (status = FALSE);

	if (!xglLookupSymbols (ddxHandle, sym, sizeof (sym) / sizeof (sym[0])))
	{
	    xglUnloadModule (ddxHandle);
	    ddxHandle = 0;

	    return (status = FALSE);
	}
    }

    return TRUE;
#else
    return FALSE;
#endif

}

void
InitOutput (ScreenInfo *pScreenInfo,
	    int	       argc,
	    char       **argv)
{
    (void) symTab;

    if (!xglEnsureDDXModule ())
	FatalError ("No DDX module loaded");

#ifdef GLXEXT
    if (loadGlx)
    {
	if (!xglLoadGLXModules ())
	    FatalError ("No GLX modules loaded");
    }
#endif

    (*__ddxFunc.initOutput) (pScreenInfo, argc, argv);
}

Bool
LegalModifier (unsigned int key,
	       DevicePtr    pDev)
{
    return (*__ddxFunc.legalModifier) (key, pDev);
}

void
ProcessInputEvents (void)
{
    (*__ddxFunc.processInputEvents) ();
}

void
InitInput (int  argc,
	   char **argv)
{
    if (!xglEnsureDDXModule ())
	FatalError ("No DDX module loaded");

    (*__ddxFunc.initInput) (argc, argv);
}

void
ddxUseMsg (void)
{
    ErrorF ("\nXgl usage:\n");
    ErrorF ("-ddx module            specify ddx module\n");

#ifdef GLXEXT
    ErrorF ("-noglx                 don't load glx extension\n");
#endif

    if (xglEnsureDDXModule ())
	(*__ddxFunc.useMsg) ();
}

#define LOPT(s, l) { (s), (l) }
#define OPT(s) LOPT (s, 0)

int
ddxProcessArgument (int  argc,
		    char **argv,
		    int  i)
{
    struct _option {
	char *name;
	int  length;
    } commonOption[] = {
	LOPT (":", 1), OPT ("-a"), OPT ("-ac"), OPT ("-audit"), OPT ("-auth"),
	OPT ("bc"), OPT ("-br"), OPT ("+bs"), OPT ("-bs"), OPT ("c"), OPT ("-c"),
	OPT ("-cc"), OPT ("-co"), OPT ("-core"), OPT ("-dpi"),
	OPT ("-deferglyphs"), OPT ("-f"), OPT ("-fc"), OPT ("-fn"), OPT ("-fp"),
	OPT ("-help"), OPT ("-nolisten"), OPT ("-noreset"), OPT ("-p"),
	OPT ("-pn"), OPT ("-nopn"), OPT ("r"), OPT ("-r"), OPT ("-s"),
	OPT ("-su"), OPT ("-t"), OPT ("-terminate"), OPT ("-to"), OPT ("-tst"),
	OPT ("v"), OPT ("-v"), OPT ("-wm"), OPT ("-x"), OPT ("-I"),
	LOPT ("tty", 3)
    };
    int skip, j;

    for (j = 0; j < sizeof (commonOption) / sizeof (commonOption[0]); j++)
    {
	if (commonOption[j].length)
	{
	    if (!strncmp (argv[i], commonOption[j].name, commonOption[j].length))
		return 0;
	}
	else
	{
	    if (!strcmp (argv[i], commonOption[j].name))
		return 0;
	}
    }

    if (!strcmp (argv[i], "-ddx"))
    {
	if ((i + 1) < argc)
	{
	    ddxModuleName = argv[i + 1];
	}
	else
	    return 1;

	return 2;
    }

#ifdef GLXEXT
    else if (!strcmp (argv[i], "-noglx"))
    {
	loadGlx = FALSE;
	return 1;
    }
#endif

    skip = xglProcessArgument (argc, argv, i);
    if (skip)
	return skip;

    if (xglEnsureDDXModule ())
	return (*__ddxFunc.processArgument) (argc, argv, i);

    return 0;
}

void
AbortDDX (void)
{
    if (xglEnsureDDXModule ())
	(*__ddxFunc.abort) ();
}

void
ddxGiveUp (void)
{
    if (xglEnsureDDXModule ())
	(*__ddxFunc.giveUp) ();
}

void
OsVendorInit (void)
{
    if (xglEnsureDDXModule ())
	(*__ddxFunc.osVendorInit) ();
}

void ddxInitGlobals(void)
{
}
