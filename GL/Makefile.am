if XQUARTZ
XQUARTZ_SUBDIRS = apple
endif

SUBDIRS = glx $(XQUARTZ_SUBDIRS)
DIST_SUBDIRS = glx apple

WINDOWS_EXTRAS = \
	windows/ChangeLog \
	windows/glwindows.h \
	windows/glwrap.c \
	windows/indirect.c

EXTRA_DIST = symlink-mesa.sh $(WINDOWS_EXTRAS)
