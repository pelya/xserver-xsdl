/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86KbdBSD.c,v 3.10 1996/12/23 06:43:30 dawes Exp $ */
/*
 * Derived from xf86Kbd.c by S_ren Schmidt (sos@login.dkuug.dk)
 * which is Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 * and from xf86KbdCODrv.c by Holger Veit
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
 */
/* $Xorg: xf86KbdBSD.c,v 1.3 2000/08/17 19:50:30 cpqbld Exp $ */

#include "X.h"
#include "Xmd.h"
#include "input.h"
#include "scrnintstr.h"

#include "compiler.h"

#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "atKeynames.h"
#include "xf86_Config.h"
#include "coKeynames.h"
#include "xf86Keymap.h"

#define KD_GET_ENTRY(i,n) \
  eascii_to_x[((keymap.key[i].spcl << (n+1)) & 0x100) + keymap.key[i].map[n]]

#ifndef __bsdi__
static KeySym coGetKeysym();
#endif

#ifndef PC98
static unsigned char remap[128] = {
     0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,   /* 0x00 - 0x07 */
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,   /* 0x08 - 0x0f */
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,   /* 0x10 - 0x17 */
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,   /* 0x18 - 0x1f */
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,   /* 0x20 - 0x27 */
  0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,   /* 0x28 - 0x2f */
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,    0,   /* 0x30 - 0x37 */
  0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,   /* 0x38 - 0x3f */
  0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,    0,   /* 0x40 - 0x47 */
     0,    0,    0,    0,    0,    0,    0,    0,   /* 0x48 - 0x4f */
     0,    0,    0,    0,    0,    0, 0x56, 0x57,   /* 0x50 - 0x57 */
  0x58,    0,    0,    0,    0,    0,    0,    0,   /* 0x58 - 0x5f */
     0,    0,    0,    0,    0,    0,    0,    0,   /* 0x60 - 0x67 */
     0,    0,    0,    0,    0,    0,    0,    0,   /* 0x68 - 0x6f */
     0,    0, 0x69, 0x65,    0,    0,    0,    0,   /* 0x70 - 0x77 */
     0,    0,    0,    0,    0,    0,    0,    0,   /* 0x78 - 0x7f */
};
#else /* PC98 */
static unsigned char remap[128] = {
     0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,   /* 0x00 - 0x07 */
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,   /* 0x08 - 0x0f */
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,   /* 0x10 - 0x17 */
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,   /* 0x18 - 0x1f */
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,   /* 0x20 - 0x27 */
  0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,   /* 0x28 - 0x2f */
  0x30, 0x31, 0x32, 0x33, 0x34,    0,    0,    0,   /* 0x30 - 0x37 */
     0,    0,    0,    0,    0,    0,    0,    0,   /* 0x38 - 0x3f */
     0,    0,    0,    0,    0,    0,    0,    0,   /* 0x40 - 0x47 */
     0,    0,    0,    0,    0,    0,    0,    0,   /* 0x48 - 0x4f */
     0,    0, 0x52, 0x53, 0x54, 0x55, 0x56,    0,   /* 0x50 - 0x57 */
     0,    0,    0,    0,    0,    0,    0,    0,   /* 0x58 - 0x5f */
  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,   /* 0x60 - 0x67 */
  0x68, 0x69, 0x6a, 0x6b,    0,    0,    0,    0,   /* 0x68 - 0x6f */
  0x70, 0x71, 0x72, 0x73, 0x74,    0,    0,    0,   /* 0x70 - 0x77 */
     0,    0,    0,    0,    0,    0,    0,    0,   /* 0x78 - 0x7f */
};
#endif /* PC98 */


/* This table assumes the ibm code page 437 coding for characters 
 * > 0x80. They are returned in this form by PCVT */
