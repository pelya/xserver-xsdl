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
//#define printf(...)
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "XSDL", __VA_ARGS__)
#endif

static Bool xsdlInit(KdCardInfo *);
static void xsdlFini(KdCardInfo *);
static Bool sdlScreenInit(KdScreenInfo *screen);
static Bool sdlFinishInitScreen(ScreenPtr pScreen);
static Bool sdlCreateRes(ScreenPtr pScreen);

static void sdlKeyboardFini(KdKeyboardInfo *ki);
static Status sdlKeyboardInit(KdKeyboardInfo *ki);
static Status sdlKeyboardEnable (KdKeyboardInfo *ki);
static void sdlKeyboardDisable (KdKeyboardInfo *ki);
static void sdlKeyboardLeds (KdKeyboardInfo *ki, int leds);
static void sdlKeyboardBell (KdKeyboardInfo *ki, int volume, int frequency, int duration);

static Bool sdlMouseInit(KdPointerInfo *pi);
static void sdlMouseFini(KdPointerInfo *pi);
static Status sdlMouseEnable (KdPointerInfo *pi);
static void sdlMouseDisable (KdPointerInfo *pi);
static Bool sdlScreenButtons = FALSE;
static void setScreenButtons(int mouseX);
static void sdlScreenBlockCallback(ScreenPtr pScreen, void *timeout);
enum sdlKeyboardType_t { KB_NATIVE = 0, KB_BUILTIN = 1, KB_BOTH = 2 };
enum sdlKeyboardType_t sdlKeyboardType = KB_NATIVE;

KdKeyboardInfo *sdlKeyboard = NULL;
KdPointerInfo *sdlPointer = NULL;

KdKeyboardDriver sdlKeyboardDriver = {
	.name = "keyboard",
	.Init = sdlKeyboardInit,
	.Fini = sdlKeyboardFini,
	.Enable = sdlKeyboardEnable,
	.Disable = sdlKeyboardDisable,
	.Leds = sdlKeyboardLeds,
	.Bell = sdlKeyboardBell,
};

KdPointerDriver sdlMouseDriver = {
	.name = "mouse",
	.Init = sdlMouseInit,
	.Fini = sdlMouseFini,
	.Enable = sdlMouseEnable,
	.Disable = sdlMouseDisable,
};


KdCardFuncs sdlFuncs = {
	.cardinit = xsdlInit,
	.cardfini = xsdlFini,
	.scrinit = sdlScreenInit,
	.finishInitScreen = sdlFinishInitScreen,
	.createRes = sdlCreateRes,
};

int mouseState = 0;

enum { NUMRECTS = 32, FULLSCREEN_REFRESH_TIME = 1000 };
//Uint32 nextFullScreenRefresh = 0;

typedef struct
{
	SDL_Surface *screen;
	Rotation randr;
	Bool shadow;
	ScreenBlockHandlerProcPtr screenBlockCallback;
} SdlDriver;

//#undef RANDR

static Bool sdlMapFramebuffer (KdScreenInfo *screen)
{
	SdlDriver			*driver = screen->driver;
	KdPointerMatrix		m;

	if (driver->randr != RR_Rotate_0)
		driver->shadow = TRUE;
	else
		driver->shadow = FALSE;

	KdComputePointerMatrix (&m, driver->randr, screen->width, screen->height);

	KdSetPointerMatrix (&m);

	screen->width = driver->screen->w;
	screen->height = driver->screen->h;

	printf("%s: shadow %d\n", __func__, driver->shadow);

	if (driver->shadow)
	{
		if (!KdShadowFbAlloc (screen,
							  driver->randr & (RR_Rotate_90|RR_Rotate_270)))
			return FALSE;
	}
	else
	{
		screen->fb.byteStride = driver->screen->pitch;
		screen->fb.pixelStride = driver->screen->w;
		screen->fb.frameBuffer = (CARD8 *) (driver->screen->pixels);
	}

	return TRUE;
}

