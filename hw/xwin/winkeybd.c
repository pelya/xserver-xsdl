/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
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
 * Authors:	Dakshinamurthy Karra
 *		Suhaib M Siddiqi
 *		Peter Busch
 *		Harold L Hunt II
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/winkeybd.c,v 1.12 2002/10/17 08:18:22 alanh Exp $ */


#include "win.h"
#include "winkeybd.h"
#include "winconfig.h"
 
#ifdef XKB
#define XKB_IN_SERVER
#include "XKBsrv.h"
#endif

static Bool g_winKeyState[NUM_KEYCODES];


#if WIN_NEW_KEYBOARD_SUPPORT

const unsigned int MaxKeysPerKey = 4;

void
winProcessKeyEvent (DWORD dwVirtualKey, DWORD dwKeyData)
{
  Bool			fDown = ((dwKeyData & 0x80000000) == 0);
  winKeyEventsRec	kerEvent;
  int			i;

  /* Get the key events */
  kerEvent = winTranslateKey (dwVirtualKey, dwKeyData);

  if (kerEvent.dwReleaseModifiers & WIN_MOD_LCONTROL)
    winSendKeyEvent (XK_Control_L, FALSE);
  if (kerEvent.dwReleaseModifiers & WIN_MOD_RCONTROL)
    winSendKeyEvent (XK_Control_R, FALSE);
  if (kerEvent.dwReleaseModifiers & WIN_MOD_LALT)
    winSendKeyEvent (XK_Alt_L, FALSE);
  if (kerEvent.dwReleaseModifiers & WIN_MOD_RALT)
    winSendKeyEvent (XK_Alt_R, FALSE);
  
  for (i = 0; kerEvent.dwXKeycodes[i] != XK_VoidSymbol; ++i)
    winSendKeyEvent (kerEvent.dwXKeycodes[i], fDown);
  
  if (kerEvent.dwReleaseModifiers & WIN_MOD_LCONTROL)
    winSendKeyEvent (XK_Control_L, FALSE);
  if (kerEvent.dwReleaseModifiers & WIN_MOD_RCONTROL)
    winSendKeyEvent (XK_Control_R, TRUE);
  if (kerEvent.dwReleaseModifiers & WIN_MOD_LALT)
    winSendKeyEvent (XK_Alt_L, FALSE);
  if (kerEvent.dwReleaseModifiers & WIN_MOD_RALT)
    winSendKeyEvent (XK_Alt_R, TRUE);
  
}