static KeySym eascii_to_x[512] = {
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

	/* 
	 * special marked entries (256 + x)
	 */

	NoSymbol,	NoSymbol,	XK_Shift_L,	XK_Shift_R,
	XK_Caps_Lock,	XK_Num_Lock,	XK_Scroll_Lock,	XK_Alt_L,
	NoSymbol,	XK_Control_L,	XK_Alt_L,	XK_Alt_R,
	XK_Control_L,	XK_Control_R,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	XK_F1,
	XK_F2,		XK_F3,		XK_F4,		XK_F5,
	XK_F6,		XK_F7,		XK_F8,		XK_F9,
	XK_F10,		XK_F11,		XK_F12,		NoSymbol,
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
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
      };

#ifdef __OpenBSD__
/* don't mark AltR and  CtrlR for remapping, since they 
 * cannot be remapped by pccons */
static unsigned char pccons_remap[128] = {
     0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,   /* 0x00 - 0x07 */
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,   /* 0x08 - 0x0f */
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,   /* 0x10 - 0x17 */
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,   /* 0x18 - 0x1f */
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,   /* 0x20 - 0x27 */
  0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,   /* 0x28 - 0x2f */
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,    0,   /* 0x30 - 0x37 */
  0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,   /* 0x38 - 0x3f */
  0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,    0,   /* 0x40 - 0x47 */
     0,    0,    0,    0,    0,    0,    0,    0,   /* 0x48 - 0x4f */
     0,    0,    0,    0,    0,    0, 0x56, 0x57,   /* 0x50 - 0x57 */
  0x58,    0,    0,    0,    0,    0,    0,    0,   /* 0x58 - 0x5f */
     0,    0,    0,    0,    0,    0,    0,    0,   /* 0x60 - 0x67 */
     0,    0,    0,    0,    0,    0,    0,    0,   /* 0x68 - 0x6f */
     0,    0,    0,    0,    0,    0,    0,    0,   /* 0x70 - 0x77 */
     0,    0,    0,    0,    0,    0,    0,    0,   /* 0x78 - 0x7f */
};

/* This table assumes an iso8859_1 encoding for the characters 
 * > 80, as returned by pccons */
static KeySym latin1_to_x[256] = {
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
	XK_adiaeresis,	XK_aring,	XK_ae,	        XK_ccedilla,
	XK_egrave,	XK_eacute,	XK_ecircumflex,	XK_ediaeresis,
	XK_igrave,	XK_iacute,	XK_icircumflex, XK_idiaeresis,
	XK_eth,		XK_ntilde,	XK_ograve, 	XK_oacute,
	XK_ocircumflex,	XK_otilde,	XK_odiaeresis,	XK_division,
	XK_oslash,	XK_ugrave,	XK_uacute,	XK_ucircumflex,
	XK_udiaeresis,	XK_yacute,	XK_thorn, 	XK_ydiaeresis
      };
#endif
/*
 * LegalModifier --
 *      determine whether a key is a legal modifier key, i.e send a
 *      press/release sequence.
 */

/*ARGSUSED*/
Bool
LegalModifier(key, pDev)
     unsigned int  key;
     DevicePtr pDev;
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
  char          type;
  int           i, j;

