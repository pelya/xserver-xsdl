/*
 * pcitweak.c
 *
 * Copyright 1999 by The XFree86 Project, Inc.
 *
 * Author: David Dawes <dawes@xfree86.org>
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/etc/pcitweak.c,v 1.16 2002/12/14 04:41:13 dawes Exp $ */

#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSproc.h"
#include "xf86Pci.h"

#ifdef __CYGWIN__
#include <getopt.h>
#endif

#include <stdarg.h>
#include <stdlib.h>
#ifdef __linux__
/* to get getopt on Linux */
#ifndef __USE_POSIX2
#define __USE_POSIX2
#endif
#endif
#include <unistd.h>
#if defined(ISC) || defined(Lynx)
extern char *optarg;
extern int optind, opterr;
#endif

pciVideoPtr *xf86PciVideoInfo = NULL;

static void usage(void);
static Bool parsePciBusString(const char *id, int *bus, int *device, int *func);
static char *myname = NULL;

int
main(int argc, char *argv[])
{
    int c;
    PCITAG tag;
    int bus, device, func;
    Bool list = FALSE, rd = FALSE, wr = FALSE;
    Bool byte = FALSE, halfword = FALSE;
    int offset = 0;
    CARD32 value = 0;
    char *id = NULL, *end;

    myname = argv[0];
    while ((c = getopt(argc, argv, "bhlr:w:")) != -1) {
	switch (c) {
	case 'b':
	    byte = TRUE;
	    break;
	case 'h':
	    halfword = TRUE;
	    break;
	case 'l':
	    list = TRUE;
	    break;
	case 'r':
	    rd = TRUE;
	    id = optarg;
	    break;
	case 'w':
	    wr = TRUE;
	    id = optarg;
	    break;
	case '?':
	default:
	    usage();
	}
    }
    argc -= optind;
    argv += optind;

    if (list) {
	xf86Verbose = 2;
	xf86EnableIO();
	xf86scanpci(0);
	xf86DisableIO();
	exit(0);
    }

    if (rd && wr)
	usage();
    if (wr && argc != 2)
	usage();
    if (rd && argc != 1)
	usage();
    if (byte && halfword)
	usage();

    if (rd || wr) {
	if (!parsePciBusString(id, &bus, &device, &func)) {
	    fprintf(stderr, "%s: Bad PCI ID string\n", myname);
	    usage();
	}
	offset = strtoul(argv[0], &end, 0);
	if (*end != '\0') {
	    fprintf(stderr, "%s: Bad offset\n", myname);
	    usage();
	}
	if (halfword) {
	    if (offset % 2) {
		fprintf(stderr, "%s: offset must be a multiple of two\n",
			myname);
		exit(1);
	    }
	} else if (!byte) {
	    if (offset % 4) {
		fprintf(stderr, "%s: offset must be a multiple of four\n",
			myname);
		exit(1);
	    }
	}
    } else {
	usage();
    }
    
    if (wr) {
	value = strtoul(argv[1], &end, 0);
	if (*end != '\0') {
	    fprintf(stderr, "%s: Bad value\n", myname);
	    usage();
	}
    }

    xf86EnableIO();

    /*
     * This is needed to setup all the buses.  Otherwise secondary buses
     * can't be accessed.
     */
    xf86scanpci(0);

    tag = pciTag(bus, device, func);
    if (rd) {
	if (byte) {
	    printf("0x%02x\n", (unsigned int)pciReadByte(tag, offset) & 0xFF);
	} else if (halfword) {
	    printf("0x%04x\n", (unsigned int)pciReadWord(tag, offset) & 0xFFFF);
	} else {
	    printf("0x%08lx\n", (unsigned long)pciReadLong(tag, offset));
	}
    } else if (wr) {
	if (byte) {
	    pciWriteByte(tag, offset, value & 0xFF);
	} else if (halfword) {
	    pciWriteWord(tag, offset, value & 0xFFFF);
	} else {
	    pciWriteLong(tag, offset, value);
	}
    }

    xf86DisableIO();
    exit(0);
}

static void
usage()
{
    fprintf(stderr, "usage:\tpcitweak -l\n"
		    "\tpcitweak -r ID [-b | -h] offset\n"
		    "\tpcitweak -w ID [-b | -h] offset value\n"
		    "\n"
		    "\t\t-l  -- list\n"
		    "\t\t-r  -- read\n"
		    "\t\t-w  -- write\n"
		    "\t\t-b  -- read/write a single byte\n"
		    "\t\t-h  -- read/write a single halfword (16 bit)\n"
		    "\t\tID  -- PCI ID string in form bus:dev:func "
					"(all in hex)\n");
	
    exit(1);
}

Bool
parsePciBusString(const char *busID, int *bus, int *device, int *func)
{
    /*
     * The format is assumed to be "bus:device:func", where bus, device
     * and func are hexadecimal integers.  func may be omitted and assumed to
     * be zero, although it doing this isn't encouraged.
     */

    char *p, *s, *end;

    s = strdup(busID);
    p = strtok(s, ":");
    if (p == NULL || *p == 0)
	return FALSE;
    *bus = strtoul(p, &end, 16);
    if (*end != '\0')
	return FALSE;
    p = strtok(NULL, ":");
    if (p == NULL || *p == 0)
	return FALSE;
    *device = strtoul(p, &end, 16);
    if (*end != '\0')
	return FALSE;
    *func = 0;
    p = strtok(NULL, ":");
    if (p == NULL || *p == 0)
	return TRUE;
    *func = strtoul(p, &end, 16);
    if (*end != '\0')
	return FALSE;
    return TRUE;
}

#include "xf86getpagesize.c"

