/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/Keyboard.c,v 1.15 2003/01/04 20:20:22 paulo Exp $ */
/* 
 * 
 * Copyright (c) 1997  Metro Link Incorporated
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
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 * 
 */

/* View/edit this file with tab stops set to 4 */

#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"
#include "ctype.h"

extern LexRec val;

static xf86ConfigSymTabRec KeyboardTab[] =
{
	{ENDSECTION, "endsection"},
	{KPROTOCOL, "protocol"},
	{AUTOREPEAT, "autorepeat"},
	{XLEDS, "xleds"},
	{PANIX106, "panix106"},
	{XKBKEYMAP, "xkbkeymap"},
	{XKBCOMPAT, "xkbcompat"},
	{XKBTYPES, "xkbtypes"},
	{XKBKEYCODES, "xkbkeycodes"},
	{XKBGEOMETRY, "xkbgeometry"},
	{XKBSYMBOLS, "xkbsymbols"},
	{XKBDISABLE, "xkbdisable"},
	{XKBRULES, "xkbrules"},
	{XKBMODEL, "xkbmodel"},
	{XKBLAYOUT, "xkblayout"},
	{XKBVARIANT, "xkbvariant"},
	{XKBOPTIONS, "xkboptions"},
	/* The next two have become ServerFlags options */
	{VTINIT, "vtinit"},
	{VTSYSREQ, "vtsysreq"},
	/* Obsolete keywords */
	{SERVERNUM, "servernumlock"},
	{LEFTALT, "leftalt"},
	{RIGHTALT, "rightalt"},
	{RIGHTALT, "altgr"},
	{SCROLLLOCK_TOK, "scrolllock"},
	{RIGHTCTL, "rightctl"},
	{-1, ""},
};

/* Obsolete */
static xf86ConfigSymTabRec KeyMapTab[] =
{
	{CONF_KM_META, "meta"},
	{CONF_KM_COMPOSE, "compose"},
	{CONF_KM_MODESHIFT, "modeshift"},
	{CONF_KM_MODELOCK, "modelock"},
	{CONF_KM_SCROLLLOCK, "scrolllock"},
	{CONF_KM_CONTROL, "control"},
	{-1, ""},
};

#define CLEANUP xf86freeInputList

