noinst_LIBRARIES = libdmxinput.a

RAWSRCS = lnx-keyboard.c lnx-ms.c lnx-ps2.c
USBSRCS = usb-keyboard.c usb-mouse.c usb-other.c usb-common.c
DRVSRCS = dmxdummy.c dmxbackend.c dmxconsole.c dmxcommon.c
DMXSRCS = dmxinputinit.c dmxarg.c dmxsigio.c dmxevents.c \
	  dmxxinput.c dmxmotion.c dmxeq.c dmxmap.c


libdmxinput_a_SOURCES = ChkNotMaskEv.c $(RAWSRCS) $(USBSRCS) $(DRVSRCS) $(DMXSRCS) $(LIBSRCS)
AM_CFLAGS = -I..