winKeyEventsRec
winTranslateKey (DWORD dwVirtualKey, DWORD dwKeyData)
{
  winKeyEventsRec	kerEvents;
  Bool			fExtended = ((HIWORD (dwKeyData) & KF_EXTENDED) != 0);
  int			i;
  DWORD			dwNumEvents = 0;
  BYTE			bKeyboardState[256];
  int			iReturn;
  unsigned char		cAscii[4];

  /* Remap extended modifiers to the right version of that key.  */
  if (fExtended)
    {
      switch (dwVirtualKey)
	{
	case VK_MENU:
	  dwVirtualKey = VK_RMENU;
	  break;

	case VK_CONTROL:
	  dwVirtualKey = VK_RCONTROL;
	  break;
	}
    }

  /* Initialize the modifiers to release flag */
  kerEvents.dwReleaseModifiers = 0;
   
  /* Look up the current virtual key code in the translation table */
  for (i = 0; i < g_winKeymapEntries; ++i)
    {
      /* Did we find a mapping? */
      if (winKeymap[i].dwVirtualKey == dwVirtualKey)
	{
	  /* Mapping found, we have at least one event now */
	  kerEvents.dwXKeycodes[dwNumEvents] = winKeymap[i].dwXKey;
	  break;
	}
    }

  
  /* Break out early, if we found the key in the translation table */
  if (dwNumEvents != 0)
    {
      /* Terminate the last of the key events with a void symbol */
      kerEvents.dwXKeycodes[dwNumEvents] = XK_VoidSymbol;
      return kerEvents;
    }
  
  /* Get the state of all keyboard keys */
  GetKeyboardState (bKeyboardState);

  /* Try to convert the key to ASCII */
  iReturn = ToAscii (dwVirtualKey, 0, bKeyboardState, (WORD *) cAscii, 0);
  
  /*
   * Left Control and Alt pressed, combined with a valid result
   * from ToAscii means that we have found the Windows version of AltGr.
   */
  if ((bKeyboardState[VK_MENU] & 0x80) && (bKeyboardState[VK_CONTROL] & 0x80)
      && (iReturn >= 1)
      && (((cAscii[0] >= 32) && (cAscii[0] <= 126))
	  || (cAscii[0] >= 160)))
    {
      /* These three calls will return 0 on Windows 95/98/Me */
      if ((GetKeyState (VK_LCONTROL) & KF_UP))
	kerEvents.dwReleaseModifiers |= WIN_MOD_LCONTROL;
      if ((GetKeyState (VK_LMENU) & KF_UP))
	kerEvents.dwReleaseModifiers |= WIN_MOD_LALT;
      if ((GetKeyState (VK_RMENU) & KF_UP))
	kerEvents.dwReleaseModifiers |= WIN_MOD_RALT;

      /* Windows 95/98/Me handling - pop all of them */
      if (kerEvents.dwReleaseModifiers == 0)
	kerEvents.dwReleaseModifiers
	  = WIN_MOD_LCONTROL | WIN_MOD_LALT | WIN_MOD_RALT;

      /* Copy the string of character events */
      for (i = 0; i < iReturn; ++i)
	kerEvents.dwXKeycodes[dwNumEvents++] = cAscii[i];
    }

  /* Handle non Ctrl+Alt cases*/
  if (dwNumEvents == 0)
    {
      bKeyboardState[VK_CONTROL] = 0;
      bKeyboardState[VK_LCONTROL] = 0;
      bKeyboardState[VK_RCONTROL] = 0;
      
      iReturn = ToAscii (dwVirtualKey, 0, bKeyboardState, (WORD *)cAscii, 0);
      if (iReturn < 0)
	{
	  switch (cAscii[0])
	    {
	    case '`':
	      kerEvents.dwXKeycodes[dwNumEvents++] = XK_dead_grave;
	      break;
	      
	    case '\'':
	      kerEvents.dwXKeycodes[dwNumEvents++] = XK_dead_acute;
	      break;
	      
	    case '~':
	      kerEvents.dwXKeycodes[dwNumEvents++] = XK_dead_tilde;
	      break;
	      
	    case '^':
	      kerEvents.dwXKeycodes[dwNumEvents++] = XK_dead_circumflex;
	      break;
	    }
	}
      
      /* Send what we've got if its a printable character */
      if (iReturn >= 1)
	for (i = 0; i < iReturn; ++i)
	  kerEvents.dwXKeycodes[dwNumEvents++] = cAscii[i];
    }

  
  /* Terminate the last of the key events with a void symbol */
  kerEvents.dwXKeycodes[dwNumEvents] = XK_VoidSymbol;
  return kerEvents;
}


#else /* WIN_NEW_KEYBOARD_SUPPORT */


/* 
 * Translate a Windows WM_[SYS]KEY(UP/DOWN) message
 * into an ASCII scan code.
 *
 * We do this ourselves, rather than letting Windows handle it,
 * because Windows tends to munge the handling of special keys,
 * like AltGr on European keyboards.
 */

