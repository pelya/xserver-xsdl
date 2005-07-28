noinst_LIBRARIES = libdmxinput.a

RAWSRCS = lnx-keyboard.c \
          lnx-keyboard.h \
          lnx-ms.c \
          lnx-ms.h \
          lnx-ps2.c \
          lnx-ps2.h

USBSRCS = usb-keyboard.c \
          usb-keyboard.h \
          usb-mouse.c \
          usb-mouse.h \
          usb-other.c \
          usb-other.h \
          usb-common.c \
          usb-common.h \
          usb-private.h

DRVSRCS = dmxdummy.c \
          dmxdummy.h \
          dmxbackend.c \
          dmxbackend.h \
          dmxconsole.c \
          dmxconsole.h \
          dmxcommon.c \
          dmxcommon.h

DMXSRCS = dmxinputinit.c \
          dmxinputinit.h \
          dmxarg.c \
          dmxarg.h \
          dmxsigio.c \
          dmxsigio.h \
          dmxevents.c \
          dmxevents.h \
	  dmxxinput.c \
          dmxmotion.c \
          dmxmotion.h \
          dmxeq.c \
          dmxeq.h \
          dmxmap.c \
          dmxmap.h


libdmxinput_a_SOURCES = ChkNotMaskEv.c \
                        ChkNotMaskEv.h \
                        $(RAWSRCS) \
                        $(USBSRCS) \
                        $(DRVSRCS) \
                        $(DMXSRCS) \
                        $(LIBSRCS)

AM_CFLAGS = -I$(top_srcdir)/hw/dmx \
            -I$(top_srcdir)/hw/xfree86/common \
            -DHAVE_DMX_CONFIG_H \
            @DMXMODULES_CFLAGS@
