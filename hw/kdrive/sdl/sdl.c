/*
 * Copyright © 2004 PillowElephantBadgerBankPond 
 * Copyright © 2014 Sergii Pylypenko
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
#ifdef HAVE_CONFIG_H
#include "kdrive-config.h"
#endif
#include "kdrive.h"
#include <SDL/SDL.h>
#include <X11/keysym.h>
#include <sys/wait.h>
#include <pthread.h>

#ifdef __ANDROID__
#include <SDL/SDL_screenkeyboard.h>
#include <android/log.h>

// DEBUG
//#define printf(...)
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "XSDL", __VA_ARGS__)
#endif

static void xsdlFini(void);
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
static int execute_command(const char * command, const char *mode, char * data, int datalen);
static int UnicodeToUtf8(int src, char * dest);
static void send_unicode(int unicode);

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
	.scrinit = sdlScreenInit,	/* scrinit */
	.finishInitScreen = sdlFinishInitScreen, /* finishInitScreen */
	.createRes = sdlCreateRes,	/* createRes */
};

int mouseState = 0;

enum { NUMRECTS = 32, FULLSCREEN_REFRESH_TIME = 1000 };
//Uint32 nextFullScreenRefresh = 0;

typedef struct
{
	SDL_Surface *screen;
	Rotation randr;
	Bool shadow;
} SdlDriver;

//#undef RANDR

Bool
sdlMapFramebuffer (KdScreenInfo *screen)
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
		screen->fb.depth = 4;
	printf("Attempting for %dx%d/%dbpp mode\n", screen->width, screen->height, screen->fb.depth);
	driver->screen = SDL_SetVideoMode(screen->width, screen->height, screen->fb.depth, 0);
	if(driver->screen == NULL)
		return FALSE;
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

#ifndef __ANDROID__
	// Not needed on Android
	if(SDL_MUSTLOCK(driver->screen))
	{
		if(SDL_LockSurface(driver->screen)<0)
		{
			printf("Couldn't lock SDL surface - d'oh!\n");
			return;
		}
	}
	
	if(SDL_MUSTLOCK(driver->screen))
		SDL_UnlockSurface(driver->screen);
#endif

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
static Bool sdlRandRGetInfo (ScreenPtr pScreen, Rotation *rotations)
{
	KdScreenPriv(pScreen);
	KdScreenInfo			*screen = pScreenPriv->screen;
	SdlDriver				*driver = screen->driver;
	RRScreenSizePtr			pSize;
	Rotation				randr;
	int						n;

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
	if (!shadowSetup (pScreen))
		return FALSE;

#ifdef RANDR
	if (!sdlRandRInit (pScreen))
		return FALSE;
#endif
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

	ki = KdParseKeyboard("keyboard");
	KdAddKeyboard(ki);
	pi = KdParsePointer("mouse");
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
						SDL_ANDROID_ToggleScreenKeyboardWithoutTextInput();
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

static int xsdlInit(void)
{
	printf("Calling SDL_Init()\n");
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
	SDL_JoystickOpen(0); // Receive pressure events
	return 0;
}


static void xsdlFini(void)
{
	//SDL_Quit(); // SDL_Quit() on Android is buggy
}

void
CloseInput (void)
{
	KdCloseInput ();
}

KdOsFuncs sdlOsFuncs={
	.Init = xsdlInit,
	.Fini = xsdlFini,
	.pollEvents = sdlPollInput,
};

void OsVendorInit (void)
{
	KdOsInit (&sdlOsFuncs);
}

static void *send_unicode_thread(void *param)
{
	// International text input - copy symbol to clipboard, and send copypaste key
	int unicode = (int)param;
	char cmd[1024] = "";
	sprintf (cmd, "127.0.0.1:%s", display);
	setenv ("DISPLAY", cmd, 1);
	char c[5] = "";
	UnicodeToUtf8 (unicode, c);
	//sprintf(cmd, "echo '%s' | %s/usr/bin/xsel -b -i >/dev/null 2>&1", c, getenv("APPDIR"));
	//sprintf(cmd, "echo '%s' >/dev/null 2>&1", c);
	//sprintf(cmd, "%s/usr/bin/xsel -b", getenv("APPDIR"));
	//printf("Launching cmd: %s", cmd);
	//int ret = system(cmd);
	//printf("Cmd ret: %d", ret);
	sprintf(cmd, "%s/usr/bin/xsel -b -i >/dev/null 2>&1", getenv("APPDIR"));
	execute_command(cmd, "w", c, 5);
	KdEnqueueKeyboardEvent (sdlKeyboard, 37, 0); // LCTRL
	KdEnqueueKeyboardEvent (sdlKeyboard, 55, 0); // V
	KdEnqueueKeyboardEvent (sdlKeyboard, 55, 1); // V
	KdEnqueueKeyboardEvent (sdlKeyboard, 37, 1); // LCTRL
	return NULL;
}

void send_unicode(int unicode)
{
	pthread_t thread_id;
	pthread_attr_t attr;
	pthread_attr_init (&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
	pthread_create (&thread_id, &attr, &send_unicode_thread, (void *)unicode);
	pthread_attr_destroy (&attr);
}

int execute_command(const char * command, const char *mode, char * data, int datalen)
{
	FILE *cmd;
	//printf ("Starting child command: %s, mode %s data: '%s'", command, mode, data);
	cmd = popen (command, mode);
	if (!cmd) {
		printf ("Error while starting child command: %s", command);
		return -1;
	}
	if (mode[0] == 'w')
		fputs (data, cmd);
	else
	{
		datalen--;
		while (datalen > 0 && fgets (data, datalen, cmd))
		{
			int count = strlen (data);
			data += count;
			datalen -= count;
		}
		data[0] = 0;
	}
	int ret = WEXITSTATUS (pclose (cmd));
	//printf ("Child command returned %d", ret);
	return ret;
}

int UnicodeToUtf8(int src, char * dest)
{
    int len = 0;
    if ( src <= 0x007f) {
        *dest++ = (char)src;
        len = 1;
    } else if (src <= 0x07ff) {
        *dest++ = (char)0xc0 | (src >> 6);
        *dest++ = (char)0x80 | (src & 0x003f);
        len = 2;
    } else if (src == 0xFEFF) {
        // nop -- zap the BOM
    } else if (src >= 0xD800 && src <= 0xDFFF) {
        // surrogates not supported
    } else if (src <= 0xffff) {
        *dest++ = (char)0xe0 | (src >> 12);
        *dest++ = (char)0x80 | ((src >> 6) & 0x003f);
        *dest++ = (char)0x80 | (src & 0x003f);
        len = 3;
    } else if (src <= 0xffff) {
        *dest++ = (char)0xf0 | (src >> 18);
        *dest++ = (char)0x80 | ((src >> 12) & 0x3f);
        *dest++ = (char)0x80 | ((src >> 6) & 0x3f);
        *dest++ = (char)0x80 | (src & 0x3f);
        len = 4;
    } else {
        // out of Unicode range
    }
    *dest = 0;
    return len;
}