void
winTranslateKey (WPARAM wParam, LPARAM lParam, int *piScanCode)
{
  int		iKeyFixup = g_iKeyMap[wParam * WIN_KEYMAP_COLS + 1];
  int		iKeyFixupEx = g_iKeyMap[wParam * WIN_KEYMAP_COLS + 2];

  /* Branch on special extended, special non-extended, or normal key */
  if ((HIWORD (lParam) & KF_EXTENDED) && iKeyFixupEx)
    *piScanCode = iKeyFixupEx;
  else if (iKeyFixup)
    *piScanCode = iKeyFixup;
  else
    *piScanCode = LOBYTE (HIWORD (lParam));
}

#endif /* WIN_NEW_KEYBOARD_SUPPORT */


/*
 * We call this function from winKeybdProc when we are
 * initializing the keyboard.
 */

void
winGetKeyMappings (KeySymsPtr pKeySyms, CARD8 *pModMap)
{
  int			i;
  KeySym		*pMap = map;
  KeySym		*pKeySym;

  /*
   * Initialize all key states to up... which may not be true
   * but it is close enough.
   */
  ZeroMemory (g_winKeyState, sizeof (g_winKeyState[0]) * NUM_KEYCODES);

  /* MAP_LENGTH is defined in Xserver/include/input.h to be 256 */
  for (i = 0; i < MAP_LENGTH; i++)
    pModMap[i] = NoSymbol;  /* make sure it is restored */

  /* Loop through all valid entries in the key symbol table */
  for (pKeySym = pMap, i = MIN_KEYCODE;
       i < (MIN_KEYCODE + NUM_KEYCODES);
       i++, pKeySym += GLYPHS_PER_KEY)
    {
      switch (*pKeySym)
	{
	case XK_Shift_L:
	case XK_Shift_R:
	  pModMap[i] = ShiftMask;
	  break;

	case XK_Control_L:
	case XK_Control_R:
	  pModMap[i] = ControlMask;
	  break;

	case XK_Caps_Lock:
	  pModMap[i] = LockMask;
	  break;

	case XK_Alt_L:
	case XK_Alt_R:
	  pModMap[i] = AltMask;
	  break;

#if !WIN_NEW_KEYBOARD_SUPPORT
	case XK_Num_Lock:
	  pModMap[i] = NumLockMask;
	  break;

	case XK_Scroll_Lock:
	  pModMap[i] = ScrollLockMask;
	  break;

	/* Hirigana/Katakana toggle */
	case XK_Kana_Lock:
	case XK_Kana_Shift:
	  pModMap[i] = KanaMask;
	  break;
#endif

	/* alternate toggle for multinational support */
	case XK_Mode_switch:
	  pModMap[i] = AltLangMask;
	  break;
	}
    }

  pKeySyms->map        = (KeySym *) pMap;
  pKeySyms->mapWidth   = GLYPHS_PER_KEY;
  pKeySyms->minKeyCode = MIN_KEYCODE;
  pKeySyms->maxKeyCode = MAX_KEYCODE;
}


/* Ring the keyboard bell (system speaker on PCs) */
void
winKeybdBell (int iPercent, DeviceIntPtr pDeviceInt,
	      pointer pCtrl, int iClass)
{
  /*
   * We can't use Beep () here because it uses the PC speaker
   * on NT/2000.  MessageBeep (MB_OK) will play the default system
   * sound on systems with a sound card or it will beep the PC speaker
   * on systems that do not have a sound card.
   */
  MessageBeep (MB_OK);
}


/* Change some keyboard configuration parameters */
void
winKeybdCtrl (DeviceIntPtr pDevice, KeybdCtrl *pCtrl)
{

}


/* 
 * See Porting Layer Definition - p. 18
 * winKeybdProc is known as a DeviceProc.
 */

