/*
   quartzKeyboard.c: Keyboard support for Xquartz

   Copyright (c) 2003-2008 Apple Inc.
   Copyright (c) 2001-2004 Torrey T. Lyons. All Rights Reserved.
   Copyright 2004 Kaleb S. KEITHLEY. All Rights Reserved.

   Copyright (C) 1999,2000 by Eric Sunshine <sunshine@sunshineco.com>
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
     3. The name of the author may not be used to endorse or promote products
        derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
   NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
   TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "sanitizedCarbon.h"

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

//#define XQUARTZ_USE_XKB 
#define HACK_MISSING 1
#define HACK_KEYPAD 1

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#include "quartzCommon.h"
#include "darwin.h"

#include "quartzKeyboard.h"
#include "quartzAudio.h"

#ifdef NDEBUG
#undef NDEBUG
#include <assert.h>
#define NDEBUG 1
#else
#include <assert.h>
#endif

#include "xkbsrv.h"
#include "exevents.h"
#include "X11/keysym.h"
#include "keysym2ucs.h"

void QuartzXkbUpdate(DeviceIntPtr pDev);

enum {
    MOD_COMMAND = 256,
    MOD_SHIFT = 512,
    MOD_OPTION = 2048,
    MOD_CONTROL = 4096,
};

#define UKEYSYM(u) ((u) | 0x01000000)

#define AltMask         Mod1Mask
#define MetaMask        Mod2Mask
#define FunctionMask    Mod3Mask

#define UK(a)           NoSymbol    // unknown symbol

static KeySym const next_to_x[256] = {
	NoSymbol,	NoSymbol,	NoSymbol,	XK_KP_Enter,
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
	XK_bar,		XK_braceright,	XK_asciitilde,	XK_BackSpace,
// 128
	NoSymbol,	XK_Agrave,	XK_Aacute,	XK_Acircumflex,
	XK_Atilde,	XK_Adiaeresis,	XK_Aring,	XK_Ccedilla,
	XK_Egrave,	XK_Eacute,	XK_Ecircumflex,	XK_Ediaeresis,
	XK_Igrave,	XK_Iacute,	XK_Icircumflex,	XK_Idiaeresis,
// 144
	XK_ETH,		XK_Ntilde,	XK_Ograve,	XK_Oacute,
	XK_Ocircumflex,	XK_Otilde,	XK_Odiaeresis,	XK_Ugrave,
	XK_Uacute,	XK_Ucircumflex,	XK_Udiaeresis,	XK_Yacute,
	XK_THORN,	XK_mu,		XK_multiply,	XK_division,
// 160
	XK_copyright,	XK_exclamdown,	XK_cent,	XK_sterling,
	UK(fraction),	XK_yen,		UK(fhook),	XK_section,
	XK_currency,	XK_rightsinglequotemark,
					XK_leftdoublequotemark,
							XK_guillemotleft,
	XK_leftanglebracket,
			XK_rightanglebracket,
					UK(filigature),	UK(flligature),
// 176
	XK_registered,	XK_endash,	XK_dagger,	XK_doubledagger,
	XK_periodcentered,XK_brokenbar,	XK_paragraph,	UK(bullet),
	XK_singlelowquotemark,
			XK_doublelowquotemark,
					XK_rightdoublequotemark,
							XK_guillemotright,
	XK_ellipsis,	UK(permille),	XK_notsign,	XK_questiondown,
// 192
	XK_onesuperior,	XK_dead_grave,	XK_dead_acute,	XK_dead_circumflex,
	XK_dead_tilde,	XK_dead_macron,	XK_dead_breve,	XK_dead_abovedot,
	XK_dead_diaeresis,
			XK_twosuperior,	XK_dead_abovering,
							XK_dead_cedilla,
	XK_threesuperior,
			XK_dead_doubleacute,
					XK_dead_ogonek,	XK_dead_caron,
// 208
	XK_emdash,	XK_plusminus,	XK_onequarter,	XK_onehalf,
	XK_threequarters,
			XK_agrave,	XK_aacute,	XK_acircumflex,
	XK_atilde,	XK_adiaeresis,	XK_aring,	XK_ccedilla,
	XK_egrave,	XK_eacute,	XK_ecircumflex,	XK_ediaeresis,
// 224
	XK_igrave,	XK_AE,		XK_iacute,	XK_ordfeminine,
	XK_icircumflex,	XK_idiaeresis,	XK_eth,		XK_ntilde,
	XK_Lstroke,	XK_Ooblique,	XK_OE,		XK_masculine,
	XK_ograve,	XK_oacute,	XK_ocircumflex, XK_otilde,
// 240
	XK_odiaeresis,	XK_ae,		XK_ugrave,	XK_uacute,
	XK_ucircumflex,	XK_idotless,	XK_udiaeresis,	XK_ygrave,
	XK_lstroke,	XK_ooblique,	XK_oe,		XK_ssharp,
	XK_thorn,	XK_ydiaeresis,	NoSymbol,	NoSymbol,
  };

#define MIN_SYMBOL      0xAC
static KeySym const symbol_to_x[] = {
    XK_Left,        XK_Up,          XK_Right,      XK_Down
  };
static int const NUM_SYMBOL = sizeof(symbol_to_x) / sizeof(symbol_to_x[0]);

#define MIN_FUNCKEY     0x20
static KeySym const funckey_to_x[] = {
    XK_F1,          XK_F2,          XK_F3,          XK_F4,
    XK_F5,          XK_F6,          XK_F7,          XK_F8,
    XK_F9,          XK_F10,         XK_F11,         XK_F12,
    XK_Insert,      XK_Delete,      XK_Home,        XK_End,
    XK_Page_Up,     XK_Page_Down,   XK_F13,         XK_F14,
    XK_F15
  };
static int const NUM_FUNCKEY = sizeof(funckey_to_x) / sizeof(funckey_to_x[0]);

typedef struct {
    KeySym      normalSym;
    KeySym      keypadSym;
} darwinKeyPad_t;

static darwinKeyPad_t const normal_to_keypad[] = {
    { XK_0,         XK_KP_0 },
    { XK_1,         XK_KP_1 },
    { XK_2,         XK_KP_2 },
    { XK_3,         XK_KP_3 },
    { XK_4,         XK_KP_4 },
    { XK_5,         XK_KP_5 },
    { XK_6,         XK_KP_6 },
    { XK_7,         XK_KP_7 },
    { XK_8,         XK_KP_8 },
    { XK_9,         XK_KP_9 },
    { XK_equal,     XK_KP_Equal },
    { XK_asterisk,  XK_KP_Multiply },
    { XK_plus,      XK_KP_Add },
    { XK_comma,     XK_KP_Separator },
    { XK_minus,     XK_KP_Subtract },
    { XK_period,    XK_KP_Decimal },
    { XK_slash,     XK_KP_Divide }
};

static int const NUM_KEYPAD = sizeof(normal_to_keypad) / sizeof(normal_to_keypad[0]);

/* Table of keycode->keysym mappings we use to fallback on for important
   keys that are often not in the Unicode mapping. */