XF86ConfInputPtr
xf86parseKeyboardSection (void)
{
	char *s, *s1, *s2;
	int l;
	int token, ntoken;
	parsePrologue (XF86ConfInputPtr, XF86ConfInputRec)

	while ((token = xf86getToken (KeyboardTab)) != ENDSECTION)
	{
		switch (token)
		{
		case COMMENT:
			ptr->inp_comment = xf86addComment(ptr->inp_comment, val.str);
			break;
		case KPROTOCOL:
			if (xf86getSubToken (&(ptr->inp_comment)) != STRING)
				Error (QUOTE_MSG, "Protocol");
			ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
												xf86configStrdup("Protocol"),
												val.str);
			break;
		case AUTOREPEAT:
			if (xf86getSubToken (&(ptr->inp_comment)) != NUMBER)
				Error (AUTOREPEAT_MSG, NULL);
			s1 = xf86uLongToString(val.num);
			if (xf86getSubToken (&(ptr->inp_comment)) != NUMBER)
				Error (AUTOREPEAT_MSG, NULL);
			s2 = xf86uLongToString(val.num);
			l = strlen(s1) + 1 + strlen(s2) + 1;
			s = xf86confmalloc(l);
			sprintf(s, "%s %s", s1, s2);
			xf86conffree(s1);
			xf86conffree(s2);
			ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
												xf86configStrdup("AutoRepeat"), s);
			break;
		case XLEDS:
			if (xf86getSubToken (&(ptr->inp_comment)) != NUMBER)
				Error (XLEDS_MSG, NULL);
			s = xf86uLongToString(val.num);
			l = strlen(s) + 1;
			while ((token = xf86getSubToken (&(ptr->inp_comment))) == NUMBER)
			{
				s1 = xf86uLongToString(val.num);
				l += (1 + strlen(s1));
				s = xf86confrealloc(s, l);
				strcat(s, " ");
				strcat(s, s1);
				xf86conffree(s1);
			}
			xf86unGetToken (token);
			break;
		case SERVERNUM:
			xf86parseWarning(OBSOLETE_MSG, xf86tokenString());
			break;
		case LEFTALT:
		case RIGHTALT:
		case SCROLLLOCK_TOK:
		case RIGHTCTL:
			xf86parseWarning(OBSOLETE_MSG, xf86tokenString());
				break;
			ntoken = xf86getToken (KeyMapTab);
			switch (ntoken)
			{
			case EOF_TOKEN:
				xf86parseError (UNEXPECTED_EOF_MSG);
				CLEANUP (ptr);
				return (NULL);
				break;

			default:
				Error (INVALID_KEYWORD_MSG, xf86tokenString ());
				break;
			}
			break;
		case VTINIT:
			if (xf86getSubToken (&(ptr->inp_comment)) != STRING)
				Error (QUOTE_MSG, "VTInit");
			xf86parseWarning(MOVED_TO_FLAGS_MSG, "VTInit");
			break;
		case VTSYSREQ:
			xf86parseWarning(MOVED_TO_FLAGS_MSG, "VTSysReq");
			break;
		case XKBDISABLE:
			ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
												xf86configStrdup("XkbDisable"),
												NULL);
			break;
		case XKBKEYMAP:
			if (xf86getSubToken (&(ptr->inp_comment)) != STRING)
				Error (QUOTE_MSG, "XKBKeymap");
			ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
												xf86configStrdup("XkbKeymap"),
												val.str);
			break;
		case XKBCOMPAT:
			if (xf86getSubToken (&(ptr->inp_comment)) != STRING)
				Error (QUOTE_MSG, "XKBCompat");
			ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
												xf86configStrdup("XkbCompat"),
												val.str);
			break;
		case XKBTYPES:
			if (xf86getSubToken (&(ptr->inp_comment)) != STRING)
				Error (QUOTE_MSG, "XKBTypes");
			ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
												xf86configStrdup("XkbTypes"),
												val.str);
			break;
		case XKBKEYCODES:
			if (xf86getSubToken (&(ptr->inp_comment)) != STRING)
				Error (QUOTE_MSG, "XKBKeycodes");
			ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
												xf86configStrdup("XkbKeycodes"),
												val.str);
			break;
		case XKBGEOMETRY:
			if (xf86getSubToken (&(ptr->inp_comment)) != STRING)
				Error (QUOTE_MSG, "XKBGeometry");
			ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
												xf86configStrdup("XkbGeometry"),
												val.str);
			break;
		case XKBSYMBOLS:
			if (xf86getSubToken (&(ptr->inp_comment)) != STRING)
				Error (QUOTE_MSG, "XKBSymbols");
			ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
												xf86configStrdup("XkbSymbols"),
												val.str);
			break;
		case XKBRULES:
			if (xf86getSubToken (&(ptr->inp_comment)) != STRING)
				Error (QUOTE_MSG, "XKBRules");
			ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
												xf86configStrdup("XkbRules"),
												val.str);
			break;
		case XKBMODEL:
			if (xf86getSubToken (&(ptr->inp_comment)) != STRING)
				Error (QUOTE_MSG, "XKBModel");
			ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
												xf86configStrdup("XkbModel"),
												val.str);
			break;
		case XKBLAYOUT:
			if (xf86getSubToken (&(ptr->inp_comment)) != STRING)
				Error (QUOTE_MSG, "XKBLayout");
			ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
												xf86configStrdup("XkbLayout"),
												val.str);
			break;
		case XKBVARIANT:
			if (xf86getSubToken (&(ptr->inp_comment)) != STRING)
				Error (QUOTE_MSG, "XKBVariant");
			ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
												xf86configStrdup("XkbVariant"),
												val.str);
			break;
		case XKBOPTIONS:
			if (xf86getSubToken (&(ptr->inp_comment)) != STRING)
				Error (QUOTE_MSG, "XKBOptions");
			ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
												xf86configStrdup("XkbOptions"),
												val.str);
			break;
		case PANIX106:
			ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
												xf86configStrdup("Panix106"), NULL);
			break;
		case EOF_TOKEN:
			Error (UNEXPECTED_EOF_MSG, NULL);
			break;
		default:
			Error (INVALID_KEYWORD_MSG, xf86tokenString ());
			break;
		}
	}

	ptr->inp_identifier = xf86configStrdup(CONF_IMPLICIT_KEYBOARD);
	ptr->inp_driver = xf86configStrdup("keyboard");
	ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
										xf86configStrdup("CoreKeyboard"), NULL);

#ifdef DEBUG
	printf ("Keyboard section parsed\n");
#endif

	return ptr;
}

