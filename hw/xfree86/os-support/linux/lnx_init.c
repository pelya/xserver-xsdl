/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/lnx_init.c,v 3.14 2001/10/31 22:50:30 tsi Exp $ */
/*
 * Copyright 1992 by Orest Zborowski <obz@Kodak.com>
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Orest Zborowski and David Wexelblat 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  Orest Zborowski
 * and David Wexelblat make no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * OREST ZBOROWSKI AND DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL OREST ZBOROWSKI OR DAVID WEXELBLAT BE LIABLE 
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: lnx_init.c /main/7 1996/10/23 18:46:30 kaleb $ */

#include "X.h"
#include "Xmd.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "lnx.h"

#ifdef USE_DEV_FB
extern char *getenv(const char *);
#include <linux/fb.h>
char *fb_dev_name;
#endif

static Bool KeepTty = FALSE;
static int VTnum = -1;
static int activeVT = -1;

void
xf86OpenConsole(void)
{
    int i, fd = -1;
    int result;
    struct vt_mode VT;
    char vtname[11];
    struct vt_stat vts;
    MessageType from = X_PROBED;
#ifdef USE_DEV_FB
    struct fb_var_screeninfo var;
    int fbfd;
#endif
    char *tty0[] = { "/dev/tty0", "/dev/vc/0", NULL };
    char *vcs[] = { "/dev/vc/%d", "/dev/tty%d", NULL };

    if (serverGeneration == 1) 
    {
	/* check if we're run with euid==0 */
	if (geteuid() != 0)
	{
	    FatalError("xf86OpenConsole: Server must be suid root\n");
	}

	/*
	 * setup the virtual terminal manager
	 */
	if (VTnum != -1) {
	    xf86Info.vtno = VTnum;
	    from = X_CMDLINE;
	} else {
	    i=0;
	    while (tty0[i] != NULL)
	    {
		if ((fd = open(tty0[i],O_WRONLY,0)) >= 0)
		  break;
		i++;
	    }
	    if (fd < 0)
		FatalError(
		    "xf86OpenConsole: Cannot open /dev/tty0 (%s)\n",
		    strerror(errno));
	    if ((ioctl(fd, VT_OPENQRY, &xf86Info.vtno) < 0) ||
		(xf86Info.vtno == -1)) {
		FatalError("xf86OpenConsole: Cannot find a free VT\n");
	    }
	    close(fd);
	}

#ifdef USE_DEV_FB
	fb_dev_name=getenv("FRAMEBUFFER");
	if (!fb_dev_name)
	    fb_dev_name="/dev/fb0current";
	if ((fbfd = open(fb_dev_name, O_RDONLY)) < 0)
	    FatalError("xf86OpenConsole: Cannot open %s (%s)\n",
	fb_dev_name, strerror(errno));
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &var))
	    FatalError("xf86OpenConsole: Unable to get screen info\n");
