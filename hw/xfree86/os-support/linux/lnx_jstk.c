/* $Xorg: lnx_jstk.c,v 1.3 2000/08/17 19:51:23 cpqbld Exp $ */
/* Id: lnx_jstk.c,v 1.1 1995/12/20 14:06:09 lepied Exp */
/*
 * Copyright 1995 by Frederic Lepied, France. <fred@sugix.frmug.fr.net>       
 *                                                                            
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is  hereby granted without fee, provided that
 * the  above copyright   notice appear  in   all  copies and  that both  that
 * copyright  notice   and   this  permission   notice  appear  in  supporting
 * documentation, and that   the  name of  Frederic   Lepied not  be  used  in
 * advertising or publicity pertaining to distribution of the software without
 * specific,  written      prior  permission.     Frederic  Lepied   makes  no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.                   
 *                                                                            
 * FREDERIC  LEPIED DISCLAIMS ALL   WARRANTIES WITH REGARD  TO  THIS SOFTWARE,
 * INCLUDING ALL IMPLIED   WARRANTIES OF MERCHANTABILITY  AND   FITNESS, IN NO
 * EVENT  SHALL FREDERIC  LEPIED BE   LIABLE   FOR ANY  SPECIAL, INDIRECT   OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA  OR PROFITS, WHETHER  IN  AN ACTION OF  CONTRACT,  NEGLIGENCE OR OTHER
 * TORTIOUS  ACTION, ARISING    OUT OF OR   IN  CONNECTION  WITH THE USE    OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/lnx_jstk.c,v 3.6 1996/12/23 06:50:02 dawes Exp $ */

static const char rcs_id[] = "$Xorg: lnx_jstk.c,v 1.1 1995/12/20 14:06:09 lepied Exp";

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#define inline __inline__
#include <linux/joystick.h>
#include <fcntl.h>
#include <sys/ioctl.h>

extern int errno;

/***********************************************************************
 *
 * xf86JoystickOn --
 *
 * open the device and init timeout according to the device value.
 *
 ***********************************************************************
 */

int
xf86JoystickOn(char *name, int *timeout, int *centerX, int *centerY)
{
  int			fd;
  struct JS_DATA_TYPE   js;
  extern int		xf86Verbose;
    
#ifdef DEBUG
  ErrorF("xf86JoystickOn %s\n", name);
#endif

  if ((fd = open(name, O_RDWR | O_NDELAY)) < 0)
    {
      ErrorF("Cannot open joystick '%s' (%s)\n", name,
            strerror(errno));
      return -1;
    }

  if (*timeout == 0) {
    if (ioctl (fd, JS_GET_TIMELIMIT, timeout) == -1) {
      Error("joystick JS_GET_TIMELIMIT ioctl");
    }
    else {
      if (xf86Verbose) {
	ErrorF("(--) Joystick: timeout value = %d\n", *timeout);
      }
    }
  }
  else {
    if (ioctl(fd, JS_SET_TIMELIMIT, timeout) == -1) {
      Error("joystick JS_SET_TIMELIMIT ioctl");
    }
  }

  /* Assume the joystick is centred when this is called */
  read(fd, &js, JS_RETURN);
  if (*centerX < 0) {
    *centerX = js.x;
    if (xf86Verbose) {    
      ErrorF("(--) Joystick: CenterX set to %d\n", *centerX);
    }
  }
  if (*centerY < 0) {
    *centerY = js.y;
    if (xf86Verbose) {    
      ErrorF("(--) Joystick: CenterY set to %d\n", *centerY);
    }
  }
  
  return fd;
}

/***********************************************************************
 *
 * xf86JoystickInit --
 *
 * called when X device is initialized.
 *
 ***********************************************************************
 */

void
xf86JoystickInit()
{
	return;
}

/***********************************************************************
 *
 * xf86JoystickOff --
 *
 * close the handle.
 *
 ***********************************************************************
 */

int
xf86JoystickOff(fd, doclose)
int *fd;
int doclose;
{
  int   oldfd;
  
  if (((oldfd = *fd) >= 0) && doclose) {
    close(*fd);
    *fd = -1;
  }
  return oldfd;
}

/***********************************************************************
 *
 * xf86JoystickGetState --
 *
 * return the state of buttons and the position of the joystick.
 *
 ***********************************************************************
 */

int
xf86JoystickGetState(fd, x, y, buttons)
int     fd;
int     *x;
int     *y;
int     *buttons;
{
  struct JS_DATA_TYPE   js;
  int                   status;
  
  status = read(fd, &js, JS_RETURN);
 
  if (status != JS_RETURN)
    {
      Error("Joystick read");      
      return 0;
    }
  
  *x = js.x;
  *y = js.y;
  *buttons = js.buttons;
  
  return 1;
}

/* end of lnx_jstk.c */
