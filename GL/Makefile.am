if XQUARTZ
XQUARTZ_SUBDIRS = apple
endif

SUBDIRS = glx mesa $(XQUARTZ_SUBDIRS)
DIST_SUBDIRS = glx mesa apple

WINDOWS_EXTRAS = \
	windows/ChangeLog \
	windows/glwindows.h \
	windows/glwrap.c \
	windows/indirect.c

EXTRA_DIST = symlink-mesa.sh $(WINDOWS_EXTRAS)
