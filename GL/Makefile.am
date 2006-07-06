SUBDIRS = glx mesa

# someone could get really crazy someday and add support for the SI...

# xwin/darwin/xfree86 have their accel support under the DDX

APPLE_EXTRAS = \
	apple/aglGlx.c \
	apple/indirect.c
WINDOWS_EXTRAS = \
	windows/ChangeLog \
	windows/glwindows.h \
	windows/glwrap.c \
	windows/indirect.c

EXTRA_DIST = symlink-mesa.sh $(APPLE_EXTRAS) $(WINDOWS_EXTRAS)
