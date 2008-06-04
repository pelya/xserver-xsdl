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

#include <CoreFoundation/CoreFoundation.h>

#include <X11/Xlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <mach/mach.h>
#include <mach/mach_error.h>
#include <servers/bootstrap.h>
#include "mach_startup.h"
#include "mach_startupServer.h"

#include "launchd_fd.h"
/* From darwinEvents.c ... but don't want to pull in all the server cruft */
void DarwinListenOnOpenFD(int fd);

#define DEFAULT_CLIENT "/usr/X11/bin/xterm"
#define DEFAULT_STARTX "/usr/X11/bin/startx"
#define DEFAULT_SHELL  "/bin/sh"

static int execute(const char *command);
static char *command_from_prefs(const char *key, const char *default_value);

/* This is in quartzStartup.c */
int server_main(int argc, char **argv, char **envp);

static int execute(const char *command);
static char *command_from_prefs(const char *key, const char *default_value);

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

#ifdef NEW_LAUNCH_METHOD
/*** Mach-O IPC Stuffs ***/

union MaxMsgSize {
	union __RequestUnion__mach_startup_subsystem req;
	union __ReplyUnion__mach_startup_subsystem rep; 
};

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

/*** $DISPLAY handoff ***/
static void accept_fd_handoff(int connected_fd) {
    int launchd_fd;
    
    char databuf[] = "display";
    struct iovec iov[1];
    
    iov[0].iov_base = databuf;
    iov[0].iov_len  = sizeof(databuf);
    
    union {
        struct cmsghdr hdr;
        char bytes[CMSG_SPACE(sizeof(int))];
    } buf;
    
    struct msghdr msg;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = buf.bytes;
    msg.msg_controllen = sizeof(buf);
    msg.msg_name = 0;
    msg.msg_namelen = 0;
    msg.msg_flags = 0;
    
    struct cmsghdr *cmsg = CMSG_FIRSTHDR (&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    
    msg.msg_controllen = cmsg->cmsg_len;
    
    *((int*)CMSG_DATA(cmsg)) = -1;
    
    if(recvmsg(connected_fd, &msg, 0) < 0) {
        fprintf(stderr, "Error receiving $DISPLAY file descriptor: %s\n", strerror(errno));
        return;
    }
    
    launchd_fd = *((int*)CMSG_DATA(cmsg));
    
    if(launchd_fd == -1)
        fprintf(stderr, "Error receiving $DISPLAY file descriptor, no descriptor received? %d\n", launchd_fd);
        
    fprintf(stderr, "Received new DISPLAY fd: %d\n", launchd_fd);
    DarwinListenOnOpenFD(launchd_fd);
}

typedef struct {
    string_t socket_filename;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    kern_return_t retval;
} handoff_data_t;

/* This thread loops accepting incoming connections and handing off the file
 * descriptor for the new connection to accept_fd_handoff()
 */
static void socket_handoff_thread(void *arg) {
    handoff_data_t *data = (handoff_data_t *)arg;
    string_t filename;
    struct sockaddr_un servaddr_un;
    struct sockaddr *servaddr;
    socklen_t servaddr_len;
    int handoff_fd;

    /* We need to save it since data dies after we pthread_cond_broadcast */
    strlcpy(filename, data->socket_filename, STRING_T_SIZE); 
    
    /* Make sure we only run once the dispatch thread is waiting for us */
    pthread_mutex_lock(&data->lock);
    
    /* Setup servaddr_un */
    memset (&servaddr_un, 0, sizeof (struct sockaddr_un));
    servaddr_un.sun_family  = AF_UNIX;
    strlcpy(servaddr_un.sun_path, filename, sizeof(servaddr_un.sun_path));

    servaddr = (struct sockaddr *) &servaddr_un;
    servaddr_len = sizeof(struct sockaddr_un) - sizeof(servaddr_un.sun_path) + strlen(filename);
    
    handoff_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(handoff_fd == 0) {
        fprintf(stderr, "Failed to create socket: %s - %s\n", filename, strerror(errno));

        data->retval = EXIT_FAILURE;
        pthread_cond_broadcast(&data->cond);
        pthread_mutex_unlock(&data->lock);
        return;
    }

    /* Let the dispatch thread now tell the caller that we're ready */
    data->retval = EXIT_SUCCESS;
    pthread_cond_broadcast(&data->cond);
    pthread_mutex_unlock(&data->lock);
    
    if(connect(handoff_fd, servaddr, servaddr_len) < 0) {
        fprintf(stderr, "Failed to connect to socket: %s - %s\n", filename, strerror(errno));
        return;
    }

    /* Now actually get the passed file descriptor from this connection */
    accept_fd_handoff(handoff_fd);

    close(handoff_fd);
}

kern_return_t do_prep_fd_handoff(mach_port_t port, string_t socket_filename) {
    handoff_data_t handoff_data;

    /* Initialize our data */
    pthread_mutex_init(&handoff_data.lock, NULL);
    pthread_cond_init(&handoff_data.cond, NULL);
    strlcpy(handoff_data.socket_filename, socket_filename, STRING_T_SIZE); 

    pthread_mutex_lock(&handoff_data.lock);
    
    create_thread(socket_handoff_thread, &handoff_data);

    /* Wait for our return value */
    pthread_cond_wait(&handoff_data.cond, &handoff_data.lock);
    pthread_mutex_unlock(&handoff_data.lock);

    /* Cleanup */
    pthread_cond_destroy(&handoff_data.cond);
    pthread_mutex_destroy(&handoff_data.lock);
    
    return handoff_data.retval;
}

/*** Server Startup ***/
kern_return_t do_start_x11_server(mach_port_t port, string_array_t argv,
                                  mach_msg_type_number_t argvCnt,
                                  string_array_t envp,
                                  mach_msg_type_number_t envpCnt) {
    /* And now back to char ** */
    char **_argv = alloca((argvCnt + 1) * sizeof(char *));
    char **_envp = alloca((envpCnt + 1) * sizeof(char *));
    size_t i;
    
    if(!_argv || !_envp) {
        return KERN_FAILURE;
    }

    fprintf(stderr, "X11.app: do_start_x11_server(): argc=%d\n", argvCnt);
    for(i=0; i < argvCnt; i++) {
        _argv[i] = argv[i];
        fprintf(stderr, "\targv[%u] = %s\n", (unsigned)i, argv[i]);
    }
    _argv[argvCnt] = NULL;
    
    for(i=0; i < envpCnt; i++) {
        _envp[i] = envp[i];
    }
    _envp[envpCnt] = NULL;
    
    if(server_main(argvCnt, _argv, _envp) == 0)
        return KERN_SUCCESS;
    else
        return KERN_FAILURE;
}

int startup_trigger(int argc, char **argv, char **envp) {
#else
void *add_launchd_display_thread(void *data);
    
int main(int argc, char **argv, char **envp) {
#endif
    Display *display;
    const char *s;
    
    size_t i;
#ifndef NEW_LAUNCH_METHOD
    fprintf(stderr, "X11.app: main(): argc=%d\n", argc);
    for(i=0; i < argc; i++) {
        fprintf(stderr, "\targv[%u] = %s\n", (unsigned)i, argv[i]);
    }
#endif
    
    /* Take care of the case where we're called like a normal DDX */
    if(argc > 1 && argv[1][0] == ':') {
#ifdef NEW_LAUNCH_METHOD
        kern_return_t kr;
        mach_port_t mp;
        string_array_t newenvp;
        string_array_t newargv;

        /* We need to count envp */
        int envpc;
        for(envpc=0; envp[envpc]; envpc++);

        /* We have fixed-size string lengths due to limitations in IPC,
         * so we need to copy our argv and envp.
         */
        newargv = (string_array_t)alloca(argc * sizeof(string_t));
        newenvp = (string_array_t)alloca(envpc * sizeof(string_t));
        
        if(!newargv || !newenvp) {
            fprintf(stderr, "Memory allocation failure\n");
            exit(EXIT_FAILURE);
        }
        
        for(i=0; i < argc; i++) {
            strlcpy(newargv[i], argv[i], STRING_T_SIZE);
        }
        for(i=0; i < envpc; i++) {
            strlcpy(newenvp[i], envp[i], STRING_T_SIZE);
        }

        kr = bootstrap_look_up(bootstrap_port, SERVER_BOOTSTRAP_NAME, &mp);
        if (kr != KERN_SUCCESS) {
            fprintf(stderr, "bootstrap_look_up(): %s\n", bootstrap_strerror(kr));
            exit(EXIT_FAILURE);
        }

        kr = start_x11_server(mp, newargv, argc, newenvp, envpc);
        if (kr != KERN_SUCCESS) {
            fprintf(stderr, "start_x11_server: %s\n", mach_error_string(kr));
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
#else
        create_thread(add_launchd_display_thread, NULL);
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
            /* Could open the display, start the launcher */
            XCloseDisplay(display);
            
            /* TODO: Clean up this race better... givint xinitrc time to run. */
            sleep(2);
            
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

#ifdef NEW_LAUNCH_METHOD
/*** Main ***/
int main(int argc, char **argv, char **envp) {
    Bool listenOnly = FALSE;
    int i;
    mach_msg_size_t mxmsgsz = sizeof(union MaxMsgSize) + MAX_TRAILER_SIZE;
    mach_port_t mp;
    kern_return_t kr;
    
    fprintf(stderr, "X11.app: main(): argc=%d\n", argc);
    for(i=1; i < argc; i++) {
        fprintf(stderr, "\targv[%u] = %s\n", (unsigned)i, argv[i]);
        if(!strcmp(argv[i], "--listenonly")) {
            listenOnly = TRUE;
        }
    }
    
    mp = checkin_or_register(SERVER_BOOTSTRAP_NAME);
    if(mp == MACH_PORT_NULL) {
        fprintf(stderr, "NULL mach service: %s", SERVER_BOOTSTRAP_NAME);
        return EXIT_FAILURE;
    }
    
    /* Check if we need to do something other than listen, and make another
     * thread handle it.
     */
    if(!listenOnly) {
        if(fork() == 0) {
            return startup_trigger(argc, argv, envp);
        }
    }
    
    /* Main event loop */
    fprintf(stderr, "Waiting for startup parameters via Mach IPC.\n");
    kr = mach_msg_server(mach_startup_server, mxmsgsz, mp, 0);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "org.x.X11(mp): %s\n", mach_error_string(kr));
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
#else
void *add_launchd_display_thread(void *data) {
    /* Start listening on the launchd fd */
    int launchd_fd = launchd_display_fd();
    if(launchd_fd != -1) {
        DarwinListenOnOpenFD(launchd_fd);
    }
    return NULL;
}
#endif
    
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