static void
sdlSetScreenSizes (ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	KdScreenInfo		*screen = pScreenPriv->screen;
	SdlDriver			*driver = screen->driver;

	if (driver->randr & (RR_Rotate_0|RR_Rotate_180))
	{
		pScreen->width = driver->screen->w;
		pScreen->height = driver->screen->h;
		pScreen->mmWidth = screen->width_mm;
		pScreen->mmHeight = screen->height_mm;
	}
	else
	{
		pScreen->width = driver->screen->h;
		pScreen->height = driver->screen->w;
		pScreen->mmWidth = screen->height_mm;
		pScreen->mmHeight = screen->width_mm;
	}
}

static Bool
sdlUnmapFramebuffer (KdScreenInfo *screen)
{
	KdShadowFbFree (screen);
	return TRUE;
}

static Bool sdlScreenInit(KdScreenInfo *screen)
{
	SdlDriver *driver=calloc(1, sizeof(SdlDriver));
	printf("%s\n", __func__);
	if (!screen->width || !screen->height)
	{
		screen->width = 640;
		screen->height = 480;
	}
	if (!screen->fb.depth)
		screen->fb.depth = 24;
	printf("Attempting for %dx%d/%dbpp mode\n", screen->width, screen->height, screen->fb.depth);
	driver->screen = SDL_SetVideoMode(screen->width, screen->height, screen->fb.depth, 0);
	if(driver->screen == NULL)
	{
		printf("%s: SDL_SetVideoMode failed!\n", __func__);
		return FALSE;
	}
	driver->randr = screen->randr;
	screen->driver = driver;
	printf("Set %dx%d/%dbpp mode\n", driver->screen->w, driver->screen->h, driver->screen->format->BitsPerPixel);
	screen->width = driver->screen->w;
	screen->height = driver->screen->h;
	screen->fb.depth = driver->screen->format->BitsPerPixel;
	screen->fb.visuals = (1<<TrueColor);
	screen->fb.redMask = driver->screen->format->Rmask;
	screen->fb.greenMask = driver->screen->format->Gmask;
	screen->fb.blueMask = driver->screen->format->Bmask;
	screen->fb.bitsPerPixel = driver->screen->format->BitsPerPixel;
	//screen->fb.shadow = FALSE;
	screen->rate=30; // 60 is too intense for CPU

	SDL_WM_SetCaption("Freedesktop.org X server (SDL)", NULL);

	SDL_EnableUNICODE(1);
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
#ifdef __ANDROID__
	set_clipboard_text(SDL_GetClipboardText());

	sdlScreenButtons = SDL_ANDROID_GetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_0);
#endif
	setScreenButtons(10000);

	if (getenv("XSDL_BUILTIN_KEYBOARD") != NULL)
	{
		sdlKeyboardType = atoi(getenv("XSDL_BUILTIN_KEYBOARD")) == KB_NATIVE ? KB_NATIVE :
						atoi(getenv("XSDL_BUILTIN_KEYBOARD")) == KB_BUILTIN ? KB_BUILTIN : KB_BOTH;
	}
	unsetenv("XSDL_BUILTIN_KEYBOARD");

	printf("sdlScreenButtons %d sdlKeyboardType %d\n", sdlScreenButtons, sdlKeyboardType);

	return sdlMapFramebuffer (screen);
}

