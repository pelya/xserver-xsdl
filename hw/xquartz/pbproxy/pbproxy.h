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

#ifndef INTEGRATED_XPBPROXY
extern BOOL prefs_reload;
#endif

/* from main.m */
extern void x_set_is_active (BOOL state);
extern BOOL x_get_is_active (void);
extern id x_selection_object (void);
extern Time x_current_timestamp (void);
extern BOOL x_init (void);

extern Display *x_dpy;
extern int x_apple_wm_event_base, x_apple_wm_error_base;
extern int x_xfixes_event_base, x_xfixes_error_base;
extern BOOL have_xfixes;

/* from x-input.m */
extern BOOL x_input_register (void);
extern void x_input_run (void);

#ifdef DEBUG
/* BEWARE: this can cause a string memory leak, according to the leaks program. */
# define DB(msg, args...) debug_printf("%s:%s:%d " msg, __FILE__, __FUNCTION__, __LINE__, ##args)
#else
# define DB(msg, args...) do {} while (0)
#endif

#define TRACE() DB("TRACE\n")
extern void debug_printf (const char *fmt, ...);

#endif /* PBPROXY_H */