const static struct {
    unsigned short keycode;
    KeySym keysym;
} known_keys[] = {
    {55,  XK_Meta_L},
    {56,  XK_Shift_L},
    {57,  XK_Caps_Lock},
    {58,  XK_Alt_L},
    {59,  XK_Control_L},

    {60,  XK_Shift_R},
    {61,  XK_Alt_R},
    {62,  XK_Control_R},
    {63,  XK_Meta_R},

    {122, XK_F1},
    {120, XK_F2},
    {99,  XK_F3},
    {118, XK_F4},
    {96,  XK_F5},
    {97,  XK_F6},
    {98,  XK_F7},
    {100, XK_F8},
    {101, XK_F9},
    {109, XK_F10},
    {103, XK_F11},
    {111, XK_F12},
    {105, XK_F13},
    {107, XK_F14},
    {113, XK_F15},
};

/* Table of keycode->old,new-keysym mappings we use to fixup the numeric
   keypad entries. */

const static struct {
    unsigned short keycode;
    KeySym normal, keypad;
} known_numeric_keys[] = {
    {65, XK_period, XK_KP_Decimal},
    {67, XK_asterisk, XK_KP_Multiply},
    {69, XK_plus, XK_KP_Add},
    {75, XK_slash, XK_KP_Divide},
    {76, 0x01000003, XK_KP_Enter},
    {78, XK_minus, XK_KP_Subtract},
    {81, XK_equal, XK_KP_Equal},
    {82, XK_0, XK_KP_0},
    {83, XK_1, XK_KP_1},
    {84, XK_2, XK_KP_2},
    {85, XK_3, XK_KP_3},
    {86, XK_4, XK_KP_4},
    {87, XK_5, XK_KP_5},
    {88, XK_6, XK_KP_6},
    {89, XK_7, XK_KP_7},
    {91, XK_8, XK_KP_8},
    {92, XK_9, XK_KP_9},
};

/* Table mapping normal keysyms to their dead equivalents.
   FIXME: all the unicode keysyms (apart from circumflex) were guessed. */

