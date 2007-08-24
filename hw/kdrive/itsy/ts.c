/*
 * Copyright © 1999 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#define NEED_EVENTS
#include "itsy.h"
#include <X11/Xproto.h>
#include "inputstr.h"
#include "Xpoll.h"

int
itsyTsReadBytes (int fd, char *buf, int len, int min)
{
    int		    n, tot;
    fd_set	    set;
    struct timeval  tv;

    tot = 0;
    while (len)
    {
	n = read (fd, buf, len);
	if (n > 0)
	{
	    tot += n;
	    buf += n;
	    len -= n;
	}
	if (tot % min == 0)
	    break;
	FD_ZERO (&set);
	FD_SET (fd, &set);
	tv.tv_sec = 0;
	tv.tv_usec = 100 * 1000;
	n = select (fd + 1, &set, 0, 0, &tv);
	if (n <= 0)
	    break;
    }
    return tot;
}

void
itsyTsRead (KdPointerInfo *pi, int tsPort)
{
    ts_event	    event;
    long	    buf[3];
    int		    n;
    long	    pressure;
    long	    x, y;
    unsigned long   flags;
    unsigned long   buttons;

    n = itsyTsReadBytes (tsPort, (char *) &event, 
			 sizeof (event), sizeof (event));
    if (n == sizeof (event))
    {
	if (event.pressure)
	{
	    flags = KD_BUTTON_1;
	    x = event.point.x;
	    y = event.point.y;
	}
	else
	{
	    flags = KD_MOUSE_DELTA;
	    x = 0;
	    y = 0;
	}
	KdEnqueuePointerEvent (pi, flags, x, y, 0);
    }
}

#if 0
#define ITSY_DEBUG_LOW	1

//
// Touch screen parameters are stored
// in the flash. This code is taken from 'wm1'.
//
void itsySetTouchCalibration (int mou_filedsc, 
			      int xs, int xt, int ys, int yt, int xys)
{
  int k, ibuf[10];

  ibuf[0] = xs;
  ibuf[1] = xt;
  ibuf[2] = ys;
  ibuf[3] = yt;
  ibuf[4] = xys;
  if ((k=ioctl(mou_filedsc, TS_SET_CALM, ibuf)) != 0) {
    fprintf(stderr, "ERROR: ioctl TS_SET_CALM returns %d\n", k);
  }
}


int itsyReadFlashBlock(int location, signed char *data, int dbytes) 
{
  int offset, bytes;
  int flashfd;

  flashfd = open("/dev/flash1", O_RDONLY);
  if (flashfd < 0) return(0);

  offset = lseek(flashfd, location, SEEK_SET);
  if (offset != location) {
    close(flashfd);
    return(0);
  }

  bytes = read(flashfd, data, dbytes);
  if (bytes != dbytes) {
    close(flashfd);
    return(0);
  }

  close(flashfd);
  return(1);
}

/**********************************************************************/
#define RAMSIZE (0x400000)
#define MONITOR_BLOCKSIZE (32)
/**********************************************************************/

/* code for storing calibration into flash */

#define CALIBRATE_BLOCKSIZE (32)
#define CALIBRATE_OFFSET (RAMSIZE-MONITOR_BLOCKSIZE-CALIBRATE_BLOCKSIZE)
#define CALIBRATE_MAGIC_NUM (0x0babedee)


static int check_if_calibrated_and_set(int mou_filedsc) 
{
  signed char cal_data[CALIBRATE_BLOCKSIZE];
  int *iptr;

  if (itsyReadFlashBlock(CALIBRATE_OFFSET,
			 cal_data, CALIBRATE_BLOCKSIZE) == 0) {
    if ( ITSY_DEBUG_LOW ) {
      fprintf(stderr,"unable to read calibration data for touch screen\n");
    }
    return(0);
  }

  iptr = (int *) cal_data;
  if (iptr[0] == CALIBRATE_MAGIC_NUM) {
    if ( ITSY_DEBUG_LOW ) {
      fprintf(stderr,"Calibrating touch screen using %d, %d, %d, %d, %d\n",
	      iptr[1], iptr[2], iptr[3], iptr[4], iptr[5]);
    }
    itsySetTouchCalibration(mou_filedsc, iptr[1], iptr[2], iptr[3], iptr[4], iptr[5]);
    return(1);
  }
  else {
    if ( ITSY_DEBUG_LOW ) {
      fprintf(stderr,"Couldn't calibrate screen\n");
    }
    return(0);
  }
}
#endif

int
itsyTsInit (void)
{
    int	tsPort;

    tsPort = open ("/dev/ts", 0);
    fprintf (stderr, "tsPort %d\n", tsPort);
#if 0
    if (tsPort >= 0)
	check_if_calibrated_and_set (tsPort);
#endif
    return tsPort;
}

void
itsyTsFini (int tsPort)
{
    if (tsPort >= 0)
	close (tsPort);
}

KdPointerDriver itsyTsMouseDriver = {
    "itsyts",
    itsyTsInit,
    itsyTsRead,
    itsyTsFini
};

