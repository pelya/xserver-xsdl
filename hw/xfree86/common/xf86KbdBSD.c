/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86KbdBSD.c,v 3.20 2002/05/22 21:38:27 herrb Exp $ */
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
/* $XConsortium: xf86KbdBSD.c /main/6 1996/10/23 13:12:27 kaleb $ */

#include "X.h"
#include "Xmd.h"
#include "input.h"
#include "scrnintstr.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "atKeynames.h"
#include "xf86Keymap.h"

#if (defined(SYSCONS_SUPPORT) || defined(PCVT_SUPPORT)) && defined(GIO_KEYMAP)
#define KD_GET_ENTRY(i,n) \
  eascii_to_x[((keymap.key[i].spcl << (n+1)) & 0x100) + keymap.key[i].map[n]]

static unsigned char remap[NUM_KEYCODES] = {
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

#if 1
	/* This has been checked against what syscons actually does */
	NoSymbol,	NoSymbol,	XK_Shift_L,	XK_Shift_R,
	XK_Caps_Lock,	XK_Num_Lock,	XK_Scroll_Lock,	XK_Alt_L,
	XK_ISO_Left_Tab,XK_Control_L,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
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
	XK_Control_R,	XK_Alt_R,	NoSymbol,	NoSymbol,
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
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol
#else
	/* The old table, supposedly for pcvt. */
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
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol
#endif
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
#endif /* SYSCONS_SUPPORT || PCVT_SUPPORT */
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
  int           i;

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

/*
 * XXX wscons has no GIO_KEYMAP
 */
#if (defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)) && defined(GIO_KEYMAP)
  case SYSCONS:
  case PCVT:
    {
      keymap_t keymap;
    
      if (ioctl(xf86Info.consoleFd, GIO_KEYMAP, &keymap) != -1) {
	for (i = 0; i < keymap.n_keys && i < NUM_KEYCODES; i++)
	  if (remap[i]) {
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
    
  } 
#endif /* !bsdi */

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
  
  xf86Info.kbdType = 0;

  pKeySyms->map        = map;
  pKeySyms->mapWidth   = GLYPHS_PER_KEY;
  pKeySyms->minKeyCode = MIN_KEYCODE;
  pKeySyms->maxKeyCode = MAX_KEYCODE; 

}

#ifdef WSCONS_SUPPORT
#include "atKeynames.h"

static CARD8 wsUsbMap[] = {
	/* 0 */ KEY_NOTUSED,
	/* 1 */ KEY_NOTUSED,
	/* 2 */ KEY_NOTUSED,
	/* 3 */ KEY_NOTUSED,
	/* 4 */ KEY_A,		
	/* 5 */ KEY_B,
	/* 6 */ KEY_C,
	/* 7 */ KEY_D,
	/* 8 */ KEY_E,
	/* 9 */ KEY_F,
	/* 10 */ KEY_G,
	/* 11 */ KEY_H,
	/* 12 */ KEY_I,
	/* 13 */ KEY_J,
	/* 14 */ KEY_K,
	/* 15 */ KEY_L,
	/* 16 */ KEY_M,
	/* 17 */ KEY_N,
	/* 18 */ KEY_O,
	/* 19 */ KEY_P,
	/* 20 */ KEY_Q,
	/* 21 */ KEY_R,
	/* 22 */ KEY_S,
	/* 23 */ KEY_T,
	/* 24 */ KEY_U,
	/* 25 */ KEY_V,
	/* 26 */ KEY_W,
	/* 27 */ KEY_X,
	/* 28 */ KEY_Y,
	/* 29 */ KEY_Z,
	/* 30 */ KEY_1,		/* 1 !*/
	/* 31 */ KEY_2,		/* 2 @ */
	/* 32 */ KEY_3,		/* 3 # */
	/* 33 */ KEY_4,		/* 4 $ */
	/* 34 */ KEY_5,		/* 5 % */
	/* 35 */ KEY_6,		/* 6 ^ */
	/* 36 */ KEY_7,		/* 7 & */
	/* 37 */ KEY_8,		/* 8 * */
	/* 38 */ KEY_9,		/* 9 ( */
	/* 39 */ KEY_0,		/* 0 ) */
	/* 40 */ KEY_Enter,	/* Return  */
	/* 41 */ KEY_Escape,	/* Escape */
	/* 42 */ KEY_BackSpace,	/* Backspace Delete */
	/* 43 */ KEY_Tab,	/* Tab */
	/* 44 */ KEY_Space,	/* Space */
	/* 45 */ KEY_Minus,	/* - _ */
	/* 46 */ KEY_Equal,	/* = + */
	/* 47 */ KEY_LBrace,	/* [ { */
	/* 48 */ KEY_RBrace,	/* ] } */
	/* 49 */ KEY_BSlash,	/* \ | */
	/* 50 */ KEY_BSlash2,	/* \ _ # ~ on some keyboards */
	/* 51 */ KEY_SemiColon,	/* ; : */
	/* 52 */ KEY_Quote,	/* ' " */
	/* 53 */ KEY_Tilde,	/* ` ~ */
	/* 54 */ KEY_Comma,	/* , <  */
	/* 55 */ KEY_Period,	/* . > */
	/* 56 */ KEY_Slash,	/* / ? */
	/* 57 */ KEY_CapsLock,	/* Caps Lock */
	/* 58 */ KEY_F1,		/* F1 */
	/* 59 */ KEY_F2,		/* F2 */
	/* 60 */ KEY_F3,		/* F3 */
	/* 61 */ KEY_F4,		/* F4 */
	/* 62 */ KEY_F5,		/* F5 */
	/* 63 */ KEY_F6,		/* F6 */
	/* 64 */ KEY_F7,		/* F7 */
	/* 65 */ KEY_F8,		/* F8 */
	/* 66 */ KEY_F9,		/* F9 */
	/* 67 */ KEY_F10,	/* F10 */
	/* 68 */ KEY_F11,	/* F11 */
	/* 69 */ KEY_F12,	/* F12 */
	/* 70 */ KEY_Print,	/* PrintScrn SysReq */
	/* 71 */ KEY_ScrollLock,	/* Scroll Lock */
	/* 72 */ KEY_Pause,	/* Pause Break */
	/* 73 */ KEY_Insert,	/* Insert XXX  Help on some Mac Keyboards */
	/* 74 */ KEY_Home,	/* Home */
	/* 75 */ KEY_PgUp,	/* Page Up */
	/* 76 */ KEY_Delete,	/* Delete */
	/* 77 */ KEY_End,	/* End */
	/* 78 */ KEY_PgDown,	/* Page Down */
	/* 79 */ KEY_Right,	/* Right Arrow */
	/* 80 */ KEY_Left,	/* Left Arrow */
	/* 81 */ KEY_Down,	/* Down Arrow */
	/* 82 */ KEY_Up,		/* Up Arrow */
	/* 83 */ KEY_NumLock,	/* Num Lock */
	/* 84 */ KEY_KP_Divide,	/* Keypad / */
	/* 85 */ KEY_KP_Multiply, /* Keypad * */
	/* 86 */ KEY_KP_Minus,	/* Keypad - */
	/* 87 */ KEY_KP_Plus,	/* Keypad + */
	/* 88 */ KEY_KP_Enter,	/* Keypad Enter */
	/* 89 */ KEY_KP_1,	/* Keypad 1 End */
	/* 90 */ KEY_KP_2,	/* Keypad 2 Down */
	/* 91 */ KEY_KP_3,	/* Keypad 3 Pg Down */
	/* 92 */ KEY_KP_4,	/* Keypad 4 Left  */
	/* 93 */ KEY_KP_5,	/* Keypad 5 */
	/* 94 */ KEY_KP_6,	/* Keypad 6 */
	/* 95 */ KEY_KP_7,	/* Keypad 7 Home */
	/* 96 */ KEY_KP_8,	/* Keypad 8 Up */
	/* 97 */ KEY_KP_9,	/* KEypad 9 Pg Up */
	/* 98 */ KEY_KP_0,	/* Keypad 0 Ins */
	/* 99 */ KEY_KP_Decimal,	/* Keypad . Del */
	/* 100 */ KEY_Less,	/* < > on some keyboards */
	/* 101 */ KEY_Menu,	/* Menu */
	/* 102 */ KEY_NOTUSED,
	/* 103 */ KEY_KP_Equal, /* Keypad = on Mac keyboards */
	/* 104 */ KEY_NOTUSED,
	/* 105 */ KEY_NOTUSED,
	/* 106 */ KEY_NOTUSED,
	/* 107 */ KEY_NOTUSED,
	/* 108 */ KEY_NOTUSED,
	/* 109 */ KEY_NOTUSED,
	/* 110 */ KEY_NOTUSED,
	/* 111 */ KEY_NOTUSED,
	/* 112 */ KEY_NOTUSED,
	/* 113 */ KEY_NOTUSED,
	/* 114 */ KEY_NOTUSED,
	/* 115 */ KEY_NOTUSED,
	/* 116 */ KEY_NOTUSED,
	/* 117 */ KEY_NOTUSED,
	/* 118 */ KEY_NOTUSED,
	/* 119 */ KEY_NOTUSED,
	/* 120 */ KEY_NOTUSED,
	/* 121 */ KEY_NOTUSED,
	/* 122 */ KEY_NOTUSED,
	/* 123 */ KEY_NOTUSED,
	/* 124 */ KEY_NOTUSED,
	/* 125 */ KEY_NOTUSED,
	/* 126 */ KEY_NOTUSED,
	/* 127 */ KEY_NOTUSED,
	/* 128 */ KEY_NOTUSED,
	/* 129 */ KEY_NOTUSED,
	/* 130 */ KEY_NOTUSED,
	/* 131 */ KEY_NOTUSED,
	/* 132 */ KEY_NOTUSED,
	/* 133 */ KEY_NOTUSED,
	/* 134 */ KEY_NOTUSED,
	/* 135 */ KEY_NOTUSED,
	/* 136 */ KEY_NOTUSED,
	/* 137 */ KEY_NOTUSED,
	/* 138 */ KEY_NOTUSED,
	/* 139 */ KEY_NOTUSED,
	/* 140 */ KEY_NOTUSED,
	/* 141 */ KEY_NOTUSED,
	/* 142 */ KEY_NOTUSED,
	/* 143 */ KEY_NOTUSED,
	/* 144 */ KEY_NOTUSED,
	/* 145 */ KEY_NOTUSED,
	/* 146 */ KEY_NOTUSED,
	/* 147 */ KEY_NOTUSED,
	/* 148 */ KEY_NOTUSED,
	/* 149 */ KEY_NOTUSED,
	/* 150 */ KEY_NOTUSED,
	/* 151 */ KEY_NOTUSED,
	/* 152 */ KEY_NOTUSED,
	/* 153 */ KEY_NOTUSED,
	/* 154 */ KEY_NOTUSED,
	/* 155 */ KEY_NOTUSED,
	/* 156 */ KEY_NOTUSED,
	/* 157 */ KEY_NOTUSED,
	/* 158 */ KEY_NOTUSED,
	/* 159 */ KEY_NOTUSED,
	/* 160 */ KEY_NOTUSED,
	/* 161 */ KEY_NOTUSED,
	/* 162 */ KEY_NOTUSED,
	/* 163 */ KEY_NOTUSED,
	/* 164 */ KEY_NOTUSED,
	/* 165 */ KEY_NOTUSED,
	/* 166 */ KEY_NOTUSED,
	/* 167 */ KEY_NOTUSED,
	/* 168 */ KEY_NOTUSED,
	/* 169 */ KEY_NOTUSED,
	/* 170 */ KEY_NOTUSED,
	/* 171 */ KEY_NOTUSED,
	/* 172 */ KEY_NOTUSED,
	/* 173 */ KEY_NOTUSED,
	/* 174 */ KEY_NOTUSED,
	/* 175 */ KEY_NOTUSED,
	/* 176 */ KEY_NOTUSED,
	/* 177 */ KEY_NOTUSED,
	/* 178 */ KEY_NOTUSED,
	/* 179 */ KEY_NOTUSED,
	/* 180 */ KEY_NOTUSED,
	/* 181 */ KEY_NOTUSED,
	/* 182 */ KEY_NOTUSED,
	/* 183 */ KEY_NOTUSED,
	/* 184 */ KEY_NOTUSED,
	/* 185 */ KEY_NOTUSED,
	/* 186 */ KEY_NOTUSED,
	/* 187 */ KEY_NOTUSED,
	/* 188 */ KEY_NOTUSED,
	/* 189 */ KEY_NOTUSED,
	/* 190 */ KEY_NOTUSED,
	/* 191 */ KEY_NOTUSED,
	/* 192 */ KEY_NOTUSED,
	/* 193 */ KEY_NOTUSED,
	/* 194 */ KEY_NOTUSED,
	/* 195 */ KEY_NOTUSED,
	/* 196 */ KEY_NOTUSED,
	/* 197 */ KEY_NOTUSED,
	/* 198 */ KEY_NOTUSED,
	/* 199 */ KEY_NOTUSED,
	/* 200 */ KEY_NOTUSED,
	/* 201 */ KEY_NOTUSED,
	/* 202 */ KEY_NOTUSED,
	/* 203 */ KEY_NOTUSED,
	/* 204 */ KEY_NOTUSED,
	/* 205 */ KEY_NOTUSED,
	/* 206 */ KEY_NOTUSED,
	/* 207 */ KEY_NOTUSED,
	/* 208 */ KEY_NOTUSED,
	/* 209 */ KEY_NOTUSED,
	/* 210 */ KEY_NOTUSED,
	/* 211 */ KEY_NOTUSED,
	/* 212 */ KEY_NOTUSED,
	/* 213 */ KEY_NOTUSED,
	/* 214 */ KEY_NOTUSED,
	/* 215 */ KEY_NOTUSED,
	/* 216 */ KEY_NOTUSED,
	/* 217 */ KEY_NOTUSED,
	/* 218 */ KEY_NOTUSED,
	/* 219 */ KEY_NOTUSED,
	/* 220 */ KEY_NOTUSED,
	/* 221 */ KEY_NOTUSED,
	/* 222 */ KEY_NOTUSED,
	/* 223 */ KEY_NOTUSED,
	/* 224 */ KEY_LCtrl,	/* Left Control */
	/* 225 */ KEY_ShiftL,	/* Left Shift */
	/* 226 */ KEY_Alt,	/* Left Alt */
	/* 227 */ KEY_LMeta,	/* Left Meta */
	/* 228 */ KEY_RCtrl,	/* Right Control */
	/* 229 */ KEY_ShiftR,	/* Right Shift */
	/* 230 */ KEY_AltLang,	/* Right Alt, AKA AltGr */
	/* 231 */ KEY_LMeta,	/* Right Meta XXX */
};

#define WS_USB_MAP_SIZE (sizeof(wsUsbMap)/sizeof(unsigned char))

/* Map for adb keyboards  */
static CARD8 wsAdbMap[] = {
	/* 0 */ KEY_A,
	/* 1 */ KEY_S,
	/* 2 */ KEY_D,
	/* 3 */ KEY_F,
	/* 4 */ KEY_H,
	/* 5 */ KEY_G,
	/* 6 */ KEY_Z,
	/* 7 */ KEY_X,
	/* 8 */ KEY_C,
	/* 9 */ KEY_V,
	/* 10 */ KEY_UNKNOWN,	/* @ # on french keyboards */
	/* 11 */ KEY_B,
	/* 12 */ KEY_Q,
	/* 13 */ KEY_W,
	/* 14 */ KEY_E,
	/* 15 */ KEY_R,
	/* 16 */ KEY_Y,
	/* 17 */ KEY_T,
	/* 18 */ KEY_1,
	/* 19 */ KEY_2,
	/* 20 */ KEY_3,
	/* 21 */ KEY_4,
	/* 22 */ KEY_6,
	/* 23 */ KEY_5,
	/* 24 */ KEY_Equal,
	/* 25 */ KEY_9,
	/* 26 */ KEY_7,
	/* 27 */ KEY_Minus,
	/* 28 */ KEY_8,
	/* 29 */ KEY_0,
	/* 30 */ KEY_RBrace,
	/* 31 */ KEY_O,
	/* 32 */ KEY_U,
	/* 33 */ KEY_LBrace,
	/* 34 */ KEY_I,
	/* 35 */ KEY_P,
	/* 36 */ KEY_Enter,
	/* 37 */ KEY_L,
	/* 38 */ KEY_J,
	/* 39 */ KEY_Quote,
	/* 40 */ KEY_K,
	/* 41 */ KEY_SemiColon,
	/* 42 */ KEY_BSlash,
	/* 43 */ KEY_Comma,
	/* 44 */ KEY_Slash,
	/* 45 */ KEY_N,
	/* 46 */ KEY_M,
	/* 47 */ KEY_Period,
	/* 48 */ KEY_Tab,
	/* 49 */ KEY_Space,
	/* 50 */ KEY_Tilde,
	/* 51 */ KEY_Delete,
	/* 52 */ KEY_AltLang,
	/* 53 */ KEY_Escape,
	/* 54 */ KEY_LCtrl,
	/* 55 */ KEY_Alt,
	/* 56 */ KEY_ShiftL,
	/* 57 */ KEY_CapsLock,
	/* 58 */ KEY_LMeta,
	/* 59 */ KEY_Left,
	/* 60 */ KEY_Right,
	/* 61 */ KEY_Down,
	/* 62 */ KEY_Up,
	/* 63 */ KEY_UNKNOWN,	/* Fn */
	/* 64 */ KEY_NOTUSED,
	/* 65 */ KEY_KP_Decimal,
	/* 66 */ KEY_NOTUSED,
	/* 67 */ KEY_KP_Multiply,
	/* 68 */ KEY_NOTUSED,
	/* 69 */ KEY_KP_Plus,
	/* 70 */ KEY_NOTUSED,
	/* 71 */ KEY_UNKNOWN,	/* Clear */
	/* 72 */ KEY_NOTUSED, 
	/* 73 */ KEY_NOTUSED,
	/* 74 */ KEY_NOTUSED,
	/* 75 */ KEY_KP_Divide,
	/* 76 */ KEY_KP_Enter,
	/* 77 */ KEY_NOTUSED,
	/* 78 */ KEY_KP_Minus,
	/* 79 */ KEY_NOTUSED,
	/* 80 */ KEY_NOTUSED,
	/* 81 */ KEY_KP_Equal,	/* Keypad = */
	/* 82 */ KEY_KP_0,
	/* 83 */ KEY_KP_1,
	/* 84 */ KEY_KP_2,
	/* 85 */ KEY_KP_3,
	/* 86 */ KEY_KP_4,
	/* 87 */ KEY_KP_5,
	/* 88 */ KEY_KP_6,
	/* 89 */ KEY_KP_7,
	/* 90 */ KEY_NOTUSED,
	/* 91 */ KEY_KP_8,
	/* 92 */ KEY_KP_9,
	/* 93 */ KEY_NOTUSED,
	/* 94 */ KEY_NOTUSED,
	/* 95 */ KEY_UNKNOWN,	/* Keypad ,  */
	/* 96 */ KEY_F5,
	/* 97 */ KEY_F6,
	/* 98 */ KEY_F7,
	/* 99 */ KEY_F3,
	/* 100 */ KEY_F8,
	/* 101 */ KEY_F9,
	/* 102 */ KEY_NOTUSED,
	/* 103 */ KEY_F11,
	/* 104 */ KEY_NOTUSED,
	/* 105 */ KEY_NOTUSED,
	/* 106 */ KEY_KP_Enter,
	/* 107 */ KEY_NOTUSED,
	/* 108 */ KEY_NOTUSED,
	/* 109 */ KEY_F10,
	/* 110 */ KEY_NOTUSED,
	/* 111 */ KEY_F12,
	/* 112 */ KEY_NOTUSED,
	/* 113 */ KEY_NOTUSED,
	/* 114 */ KEY_NOTUSED,
	/* 115 */ KEY_Home,
	/* 116 */ KEY_PgUp,
	/* 117 */ KEY_NOTUSED,
	/* 118 */ KEY_F4,
	/* 119 */ KEY_End,
	/* 120 */ KEY_F2,
	/* 121 */ KEY_PgDown,
	/* 122 */ KEY_F1,
	/* 123 */ KEY_NOTUSED,
	/* 124 */ KEY_NOTUSED,
	/* 125 */ KEY_NOTUSED,
	/* 126 */ KEY_NOTUSED,
	/* 127 */ KEY_NOTUSED,
	/* 128 */ KEY_NOTUSED,
	/* 129 */ KEY_NOTUSED,
	/* 130 */ KEY_NOTUSED,
	/* 131 */ KEY_NOTUSED,
	/* 132 */ KEY_NOTUSED,
	/* 133 */ KEY_NOTUSED,
	/* 134 */ KEY_NOTUSED,
	/* 135 */ KEY_NOTUSED,
	/* 136 */ KEY_NOTUSED,
	/* 137 */ KEY_NOTUSED,
	/* 138 */ KEY_NOTUSED,
	/* 139 */ KEY_NOTUSED,
	/* 140 */ KEY_NOTUSED,
	/* 141 */ KEY_NOTUSED,
	/* 142 */ KEY_NOTUSED,
	/* 143 */ KEY_NOTUSED,
	/* 144 */ KEY_NOTUSED,
	/* 145 */ KEY_NOTUSED,
	/* 146 */ KEY_NOTUSED,
	/* 147 */ KEY_NOTUSED,
	/* 148 */ KEY_NOTUSED,
	/* 149 */ KEY_NOTUSED,
	/* 150 */ KEY_NOTUSED,
	/* 151 */ KEY_NOTUSED,
	/* 152 */ KEY_NOTUSED,
	/* 153 */ KEY_NOTUSED,
	/* 154 */ KEY_NOTUSED,
	/* 155 */ KEY_NOTUSED,
	/* 156 */ KEY_NOTUSED,
	/* 157 */ KEY_NOTUSED,
	/* 158 */ KEY_NOTUSED,
	/* 159 */ KEY_NOTUSED,
	/* 160 */ KEY_NOTUSED,
	/* 161 */ KEY_NOTUSED,
	/* 162 */ KEY_NOTUSED,
	/* 163 */ KEY_NOTUSED,
	/* 164 */ KEY_NOTUSED,
	/* 165 */ KEY_NOTUSED,
	/* 166 */ KEY_NOTUSED,
	/* 167 */ KEY_NOTUSED,
	/* 168 */ KEY_NOTUSED,
	/* 169 */ KEY_NOTUSED,
	/* 170 */ KEY_NOTUSED,
	/* 171 */ KEY_NOTUSED,
	/* 172 */ KEY_NOTUSED,
	/* 173 */ KEY_NOTUSED,
	/* 174 */ KEY_NOTUSED,
	/* 175 */ KEY_NOTUSED,
	/* 176 */ KEY_NOTUSED,
	/* 177 */ KEY_NOTUSED,
	/* 178 */ KEY_NOTUSED,
	/* 179 */ KEY_NOTUSED,
	/* 180 */ KEY_NOTUSED,
	/* 181 */ KEY_NOTUSED,
	/* 182 */ KEY_NOTUSED,
	/* 183 */ KEY_NOTUSED,
	/* 184 */ KEY_NOTUSED,
	/* 185 */ KEY_NOTUSED,
	/* 186 */ KEY_NOTUSED,
	/* 187 */ KEY_NOTUSED,
	/* 188 */ KEY_NOTUSED,
	/* 189 */ KEY_NOTUSED,
	/* 190 */ KEY_NOTUSED,
	/* 191 */ KEY_NOTUSED,
	/* 192 */ KEY_NOTUSED,
	/* 193 */ KEY_NOTUSED,
	/* 194 */ KEY_NOTUSED,
	/* 195 */ KEY_NOTUSED,
	/* 196 */ KEY_NOTUSED,
	/* 197 */ KEY_NOTUSED,
	/* 198 */ KEY_NOTUSED,
	/* 199 */ KEY_NOTUSED,
	/* 200 */ KEY_NOTUSED,
	/* 201 */ KEY_NOTUSED,
	/* 202 */ KEY_NOTUSED,
	/* 203 */ KEY_NOTUSED,
	/* 204 */ KEY_NOTUSED,
	/* 205 */ KEY_NOTUSED,
	/* 206 */ KEY_NOTUSED,
	/* 207 */ KEY_NOTUSED,
	/* 208 */ KEY_NOTUSED,
	/* 209 */ KEY_NOTUSED,
	/* 210 */ KEY_NOTUSED,
	/* 211 */ KEY_NOTUSED,
	/* 212 */ KEY_NOTUSED,
	/* 213 */ KEY_NOTUSED,
	/* 214 */ KEY_NOTUSED,
	/* 215 */ KEY_NOTUSED,
	/* 216 */ KEY_NOTUSED,
	/* 217 */ KEY_NOTUSED,
	/* 218 */ KEY_NOTUSED,
	/* 219 */ KEY_NOTUSED,
	/* 220 */ KEY_NOTUSED,
	/* 221 */ KEY_NOTUSED,
	/* 222 */ KEY_NOTUSED,
	/* 223 */ KEY_NOTUSED,
};

static CARD8 wsSunMap[] = {
	/* 0x00 */ KEY_NOTUSED,
	/* 0x01 */ KEY_NOTUSED,		/* stop */
	/* 0x02 */ KEY_NOTUSED,		/* BrightnessDown / S-VolumeDown */
	/* 0x03 */ KEY_NOTUSED,		/* again */
	/* 0x04 */ KEY_NOTUSED,		/* BridgtnessUp / S-VolumeUp */
	/* 0x05 */ KEY_F1,
	/* 0x06 */ KEY_F2,
	/* 0x07 */ KEY_F10,
	/* 0x08 */ KEY_F3,
	/* 0x09 */ KEY_F11,
	/* 0x0a */ KEY_F4,
	/* 0x0b */ KEY_F12,
	/* 0x0c */ KEY_F5,
	/* 0x0d */ KEY_AltLang,
	/* 0x0e */ KEY_F6,
	/* 0x0f */ KEY_NOTUSED,
	/* 0x10 */ KEY_F7,
	/* 0x11 */ KEY_F8,
	/* 0x12 */ KEY_F9,
	/* 0x13 */ KEY_Alt,
	/* 0x14 */ KEY_Up,
	/* 0x15 */ KEY_Pause,
	/* 0x16 */ KEY_Print,
	/* 0x17 */ KEY_NOTUSED,		/* props */
	/* 0x18 */ KEY_Left,
	/* 0x19 */ KEY_ScrollLock,
	/* 0x1a */ KEY_NOTUSED,		/* undo */
	/* 0x1b */ KEY_Down,
	/* 0x1c */ KEY_Right,
	/* 0x1d */ KEY_Escape,
	/* 0x1e */ KEY_1,
	/* 0x1f */ KEY_2,
	/* 0x20 */ KEY_3,
	/* 0x21 */ KEY_4,
	/* 0x22 */ KEY_5,
	/* 0x23 */ KEY_6,
	/* 0x24 */ KEY_7,
	/* 0x25 */ KEY_8,
	/* 0x26 */ KEY_9,
	/* 0x27 */ KEY_0,
	/* 0x28 */ KEY_Minus,
	/* 0x29 */ KEY_Equal,
	/* 0x2a */ KEY_Tilde,
	/* 0x2b */ KEY_BackSpace,
	/* 0x2c */ KEY_Insert,
	/* 0x2d */ KEY_KP_Equal,
	/* 0x2e */ KEY_KP_Divide,
	/* 0x2f */ KEY_KP_Multiply,
	/* 0x30 */ KEY_NOTUSED,
	/* 0x31 */ KEY_NOTUSED,		/* front */
	/* 0x32 */ KEY_KP_Decimal,
	/* 0x33 */ KEY_NOTUSED,		/* copy */
	/* 0x34 */ KEY_Home,
	/* 0x35 */ KEY_Tab,
	/* 0x36 */ KEY_Q,
	/* 0x37 */ KEY_W,
	/* 0x38 */ KEY_E,
	/* 0x39 */ KEY_R,
	/* 0x3a */ KEY_T,
	/* 0x3b */ KEY_Y,
	/* 0x3c */ KEY_U,
	/* 0x3d */ KEY_I,
	/* 0x3e */ KEY_O,
	/* 0x3f */ KEY_P,
	/* 0x40 */ KEY_LBrace,
	/* 0x41 */ KEY_RBrace,
	/* 0x42 */ KEY_Delete,
	/* 0x43 */ KEY_NOTUSED,		/* compose */
	/* 0x44 */ KEY_KP_7,
	/* 0x45 */ KEY_KP_8,
	/* 0x46 */ KEY_KP_9,
	/* 0x47 */ KEY_KP_Minus,
	/* 0x48 */ KEY_NOTUSED,		/* open */
	/* 0x49 */ KEY_NOTUSED,		/* paste */
	/* 0x4a */ KEY_End,
	/* 0x4b */ KEY_NOTUSED,
	/* 0x4c */ KEY_LCtrl,
	/* 0x4d */ KEY_A,
	/* 0x4e */ KEY_S,
	/* 0x4f */ KEY_D,
	/* 0x50 */ KEY_F,
	/* 0x51 */ KEY_G,
	/* 0x52 */ KEY_H,
	/* 0x53 */ KEY_J,
	/* 0x54 */ KEY_K,
	/* 0x55 */ KEY_L,
	/* 0x56 */ KEY_SemiColon,
	/* 0x57 */ KEY_Quote,
	/* 0x58 */ KEY_BSlash,
	/* 0x59 */ KEY_Enter,
	/* 0x5a */ KEY_KP_Enter,
	/* 0x5b */ KEY_KP_4,
	/* 0x5c */ KEY_KP_5,
	/* 0x5d */ KEY_KP_6,
	/* 0x5e */ KEY_KP_0,
	/* 0x5f */ KEY_NOTUSED,		/* find */
	/* 0x60 */ KEY_PgUp,
	/* 0x61 */ KEY_NOTUSED,		/* cut */
	/* 0x62 */ KEY_NumLock,
	/* 0x63 */ KEY_ShiftL,
	/* 0x64 */ KEY_Z,
	/* 0x65 */ KEY_X,
	/* 0x66 */ KEY_C,
	/* 0x67 */ KEY_V,
	/* 0x68 */ KEY_B,
	/* 0x69 */ KEY_N,
	/* 0x6a */ KEY_M,
	/* 0x6b */ KEY_Comma,
	/* 0x6c */ KEY_Period,
	/* 0x6d */ KEY_Slash,
	/* 0x6e */ KEY_ShiftR,
	/* 0x6f */ KEY_NOTUSED,		/* linefeed */
	/* 0x70 */ KEY_KP_1,
	/* 0x71 */ KEY_KP_2,
	/* 0x72 */ KEY_KP_3,
	/* 0x73 */ KEY_NOTUSED,
	/* 0x74 */ KEY_NOTUSED,
	/* 0x75 */ KEY_NOTUSED,
	/* 0x76 */ KEY_NOTUSED,		/* help */
	/* 0x77 */ KEY_CapsLock,
	/* 0x78 */ KEY_LMeta,
	/* 0x79 */ KEY_Space,
	/* 0x7a */ KEY_RMeta,
	/* 0x7b */ KEY_PgDown,
	/* 0x7c */ KEY_NOTUSED,
	/* 0x7d */ KEY_KP_Plus,
	/* 0x7e */ KEY_NOTUSED,
	/* 0x7f */ KEY_NOTUSED
};

#define WS_SUN_MAP_SIZE (sizeof(wsSunMap)/sizeof(unsigned char))


#define WS_ADB_MAP_SIZE (sizeof(wsAdbMap)/sizeof(unsigned char))

/*
 * Translate raw wskbd keyboard event values to XFree86 standard keycodes
 * (based on the AT keyboard scan codes using the tables above
 */
int 
WSKbdToKeycode(int keycode)
{
	switch (xf86Info.wsKbdType) {
	case WSKBD_TYPE_PC_XT:
	case WSKBD_TYPE_PC_AT:
		/* No translation needed */
		return keycode;
	case WSKBD_TYPE_USB:
		if (keycode < 0 || keycode >= WS_USB_MAP_SIZE) 
			return KEY_UNKNOWN;
		else 
			return wsUsbMap[keycode];
#ifdef WSKBD_TYPE_ADB			
	case WSKBD_TYPE_ADB:
		if (keycode < 0 || keycode >= WS_ADB_MAP_SIZE) 
			return KEY_UNKNOWN;
		else 
			return wsAdbMap[keycode];
#endif
#ifdef WSKBD_TYPE_SUN
	case WSKBD_TYPE_SUN:
		if (keycode < 0 || keycode >= WS_SUN_MAP_SIZE)
			return KEY_UNKNOWN;
		else
			return wsSunMap[keycode];
#endif
	default:
		ErrorF("Unknown wskbd type %d\n", xf86Info.wsKbdType);
		return KEY_UNKNOWN;
	}
}

#endif /* WSCONS_SUPPORT */