#ifndef __bsdi__
  switch (xf86Info.consType) {

#ifdef PCCONS_SUPPORT
  case PCCONS:
#if defined(__OpenBSD__)
    /*
     * on OpenBSD, the pccons keymap is programmable, too
     */
    {
      pccons_keymap_t keymap[KB_NUM_KEYS];
      if (ioctl(xf86Info.consoleFd, CONSOLE_GET_KEYMAP, &keymap) != -1) {
	for (i = 0; i < KB_NUM_KEYS; i++) 
	  if (pccons_remap[i]) {
	    k = map + (pccons_remap[i] << 2);
	    switch (keymap[i].type) {
	    case KB_ASCII:
	      /* For ASCII keys, there is only one char in the keymap */
	      k[0] = latin1_to_x[(unsigned char)keymap[i].unshift[0]];
	      k[1] = latin1_to_x[(unsigned char)keymap[i].shift[0]];
	      k[2] = latin1_to_x[(unsigned char)keymap[i].altgr[0]];
	      k[3] = latin1_to_x[(unsigned char)keymap[i].shift_altgr[0]];
	      break;
	    case KB_SCROLL:
	      k[0] = XK_Scroll_Lock;
	      goto special;
	    case KB_NUM:
	      k[0] = XK_Num_Lock;
	      goto special;
	    case KB_CAPS:
	      k[0] = XK_Caps_Lock;
	      goto special;
	    case KB_SHIFT:
	      switch (keymap[i].unshift[0]) {
	      case 1:
		/* left shift */
		k[0] = XK_Shift_L;
		break;
	      case 2:
		/* right shift */
		k[0] = XK_Shift_R;
		break;
	      default:
		k[0] = NoSymbol;
	      }
	      goto special;
	    case KB_CTL:
	      k[0] = XK_Control_L;
	      goto special;
	    case KB_ALT:
	      k[0] = XK_Alt_L;
	      goto special;
	    case KB_FUNC:
	      switch (keymap[i].unshift[2]) {
	      case 'M':
		k[0] = XK_F1;
		break;
	      case 'N':
		k[0] = XK_F2;
		break;
	      case 'O':
		k[0] = XK_F3;
		break;
	      case 'P':
		k[0] = XK_F4;
		break;
	      case 'Q':
		k[0] = XK_F5;
		break;
	      case 'R':
		k[0] = XK_F6;
		break;
	      case 'S':
		k[0] = XK_F7;
		break;
	      case 'T':
		k[0] = XK_F8;
		break;
	      case 'U':
		k[0] = XK_F9;
		break;
	      case 'V':
		k[0] = XK_F10;
		break;
	      case 'W':
		k[0] = XK_F11;
		break;
	      case 'X':
		k[0] = XK_F12;
		break;
	      default:
		k[0] = NoSymbol;
		break;
	      }
	      goto special;
	    default:
	      k[0] = NoSymbol;
	    special:
	      k[1] = k[2] = k[3] = NoSymbol;
	    }
	  }
      } else {
	ErrorF("Can't read pccons keymap\n");
      }
    }
#endif /* __OpenBSD__ */
  break;
#endif

#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
  case SYSCONS:
  case PCVT:
    {
      keymap_t keymap;
    
      if (ioctl(xf86Info.consoleFd, GIO_KEYMAP, &keymap) != -1) {
	for (i = 0; i < keymap.n_keys && i < NUM_KEYCODES; i++)
#ifdef PC98
	  if (remap[i] || i == 0) {
#else
	  if (remap[i]) {
#endif
	    k = map + (remap[i] << 2);
	    k[0] = KD_GET_ENTRY(i,0);           /* non-shifed */
	    k[1] = KD_GET_ENTRY(i,1);	      /* shifted */
	    k[2] = KD_GET_ENTRY(i,4);	      /* alt */
	    k[3] = KD_GET_ENTRY(i,5);	      /* alt - shifted */
	    if (k[3] == k[2]) k[3] = NoSymbol;
	    if (k[2] == k[1]) k[2] = NoSymbol;
	    if (k[1] == k[0]) k[1] = NoSymbol;
	    if (k[0] == k[2] && k[1] == k[3])
	      k[2] = k[3] = NoSymbol;
	  }
      }
    }
    break;
#endif /* SYSCONS || PCVT */
    
#ifdef CODRV_SUPPORT
  case CODRV011:
  case CODRV01X:
    for (i=1; i<= 128; i++) {
      struct kbd_ovlkey cokeymap;
      struct oldkbd_ovlkey ocokeymap;
      KeySym coGetKeysym();

      if (xf86Info.consType == CODRV011) {
        ocokeymap.keynum = i;
        if (ioctl(xf86Info.consoleFd, OLDKBDGCKEY, &ocokeymap) < 0)
          break;  /* Don't try again if it fails once */
        if ((ocokeymap.type & KBD_OVERLOAD)) { /* changed from default setting */
  	  k = map + (xf86CodrvMap[i] << 2);
  	  k[0] = coGetKeysym(ocokeymap.type,(CARD8*)&ocokeymap.unshift,k[0]);
	  k[1] = coGetKeysym(ocokeymap.type,(CARD8*)&ocokeymap.shift,k[1]);
	  k[2] = coGetKeysym(ocokeymap.type,(CARD8*)&ocokeymap.altgr,k[2]);
	  k[3] = NoSymbol;
        }
      } else {
        cokeymap.keynum = i;
        if (ioctl(xf86Info.consoleFd, KBDGCKEY, &cokeymap) < 0)
          break;  /* Don't try again if it fails once */
        if ((cokeymap.type & KBD_OVERLOAD)) { /* changed from default setting */
	  k = map + (xf86CodrvMap[i] << 2);
	  k[0] = coGetKeysym(cokeymap.type,(CARD8*)&cokeymap.unshift,k[0]);
	  k[1] = coGetKeysym(cokeymap.type,(CARD8*)&cokeymap.shift,k[1]);
	  k[2] = coGetKeysym(cokeymap.type,(CARD8*)&cokeymap.altgr,k[2]);
	  k[3] = coGetKeysym(cokeymap.type,(CARD8*)&cokeymap.shiftaltgr,k[3]);
        }
      }
      if ((cokeymap.type & KBD_OVERLOAD)) { /* changed from default setting */
        if (k[3] == k[2]) k[3] = NoSymbol;
        if (k[2] == k[1]) k[2] = NoSymbol;
        if (k[1] == k[0]) k[1] = NoSymbol;
        if (k[0] == k[2] && k[1] == k[3]) k[2] = k[3] = NoSymbol;
      }
    }
    break;
#endif /* CODRV */
  } 
#endif /* !bsdi */

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
  
  xf86Info.kbdType = 0;

  pKeySyms->map        = map;
  pKeySyms->mapWidth   = GLYPHS_PER_KEY;
  pKeySyms->minKeyCode = MIN_KEYCODE;
  if (xf86Info.serverNumLock)
    pKeySyms->maxKeyCode = MAX_KEYCODE; 
  else
    pKeySyms->maxKeyCode = MAX_STD_KEYCODE;

}