const static struct {
    KeySym normal, dead;
} dead_keys[] = {
    {XK_grave, XK_dead_grave},
    {XK_acute, XK_dead_acute},
    {XK_asciicircum, XK_dead_circumflex},
    {UKEYSYM (0x2c6), XK_dead_circumflex},	/* MODIFIER LETTER CIRCUMFLEX ACCENT */
    {XK_asciitilde, XK_dead_tilde},
    {UKEYSYM (0x2dc), XK_dead_tilde},		/* SMALL TILDE */
    {XK_macron, XK_dead_macron},
    {XK_breve, XK_dead_breve},
    {XK_abovedot, XK_dead_abovedot},
    {XK_diaeresis, XK_dead_diaeresis},
    {UKEYSYM (0x2da), XK_dead_abovering},	/* DOT ABOVE */
    {XK_doubleacute, XK_dead_doubleacute},
    {XK_caron, XK_dead_caron},
    {XK_cedilla, XK_dead_cedilla},
    {XK_ogonek, XK_dead_ogonek},
    {UKEYSYM (0x269), XK_dead_iota},		/* LATIN SMALL LETTER IOTA */
    {UKEYSYM (0x2ec), XK_dead_voiced_sound},	/* MODIFIER LETTER VOICING */
/*  {XK_semivoiced_sound, XK_dead_semivoiced_sound}, */
    {UKEYSYM (0x323), XK_dead_belowdot},	/* COMBINING DOT BELOW */
    {UKEYSYM (0x309), XK_dead_hook}, 		/* COMBINING HOOK ABOVE */
    {UKEYSYM (0x31b), XK_dead_horn},		/* COMBINING HORN */
};

darwinKeyboardInfo keyInfo;

static void DarwinChangeKeyboardControl( DeviceIntPtr device, KeybdCtrl *ctrl )
{
	// FIXME: to be implemented
    // keyclick, bell volume / pitch, autorepead, LED's
}

//-----------------------------------------------------------------------------
// Utility functions to help parse Darwin keymap
//-----------------------------------------------------------------------------

/*
 * DarwinBuildModifierMaps
 *      Use the keyMap field of keyboard info structure to populate
 *      the modMap and modifierKeycodes fields.
 */
static void DarwinBuildModifierMaps(darwinKeyboardInfo *info) {
    int i;
    KeySym *k;

    memset(info->modMap, NoSymbol, sizeof(info->modMap));
    memset(info->modifierKeycodes, 0, sizeof(info->modifierKeycodes));

    for (i = 0; i < NUM_KEYCODES; i++) {
        k = info->keyMap + i * GLYPHS_PER_KEY;

        switch (*k) {
            case XK_Shift_L:
                info->modifierKeycodes[NX_MODIFIERKEY_SHIFT][0] = i;
                info->modMap[MIN_KEYCODE + i] = ShiftMask;
                break;

            case XK_Shift_R:
#ifdef NX_MODIFIERKEY_RSHIFT
                info->modifierKeycodes[NX_MODIFIERKEY_RSHIFT][0] = i;
#else
                info->modifierKeycodes[NX_MODIFIERKEY_SHIFT][0] = i;
#endif
                info->modMap[MIN_KEYCODE + i] = ShiftMask;
                break;

            case XK_Control_L:
                info->modifierKeycodes[NX_MODIFIERKEY_CONTROL][0] = i;
                info->modMap[MIN_KEYCODE + i] = ControlMask;
                break;

            case XK_Control_R:
#ifdef NX_MODIFIERKEY_RCONTROL
                info->modifierKeycodes[NX_MODIFIERKEY_RCONTROL][0] = i;
#else
                info->modifierKeycodes[NX_MODIFIERKEY_CONTROL][0] = i;
#endif
                info->modMap[MIN_KEYCODE + i] = ControlMask;
                break;

            case XK_Caps_Lock:
                info->modifierKeycodes[NX_MODIFIERKEY_ALPHALOCK][0] = i;
                info->modMap[MIN_KEYCODE + i] = LockMask;
                break;

            case XK_Alt_L:
                info->modifierKeycodes[NX_MODIFIERKEY_ALTERNATE][0] = i;
                info->modMap[MIN_KEYCODE + i] = Mod1Mask;
                *k = XK_Mode_switch; // Yes, this is ugly.  This needs to be cleaned up when we integrate quartzKeyboard with this code and refactor.
                break;

            case XK_Alt_R:
#ifdef NX_MODIFIERKEY_RALTERNATE
                info->modifierKeycodes[NX_MODIFIERKEY_RALTERNATE][0] = i;
#else
                info->modifierKeycodes[NX_MODIFIERKEY_ALTERNATE][0] = i;
#endif
                *k = XK_Mode_switch; // Yes, this is ugly.  This needs to be cleaned up when we integrate quartzKeyboard with this code and refactor.
                info->modMap[MIN_KEYCODE + i] = Mod1Mask;
                break;

            case XK_Mode_switch:
                info->modMap[MIN_KEYCODE + i] = Mod1Mask;
                break;

            case XK_Meta_L:
                info->modifierKeycodes[NX_MODIFIERKEY_COMMAND][0] = i;
                info->modMap[MIN_KEYCODE + i] = Mod2Mask;
                break;

            case XK_Meta_R:
#ifdef NX_MODIFIERKEY_RCOMMAND
                info->modifierKeycodes[NX_MODIFIERKEY_RCOMMAND][0] = i;
#else
                info->modifierKeycodes[NX_MODIFIERKEY_COMMAND][0] = i;
#endif
                info->modMap[MIN_KEYCODE + i] = Mod2Mask;
                break;

            case XK_Num_Lock:
                info->modMap[MIN_KEYCODE + i] = Mod3Mask;
                break;
        }
    }
}

