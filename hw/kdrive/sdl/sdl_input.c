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
#include "sdl_input.h"
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
#include <unistd.h>

#ifdef __ANDROID__
#include <SDL/SDL_screenkeyboard.h>
#include <android/log.h>

// DEBUG
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "XSDL", __VA_ARGS__)
#endif

static int sdlInputNotifyFd[2] = { -1, -1 };

static int mouseState = 0;

enum sdlKeyboardType_t { KB_NATIVE = 0, KB_BUILTIN = 1, KB_BOTH = 2 };
enum sdlKeyboardType_t sdlKeyboardType = KB_NATIVE;

static int unShiftKeysym( int * sym );

extern unsigned char SDL_android_keysym_to_scancode[SDLK_LAST]; // Internal SDL symbol

void sdlInitInput(void)
{
	if (getenv("XSDL_BUILTIN_KEYBOARD") != NULL)
	{
		sdlKeyboardType = atoi(getenv("XSDL_BUILTIN_KEYBOARD")) == KB_NATIVE ? KB_NATIVE :
						atoi(getenv("XSDL_BUILTIN_KEYBOARD")) == KB_BUILTIN ? KB_BUILTIN : KB_BOTH;
	}
	unsetenv("XSDL_BUILTIN_KEYBOARD");

	printf("sdlKeyboardType %d\n", sdlKeyboardType);
}

