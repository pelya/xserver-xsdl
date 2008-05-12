/* main.c -- X application launcher
 
 Copyright (c) 2007 Jeremy Huddleston
 Copyright (c) 2007 Apple Inc
 
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

#include <X11/Xlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include <CoreFoundation/CoreFoundation.h>

#include <mach/mach.h>
#include <mach/mach_error.h>
#include <servers/bootstrap.h>
#include "mach_startup.h"
#include "mach_startupServer.h"

#define DEFAULT_CLIENT "/usr/X11/bin/xterm"
#define DEFAULT_STARTX "/usr/X11/bin/startx"
#define DEFAULT_SHELL  "/bin/sh"

static int execute(const char *command);
static char *command_from_prefs(const char *key, const char *default_value);

/* This is in quartzStartup.c */
int server_main(int argc, char **argv, char **envp);

struct arg {
    int argc;
    char **argv;
    char **envp;
};

/*** Mach-O IPC Stuffs ***/

union MaxMsgSize {
	union __RequestUnion__mach_startup_subsystem req;
	union __ReplyUnion__mach_startup_subsystem rep; 
};

kern_return_t do_start_x11_server(mach_port_t port, string_array_t argv,
                                  mach_msg_type_number_t argvCnt,
                                  string_array_t envp,
                                  mach_msg_type_number_t envpCnt) {
    if(server_main(argvCnt, argv, envp) == 0)
        return KERN_SUCCESS;
    else
        return KERN_FAILURE;
}

kern_return_t do_exit(mach_port_t port, int value) {
    exit(value);
}

static mach_port_t checkin_or_register(char *bname) {
    kern_return_t kr;
    mach_port_t mp;

    /* If we're started by launchd or the old mach_init */
    kr = bootstrap_check_in(bootstrap_port, bname, &mp);
    if (kr == KERN_SUCCESS)
        return mp;

    /* We probably were not started by launchd or the old mach_init */
    kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &mp);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "mach_port_allocate(): %s\n", mach_error_string(kr));
        exit(EXIT_FAILURE);
    }

    kr = mach_port_insert_right(mach_task_self(), mp, mp, MACH_MSG_TYPE_MAKE_SEND);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "mach_port_insert_right(): %s\n", mach_error_string(kr));
        exit(EXIT_FAILURE);
    }

    kr = bootstrap_register(bootstrap_port, bname, mp);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "bootstrap_register(): %s\n", mach_error_string(kr));
        exit(EXIT_FAILURE);
    }

    return mp;
}