#endif
	xf86Msg(from, "using VT number %d\n\n", xf86Info.vtno);

	if (!KeepTty) {
	    setpgrp();
	}

        i=0;
        while (vcs[i] != NULL)
        {
            sprintf(vtname, vcs[i], xf86Info.vtno); /* /dev/tty1-64 */
     	    if ((xf86Info.consoleFd = open(vtname, O_RDWR|O_NDELAY, 0)) >= 0)
		break;
            i++;
        }

	if (xf86Info.consoleFd < 0) {
	    FatalError("xf86OpenConsole: Cannot open virtual console %d (%s)\n",
		       xf86Info.vtno, strerror(errno));
	}

	/* change ownership of the vt */
	chown(vtname, getuid(), getgid());

	/*
	 * the current VT device we're running on is not "console", we want
	 * to grab all consoles too
	 *
	 * Why is this needed??
	 */
	chown("/dev/tty0", getuid(), getgid());

	/*
	 * Linux doesn't switch to an active vt after the last close of a vt,
	 * so we do this ourselves by remembering which is active now.
	 */
	if (ioctl(xf86Info.consoleFd, VT_GETSTATE, &vts) == 0)
	{
	    activeVT = vts.v_active;
	}

	if (!KeepTty)
	{
	    /*
	     * Detach from the controlling tty to avoid char loss
	     */
	    if ((i = open("/dev/tty",O_RDWR)) >= 0)
	    {
		ioctl(i, TIOCNOTTY, 0);
		close(i);
	    }
	}

	/*
	 * now get the VT
	 */
	SYSCALL(result = ioctl(xf86Info.consoleFd, VT_ACTIVATE, xf86Info.vtno));
	if (result != 0)
	{
	    xf86Msg(X_WARNING, "xf86OpenConsole: VT_ACTIVATE failed\n");
	}
	SYSCALL(result =
		  ioctl(xf86Info.consoleFd, VT_WAITACTIVE, xf86Info.vtno));
	if (result != 0)
	{
	    xf86Msg(X_WARNING, "xf86OpenConsole: VT_WAITACTIVE failed\n");
	}
	SYSCALL(result = ioctl(xf86Info.consoleFd, VT_GETMODE, &VT));
	if (result < 0) 
	{
	    FatalError("xf86OpenConsole: VT_GETMODE failed\n");
	}

	signal(SIGUSR1, xf86VTRequest);

	VT.mode = VT_PROCESS;
	VT.relsig = SIGUSR1;
	VT.acqsig = SIGUSR1;
	if (ioctl(xf86Info.consoleFd, VT_SETMODE, &VT) < 0) 
	{
	    FatalError("xf86OpenConsole: VT_SETMODE VT_PROCESS failed\n");
	}
	if (ioctl(xf86Info.consoleFd, KDSETMODE, KD_GRAPHICS) < 0)
	{
	    FatalError("xf86OpenConsole: KDSETMODE KD_GRAPHICS failed\n");
	}

	/* we really should have a InitOSInputDevices() function instead
	 * of Init?$#*&Device(). So I just place it here */
	
#ifdef USE_DEV_FB
	/* copy info to new console */
	var.yoffset=0;
	var.xoffset=0;
	if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &var))
	    FatalError("Unable to set screen info\n");
	close(fbfd);
#endif
    }
    else 
    {
	/* serverGeneration != 1 */
	/*
	 * now get the VT
	 */
	SYSCALL(result = ioctl(xf86Info.consoleFd, VT_ACTIVATE, xf86Info.vtno));
	if (result != 0)
	{
	    xf86Msg(X_WARNING, "xf86OpenConsole: VT_ACTIVATE failed\n");
	}
	SYSCALL(result =
		ioctl(xf86Info.consoleFd, VT_WAITACTIVE, xf86Info.vtno));
	if (result != 0)
	{
	    xf86Msg(X_WARNING, "xf86OpenConsole: VT_WAITACTIVE failed\n");
	}
    }
    return;
}

void
xf86CloseConsole()
{
    struct vt_mode   VT;

#if 0
    ioctl(xf86Info.consoleFd, VT_ACTIVATE, xf86Info.vtno);
    ioctl(xf86Info.consoleFd, VT_WAITACTIVE, 0);
#endif
    ioctl(xf86Info.consoleFd, KDSETMODE, KD_TEXT);  /* Back to text mode ... */
    if (ioctl(xf86Info.consoleFd, VT_GETMODE, &VT) != -1)
    {
	VT.mode = VT_AUTO;
	ioctl(xf86Info.consoleFd, VT_SETMODE, &VT); /* set dflt vt handling */
    }
    /*
     * Perform a switch back to the active VT when we were started
     */
    if (activeVT >= 0)
    {
	ioctl(xf86Info.consoleFd, VT_ACTIVATE, activeVT);
	activeVT = -1;
    }
    close(xf86Info.consoleFd);                /* make the vt-manager happy */
    return;
}

int
xf86ProcessArgument(int argc, char *argv[], int i)
{
	/*
	 * Keep server from detaching from controlling tty.  This is useful 
	 * when debugging (so the server can receive keyboard signals.
	 */
	if (!strcmp(argv[i], "-keeptty"))
	{
		KeepTty = TRUE;
		return(1);
	}
	if ((argv[i][0] == 'v') && (argv[i][1] == 't'))
	{
		if (sscanf(argv[i], "vt%2d", &VTnum) == 0)
		{
			UseMsg();
			VTnum = -1;
			return(0);
		}
		return(1);
	}
	return(0);
}

void
xf86UseMsg()
{
	ErrorF("vtXX                   use the specified VT number\n");
	ErrorF("-keeptty               ");
	ErrorF("don't detach controlling tty (for debugging only)\n");
	return;
}