/*
 * DarwinLoadKeyboardMapping
 *  Load the keyboard map from a file or system and convert
 *  it to an equivalent X keyboard map and modifier map.
 */
static void DarwinLoadKeyboardMapping(KeySymsRec *keySyms) {
    memset(keyInfo.keyMap, 0, sizeof(keyInfo.keyMap));

    if (!QuartzReadSystemKeymap(&keyInfo)) {
        FatalError("Could not build a valid keymap.");
    }

    DarwinBuildModifierMaps(&keyInfo);

    keySyms->map        = keyInfo.keyMap;
    keySyms->mapWidth   = GLYPHS_PER_KEY;
    keySyms->minKeyCode = MIN_KEYCODE;
    keySyms->maxKeyCode = MAX_KEYCODE;
}

void QuartzXkbUpdate(DeviceIntPtr pDev) {
#ifdef XQUARTZ_USE_XKB
	SendDeviceMappingNotify(serverClient, MappingKeyboard, 
		pDev->key->curKeySyms.minKeyCode, 
		pDev->key->curKeySyms.maxKeyCode - pDev->key->curKeySyms.minKeyCode, pDev);
	SendDeviceMappingNotify(serverClient, MappingModifier, 0, 0, pDev);
	SwitchCoreKeyboard(pDev);   
#endif
}

/*
 * DarwinKeyboardInit
 *      Get the Darwin keyboard map and compute an equivalent
 *      X keyboard map and modifier map. Set the new keyboard
 *      device structure.
 */
void DarwinKeyboardInit(DeviceIntPtr pDev) {
    KeySymsRec          keySyms;

    // Open a shared connection to the HID System.
    // Note that the Event Status Driver is really just a wrapper
    // for a kIOHIDParamConnectType connection.
    assert( darwinParamConnect = NXOpenEventStatus() );

    DarwinLoadKeyboardMapping(&keySyms);
    /* Initialize the seed, so we don't reload the keymap unnecessarily
       (and possibly overwrite xinitrc changes) */
    QuartzSystemKeymapSeed();

#ifdef XQUARTZ_USE_XKB
	XkbComponentNamesRec names;
	bzero(&names, sizeof(names));
    /* We need to really have rules... or something... */
    XkbSetRulesDflts("base", "pc105", "us", NULL, NULL);
    assert(XkbInitKeyboardDeviceStruct(pDev, &names, &keySyms, keyInfo.modMap,
                                        QuartzBell, DarwinChangeKeyboardControl));
	assert(SetKeySymsMap(&pDev->key->curKeySyms, &keySyms));
	assert(keyInfo.modMap!=NULL);
	assert(pDev->key->modifierMap!=NULL);
	memcpy(pDev->key->modifierMap, keyInfo.modMap, sizeof(keyInfo.modMap));
	
	QuartzXkbUpdate(pDev);
#else
    assert( InitKeyboardDeviceStruct( (DevicePtr)pDev, &keySyms,
                                      keyInfo.modMap, QuartzBell,
                                      DarwinChangeKeyboardControl ));
    SwitchCoreKeyboard(pDev);
#endif
}


void DarwinKeyboardReloadHandler(int screenNum, xEventPtr xe, DeviceIntPtr pDev, int nevents) {
    if (pDev == NULL) pDev = darwinKeyboard;
    
    DEBUG_LOG("DarwinKeyboardReloadHandler(%p)\n", pDev);

#ifdef XQUARTZ_USE_XKB
    QuartzXkbUpdate(pDev);
#else
    KeySymsRec keySyms;
    DarwinLoadKeyboardMapping(&keySyms);

    if (pDev->key) {
        if (pDev->key->curKeySyms.map) xfree(pDev->key->curKeySyms.map);
        if (pDev->key->modifierKeyMap) xfree(pDev->key->modifierKeyMap);
        xfree(pDev->key);
    }
    
    if (!InitKeyClassDeviceStruct(pDev, &keySyms, keyInfo.modMap)) {
        DEBUG_LOG("InitKeyClassDeviceStruct failed\n");
        return;
    }

    SendMappingNotify(MappingKeyboard, MIN_KEYCODE, NUM_KEYCODES, 0);
    SendMappingNotify(MappingModifier, 0, 0, 0);
#endif
}


