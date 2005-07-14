module_LTLIBRARIES = libi2c.la

libi2c_la_LDFLAGS = -avoid-version
libi2c_la_SOURCES = xf86i2c.c xf86i2cmodule.c

INCLUDES = $(XORG_INCS)

AM_CFLAGS = $(XORG_CFLAGS)

sdk_HEADERS = xf86i2c.h bt829.h fi1236.h msp3430.h tda8425.h tda9850.h tda9885.h uda1380.h i2c_def.h



#
# i2c drivers
#
bt829_drv_la_LTLIBRARIES = bt829_drv.la
bt829_drv_la_LDFLAGS = -module -avoid-version
bt829_drv_ladir = @moduledir@/multimedia
bt829_drv_la_SOURCES = bt829.c bt829.h bt829_module.c

fi1236_drv_la_LTLIBRARIES = fi1236_drv.la
fi1236_drv_la_LDFLAGS = -module -avoid-version
fi1236_drv_ladir = @moduledir@/multimedia
fi1236_drv_la_SOURCES = fi1236.c fi1236.h fi1236_module.c

msp3430_drv_la_LTLIBRARIES = msp3430_drv.la
msp3430_drv_la_LDFLAGS = -module -avoid-version
msp3430_drv_ladir = @moduledir@/multimedia
msp3430_drv_la_SOURCES = msp3430.c msp3430.h msp3430_module.c

tda8425_drv_la_LTLIBRARIES = tda8425_drv.la
tda8425_drv_la_LDFLAGS = -module -avoid-version
tda8425_drv_ladir = @moduledir@/multimedia
tda8425_drv_la_SOURCES = tda8425.c tda8425.h tda8425_module.c

tda9850_drv_la_LTLIBRARIES = tda9850_drv.la
tda9850_drv_la_LDFLAGS = -module -avoid-version
tda9850_drv_ladir = @moduledir@/multimedia
tda9850_drv_la_SOURCES = tda9850.c tda9850.h tda9850_module.c

tda9885_drv_la_LTLIBRARIES = tda9885_drv.la
tda9885_drv_la_LDFLAGS = -module -avoid-version
tda9885_drv_ladir = @moduledir@/multimedia
tda9885_drv_la_SOURCES = tda9885.c tda9885.h tda9885_module.c

uda1380_drv_la_LTLIBRARIES = uda1380_drv.la
uda1380_drv_la_LDFLAGS = -module -avoid-version
uda1380_drv_ladir = @moduledir@/multimedia
uda1380_drv_la_SOURCES = uda1380.c uda1380.h uda1380_module.c
