<!-- $XFree86: xc/programs/Xserver/hw/darwin/bundle/ko.lproj/XDarwinHelp.html.cpp,v 1.1 2001/12/04 03:36:39 torrey Exp $ -->

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
="http://www.x.org/">X Window System</a> produced by the <a HREF="http://www.XFree86.Org/">XFree86 Project, Inc.</a> The X window server for Darwin and Mac OS X provided by XFree86 is called XDarwin. XDarwin runs on Mac OS X in full screen or rootless modes.</p>
<p>In full screen mode, when the X window system is active, it takes over the entire screen. You can switch back to the Mac OS X desktop by holding down Command-Option-A. This key combination can be changed in the user preferences. From the Mac OS X desktop, click on the XDarwin icon in the Dock to switch back to the X window system.  (You can change this behavior in the user preferences so that you must click the XDarwin icon in the floating switch window instead.)</p>
<p>In rootless mode, the X window system and Aqua share your display. The root window of the X11 display is the size of the screen and contains all the other windows. The X11 root window is not displayed in rootless mode as Aqua handles the desktop background.</p>
<h3>Multi-Button Mouse Emulation</h3>
<p>Many X11 applications rely on the use of a 3-button mouse. You can emulate a 3-button mouse with a single button by holding down various modifier keys while you click the mouse button. This is controlled by settings in the "Multi-Button Mouse Emulation" section of the "General" preferences. By default, emulation is on and holding down the command key and clicking the mouse button will simulate clicking the second mouse button. Holding down the option key and clicking will simulate the third button. You can change to any combination of modifiers to emulate buttons two and three in the preferences. Note, even if the modifiers keys are mapped to some other key with xmodmap, you still must use the actual keys specified in the preferences for multi-button mouse emulation.</p>

<h2><a NAME="path">Setting Your Path</a></h2>
<p>Your path is the list of directories to be searched for executable commands. The X11 commands are located in <code>/usr/X11R6/bin</code>, which needs to be added to your path. XDarwin does this for you by default and can also add additional directories where you have installed command line applications.</p>
<p>More experienced users will have already set their path correctly using the initialization files for their shell. In this case, you can inform XDarwin not to modify your path in the preferences. XDarwin launches the initial X11 clients in the user's default login shell. (An alternate shell can also be specified in the preferences.) The way to set the path depends on the shell you are using. This is described in the man page documentation for the shell.</p>
<p>In addition you may also want to add the man pages from XFree86 to the list of pages to be searched when you are looking for documentation. The X11 man pages are located in <code>/usr/X11R6/man</code> and the <code>MANPATH</code> environment variable contains the list of directories to search.</p>

<h2><a NAME="prefs">User Preferences</a></h2>
<p>A number of options may be set from the user preferences, accessible from the "Preferences..." menu item in the "XDarwin" menu. The options listed as start up options will not take effect until you have restarted XDarwin. All other options take effect immediately. The various options are described below:</p>
<h3>General</h3>
<ul>
    <li><b>Use System beep for X11:</b> When enabled the standard Mac OS X alert sound is used as the X11 bell. When disabled (default) a simple tone is used.</li>
    <li><b>Allow X11 to change mouse acceleration:</b> In a standard X window system implementation, the window manager can change the mouse acceleration. This can lead to confusion as the mouse acceleration may be set to different values by the Mac OS X System Preferences and the X window manager. By default, X11 is not allowed to change the mouse acceleration to avoid this problem.</li>
    <li><b>Multi-Button Mouse Emulation:</b> This is described above under <a HREF="#usage">Usage</a>. When emulation is enabled the selected modifiers must be held down when the mouse button is pushed to emulate the second or third mouse buttons.</li>
</ul>
<h3>Start Up</h3>
<ul>
    <li><b>Default Mode:</b> If the user does not indicate whether to run in full screen or rootless mode, the mode specified here will be used.</li>
    <li><b>Show mode pick panel on startup:</b> By default, a panel is displayed when XDarwin is started to allow the user to choose between full screen or rootless mode. If this option is turned off, the default mode will be started automatically.</li>
    <li><b>X11 Display number:</b> X11 allows there to be multiple displays managed by separate X servers on a single computer. The user may specify an integer display number for XDarwin to use if more than one X server is going to be run simultaneously.</li>
    <li><b>Allow Xinerama multiple monitor support:</b> XDarwin supports multiple monitors with Xinerama, which treats all monitors as being part of one large rectangular screen. You can disable Xinerama with this option, but currently XDarwin does not handle multiple monitors correctly without it. If you only have a single monitor, Xinerama is automatically disabled.</li>
    <li><b>Keymapping File:</b> A keymapping file is read at startup and translated to an X11 keymap. Keymapping files, available for a wide variety of languages, are found in <code>/System/Library/Keyboards</code>.</li>
    <li><b>Starting First X11 Clients:</b> When XDarwin is started from the Finder, it will run <code>xinit</code> to launch the X window manager and other X clients. (See "<code>man xinit</code>" for more information.) Before XDarwin runs <code>xinit</code> it will add the specified directories to the user's path. By default only <code>/usr/X11R6/bin</code> is added. Additional directories may added, separated by a colon. The X clients are started in the user's default login shell so that the user's shell initialization files are read. If desired, an alternate shell may be specified.</li>
</ul>
<h3>Full Screen</h3>
<ul>
    <li><b>Key combination button:</b> Click this button and then press any number of modifiers followed by a standard key to change the key combination to switch between Aqua and X11.</li>
    <li><b>Click on icon in Dock switches to X11:</b> Enable this to activate switching to X11 by clicking on the XDarwin icon in the Dock. On some versions of Mac OS X, switching by clicking in the Dock can cause the cursor to disappear on returning to Aqua.</li>
    <li><b>Show help on startup:</b> This will show an introductory splash screen when XDarwin is started in full screen mode.</li>
    <li><b>Color bit depth:</b> In full screen mode, the X11 display can use a different color bit depth than is used by Aqua. If "Current" is specified, the depth used by Aqua when XDarwin starts will be used. Otherwise 8, 15, or 24 bits may be specified.</li>
</ul>

<h2><a NAME="license">License</a></h2>
The XFree86 Project is committed to providing freely redistributable binary and source releases. The main license we use is one based on the traditional MIT X11 / X Consortium License, which does not impose any conditions on modification or redistribution of source code or binaries other than requiring that copyright/license notices are left intact. For more information and additional copyright/licensing notices covering some sections of the code, please see the <A HREF="http://www.xfree86.org/legal/licence.html">XFree86
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