static void sdlShadowUpdate (ScreenPtr pScreen, shadowBufPtr pBuf)
{
	KdScreenPriv(pScreen);
	KdScreenInfo *screen = pScreenPriv->screen;
	SdlDriver *driver = screen->driver;
	pixman_box16_t * rects;
	int amount, i;
	int updateRectsPixelCount = 0;

	//printf("sdlShadowUpdate: time %d", SDL_GetTicks());

	if (driver->shadow)
	{
		ShadowUpdateProc update;
		if (driver->randr)
		{
			/*
			if (driver->screen->format->BitsPerPixel == 16)
			{
				switch (driver->randr) {
				case RR_Rotate_90:
					update = shadowUpdateRotate16_90YX;
					break;
				case RR_Rotate_180:
					update = shadowUpdateRotate16_180;
					break;
				case RR_Rotate_270:
					update = shadowUpdateRotate16_270YX;
					break;
				default:
					update = shadowUpdateRotate16;
					break;
				}
			} else
			*/
				update = shadowUpdateRotatePacked;
		}
		else
			update = shadowUpdatePacked;

		update(pScreen, pBuf);
	}

	rects = pixman_region_rectangles(&pBuf->pDamage->damage, &amount);
	for ( i = 0; i < amount; i++ )
	{
		updateRectsPixelCount += (pBuf->pDamage->damage.extents.x2 - pBuf->pDamage->damage.extents.x1) *
						(pBuf->pDamage->damage.extents.y2 - pBuf->pDamage->damage.extents.y1);
	}
	// Each subrect is copied into temp buffer before uploading to OpenGL texture,
	// so if total area of pixels copied is more than 1/3 of the whole screen area,
	// there will be performance hit instead of optimization.

	if ( amount > NUMRECTS || updateRectsPixelCount * 3 > driver->screen->w * driver->screen->h )
	{
		//printf("SDL_Flip\n");
		SDL_Flip(driver->screen);
		//nextFullScreenRefresh = 0;
	}
	else
	{
		SDL_Rect updateRects[NUMRECTS];
		//if ( ! nextFullScreenRefresh )
		//	nextFullScreenRefresh = SDL_GetTicks() + FULLSCREEN_REFRESH_TIME;
		for ( i = 0; i < amount; i++ )
		{
			updateRects[i].x = rects[i].x1;
			updateRects[i].y = rects[i].y1;
			updateRects[i].w = rects[i].x2 - rects[i].x1;
			updateRects[i].h = rects[i].y2 - rects[i].y1;
			//printf("sdlShadowUpdate: rect %d: %04d:%04d:%04d:%04d", i, rects[i].x1, rects[i].y1, rects[i].x2, rects[i].y2);
		}
		//printf("SDL_UpdateRects %d\n", amount);
		SDL_UpdateRects(driver->screen, amount, updateRects);
	}
}

static void *sdlShadowWindow (ScreenPtr pScreen, CARD32 row, CARD32 offset, int mode, CARD32 *size, void *closure)
{
	KdScreenPriv(pScreen);
	KdScreenInfo *screen = pScreenPriv->screen;
	SdlDriver *driver = screen->driver;

//	if (!pScreenPriv->enabled)
//		return NULL;

	*size = driver->screen->pitch;

	//printf("%s\n", __func__);

	return (void *)((CARD8 *)driver->screen->pixels + row * (*size) + offset);
}


static Bool sdlCreateRes(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	KdScreenInfo *screen = pScreenPriv->screen;
	SdlDriver *driver = screen->driver;
	Bool oldShadow = screen->fb.shadow;

	printf("%s\n", __func__);

	// Hack: Kdrive assumes we have dumb videobuffer, which updates automatically,
	// and does not call update callback if shadow flag is not set.
	screen->fb.shadow = TRUE;
	KdShadowSet (pScreen, driver->randr, sdlShadowUpdate, sdlShadowWindow);
	screen->fb.shadow = oldShadow;

	return TRUE;
}

#ifdef RANDR

typedef struct { int width; int height; } screen_size_t;

