/*
 * $XFree86$
 *
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

#include "kdrive.h"
#include <errno.h>
#include <signal.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <sys/stat.h>
#include <keysym.h>

static int  vtno;
int  LinuxConsoleFd;
static int  activeVT;
static Bool enabled;

void
LinuxVTRequest (int sig)
{
    kdSwitchPending = TRUE;
}

/* Check before chowning -- this avoids touching the file system */
void
LinuxCheckChown (char *file)
{
    struct stat	    st;
    __uid_t	    u;
    __gid_t	    g;

    if (stat (file, &st) < 0)
	return;
    u = getuid ();
    g = getgid ();
    if (st.st_uid != u || st.st_gid != g)
	chown (file, u, g);
}

int
LinuxInit ()
{
    int i, fd;
    char vtname[11];
    struct vt_stat vts;
    struct stat	statb;

    LinuxConsoleFd = -1;
    /* check if we're run with euid==0 */
    if (geteuid() != 0)
    {
	FatalError("LinuxInit: Server must be suid root\n");
    }

    if ((fd = open("/dev/tty0",O_WRONLY,0)) < 0) 
    {
	FatalError(
	    "LinuxInit: Cannot open /dev/tty0 (%s)\n",
	    strerror(errno));
    }
    if ((ioctl(fd, VT_OPENQRY, &vtno) < 0) ||
	(vtno == -1))
    {
	FatalError("xf86OpenConsole: Cannot find a free VT\n");
    }
    close(fd);

    sprintf(vtname,"/dev/tty%d",vtno); /* /dev/tty1-64 */

    if ((LinuxConsoleFd = open(vtname, O_RDWR|O_NDELAY, 0)) < 0)
    {
	FatalError("LinuxInit: Cannot open %s (%s)\n",
		   vtname, strerror(errno));
    }

    /* change ownership of the vt */
    LinuxCheckChown (vtname);

    /*
     * the current VT device we're running on is not "console", we want
     * to grab all consoles too
     *
     * Why is this needed?
     */
    LinuxCheckChown ("/dev/tty0");
    /*
     * Linux doesn't switch to an active vt after the last close of a vt,
     * so we do this ourselves by remembering which is active now.
     */
    if (ioctl(LinuxConsoleFd, VT_GETSTATE, &vts) == 0)
    {
	activeVT = vts.v_active;
    }

    return 1;
}

Bool
LinuxFindPci (CARD16 vendor, CARD16 device, CARD32 count, KdCardAttr *attr)
{
    FILE    *f;
    char    line[2048], *l, *end;
    CARD32  bus, id, mode, addr;
    int	    n;
    CARD32  ven_dev;
    Bool    ret = FALSE;
    int	    i;

    ven_dev = (((CARD32) vendor) << 16) | ((CARD32) device);
    f = fopen ("/proc/bus/pci/devices", "r");
    if (!f)
	return FALSE;
    attr->io = 0;
    while (fgets (line, sizeof (line)-1, f))
    {
	line[sizeof(line)-1] = '\0';
	l = line;
	bus = strtoul (l, &end, 16);
	if (end == l)
	    continue;
	l = end;
	id = strtoul (l, &end, 16);
	if (end == l)
	    continue;
	l = end;
	if (id != ven_dev)
	    continue;
	if (count--)
	    continue;
	(void) strtoul (l, &end, 16);
	if (end == l)
	    continue;
	l = end;
	n = 0;
	for (i = 0; i < 6; i++)
	{
	    addr = strtoul (l, &end, 16);
	    if (end == l)
		break;
	    if (addr & 1)
		attr->io = addr & ~0xf;
	    else
	    {
		if (n == KD_MAX_CARD_ADDRESS)
		    break;
		attr->address[n++] = addr & ~0xf;
	    }
	    l = end;
	}
	while (n > 0)
	{
	    if (attr->address[n-1] != 0)
		break;
	    n--;
	}
	attr->naddr = n;
	ret = TRUE;
	break;
    }
    fclose (f);
    return ret;
}