int
winKeybdProc (DeviceIntPtr pDeviceInt, int iState)
{
  KeySymsRec		keySyms;
  CARD8 		modMap[MAP_LENGTH];
  DevicePtr		pDevice = (DevicePtr) pDeviceInt;
#ifdef XKB
  XkbComponentNamesRec names;
#endif

  switch (iState)
    {
    case DEVICE_INIT:
      winConfigKeyboard (pDeviceInt);

      winGetKeyMappings (&keySyms, modMap);

#ifdef XKB
      /* FIXME: Maybe we should use winGetKbdLeds () here? */
      defaultKeyboardControl.leds = g_winInfo.keyboard.leds;
#else
      defaultKeyboardControl.leds = g_winInfo.keyboard.leds;
#endif

#ifdef XKB
      if (g_winInfo.xkb.disable) 
	{
#endif
	  InitKeyboardDeviceStruct (pDevice,
				    &keySyms,
				    modMap,
				    winKeybdBell,
				    winKeybdCtrl);
#ifdef XKB
	} 
      else 
	{

	  if (XkbInitialMap) 
	    {
	      names.keymap = XkbInitialMap;
	      names.keycodes = NULL;
	      names.types = NULL;
	      names.compat = NULL;
	      names.symbols = NULL;
	      names.geometry = NULL;
	    } 
	  else 
	    {
	      names.keymap = g_winInfo.xkb.keymap;
	      names.keycodes = g_winInfo.xkb.keycodes;
	      names.types = g_winInfo.xkb.types;
	      names.compat = g_winInfo.xkb.compat;
	      names.symbols = g_winInfo.xkb.symbols;
	      names.geometry = g_winInfo.xkb.geometry;
	    }

	  ErrorF("Rules = \"%s\" Model = \"%s\" Layout = \"%s\""
		 " Variant = \"%s\" Options = \"%s\"\n",
		 g_winInfo.xkb.rules, g_winInfo.xkb.model,
		 g_winInfo.xkb.layout, g_winInfo.xkb.variant,
		 g_winInfo.xkb.options);
          
	  XkbSetRulesDflts (g_winInfo.xkb.rules, g_winInfo.xkb.model, 
			    g_winInfo.xkb.layout, g_winInfo.xkb.variant, 
			    g_winInfo.xkb.options);
	  XkbInitKeyboardDeviceStruct (pDeviceInt, &names, &keySyms,
				       modMap, winKeybdBell, winKeybdCtrl);
	}
#endif
      break;
    case DEVICE_ON: 
      pDevice->on = TRUE;
      break;

    case DEVICE_CLOSE:
    case DEVICE_OFF: 
      pDevice->on = FALSE;
      break;
    }

  return Success;
}


/*
 * Detect current mode key states upon server startup.
 *
 * Simulate a press and release of any key that is currently
 * toggled.
 */

void
winInitializeModeKeyStates (void)
{
#if !WIN_NEW_KEYBOARD_SUPPORT
  /* Restore NumLock */
  if (GetKeyState (VK_NUMLOCK) & 0x0001)
    {
      winSendKeyEvent (KEY_NumLock, TRUE);
      winSendKeyEvent (KEY_NumLock, FALSE);
    }

  /* Restore CapsLock */
  if (GetKeyState (VK_CAPITAL) & 0x0001)
    {
      winSendKeyEvent (KEY_CapsLock, TRUE);
      winSendKeyEvent (KEY_CapsLock, FALSE);
    }

  /* Restore ScrollLock */
  if (GetKeyState (VK_SCROLL) & 0x0001)
    {
      winSendKeyEvent (KEY_ScrollLock, TRUE);
      winSendKeyEvent (KEY_ScrollLock, FALSE);
    }

  /* Restore KanaLock */
  if (GetKeyState (VK_KANA) & 0x0001)
    {
      winSendKeyEvent (KEY_HKTG, TRUE);
      winSendKeyEvent (KEY_HKTG, FALSE);
    }
#endif
}


/*
 * We have to store the last state of each mode
 * key before we lose the keyboard focus.
 */

