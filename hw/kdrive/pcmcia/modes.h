/*
 * Copyright 2001 by Alan Hourihane, Sychdyn, North Wales, UK.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 */

#define V_NHSYNC 0x01
#define V_NVSYNC 0x02
#define V_PHSYNC 0x04
#define V_PVSYNC 0x08
#define V_INTERLACE 0x10

pcmciaDisplayModeRec pcmciaDefaultModes [] = {
/* 640x400 @ 70Hz (VGA) hsync: 37.9kHz */
	{640, 400, 70 ,31500, 640,672,736,832,0, 400,401,404,445,0, V_NHSYNC | V_PVSYNC},
/* 640x480 @ 60Hz (Industry standard) hsync: 31.5kHz */
	{640, 480, 60 ,25200, 640,656,752,800,0, 480,490,492,525,0, V_NHSYNC | V_NVSYNC},
/* 640x480 @ 72Hz (VESA) hsync: 37.9kHz */
	{640, 480, 72 ,31500, 640,664,704,832,0, 480,489,491,520,0, V_NHSYNC | V_NVSYNC},
/* 640x480 @ 75Hz (VESA) hsync: 37.5kHz */
	{640, 480, 75 ,31500, 640,656,720,840,0, 480,481,484,500,0, V_NHSYNC | V_NVSYNC},
/* 640x480 @ 85Hz (VESA) hsync: 43.3kHz */
	{640, 480, 85 ,36000, 640,696,752,832,0, 480,481,484,509,0, V_NHSYNC | V_NVSYNC},
/* 800x600 @ 56Hz (VESA) hsync: 35.2kHz */
	{800, 600, 56 ,36000, 800,824,896,1024,0, 600,601,603,625,0, V_PHSYNC | V_PVSYNC},
/* 800x600 @ 60Hz (VESA) hsync: 37.9kHz */
	{800, 600, 60 ,40000, 800,840,968,1056,0, 600,601,605,628,0, V_PHSYNC | V_PVSYNC},
/* 800x600 @ 72Hz (VESA) hsync: 48.1kHz */
	{800, 600, 72 ,50000, 800,856,976,1040,0, 600,637,643,666,0, V_PHSYNC | V_PVSYNC},
/* 800x600 @ 75Hz (VESA) hsync: 46.9kHz */
	{800, 600, 75 ,49500, 800,816,896,1056,0, 600,601,604,625,0, V_PHSYNC | V_PVSYNC},
/* 800x600 @ 85Hz (VESA) hsync: 53.7kHz */
	{800, 600, 85 ,56300, 800,832,896,1048,0, 600,601,604,631,0, V_PHSYNC | V_PVSYNC},
/* 1024x768i @ 43Hz (industry standard) hsync: 35.5kHz */
	{1024, 768, 43 ,44900, 1024,1032,1208,1264,0, 768,768,776,817,0, V_PHSYNC | V_PVSYNC | V_INTERLACE},
/* 1024x768 @ 60Hz (VESA) hsync: 48.4kHz */
	{1024, 768, 60 ,65000, 1024,1048,1184,1344,0, 768,771,777,806,0, V_NHSYNC | V_NVSYNC},
/* 1024x768 @ 70Hz (VESA) hsync: 56.5kHz */
	{1024, 768, 70 ,75000, 1024,1048,1184,1328,0, 768,771,777,806,0, V_NHSYNC | V_NVSYNC},
/* 1024x768 @ 75Hz (VESA) hsync: 60.0kHz */
	{1024, 768, 75 ,78800, 1024,1040,1136,1312,0, 768,769,772,800,0, V_PHSYNC | V_PVSYNC},
/* 1024x768 @ 85Hz (VESA) hsync: 68.7kHz */
	{1024, 768, 85 ,94500, 1024,1072,1168,1376,0, 768,769,772,808,0, V_PHSYNC | V_PVSYNC},
/* 1152x864 @ 75Hz (VESA) hsync: 67.5kHz */
	{1152, 864, 75 ,108000, 1152,1216,1344,1600,0, 864,865,868,900,0, V_PHSYNC | V_PVSYNC},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};
