/* sigio.c -- Support for SIGIO handler installation and removal
 * Created: Thu Jun  3 15:39:18 1999 by faith@precisioninsight.com
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * 
 * Authors: Rickard E. (Rik) Faith <faith@valinux.com>
 *
 * $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/sigio.c,v 1.14 2002/05/05 19:18:14 herrb Exp $
 * 
 */


#ifdef XFree86Server
# include "X.h"
# include "xf86.h"
# include "xf86drm.h"
# include "xf86Priv.h"
# include "xf86_OSlib.h"
# include "xf86drm.h"
# include "inputstr.h"
#else
# include <unistd.h>
# include <signal.h>
# include <fcntl.h>
# include <sys/time.h>
# include <errno.h>
# include <stdio.h>
# include <string.h>
# define SYSCALL(call) while(((call) == -1) && (errno == EINTR))
#endif

/*
 * Linux libc5 defines FASYNC, but not O_ASYNC.  Don't know if it is
 * functional or not.
 */
#if defined(FASYNC) && !defined(O_ASYNC)
#  define O_ASYNC FASYNC
#endif

#ifdef MAX_DEVICES
/* MAX_DEVICES represents the maximimum number of input devices usable
 * at the same time plus one entry for DRM support.
 */
# define MAX_FUNCS   (MAX_DEVICES + 1)
#else
# define MAX_FUNCS 16
#endif

typedef struct _xf86SigIOFunc {
    void    (*f) (int, void *);
    int	    fd;
    void    *closure;
} Xf86SigIOFunc;

static Xf86SigIOFunc	xf86SigIOFuncs[MAX_FUNCS];
static int		xf86SigIOMax;
static int		xf86SigIOMaxFd;
static fd_set		xf86SigIOMask;

/*
 * SIGIO gives no way of discovering which fd signalled, select
 * to discover
 */
static void
xf86SIGIO (int sig)
{
    int	    i;
    fd_set  ready;
    struct timeval  to;
    int	    r;

    ready = xf86SigIOMask;
    to.tv_sec = 0;
    to.tv_usec = 0;
    SYSCALL (r = select (xf86SigIOMaxFd, &ready, 0, 0, &to));
    for (i = 0; r > 0 && i < xf86SigIOMax; i++)
	if (xf86SigIOFuncs[i].f && FD_ISSET (xf86SigIOFuncs[i].fd, &ready))
	{
	    (*xf86SigIOFuncs[i].f)(xf86SigIOFuncs[i].fd,
				   xf86SigIOFuncs[i].closure);
	    r--;
	}
#ifdef XFree86Server
    if (r > 0) {
      xf86Msg(X_ERROR, "SIGIO %d descriptors not handled\n", r);
    }
#endif
}

static int
xf86IsPipe (int fd)
{
    struct stat	buf;
    
    if (fstat (fd, &buf) < 0)
	return 0;
    return S_ISFIFO(buf.st_mode);
}

int
xf86InstallSIGIOHandler(int fd, void (*f)(int, void *), void *closure)
{
    struct sigaction sa;
    struct sigaction osa;
    int	i;
    int blocked;

    for (i = 0; i < MAX_FUNCS; i++)
    {
	if (!xf86SigIOFuncs[i].f)
	{
	    if (xf86IsPipe (fd))
		return 0;
	    blocked = xf86BlockSIGIO();
	    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_ASYNC) == -1) {
#ifdef XFree86Server
		xf86Msg(X_WARNING, "fcntl(%d, O_ASYNC): %s\n", 
			fd, strerror(errno));
#else
		fprintf(stderr,"fcntl(%d, O_ASYNC): %s\n", 
			fd, strerror(errno));
#endif
		xf86UnblockSIGIO(blocked);
		return 0;
	    }
	    if (fcntl(fd, F_SETOWN, getpid()) == -1) {
#ifdef XFree86Server
		xf86Msg(X_WARNING, "fcntl(%d, F_SETOWN): %s\n", 
			fd, strerror(errno));
#else
		fprintf(stderr,"fcntl(%d, F_SETOWN): %s\n", 
			fd, strerror(errno));
#endif
		return 0;
	    }
	    sigemptyset(&sa.sa_mask);
	    sigaddset(&sa.sa_mask, SIGIO);
	    sa.sa_flags   = 0;
	    sa.sa_handler = xf86SIGIO;
	    sigaction(SIGIO, &sa, &osa);
	    xf86SigIOFuncs[i].fd = fd;
	    xf86SigIOFuncs[i].closure = closure;
	    xf86SigIOFuncs[i].f = f;
	    if (i >= xf86SigIOMax)
		xf86SigIOMax = i+1;
	    if (fd >= xf86SigIOMaxFd)
		xf86SigIOMaxFd = fd + 1;
	    FD_SET (fd, &xf86SigIOMask);
	    xf86UnblockSIGIO(blocked);
	    return 1;
	}
 	/* Allow overwriting of the closure and callback */
 	else if (xf86SigIOFuncs[i].fd == fd)
 	{
 	    xf86SigIOFuncs[i].closure = closure;
 	    xf86SigIOFuncs[i].f = f;
 	    return 1;
 	}
    }
    return 0;
}

int
xf86RemoveSIGIOHandler(int fd)
{
    struct sigaction sa;
    struct sigaction osa;
    int	i;
    int max;
    int maxfd;
    int ret;

    max = 0;
    maxfd = -1;
    ret = 0;
    for (i = 0; i < MAX_FUNCS; i++)
    {
	if (xf86SigIOFuncs[i].f)
	{
	    if (xf86SigIOFuncs[i].fd == fd)
	    {
		xf86SigIOFuncs[i].f = 0;
		xf86SigIOFuncs[i].fd = 0;
		xf86SigIOFuncs[i].closure = 0;
		FD_CLR (fd, &xf86SigIOMask);
		ret = 1;
	    }
	    else
	    {
		max = i + 1;
		if (xf86SigIOFuncs[i].fd >= maxfd)
		    maxfd = xf86SigIOFuncs[i].fd + 1;
	    }
	}
    }
    if (ret)
    {
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_ASYNC);
	xf86SigIOMax = max;
	xf86SigIOMaxFd = maxfd;
	if (!max)
	{
	    sigemptyset(&sa.sa_mask);
	    sigaddset(&sa.sa_mask, SIGIO);
	    sa.sa_flags   = 0;
	    sa.sa_handler = SIG_DFL;
	    sigaction(SIGIO, &sa, &osa);
	}
    }
    return ret;
}

int
xf86BlockSIGIO (void)
{
    sigset_t	set, old;

    sigemptyset (&set);
    sigaddset (&set, SIGIO);
    sigprocmask (SIG_BLOCK, &set, &old);
    return sigismember (&old, SIGIO);
}

void
xf86UnblockSIGIO (int wasset)
{
    sigset_t	set;

    if (!wasset)
    {
	sigemptyset (&set);
	sigaddset (&set, SIGIO);
	sigprocmask (SIG_UNBLOCK, &set, NULL);
    }
}

#ifdef XFree86Server
void
xf86AssertBlockedSIGIO (char *where)
{
    sigset_t	set, old;

    sigemptyset (&set);
    sigprocmask (SIG_BLOCK, &set, &old);
    if (!sigismember (&old, SIGIO))
	xf86Msg (X_ERROR, "SIGIO not blocked at %s\n", where);
}

/* XXX This is a quick hack for the benefit of xf86SetSilkenMouse() */

int
xf86SIGIOSupported (void)
{
    return 1;
}

#endif
