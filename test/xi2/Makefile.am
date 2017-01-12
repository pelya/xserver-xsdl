if ENABLE_UNIT_TESTS
if HAVE_LD_WRAP
noinst_PROGRAMS = tests

TESTS = tests

TESTS_ENVIRONMENT = $(XORG_MALLOC_DEBUG_ENV)

tests_CFLAGS = $(DIX_CFLAGS) @XORG_CFLAGS@

tests_CPPFLAGS = \
	@XORG_INCS@ \
	-I$(srcdir)/..

tests_LDADD = ../libxservertest.la $(XORG_SYS_LIBS) $(XSERVER_SYS_LIBS) $(GLX_SYS_LIBS)

if SPECIAL_DTRACE_OBJECTS
tests_LDADD += $(OS_LIB) $(DIX_LIB)
endif

tests_LDFLAGS = \
        -Wl,-wrap,WriteToClient \
        -Wl,-wrap,dixLookupWindow \
        -Wl,-wrap,XISetEventMask \
        -Wl,-wrap,AddResource \
        -Wl,-wrap,GrabButton \
        -Wl,-wrap,dixLookupClient \
        $()

tests_SOURCES = \
	$(srcdir)/../tests-common.c \
	protocol-common.c \
	protocol-xiqueryversion.c \
	protocol-xiquerydevice.c \
	protocol-xiselectevents.c \
	protocol-xigetselectedevents.c \
	protocol-xisetclientpointer.c \
	protocol-xigetclientpointer.c \
	protocol-xiquerypointer.c \
	protocol-xipassivegrabdevice.c \
	protocol-xiwarppointer.c \
	protocol-eventconvert.c \
	xi2.c \
	tests.c

else
# Print that xi2-tests were skipped (exit code 77 for automake test harness)
TESTS = xi2-tests
CLEANFILES = $(TESTS)

xi2-tests:
	@echo 'echo "ld -wrap support required for xi2 unit tests, skipping"' > $@
	@echo 'exit 77' >> $@
	$(AM_V_GEN)chmod +x $@
endif
endif
