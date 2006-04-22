/* $XFree86$ */

/*
 * Slightly modified xf86KbdLnx.c which is
 *
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 */

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
#include "xf86Xinput.h"
#include "xf86OSKbd.h"
#include "atKeynames.h"

#include "xf86Keymap.h"

#include "lnx_kbd.h"

/*ARGSUSED*/

/*
 * KbdGetMapping --
 *	Get the national keyboard mapping. The keyboard type is set, a new map
 *      and the modifiermap is computed.
 */

static void readKernelMapping(InputInfoPtr pInfo,
                              KeySymsPtr pKeySyms, CARD8 *pModMap);
void
KbdGetMapping (InputInfoPtr pInfo, KeySymsPtr pKeySyms, CARD8 *pModMap)
{
  KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
  KeySym        *k;
  char          type;
  int           i;

  readKernelMapping(pInfo, pKeySyms, pModMap);

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
      pModMap[i] = NumLockMask;
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
  
  pKbd->kbdType = ioctl(pInfo->fd, KDGKBTYPE, &type) != -1 ? type : KB_101;

  pKeySyms->map        = map;
  pKeySyms->mapWidth   = GLYPHS_PER_KEY;
  pKeySyms->minKeyCode = MIN_KEYCODE;
  pKeySyms->maxKeyCode = MAX_KEYCODE; 
}

#include <linux/keyboard.h>

static KeySym linux_to_x[256] = {
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	XK_BackSpace,	XK_Tab,		XK_Linefeed,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
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
	XK_bar,		XK_braceright,	XK_asciitilde,	XK_BackSpace,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	XK_nobreakspace,XK_exclamdown,	XK_cent,	XK_sterling,
	XK_currency,	XK_yen,		XK_brokenbar,	XK_section,
	XK_diaeresis,	XK_copyright,	XK_ordfeminine,	XK_guillemotleft,
	XK_notsign,	XK_hyphen,	XK_registered,	XK_macron,
	XK_degree,	XK_plusminus,	XK_twosuperior,	XK_threesuperior,
	XK_acute,	XK_mu,		XK_paragraph,	XK_periodcentered,
	XK_cedilla,	XK_onesuperior,	XK_masculine,	XK_guillemotright,
	XK_onequarter,	XK_onehalf,	XK_threequarters,XK_questiondown,
	XK_Agrave,	XK_Aacute,	XK_Acircumflex,	XK_Atilde,
	XK_Adiaeresis,	XK_Aring,	XK_AE,		XK_Ccedilla,
	XK_Egrave,	XK_Eacute,	XK_Ecircumflex,	XK_Ediaeresis,
	XK_Igrave,	XK_Iacute,	XK_Icircumflex,	XK_Idiaeresis,
	XK_ETH,		XK_Ntilde,	XK_Ograve,	XK_Oacute,
	XK_Ocircumflex,	XK_Otilde,	XK_Odiaeresis,	XK_multiply,
	XK_Ooblique,	XK_Ugrave,	XK_Uacute,	XK_Ucircumflex,
	XK_Udiaeresis,	XK_Yacute,	XK_THORN,	XK_ssharp,
	XK_agrave,	XK_aacute,	XK_acircumflex,	XK_atilde,
	XK_adiaeresis,	XK_aring,	XK_ae,		XK_ccedilla,
	XK_egrave,	XK_eacute,	XK_ecircumflex,	XK_ediaeresis,
	XK_igrave,	XK_iacute,	XK_icircumflex,	XK_idiaeresis,
	XK_eth,		XK_ntilde,	XK_ograve,	XK_oacute,
	XK_ocircumflex,	XK_otilde,	XK_odiaeresis,	XK_division,
	XK_oslash,	XK_ugrave,	XK_uacute,	XK_ucircumflex,
	XK_udiaeresis,	XK_yacute,	XK_thorn,	XK_ydiaeresis
};

/*
 * Maps the AT keycodes to Linux keycodes
 */
