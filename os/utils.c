/* $Xorg: utils.c,v 1.5 2001/02/09 02:05:24 xorgcvs Exp $ */
/*

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
Copyright 1994 Quarterdeck Office Systems.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital and
Quarterdeck not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

DIGITAL AND QUARTERDECK DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT
OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
OR PERFORMANCE OF THIS SOFTWARE.

*/

#ifdef WIN32
#include <X11/Xwinsock.h>
#endif
#include "Xos.h"
#include <stdio.h>
#include "misc.h"
#include "X.h"
#include "input.h"
#include "opaque.h"
#ifdef X_POSIX_C_SOURCE
#define _POSIX_C_SOURCE X_POSIX_C_SOURCE
#include <signal.h>
#undef _POSIX_C_SOURCE
#else
#if defined(X_NOT_POSIX) || defined(_POSIX_SOURCE)
#include <signal.h>
#else
#define _POSIX_SOURCE
#include <signal.h>
#undef _POSIX_SOURCE
#endif
#endif
#ifndef WIN32
#ifndef SYSV
#include <sys/resource.h>
#endif
#endif
#include <time.h>
#include <sys/stat.h>
#include <ctype.h>    /* for isspace */
#if NeedVarargsPrototypes
#include <stdarg.h>
#endif
#if defined(TCPCONN) || defined(STREAMSCONN)
# ifndef WIN32
#  include <netdb.h>
# endif
#endif

#define X_INCLUDE_NETDB_H
#include <X11/Xos_r.h>

extern char *display;

extern CARD32 defaultScreenSaverTime;	/* for parsing command line */
extern CARD32 defaultScreenSaverInterval;
extern int defaultScreenSaverBlanking;
extern int defaultBackingStore;
extern Bool disableBackingStore;
extern Bool disableSaveUnders;
extern Bool PartialNetwork;
#ifndef NOLOGOHACK
extern int logoScreenSaver;
#endif
#ifdef RLIMIT_DATA
extern int limitDataSpace;
#endif
#ifdef RLIMIT_STACK
extern int limitStackSpace;
#endif
#ifdef RLIMIT_NOFILE
extern int limitNoFile;
#endif
extern int defaultColorVisualClass;
extern Bool permitOldBugs;
extern int monitorResolution;
extern Bool defeatAccessControl;

#ifdef DPMSExtension
extern BOOL DPMSEnabledSwitch;
extern BOOL DPMSDisabledSwitch;
#endif

Bool CoreDump;
Bool noTestExtensions;

Bool noPanoramiXExtension = TRUE;
#ifdef PANORAMIX
Bool PanoramiXVisibilityNotifySent = FALSE;
Bool PanoramiXMapped = FALSE;
Bool PanoramiXWindowExposureSent = FALSE;
Bool PanoramiXOneExposeRequest = FALSE;
#endif

int auditTrailLevel = 1;

void ddxUseMsg();
#if NeedVarargsPrototypes
void VErrorF(char*, va_list);
#endif

#ifdef AIXV3
int SyncOn  = 0;
extern int SelectWaitTime;
#endif

#ifdef DEBUG
#ifndef SPECIAL_MALLOC
#define MEMBUG
#endif
#endif

#ifdef MEMBUG
#define MEM_FAIL_SCALE 100000
long Memory_fail = 0;

#endif

#ifdef sgi
int userdefinedfontpath = 0;
#endif /* sgi */

Bool Must_have_memory = FALSE;

char *dev_tty_from_init = NULL;		/* since we need to parse it anyway */

OsSigHandlerPtr
OsSignal(sig, handler)
    int sig;
    OsSigHandlerPtr handler;
{
#ifdef X_NOT_POSIX
    return signal(sig, handler);
#else
    struct sigaction act, oact;

    sigemptyset(&act.sa_mask);
    if (handler != SIG_IGN)
	sigaddset(&act.sa_mask, sig);
    act.sa_flags = 0;
    act.sa_handler = handler;
    sigaction(sig, &act, &oact);
    return oact.sa_handler;
#endif
}

/* Force connections to close on SIGHUP from init */