static Bool sdlRandRGetInfo (ScreenPtr pScreen, Rotation *rotations)
{
	KdScreenPriv(pScreen);
	KdScreenInfo			*screen = pScreenPriv->screen;
	SdlDriver				*driver = screen->driver;
	RRScreenSizePtr			pSize;
	Rotation				randr;
	int						n;
	screen_size_t sizes[] =
	{
		{ 1920, 1200 },
		{ 1920, 1080 },
		{ 1600, 1200 },
		{ 1400, 1050 },
		{ 1280, 1024 },
		{ 1280, 960  },
		{ 1280, 800  },
		{ 1280, 720  },
		{ 1152, 864 },
		{ 1024, 768 },
		{ 832, 624 },
		{ 800, 600 },
		{ 800, 480 },
		{ 720, 400 },
		{ 640, 480 },
		{ 640, 400 },
		{ 320, 240 },
		{ 320, 200 },
		{ 160, 160 },
		{ 0, 0 }
	};


	printf("%s", __func__);

	*rotations = RR_Rotate_All|RR_Reflect_All;

	for (n = 0; n < pScreen->numDepths; n++)
		if (pScreen->allowedDepths[n].numVids)
			break;
	if (n == pScreen->numDepths)
		return FALSE;

	pSize = RRRegisterSize (pScreen,
							screen->width,
							screen->height,
							screen->width_mm,
							screen->height_mm);

	n = 0;
	while (sizes[n].width != 0 && sizes[n].height != 0)
	{
		RRRegisterSize (pScreen,
				sizes[n].width,
				sizes[n].height, 
				(sizes[n].width * screen->width_mm)/screen->width,
				(sizes[n].height *screen->height_mm)/screen->height
				);
		n++;
	}

	randr = KdSubRotation (driver->randr, screen->randr);

	RRSetCurrentConfig (pScreen, randr, 0, pSize);

	return TRUE;
}

static Bool sdlRandRSetConfig (ScreenPtr			pScreen,
					 Rotation			randr,
					 int				rate,
					 RRScreenSizePtr	pSize)
{
	KdScreenPriv(pScreen);
	KdScreenInfo		*screen = pScreenPriv->screen;
	SdlDriver			*driver = screen->driver;
	Bool				wasEnabled = pScreenPriv->enabled;
	SdlDriver			oldDriver;
	int					oldwidth;
	int					oldheight;
	int					oldmmwidth;
	int					oldmmheight;

	if (wasEnabled)
		KdDisableScreen (pScreen);

	oldDriver = *driver;

	oldwidth = screen->width;
	oldheight = screen->height;
	oldmmwidth = pScreen->mmWidth;
	oldmmheight = pScreen->mmHeight;

	/*
	 * Set new configuration
	 */

	driver->randr = KdAddRotation (screen->randr, randr);

	printf("%s driver->randr %d", __func__, driver->randr);

	sdlUnmapFramebuffer (screen);

	if (!sdlMapFramebuffer (screen))
		goto bail4;

	KdShadowUnset (screen->pScreen);

	if (!sdlCreateRes (screen->pScreen))
		goto bail4;

	sdlSetScreenSizes (screen->pScreen);

	/*
	 * Set frame buffer mapping
	 */
	(*pScreen->ModifyPixmapHeader) (fbGetScreenPixmap (pScreen),
									pScreen->width,
									pScreen->height,
									screen->fb.depth,
									screen->fb.bitsPerPixel,
									screen->fb.byteStride,
									screen->fb.frameBuffer);

	/* set the subpixel order */

	KdSetSubpixelOrder (pScreen, driver->randr);
	if (wasEnabled)
		KdEnableScreen (pScreen);

	return TRUE;

bail4:
	sdlUnmapFramebuffer (screen);
	*driver = oldDriver;
	(void) sdlMapFramebuffer (screen);
	pScreen->width = oldwidth;
	pScreen->height = oldheight;
	pScreen->mmWidth = oldmmwidth;
	pScreen->mmHeight = oldmmheight;

	if (wasEnabled)
		KdEnableScreen (pScreen);
	return FALSE;
}

static Bool sdlRandRInit (ScreenPtr pScreen)
{
	rrScrPrivPtr	pScrPriv;

	printf("%s", __func__);

	if (!RRScreenInit (pScreen))
		return FALSE;

	pScrPriv = rrGetScrPriv(pScreen);
	pScrPriv->rrGetInfo = sdlRandRGetInfo;
	pScrPriv->rrSetConfig = sdlRandRSetConfig;
	return TRUE;
}
#endif