static unsigned char at2lnx[] =
{
	0x01,	/* KEY_Escape */	0x02,	/* KEY_1 */
	0x03,	/* KEY_2 */		0x04,	/* KEY_3 */
	0x05,	/* KEY_4 */		0x06,	/* KEY_5 */
	0x07,	/* KEY_6 */		0x08,	/* KEY_7 */
	0x09,	/* KEY_8 */		0x0a,	/* KEY_9 */
	0x0b,	/* KEY_0 */		0x0c,	/* KEY_Minus */
	0x0d,	/* KEY_Equal */		0x0e,	/* KEY_BackSpace */
	0x0f,	/* KEY_Tab */		0x10,	/* KEY_Q */
	0x11,	/* KEY_W */		0x12,	/* KEY_E */
	0x13,	/* KEY_R */		0x14,	/* KEY_T */
	0x15,	/* KEY_Y */		0x16,	/* KEY_U */
	0x17,	/* KEY_I */		0x18,	/* KEY_O */
	0x19,	/* KEY_P */		0x1a,	/* KEY_LBrace */
	0x1b,	/* KEY_RBrace */	0x1c,	/* KEY_Enter */
	0x1d,	/* KEY_LCtrl */		0x1e,	/* KEY_A */
	0x1f,	/* KEY_S */		0x20,	/* KEY_D */
	0x21,	/* KEY_F */		0x22,	/* KEY_G */
	0x23,	/* KEY_H */		0x24,	/* KEY_J */
	0x25,	/* KEY_K */		0x26,	/* KEY_L */
	0x27,	/* KEY_SemiColon */	0x28,	/* KEY_Quote */
	0x29,	/* KEY_Tilde */		0x2a,	/* KEY_ShiftL */
	0x2b,	/* KEY_BSlash */	0x2c,	/* KEY_Z */
	0x2d,	/* KEY_X */		0x2e,	/* KEY_C */
	0x2f,	/* KEY_V */		0x30,	/* KEY_B */
	0x31,	/* KEY_N */		0x32,	/* KEY_M */
	0x33,	/* KEY_Comma */		0x34,	/* KEY_Period */
	0x35,	/* KEY_Slash */		0x36,	/* KEY_ShiftR */
	0x37,	/* KEY_KP_Multiply */	0x38,	/* KEY_Alt */
	0x39,	/* KEY_Space */		0x3a,	/* KEY_CapsLock */
	0x3b,	/* KEY_F1 */		0x3c,	/* KEY_F2 */
	0x3d,	/* KEY_F3 */		0x3e,	/* KEY_F4 */
	0x3f,	/* KEY_F5 */		0x40,	/* KEY_F6 */
	0x41,	/* KEY_F7 */		0x42,	/* KEY_F8 */
	0x43,	/* KEY_F9 */		0x44,	/* KEY_F10 */
	0x45,	/* KEY_NumLock */	0x46,	/* KEY_ScrollLock */
	0x47,	/* KEY_KP_7 */		0x48,	/* KEY_KP_8 */
	0x49,	/* KEY_KP_9 */		0x4a,	/* KEY_KP_Minus */
	0x4b,	/* KEY_KP_4 */		0x4c,	/* KEY_KP_5 */
	0x4d,	/* KEY_KP_6 */		0x4e,	/* KEY_KP_Plus */
	0x4f,	/* KEY_KP_1 */		0x50,	/* KEY_KP_2 */
	0x51,	/* KEY_KP_3 */		0x52,	/* KEY_KP_0 */
	0x53,	/* KEY_KP_Decimal */	0x54,	/* KEY_SysReqest */
	0x00,	/* 0x55 */		0x56,	/* KEY_Less */
	0x57,	/* KEY_F11 */		0x58,	/* KEY_F12 */
	0x66,	/* KEY_Home */		0x67,	/* KEY_Up */
	0x68,	/* KEY_PgUp */		0x69,	/* KEY_Left */
	0x5d,	/* KEY_Begin */		0x6a,	/* KEY_Right */
	0x6b,	/* KEY_End */		0x6c,	/* KEY_Down */
	0x6d,	/* KEY_PgDown */	0x6e,	/* KEY_Insert */
	0x6f,	/* KEY_Delete */	0x60,	/* KEY_KP_Enter */
	0x61,	/* KEY_RCtrl */		0x77,	/* KEY_Pause */
	0x63,	/* KEY_Print */		0x62,	/* KEY_KP_Divide */
	0x64,	/* KEY_AltLang */	0x65,	/* KEY_Break */
	0x00,	/* KEY_LMeta */		0x00,	/* KEY_RMeta */
	0x7A,	/* KEY_Menu/FOCUS_PF11*/0x00,	/* 0x6e */
	0x7B,	/* FOCUS_PF12 */	0x00,	/* 0x70 */
	0x00,	/* 0x71 */		0x00,	/* 0x72 */
	0x59,	/* FOCUS_PF2 */		0x78,	/* FOCUS_PF9 */
	0x00,	/* 0x75 */		0x00,	/* 0x76 */
	0x5A,	/* FOCUS_PF3 */		0x5B,	/* FOCUS_PF4 */
	0x5C,	/* FOCUS_PF5 */		0x5D,	/* FOCUS_PF6 */
	0x5E,	/* FOCUS_PF7 */		0x5F,	/* FOCUS_PF8 */
	0x7C,	/* JAP_86 */		0x79,	/* FOCUS_PF10 */
	0x00,	/* 0x7f */
};
#define NUM_AT2LNX (sizeof(at2lnx) / sizeof(at2lnx[0]))

