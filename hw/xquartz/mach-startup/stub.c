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

static void send_fd_handoff(int connected_fd, int launchd_fd) {
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
    
    *((int*)CMSG_DATA(cmsg)) = launchd_fd;
    
    if (sendmsg(connected_fd, &msg, 0) < 0) {
        fprintf(stderr, "Error sending $DISPLAY file descriptor: %s\n", strerror(errno));
        return;
    }

    fprintf(stderr, "send %d %d %d %s\n", connected_fd, launchd_fd, errno, strerror(errno));
}

static void handoff_fd(const char *filename, int launchd_fd) {
    struct sockaddr_un servaddr_un;
    struct sockaddr *servaddr;
    socklen_t servaddr_len;
    int handoff_fd, connected_fd;

    /* Setup servaddr_un */
    memset (&servaddr_un, 0, sizeof (struct sockaddr_un));
    servaddr_un.sun_family = AF_UNIX;
    strlcpy(servaddr_un.sun_path, filename, sizeof(servaddr_un.sun_path));

    servaddr = (struct sockaddr *) &servaddr_un;
    servaddr_len = sizeof(struct sockaddr_un) - sizeof(servaddr_un.sun_path) + strlen(filename);

    handoff_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if(handoff_fd == 0) {
        fprintf(stderr, "Failed to create socket: %s - %s\n", filename, strerror(errno));
        return;
    }

    connected_fd = connect(handoff_fd, servaddr, servaddr_len);

    if(connected_fd == -1) {
        fprintf(stderr, "Failed to establish connection on socket: %s - %s\n", filename, strerror(errno));
        return;
    }

    send_fd_handoff(connected_fd, launchd_fd);

    close(handoff_fd);
}

int main(int argc, char **argv, char **envp) {
#ifdef NEW_LAUNCH_METHOD
    int envpc;
    kern_return_t kr;
    mach_port_t mp;
    string_array_t newenvp;
    string_array_t newargv;
    size_t i;
    int launchd_fd;
    string_t handoff_socket;
#endif

    if(argc == 2 && !strcmp(argv[1], "-version")) {
        fprintf(stderr, "X.org Release 7.3\n");
        fprintf(stderr, "X.Org X Server %s\n", XSERVER_VERSION);
        fprintf(stderr, "Build Date: %s\n", BUILD_DATE);
        return EXIT_SUCCESS;
    }

#ifdef NEW_LAUNCH_METHOD
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
        tmpnam(handoff_socket);
        if(prep_fd_handoff(mp, handoff_socket) == KERN_SUCCESS) {
            handoff_fd(handoff_socket, launchd_fd);
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