/*ARGSUSED*/
SIGVAL
AutoResetServer (sig)
    int sig;
{
    dispatchException |= DE_RESET;
    isItTimeToYield = TRUE;
#ifdef GPROF
    chdir ("/tmp");
    exit (0);
#endif
#if defined(SYSV) && defined(X_NOT_POSIX)
    OsSignal (SIGHUP, AutoResetServer);
#endif
}

/* Force connections to close and then exit on SIGTERM, SIGINT */

/*ARGSUSED*/
SIGVAL
GiveUp(sig)
    int sig;
{
    dispatchException |= DE_TERMINATE;
    isItTimeToYield = TRUE;
#if defined(SYSV) && defined(X_NOT_POSIX)
    if (sig)
	OsSignal(sig, SIG_IGN);
#endif
}


static void
AbortServer()
{
    extern void AbortDDX();

    AbortDDX();
    fflush(stderr);
    if (CoreDump)
	abort();
    exit (1);
}

void
Error(str)
    char *str;
{
    perror(str);
}

#ifndef DDXTIME
CARD32
GetTimeInMillis()
{
    struct timeval  tp;

    X_GETTIMEOFDAY(&tp);
    return(tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}
#endif

AdjustWaitForDelay (waitTime, newdelay)
    pointer	    waitTime;
    unsigned long   newdelay;
{
    static struct timeval   delay_val;
    struct timeval	    **wt = (struct timeval **) waitTime;
    unsigned long	    olddelay;

    if (*wt == NULL)
    {
	delay_val.tv_sec = newdelay / 1000;
	delay_val.tv_usec = 1000 * (newdelay % 1000);
	*wt = &delay_val;
    }
    else
    {
	olddelay = (*wt)->tv_sec * 1000 + (*wt)->tv_usec / 1000;
	if (newdelay < olddelay)
	{
	    (*wt)->tv_sec = newdelay / 1000;
	    (*wt)->tv_usec = 1000 * (newdelay % 1000);
	}
    }
}

void UseMsg()
{
#if !defined(AIXrt) && !defined(AIX386)
    ErrorF("use: X [:<display>] [option]\n");
    ErrorF("-a #                   mouse acceleration (pixels)\n");
    ErrorF("-ac                    disable access control restrictions\n");
#ifdef MEMBUG
    ErrorF("-alloc int             chance alloc should fail\n");
#endif
    ErrorF("-audit int             set audit trail level\n");	
    ErrorF("-auth file             select authorization file\n");	
    ErrorF("bc                     enable bug compatibility\n");
    ErrorF("-bs                    disable any backing store support\n");
    ErrorF("-c                     turns off key-click\n");
    ErrorF("c #                    key-click volume (0-100)\n");
    ErrorF("-cc int                default color visual class\n");
    ErrorF("-co file               color database file\n");
#ifdef COMMANDLINE_CHALLENGED_OPERATING_SYSTEMS
    ErrorF("-config file           read options from file\n");
#endif
    ErrorF("-core                  generate core dump on fatal error\n");
    ErrorF("-dpi int               screen resolution in dots per inch\n");
#ifdef DPMSExtension
    ErrorF("dpms                   enables VESA DPMS monitor control\n");
    ErrorF("-dpms                  disables VESA DPMS monitor control\n");
#endif
    ErrorF("-deferglyphs [none|all|16] defer loading of [no|all|16-bit] glyphs\n");
    ErrorF("-f #                   bell base (0-100)\n");
    ErrorF("-fc string             cursor font\n");
    ErrorF("-fn string             default font name\n");
    ErrorF("-fp string             default font path\n");
    ErrorF("-help                  prints message with these options\n");
    ErrorF("-I                     ignore all remaining arguments\n");
#ifdef RLIMIT_DATA
    ErrorF("-ld int                limit data space to N Kb\n");
#endif
#ifdef RLIMIT_NOFILE
    ErrorF("-lf int                limit number of open files to N\n");
#endif
#ifdef RLIMIT_STACK
    ErrorF("-ls int                limit stack space to N Kb\n");
#endif
#ifndef NOLOGOHACK
    ErrorF("-logo                  enable logo in screen saver\n");
    ErrorF("nologo                 disable logo in screen saver\n");
#endif
    ErrorF("-p #                   screen-saver pattern duration (minutes)\n");
    ErrorF("-pn                    accept failure to listen on all ports\n");
    ErrorF("-r                     turns off auto-repeat\n");
    ErrorF("r                      turns on auto-repeat \n");
    ErrorF("-s #                   screen-saver timeout (minutes)\n");
#ifdef XCSECURITY
    ErrorF("-sp file               security policy file\n");
#endif
    ErrorF("-su                    disable any save under support\n");
    ErrorF("-t #                   mouse threshold (pixels)\n");
    ErrorF("-terminate             terminate at server reset\n");
    ErrorF("-to #                  connection time out\n");
    ErrorF("-tst                   disable testing extensions\n");
    ErrorF("ttyxx                  server started from init on /dev/ttyxx\n");
    ErrorF("v                      video blanking for screen-saver\n");
    ErrorF("-v                     screen-saver without video blanking\n");
    ErrorF("-wm                    WhenMapped default backing-store\n");
#ifdef PANORAMIX
    ErrorF("+xinerama              Enable XINERAMA extension\n");
    ErrorF("-xinerama              Disable XINERAMA extension\n");
#endif
    ErrorF("-x string              loads named extension at init time \n");
#ifdef XDMCP
    XdmcpUseMsg();
#endif
#endif /* !AIXrt && ! AIX386 */
#ifdef XKB
    XkbUseMsg();
#endif
    ddxUseMsg();
}

/*
 * This function parses the command line. Handles device-independent fields
 * and allows ddx to handle additional fields.  It is not allowed to modify
 * argc or any of the strings pointed to by argv.
 */
void
ProcessCommandLine ( argc, argv )
int	argc;
char	*argv[];

{
    int i, skip;

    defaultKeyboardControl.autoRepeat = TRUE;

    for ( i = 1; i < argc; i++ )
    {
	/* call ddx first, so it can peek/override if it wants */
        if(skip = ddxProcessArgument(argc, argv, i))
	{
	    i += (skip - 1);
	}
	else if(argv[i][0] ==  ':')  
	{
	    /* initialize display */
	    display = argv[i];
	    display++;
	}
	else if ( strcmp( argv[i], "-a") == 0)
	{
	    if(++i < argc)
	        defaultPointerControl.num = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-ac") == 0)
	{
	    defeatAccessControl = TRUE;
	}
#ifdef MEMBUG
	else if ( strcmp( argv[i], "-alloc") == 0)
	{
	    if(++i < argc)
	        Memory_fail = atoi(argv[i]);
	    else
		UseMsg();
	}
#endif
	else if ( strcmp( argv[i], "-audit") == 0)
	{
	    if(++i < argc)
	        auditTrailLevel = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-auth") == 0)
	{
	    if(++i < argc)
	        InitAuthorization (argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "bc") == 0)
	    permitOldBugs = TRUE;
	else if ( strcmp( argv[i], "-bs") == 0)
	    disableBackingStore = TRUE;
	else if ( strcmp( argv[i], "c") == 0)
	{
	    if(++i < argc)
	        defaultKeyboardControl.click = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-c") == 0)
	{
	    defaultKeyboardControl.click = 0;
	}
	else if ( strcmp( argv[i], "-cc") == 0)
	{
	    if(++i < argc)
	        defaultColorVisualClass = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-co") == 0)
	{
	    if(++i < argc)
	        rgbPath = argv[i];
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-core") == 0)
	    CoreDump = TRUE;
	else if ( strcmp( argv[i], "-dpi") == 0)
	{
	    if(++i < argc)
	        monitorResolution = atoi(argv[i]);
	    else
		UseMsg();
	}
#ifdef DPMSExtension
	else if ( strcmp( argv[i], "dpms") == 0)
	    DPMSEnabledSwitch = TRUE;
	else if ( strcmp( argv[i], "-dpms") == 0)
	    DPMSDisabledSwitch = TRUE;
#endif
	else if ( strcmp( argv[i], "-deferglyphs") == 0)
	{
	    if(++i >= argc || !ParseGlyphCachingMode(argv[i]))
		UseMsg();
	}
	else if ( strcmp( argv[i], "-f") == 0)
	{
	    if(++i < argc)
	        defaultKeyboardControl.bell = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-fc") == 0)
	{
	    if(++i < argc)
	        defaultCursorFont = argv[i];
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-fn") == 0)
	{
	    if(++i < argc)
	        defaultTextFont = argv[i];
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-fp") == 0)
	{
	    if(++i < argc)
	    {
#ifdef sgi
		userdefinedfontpath = 1;
#endif /* sgi */
	        defaultFontPath = argv[i];
	    }
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-help") == 0)
	{
	    UseMsg();
	    exit(0);
	}
#ifdef XKB
        else if ( (skip=XkbProcessArguments(argc,argv,i))!=0 ) {
	    if (skip>0)
		 i+= skip-1;
	    else UseMsg();
	}
#endif
#ifdef RLIMIT_DATA
	else if ( strcmp( argv[i], "-ld") == 0)
	{
	    if(++i < argc)
	    {
	        limitDataSpace = atoi(argv[i]);
		if (limitDataSpace > 0)
		    limitDataSpace *= 1024;
	    }
	    else
		UseMsg();
	}
#endif
#ifdef RLIMIT_NOFILE
	else if ( strcmp( argv[i], "-lf") == 0)
	{
	    if(++i < argc)
	        limitNoFile = atoi(argv[i]);
	    else
		UseMsg();
	}
#endif
#ifdef RLIMIT_STACK
	else if ( strcmp( argv[i], "-ls") == 0)
	{
	    if(++i < argc)
	    {
	        limitStackSpace = atoi(argv[i]);
		if (limitStackSpace > 0)
		    limitStackSpace *= 1024;
	    }
	    else
		UseMsg();
	}
#endif
#ifndef NOLOGOHACK
	else if ( strcmp( argv[i], "-logo") == 0)
	{
	    logoScreenSaver = 1;
	}
	else if ( strcmp( argv[i], "nologo") == 0)
	{
	    logoScreenSaver = 0;
	}
#endif
	else if ( strcmp( argv[i], "-p") == 0)
	{
	    if(++i < argc)
	        defaultScreenSaverInterval = ((CARD32)atoi(argv[i])) *
					     MILLI_PER_MIN;
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-pn") == 0)
	    PartialNetwork = TRUE;
	else if ( strcmp( argv[i], "r") == 0)
	    defaultKeyboardControl.autoRepeat = TRUE;
	else if ( strcmp( argv[i], "-r") == 0)
	    defaultKeyboardControl.autoRepeat = FALSE;
	else if ( strcmp( argv[i], "-s") == 0)
	{
	    if(++i < argc)
	        defaultScreenSaverTime = ((CARD32)atoi(argv[i])) *
					 MILLI_PER_MIN;
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-su") == 0)
	    disableSaveUnders = TRUE;
	else if ( strcmp( argv[i], "-t") == 0)
	{
	    if(++i < argc)
	        defaultPointerControl.threshold = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-terminate") == 0)
	{
	    extern Bool terminateAtReset;
	    
	    terminateAtReset = TRUE;
	}
	else if ( strcmp( argv[i], "-to") == 0)
	{
	    if(++i < argc)
		TimeOutValue = ((CARD32)atoi(argv[i])) * MILLI_PER_SECOND;
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-tst") == 0)
	{
	    noTestExtensions = TRUE;
	}
	else if ( strcmp( argv[i], "v") == 0)
	    defaultScreenSaverBlanking = PreferBlanking;
	else if ( strcmp( argv[i], "-v") == 0)
	    defaultScreenSaverBlanking = DontPreferBlanking;
	else if ( strcmp( argv[i], "-wm") == 0)
	    defaultBackingStore = WhenMapped;
#ifdef PANORAMIX
	else if ( strcmp( argv[i], "+xinerama") == 0){
	    noPanoramiXExtension = FALSE;
	}
	else if ( strcmp( argv[i], "-xinerama") == 0){
	    noPanoramiXExtension = TRUE;
	}
#endif
	else if ( strcmp( argv[i], "-x") == 0)
	{
	    if(++i >= argc)
		UseMsg();
	    /* For U**x, which doesn't support dynamic loading, there's nothing
	     * to do when we see a -x.  Either the extension is linked in or
	     * it isn't */
	}
	else if ( strcmp( argv[i], "-I") == 0)
	{
	    /* ignore all remaining arguments */
	    break;
	}
	else if (strncmp (argv[i], "tty", 3) == 0)
	{
	    /* just in case any body is interested */
	    dev_tty_from_init = argv[i];
	}
#ifdef XDMCP
	else if ((skip = XdmcpOptions(argc, argv, i)) != i)
	{
	    i = skip - 1;
	}
#endif
#ifdef XPRINT
	else if ((skip = XprintOptions(argc, argv, i)) != i)
	{
	    i = skip - 1;
	}
#endif
#ifdef XCSECURITY
	else if ((skip = XSecurityOptions(argc, argv, i)) != i)
	{
	    i = skip - 1;
	}
#endif
#ifdef AIXV3
        else if ( strcmp( argv[i], "-timeout") == 0)
        {
            if(++i < argc)
                SelectWaitTime = atoi(argv[i]);
            else
                UseMsg();
        }
        else if ( strcmp( argv[i], "-sync") == 0)
        {
            SyncOn++;
        }
#endif
 	else
 	{
	    UseMsg();
	    exit (1);
        }
    }
}

#ifdef COMMANDLINE_CHALLENGED_OPERATING_SYSTEMS
static void
InsertFileIntoCommandLine(resargc, resargv, prefix_argc, prefix_argv,
			  filename, suffix_argc, suffix_argv)
    int *resargc;
    char ***resargv;
    int prefix_argc;
    char **prefix_argv;
    char *filename;
    int suffix_argc;
    char **suffix_argv;
{
    struct stat     st;
    FILE           *f;
    char           *p;
    char           *q;
    int             insert_argc;
    char           *buf;
    int             len;
    int             i;

    f = fopen(filename, "r");
    if (!f)
	FatalError("Can't open option file %s\n", filename);

    fstat(fileno(f), &st);

    buf = (char *) xalloc((unsigned) st.st_size + 1);
    if (!buf)
	FatalError("Out of Memory\n");

    len = fread(buf, 1, (unsigned) st.st_size, f);

    fclose(f);

    if (len < 0)
	FatalError("Error reading option file %s\n", filename);

    buf[len] = '\0';

    p = buf;
    q = buf;
    insert_argc = 0;

    while (*p)
    {
	while (isspace(*p))
	    p++;
	if (!*p)
	    break;
	if (*p == '#')
	{
	    while (*p && *p != '\n')
		p++;
	} else
	{
	    while (*p && !isspace(*p))
		*q++ = *p++;
	    /* Since p and q might still be pointing at the same place, we	 */
	    /* need to step p over the whitespace now before we add the null.	 */
	    if (*p)
		p++;
	    *q++ = '\0';
	    insert_argc++;
	}
    }

    buf = (char *) xrealloc(buf, q - buf);
    if (!buf)
	FatalError("Out of memory reallocing option buf\n");

    *resargc = prefix_argc + insert_argc + suffix_argc;
    *resargv = (char **) xalloc((*resargc + 1) * sizeof(char *));
    if (!*resargv)
	FatalError("Out of Memory\n");

    memcpy(*resargv, prefix_argv, prefix_argc * sizeof(char *));

    p = buf;
    for (i = 0; i < insert_argc; i++)
    {
	(*resargv)[prefix_argc + i] = p;
	p += strlen(p) + 1;
    }

    memcpy(*resargv + prefix_argc + insert_argc,
	   suffix_argv, suffix_argc * sizeof(char *));

    (*resargv)[*resargc] = NULL;
} /* end InsertFileIntoCommandLine */


void
ExpandCommandLine(pargc, pargv)
    int *pargc;
    char ***pargv;
{
    int i;
    for (i = 1; i < *pargc; i++)
    {
	if ( (0 == strcmp((*pargv)[i], "-config")) && (i < (*pargc - 1)) )
	{
	    InsertFileIntoCommandLine(pargc, pargv,
					  i, *pargv,
					  (*pargv)[i+1], /* filename */
					  *pargc - i - 2, *pargv + i + 2);
	    i--;
	}
    }
} /* end ExpandCommandLine */
#endif

/* Implement a simple-minded font authorization scheme.  The authorization
   name is "hp-hostname-1", the contents are simply the host name. */
int
set_font_authorizations(authorizations, authlen, client)
char **authorizations;
int *authlen;
pointer client;
{
#define AUTHORIZATION_NAME "hp-hostname-1"
#if defined(TCPCONN) || defined(STREAMSCONN)
    static char result[1024];
    static char *p = NULL;

    if (p == NULL)
    {
	char hname[1024], *hnameptr;
	struct hostent *host;
	int len;
	_Xgethostbynameparams hparams;

	gethostname(hname, 1024);
	host = _XGethostbyname(hname, hparams);
	if (host == NULL)
	    hnameptr = hname;
	else
	    hnameptr = host->h_name;

	p = result;
        *p++ = sizeof(AUTHORIZATION_NAME) >> 8;
        *p++ = sizeof(AUTHORIZATION_NAME) & 0xff;
        *p++ = (len = strlen(hnameptr) + 1) >> 8;
        *p++ = (len & 0xff);

	memmove(p, AUTHORIZATION_NAME, sizeof(AUTHORIZATION_NAME));
	p += sizeof(AUTHORIZATION_NAME);
	memmove(p, hnameptr, len);
	p += len;
    }
    *authlen = p - result;
    *authorizations = result;
    return 1;
#else /* TCPCONN */
    return 0;
#endif /* TCPCONN */
}

/* XALLOC -- X's internal memory allocator.  Why does it return unsigned
 * long * instead of the more common char *?  Well, if you read K&R you'll
 * see they say that alloc must return a pointer "suitable for conversion"
 * to whatever type you really want.  In a full-blown generic allocator
 * there's no way to solve the alignment problems without potentially
 * wasting lots of space.  But we have a more limited problem. We know
 * we're only ever returning pointers to structures which will have to
 * be long word aligned.  So we are making a stronger guarantee.  It might
 * have made sense to make Xalloc return char * to conform with people's
 * expectations of malloc, but this makes lint happier.
 */

unsigned long * 
Xalloc (amount)
    unsigned long amount;
{
    char		*malloc();
    register pointer  ptr;
	
    if ((long)amount <= 0)
	return (unsigned long *)NULL;
    /* aligned extra on long word boundary */
    amount = (amount + (sizeof(long) - 1)) & ~(sizeof(long) - 1);
#ifdef MEMBUG
    if (!Must_have_memory && Memory_fail &&
	((random() % MEM_FAIL_SCALE) < Memory_fail))
	return (unsigned long *)NULL;
#endif
    if (ptr = (pointer)malloc(amount))
	return (unsigned long *)ptr;
    if (Must_have_memory)
	FatalError("Out of memory");
    return (unsigned long *)NULL;
}

/*****************
 * XNFalloc 
 * "no failure" realloc, alternate interface to Xalloc w/o Must_have_memory
 *****************/

unsigned long *
XNFalloc (amount)
    unsigned long amount;
{
    char             *malloc();
    register pointer ptr;

    if ((long)amount <= 0)
    {
        return (unsigned long *)NULL;
    }
    /* aligned extra on long word boundary */
    amount = (amount + (sizeof(long) - 1)) & ~(sizeof(long) - 1);
    ptr = (pointer)malloc(amount);
    if (!ptr)
    {
        FatalError("Out of memory");
    }
    return ((unsigned long *)ptr);
}

/*****************
 * Xcalloc
 *****************/

unsigned long *
Xcalloc (amount)
    unsigned long   amount;
{
    unsigned long   *ret;

    ret = Xalloc (amount);
    if (ret)
	bzero ((char *) ret, (int) amount);
    return ret;
}

/*****************
 * Xrealloc
 *****************/

unsigned long *
Xrealloc (ptr, amount)
    register pointer ptr;
    unsigned long amount;
{
    char *malloc();
    char *realloc();

#ifdef MEMBUG
    if (!Must_have_memory && Memory_fail &&
	((random() % MEM_FAIL_SCALE) < Memory_fail))
	return (unsigned long *)NULL;
#endif
    if ((long)amount <= 0)
    {
	if (ptr && !amount)
	    free(ptr);
	return (unsigned long *)NULL;
    }
    amount = (amount + (sizeof(long) - 1)) & ~(sizeof(long) - 1);
    if (ptr)
        ptr = (pointer)realloc((char *)ptr, amount);
    else
	ptr = (pointer)malloc(amount);
    if (ptr)
        return (unsigned long *)ptr;
    if (Must_have_memory)
	FatalError("Out of memory");
    return (unsigned long *)NULL;
}
                    
/*****************
 * XNFrealloc 
 * "no failure" realloc, alternate interface to Xrealloc w/o Must_have_memory
 *****************/

unsigned long *
XNFrealloc (ptr, amount)
    register pointer ptr;
    unsigned long amount;
{
    if (( ptr = (pointer)Xrealloc( ptr, amount ) ) == NULL)
    {
        FatalError( "Out of memory" );
    }
    return ((unsigned long *)ptr);
}

/*****************
 *  Xfree
 *    calls free 
 *****************/    

void
Xfree(ptr)
    register pointer ptr;
{
    if (ptr)
	free((char *)ptr); 
}

OsInitAllocator ()
{
#ifdef MEMBUG
    static int	been_here;

    /* Check the memory system after each generation */
    if (been_here)
	CheckMemory ();
    else
	been_here = 1;
#endif
}

void
AuditPrefix(f)
    char *f;
{
#ifdef X_NOT_STDC_ENV
    long tm;
#else
    time_t tm;
#endif
    char *autime, *s;
    if (*f != ' ')
    {
	time(&tm);
	autime = ctime(&tm);
	if (s = strchr(autime, '\n'))
	    *s = '\0';
	if (s = strrchr(argvGlobal[0], '/'))
	    s++;
	else
	    s = argvGlobal[0];
	ErrorF("AUDIT: %s: %d %s: ", autime, getpid(), s);
    }
}

/*VARARGS1*/
void
AuditF(
#if NeedVarargsPrototypes
    char * f, ...)
#else
    f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9) /* limit of ten args */
    char *f;
    char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9;
#endif
{
#if NeedVarargsPrototypes
    va_list args;
#endif

    AuditPrefix(f);

#if NeedVarargsPrototypes
    va_start(args, f);
    VErrorF(f, args);
    va_end(args);
#else
    ErrorF(f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9);
#endif
}

/*VARARGS1*/
void
FatalError(
#if NeedVarargsPrototypes
    char *f, ...)
#else
f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9) /* limit of ten args */
    char *f;
    char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9;
#endif
{
#if NeedVarargsPrototypes
    va_list args;
#endif
    ErrorF("\nFatal server error:\n");
#if NeedVarargsPrototypes
    va_start(args, f);
    VErrorF(f, args);
    va_end(args);
#else
    ErrorF(f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9);
#endif
    ErrorF("\n");
    AbortServer();
    /*NOTREACHED*/
}

#if NeedVarargsPrototypes
void
VErrorF(f, args)
    char *f;
    va_list args;
{
#ifdef AIXV3
    if (SyncOn)
        sync();
#else
    vfprintf(stderr, f, args);
#endif /* AIXV3 */
}
#endif

/*VARARGS1*/
void
ErrorF(
#if NeedVarargsPrototypes
    char * f, ...)
#else
 f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9) /* limit of ten args */
    char *f;
    char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9;
#endif
{
#if NeedVarargsPrototypes
    va_list args;
    va_start(args, f);
    VErrorF(f, args);
    va_end(args);
#else
#ifdef AIXV3
    if (SyncOn)
        sync();
#else /* not AIXV3 */
    fprintf( stderr, f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9);
#endif /* AIXV3 */
#endif
}
