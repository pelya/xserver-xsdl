/*
 * mipointer.h
 *
 */

/* $Xorg: mipointer.h,v 1.4 2001/02/09 02:05:21 xorgcvs Exp $ */

/*

Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
*/

typedef struct _miPointerSpriteFuncRec {
    Bool	(*RealizeCursor)();	/* pScreen, pCursor */
    Bool	(*UnrealizeCursor)();	/* pScreen, pCursor */
    void	(*SetCursor)();		/* pScreen, pCursor, x, y */
    void	(*MoveCursor)();	/* pScreen, x, y */
} miPointerSpriteFuncRec, *miPointerSpriteFuncPtr;

typedef struct _miPointerScreenFuncRec {
    Bool	(*CursorOffScreen)();	/* ppScreen, px, py */
    void	(*CrossScreen)();	/* pScreen, entering */
    void	(*WarpCursor)();	/* pScreen, x, y */
    void	(*EnqueueEvent)();	/* xEvent */
    void	(*NewEventScreen)();	/* pScreen */
} miPointerScreenFuncRec, *miPointerScreenFuncPtr;

extern Bool miDCInitialize(
#if NeedFunctionPrototypes
    ScreenPtr /*pScreen*/,
    miPointerScreenFuncPtr /*screenFuncs*/
#endif
);

extern Bool miPointerInitialize(
#if NeedFunctionPrototypes
    ScreenPtr /*pScreen*/,
    miPointerSpriteFuncPtr /*spriteFuncs*/,
    miPointerScreenFuncPtr /*screenFuncs*/,
    Bool /*waitForUpdate*/
#endif
);

extern void miPointerWarpCursor(
#if NeedFunctionPrototypes
    ScreenPtr /*pScreen*/,
    int /*x*/,
    int /*y*/
#endif
);

extern int miPointerGetMotionBufferSize(
#if NeedFunctionPrototypes
    void
#endif
);

extern int miPointerGetMotionEvents(
#if NeedFunctionPrototypes
    DeviceIntPtr /*pPtr*/,
    xTimecoord * /*coords*/,
    unsigned long /*start*/,
    unsigned long /*stop*/,
    ScreenPtr /*pScreen*/
#endif
);

extern void miPointerUpdate(
#if NeedFunctionPrototypes
    void
#endif
);

extern void miPointerDeltaCursor(
#if NeedFunctionPrototypes
    int /*dx*/,
    int /*dy*/,
    unsigned long /*time*/
#endif
);

extern void miPointerAbsoluteCursor(
#if NeedFunctionPrototypes
    int /*x*/,
    int /*y*/,
    unsigned long /*time*/
#endif
);

extern void miPointerPosition(
#if NeedFunctionPrototypes
    int * /*x*/,
    int * /*y*/
#endif
);

extern void miRegisterPointerDevice(
#if NeedFunctionPrototypes
    ScreenPtr /*pScreen*/,
    DevicePtr /*pDevice*/
#endif
);