static Bool sdlFinishInitScreen(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	KdScreenInfo *screen = pScreenPriv->screen;
	SdlDriver *scrpriv = screen->driver;

	if (!shadowSetup (pScreen))
		return FALSE;

#ifdef RANDR
	if (!sdlRandRInit (pScreen))
		return FALSE;
#endif

	scrpriv->screenBlockCallback = pScreen->BlockHandler;
	pScreen->BlockHandler = sdlScreenBlockCallback;

	return TRUE;
}

static void sdlKeyboardFini(KdKeyboardInfo *ki)
{
	printf("sdlKeyboardFini() %p\n", ki);
	sdlKeyboard = NULL;
}

static Status sdlKeyboardInit(KdKeyboardInfo *ki)
{
	ki->minScanCode = 8;
	ki->maxScanCode = 255;
	ki->name = strdup("Android keyboard");

	sdlKeyboard = ki;
	printf("sdlKeyboardInit() %p\n", ki);
		return Success;
}

static Status sdlKeyboardEnable (KdKeyboardInfo *ki)
{
	return Success;
}

static void sdlKeyboardDisable (KdKeyboardInfo *ki)
{
}

static void sdlKeyboardLeds (KdKeyboardInfo *ki, int leds)
{
}

static void sdlKeyboardBell (KdKeyboardInfo *ki, int volume, int frequency, int duration)
{
}

static Status sdlMouseInit (KdPointerInfo *pi)
{
	pi->nButtons = 7;
	pi->name = strdup("Android touchscreen and stylus");
	sdlPointer = pi;
	printf("sdlMouseInit() %p\n", pi);
	return Success;
}

static void sdlMouseFini(KdPointerInfo *pi)
{
	printf("sdlMouseFini() %p\n", pi);
	sdlPointer = NULL;
}

static Status sdlMouseEnable (KdPointerInfo *pi)
{
	return Success;
}

static void sdlMouseDisable (KdPointerInfo *pi)
{
	return;
}


void InitCard(char *name)
{
	KdCardInfoAdd (&sdlFuncs,  0);
	printf("InitCard: %s\n", name);
}

void InitOutput(ScreenInfo *pScreenInfo, int argc, char **argv)
{
	KdInitOutput(pScreenInfo, argc, argv);
	printf("InitOutput()\n");
}

void InitInput(int argc, char **argv)
{
	KdPointerInfo *pi;
	KdKeyboardInfo *ki;

	KdAddKeyboardDriver(&sdlKeyboardDriver);
	KdAddPointerDriver(&sdlMouseDriver);

	ki = KdNewKeyboard();
	ki->driver = &sdlKeyboardDriver;
	KdAddKeyboard(ki);
	pi = KdNewPointer();
	pi->driver = &sdlMouseDriver;
	KdAddPointer(pi);

	KdInitInput();
}

#ifdef DDXBEFORERESET
void ddxBeforeReset(void)
{
}
#endif

void ddxUseMsg(void)
{
	KdUseMsg();
}

int ddxProcessArgument(int argc, char **argv, int i)
{
	return KdProcessArgument(argc, argv, i);
}

