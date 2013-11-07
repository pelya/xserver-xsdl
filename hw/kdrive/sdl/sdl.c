/*
 * Copyright © 2004 PillowElephantBadgerBankPond 
 
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of PillowElephantBadgerBankPond not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  PillowElephantBadgerBankPond makes no
 * representations about the suitability of this software for any purpose.  It
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
 * 	- jaymz
 *
 */
#ifdef HAVE_CONFIG_H
#include "kdrive-config.h"
#endif
#include "kdrive.h"
#include <SDL/SDL.h>
#include <SDL/SDL_screenkeyboard.h>
#include <X11/keysym.h>
#include <android/log.h>

// DEBUG
//#define printf(...)
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "XSDL", __VA_ARGS__)

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

void *sdlShadowWindow (ScreenPtr pScreen, CARD32 row, CARD32 offset, int mode, CARD32 *size, void *closure);
void sdlShadowUpdate (ScreenPtr pScreen, shadowBufPtr pBuf);

void sdlPollInput(void);

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

int mouseState=0;

struct SdlDriver
{
	SDL_Surface *screen;
};



static Bool sdlScreenInit(KdScreenInfo *screen)
{
	struct SdlDriver *sdlDriver=calloc(1, sizeof(struct SdlDriver));
	printf("sdlScreenInit()\n");
	if (!screen->width || !screen->height)
	{
		screen->width = 640;
		screen->height = 480;
	}
	if (!screen->fb.depth)
		screen->fb.depth = 4;
	printf("Attempting for %dx%d/%dbpp mode\n", screen->width, screen->height, screen->fb.depth);
	sdlDriver->screen=SDL_SetVideoMode(screen->width, screen->height, screen->fb.depth, 0);
	if(sdlDriver->screen==NULL)
		return FALSE;
	printf("Set %dx%d/%dbpp mode\n", sdlDriver->screen->w, sdlDriver->screen->h, sdlDriver->screen->format->BitsPerPixel);
	screen->width=sdlDriver->screen->w;
	screen->height=sdlDriver->screen->h;
	screen->fb.depth=sdlDriver->screen->format->BitsPerPixel;
	screen->fb.visuals=(1<<TrueColor);
	screen->fb.redMask=sdlDriver->screen->format->Rmask;
	screen->fb.greenMask=sdlDriver->screen->format->Gmask;
	screen->fb.blueMask=sdlDriver->screen->format->Bmask;
	screen->fb.bitsPerPixel=sdlDriver->screen->format->BitsPerPixel;
	screen->rate=30; // 60 is too intense for CPU
	screen->driver=sdlDriver;
	screen->fb.byteStride=sdlDriver->screen->pitch;
	screen->fb.pixelStride=sdlDriver->screen->w;
	screen->fb.frameBuffer=(CARD8 *)sdlDriver->screen->pixels;
	SDL_WM_SetCaption("Freedesktop.org X server (SDL)", NULL);

	return TRUE;
}

void sdlShadowUpdate (ScreenPtr pScreen, shadowBufPtr pBuf)
{
	KdScreenPriv(pScreen);
	KdScreenInfo *screen = pScreenPriv->screen;
	struct SdlDriver *sdlDriver=screen->driver;
	pixman_box16_t * rects;
	int amount, pixelCount = 0, i;
	enum { SDL_NUMRECTS = 16 };
	
	/*
	// Not needed on Android
	if(SDL_MUSTLOCK(sdlDriver->screen))
	{
		if(SDL_LockSurface(sdlDriver->screen)<0)
		{
			printf("Couldn't lock SDL surface - d'oh!\n");
			return;
		}
	}
	
	if(SDL_MUSTLOCK(sdlDriver->screen))
		SDL_UnlockSurface(sdlDriver->screen);
	*/

	rects = pixman_region_rectangles(&pBuf->pDamage->damage, &amount);
	for (i = 0; i < amount; i++)
	{
		pixelCount += (pBuf->pDamage->damage.extents.x2 - pBuf->pDamage->damage.extents.x1) *
						(pBuf->pDamage->damage.extents.y2 - pBuf->pDamage->damage.extents.y1);
	}
	// Each subrect is copied into temp buffer before uploading to OpenGL texture,
	// so if total area of pixels copied is more than 1/3 of the whole screen area,
	// there will be performance hit instead of optimization.
	if( amount > SDL_NUMRECTS || pixelCount * 3 > sdlDriver->screen->w * sdlDriver->screen->h )
		SDL_Flip(sdlDriver->screen);
	else
	{
		SDL_Rect sdlrects[SDL_NUMRECTS];
		for (i = 0; i < amount; i++)
		{
			sdlrects[i].x = rects[i].x1;
			sdlrects[i].y = rects[i].y1;
			sdlrects[i].w = rects[i].x2 - rects[i].x1;
			sdlrects[i].h = rects[i].y2 - rects[i].y1;
		}
		SDL_UpdateRects(sdlDriver->screen, amount, sdlrects);
	}
}


void *sdlShadowWindow (ScreenPtr pScreen, CARD32 row, CARD32 offset, int mode, CARD32 *size, void *closure)
{
	KdScreenPriv(pScreen);
	KdScreenInfo *screen = pScreenPriv->screen;
	struct SdlDriver *sdlDriver=screen->driver;
	*size=sdlDriver->screen->pitch;
	//printf("Shadow window()\n");
	return (void *)((CARD8 *)sdlDriver->screen->pixels + row * (*size) + offset);
}


static Bool sdlCreateRes(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	KdScreenInfo *screen = pScreenPriv->screen;
	KdShadowFbAlloc(screen, 0);
	KdShadowSet(pScreen, RR_Rotate_0, sdlShadowUpdate, sdlShadowWindow);
	return TRUE;
}

static Bool sdlFinishInitScreen(ScreenPtr pScreen)
{
	if (!shadowSetup (pScreen))
		return FALSE;
		
/*
#ifdef RANDR
	if (!sdlRandRInit (pScreen))
		return FALSE;
#endif
*/
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

void sdlPollInput(void)
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
					case 1:
						buttonState=KD_BUTTON_1;
						break;
					case 2:
						buttonState=KD_BUTTON_2;
						break;
					case 3:
						buttonState=KD_BUTTON_3;
						break;
				}
				mouseState|=buttonState;
				KdEnqueuePointerEvent(sdlPointer, mouseState|KD_MOUSE_DELTA, 0, 0, 0);
				break;
			case SDL_MOUSEBUTTONUP:
				switch(event.button.button)
				{
					case 1:
						buttonState=KD_BUTTON_1;
						pressure = 0;
						break;
					case 2:
						buttonState=KD_BUTTON_2;
						break;
					case 3:
						buttonState=KD_BUTTON_3;
						break;
				}
				mouseState &= ~buttonState;
				KdEnqueuePointerEvent(sdlPointer, mouseState|KD_MOUSE_DELTA, 0, 0, 0);
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if (event.key.keysym.sym == SDLK_UNDO)
				{
					if(event.type == SDL_KEYUP)
						SDL_ANDROID_ToggleScreenKeyboardWithoutTextInput();
				}
				else
					KdEnqueueKeyboardEvent (sdlKeyboard, event.key.keysym.scancode, event.type==SDL_KEYUP);
				break;
			case SDL_JOYAXISMOTION:
				if(event.jaxis.which == 0 && event.jaxis.axis == 4)
					pressure = event.jaxis.value;
				break;

			case SDL_QUIT:
				/* this should never happen */
				SDL_Quit();
		}
	}
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
	SDL_Quit();
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