void
winStoreModeKeyStates (ScreenPtr pScreen)
{
#if !WIN_NEW_KEYBOARD_SUPPORT
  winScreenPriv(pScreen);

  /* Initialize all mode key states to off */
  pScreenPriv->dwModeKeyStates = 0x0L;

  pScreenPriv->dwModeKeyStates |= 
    (GetKeyState (VK_NUMLOCK) & 0x0001) << NumLockMapIndex;

  pScreenPriv->dwModeKeyStates |=
    (GetKeyState (VK_SCROLL) & 0x0001) << ScrollLockMapIndex;

  pScreenPriv->dwModeKeyStates |=
    (GetKeyState (VK_CAPITAL) & 0x0001) << LockMapIndex;

  pScreenPriv->dwModeKeyStates |=
    (GetKeyState (VK_KANA) & 0x0001) << KanaMapIndex;
#endif
}


/*
 * Upon regaining the keyboard focus we must
 * resynchronize our internal mode key states
 * with the actual state of the keys.
 */

void
winRestoreModeKeyStates (ScreenPtr pScreen)
{
#if !WIN_NEW_KEYBOARD_SUPPORT
  winScreenPriv(pScreen);
  DWORD			dwKeyState;

  /* 
   * NOTE: The C XOR operator, ^, will not work here because it is
   * a bitwise operator, not a logical operator.  C does not
   * have a logical XOR operator, so we use a macro instead.
   */

  /* Has the key state changed? */
  dwKeyState = GetKeyState (VK_NUMLOCK) & 0x0001;
  if (WIN_XOR (pScreenPriv->dwModeKeyStates & NumLockMask, dwKeyState))
    {
      winSendKeyEvent (KEY_NumLock, TRUE);
      winSendKeyEvent (KEY_NumLock, FALSE);
    }

  /* Has the key state changed? */
  dwKeyState = GetKeyState (VK_CAPITAL) & 0x0001;
  if (WIN_XOR (pScreenPriv->dwModeKeyStates & LockMask, dwKeyState))
    {
      winSendKeyEvent (KEY_CapsLock, TRUE);
      winSendKeyEvent (KEY_CapsLock, FALSE);
    }

  /* Has the key state changed? */
  dwKeyState = GetKeyState (VK_SCROLL) & 0x0001;
  if (WIN_XOR (pScreenPriv->dwModeKeyStates & ScrollLockMask, dwKeyState))
    {
      winSendKeyEvent (KEY_ScrollLock, TRUE);
      winSendKeyEvent (KEY_ScrollLock, FALSE);
    }

  /* Has the key state changed? */
  dwKeyState = GetKeyState (VK_KANA) & 0x0001;
  if (WIN_XOR (pScreenPriv->dwModeKeyStates & KanaMask, dwKeyState))
    {
      winSendKeyEvent (KEY_HKTG, TRUE);
      winSendKeyEvent (KEY_HKTG, FALSE);
    }
#endif
}


#if !WIN_NEW_KEYBOARD_SUPPORT
/*
 * Look for the lovely fake Control_L press/release generated by Windows
 * when AltGr is pressed/released on a non-U.S. keyboard.
 */

