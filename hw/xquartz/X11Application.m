/* X11Application.m -- subclass of NSApplication to multiplex events
 
 Copyright (c) 2002-2008 Apple Inc.
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation files
 (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge,
 publish, distribute, sublicense, and/or sell copies of the Software,
 and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS IN THE SOFTWARE.
 
 Except as contained in this notice, the name(s) of the above
 copyright holders shall not be used in advertising or otherwise to
 promote the sale, use or other dealings in this Software without
 prior written authorization. */

#include "sanitizedCarbon.h"

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "quartzCommon.h"

#import "X11Application.h"

#include "darwin.h"
#include "darwinEvents.h"
#include "quartz.h"
#define _APPLEWM_SERVER_
#include "X11/extensions/applewm.h"
#include "micmap.h"

#include <mach/mach.h>
#include <unistd.h>

#include <Xplugin.h>

#define DEFAULTS_FILE "/usr/X11/lib/X11/xserver/Xquartz.plist"

#ifndef XSERVER_VERSION
#define XSERVER_VERSION "?"
#endif

#define ProximityIn    0
#define ProximityOut   1

int X11EnableKeyEquivalents = TRUE;
int quartzHasRoot = FALSE, quartzEnableRootless = TRUE;

extern int darwinFakeButtons;
extern Bool enable_stereo;

X11Application *X11App;

#define ALL_KEY_MASKS (NSShiftKeyMask | NSControlKeyMask | NSAlternateKeyMask | NSCommandKeyMask)

@implementation X11Application

typedef struct message_struct message;
struct message_struct {
    mach_msg_header_t hdr;
    SEL selector;
    NSObject *arg;
};

static mach_port_t _port;

static void send_nsevent(NSEvent *e);

/* Quartz mode initialization routine. This is often dynamically loaded
   but is statically linked into this X server. */
Bool QuartzModeBundleInit(void);

