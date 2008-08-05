/* Copyright (c) 2008 Apple Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 * HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above
 * copyright holders shall not be used in advertising or otherwise to
 * promote the sale, use or other dealings in this Software without
 * prior written authorization.
 */

#include <CoreServices/CoreServices.h>

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>

#define kX11AppBundleId "org.x.X11"
#define kX11AppBundlePath "/Contents/MacOS/X11"

#include <mach/mach.h>
#include <mach/mach_error.h>
#include <servers/bootstrap.h>
#include "mach_startup.h"

#include "launchd_fd.h"

#ifndef BUILD_DATE
#define BUILD_DATE "?"
#endif
#ifndef XSERVER_VERSION
#define XSERVER_VERSION "?"
#endif

#define DEBUG 1

static char x11_path[PATH_MAX + 1];

static void set_x11_path() {
    CFURLRef appURL = NULL;
    CFBundleRef bundle = NULL;
    OSStatus osstatus = LSFindApplicationForInfo(kLSUnknownCreator, CFSTR(kX11AppBundleId), nil, nil, &appURL);
    UInt32 ver;

    switch (osstatus) {
        case noErr:
            if (appURL == NULL) {
                fprintf(stderr, "Xquartz: Invalid response from LSFindApplicationForInfo(%s)\n", 
                        kX11AppBundleId);
                exit(1);
            }

            bundle = CFBundleCreate(NULL, appURL);
            if(!bundle) {
                fprintf(stderr, "Xquartz: Null value returned from CFBundleCreate().\n");
                exit(2);                
            }

            if (!CFURLGetFileSystemRepresentation(appURL, true, (unsigned char *)x11_path, sizeof(x11_path))) {
                fprintf(stderr, "Xquartz: Error resolving URL for %s\n", kX11AppBundleId);
                exit(3);
            }

            ver = CFBundleGetVersionNumber(bundle);
            if(ver < 0x02308000) {
                CFStringRef versionStr = CFBundleGetValueForInfoDictionaryKey(bundle, kCFBundleVersionKey);
                const char * versionCStr = "Unknown";

                if(versionStr) 
                    versionCStr = CFStringGetCStringPtr(versionStr, kCFStringEncodingMacRoman);

                fprintf(stderr, "Xquartz: Could not find a new enough X11.app LSFindApplicationForInfo() returned\n");
                fprintf(stderr, "         X11.app = %s\n", x11_path);
                fprintf(stderr, "         Version = %s (%x), Expected Version > 2.3.0\n", versionCStr, (unsigned)ver);
                exit(9);
            }

            strlcat(x11_path, kX11AppBundlePath, sizeof(x11_path));
#ifdef DEBUG
            fprintf(stderr, "Xquartz: X11.app = %s\n", x11_path);
#endif
            break;
        case kLSApplicationNotFoundErr:
            fprintf(stderr, "Xquartz: Unable to find application for %s\n", kX11AppBundleId);
            exit(10);
        default:
            fprintf(stderr, "Xquartz: Unable to find application for %s, error code = %d\n", 
                    kX11AppBundleId, (int)osstatus);
            exit(11);
    }
}

#ifdef MACHO_STARTUP
static int create_socket(char *filename_out) {
    struct sockaddr_un servaddr_un;
    struct sockaddr *servaddr;
    socklen_t servaddr_len;
    int ret_fd;
    size_t try, try_max;

    for(try=0, try_max=5; try < try_max; try++) {
        tmpnam(filename_out);

        /* Setup servaddr_un */
        memset (&servaddr_un, 0, sizeof (struct sockaddr_un));
        servaddr_un.sun_family = AF_UNIX;
        strlcpy(servaddr_un.sun_path, filename_out, sizeof(servaddr_un.sun_path));
        
        servaddr = (struct sockaddr *) &servaddr_un;
        servaddr_len = sizeof(struct sockaddr_un) - sizeof(servaddr_un.sun_path) + strlen(filename_out);
        
        ret_fd = socket(PF_UNIX, SOCK_STREAM, 0);
        if(ret_fd == -1) {
            fprintf(stderr, "Xquartz: Failed to create socket (try %d / %d): %s - %s\n", (int)try+1, (int)try_max, filename_out, strerror(errno));
            continue;
        }
        
        if(bind(ret_fd, servaddr, servaddr_len) != 0) {
            fprintf(stderr, "Xquartz: Failed to bind socket: %d - %s\n", errno, strerror(errno));
            close(ret_fd);
            return 0;
        }

        if(listen(ret_fd, 10) != 0) {
            fprintf(stderr, "Xquartz: Failed to listen to socket: %s - %d - %s\n", filename_out, errno, strerror(errno));
            close(ret_fd);
            return 0;
        }

#ifdef DEBUG
        fprintf(stderr, "Xquartz: Listening on socket for fd handoff:  %s\n", filename_out);
#endif

        return ret_fd;
    }
    
    return 0;
}