void sdlPollInput(void)
{
	static int buttonState=0;
	static int pressure = 0;
	static int shiftState = 0;
	SDL_Event event;
	int tmp;

	//printf("sdlPollInput() %d thread %d fd %d\n", SDL_GetTicks(), (int) pthread_self(), sdlInputNotifyFd[0]);
	while (read(sdlInputNotifyFd[0], &tmp, 1) == 1) { }

	while ( SDL_PollEvent(&event) )
	{
		switch (event.type)
		{
			case SDL_MOUSEMOTION:
				//printf("SDL_MOUSEMOTION x:y %d:%d buttons %d\n", event.motion.x, event.motion.y, mouseState);
				KdEnqueuePointerEvent(sdlPointer, mouseState, event.motion.x, event.motion.y, pressure);
				setScreenButtons();
				break;
			case SDL_MOUSEBUTTONDOWN:
				switch (event.button.button)
				{
					case SDL_BUTTON_LEFT:
						buttonState = KD_BUTTON_1;
						break;
					case SDL_BUTTON_MIDDLE:
						buttonState = KD_BUTTON_2;
						break;
					case SDL_BUTTON_RIGHT:
						buttonState = KD_BUTTON_3;
						break;
					case SDL_BUTTON_WHEELUP:
						buttonState = KD_BUTTON_4;
						break;
					case SDL_BUTTON_WHEELDOWN:
						buttonState = KD_BUTTON_5;
						break;
					/*
					case SDL_BUTTON_X1:
						buttonState = KD_BUTTON_6;
						break;
					case SDL_BUTTON_X2:
						buttonState = KD_BUTTON_7;
						break;
					*/
					default:
						//buttonState = 1 << (event.button.button - 1);
						break;
				}
				mouseState |= buttonState;
				KdEnqueuePointerEvent(sdlPointer, mouseState|KD_MOUSE_DELTA, 0, 0, pressure);
				break;
			case SDL_MOUSEBUTTONUP:
				switch (event.button.button)
				{
					case SDL_BUTTON_LEFT:
						buttonState = KD_BUTTON_1;
						pressure = 0;
						break;
					case SDL_BUTTON_MIDDLE:
						buttonState = KD_BUTTON_2;
						break;
					case SDL_BUTTON_RIGHT:
						buttonState = KD_BUTTON_3;
						break;
					case SDL_BUTTON_WHEELUP:
						buttonState = KD_BUTTON_4;
						break;
					case SDL_BUTTON_WHEELDOWN:
						buttonState = KD_BUTTON_5;
						break;
					/*
					case SDL_BUTTON_X1:
						buttonState = KD_BUTTON_6;
						break;
					case SDL_BUTTON_X2:
						buttonState = KD_BUTTON_7;
						break;
					*/
					default:
						//buttonState = 1 << (event.button.button - 1);
						break;
				}
				mouseState &= ~buttonState;
				KdEnqueuePointerEvent(sdlPointer, mouseState|KD_MOUSE_DELTA, 0, 0, pressure);
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				//printf("===> Key sym %d scancode %d unicode %d down %d shift %d %d z %d\n", event.key.keysym.sym, event.key.keysym.scancode, event.key.keysym.unicode, event.type == SDL_KEYDOWN, SDL_GetKeyState(NULL)[SDLK_LSHIFT], SDL_GetKeyState(NULL)[SDLK_RSHIFT], SDL_GetKeyState(NULL)[SDLK_z]);
#ifdef __ANDROID__
				if (event.key.keysym.sym == SDLK_HELP)
				{
					if(event.type == SDL_KEYUP)
					{
						// SDL_ANDROID_ToggleScreenKeyboardWithoutTextInput();
						static int keyboard = -1;
						keyboard++;
						//SDL_Delay(150);
						if (keyboard >= 2 || (sdlKeyboardType != KB_BOTH && keyboard >= 1))
						{
							if (SDL_IsScreenKeyboardShown(NULL))
							{
								SDL_HideScreenKeyboard(NULL);
								SDL_Flip(SDL_GetVideoSurface());
							}
							else
							{
								keyboard = 0;
							}
						}
						if (keyboard == 0)
						{
							//SDL_Delay(100);
							if (sdlKeyboardType == KB_NATIVE || sdlKeyboardType == KB_BOTH)
								SDL_ANDROID_ToggleScreenKeyboardWithoutTextInput();
							if (sdlKeyboardType == KB_BUILTIN)
								SDL_ANDROID_ToggleInternalScreenKeyboard(SDL_KEYBOARD_QWERTY);
							SDL_Flip(SDL_GetVideoSurface());
						}
						if (keyboard == 1 && sdlKeyboardType == KB_BOTH)
						{
							//SDL_Delay(100);
							SDL_ANDROID_ToggleInternalScreenKeyboard(SDL_KEYBOARD_QWERTY);
							SDL_Flip(SDL_GetVideoSurface());
						}
					}
					setScreenButtons();
				}
				else
#endif
				if (event.key.keysym.sym == SDLK_UNDO)
				{
					if(event.type == SDL_KEYUP)
					{
						// Send Ctrl-Z
						KdEnqueueKeyboardEvent (sdlKeyboard, SCANCODE_LCTRL, 0); // LCTRL
						KdEnqueueKeyboardEvent (sdlKeyboard, SCANCODE_Z, 0); // Z
						KdEnqueueKeyboardEvent (sdlKeyboard, SCANCODE_Z, 1); // Z
						KdEnqueueKeyboardEvent (sdlKeyboard, SCANCODE_LCTRL, 1); // LCTRL
					}
				}
				else if ((event.key.keysym.unicode & 0xFF80) != 0)
				{
					if (event.type == SDL_KEYDOWN)
					{
						send_unicode (event.key.keysym.unicode);
					}
				}
				else
				{
					if (event.key.keysym.unicode < SDLK_SPACE || event.key.keysym.unicode == event.key.keysym.sym)
					{
						KdEnqueueKeyboardEvent (sdlKeyboard, event.key.keysym.scancode, event.type == SDL_KEYUP);
						// SDL_GetKeyState(NULL)[SDLK_LSHIFT] is unreliable for some unknown reason
						if (event.key.keysym.sym == SDLK_LSHIFT)
						{
							if (event.type == SDL_KEYDOWN)
							{
								shiftState |= 0x1;
							}
							else
							{
								shiftState &= ~0x1;
							}
						}
						if (event.key.keysym.sym == SDLK_RSHIFT)
						{
							if (event.type == SDL_KEYDOWN)
							{
								shiftState |= 0x2;
							}
							else
							{
								shiftState &= ~0x2;
							}
						}
					}
					else
					{
						int unshifted = event.key.keysym.unicode;
						unShiftKeysym(&unshifted);
						//printf("===> shiftState %d event.key.keysym.unicode %d event.key.keysym.sym %d unshifted %d\n", shiftState, event.key.keysym.unicode, event.key.keysym.sym, unshifted);
						if (shiftState && unshifted == event.key.keysym.sym)
						{
							KdEnqueueKeyboardEvent (sdlKeyboard, event.key.keysym.scancode, event.type == SDL_KEYUP);
						}
						else
						{
							// Handle Italian physical keyboard, where Shift-7 produces '/' symbol
							if (unshifted == event.key.keysym.unicode)
							{
								if (shiftState & 0x1)
								{
									KdEnqueueKeyboardEvent (sdlKeyboard, SCANCODE_LSHIFT, 1);
								}
								if (shiftState & 0x2)
								{
									KdEnqueueKeyboardEvent (sdlKeyboard, SCANCODE_RSHIFT, 1);
								}
							}
							else
							{
								if (!shiftState)
								{
									KdEnqueueKeyboardEvent (sdlKeyboard, SCANCODE_LSHIFT, 0);
								}
							}
							//send_unicode (event.key.keysym.unicode);
							#ifdef __ANDROID__
							KdEnqueueKeyboardEvent (sdlKeyboard, SDL_android_keysym_to_scancode[event.key.keysym.unicode], 0);
							KdEnqueueKeyboardEvent (sdlKeyboard, SDL_android_keysym_to_scancode[event.key.keysym.unicode], 1);
							#endif
							if (unshifted == event.key.keysym.unicode)
							{
								if (shiftState & 0x1)
								{
									KdEnqueueKeyboardEvent (sdlKeyboard, SCANCODE_LSHIFT, 0);
								}
								if (shiftState & 0x2)
								{
									KdEnqueueKeyboardEvent (sdlKeyboard, SCANCODE_RSHIFT, 0);
								}
							}
							else
							{
								if (!shiftState)
								{
									KdEnqueueKeyboardEvent (sdlKeyboard, SCANCODE_LSHIFT, 1);
								}
							}
						}
					}
				}
				// Force SDL screen update, so SDL virtual on-screen buttons will change their images
				{
					SDL_Rect r = {0, 0, 1, 1};
					SDL_UpdateRects(SDL_GetVideoSurface(), 1, &r);
				}
				break;
			case SDL_JOYAXISMOTION:
				if (event.jaxis.which == 0 && event.jaxis.axis == 4 && pressure != event.jaxis.value)
				{
					pressure = event.jaxis.value;
					if (mouseState & KD_BUTTON_1)
						KdEnqueuePointerEvent(sdlPointer, mouseState|KD_MOUSE_DELTA, 0, 0, pressure);
				}
				break;
			case SDL_ACTIVEEVENT:
				// We need this to re-init OpenGL and redraw screen
				// And we need to also call this when OpenGL context was destroyed
				// Oherwise SDL will stuck and we will get a permanent black screen
				SDL_Flip(SDL_GetVideoSurface());
				break;

			case SDL_SYSWMEVENT:
				process_clipboard_event(&event.syswm);
				break;
			//case SDL_QUIT:
				/* this should never happen */
				//SDL_Quit(); // SDL_Quit() on Android is buggy
		}
	}
	/*
	if ( nextFullScreenRefresh && nextFullScreenRefresh < SDL_GetTicks() )
	{
		//printf("SDL_Flip from sdlPollInput");
		SDL_Flip(SDL_GetVideoSurface());
		nextFullScreenRefresh = 0;
	}
	*/
}

