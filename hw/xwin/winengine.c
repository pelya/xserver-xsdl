/*
 *Copyright (C) 1994-2001 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Harold L Hunt II
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/winengine.c,v 1.5 2003/07/29 21:25:17 dawes Exp $ */

#include "win.h"


/*
 * External global variables
 */

extern const GUID _IID_IDirectDraw4;


/*
 * Detect engines supported by current Windows version
 * DirectDraw version and hardware
 */

void
winDetectSupportedEngines ()
{
  OSVERSIONINFO		osvi;

  /* Initialize the engine support flags */
  g_dwEnginesSupported = WIN_SERVER_SHADOW_GDI;

#if WIN_NATIVE_GDI_SUPPORT
  g_dwEnginesSupported |= WIN_SERVER_NATIVE_GDI;
#endif

  /* Get operating system version information */
  ZeroMemory (&osvi, sizeof (osvi));
  osvi.dwOSVersionInfoSize = sizeof (osvi);
  GetVersionEx (&osvi);

  /* Branch on platform ID */
  switch (osvi.dwPlatformId)
    {
    case VER_PLATFORM_WIN32_NT:
      /* Engine 4 is supported on NT only */
      ErrorF ("winDetectSupportedEngines - Windows NT/2000/XP\n");
      break;

    case VER_PLATFORM_WIN32_WINDOWS:
      /* Engine 4 is supported on NT only */
      ErrorF ("winDetectSupportedEngines - Windows 95/98/Me\n");
      break;
    }

  /* Do we have DirectDraw? */
  if (g_hmodDirectDraw != NULL)
    {
      LPDIRECTDRAW	lpdd = NULL;
      LPDIRECTDRAW4	lpdd4 = NULL;
      HRESULT		ddrval;

      /* Was the DirectDrawCreate function found? */
      if (g_fpDirectDrawCreate == NULL)
	{
	  /* No DirectDraw support */
	  return;
	}

      /* DirectDrawCreate exists, try to call it */
      /* Create a DirectDraw object, store the address at lpdd */
      ddrval = (*g_fpDirectDrawCreate) (NULL,
					(void**) &lpdd,
					NULL);
      if (FAILED (ddrval))
	{
	  /* No DirectDraw support */
	  ErrorF ("winDetectSupportedEngines - DirectDraw not installed\n");
	  return;
	}
      else
	{
	  /* We have DirectDraw */
	  ErrorF ("winDetectSupportedEngines - DirectDraw installed\n");
	  g_dwEnginesSupported |= WIN_SERVER_SHADOW_DD;

	  /* Allow PrimaryDD engine if NT */
	  if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
	    {
	      g_dwEnginesSupported |= WIN_SERVER_PRIMARY_DD;
	      ErrorF ("winDetectSupportedEngines - Allowing PrimaryDD\n");
	    }
	}
      
      /* Try to query for DirectDraw4 interface */
      ddrval = IDirectDraw_QueryInterface (lpdd,
					   &IID_IDirectDraw4,
					   (LPVOID*) &lpdd4);
      if (SUCCEEDED (ddrval))
	{
	  /* We have DirectDraw4 */
	  ErrorF ("winDetectSupportedEngines - DirectDraw4 installed\n");
	  g_dwEnginesSupported |= WIN_SERVER_SHADOW_DDNL;
	}

      /* Cleanup DirectDraw interfaces */
      if (lpdd4 != NULL)
	IDirectDraw_Release (lpdd4);
      if (lpdd != NULL)
	IDirectDraw_Release (lpdd);
    }

  ErrorF ("winDetectSupportedEngines - Returning, supported engines %08x\n",
	  g_dwEnginesSupported);
}


/*
 * Set the engine type, depending on the engines
 * supported for this screen, and whether the user
 * suggested an engine type
 */

