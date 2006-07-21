/*
 *****************************************************************************
 * HISTORY
 * Log:	xf86KbdMach.c,v
 * Revision 2.1.2.1  92/06/25  10:32:08  moore
 * 	Incorporate the Elliot Dresselhaus's, Ernest Hua's and local changes
 * 	to run Thomas Roell's I386 color X11R5.  Original code only worked
 * 	with SCO Unix.  New code works with 2.5 and 3.0 Mach
 * 	[92/06/24            rvb]
 * 
 * 	EndLog
 * 
 *****************************************************************************
 */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: xf86KbdMach.c /main/9 1996/02/21 17:38:43 kaleb $ */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include <X11/Xmd.h>
#include "input.h"
#include "scrnintstr.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "atKeynames.h"
#include "xf86Config.h"

#include "xf86Keymap.h"

static KeySym ascii_to_x[256] = {
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	XK_BackSpace,	XK_Tab,		XK_Linefeed,	NoSymbol,
	NoSymbol,	XK_Return,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	XK_Escape,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	XK_space,	XK_exclam,	XK_quotedbl,	XK_numbersign,
	XK_dollar,	XK_percent,	XK_ampersand,	XK_apostrophe,
	XK_parenleft,	XK_parenright,	XK_asterisk,	XK_plus,
	XK_comma,	XK_minus,	XK_period,	XK_slash,
	XK_0,		XK_1,		XK_2,		XK_3,
	XK_4,		XK_5,		XK_6,		XK_7,
	XK_8,		XK_9,		XK_colon,	XK_semicolon,
	XK_less,	XK_equal,	XK_greater,	XK_question,
	XK_at,		XK_A,		XK_B,		XK_C,
	XK_D,		XK_E,		XK_F,		XK_G,
	XK_H,		XK_I,		XK_J,		XK_K,
	XK_L,		XK_M,		XK_N,		XK_O,
	XK_P,		XK_Q,		XK_R,		XK_S,
	XK_T,		XK_U,		XK_V,		XK_W,
	XK_X,		XK_Y,		XK_Z,		XK_bracketleft,
	XK_backslash,	XK_bracketright,XK_asciicircum,	XK_underscore,
	XK_grave,	XK_a,		XK_b,		XK_c,
	XK_d,		XK_e,		XK_f,		XK_g,
	XK_h,		XK_i,		XK_j,		XK_k,
	XK_l,		XK_m,		XK_n,		XK_o,
	XK_p,		XK_q,		XK_r,		XK_s,
	XK_t,		XK_u,		XK_v,		XK_w,
	XK_x,		XK_y,		XK_z,		XK_braceleft,
	XK_bar,		XK_braceright,	XK_asciitilde,	XK_Delete,
	XK_Ccedilla,	XK_udiaeresis,	XK_eacute,	XK_acircumflex,
	XK_adiaeresis,	XK_agrave,	XK_aring,	XK_ccedilla,
	XK_ecircumflex,	XK_ediaeresis,	XK_egrave,	XK_idiaeresis,
	XK_icircumflex,	XK_igrave,	XK_Adiaeresis,	XK_Aring,
	XK_Eacute,	XK_ae,		XK_AE,		XK_ocircumflex,
	XK_odiaeresis,	XK_ograve,	XK_ucircumflex,	XK_ugrave,
	XK_ydiaeresis,	XK_Odiaeresis,	XK_Udiaeresis,	XK_cent,
	XK_sterling,	XK_yen,		XK_paragraph,	XK_section,
	XK_aacute,	XK_iacute,	XK_oacute,	XK_uacute,
	XK_ntilde,	XK_Ntilde,	XK_ordfeminine,	XK_masculine,
	XK_questiondown,XK_hyphen,	XK_notsign,	XK_onehalf,
	XK_onequarter,	XK_exclamdown,	XK_guillemotleft,XK_guillemotright,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	XK_Greek_alpha,	XK_ssharp,	XK_Greek_GAMMA,	XK_Greek_pi,
	XK_Greek_SIGMA,	XK_Greek_sigma,	XK_mu,	        XK_Greek_tau,
	XK_Greek_PHI,	XK_Greek_THETA,	XK_Greek_OMEGA,	XK_Greek_delta,
	XK_infinity,	XK_Ooblique,	XK_Greek_epsilon, XK_intersection,
	XK_identical,	XK_plusminus,	XK_greaterthanequal, XK_lessthanequal,
	XK_topintegral,	XK_botintegral,	XK_division,	XK_similarequal,
	XK_degree,	NoSymbol,	NoSymbol,	XK_radical,
	XK_Greek_eta,	XK_twosuperior,	XK_periodcentered, NoSymbol,
      };

/*
 * LegalModifier --
 *      determine whether a key is a legal modifier key, i.e send a
 *      press/release sequence.
 */