Bool
winIsFakeCtrl_L (UINT message, WPARAM wParam, LPARAM lParam)
{
  MSG		msgNext;
  LONG		lTime;
  Bool		fReturn;

  /*
   * Fake Ctrl_L presses will be followed by an Alt_R keypress
   * with the same timestamp as the Ctrl_L press.
   */
  if (message == WM_KEYDOWN
      && wParam == VK_CONTROL
      && (HIWORD (lParam) & KF_EXTENDED) == 0)
    {
      /* Got a Ctrl_L press */

      /* Get time of current message */
      lTime = GetMessageTime ();
      			
      /* Look for fake Ctrl_L preceeding an Alt_R press. */
      fReturn = PeekMessage (&msgNext, NULL,
			     WM_KEYDOWN, WM_KEYDOWN,
			     PM_NOREMOVE);

      /* Is next press an Alt_R with the same timestamp? */
      if (fReturn && msgNext.wParam == VK_MENU
	  && msgNext.time == lTime
	  && (HIWORD (msgNext.lParam) & KF_EXTENDED))
	{
	  /* 
	   * Next key press is Alt_R with same timestamp as current
	   * Ctrl_L message.  Therefore, this Ctrl_L press is a fake
	   * event, so discard it.
	   */
	  return TRUE;
	}
    }

  /* 
   * Fake Ctrl_L releases will be followed by an Alt_R release
   * with the same timestamp as the Ctrl_L release.
   */
  if ((message == WM_KEYUP || message == WM_SYSKEYUP)
      && wParam == VK_CONTROL
      && (HIWORD (lParam) & KF_EXTENDED) == 0)
    {
      /* Got a Ctrl_L release */

      /* Get time of current message */
      lTime = GetMessageTime ();

      /* Look for fake Ctrl_L release preceeding an Alt_R release. */
      fReturn = PeekMessage (&msgNext, NULL,
			     WM_KEYUP, WM_SYSKEYUP, 
			     PM_NOREMOVE);

      /* Is next press an Alt_R with the same timestamp? */
      if (fReturn
	  && (msgNext.message == WM_KEYUP
	      || msgNext.message == WM_SYSKEYUP)
	  && msgNext.wParam == VK_MENU
	  && msgNext.time == lTime
	  && (HIWORD (msgNext.lParam) & KF_EXTENDED))
	{
	  /*
	   * Next key release is Alt_R with same timestamp as current
	   * Ctrl_L message. Therefore, this Ctrl_L release is a fake
	   * event, so discard it.
	   */
	  return TRUE;
	}
    }
  
  /* Not a fake control left press/release */
  return FALSE;
}
#endif /* WIN_NEW_KEYBOARD_SUPPORT */


/*
 * Lift any modifier keys that are pressed
 */

void
winKeybdReleaseKeys ()
{
#if !WIN_NEW_KEYBOARD_SUPPORT
#if 0 /* Old function that just pops modifiers */
  /* Verify that the mi input system has been initialized */
  if (g_fdMessageQueue == WIN_FD_INVALID)
    return;

  winSendKeyEvent (KEY_Alt, FALSE);
  winSendKeyEvent (KEY_AltLang, FALSE);
  winSendKeyEvent (KEY_LCtrl, FALSE);
  winSendKeyEvent (KEY_RCtrl, FALSE);
  winSendKeyEvent (KEY_ShiftL, FALSE);
  winSendKeyEvent (KEY_ShiftR, FALSE);
#else /* New function that pops all keys */
  int				i;

  /* Verify that the mi input system has been initialized */
  if (g_fdMessageQueue == WIN_FD_INVALID)
    return;

  /* Pop any pressed keys */
  for (i = 0; i < NUM_KEYCODES; ++i)
    {
      if (g_winKeyState[i]) winSendKeyEvent (i, FALSE);
    }
#endif
#endif
}


/*
 * Take a raw X key code and send an up or down event for it.
 *
 * Thanks to VNC for inspiration, though it is a simple function.
 */

void
winSendKeyEvent (DWORD dwKey, Bool fDown)
{
  xEvent			xCurrentEvent;

  /*
   * When alt-tabing between screens we can get phantom key up messages
   * Here we only pass them through it we think we should!
   */
  if (g_winKeyState[dwKey] == FALSE && fDown == FALSE) return;

  /* Update the keyState map */
  g_winKeyState[dwKey] = fDown;
  
  ZeroMemory (&xCurrentEvent, sizeof (xCurrentEvent));

  xCurrentEvent.u.u.type = fDown ? KeyPress : KeyRelease;
  xCurrentEvent.u.keyButtonPointer.time =
    g_c32LastInputEventTime = GetTickCount ();
  xCurrentEvent.u.u.detail = dwKey + MIN_KEYCODE;
  mieqEnqueue (&xCurrentEvent);
}