void
LinuxSetSwitchMode (int mode)
{
    struct sigaction	act;
    struct vt_mode	VT;
    
    if (ioctl(LinuxConsoleFd, VT_GETMODE, &VT) < 0) 
    {
	FatalError ("LinuxInit: VT_GETMODE failed\n");
    }

    if (mode == VT_PROCESS)
    {
	act.sa_handler = LinuxVTRequest;
	sigemptyset (&act.sa_mask);
	act.sa_flags = 0;
	act.sa_restorer = 0;
	sigaction (SIGUSR1, &act, 0);
    
	VT.mode = mode;
	VT.relsig = SIGUSR1;
	VT.acqsig = SIGUSR1;
    }
    else
    {
	act.sa_handler = SIG_IGN;
	sigemptyset (&act.sa_mask);
	act.sa_flags = 0;
	act.sa_restorer = 0;
	sigaction (SIGUSR1, &act, 0);
    
	VT.mode = mode;
	VT.relsig = 0;
	VT.acqsig = 0;
    }
    if (ioctl(LinuxConsoleFd, VT_SETMODE, &VT) < 0) 
    {
	FatalError("LinuxInit: VT_SETMODE failed\n");
    }
}

void
LinuxEnable (void)
{
    if (enabled)
	return;
    if (kdSwitchPending)
    {
	kdSwitchPending = FALSE;
	ioctl (LinuxConsoleFd, VT_RELDISP, VT_ACKACQ);
    }
    /*
     * now get the VT
     */
    LinuxSetSwitchMode (VT_AUTO);
    if (ioctl(LinuxConsoleFd, VT_ACTIVATE, vtno) != 0)
    {
	FatalError("LinuxInit: VT_ACTIVATE failed\n");
    }
    if (ioctl(LinuxConsoleFd, VT_WAITACTIVE, vtno) != 0)
    {
	FatalError("LinuxInit: VT_WAITACTIVE failed\n");
    }
    LinuxSetSwitchMode (VT_PROCESS);
    if (ioctl(LinuxConsoleFd, KDSETMODE, KD_GRAPHICS) < 0)
    {
	FatalError("LinuxInit: KDSETMODE KD_GRAPHICS failed\n");
    }
    enabled = TRUE;
}

Bool
LinuxSpecialKey (KeySym sym)
{
    struct vt_stat  vts;
    int		    con;
    
    if (XK_F1 <= sym && sym <= XK_F12)
    {
	con = sym - XK_F1 + 1;
	ioctl (LinuxConsoleFd, VT_GETSTATE, &vts);
	if (con != vts.v_active && (vts.v_state & (1 << con)))
	{
	    ioctl (LinuxConsoleFd, VT_ACTIVATE, con);
	    return TRUE;
	}
    }
    return FALSE;
}

void
LinuxDisable (void)
{
    ioctl(LinuxConsoleFd, KDSETMODE, KD_TEXT);  /* Back to text mode ... */
    if (kdSwitchPending)
    {
	kdSwitchPending = FALSE;
	ioctl (LinuxConsoleFd, VT_RELDISP, 1);
    }
    enabled = FALSE;
}

void
LinuxFini (void)
{
    struct vt_mode   VT;
    struct vt_stat  vts;
    int		    fd;

    if (LinuxConsoleFd < 0)
	return;

    if (ioctl(LinuxConsoleFd, VT_GETMODE, &VT) != -1)
    {
	VT.mode = VT_AUTO;
	ioctl(LinuxConsoleFd, VT_SETMODE, &VT); /* set dflt vt handling */
    }
    ioctl (LinuxConsoleFd, VT_GETSTATE, &vts);
    /*
     * Find a legal VT to switch to, either the one we started from
     * or the lowest active one that isn't ours
     */
    if (activeVT < 0 || 
	activeVT == vts.v_active || 
	!(vts.v_state & (1 << activeVT)))
    {
	for (activeVT = 1; activeVT < 16; activeVT++)
	    if (activeVT != vtno && (vts.v_state & (1 << activeVT)))
		break;
	if (activeVT == 16)
	    activeVT = -1;
    }
    /*
     * Perform a switch back to the active VT when we were started
     */
    if (activeVT >= -1)
    {
	ioctl (LinuxConsoleFd, VT_ACTIVATE, activeVT);
	ioctl (LinuxConsoleFd, VT_WAITACTIVE, activeVT);
	activeVT = -1;
    }
    close(LinuxConsoleFd);                /* make the vt-manager happy */
    fd = open ("/dev/tty0", O_RDWR|O_NDELAY, 0);
    if (fd >= 0)
    {
	ioctl (fd, VT_GETSTATE, &vts);
	if (ioctl (fd, VT_DISALLOCATE, vtno) < 0)
	    fprintf (stderr, "Can't deallocate console %d errno %d\n", vtno, errno);
	close (fd);
    }
    return;
}

KdOsFuncs   LinuxFuncs = {
    LinuxInit,
    LinuxEnable,
    LinuxSpecialKey,
    LinuxDisable,
    LinuxFini,
};

void
OsVendorInit (void)
{
    KdOsInit (&LinuxFuncs);
}