/*** Pthread Magics ***/
static pthread_t create_thread(void *func, void *arg) {
    pthread_attr_t attr;
    pthread_t tid;
	
    pthread_attr_init (&attr);
    pthread_attr_setscope (&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
    pthread_create (&tid, &attr, func, arg);
    pthread_attr_destroy (&attr);
	
    return tid;
}

/*** Main ***/
static int execute(const char *command);
static char *command_from_prefs(const char *key, const char *default_value);

#ifdef NEW_LAUNCH_METHOD
static void startup_trigger_thread(void *arg) {
    struct arg args = *((struct arg *)arg);
    free(arg);
    startup_trigger(args.argc, args.argv, args.envp);
}

int main(int argc, char **argv, char **envp) {
    BOOL listenOnly = FALSE;
    int i;
    mach_msg_size_t mxmsgsz = sizeof(union MaxMsgSize) + MAX_TRAILER_SIZE;
    mach_port_t mp;
    kern_return_t kr;

    for(i=1; i < argc; i++) {
        if(!strcmp(argv[i], "--listenonly")) {
            listenOnly = TRUE;
            break;
        }
    }

    /* TODO: This should be unconditional once we figure out fd passing */
    if((argc > 1 && argv[1][0] == ':') || listenOnly) {
        mp = checkin_or_register(SERVER_BOOTSTRAP_NAME);
    }

    /* Check if we need to do something other than listen, and make another
     * thread handle it.
     */
    if(!listenOnly) {
        struct arg *args = (struct arg*)malloc(sizeof(struct arg));
        if(!args)
            FatalError("Could not allocate memory.\n");

        args->argc = argc;
        args->argv = argv;
        args->envp = envp;

        create_thread(startup_trigger_thread, args);
    }

    /* TODO: This should actually fall through rather than be the else
     *       case once we figure out how to get the stub to pass the
     *       file descriptor.  For now, we only listen if we are explicitly
     *       told to.
     */
    if((argc > 1 && argv[1][0] == ':') || listenOnly) {
        /* Main event loop */
        kr = mach_msg_server(mach_startup_server, mxmsgsz, mp, 0);
        if (kr != KERN_SUCCESS) {
            asl_log(NULL, NULL, ASL_LEVEL_ERR,
                    "org.x.X11(mp): %s\n", mach_error_string(kr));
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}

int startup_trigger(int argc, char **argv, char **envp) {
#else
int main(int argc, char **argv, char **envp) {
#endif
    Display *display;
    const char *s;
    
    size_t i;
    fprintf(stderr, "X11.app: main(): argc=%d\n", argc);
    for(i=0; i < argc; i++) {
        fprintf(stderr, "\targv[%u] = %s\n", (unsigned)i, argv[i]);
    }
    
    /* Take care of the case where we're called like a normal DDX */
    if(argc > 1 && argv[1][0] == ':') {
#ifdef NEW_LAUNCH_METHOD
        /* We need to count envp */
        int envpc;
        for(envpc=0; envp[envpc]; envpc++);

        return start_x11_server(argc, argv, envp, envpc);
#else
        return server_main(argc, argv, envp);
#endif
    }

    /* If we have a process serial number and it's our only arg, act as if
     * the user double clicked the app bundle: launch app_to_run if possible
     */
    if(argc == 1 || (argc == 2 && !strncmp(argv[1], "-psn_", 5))) {
        /* Now, try to open a display, if so, run the launcher */
        display = XOpenDisplay(NULL);
        if(display) {
            fprintf(stderr, "X11.app: Closing the display and sleeping for 2s to allow the X server to start up.\n");
            /* Could open the display, start the launcher */
            XCloseDisplay(display);
            
            /* Give 2 seconds for the server to start... 
             * TODO: *Really* fix this race condition
             */
            usleep(2000);
            return execute(command_from_prefs("app_to_run", DEFAULT_CLIENT));
        }
    }

    /* Start the server */
    if((s = getenv("DISPLAY"))) {
        fprintf(stderr, "X11.app: Could not connect to server (DISPLAY=\"%s\", unsetting).  Starting X server.\n", s);
        unsetenv("DISPLAY");
    } else {
        fprintf(stderr, "X11.app: Could not connect to server (DISPLAY is not set).  Starting X server.\n");
    }
    return execute(command_from_prefs("startx_script", DEFAULT_STARTX));
}

static int execute(const char *command) {
    const char *newargv[7];
    const char **s;

    newargv[0] = "/usr/bin/login";
    newargv[1] = "-fp";
    newargv[2] = getlogin();
    newargv[3] = command_from_prefs("login_shell", DEFAULT_SHELL);
    newargv[4] = "-c";
    newargv[5] = command;
    newargv[6] = NULL;
    
    fprintf(stderr, "X11.app: Launching %s:\n", command);
    for(s=newargv; *s; s++) {
        fprintf(stderr, "\targv[%d] = %s\n", s - newargv, *s);
    }

    execvp (newargv[0], (char * const *) newargv);
    perror ("X11.app: Couldn't exec.");
    return(1);
}

static char *command_from_prefs(const char *key, const char *default_value) {
    char *command = NULL;
    
    CFStringRef cfKey = CFStringCreateWithCString(NULL, key, kCFStringEncodingASCII);
    CFPropertyListRef PlistRef = CFPreferencesCopyAppValue(cfKey, kCFPreferencesCurrentApplication);
    
    if ((PlistRef == NULL) || (CFGetTypeID(PlistRef) != CFStringGetTypeID())) {
        CFStringRef cfDefaultValue = CFStringCreateWithCString(NULL, default_value, kCFStringEncodingASCII);

        CFPreferencesSetAppValue(cfKey, cfDefaultValue, kCFPreferencesCurrentApplication);
        CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
        
        int len = strlen(default_value) + 1;
        command = (char *)malloc(len * sizeof(char));
        if(!command)
            return NULL;
        strcpy(command, default_value);
    } else {
        int len = CFStringGetLength((CFStringRef)PlistRef) + 1;
        command = (char *)malloc(len * sizeof(char));
        if(!command)
            return NULL;
        CFStringGetCString((CFStringRef)PlistRef, command, len,  kCFStringEncodingASCII);
	}
    
    if (PlistRef)
        CFRelease(PlistRef);
    
    return command;
}
