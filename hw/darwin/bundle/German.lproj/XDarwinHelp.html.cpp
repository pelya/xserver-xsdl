<!-- $XFree86: xc/programs/Xserver/hw/darwin/bundle/German.lproj/XDarwinHelp.html.cpp,v 1.1 2001/11/03 00:25:34 torrey Exp $ -->

#include "xf86Version.h"
#ifndef PRE_RELEASE
#define PRE_RELEASE XF86_VERSION_SNAP
#endif

<html>
<head>
<title>XFree86 for Mac OS X</title>
</head>
<body>
<center>
    <h1>XFree86 on Darwin and Mac OS X</h1>
    XFree86 XF86_VERSION<br>
    Release Date: XF86_REL_DATE
</center>
<h2>Contents</h2>
<ol>
    <li><A HREF="#notice">Important Notice</A></li>
    <li><A HREF="#usage">Usage</A></li>
    <li><A HREF="#path">Setting Your Path</A></li>
    <li><A HREF="#prefs">User Preferences</A></li>
    <li><A HREF="#license">License</A></li>
</ol>
<center>
    <h2><a NAME="notice">Important Notice</a></h2>
</center>
<blockquote>
#if PRE_RELEASE
This is a pre-release version of XFree86, and is not supported in any way. Bugs may be reported and patches may be submitted to the <A HREF="http://sourceforge.net/projects/xonx/">XonX project page</A> at SourceForge.  Before reporting bugs in pre-release versions, please check the latest version from <A HREF="http://sourceforge.net/projects/xonx/">XonX</A> or in the <A HREF="http://www.XFree86.Org/cvs">XFree86 CVS repository</A>.
#else
If the server is older than 6-12 months, or if your hardware is newer than the above date, look for a newer version before reporting problems. Bugs may be reported and patches may be submitted to the <A HREF="http://sourceforge.net/projects/xonx/">XonX project page</A> at SourceForge.
#endif
</blockquote>
<blockquote>
This software is distributed under the terms of the <A HREF="#license">MIT X11 / X Consortium License</A> and is provided AS IS, with no warranty. Please read the <A HREF="#license">License</A> before using.</blockquote>
<h2><a NAME="usage">Usage</a></h2>
<p>XFree86 is a freely redistributable open-source implementation of the <a HREF
="http://www.x.org/">X Window System</a> produced by the <a HREF="http://www.XFree86.Org/">XFree86 Project, Inc.</a> XFree86 runs on Mac OS X in full screen mode. When the X window system is active, it takes over the entire screen. You can switch back to the Mac OS X desktop by holding down Command-Option-A. This key combination can be changed in the user preferences. From the Mac OS X desktop, just click on the XDarwin icon in the floating switch window to switch back to the X window system. You can change this behavior in the user preferences so that clicking on the XDarwin icon in the Dock switches as well.</p>
<h3>Multi-Button Mouse Emulation</h3>
<p>Many X11 applications rely on the use of a 3-button mouse. To emulate a 3-button mouse with a single button, select "Enable emulation of multiple mouse buttons" in the Preferences. When emulating a 3-button mouse, holding down the left command key and clicking the mouse button will simulate clicking the second mouse button. Holding down the left option key and clicking will simulate the third button.</p>
<p>Notes:</p>
<ul>
    <li>With most keyboards the left and right command and option keys are not differentiated so either will work.
    <li>Even with command and/or option keys mapped to some other key with xmodmap, you still must use the original command and option keys for multibutton mouse emulation.
    <li>The only way to simulate holding down the left command key and clicking the second mouse button is to map some other key to be the left command key. The same is true for simulating holding down the left option key and clicking the third mouse button.
</ul>
<h2><a NAME="path">Setting Your Path</a></h2>
<p>The X11 binaries are located in /usr/X11R6/bin, which you may need to add to your path. Your path is the list of directories to be searched for executable commands. The way to do this depends on the shell you are using. The following directions are for tcsh, which is the default shell on Darwin and Mac OS X.</p>
<p>You can check your path by typing "printenv PATH". You should see /usr/X11R6/bin listed as one of the directories. If not, you should add it to your default path. To do so, you can add the following line to the file ~/Library/init/tcsh/path: (You may need to create this file and directory path if it does not exist already.)</p>
<blockquote>setenv PATH "${PATH}:/usr/X11R6/bin"</blockquote>
<p>Note that if you have created a .cshrc or .tcshrc file, these files will override your settings in ~/Library/init/tcsh/ and you will need to change one of these files instead. These changes will not take effect until you open a new Terminal window. You may also want to add the man pages from XFree86 to the list of pages to be searched when you are looking for documentation. The X11 man pages are located in /usr/X11R6/man and the MANPATH environment variable contains the list of directories to search.</p>
<h2><a NAME="prefs">User Preferences</a></h2>
<p>A number of options may be set from the user preferences, accessible from the "Preferences..." menu item in the "XDarwin" menu. The options listed under Startup Options will not take effect until you have restarted XDarwin. All other options take effect immediately. The various options are described below:</p>
<ul>
    <li>Key combination button: Click this button and then press any number of modifiers followed by a standard key to change the key combination to switch between Aqua and X11.</li>
    <li>Use System beep for X11: When enabled the standard Mac OS X alert sound is used as X11 bell. When disabled (default) a simple tone is used.</li>
    <li>Click on icon in Dock switches to X11: Enable this to activate switching to X11 by clicking on the XDarwin icon in the Dock. On some versions of Mac OS X, switching by clicking in the Dock can cause the cursor to disappear on returning to Aqua.</li>
    <li>Show help on startup: This will show the introductory splash screen when XDarwin is launched.</li>
    <li>Display number: This sets what X display number XDarwin should assign to the display. Note that XDarwin always takes over the main display when showing X11.</li>
    <li>Keymapping: By default, XDarwin loads the keymapping from the Darwin kernel on startup. On portables, this keymapping is sometimes empty so that the keyboard will appear to be dead in X11. If "Load from file" is selected, XDarwin will load the keymapping from the specified file instead.</li>
</ul>
<h2><a NAME="license">License</a></h2>
The XFree86 Project is committed to providing freely redistributable binary and source releases. The main license we use is one based on the traditional MIT X11 / X Consortium License, which doesn't impose any conditions on modification or redistribution of source code or binaries other than requiring that copyright/license notices are left intact. For more information and additional copyright/licensing notices covering some sections of the code, please see the <A HREF="http://www.xfree86.org/legal/licence.html">XFree86
License page</A>.
<H3><A NAME="3"></A>X Consortium License</H3>
<p>Copyright (C) 1996 X Consortium</p>
<p>Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without
limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to
whom the Software is furnished to do so, subject to the following conditions:</p>
<p>The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.</p>
<p>THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT
SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.</p>
<p>Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization from
the X Consortium.</p>
<p>X Window System is a trademark of X Consortium, Inc.</p>
</body>
</html>
