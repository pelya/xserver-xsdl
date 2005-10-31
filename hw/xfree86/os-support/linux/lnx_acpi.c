#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#define XF86_OS_PRIVS
#include "xf86_OSproc.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
 
#define ACPI_SOCKET  "/var/run/acpid.socket"
#define ACPI_EVENTS  "/proc/acpi/event"

#define ACPI_VIDEO_NOTIFY_SWITCH	0x80
#define ACPI_VIDEO_NOTIFY_PROBE		0x81
#define ACPI_VIDEO_NOTIFY_CYCLE		0x82
#define ACPI_VIDEO_NOTIFY_NEXT_OUTPUT	0x83
#define ACPI_VIDEO_NOTIFY_PREV_OUTPUT	0x84

#define ACPI_VIDEO_NOTIFY_CYCLE_BRIGHTNESS	0x82
#define	ACPI_VIDEO_NOTIFY_INC_BRIGHTNESS	0x83
#define ACPI_VIDEO_NOTIFY_DEC_BRIGHTNESS	0x84
#define ACPI_VIDEO_NOTIFY_ZERO_BRIGHTNESS	0x85
#define ACPI_VIDEO_NOTIFY_DISPLAY_OFF		0x86

#define ACPI_VIDEO_HEAD_INVALID		(~0u - 1)
#define ACPI_VIDEO_HEAD_END		(~0u)

static void lnxCloseACPI(void);
static pointer ACPIihPtr = NULL;
PMClose lnxACPIOpen(void);

#define LINE_LENGTH 80

static int
lnxACPIGetEventFromOs(int fd, pmEvent *events, int num)
{
    char ev[LINE_LENGTH];
    int n;

    memset(ev, 0, LINE_LENGTH);

    n = read( fd, ev, LINE_LENGTH );

    /* Check that we have a video event */
    if (strstr(ev, "video") == ev) {
	char *video = NULL;
	char *GFX = NULL;
	char *notify = NULL;
	char *data = NULL; /* doesn't appear to be used in the kernel */
	unsigned long int notify_l, data_l;

	video = strtok(ev, "video");

	GFX = strtok(NULL, " ");
#if 0
	ErrorF("GFX: %s\n",GFX);
#endif

	notify = strtok(NULL, " ");
	notify_l = strtoul(notify, NULL, 16);
#if 0
	ErrorF("notify: 0x%lx\n",notify_l);
#endif

	data = strtok(NULL, " ");
	data_l = strtoul(data, NULL, 16);
#if 0
	ErrorF("data: 0x%lx\n",data_l);
#endif

	/* We currently don't differentiate between any event */
	switch (notify_l) {
		case ACPI_VIDEO_NOTIFY_SWITCH:
			break;
		case ACPI_VIDEO_NOTIFY_PROBE:
			break;
		case ACPI_VIDEO_NOTIFY_CYCLE:
			break;
		case ACPI_VIDEO_NOTIFY_NEXT_OUTPUT:
			break;
		case ACPI_VIDEO_NOTIFY_PREV_OUTPUT:
			break;
		default:
			break;
	}

	/* Deal with all ACPI events as a capability change */
        events[0] = XF86_APM_CAPABILITY_CHANGED;

	return 1;
    }
    
    return 0;
}

static pmWait
lnxACPIConfirmEventToOs(int fd, pmEvent event)
{
    /* No ability to send back to the kernel in ACPI */
    switch (event) {
    default:
	return PM_NONE;
    }
}

PMClose
lnxACPIOpen(void)
{
    int fd;    
    struct sockaddr_un addr;
    int r = -1;

#ifdef DEBUG
    ErrorF("ACPI: OSPMOpen called\n");
#endif
    if (ACPIihPtr || !xf86Info.pmFlag)
	return NULL;
   
#ifdef DEBUG
    ErrorF("ACPI: Opening device\n");
#endif
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) > -1) {
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, ACPI_SOCKET);
	if ((r = connect(fd, (struct sockaddr*)&addr, sizeof(addr))) == -1) {
	    shutdown(fd, 2);
	    close(fd);
	    fd = -1;
	}
    }

    /* acpid's socket isn't available, so try going direct */
    if (fd == -1) {
        if ((fd = open(ACPI_EVENTS, O_RDONLY)) < 0) {
	    xf86MsgVerb(X_WARNING,3,"Open ACPI failed (%s) (%s)\n", ACPI_EVENTS,
	    	strerror(errno));
	    return NULL;
    	}
    }

    xf86PMGetEventFromOs = lnxACPIGetEventFromOs;
    xf86PMConfirmEventToOs = lnxACPIConfirmEventToOs;
    ACPIihPtr = xf86AddInputHandler(fd,xf86HandlePMEvents,NULL);
    xf86MsgVerb(X_INFO,3,"Open ACPI successful (%s)\n", (r != -1) ? ACPI_SOCKET : ACPI_EVENTS);

    return lnxCloseACPI;
}

static void
lnxCloseACPI(void)
{
    int fd;
    
#ifdef DEBUG
   ErrorF("ACPI: Closing device\n");
#endif
    if (ACPIihPtr) {
	fd = xf86RemoveInputHandler(ACPIihPtr);
	shutdown(fd, 2);
	close(fd);
	ACPIihPtr = NULL;
    }
}

