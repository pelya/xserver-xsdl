/*
 * Copyright © 2004 PillowElephantBadgerBankPond 
 * Copyright © 2014-2019 Sergii Pylypenko
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of PillowElephantBadgerBankPond not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.	PillowElephantBadgerBankPond makes no
 * representations about the suitability of this software for any purpose.	It
 * is provided "as is" without express or implied warranty.
 *
 * PillowElephantBadgerBankPond DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL PillowElephantBadgerBankPond BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * It's really not my fault - see it was the elephants!!
 *	- jaymz
 *
 */

#include "sdl_send_text.h"
#include "sdl_kdrive.h"
#include "sdl_screen_buttons.h"

#include <xorg-config.h>
#include "kdrive.h"
#include "dix.h"
#include <SDL/SDL.h>
#include <X11/keysym.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#ifdef __ANDROID__
#include <SDL/SDL_screenkeyboard.h>
#include <android/log.h>

// DEBUG
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "XSDL", __VA_ARGS__)
#endif

static Bool sdlScreenButtons = FALSE;

void initScreenButtons(void)
{
#ifdef __ANDROID__
	sdlScreenButtons = SDL_ANDROID_GetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_0);
#endif
}

void setScreenButtons(int mouseX)
{
#ifdef __ANDROID__
	//printf ("setScreenButtons: kbShown %d sdlScreenButtons %d alignLeft %d", SDL_IsScreenKeyboardShown(NULL), sdlScreenButtons, mouseX > (((unsigned)SDL_GetVideoSurface()->w) >> 3));

	if ( SDL_ANDROID_GetScreenKeyboardRedefinedByUser() )
		return;

	int kbShown = SDL_IsScreenKeyboardShown(NULL);
	if (!sdlScreenButtons)
	{
		if (SDL_ANDROID_GetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_0))
		{
			SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_0, 0);
			SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_1, 0);
			SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_2, 0);
			SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_3, 0);
		}
		return;
	}

	SDL_Rect pos;
	SDL_ANDROID_GetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_0, &pos);

	//printf ("setScreenButtons: pos %d %d shown %d", pos.x, pos.y, SDL_ANDROID_GetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_0));

	int alignLeft;
	alignLeft = mouseX > (((unsigned)SDL_GetVideoSurface()->w) >> 3);

	if (!kbShown && pos.y > 0 && (pos.x == 0) == alignLeft)
		return;

	if (kbShown && pos.y == 0 && (pos.x == 0) == alignLeft && SDL_ANDROID_GetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_0))
		return;

	int resolutionW;
	resolutionW = atoi(getenv("DISPLAY_RESOLUTION_WIDTH"));
	if (resolutionW <= 0)
		resolutionW = SDL_ListModes(NULL, 0)[0]->w;

	pos.w = (kbShown ? 60 : 40) * SDL_ListModes(NULL, 0)[0]->w / resolutionW;
	pos.h = SDL_ListModes(NULL, 0)[0]->h / 20;
	pos.x = alignLeft ? 0 : SDL_ListModes(NULL, 0)[0]->w - pos.w;
	pos.y = kbShown ? 0 : SDL_ListModes(NULL, 0)[0]->h - pos.h * 4;

	pos.y += pos.h;
	SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_0, 1);
	SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_0, &pos);
	SDL_ANDROID_SetScreenKeyboardButtonImagePos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_0, &pos);
	SDL_ANDROID_SetScreenKeyboardButtonStayPressedAfterTouch(SDL_ANDROID_SCREENKEYBOARD_BUTTON_0, 1);
	pos.y += pos.h;
	SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_1, 1);
	SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_1, &pos);
	SDL_ANDROID_SetScreenKeyboardButtonImagePos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_1, &pos);
	SDL_ANDROID_SetScreenKeyboardButtonStayPressedAfterTouch(SDL_ANDROID_SCREENKEYBOARD_BUTTON_1, 1);
	pos.y += pos.h;
	SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_2, 1);
	SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_2, &pos);
	SDL_ANDROID_SetScreenKeyboardButtonImagePos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_2, &pos);
	SDL_ANDROID_SetScreenKeyboardButtonStayPressedAfterTouch(SDL_ANDROID_SCREENKEYBOARD_BUTTON_2, 1);
	pos.y -= pos.h * 3;
	SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_3, 1);
	SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_3, &pos);
	SDL_ANDROID_SetScreenKeyboardButtonImagePos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_3, &pos);
	SDL_ANDROID_SetScreenKeyboardButtonStayPressedAfterTouch(SDL_ANDROID_SCREENKEYBOARD_BUTTON_3, 1);

	SDL_ANDROID_SetScreenKeyboardTransparency(255); // opaque

#endif
}

