if UNITTESTS
check_PROGRAMS =  \
	protocol-xiqueryversion \
	protocol-xiquerydevice

TESTS=$(check_PROGRAMS)

AM_CFLAGS = $(DIX_CFLAGS) $(GLIB_CFLAGS) @XORG_CFLAGS@
INCLUDES = @XORG_INCS@
TEST_LDADD=../libxservertest.la $(XORG_SYS_LIBS) $(XSERVER_SYS_LIBS) $(GLIB_LIBS)
COMMON_SOURCES=protocol-common.h protocol-common.c

protocol_xiqueryversion_LDADD=$(TEST_LDADD)
protocol_xiquerydevice_LDADD=$(TEST_LDADD)

protocol_xiqueryversion_CFLAGS=$(AM_CFLAGS) -Wl,-wrap,WriteToClient
protocol_xiquerydevice_CFLAGS=$(AM_CFLAGS) -Wl,-wrap,WriteToClient

protocol_xiqueryversion_SOURCES=$(COMMON_SOURCES) protocol-xiqueryversion.c
protocol_xiquerydevice_SOURCES=$(COMMON_SOURCES) protocol-xiquerydevice.c
endif
