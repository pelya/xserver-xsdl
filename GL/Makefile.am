# someone could get really crazy someday and add support for the SI...

# xwin/darwin/xfree86 have their accel support under the DDX

if BUILD_DARWIN
DARWIN_SUBDIRS = apple
endif
SUBDIRS = glx mesa $(DARWIN_SUBDIRS)

WINDOWS_EXTRAS = \
	windows/ChangeLog \
	windows/glwindows.h \
	windows/glwrap.c \
	windows/indirect.c

EXTRA_DIST = symlink-mesa.sh $(WINDOWS_EXTRAS)
