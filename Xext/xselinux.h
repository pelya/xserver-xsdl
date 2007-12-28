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
#define SELINUX_EXTENSION_NAME		"SELinux"
#define SELINUX_MAJOR_VERSION		1
#define SELINUX_MINOR_VERSION		0
#define SELinuxNumberEvents		0
#define SELinuxNumberErrors		0

/* Extension protocol */
#define X_SELinuxQueryVersion			0
#define X_SELinuxSetSelectionManager		1
#define X_SELinuxGetSelectionManager		2
#define X_SELinuxSetDeviceCreateContext		3
#define X_SELinuxGetDeviceCreateContext		4
#define X_SELinuxSetDeviceContext		5
#define X_SELinuxGetDeviceContext		6
#define X_SELinuxSetPropertyCreateContext	7
#define X_SELinuxGetPropertyCreateContext	8
#define X_SELinuxGetPropertyContext		9
#define X_SELinuxSetWindowCreateContext		10
#define X_SELinuxGetWindowCreateContext		11
#define X_SELinuxGetWindowContext		12

typedef struct {
    CARD8   reqType;
    CARD8   SELinuxReqType;
    CARD16  length;
    CARD8   client_major;
    CARD8   client_minor;
    CARD16  unused;
} SELinuxQueryVersionReq;

typedef struct {
    CARD8   type;
    CARD8   pad1;
    CARD16  sequenceNumber;
    CARD32  length;
    CARD16  server_major;
    CARD16  server_minor;
    CARD32  pad2;
    CARD32  pad3;
    CARD32  pad4;
    CARD32  pad5;
    CARD32  pad6; 
} SELinuxQueryVersionReply;

typedef struct {
    CARD8   reqType;
    CARD8   SELinuxReqType;
    CARD16  length;
    CARD32  window;
} SELinuxSetSelectionManagerReq;

typedef struct {
    CARD8   reqType;
    CARD8   SELinuxReqType;
    CARD16  length;
} SELinuxGetSelectionManagerReq;

typedef struct {
    CARD8   type;
    CARD8   pad1;
    CARD16  sequenceNumber;
    CARD32  length;
    CARD32  window;
    CARD32  pad2;
    CARD32  pad3;
    CARD32  pad4;
    CARD32  pad5;
    CARD32  pad6;
} SELinuxGetSelectionManagerReply;

typedef struct {
    CARD8   reqType;
    CARD8   SELinuxReqType;
    CARD16  length;
    CARD8   permanent;
    CARD8   unused;
    CARD16  context_len;
} SELinuxSetCreateContextReq;

typedef struct {
    CARD8   reqType;
    CARD8   SELinuxReqType;
    CARD16  length;
} SELinuxGetCreateContextReq;

typedef struct {
    CARD8   type;
    CARD8   permanent;
    CARD16  sequenceNumber;
    CARD32  length;
    CARD16  context_len;
    CARD16  pad1;
    CARD32  pad2;
    CARD32  pad3;
    CARD32  pad4;
    CARD32  pad5;
    CARD32  pad6;
} SELinuxGetCreateContextReply;

typedef struct {
    CARD8   reqType;
    CARD8   SELinuxReqType;
    CARD16  length;
    CARD32  id;
    CARD16  unused;
    CARD16  context_len;
} SELinuxSetContextReq;

typedef struct {
    CARD8   reqType;
    CARD8   SELinuxReqType;
    CARD16  length;
    CARD32  id;
} SELinuxGetContextReq;

typedef struct {
    CARD8   reqType;
    CARD8   SELinuxReqType;
    CARD16  length;
    CARD32  window;
    CARD32  property;
} SELinuxGetPropertyContextReq;

typedef struct {
    CARD8   type;
    CARD8   pad1;
    CARD16  sequenceNumber;
    CARD32  length;
    CARD16  context_len;
    CARD16  pad2;
    CARD32  pad3;
    CARD32  pad4;
    CARD32  pad5;
    CARD32  pad6;
    CARD32  pad7;
} SELinuxGetContextReply;


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
#define SECCLASS_X_EVENT		13
#define SECCLASS_X_FAKEEVENT		14
#define SECCLASS_X_RESOURCE		15

#endif /* _XSELINUX_H */
