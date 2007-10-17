/************************************************************

Author: Eamon Walsh <ewalsh@epoch.ncsc.mil>

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
this permission notice appear in supporting documentation.  This permission
notice shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

********************************************************/

#ifndef _XSELINUX_H
#define _XSELINUX_H

#include "dixaccess.h"

/* Extension info */
#define XSELINUX_EXTENSION_NAME		"SELinux"
#define XSELINUX_MAJOR_VERSION		1
#define XSELINUX_MINOR_VERSION		0
#define XSELinuxNumberEvents		0
#define XSELinuxNumberErrors		0

/* Private Flask definitions */
#define SECCLASS_X_DRAWABLE		1
#define SECCLASS_X_SCREEN		2
#define SECCLASS_X_GC			3
#define SECCLASS_X_FONT			4
#define SECCLASS_X_COLORMAP		5
#define SECCLASS_X_PROPERTY		6
#define SECCLASS_X_SELECTION		7
#define SECCLASS_X_CURSOR		8
#define SECCLASS_X_CLIENT		9
#define SECCLASS_X_DEVICE		10
#define SECCLASS_X_SERVER		11
#define SECCLASS_X_EXTENSION		12
#define SECCLASS_X_RESOURCE		13

#endif /* _XSELINUX_H */