static void init_ports (void) {
    kern_return_t r;
    NSPort *p;
	
    if (_port != MACH_PORT_NULL) return;
	
    r = mach_port_allocate (mach_task_self (), MACH_PORT_RIGHT_RECEIVE, &_port);
    if (r != KERN_SUCCESS) return;
	
    p = [NSMachPort portWithMachPort:_port];
    [p setDelegate:NSApp];
    [p scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
}

static void message_kit_thread (SEL selector, NSObject *arg) {
    message msg;
    kern_return_t r;
	
    msg.hdr.msgh_bits = MACH_MSGH_BITS (MACH_MSG_TYPE_MAKE_SEND, 0);
    msg.hdr.msgh_size = sizeof (msg);
    msg.hdr.msgh_remote_port = _port;
    msg.hdr.msgh_local_port = MACH_PORT_NULL;
    msg.hdr.msgh_reserved = 0;
    msg.hdr.msgh_id = 0;
	
    msg.selector = selector;
    msg.arg = [arg retain];
	
    r = mach_msg (&msg.hdr, MACH_SEND_MSG, msg.hdr.msgh_size,
		  0, MACH_PORT_NULL, 0, MACH_PORT_NULL);
    if (r != KERN_SUCCESS)
		ErrorF("%s: mach_msg failed: %x\n", __FUNCTION__, r);
}

- (void) handleMachMessage:(void *)_msg {
    message *msg = _msg;
	
    [self performSelector:msg->selector withObject:msg->arg];
    [msg->arg release];
}

- (void) set_controller:obj {
    if (_controller == nil) _controller = [obj retain];
}

- (void) dealloc {
    if (_controller != nil) [_controller release];
	
    if (_port != MACH_PORT_NULL)
		mach_port_deallocate (mach_task_self (), _port);
	
    [super dealloc];
}

- (void) orderFrontStandardAboutPanel: (id) sender {
    NSMutableDictionary *dict;
    NSDictionary *infoDict;
    NSString *tem;
    
    dict = [NSMutableDictionary dictionaryWithCapacity:3];
    infoDict = [[NSBundle mainBundle] infoDictionary];
    
    [dict setObject: NSLocalizedString (@"The X Window System", @"About panel")
          forKey:@"ApplicationName"];
    
    tem = [infoDict objectForKey:@"CFBundleShortVersionString"];
    
    [dict setObject:[NSString stringWithFormat:@"XQuartz %@", tem]
          forKey:@"ApplicationVersion"];

    [dict setObject:[NSString stringWithFormat:@"xorg-server %s", XSERVER_VERSION]
          forKey:@"Version"];
    
    [self orderFrontStandardAboutPanelWithOptions: dict];
}

- (void) activateX:(OSX_BOOL)state {
    /* Create a TSM document that supports full Unicode input, and
	 have it activated while X is active (unless using the old
	 keymapping files) */
    static TSMDocumentID x11_document;
	DEBUG_LOG("state=%d, _x_active=%d, \n", state, _x_active)
    if (state) {
		DarwinSendDDXEvent(kXquartzActivate, 0);

		if (!_x_active) {
			if (x11_document == 0) {
				OSType types[1];
				types[0] = kUnicodeDocument;
				NewTSMDocument (1, types, &x11_document, 0);
			}

			if (x11_document != 0)	ActivateTSMDocument (x11_document);
		}
    } else {
		DarwinSendDDXEvent(kXquartzDeactivate, 0);

		if (_x_active && x11_document != 0)
			DeactivateTSMDocument (x11_document);
	}

	_x_active = state;
}

- (void) became_key:(NSWindow *)win {
	[self activateX:NO];
}

- (void) sendEvent:(NSEvent *)e {
    NSEventType type;
    OSX_BOOL for_appkit, for_x;
    
    type = [e type];
    
    /* By default pass down the responder chain and to X. */
    for_appkit = YES;
    for_x = YES;
    
    switch ([e type]) {
        case NSLeftMouseDown: case NSRightMouseDown: case NSOtherMouseDown:
        case NSLeftMouseUp: case NSRightMouseUp: case NSOtherMouseUp:
            if ([e window] != nil) {
                /* Pointer event has an (AppKit) window. Probably something for the kit. */
                for_x = NO;
                if (_x_active) [self activateX:NO];
            } else if ([self modalWindow] == nil) {
                /* Must be an X window. Tell appkit it doesn't have focus. */
                for_appkit = NO;
                
                if ([self isActive]) {
                    [self deactivate];
                    if (!_x_active && quartzProcs->IsX11Window([e window],
                                                               [e windowNumber]))
                        [self activateX:YES];
                }
            }
            break;
            
        case NSKeyDown: case NSKeyUp:
            
            if(_x_active) {
                static int swallow_up;
                
                if([e type] == NSKeyDown) {
                    /* Before that though, see if there are any global
                     shortcuts bound to it. */

                    if(darwinAppKitModMask & [e modifierFlags]) {
                        /* Override to force sending to Appkit */
                        swallow_up = [e keyCode];
                        for_x = NO;
#if 0
                    } else if(!quartzEnableRootless
                              && ([e modifierFlags] & ALL_KEY_MASKS) == (NSCommandKeyMask | NSAlternateKeyMask)
                              && ([e keyCode] == 0 /*a*/ || [e keyCode] == 53 /*Esc*/)) {
                        swallow_up = [e keyCode];
                        for_x = NO;
                        DarwinSendDDXEvent(kXquartzToggleFullscreen, 0);
#endif
#if XPLUGIN_VERSION >= 1
                    } else if(X11EnableKeyEquivalents &&
                              xp_is_symbolic_hotkey_event([e eventRef])) {
                        swallow_up = [e keyCode];
                        for_x = NO;
#endif
                    } else if(X11EnableKeyEquivalents &&
                              [[self mainMenu] performKeyEquivalent:e]) {
                        for_appkit = NO;
                        swallow_up = [e keyCode];
                        for_x = NO;
                    } else {
                        /* No kit window is focused, so send it to X. */
                        for_appkit = NO;
                    }
                } else { /* KeyUp */
                    /* If we saw a key equivalent on the down, don't pass
                     the up through to X. */
                    
                    if (swallow_up != 0 && [e keyCode] == swallow_up) {
                        swallow_up = 0;
                        for_x = NO;
                    }
                }
            } else { /* !_x_active */
                for_x = NO;
            }
            break;
            
        case NSFlagsChanged:
            /* Don't tell X11 about modifiers changing while it's not active */
            if (!_x_active)
                for_x = NO;
            break;
            
        case NSAppKitDefined:
            switch ([e subtype]) {
                case NSApplicationActivatedEventType:
                    for_x = NO;
                    if ([self modalWindow] == nil) {
                        for_appkit = NO;
                        
                        /* FIXME: hack to avoid having to pass the event to appkit,
                         which would cause it to raise one of its windows. */
                        _appFlags._active = YES;
                        
                        [self activateX:YES];
                        if ([e data2] & 0x10) 
                            DarwinSendDDXEvent(kXquartzBringAllToFront, 0);
                    }
                    break;
                    
                case 18: /* ApplicationDidReactivate */
                    if (quartzHasRoot) for_appkit = NO;
                    break;
                    
                case NSApplicationDeactivatedEventType:
                    for_x = NO;
                    [self activateX:NO];
                    break;
            }
            break;
            
        default: break; /* for gcc */
    }
    
    if (for_appkit) [super sendEvent:e];
    
    if (for_x) send_nsevent(e);
}

- (void) set_window_menu:(NSArray *)list {
	[_controller set_window_menu:list];
}

- (void) set_window_menu_check:(NSNumber *)n {
	[_controller set_window_menu_check:n];
}

- (void) set_apps_menu:(NSArray *)list {
	[_controller set_apps_menu:list];
}

- (void) set_front_process:unused {
	[NSApp activateIgnoringOtherApps:YES];

	if ([self modalWindow] == nil)
		[self activateX:YES];
}

- (void) set_can_quit:(NSNumber *)state {
	[_controller set_can_quit:[state boolValue]];
}

- (void) server_ready:unused {
	[_controller server_ready];
}

- (void) show_hide_menubar:(NSNumber *)state {
	if ([state boolValue]) ShowMenuBar ();
	else HideMenuBar ();
}


/* user preferences */

/* Note that these functions only work for arrays whose elements
 can be toll-free-bridged between NS and CF worlds. */

static const void *cfretain (CFAllocatorRef a, const void *b) {
    return CFRetain (b);
}

static void cfrelease (CFAllocatorRef a, const void *b) {
    CFRelease (b);
}

static CFMutableArrayRef nsarray_to_cfarray (NSArray *in) {
	CFMutableArrayRef out;
	CFArrayCallBacks cb;
	NSObject *ns;
	const CFTypeRef *cf;
	int i, count;

	memset (&cb, 0, sizeof (cb));
	cb.version = 0;
	cb.retain = cfretain;
	cb.release = cfrelease;

	count = [in count];
	out = CFArrayCreateMutable (NULL, count, &cb);

	for (i = 0; i < count; i++) {
		ns = [in objectAtIndex:i];

		if ([ns isKindOfClass:[NSArray class]])
			cf = (CFTypeRef) nsarray_to_cfarray ((NSArray *) ns);
		else
			cf = CFRetain ((CFTypeRef) ns);

		CFArrayAppendValue (out, cf);
		CFRelease (cf);
	}

	return out;
}

static NSMutableArray * cfarray_to_nsarray (CFArrayRef in) {
	NSMutableArray *out;
	const CFTypeRef *cf;
	NSObject *ns;
	int i, count;

	count = CFArrayGetCount (in);
	out = [[NSMutableArray alloc] initWithCapacity:count];

	for (i = 0; i < count; i++) {
		cf = CFArrayGetValueAtIndex (in, i);

		if (CFGetTypeID (cf) == CFArrayGetTypeID ())
			ns = cfarray_to_nsarray ((CFArrayRef) cf);
		else
			ns = [(id)cf retain];

		[out addObject:ns];
		[ns release];
	}

	return out;
}

- (CFPropertyListRef) prefs_get:(NSString *)key {
    CFPropertyListRef value;
	
    value = CFPreferencesCopyAppValue ((CFStringRef) key, CFSTR (APP_PREFS));
	
    if (value == NULL) {
      static CFDictionaryRef defaults;
      
      if (defaults == NULL) {
	CFStringRef error = NULL;
	CFDataRef data;
	CFURLRef url;
	SInt32 error_code;
	
	url = (CFURLCreateFromFileSystemRepresentation
	       (NULL, (unsigned char *)DEFAULTS_FILE, strlen (DEFAULTS_FILE), false));
	if (CFURLCreateDataAndPropertiesFromResource (NULL, url, &data,
						      NULL, NULL, &error_code)) {
	  defaults = (CFPropertyListCreateFromXMLData
		      (NULL, data, kCFPropertyListMutableContainersAndLeaves, &error));
	  if (error != NULL) CFRelease (error);
	  CFRelease (data);
	}
	CFRelease (url);
			
	if (defaults != NULL) {
	  NSMutableArray *apps, *elt;
	  int count, i;
	  NSString *name, *nname;
	  
	  /* Localize the names in the default apps menu. */
	  
	  apps = [(NSDictionary *)defaults objectForKey:@PREFS_APPSMENU];
	  if (apps != nil) {
	    count = [apps count];
	    for (i = 0; i < count; i++)	{
	      elt = [apps objectAtIndex:i];
	      if (elt != nil && [elt isKindOfClass:[NSArray class]]) {
		name = [elt objectAtIndex:0];
		if (name != nil) {
		  nname = NSLocalizedString (name, nil);
		  if (nname != nil && nname != name)
		    [elt replaceObjectAtIndex:0 withObject:nname];
		}
	      }
	    }
	  }
	}
      }
		
      if (defaults != NULL) value = CFDictionaryGetValue (defaults, key);
      if (value != NULL) CFRetain (value);
    }
	
    return value;
}

- (int) prefs_get_integer:(NSString *)key default:(int)def {
  CFPropertyListRef value;
  int ret;
  
  value = [self prefs_get:key];
  
  if (value != NULL && CFGetTypeID (value) == CFNumberGetTypeID ())
    CFNumberGetValue (value, kCFNumberIntType, &ret);
  else if (value != NULL && CFGetTypeID (value) == CFStringGetTypeID ())
    ret = CFStringGetIntValue (value);
  else
    ret = def;
  
  if (value != NULL) CFRelease (value);
  
  return ret;
}

- (const char *) prefs_get_string:(NSString *)key default:(const char *)def {
  CFPropertyListRef value;
  const char *ret = NULL;
  
  value = [self prefs_get:key];
  
  if (value != NULL && CFGetTypeID (value) == CFStringGetTypeID ()) {
    NSString *s = (NSString *) value;
    
    ret = [s UTF8String];
  }
  
  if (value != NULL) CFRelease (value);
  
  return ret != NULL ? ret : def;
}

- (float) prefs_get_float:(NSString *)key default:(float)def {
  CFPropertyListRef value;
  float ret = def;
  
  value = [self prefs_get:key];
  
  if (value != NULL
      && CFGetTypeID (value) == CFNumberGetTypeID ()
      && CFNumberIsFloatType (value))
    CFNumberGetValue (value, kCFNumberFloatType, &ret);
  else if (value != NULL && CFGetTypeID (value) == CFStringGetTypeID ())
    ret = CFStringGetDoubleValue (value);
	
  if (value != NULL) CFRelease (value);
  
  return ret;
}

- (int) prefs_get_boolean:(NSString *)key default:(int)def {
  CFPropertyListRef value;
  int ret = def;
  
  value = [self prefs_get:key];
  
  if (value != NULL) {
    if (CFGetTypeID (value) == CFNumberGetTypeID ())
      CFNumberGetValue (value, kCFNumberIntType, &ret);
    else if (CFGetTypeID (value) == CFBooleanGetTypeID ())
      ret = CFBooleanGetValue (value);
    else if (CFGetTypeID (value) == CFStringGetTypeID ()) {
      const char *tem = [(NSString *) value UTF8String];
      if (strcasecmp (tem, "true") == 0 || strcasecmp (tem, "yes") == 0)
	ret = YES;
      else
	ret = NO;
    }
    
    CFRelease (value);
  }
  return ret;
}

- (NSArray *) prefs_get_array:(NSString *)key {
  NSArray *ret = nil;
  CFPropertyListRef value;
  
  value = [self prefs_get:key];
  
  if (value != NULL) {
    if (CFGetTypeID (value) == CFArrayGetTypeID ())
      ret = [cfarray_to_nsarray (value) autorelease];
    
    CFRelease (value);
  }
  
  return ret;
}

- (void) prefs_set_integer:(NSString *)key value:(int)value {
    CFNumberRef x;
	
    x = CFNumberCreate (NULL, kCFNumberIntType, &value);
	
    CFPreferencesSetValue ((CFStringRef) key, (CFTypeRef) x, CFSTR (APP_PREFS),
			   kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
	
    CFRelease (x);
}

- (void) prefs_set_float:(NSString *)key value:(float)value {
    CFNumberRef x;
	
    x = CFNumberCreate (NULL, kCFNumberFloatType, &value);
	
    CFPreferencesSetValue ((CFStringRef) key, (CFTypeRef) x, CFSTR (APP_PREFS),
			   kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
	
    CFRelease (x);
}

- (void) prefs_set_boolean:(NSString *)key value:(int)value {
  CFPreferencesSetValue ((CFStringRef) key,
			 (CFTypeRef) value ? kCFBooleanTrue
			 : kCFBooleanFalse, CFSTR (APP_PREFS),
			 kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
  
}

- (void) prefs_set_array:(NSString *)key value:(NSArray *)value {
  CFArrayRef cfarray;
  
  cfarray = nsarray_to_cfarray (value);
  CFPreferencesSetValue ((CFStringRef) key,
			 (CFTypeRef) cfarray,
			 CFSTR (APP_PREFS),
			 kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
  CFRelease (cfarray);
}

- (void) prefs_set_string:(NSString *)key value:(NSString *)value {
  CFPreferencesSetValue ((CFStringRef) key, (CFTypeRef) value,
			 CFSTR (APP_PREFS), kCFPreferencesCurrentUser,
			 kCFPreferencesAnyHost);
}

- (void) prefs_synchronize {
    CFPreferencesAppSynchronize (kCFPreferencesCurrentApplication);
}

- (void) read_defaults
{
    NSString *nsstr;
    const char *tem;
	
    quartzUseSysBeep = [self prefs_get_boolean:@PREFS_SYSBEEP
                        default:quartzUseSysBeep];

    // TODO: Add fullscreen support
    //quartzEnableRootless = [self prefs_get_boolean:@PREFS_ROOTLESS
    //                    default:quartzEnableRootless];
#ifdef DARWIN_DDX_MISSING
    quartzFullscreenDisableHotkeys = ![self prefs_get_boolean:
					      @PREFS_FULLSCREEN_HOTKEYS default:
					      !quartzFullscreenDisableHotkeys];
    quartzXpluginOptions = [self prefs_get_integer:@PREFS_XP_OPTIONS
                            default:quartzXpluginOptions];
#endif
    darwinFakeButtons = [self prefs_get_boolean:@PREFS_FAKEBUTTONS
                         default:darwinFakeButtons];
    if (darwinFakeButtons) {
        const char *fake2, *fake3;

        fake2 = [self prefs_get_string:@PREFS_FAKE_BUTTON2 default:NULL];
        fake3 = [self prefs_get_string:@PREFS_FAKE_BUTTON3 default:NULL];

        if (fake2 != NULL) darwinFakeMouse2Mask = DarwinParseModifierList(fake2, TRUE);
        if (fake3 != NULL) darwinFakeMouse3Mask = DarwinParseModifierList(fake3, TRUE);
    }

    tem = [self prefs_get_string:@PREFS_APPKIT_MODIFIERS default:NULL];
    if (tem != NULL) darwinAppKitModMask = DarwinParseModifierList(tem, TRUE);
	
    tem = [self prefs_get_string:@PREFS_WINDOW_ITEM_MODIFIERS default:NULL];
    if (tem != NULL) {
        windowItemModMask = DarwinParseModifierList(tem, FALSE);
    } else {
        nsstr = NSLocalizedString (@"window item modifiers", @"window item modifiers");
        if(nsstr != NULL) {
            tem = [nsstr UTF8String];
            if((tem != NULL) && strcmp(tem, "window item modifiers")) {
                windowItemModMask = DarwinParseModifierList(tem, FALSE);
            }
        }
    }

    X11EnableKeyEquivalents = [self prefs_get_boolean:@PREFS_KEYEQUIVS
                               default:X11EnableKeyEquivalents];
	
    darwinSyncKeymap = [self prefs_get_boolean:@PREFS_SYNC_KEYMAP
                        default:darwinSyncKeymap];
		
    darwinDesiredDepth = [self prefs_get_integer:@PREFS_DEPTH
                          default:darwinDesiredDepth];
	
//    enable_stereo = [self prefs_get_boolean:@PREFS_ENABLE_STEREO
//                     default:false];
    
    pbproxy_active = [self prefs_get_boolean:@PREFS_SYNC_PB
                                     default:pbproxy_active];
    pbproxy_pasteboard_to_clipboard = [self prefs_get_boolean:@PREFS_SYNC_PB_TO_CLIPBOARD
                                                      default:pbproxy_pasteboard_to_clipboard];
    pbproxy_pasteboard_to_primary = [self prefs_get_boolean:@PREFS_SYNC_PB_TO_PRIMARY
                                                    default:pbproxy_pasteboard_to_primary];
    pbproxy_clipboard_to_pasteboard = [self prefs_get_boolean:@PREFS_SYNC_CLIPBOARD_TO_PB
                                                      default:pbproxy_clipboard_to_pasteboard];
    pbproxy_primary_on_grab = [self prefs_get_boolean:@PREFS_SYNC_PRIMARY_ON_SELECT
                                              default:pbproxy_primary_on_grab];
}

/* This will end up at the end of the responder chain. */
- (void) copy:sender {
  DarwinSendDDXEvent(kXquartzPasteboardNotify, 1,
			     AppleWMCopyToPasteboard);
}

- (OSX_BOOL) x_active {
    return _x_active;
}

@end

static NSArray *
array_with_strings_and_numbers (int nitems, const char **items,
				const char *numbers) {
  NSMutableArray *array, *subarray;
  NSString *string, *number;
  int i;
	
  /* (Can't autorelease on the X server thread) */
  
  array = [[NSMutableArray alloc] initWithCapacity:nitems];
  
  for (i = 0; i < nitems; i++) {
    subarray = [[NSMutableArray alloc] initWithCapacity:2];
    
    string = [[NSString alloc] initWithUTF8String:items[i]];
    [subarray addObject:string];
    [string release];
    
    if (numbers[i] != 0) {
      number = [[NSString alloc] initWithFormat:@"%d", numbers[i]];
      [subarray addObject:number];
      [number release];
    } else
      [subarray addObject:@""];
    
    [array addObject:subarray];
    [subarray release];
  }
  
  return array;
}

void X11ApplicationSetWindowMenu (int nitems, const char **items,
				  const char *shortcuts) {
  NSArray *array;
  array = array_with_strings_and_numbers (nitems, items, shortcuts);
  
  /* Send the array of strings over to the appkit thread */
  
  message_kit_thread (@selector (set_window_menu:), array);
  [array release];
}

void X11ApplicationSetWindowMenuCheck (int idx) {
  NSNumber *n;
  
  n = [[NSNumber alloc] initWithInt:idx];
  
  message_kit_thread (@selector (set_window_menu_check:), n);
  
  [n release];
}

void X11ApplicationSetFrontProcess (void) {
    message_kit_thread (@selector (set_front_process:), nil);
}

void X11ApplicationSetCanQuit (int state) {
    NSNumber *n;
	
    n = [[NSNumber alloc] initWithBool:state];
	
    message_kit_thread (@selector (set_can_quit:), n);
	
    [n release];
}

void X11ApplicationServerReady (void) {
    message_kit_thread (@selector (server_ready:), nil);
}

void X11ApplicationShowHideMenubar (int state) {
    NSNumber *n;
	
    n = [[NSNumber alloc] initWithBool:state];
	
    message_kit_thread (@selector (show_hide_menubar:), n);
	
    [n release];
}

static void check_xinitrc (void) {
    char *tem, buf[1024];
    NSString *msg;
	
    if ([X11App prefs_get_boolean:@PREFS_DONE_XINIT_CHECK default:NO])
		return;
	
    tem = getenv ("HOME");
    if (tem == NULL) goto done;
	
    snprintf (buf, sizeof (buf), "%s/.xinitrc", tem);
    if (access (buf, F_OK) != 0)
		goto done;
	
    /* FIXME: put localized strings into Resources/English.lproj */
	
    msg = NSLocalizedString (@"You have an existing ~/.xinitrc file.\n\n\
Windows displayed by X11 applications may not have titlebars, or may look \
different to windows displayed by native applications.\n\n\
Would you like to move aside the existing file and use the standard X11 \
environment?", @"Startup xinitrc dialog");
	
    if (NSRunAlertPanel (nil, msg, NSLocalizedString (@"Yes", @""),
			 NSLocalizedString (@"No", @""), nil)
	== NSAlertDefaultReturn) {
      char buf2[1024];
      int i = -1;
      
      snprintf (buf2, sizeof (buf2), "%s.old", buf);
      
      for (i = 1; access (buf2, F_OK) == 0; i++)
	snprintf (buf2, sizeof (buf2), "%s.old.%d", buf, i);
      
      rename (buf, buf2);
    }
    
 done:
    [X11App prefs_set_boolean:@PREFS_DONE_XINIT_CHECK value:YES];
    [X11App prefs_synchronize];
}

void X11ApplicationMain (int argc, char **argv, char **envp) {
    NSAutoreleasePool *pool;

#ifdef DEBUG
    while (access ("/tmp/x11-block", F_OK) == 0) sleep (1);
#endif
  
    pool = [[NSAutoreleasePool alloc] init];
    X11App = (X11Application *) [X11Application sharedApplication];
    init_ports ();
    [NSApp read_defaults];
    [NSBundle loadNibNamed:@"main" owner:NSApp];
    [[NSNotificationCenter defaultCenter] addObserver:NSApp
					selector:@selector (became_key:)
					name:NSWindowDidBecomeKeyNotification object:nil];
    check_xinitrc ();

    /*
     * The xpr Quartz mode is statically linked into this server.
     * Initialize all the Quartz functions.
     */
    QuartzModeBundleInit();

    /* Calculate the height of the menubar so we can avoid it. */
    aquaMenuBarHeight = NSHeight([[NSScreen mainScreen] frame]) -
    NSMaxY([[NSScreen mainScreen] visibleFrame]);

    /* Tell the server thread that it can proceed */
    QuartzInitServer(argc, argv, envp);
    
    [NSApp run];
    /* not reached */
}

/* event conversion */

static inline unsigned short
convert_flags (unsigned int nsflags) {
    unsigned int xflags = 0;
	
    if (nsflags == ~0) return 0xffff;
	
    if (nsflags & NSAlphaShiftKeyMask)	xflags |= LockMask;
    if (nsflags & NSShiftKeyMask)	xflags |= ShiftMask;
    if (nsflags & NSControlKeyMask)	xflags |= ControlMask;
    if (nsflags & NSAlternateKeyMask)	xflags |= Mod1Mask;
    if (nsflags & NSCommandKeyMask)	xflags |= Mod2Mask;
    /* FIXME: secondaryfn? */
	
    return xflags;
}

extern int darwin_modifier_flags; // darwinEvents.c

static void send_nsevent(NSEvent *e) {
	NSRect screen;
	NSPoint location;
	NSWindow *window;
	int pointer_x, pointer_y, ev_button, ev_type;
	float pressure, tilt_x, tilt_y;

	/* convert location to be relative to top-left of primary display */
	location = [e locationInWindow];
	window = [e window];
	screen = [[[NSScreen screens] objectAtIndex:0] frame];

    if (window != nil)	{
		NSRect frame = [window frame];
		pointer_x = location.x + frame.origin.x;
		pointer_y = (((screen.origin.y + screen.size.height)
                      - location.y) - frame.origin.y);
	} else {
		pointer_x = location.x;
		pointer_y = (screen.origin.y + screen.size.height) - location.y;
	}

	pressure = 0;  // for tablets
	tilt_x = 0;
	tilt_y = 0;

    /* We don't receive modifier key events while out of focus, and 3button
     * emulation mucks this up, so we need to check our modifier flag state
     * on every event... ugg
     */
    if(darwin_modifier_flags != [e modifierFlags])
        DarwinUpdateModKeys([e modifierFlags]);

	switch ([e type]) {
		case NSLeftMouseDown:     ev_button=1; ev_type=ButtonPress;   goto handle_mouse;
		case NSOtherMouseDown:    ev_button=2; ev_type=ButtonPress;   goto handle_mouse;
		case NSRightMouseDown:    ev_button=3; ev_type=ButtonPress;   goto handle_mouse;
		case NSLeftMouseUp:       ev_button=1; ev_type=ButtonRelease; goto handle_mouse;
		case NSOtherMouseUp:      ev_button=2; ev_type=ButtonRelease; goto handle_mouse;
		case NSRightMouseUp:      ev_button=3; ev_type=ButtonRelease; goto handle_mouse;
		case NSLeftMouseDragged:  ev_button=1; ev_type=MotionNotify;  goto handle_mouse;
		case NSOtherMouseDragged: ev_button=2; ev_type=MotionNotify;  goto handle_mouse;
		case NSRightMouseDragged: ev_button=3; ev_type=MotionNotify;  goto handle_mouse;
		case NSMouseMoved:        ev_button=0; ev_type=MotionNotify;  goto handle_mouse;
        case NSTabletPoint:       ev_button=0; ev_type=MotionNotify;  goto handle_mouse;

        handle_mouse:
			if ([e type] == NSTabletPoint || [e subtype] == NSTabletPointEventSubtype) {
                pressure = [e pressure];
                tilt_x   = [e tilt].x;
                tilt_y   = [e tilt].y; 
            }
            
            if([e subtype] == NSTabletProximityEventSubtype) {
                switch([e pointingDeviceType]) {
                    case NSEraserPointingDevice:
                        darwinTabletCurrent=darwinTabletEraser;
                        break;
                    case NSPenPointingDevice:
                        darwinTabletCurrent=darwinTabletStylus;
                        break;
                    case NSCursorPointingDevice:
                    case NSUnknownPointingDevice:
                    default:
                        darwinTabletCurrent=darwinTabletCursor;
                        break;
                }

                DarwinSendProximityEvents([e isEnteringProximity]?ProximityIn:ProximityOut,
                                          pointer_x, pointer_y);
            }

            DarwinSendPointerEvents(ev_type, ev_button, pointer_x, pointer_y,
                                    pressure, tilt_x, tilt_y);

            break;

		case NSTabletProximity:
            switch([e pointingDeviceType]) {
                case NSEraserPointingDevice:
                    darwinTabletCurrent=darwinTabletEraser;
                    break;
                case NSPenPointingDevice:
                    darwinTabletCurrent=darwinTabletStylus;
                    break;
                case NSCursorPointingDevice:
                case NSUnknownPointingDevice:
                default:
                    darwinTabletCurrent=darwinTabletCursor;
                    break;
            }
                    
			DarwinSendProximityEvents([e isEnteringProximity]?ProximityIn:ProximityOut,
                                      pointer_x, pointer_y);
            break;

		case NSScrollWheel:
			DarwinSendScrollEvents([e deltaX], [e deltaY], pointer_x, pointer_y,
                                   pressure, tilt_x, tilt_y);
            break;

		case NSKeyDown: case NSKeyUp:
            DarwinSendKeyboardEvents(([e type] == NSKeyDown) ? KeyPress : KeyRelease, [e keyCode]);
            break;

        default: break; /* for gcc */
	}	
}