static int sdlEventNotifyCbk(const SDL_Event *event)
{
	// Called from random thread, not from main thread
	//printf("==> sdlEventNotifyCbk() event %d thread %d\n", event->type, (int) pthread_self());

	write(sdlInputNotifyFd[1], "1", 1);
	return 1;
}

int sdlGetInputNotifyFd(void)
{
	if ( sdlInputNotifyFd[0] != -1 )
		return sdlInputNotifyFd[0];

	if ( pipe2(sdlInputNotifyFd, O_NONBLOCK) != 0 )
	{
		printf("Error calling pipe()!");
		exit(1);
	}

	SDL_SetEventFilter(&sdlEventNotifyCbk);

	return sdlInputNotifyFd[0];
}

// Copied from SDL Android code checkShiftRequired()
int unShiftKeysym( int * sym )
{
	switch( *sym )
	{
		case '!': *sym = '1'; return 1;
		case '@': *sym = '2'; return 1;
		case '#': *sym = '3'; return 1;
		case '$': *sym = '4'; return 1;
		case '%': *sym = '5'; return 1;
		case '^': *sym = '6'; return 1;
		case '&': *sym = '7'; return 1;
		case '*': *sym = '8'; return 1;
		case '(': *sym = '9'; return 1;
		case ')': *sym = '0'; return 1;
		case '_': *sym = '-'; return 1;
		case '+': *sym = '='; return 1;
		case '|': *sym = '\\';return 1;
		case '<': *sym = ','; return 1;
		case '>': *sym = '.'; return 1;
		case '?': *sym = '/'; return 1;
		case ':': *sym = ';'; return 1;
		case '"': *sym = '\'';return 1;
		case '{': *sym = '['; return 1;
		case '}': *sym = ']'; return 1;
		case '~': *sym = '`'; return 1;
		default: if( *sym >= 'A' && *sym <= 'Z' ) { *sym += 'a' - 'A'; return 1; };
	}
	return 0;
}
