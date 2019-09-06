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

#ifndef _SDL_SEND_TEXT_H_
#define _SDL_SEND_TEXT_H_

#include <xorg-config.h>
#include "kdrive.h"

#include <SDL/SDL_events.h>

extern KdKeyboardInfo *sdlKeyboard;
extern KdPointerInfo *sdlPointer;

int execute_command(const char * command, const char *mode, char * data, int datalen);
int UnicodeToUtf8(int src, char * dest);
void send_unicode(int unicode);
void set_clipboard_text(char *text);
void process_clipboard_event(SDL_SysWMEvent *event);


#endif
