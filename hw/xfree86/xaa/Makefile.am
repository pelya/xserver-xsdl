LSB_FIRST = l-xaaBitmap.c l3-xaaBitmap.c l-xaaStipple.c l3-xaaStipple.c \
            l-xaaTEGlyph.c
MSB_FIRST = m-xaaBitmap.c m3-xaaBitmap.c m-xaaStipple.c m3-xaaStipple.c \
            m-xaaTEGlyph.c
LSB_FIXED = lf-xaaBitmap.c lf3-xaaBitmap.c lf-xaaStipple.c lf3-xaaStipple.c \
            lf-xaaTEGlyph.c
MSB_FIXED = mf-xaaBitmap.c mf3-xaaBitmap.c mf-xaaStipple.c mf3-xaaStipple.c \
            mf-xaaTEGlyph.c
POLYSEG = s-xaaLine.c s-xaaDashLine.c

module_LTLIBRARIES = libxaa.la
libxaa_la_SOURCES = xaaInit.c xaaGC.c xaaInitAccel.c xaaFallback.c \
                   xaaBitBlt.c xaaCpyArea.c xaaGCmisc.c xaaCpyWin.c \
                   xaaCpyPlane.c xaaFillRect.c xaaTEText.c xaaNonTEText.c \
                   xaaPCache.c xaaSpans.c xaaROP.c xaaImage.c xaaWrapper.c \
                   xaaPaintWin.c xaaRect.c xaaLineMisc.c xaaBitOrder.c \
                   xaaFillPoly.c xaaWideLine.c xaaTables.c xaaFillArc.c \
                   xaaLine.c xaaDashLine.c xaaOverlay.c xaaOffscreen.c \
                   xaaOverlayDF.c xaaStateChange.c xaaPict.c $(POLYSEG) \
                   $(LSB_FIRST) $(MSB_FIRST) $(LSB_FIXED) $(MSB_FIXED)
s-%.c:
	echo "#define POLYSEGMENT" > $@
	echo '#include "$(srcdir)/$*.c"' >> $@
l-%.c:
	echo "#define LSBFIRST" > $@
	echo '#include "$(srcdir)/$*.c"' >> $@
l3-%.c:
	echo "#define LSBFIRST" > $@
	echo "#define TRIPLE_BITS" >> $@
	echo '#include "$(srcdir)/$*.c"' >> $@
lf-%.c:
	echo "#define LSBFIRST" > $@
	echo "#define FIXEDBASE" >> $@
	echo '#include "$(srcdir)/$*.c"' >> $@
lf3-%.c:
	echo "#define LSBFIRST" > $@
	echo "#define TRIPLE_BITS" >> $@
	echo "#define FIXEDBASE" >> $@
	echo '#include "$(srcdir)/$*.c"' >> $@
m-%.c:
	echo "#define MSBFIRST" > $@
	echo '#include "$(srcdir)/$*.c"' >> $@
m3-%.c:
	echo "#define MSBFIRST" > $@
	echo "#define TRIPLE_BITS" >> $@
	echo '#include "$(srcdir)/$*.c"' >> $@
mf-%.c:
	echo "#define MSBFIRST" > $@
	echo "#define FIXEDBASE" >> $@
	echo '#include "$(srcdir)/$*.c"' >> $@
mf3-%.c:
	echo "#define MSBFIRST" > $@
	echo "#define TRIPLE_BITS" >> $@
	echo "#define FIXEDBASE" >> $@
	echo '#include "$(srcdir)/$*.c"' >> $@

DISTCLEANFILES = $(POLYSEG) $(LSB_FIRST) $(LSB_FIXED) $(MSB_FIRST) $(MSB_FIXED)

sdk_HEADERS = xaa.h xaalocal.h xaarop.h
EXTRA_DIST = xaaWrapper.h xaacexp.h xaawrap.h xaaLine.c xaaDashLine.c \
             xaaStipple.c xaaTEGlyph.c xaaBitmap.c

INCLUDES = $(XORG_INCS)