#ifdef CODRV_SUPPORT
/* Converts a CoDriver ASCII+Special combination into a KeySym
 */
static KeySym
coGetKeysym(typ,str,old)
	int typ;
	CARD8 *str;
	KeySym old;
{
	if (strlen((char *)str) > 1) return old;
	switch (typ & KBD_MASK) {
	case KBD_NONE:	
		return NoSymbol;
	case KBD_SHIFT: 
		if (old==XK_Shift_L || old==XK_Shift_R) return old;
		else return XK_Shift_L;
	case KBD_NUM:	
		return XK_Num_Lock;
	case KBD_CTL:
		if (old==XK_Control_L || old==XK_Control_R) return old;
		else return XK_Control_L;
	case KBD_ALTGR:	
		return XK_Mode_switch;
	case KBD_META:
		if (old==XK_Alt_L || old==XK_Alt_R) return old;
		else return XK_Alt_L;
	case KBD_SHFTLOCK:
	case KBD_CAPS:
		return XK_Caps_Lock;
	case KBD_SCROLL:
		return XK_Scroll_Lock;
	case KBD_BREAK:
		return XK_Break;
	default:
	case KBD_KP:	/* there are few good reasons to overload 
			 * F Keys and KP Keys, so we ignore any attempt
			 * at all
			 */
		 return old;
	case KBD_ASCII:
		return *str;
	}
}
#endif