//-----------------------------------------------------------------------------
// Modifier translation functions
//
// There are three different ways to specify a Mac modifier key:
// keycode - specifies hardware key, read from keymapping
// key     - NX_MODIFIERKEY_*, really an index
// mask    - NX_*MASK, mask for modifier flags in event record
// Left and right side have different keycodes but the same key and mask.
//-----------------------------------------------------------------------------

/*
 * DarwinModifierNXKeyToNXKeycode
 *      Return the keycode for an NX_MODIFIERKEY_* modifier.
 *      side = 0 for left or 1 for right.
 *      Returns 0 if key+side is not a known modifier.
 */
int DarwinModifierNXKeyToNXKeycode(int key, int side) {
    return keyInfo.modifierKeycodes[key][side];
}

/*
 * DarwinModifierNXKeycodeToNXKey
 *      Returns -1 if keycode+side is not a modifier key
 *      outSide may be NULL, else it gets 0 for left and 1 for right.
 */
int DarwinModifierNXKeycodeToNXKey(unsigned char keycode, int *outSide) {
    int key, side;

    keycode += MIN_KEYCODE;
    // search modifierKeycodes for this keycode+side
    for (key = 0; key < NX_NUMMODIFIERS; key++) {
        for (side = 0; side <= 1; side++) {
            if (keyInfo.modifierKeycodes[key][side] == keycode) break;
        }
    }
    if (key == NX_NUMMODIFIERS) return -1;
    if (outSide) *outSide = side;
    return key;
}

/*
 * DarwinModifierNXMaskToNXKey
 *      Returns -1 if mask is not a known modifier mask.
 */
int DarwinModifierNXMaskToNXKey(int mask) {
    switch (mask) {
        case NX_ALPHASHIFTMASK:       return NX_MODIFIERKEY_ALPHALOCK;
        case NX_SHIFTMASK:            return NX_MODIFIERKEY_SHIFT;
#ifdef NX_DEVICELSHIFTKEYMASK
        case NX_DEVICELSHIFTKEYMASK:  return NX_MODIFIERKEY_SHIFT;
        case NX_DEVICERSHIFTKEYMASK:  return NX_MODIFIERKEY_RSHIFT;
#endif
        case NX_CONTROLMASK:          return NX_MODIFIERKEY_CONTROL;
#ifdef NX_DEVICELCTLKEYMASK
        case NX_DEVICELCTLKEYMASK:    return NX_MODIFIERKEY_CONTROL;
        case NX_DEVICERCTLKEYMASK:    return NX_MODIFIERKEY_RCONTROL;
#endif
        case NX_ALTERNATEMASK:        return NX_MODIFIERKEY_ALTERNATE;
#ifdef NX_DEVICELALTKEYMASK
        case NX_DEVICELALTKEYMASK:    return NX_MODIFIERKEY_ALTERNATE;
        case NX_DEVICERALTKEYMASK:    return NX_MODIFIERKEY_RALTERNATE;
#endif
        case NX_COMMANDMASK:          return NX_MODIFIERKEY_COMMAND;
#ifdef NX_DEVICELCMDKEYMASK
        case NX_DEVICELCMDKEYMASK:    return NX_MODIFIERKEY_COMMAND;
        case NX_DEVICERCMDKEYMASK:    return NX_MODIFIERKEY_RCOMMAND;
#endif
        case NX_NUMERICPADMASK:       return NX_MODIFIERKEY_NUMERICPAD;
        case NX_HELPMASK:             return NX_MODIFIERKEY_HELP;
        case NX_SECONDARYFNMASK:      return NX_MODIFIERKEY_SECONDARYFN;
    }
    return -1;
}

/*
 * DarwinModifierNXKeyToNXMask
 *      Returns 0 if key is not a known modifier key.
 */
int DarwinModifierNXKeyToNXMask(int key) {
    switch (key) {
        case NX_MODIFIERKEY_ALPHALOCK:   return NX_ALPHASHIFTMASK;
#ifdef NX_DEVICELSHIFTKEYMASK
        case NX_MODIFIERKEY_SHIFT:       return NX_DEVICELSHIFTKEYMASK;
        case NX_MODIFIERKEY_RSHIFT:      return NX_DEVICERSHIFTKEYMASK;
        case NX_MODIFIERKEY_CONTROL:     return NX_DEVICELCTLKEYMASK;
        case NX_MODIFIERKEY_RCONTROL:    return NX_DEVICERCTLKEYMASK;
        case NX_MODIFIERKEY_ALTERNATE:   return NX_DEVICELALTKEYMASK;
        case NX_MODIFIERKEY_RALTERNATE:  return NX_DEVICERALTKEYMASK;
        case NX_MODIFIERKEY_COMMAND:     return NX_DEVICELCMDKEYMASK;
        case NX_MODIFIERKEY_RCOMMAND:    return NX_DEVICERCMDKEYMASK;
#else
        case NX_MODIFIERKEY_SHIFT:       return NX_SHIFTMASK;
        case NX_MODIFIERKEY_CONTROL:     return NX_CONTROLMASK;
        case NX_MODIFIERKEY_ALTERNATE:   return NX_ALTERNATEMASK;
        case NX_MODIFIERKEY_COMMAND:     return NX_COMMANDMASK;
#endif
        case NX_MODIFIERKEY_NUMERICPAD:  return NX_NUMERICPADMASK;
        case NX_MODIFIERKEY_HELP:        return NX_HELPMASK;
        case NX_MODIFIERKEY_SECONDARYFN: return NX_SECONDARYFNMASK;
    }
    return 0;
}

