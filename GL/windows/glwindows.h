#include <GL/gl.h>
#include <GL/glext.h>

#include <glxserver.h>
#include <glxext.h>

#include <windowstr.h>
#include <resource.h>
#include <GL/glxint.h>
#include <GL/glxtokens.h>
#include <scrnintstr.h>
#include <glxserver.h>
#include <glxscreens.h>
#include <glxdrawable.h>
#include <glxcontext.h>
#include <glxext.h>
#include <glxutil.h>
#include <glxscreens.h>
#include <GL/internal/glcore.h>
#include <stdlib.h>

#define WINDOWS_LEAN_AND_CLEAN
#include <windows.h>


typedef struct {
    unsigned enableDebug : 1;
    unsigned dumpPFD : 1;
    unsigned dumpHWND : 1;
    unsigned dumpDC : 1;
} glWinDebugSettingsRec, *glWinDebugSettingsPtr;
extern glWinDebugSettingsRec glWinDebugSettings;

typedef struct {
    int num_vis;
    __GLcontextModes *modes;
    void **priv;

    /* wrapped screen functions */
    RealizeWindowProcPtr RealizeWindow;
    UnrealizeWindowProcPtr UnrealizeWindow;
    CopyWindowProcPtr CopyWindow;
} glWinScreenRec;

extern glWinScreenRec glWinScreens[MAXSCREENS];

#define glWinGetScreenPriv(pScreen)  &glWinScreens[pScreen->myNum]
#define glWinScreenPriv(pScreen) glWinScreenRec *pScreenPriv = glWinGetScreenPriv(pScreen);

#if 1
#define GLWIN_DEBUG_MSG(msg, args...) if (glWinDebugSettings.enableDebug) ErrorF("%s:%d: " msg, __FUNCTION__, __LINE__, ##args )
#define GLWIN_DEBUG_MSG2(msg, args...) if (glWinDebugSettings.enableDebug) ErrorF(msg, ##args )
#else
#define GLWIN_DEBUG_MSG(a, ...)
#define GLWIN_DEBUG_MSG2(a, ...)
#endif