/*ARGSUSED*/
Bool
LegalModifier(key, pDev)
     unsigned int  key;
     DevicePtr  pDev;
{
  return (TRUE);
}


      
/*
 * xf86KbdGetMapping --
 *	Get the national keyboard mapping. The keyboard type is set, a new map
 *      and the modifiermap is computed.
 */

void
xf86KbdGetMapping (pKeySyms, pModMap)
     KeySymsPtr pKeySyms;
     CARD8      *pModMap;
{
  KeySym        *k;
  struct	kbentry kbe;
  char          type;
  int           i, j;
  
  for (i = 0; i < NUMKEYS; i++)
    {
      static int states[] = { NORM_STATE, SHIFT_STATE, ALT_STATE, SHIFT_ALT };
      int j;

      k = &map[i*4];
      kbe.kb_index = i;
	
      for (j = 0; j < 4; j++)
	{
	  kbe.kb_state = states[j];

	  if (ioctl (xf86Info.consoleFd, KDGKBENT, &kbe) != -1)
	    continue;
      
	  if (kbe.kb_value [0] == K_SCAN)
	    {
	      int keycode = -1;
	      switch (kbe.kb_value [1])
		{
		case K_CTLSC: keycode = XK_Control_L; break;
		case K_LSHSC: keycode = XK_Shift_L; break;
		case K_RSHSC: keycode = XK_Shift_R; break;
		case K_ALTSC: keycode = XK_Alt_L; break;
		case K_CLCKSC: keycode = XK_Caps_Lock; break;
		case K_NLCKSC: keycode = XK_Num_Lock; break;
		default: break;
		}
	      if (keycode > 0)
		k[j] = keycode;
	    }
	  else if (kbe.kb_value[1] != NC)
	    {
	      /* How to handle multiple characters?
	         Ignore them for now. */
	    }
	  else
	    {
	      k[j] = ascii_to_x[kbe.kb_value[0]];
	    }
	}
    }

  /*
   * Apply the special key mapping specified in XF86Config 
   */
  for (k = map, i = MIN_KEYCODE;
       i < (NUM_KEYCODES + MIN_KEYCODE);
       i++, k += 4) {
    switch (k[0]) {
      case XK_Alt_L:
        j = K_INDEX_LEFTALT;
        break;
      case XK_Alt_R:
        j = K_INDEX_RIGHTALT;
        break;
      case XK_Scroll_Lock:
        j = K_INDEX_SCROLLLOCK;
        break;
      case XK_Control_R:
        j = K_INDEX_RIGHTCTL;
        break;
      default:
        j = -1;
    }
    if (j >= 0)
      switch (xf86Info.specialKeyMap[j]) {
        case KM_META:
          if (k[0] == XK_Alt_R)
            k[1] = XK_Meta_R;
          else {
            k[0] = XK_Alt_L;
            k[1] = XK_Meta_L;
          }
          break;
        case KM_COMPOSE:
          k[0] = XK_Multi_key;
          break;
        case KM_MODESHIFT:
          k[0] = XK_Mode_switch;
          k[1] = NoSymbol;
          break;
        case KM_MODELOCK:
          k[0] = XK_Mode_switch;
          k[1] = XF86XK_ModeLock;
          break;
        case KM_SCROLLLOCK:
          k[0] = XK_Scroll_Lock;
          break;
        case KM_CONTROL:
          k[0] = XK_Control_R;
          break;
      }
  }

  /*
   * compute the modifier map
   */
  for (i = 0; i < MAP_LENGTH; i++)
    pModMap[i] = NoSymbol;  /* make sure it is restored */
  
  for (k = map, i = MIN_KEYCODE;
       i < (NUM_KEYCODES + MIN_KEYCODE);
       i++, k += 4)
    
    switch(*k) {
      
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
      
    case XK_Num_Lock:
      if (!xf86Info.serverNumLock) pModMap[i] = NumLockMask;
      break;

    case XK_Scroll_Lock:
      pModMap[i] = ScrollLockMask;
      break;

      /* kana support */
    case XK_Kana_Lock:
    case XK_Kana_Shift:
      pModMap[i] = KanaMask;
      break;

      /* alternate toggle for multinational support */
    case XK_Mode_switch:
      pModMap[i] = AltLangMask;
      break;

    }
  
  xf86Info.kbdType =
    ioctl(xf86Info.kbdFd, KDGKBDTYPE, &type) != -1 ? type : KB_VANILLAKB;

  pKeySyms->map        = map;
  pKeySyms->mapWidth   = GLYPHS_PER_KEY;
  pKeySyms->minKeyCode = MIN_KEYCODE;
  if (xf86Info.serverNumLock)
    pKeySyms->maxKeyCode = MAX_KEYCODE; 
  else
    pKeySyms->maxKeyCode = MAX_STD_KEYCODE;

}
