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

#include <SDL/SDL.h>

#include <stdio.h>
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

#include <xorg-config.h>
#include "kdrive.h"
#include "opaque.h"



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

static void *send_unicode_thread(void *param)
{
	// International text input - copy symbol to clipboard, and send copypaste key
	int unicode = (int)(uint64_t)param;
	char cmd[1024] = "";
	char c[5] = "";
	sprintf (cmd, "127.0.0.1:%s", display);
	setenv ("DISPLAY", cmd, 1);
	UnicodeToUtf8 (unicode, c);
	sprintf(cmd, "%s/usr/bin/xsel -b -i >/dev/null 2>&1", getenv("APPDIR"));
	execute_command(cmd, "w", c, 5);
	KdEnqueueKeyboardEvent (sdlKeyboard, 37, 0); // LCTRL
	KdEnqueueKeyboardEvent (sdlKeyboard, 55, 0); // V
	KdEnqueueKeyboardEvent (sdlKeyboard, 55, 1); // V
	KdEnqueueKeyboardEvent (sdlKeyboard, 37, 1); // LCTRL
	return NULL;
}

static void *set_clipboard_text_thread(void *param)
{
	// International text input - copy symbol to clipboard, and send copypaste key
	char *text = (char *)param;
	char cmd[1024] = "";
	sprintf (cmd, "127.0.0.1:%s", display);
	setenv ("DISPLAY", cmd, 1);
	sprintf(cmd, "%s/usr/bin/xsel -b -i >/dev/null 2>&1", getenv("APPDIR"));
	execute_command(cmd, "w", text, strlen(text));
	SDL_free(text);
	return NULL;
}

void send_unicode(int unicode)
{
	pthread_t thread_id;
	pthread_attr_t attr;
	pthread_attr_init (&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
	pthread_create (&thread_id, &attr, &send_unicode_thread, (void *)(size_t)unicode);
	pthread_attr_destroy (&attr);
}

void set_clipboard_text(char *text)
{
	pthread_t thread_id;
	pthread_attr_t attr;
	pthread_attr_init (&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
	pthread_create (&thread_id, &attr, &set_clipboard_text_thread, (void *)text);
	pthread_attr_destroy (&attr);
}

int execute_command(const char * command, const char *mode, char * data, int datalen)
{
	FILE *cmd;
	int ret;
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
	ret = WEXITSTATUS (pclose (cmd));
	//printf ("Child command returned %d", ret);
	return ret;
}
