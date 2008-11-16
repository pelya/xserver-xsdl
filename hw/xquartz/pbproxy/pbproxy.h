/* pbproxy.h
   Copyright (c) 2002 Apple Computer, Inc. All rights reserved. */

#ifndef PBPROXY_H
#define PBPROXY_H 1

#import  <Foundation/Foundation.h>

#define  Cursor X_Cursor
#undef _SHAPE_H_
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#undef   Cursor

#ifdef STANDALONE_XPBPROXY
/* Just used for the standalone to respond to SIGHUP to reload prefs */
extern BOOL xpbproxy_prefs_reload;

/* Setting this to YES (for the standalone app) causes us to ignore the
 * 'sync_pasteboard' defaults preference since we assume it to be on... this is
 * mainly useful for debugging/developing xpbproxy with XQuartz still running.
 * Just disable the one in the server with X11's preference pane, then run
 * the standalone app.
 */
extern BOOL xpbproxy_is_standalone;
#endif

/* from main.m */
extern void xpbproxy_set_is_active (BOOL state);
extern BOOL xpbproxy_get_is_active (void);
extern id xpbproxy_selection_object (void);
extern Time xpbproxy_current_timestamp (void);
extern BOOL xpbproxy_init (void);

extern Display *xpbproxy_dpy;
extern int xpbproxy_apple_wm_event_base, xpbproxy_apple_wm_error_base;
extern int xpbproxy_xfixes_event_base, xpbproxy_xfixes_error_base;
extern BOOL xpbproxy_have_xfixes;

/* from x-input.m */
extern BOOL xpbproxy_input_register (void);
extern void xpbproxy_input_run (void);

#ifdef DEBUG
/* BEWARE: this can cause a string memory leak, according to the leaks program. */
# define DB(msg, args...) debug_printf("%s:%s:%d " msg, __FILE__, __FUNCTION__, __LINE__, ##args)
#else
# define DB(msg, args...) do {} while (0)
#endif

#define TRACE() DB("TRACE\n")
extern void debug_printf (const char *fmt, ...);

#endif /* PBPROXY_H */