/*
 * DarwinModifierStringToNXMask
 *      Returns 0 if string is not a known modifier.
 */
int DarwinModifierStringToNXMask(const char *str, int separatelr) {
#ifdef NX_DEVICELSHIFTKEYMASK
    if(separatelr) {
        if (!strcasecmp(str, "shift"))    return NX_DEVICELSHIFTKEYMASK | NX_DEVICERSHIFTKEYMASK;
        if (!strcasecmp(str, "control"))  return NX_DEVICELCTLKEYMASK | NX_DEVICERCTLKEYMASK;
        if (!strcasecmp(str, "option"))   return NX_DEVICELALTKEYMASK | NX_DEVICERALTKEYMASK;
        if (!strcasecmp(str, "alt"))   return NX_DEVICELALTKEYMASK | NX_DEVICERALTKEYMASK;
        if (!strcasecmp(str, "command"))  return NX_DEVICELCMDKEYMASK | NX_DEVICERCMDKEYMASK;
        if (!strcasecmp(str, "lshift"))   return NX_DEVICELSHIFTKEYMASK;
        if (!strcasecmp(str, "rshift"))   return NX_DEVICERSHIFTKEYMASK;
        if (!strcasecmp(str, "lcontrol")) return NX_DEVICELCTLKEYMASK;
        if (!strcasecmp(str, "rcontrol")) return NX_DEVICERCTLKEYMASK;
        if (!strcasecmp(str, "loption"))  return NX_DEVICELALTKEYMASK;
        if (!strcasecmp(str, "roption"))  return NX_DEVICERALTKEYMASK;
        if (!strcasecmp(str, "lalt"))  return NX_DEVICELALTKEYMASK;
        if (!strcasecmp(str, "ralt"))  return NX_DEVICERALTKEYMASK;
        if (!strcasecmp(str, "lcommand")) return NX_DEVICELCMDKEYMASK;
        if (!strcasecmp(str, "rcommand")) return NX_DEVICERCMDKEYMASK;
    } else {
#endif
        if (!strcasecmp(str, "shift"))    return NX_SHIFTMASK;
        if (!strcasecmp(str, "control"))  return NX_CONTROLMASK;
        if (!strcasecmp(str, "option"))   return NX_ALTERNATEMASK;
        if (!strcasecmp(str, "alt"))   return NX_ALTERNATEMASK;
        if (!strcasecmp(str, "command"))  return NX_COMMANDMASK;
        if (!strcasecmp(str, "lshift"))   return NX_SHIFTMASK;
        if (!strcasecmp(str, "rshift"))   return NX_SHIFTMASK;
        if (!strcasecmp(str, "lcontrol")) return NX_CONTROLMASK;
        if (!strcasecmp(str, "rcontrol")) return NX_CONTROLMASK;
        if (!strcasecmp(str, "loption"))  return NX_ALTERNATEMASK;
        if (!strcasecmp(str, "roption"))  return NX_ALTERNATEMASK;
        if (!strcasecmp(str, "lalt"))  return NX_ALTERNATEMASK;
        if (!strcasecmp(str, "ralt"))  return NX_ALTERNATEMASK;
        if (!strcasecmp(str, "lcommand")) return NX_COMMANDMASK;
        if (!strcasecmp(str, "rcommand")) return NX_COMMANDMASK;
#ifdef NX_DEVICELSHIFTKEYMASK
    }
#endif
    if (!strcasecmp(str, "lock"))     return NX_ALPHASHIFTMASK;
    if (!strcasecmp(str, "fn"))       return NX_SECONDARYFNMASK;
    if (!strcasecmp(str, "help"))     return NX_HELPMASK;
    if (!strcasecmp(str, "numlock"))  return NX_NUMERICPADMASK;
    return 0;
}

/*
 * LegalModifier
 *      This allows the ddx layer to prevent some keys from being remapped
 *      as modifier keys.
 */
Bool LegalModifier(unsigned int key, DeviceIntPtr pDev)
{
    return 1;
}

