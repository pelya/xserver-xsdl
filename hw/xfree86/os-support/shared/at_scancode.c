/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/at_scancode.c,v 1.1 2002/10/11 01:40:37 dawes Exp $ */

/*
 * Copyright (c) 2002 by The XFree86 Project, Inc.
 */

#include "xf86.h"
#include "xf86Xinput.h"
#include "xf86OSKbd.h"
#include "atKeynames.h"

Bool
ATScancode(InputInfoPtr pInfo, int *scanCode)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;

    switch (pKbd->scanPrefix) {
       case 0:
          switch (*scanCode) {
             case KEY_Prefix0:
             case KEY_Prefix1:
                  pKbd->scanPrefix = *scanCode;  /* special prefixes */
                  return TRUE;
          }
          break;
       case KEY_Prefix0:
          pKbd->scanPrefix = 0;
          switch (*scanCode) {
            case KEY_KP_7:        *scanCode = KEY_Home;      break;  /* curs home */
            case KEY_KP_8:        *scanCode = KEY_Up;        break;  /* curs up */
            case KEY_KP_9:        *scanCode = KEY_PgUp;      break;  /* curs pgup */
            case KEY_KP_4:        *scanCode = KEY_Left;      break;  /* curs left */
            case KEY_KP_5:        *scanCode = KEY_Begin;     break;  /* curs begin */
            case KEY_KP_6:        *scanCode = KEY_Right;     break;  /* curs right */
            case KEY_KP_1:        *scanCode = KEY_End;       break;  /* curs end */
            case KEY_KP_2:        *scanCode = KEY_Down;      break;  /* curs down */
            case KEY_KP_3:        *scanCode = KEY_PgDown;    break;  /* curs pgdown */
            case KEY_KP_0:        *scanCode = KEY_Insert;    break;  /* curs insert */
            case KEY_KP_Decimal:  *scanCode = KEY_Delete;    break;  /* curs delete */
            case KEY_Enter:       *scanCode = KEY_KP_Enter;  break;  /* keypad enter */
            case KEY_LCtrl:       *scanCode = KEY_RCtrl;     break;  /* right ctrl */
            case KEY_KP_Multiply: *scanCode = KEY_Print;     break;  /* print */
            case KEY_Slash:       *scanCode = KEY_KP_Divide; break;  /* keyp divide */
            case KEY_Alt:         *scanCode = KEY_AltLang;   break;  /* right alt */
            case KEY_ScrollLock:  *scanCode = KEY_Break;     break;  /* curs break */
            case 0x5b:            *scanCode = KEY_LMeta;     break;
            case 0x5c:            *scanCode = KEY_RMeta;     break;
            case 0x5d:            *scanCode = KEY_Menu;      break;
            case KEY_F3:          *scanCode = KEY_F13;       break;
            case KEY_F4:          *scanCode = KEY_F14;       break;
            case KEY_F5:          *scanCode = KEY_F15;       break;
            case KEY_F6:          *scanCode = KEY_F16;       break;
            case KEY_F7:          *scanCode = KEY_F17;       break;
            case KEY_KP_Plus:     *scanCode = KEY_KP_DEC;    break;
            case 0x2A:
            case 0x36:
	         return TRUE;
            default:
                 xf86MsgVerb(X_INFO, 4, "Unreported Prefix0 scancode: 0x%02x\n",
		             *scanCode);
                 *scanCode += 0x78;
          }
       break;
       case KEY_Prefix1: 
            pKbd->scanPrefix = (*scanCode == KEY_LCtrl) ? KEY_LCtrl : 0;
            return TRUE;
       case KEY_LCtrl:
            pKbd->scanPrefix = 0;
            if (*scanCode != KEY_NumLock)
                return TRUE;
            *scanCode = KEY_Pause;       /* pause */
    }
    return FALSE;
}