static void sdlPollInput(void)
{
	static int buttonState=0;
	static int pressure = 0;
	SDL_Event event;

	//printf("sdlPollInput() %d\n", SDL_GetTicks());

	while ( SDL_PollEvent(&event) ) {
		switch (event.type) {
			case SDL_MOUSEMOTION:
				//printf("SDL_MOUSEMOTION pressure %d\n", pressure);
				KdEnqueuePointerEvent(sdlPointer, mouseState, event.motion.x, event.motion.y, pressure);
				setScreenButtons(event.motion.x);
				break;
			case SDL_MOUSEBUTTONDOWN:
				switch(event.button.button)
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
						buttonState = 1 << (event.button.button - 1);
						break;
				}
				mouseState |= buttonState;
				KdEnqueuePointerEvent(sdlPointer, mouseState|KD_MOUSE_DELTA, 0, 0, pressure);
				break;
			case SDL_MOUSEBUTTONUP:
				switch(event.button.button)
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
						buttonState = 1 << (event.button.button - 1);
						break;
				}
				mouseState &= ~buttonState;
				KdEnqueuePointerEvent(sdlPointer, mouseState|KD_MOUSE_DELTA, 0, 0, pressure);
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				//printf("Key sym %d scancode %d unicode %d", event.key.keysym.sym, event.key.keysym.scancode, event.key.keysym.unicode);
#ifdef __ANDROID__
				if (event.key.keysym.sym == SDLK_HELP)
				{
					if(event.type == SDL_KEYUP)
					{
						// SDL_ANDROID_ToggleScreenKeyboardWithoutTextInput();
						static int keyboard = 0;
						keyboard++;
						if (keyboard > 1 || (sdlKeyboardType != KB_BOTH && keyboard > 0))
							keyboard = 0;
						SDL_HideScreenKeyboard(NULL);
						//SDL_Delay(150);
						SDL_Flip(SDL_GetVideoSurface());
						if (keyboard == 0)
						{
							SDL_Delay(100);
							if (sdlKeyboardType == KB_NATIVE || sdlKeyboardType == KB_BOTH)
								SDL_ANDROID_ToggleScreenKeyboardWithoutTextInput();
							if (sdlKeyboardType == KB_BUILTIN)
								SDL_ANDROID_ToggleInternalScreenKeyboard(SDL_KEYBOARD_QWERTY);
							SDL_Flip(SDL_GetVideoSurface());
						}
						if (keyboard == 1 && sdlKeyboardType == KB_BOTH)
						{
							SDL_Delay(100);
							SDL_ANDROID_ToggleInternalScreenKeyboard(SDL_KEYBOARD_QWERTY);
							SDL_Flip(SDL_GetVideoSurface());
						}
					}
					setScreenButtons(10000);
				}
				else
#endif
				if (event.key.keysym.sym == SDLK_UNDO)
				{
					if(event.type == SDL_KEYUP)
					{
						// Send Ctrl-Z
						KdEnqueueKeyboardEvent (sdlKeyboard, 37, 0); // LCTRL
						KdEnqueueKeyboardEvent (sdlKeyboard, 52, 0); // Z
						KdEnqueueKeyboardEvent (sdlKeyboard, 52, 1); // Z
						KdEnqueueKeyboardEvent (sdlKeyboard, 37, 1); // LCTRL
					}
				}
				else if((event.key.keysym.unicode & 0xFF80) != 0)
				{
					send_unicode (event.key.keysym.unicode);
				}
				else
					KdEnqueueKeyboardEvent (sdlKeyboard, event.key.keysym.scancode, event.type==SDL_KEYUP);
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

void sdlScreenBlockCallback(ScreenPtr pScreen, void *timeout)
{
	KdScreenPriv(pScreen);
	KdScreenInfo *screen = pScreenPriv->screen;
	SdlDriver *scrpriv = screen->driver;

	pScreen->BlockHandler = scrpriv->screenBlockCallback;
	(*pScreen->BlockHandler)(pScreen, timeout);
	scrpriv->screenBlockCallback = pScreen->BlockHandler;
	pScreen->BlockHandler = sdlScreenBlockCallback;

	sdlPollInput();
}

static Bool xsdlInit(KdCardInfo * card)
{
	printf("Calling SDL_Init()\n");
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO);
	SDL_JoystickOpen(0); // Receive pressure events
	return TRUE;
}

static void xsdlFini(KdCardInfo * card)
{
	//SDL_Quit(); // SDL_Quit() on Android is buggy
}

void
CloseInput (void)
{
	KdCloseInput ();
}

void OsVendorInit (void)
{
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
	pos.y = kbShown ? 0 : SDL_ListModes(NULL, 0)[0]->h - pos.h * 3;

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

	SDL_ANDROID_SetScreenKeyboardTransparency(255); // opaque

#endif
}