/* TODO: Not thread safe */
unsigned int QuartzSystemKeymapSeed(void) {
    static unsigned int seed = 0;
//#if defined(__x86_64__) || defined(__ppc64__)
#if 1
    static TISInputSourceRef last_key_layout = NULL;
    TISInputSourceRef key_layout;

    key_layout = TISCopyCurrentKeyboardLayoutInputSource();

    if(last_key_layout) {
        if (CFEqual(key_layout, last_key_layout)) {
            CFRelease(key_layout);
        } else {
            seed++;
            CFRelease(last_key_layout);
            last_key_layout = key_layout;
        }
    } else {
        last_key_layout = key_layout;
    }
#else
    static KeyboardLayoutRef last_key_layout;
    KeyboardLayoutRef key_layout;

    KLGetCurrentKeyboardLayout (&key_layout);
    if (key_layout != last_key_layout)
        seed++;
    last_key_layout = key_layout;
#endif
    return seed;
}

static inline UniChar macroman2ucs(unsigned char c) {
    /* Precalculated table mapping MacRoman-128 to Unicode. Generated
       by creating single element CFStringRefs then extracting the
       first character. */

    static const unsigned short table[128] = {
        0xc4, 0xc5, 0xc7, 0xc9, 0xd1, 0xd6, 0xdc, 0xe1,
        0xe0, 0xe2, 0xe4, 0xe3, 0xe5, 0xe7, 0xe9, 0xe8,
        0xea, 0xeb, 0xed, 0xec, 0xee, 0xef, 0xf1, 0xf3,
        0xf2, 0xf4, 0xf6, 0xf5, 0xfa, 0xf9, 0xfb, 0xfc,
        0x2020, 0xb0, 0xa2, 0xa3, 0xa7, 0x2022, 0xb6, 0xdf,
        0xae, 0xa9, 0x2122, 0xb4, 0xa8, 0x2260, 0xc6, 0xd8,
        0x221e, 0xb1, 0x2264, 0x2265, 0xa5, 0xb5, 0x2202, 0x2211,
        0x220f, 0x3c0, 0x222b, 0xaa, 0xba, 0x3a9, 0xe6, 0xf8,
        0xbf, 0xa1, 0xac, 0x221a, 0x192, 0x2248, 0x2206, 0xab,
        0xbb, 0x2026, 0xa0, 0xc0, 0xc3, 0xd5, 0x152, 0x153,
        0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0xf7, 0x25ca,
        0xff, 0x178, 0x2044, 0x20ac, 0x2039, 0x203a, 0xfb01, 0xfb02,
        0x2021, 0xb7, 0x201a, 0x201e, 0x2030, 0xc2, 0xca, 0xc1,
        0xcb, 0xc8, 0xcd, 0xce, 0xcf, 0xcc, 0xd3, 0xd4,
        0xf8ff, 0xd2, 0xda, 0xdb, 0xd9, 0x131, 0x2c6, 0x2dc,
        0xaf, 0x2d8, 0x2d9, 0x2da, 0xb8, 0x2dd, 0x2db, 0x2c7,
    };

    if (c < 128) return c;
    else         return table[c - 128];
}

static KeySym make_dead_key(KeySym in) {
    int i;

    for (i = 0; i < sizeof (dead_keys) / sizeof (dead_keys[0]); i++)
        if (dead_keys[i].normal == in) return dead_keys[i].dead;

    return in;
}