Bool
winSetEngine (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  HDC			hdc;
  DWORD			dwBPP;

  /* Get a DC */
  hdc = GetDC (NULL);
  if (hdc == NULL)
    {
      ErrorF ("winSetEngine - Couldn't get an HDC\n");
      return FALSE;
    }

  /*
   * pScreenInfo->dwBPP may be 0 to indicate that the current screen
   * depth is to be used.  Thus, we must query for the current display
   * depth here.
   */
  dwBPP = GetDeviceCaps (hdc, BITSPIXEL);

  /* Release the DC */
  ReleaseDC (NULL, hdc);
  hdc = NULL;

  /* ShadowGDI is the only engine that supports windowed PseudoColor */
  if (dwBPP == 8 && !pScreenInfo->fFullScreen)
    {
      ErrorF ("winSetEngine - Windowed && PseudoColor => ShadowGDI\n");
      pScreenInfo->dwEngine = WIN_SERVER_SHADOW_GDI;

      /* Set engine function pointers */
      winSetEngineFunctionsShadowGDI (pScreen);
      return TRUE;
    }

  /* ShadowGDI is the only engine that supports Multi Window Mode */
  if (pScreenInfo->fMultiWindow)
    {
      ErrorF ("winSetEngine - Multi Window => ShadowGDI\n");
      pScreenInfo->dwEngine = WIN_SERVER_SHADOW_GDI;

      /* Set engine function pointers */
      winSetEngineFunctionsShadowGDI (pScreen);
      return TRUE;
    }

  /* If the user's choice is supported, we'll use that */
  if (g_dwEnginesSupported & pScreenInfo->dwEnginePreferred)
    {
      ErrorF ("winSetEngine - Using user's preference: %d\n",
	      pScreenInfo->dwEnginePreferred);
      pScreenInfo->dwEngine = pScreenInfo->dwEnginePreferred;

      /* Setup engine function pointers */
      switch (pScreenInfo->dwEngine)
	{
	case WIN_SERVER_SHADOW_GDI:
	  winSetEngineFunctionsShadowGDI (pScreen);
	  break;
	case WIN_SERVER_SHADOW_DD:
	  winSetEngineFunctionsShadowDD (pScreen);
	  break;
	case WIN_SERVER_SHADOW_DDNL:
	  winSetEngineFunctionsShadowDDNL (pScreen);
	  break;
	case WIN_SERVER_PRIMARY_DD:
	  winSetEngineFunctionsPrimaryDD (pScreen);
	  break;
	case WIN_SERVER_NATIVE_GDI:
	  winSetEngineFunctionsNativeGDI (pScreen);
	  break;
	default:
	  FatalError ("winSetEngine - Invalid engine type\n");
	}
      return TRUE;
    }

  /* ShadowDDNL has good performance, so why not */
  if (g_dwEnginesSupported & WIN_SERVER_SHADOW_DDNL)
    {
      ErrorF ("winSetEngine - Using Shadow DirectDraw NonLocking\n");
      pScreenInfo->dwEngine = WIN_SERVER_SHADOW_DDNL;

      /* Set engine function pointers */
      winSetEngineFunctionsShadowDDNL (pScreen);
      return TRUE;
    }

  /* ShadowDD is next in line */
  if (g_dwEnginesSupported & WIN_SERVER_SHADOW_DD)
    {
      ErrorF ("winSetEngine - Using Shadow DirectDraw\n");
      pScreenInfo->dwEngine = WIN_SERVER_SHADOW_DD;

      /* Set engine function pointers */
      winSetEngineFunctionsShadowDD (pScreen);
      return TRUE;
    }

  /* ShadowGDI is next in line */
  if (g_dwEnginesSupported & WIN_SERVER_SHADOW_GDI)
    {
      ErrorF ("winSetEngine - Using Shadow GDI DIB\n");
      pScreenInfo->dwEngine = WIN_SERVER_SHADOW_GDI;

      /* Set engine function pointers */
      winSetEngineFunctionsShadowGDI (pScreen);
      return TRUE;
    }

  return TRUE;
}


/*
 * Get procedure addresses for DirectDrawCreate and DirectDrawCreateClipper
 */

Bool
winGetDDProcAddresses ()
{
  Bool			fReturn = TRUE;
  
  /* Load the DirectDraw library */
  g_hmodDirectDraw = LoadLibraryEx ("ddraw.dll", NULL, 0);
  if (g_hmodDirectDraw == NULL)
    {
      ErrorF ("winGetDDProcAddresses - Could not load ddraw.dll\n");
      fReturn = TRUE;
      goto winGetDDProcAddresses_Exit;
    }

  /* Try to get the DirectDrawCreate address */
  g_fpDirectDrawCreate = GetProcAddress (g_hmodDirectDraw,
					 "DirectDrawCreate");
  if (g_fpDirectDrawCreate == NULL)
    {
      ErrorF ("winGetDDProcAddresses - Could not get DirectDrawCreate "
	      "address\n");
      fReturn = TRUE;
      goto winGetDDProcAddresses_Exit;
    }

  /* Try to get the DirectDrawCreateClipper address */
  g_fpDirectDrawCreateClipper = GetProcAddress (g_hmodDirectDraw,
						"DirectDrawCreateClipper");
  if (g_fpDirectDrawCreateClipper == NULL)
    {
      ErrorF ("winGetDDProcAddresses - Could not get "
	      "DirectDrawCreateClipper address\n");
      fReturn = FALSE;
      goto winGetDDProcAddresses_Exit;
    }

  /*
   * Note: Do not unload ddraw.dll here.  Do it in GiveUp
   */

 winGetDDProcAddresses_Exit:
  /* Unload the DirectDraw library if we failed to initialize */
  if (!fReturn && g_hmodDirectDraw != NULL)
    {
      FreeLibrary (g_hmodDirectDraw);
      g_hmodDirectDraw = NULL;
    }
  
  return fReturn;
}
