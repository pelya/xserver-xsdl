/*
 * Copyright (c) 2002-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

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
          if (!xf86IsPc98()) {
            switch (*scanCode) {
             case 0x59: *scanCode = KEY_0x59; break;
             case 0x5a: *scanCode = KEY_0x5A; break;
             case 0x5b: *scanCode = KEY_0x5B; break;
             case 0x5c: *scanCode = KEY_KP_Equal; break; /* Keypad Equal */
             case 0x5d: *scanCode = KEY_0x5D; break;
             case 0x5e: *scanCode = KEY_0x5E; break;
             case 0x5f: *scanCode = KEY_0x5F; break;
             case 0x62: *scanCode = KEY_0x62; break;
             case 0x63: *scanCode = KEY_0x63; break;
             case 0x64: *scanCode = KEY_0x64; break;
             case 0x65: *scanCode = KEY_0x65; break;
             case 0x66: *scanCode = KEY_0x66; break;
             case 0x67: *scanCode = KEY_0x67; break;
             case 0x68: *scanCode = KEY_0x68; break;
             case 0x69: *scanCode = KEY_0x69; break;
             case 0x6a: *scanCode = KEY_0x6A; break;
             case 0x6b: *scanCode = KEY_0x6B; break;
             case 0x6c: *scanCode = KEY_0x6C; break;
             case 0x6d: *scanCode = KEY_0x6D; break;
             case 0x6e: *scanCode = KEY_0x6E; break;
             case 0x6f: *scanCode = KEY_0x6F; break;
             case 0x70: *scanCode = KEY_0x70; break;
             case 0x71: *scanCode = KEY_0x71; break;
             case 0x72: *scanCode = KEY_0x72; break;
             case 0x73: *scanCode = KEY_0x73; break;
             case 0x74: *scanCode = KEY_0x74; break;
             case 0x75: *scanCode = KEY_0x75; break;
             case 0x76: *scanCode = KEY_0x76; break;
            }
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
