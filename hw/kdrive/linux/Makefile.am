INCLUDES = 					\
	@KDRIVE_INCS@				\
	@XSERVER_CFLAGS@


noinst_LIBRARIES = liblinux.a

if TSLIB
TSLIB_C = tslib.c
endif

if H3600_TS
TS_C = ts.c
endif

liblinux_a_SOURCES = 	\
	agp.c		\
	bus.c		\
	keyboard.c	\
	linux.c		\
	mouse.c		\
	ms.c		\
	ps2.c		\
	$(TSLIB_C)	\
	$(TS_C)

liblinux_a_DEPENDENCIES = \
	agp.c		\
	bus.c		\
	keyboard.c	\
	linux.c		\
	mouse.c		\
	ms.c		\
	ps2.c		\
	$(TSLIB_C)	\
	$(TS_C)