#define NUM_CUSTOMKEYS	NR_KEYS

static void
readKernelMapping(InputInfoPtr pInfo, KeySymsPtr pKeySyms, CARD8 *pModMap)
{
  KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
  KeySym        *k;
  int           i;
  int           maxkey;
  static unsigned char tbl[GLYPHS_PER_KEY] =
  {
	0,	/* unshifted */
	1,	/* shifted */
	0,	/* modeswitch unshifted */
	0	/* modeswitch shifted */
  };

  /*
   * Read the mapping from the kernel.
   * Since we're still using the XFree86 scancode->AT keycode mapping
   * routines, we need to convert the AT keycodes to Linux keycodes,
   * then translate the Linux keysyms into X keysyms.
   *
   * First, figure out which tables to use for the modeswitch columns
   * above, from the XF86Config fields.
   */
  tbl[2] = 8;	/* alt */
  tbl[3] = tbl[2] | 1;

  if (pKbd->CustomKeycodes) {
    k = map;
    maxkey = NUM_CUSTOMKEYS;
  }
  else {
    k = map+GLYPHS_PER_KEY;
    maxkey = NUM_AT2LNX;
  }

  for (i = 0; i < maxkey; ++i)
  {
    struct kbentry kbe;
    int j;

    if (pKbd->CustomKeycodes)
      kbe.kb_index = i;
    else
      kbe.kb_index = at2lnx[i];

    for (j = 0; j < GLYPHS_PER_KEY; ++j, ++k)
    {
      unsigned short kval;

      *k = NoSymbol;

      kbe.kb_table = tbl[j];
      if (
	  (!pKbd->CustomKeycodes && kbe.kb_index == 0) ||
	  ioctl(pInfo->fd, KDGKBENT, &kbe))
	continue;

      kval = KVAL(kbe.kb_value);
      switch (KTYP(kbe.kb_value))
      {
      case KT_LATIN:
      case KT_LETTER:
	*k = linux_to_x[kval];
	break;

      case KT_FN:
	if (kval <= 19)
	  *k = XK_F1 + kval;
	else switch (kbe.kb_value)
	{
	case K_FIND:
	  *k = XK_Home; /* or XK_Find */
	  break;
	case K_INSERT:
	  *k = XK_Insert;
	  break;
	case K_REMOVE:
	  *k = XK_Delete;
	  break;
	case K_SELECT:
	  *k = XK_End; /* or XK_Select */
	  break;
	case K_PGUP:
	  *k = XK_Prior;
	  break;
	case K_PGDN:
	  *k = XK_Next;
	  break;
	case K_HELP:
	  *k = XK_Help;
	  break;
	case K_DO:
	  *k = XK_Execute;
	  break;
	case K_PAUSE:
	  *k = XK_Pause;
	  break;
	case K_MACRO:
	  *k = XK_Menu;
	  break;
	default:
	  break;
	}
	break;

      case KT_SPEC:
	switch (kbe.kb_value)
	{
	case K_ENTER:
	  *k = XK_Return;
	  break;
	case K_BREAK:
	  *k = XK_Break;
	  break;
	case K_CAPS:
	  *k = XK_Caps_Lock;
	  break;
	case K_NUM:
	  *k = XK_Num_Lock;
	  break;
	case K_HOLD:
	  *k = XK_Scroll_Lock;
	  break;
	case K_COMPOSE:
          *k = XK_Multi_key;
	  break;
	default:
	  break;
	}
	break;

      case KT_PAD:
	switch (kbe.kb_value)
	{
	case K_PPLUS:
	  *k = XK_KP_Add;
	  break;
	case K_PMINUS:
	  *k = XK_KP_Subtract;
	  break;
	case K_PSTAR:
	  *k = XK_KP_Multiply;
	  break;
	case K_PSLASH:
	  *k = XK_KP_Divide;
	  break;
	case K_PENTER:
	  *k = XK_KP_Enter;
	  break;
	case K_PCOMMA:
	  *k = XK_KP_Separator;
	  break;
	case K_PDOT:
	  *k = XK_KP_Decimal;
	  break;
	case K_PPLUSMINUS:
	  *k = XK_KP_Subtract;
	  break;
	default:
	  if (kval <= 9)
	    *k = XK_KP_0 + kval;
	  break;
	}
	break;

      /*
       * KT_DEAD keys are for accelerated diacritical creation.
       */
      case KT_DEAD:
	switch (kbe.kb_value)
	  {
	  case K_DGRAVE:
	    *k = XK_dead_grave;
	    break;
	  case K_DACUTE:
	    *k = XK_dead_acute;
	    break;
	  case K_DCIRCM:
	    *k = XK_dead_circumflex;
	    break;
	  case K_DTILDE:
	    *k = XK_dead_tilde;
	    break;
	  case K_DDIERE:
	    *k = XK_dead_diaeresis;
	    break;
	  }
	break;

      case KT_CUR:
	switch (kbe.kb_value)
	{
	case K_DOWN:
	  *k = XK_Down;
	  break;
	case K_LEFT:
	  *k = XK_Left;
	  break;
	case K_RIGHT:
	  *k = XK_Right;
	  break;
	case K_UP:
	  *k = XK_Up;
	  break;
	}
	break;

      case KT_SHIFT:
	switch (kbe.kb_value)
	{
	case K_ALTGR:
	  *k = XK_Alt_R;
	  break;
	case K_ALT:
	  *k = (kbe.kb_index == 0x64 ?
		XK_Alt_R : XK_Alt_L);
	  break;
	case K_CTRL:
	  *k = (kbe.kb_index == 0x61 ?
		XK_Control_R : XK_Control_L);
	  break;
        case K_CTRLL:
	  *k = XK_Control_L;
	  break;
        case K_CTRLR:
	  *k = XK_Control_R;
	  break;
	case K_SHIFT:
	  *k = (kbe.kb_index == 0x36 ?
		XK_Shift_R : XK_Shift_L);
	  break;
        case K_SHIFTL:
	  *k = XK_Shift_L;
	  break;
        case K_SHIFTR:
	  *k = XK_Shift_R;
	  break;
	default:
	  break;
	}
	break;

      /*
       * KT_ASCII keys accumulate a 3 digit decimal number that gets
       * emitted when the shift state changes. We can't emulate that.
       */
      case KT_ASCII:
	break;

      case KT_LOCK:
	if (kbe.kb_value == K_SHIFTLOCK)
	  *k = XK_Shift_Lock;
	break;

      default:
	break;
      }
    }

    if (k[-1] == k[-2]) k[-1] = NoSymbol;
    if (k[-2] == k[-3]) k[-2] = NoSymbol;
    if (k[-3] == k[-4]) k[-3] = NoSymbol;
    if (k[-4] == k[-2] && k[-3] == k[-1]) k[-2] = k[-1] = NoSymbol;
    if (k[-1] == k[-4] && k[-2] == k[-3] && k[-2] == NoSymbol) k[-1] =NoSymbol;
  }

  if (!pKbd->CustomKeycodes)
    return;

  /*
   * Find the Mapping for the special server functions
   */
  pKbd->specialMap = (TransMapPtr) xcalloc(NUM_CUSTOMKEYS, 1);
  if (pKbd->specialMap != NULL) {
      pKbd->specialMap->end = NUM_CUSTOMKEYS;
      pKbd->specialMap->map = (unsigned char*) xcalloc(NUM_CUSTOMKEYS, 1);
      if (pKbd->specialMap == NULL) {
      	  xfree(pKbd->specialMap);
      	  pKbd->specialMap = NULL;
      }
  }
  if (pKbd->specialMap == NULL) {
      xf86Msg(X_ERROR, "%s can't allocate \"special map\"\n", pInfo->name);
      return;
  }

  for (i = 0; i < NUM_CUSTOMKEYS; ++i) {
    struct kbentry kbe;
    int special = 0;

    kbe.kb_index = i;
    kbe.kb_table = 0; /* Plain map */
    if (!ioctl(pInfo->fd, KDGKBENT, &kbe))
      switch (kbe.kb_value) {
	case K(KT_LATIN,0x7f):	/* This catches DEL too... But who cares? */
	  special = KEY_BackSpace;
	  break;
	case K_PMINUS:
	  special = KEY_KP_Minus;
	  break;
	case K_PPLUS:
	  special = KEY_KP_Plus;
	  break;
	case K_F1:
	  special = KEY_F1;
	  break;
	case K_F2:
	  special = KEY_F2;
	  break;
	case K_F3:
	  special = KEY_F3;
	  break;
	case K_F4:
	  special = KEY_F4;
	  break;
	case K_F5:
	  special = KEY_F5;
	  break;
	case K_F6:
	  special = KEY_F6;
	  break;
	case K_F7:
	  special = KEY_F7;
	  break;
	case K_F8:
	  special = KEY_F8;
	  break;
	case K_F9:
	  special = KEY_F9;
	  break;
	case K_F10:
	  special = KEY_F10;
	  break;
	case K_F11:
	  special = KEY_F11;
	  break;
	case K_F12:
	  special = KEY_F12;
	  break;
	case K_ALT:
	  special = KEY_Alt;
	  break;
	case K_ALTGR:
	  special = KEY_AltLang;
	  break;
	case K_CONS:
	  special = KEY_SysReqest;
	  break;
      }
    pKbd->specialMap->map[i] = special;
  }
}