Bool QuartzReadSystemKeymap(darwinKeyboardInfo *info) {
#if !defined(__x86_64__) && !defined(__ppc64__)
    KeyboardLayoutRef key_layout;
#endif
    const void *chr_data = NULL;
    int num_keycodes = NUM_KEYCODES;
    UInt32 keyboard_type = 0;
    int is_uchr = 1, i, j;
    OSStatus err;
    KeySym *k;
    CFDataRef currentKeyLayoutDataRef = NULL;

    TISInputSourceRef currentKeyLayoutRef = TISCopyCurrentKeyboardLayoutInputSource();
    keyboard_type = LMGetKbdType();

    if (currentKeyLayoutRef) {
      currentKeyLayoutDataRef = (CFDataRef )TISGetInputSourceProperty(currentKeyLayoutRef, kTISPropertyUnicodeKeyLayoutData);
      if (currentKeyLayoutDataRef)
          chr_data = CFDataGetBytePtr(currentKeyLayoutDataRef);
    }

#if !defined(__x86_64__) && !defined(__ppc64__)
    if (chr_data == NULL) {
        ErrorF("X11.app: Error detected in determining keyboard layout.  If you are using an Apple-provided keyboard layout, please report this error at http://xquartz.macosforge.org and http://bugreport.apple.com\n");
        ErrorF("X11.app: Debug Info: keyboard_type=%u, currentKeyLayoutRef=%p, currentKeyLayoutDataRef=%p, chr_data=%p\n",
               (unsigned)keyboard_type, currentKeyLayoutRef, currentKeyLayoutDataRef, chr_data);

        KLGetCurrentKeyboardLayout (&key_layout);
        KLGetKeyboardLayoutProperty (key_layout, kKLuchrData, &chr_data);

        if(chr_data != NULL) {
            ErrorF("X11.app: Fallback succeeded, but this is still a bug.  Please report the above information.\n");
        }
    }

    if (chr_data == NULL) {
        ErrorF("X11.app: Debug Info: kKLuchrData fallback failed, trying kKLKCHRData.\n");
        ErrorF("If you are using a 3rd party keyboard layout, please see http://xquartz.macosforge.org/trac/ticket/154\n");
        KLGetKeyboardLayoutProperty (key_layout, kKLKCHRData, &chr_data);
        is_uchr = 0;
        num_keycodes = 128;
        
        if(chr_data != NULL) {
            ErrorF("X11.app: Fallback succeeded, but this is still a bug.  Please report the above information.\n");
        }
    }
#endif

    if (chr_data == NULL) {
      ErrorF ( "Couldn't get uchr or kchr resource\n");
      return FALSE;
    }
    
    /* Scan the keycode range for the Unicode character that each
       key produces in the four shift states. Then convert that to
       an X11 keysym (which may just the bit that says "this is
       Unicode" if it can't find the real symbol.) */

    for (i = 0; i < num_keycodes; i++) {
        static const int mods[4] = {0, MOD_SHIFT, MOD_OPTION,
                                    MOD_OPTION | MOD_SHIFT};

        k = info->keyMap + i * GLYPHS_PER_KEY;

        for (j = 0; j < 4; j++) {
            if (is_uchr)  {
                UniChar s[8];
                UniCharCount len;
                UInt32 dead_key_state = 0, extra_dead = 0;

                err = UCKeyTranslate (chr_data, i, kUCKeyActionDown,
                                      mods[j] >> 8, keyboard_type, 0,
                                      &dead_key_state, 8, &len, s);
                if (err != noErr) continue;

                if (len == 0 && dead_key_state != 0) {
                    /* Found a dead key. Work out which one it is, but
                       remembering that it's dead. */
                    err = UCKeyTranslate (chr_data, i, kUCKeyActionDown,
                                          mods[j] >> 8, keyboard_type,
                                          kUCKeyTranslateNoDeadKeysMask,
                                          &extra_dead, 8, &len, s);
                    if (err != noErr) continue;
                }

                if (len > 0 && s[0] != 0x0010) {
                    k[j] = ucs2keysym (s[0]);
                    if (dead_key_state != 0) k[j] = make_dead_key (k[j]);
                }
            } else { // kchr
	      UInt32 c, state = 0, state2 = 0;
                UInt16 code;

                code = i | mods[j];
                c = KeyTranslate (chr_data, code, &state);

                /* Dead keys are only processed on key-down, so ask
                   to translate those events. When we find a dead key,
                   translating the matching key up event will give
                   us the actual dead character. */

                if (state != 0)
                    c = KeyTranslate (chr_data, code | 128, &state2);

                /* Characters seem to be in MacRoman encoding. */

                if (c != 0 && c != 0x0010) {
                    k[j] = ucs2keysym (macroman2ucs (c & 255));

                    if (state != 0) k[j] = make_dead_key (k[j]);
                }
            }
        }
	
        if (k[3] == k[2]) k[3] = NoSymbol;
        if (k[2] == k[1]) k[2] = NoSymbol;
        if (k[1] == k[0]) k[1] = NoSymbol;
        if (k[0] == k[2] && k[1] == k[3]) k[2] = k[3] = NoSymbol;
    }

    /* Fix up some things that are normally missing.. */

    if (HACK_MISSING) {
        for (i = 0; i < sizeof (known_keys) / sizeof (known_keys[0]); i++) {
            k = info->keyMap + known_keys[i].keycode * GLYPHS_PER_KEY;

            if    (k[0] == NoSymbol && k[1] == NoSymbol
                && k[2] == NoSymbol && k[3] == NoSymbol)
	      k[0] = known_keys[i].keysym;
        }
    }

    /* And some more things. We find the right symbols for the numeric
       keypad, but not the KP_ keysyms. So try to convert known keycodes. */

    if (HACK_KEYPAD) {
        for (i = 0; i < sizeof (known_numeric_keys)
                        / sizeof (known_numeric_keys[0]); i++) {
            k = info->keyMap + known_numeric_keys[i].keycode * GLYPHS_PER_KEY;

            if (k[0] == known_numeric_keys[i].normal)
                k[0] = known_numeric_keys[i].keypad;
        }
    }
    if(currentKeyLayoutRef)	CFRelease(currentKeyLayoutRef);
    
    return TRUE;
}
