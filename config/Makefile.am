AM_CFLAGS = @DIX_CFLAGS@

noinst_LIBRARIES = libconfig.a
libconfig_a_SOURCES = config.c

if HAVE_DBUS
AM_CFLAGS += @DBUS_CFLAGS@
libconfig_a_SOURCES += dbus-core.c
endif

if CONFIG_DBUS_API
dbusconfigdir = $(sysconfdir)/dbus-1/system.d
dbusconfig_DATA = xorg-server.conf

noinst_LIBRARIES = libconfig.a
libconfig_a_SOURCES += dbus.c
endif

EXTRA_DIST = xorg-server.conf
