#ifndef SDL_INPUT_H
#define SDL_INPUT_H

enum {
	SCANCODE_XF86PASTE = 143,
	SCANCODE_LSHIFT = 50,
	SCANCODE_RSHIFT = 62,
	SCANCODE_LCTRL = 37,
	SCANCODE_V = 55,
	SCANCODE_Z = 52,
	SCANCODE_KP_PLUS = 86,
	SCANCODE_KP_MINUS = 82,
};

void sdlInitInput(void);

void sdlPollInput(void);

int sdlGetInputNotifyFd(void);

#endif
