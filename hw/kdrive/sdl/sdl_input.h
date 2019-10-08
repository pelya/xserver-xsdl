#ifndef SDL_INPUT_H
#define SDL_INPUT_H

void sdlInitInput(void);

void sdlPollInput(void);

int sdlGetInputNotifyFd(void);

#endif
