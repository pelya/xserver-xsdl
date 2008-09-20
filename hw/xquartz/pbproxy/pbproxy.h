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

#define DEBUG 1

/* from main.m */
extern void x_set_is_active (BOOL state);
extern BOOL x_get_is_active (void);
extern id x_selection_object (void);
extern Time x_current_timestamp (void);

extern Display *x_dpy;
extern int x_apple_wm_event_base, x_apple_wm_error_base;

/* from x-input.m */
extern void x_input_register (void);
extern void x_input_run (void);
 
#if DEBUG == 0
# define DB(msg, args...) do {} while (0)
#else
# define DB(msg, args...) debug_printf("%s:%s:%d " msg, __FILE__, __FUNCTION__, __LINE__, ##args)
#endif

#define TRACE() DB("TRACE\n")
extern void debug_printf (const char *fmt, ...);

#endif /* PBPROXY_H */