static void send_fd_handoff(int handoff_fd, int launchd_fd) {
    char databuf[] = "display";
    struct iovec iov[1];
    int connected_fd;
    
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
    
    *((int*)CMSG_DATA(cmsg)) = launchd_fd;
    
#ifdef DEBUG
    fprintf(stderr, "Xquartz: Waiting for fd handoff connection.\n");
#endif
    connected_fd = accept(handoff_fd, NULL, NULL);
    if(connected_fd == -1) {
        fprintf(stderr, "Xquartz: Failed to accept incoming connection on socket: %s\n", strerror(errno));
        return;
    }
    
#ifdef DEBUG
    fprintf(stderr, "Xquartz: Handoff connection established.  Sending message.\n");
#endif
    if(sendmsg(connected_fd, &msg, 0) < 0) {
        fprintf(stderr, "Xquartz: Error sending $DISPLAY file descriptor: %s\n", strerror(errno));
        return;
    }

#ifdef DEBUG
    fprintf(stderr, "Xquartz: Message sent.  Closing.\n");
#endif
    close(connected_fd);
#ifdef DEBUG
    fprintf(stderr, "Xquartz: end of send debug: %d %d %d %s\n", handoff_fd, launchd_fd, errno, strerror(errno));
#endif
}

#endif

int main(int argc, char **argv, char **envp) {
#ifdef MACHO_STARTUP
    int envpc;
    kern_return_t kr;
    mach_port_t mp;
    string_array_t newenvp;
    string_array_t newargv;
    size_t i;
    int launchd_fd;
    string_t handoff_socket_filename;
#endif
    sig_t handler;

    if(argc == 2 && !strcmp(argv[1], "-version")) {
        fprintf(stderr, "X.org Release 7.3\n");
        fprintf(stderr, "X.Org X Server %s\n", XSERVER_VERSION);
        fprintf(stderr, "Build Date: %s\n", BUILD_DATE);
        return EXIT_SUCCESS;
    }

    /* We don't have a mechanism in place to handle this interrupt driven
     * server-start notification, so just send the signal now, so xinit doesn't
     * time out waiting for it and will just poll for the server.
     */
    handler = signal(SIGUSR1, SIG_IGN);
    if(handler == SIG_IGN)
        kill(getppid(), SIGUSR1);
    signal(SIGUSR1, handler);

#ifdef MACHO_STARTUP
    /* Get the $DISPLAY FD */
    launchd_fd = launchd_display_fd();

    kr = bootstrap_look_up(bootstrap_port, SERVER_BOOTSTRAP_NAME, &mp);
    if(kr != KERN_SUCCESS) {
        set_x11_path();

        /* This forking is ugly and will be cleaned up later */
        pid_t child = fork();
        if(child == -1) {
            fprintf(stderr, "Could not fork: %s\n", strerror(errno));
            return EXIT_FAILURE;
        }

        if(child == 0) {
            char *_argv[3];
            _argv[0] = x11_path;
            _argv[1] = "--listenonly";
            _argv[2] = NULL;
            fprintf(stderr, "Starting X server: %s --listenonly\n", x11_path);
            return execvp(x11_path, _argv);
        }

        /* Try connecting for 10 seconds */
        for(i=0; i < 80; i++) {
            usleep(250000);
            kr = bootstrap_look_up(bootstrap_port, SERVER_BOOTSTRAP_NAME, &mp);
            if(kr == KERN_SUCCESS)
                break;
        }

        if(kr != KERN_SUCCESS) {
            fprintf(stderr, "bootstrap_look_up(): Timed out: %s\n", bootstrap_strerror(kr));
            return EXIT_FAILURE;
        }
    }
    
    /* Handoff the $DISPLAY FD */
    if(launchd_fd != -1) {
        int handoff_fd = create_socket(handoff_socket_filename);
        
        if((handoff_fd != 0) &&
           (prep_fd_handoff(mp, handoff_socket_filename) == KERN_SUCCESS)) {
            send_fd_handoff(handoff_fd, launchd_fd);
            
            // Cleanup
            close(handoff_fd);
            unlink(handoff_socket_filename);
        } else {
            fprintf(stderr, "Unable to hand of $DISPLAY file descriptor\n");
        }
    }

    /* Count envp */
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

    kr = start_x11_server(mp, newargv, argc, newenvp, envpc);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "start_x11_server: %s\n", mach_error_string(kr));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
#else
    set_x11_path();
    argv[0] = x11_path;
    return execvp(x11_path, argv);
#endif
}
