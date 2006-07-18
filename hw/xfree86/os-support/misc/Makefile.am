# FIXME: Add the *.S files to build when applicable
I386_SOURCES = BUSmemcpy.S IODelay.S SlowBcopy.S
OTHER_SOURCES = BUSmemcpy.c IODelay.c SlowBcopy.c

ARCH_SOURCES = $(OTHER_SOURCES)

# FIXME: Add to the build (NeedPortIO)
PORTIO_SOURCES = PortIO.S

# FIXME: Add to the build (if HasGcc || HasGcc2)
ILHACK_SOURCES = xf86_IlHack.c

noinst_LTLIBRARIES = libmisc.la

libmisc_la_SOURCES = xf86_Util.c Delay.c $(ARCH_SOURCES)

#AM_LDFLAGS = -r

INCLUDES = $(XORG_INCS)

AM_CFLAGS = $(XORG_CFLAGS)

EXTRA_DIST = $(I386_SOURCES) $(PORTIO_SOURCES) $(ILHACK_SOURCES)
