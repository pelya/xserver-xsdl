/*
 * Xephyr - A kdrive X server thats runs in a host X window.
 *          Authored by Matthew Allum <mallum@o-hand.com>
 * 
 * Copyright © 2004 Nokia 
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Nokia not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission. Nokia makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * NOKIA DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL NOKIA BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "ephyr.h"

extern Window EphyrPreExistingHostWin;
extern Bool   EphyrWantGrayScale;

void
InitCard (char *name)
{
    KdCardAttr	attr;

    EPHYR_DBG("mark");


    KdCardInfoAdd (&ephyrFuncs, &attr, 0);
}

void
InitOutput (ScreenInfo *pScreenInfo, int argc, char **argv)
{
  KdInitOutput (pScreenInfo, argc, argv);
}

void
InitInput (int argc, char **argv)
{
  KdInitInput (&EphyrMouseFuncs, &EphyrKeyboardFuncs);
}

void
ddxUseMsg (void)
{
  KdUseMsg();

  ErrorF("\nXephyr Option Usage:\n");
  ErrorF("-parent XID   Use existing window as Xephyr root win\n");
  ErrorF("-host-cursor  Re-use exisiting X host server cursor\n");
  ErrorF("-fullscreen   Attempt to run Xephyr fullscreen\n");
  ErrorF("-grayscale    Simulate 8bit grayscale\n");
  ErrorF("-fakexa	Simulate acceleration using software rendering\n");
  ErrorF("\n");

  exit(1);
}

int
ddxProcessArgument (int argc, char **argv, int i)
{
  EPHYR_DBG("mark");

  if (!strcmp (argv[i], "-parent"))
    {
      if(i+1 < argc) 
	{
	  hostx_use_preexisting_window(strtol(argv[i+1], NULL, 0));
	  return 2;
	} 
      
      UseMsg();
      exit(1);
    }
  else if (!strcmp (argv[i], "-host-cursor"))
    {
      hostx_use_host_cursor();
      return 1;
    }
  else if (!strcmp (argv[i], "-fullscreen"))
    {
      hostx_use_fullscreen();
      return 1;
    }
  else if (!strcmp (argv[i], "-grayscale"))
    {
      EphyrWantGrayScale = 1;      
      return 1;
    }
  else if (!strcmp (argv[i], "-fakexa"))
    {
      ephyrFuncs.initAccel = ephyrDrawInit;
      ephyrFuncs.enableAccel = ephyrDrawEnable;
      ephyrFuncs.disableAccel = ephyrDrawDisable;
      ephyrFuncs.finiAccel = ephyrDrawFini;
      return 1;
    }
  else if (argv[i][0] == ':')
    {
      hostx_set_display_name(argv[i]);
    }

  return KdProcessArgument (argc, argv, i);
}

void
OsVendorInit (void)
{
  EPHYR_DBG("mark");

  if (hostx_want_host_cursor())
    {
      ephyrFuncs.initCursor   = &ephyrCursorInit;
      ephyrFuncs.enableCursor = &ephyrCursorEnable;
    }

  KdOsInit (&EphyrOsFuncs);
}

/* 'Fake' cursor stuff, could be improved */

static Bool
ephyrRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor)
{
  return TRUE;
}

static Bool
ephyrUnrealizeCursor(ScreenPtr pScreen, CursorPtr pCursor)
{
  return TRUE;
}

static void
ephyrSetCursor(ScreenPtr pScreen, CursorPtr pCursor, int x, int y)
{
  ;
}

static void
ephyrMoveCursor(ScreenPtr pScreen, int x, int y)
{
  ;
}

miPointerSpriteFuncRec EphyrPointerSpriteFuncs = {
	ephyrRealizeCursor,
	ephyrUnrealizeCursor,
	ephyrSetCursor,
	ephyrMoveCursor,
};


Bool
ephyrCursorInit(ScreenPtr pScreen)
{
  miPointerInitialize(pScreen, &EphyrPointerSpriteFuncs,
		      &kdPointerScreenFuncs, FALSE);

  return TRUE;
}

void
ephyrCursorEnable(ScreenPtr pScreen)
{
  ;
}

KdCardFuncs ephyrFuncs = {
    ephyrCardInit,	    /* cardinit */
    ephyrScreenInit,	    /* scrinit */
    ephyrInitScreen,	    /* initScreen */
    ephyrFinishInitScreen,  /* finishInitScreen */
    ephyrCreateResources,   /* createRes */
    ephyrPreserve,	    /* preserve */
    ephyrEnable,	    /* enable */
    ephyrDPMS,		    /* dpms */
    ephyrDisable,	    /* disable */
    ephyrRestore,	    /* restore */
    ephyrScreenFini,	    /* scrfini */
    ephyrCardFini,	    /* cardfini */
    
    0,	                    /* initCursor */
    0,          	    /* enableCursor */
    0,			    /* disableCursor */
    0,			    /* finiCursor */
    0,			    /* recolorCursor */
    
    0,			    /* initAccel */
    0,			    /* enableAccel */
    0,			    /* disableAccel */
    0,			    /* finiAccel */
    
    ephyrGetColors,    	    /* getColors */
    ephyrPutColors,	    /* putColors */
};
