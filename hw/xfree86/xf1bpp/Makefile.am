module_LTLIBRARIES = libxf1bpp.la

sdk_HEADERS = xf1bpp.h mfbmap.h mfbunmap.h

libxf1bpp_la_LDFLAGS = -avoid-version

libxf1bppgen_a_SOURCES = \
        mfbseg.c \
        mfbpgbwht.c \
        mfbpgbblak.c \
        mfbpgbinv.c \
        mfbigbwht.c \
        mfbigbblak.c \
        mfbpawhite.c \
        mfbpablack.c \
        mfbpainv.c \
        mfbtewhite.c \
        mfbteblack.c \
        mfbplywhite.c \
        mfbplyblack.c \
        mfbplyinv.c \
        mfbbltC.c \
        mfbbltX.c \
        mfbbltCI.c \
        mfbbltO.c \
        mfbbltG.c \
        mfbtileC.c \
        mfbtileG.c

DISTCLEANFILES = $(libxf1bppgen_a_SOURCES)

libxf1bppmfb_a_SOURCES = \
        $(top_srcdir)/mfb/maskbits.c \
        $(top_srcdir)/mfb/mfbbitblt.c \
        $(top_srcdir)/mfb/mfbbres.c \
        $(top_srcdir)/mfb/mfbbresd.c \
        $(top_srcdir)/mfb/mfbbstore.c \
        $(top_srcdir)/mfb/mfbclip.c \
        $(top_srcdir)/mfb/mfbcmap.c \
        $(top_srcdir)/mfb/mfbfillarc.c \
        $(top_srcdir)/mfb/mfbfillrct.c \
        $(top_srcdir)/mfb/mfbfillsp.c \
        $(top_srcdir)/mfb/mfbfont.c \
        $(top_srcdir)/mfb/mfbgc.c \
        $(top_srcdir)/mfb/mfbgetsp.c \
        $(top_srcdir)/mfb/mfbhrzvert.c \
        $(top_srcdir)/mfb/mfbimage.c \
        $(top_srcdir)/mfb/mfbline.c \
        $(top_srcdir)/mfb/mfbmisc.c \
        $(top_srcdir)/mfb/mfbpixmap.c \
        $(top_srcdir)/mfb/mfbpntwin.c \
        $(top_srcdir)/mfb/mfbpolypnt.c \
        $(top_srcdir)/mfb/mfbpushpxl.c \
        $(top_srcdir)/mfb/mfbscrclse.c \
        $(top_srcdir)/mfb/mfbscrinit.c \
        $(top_srcdir)/mfb/mfbsetsp.c \
        $(top_srcdir)/mfb/mfbwindow.c \
        $(top_srcdir)/mfb/mfbzerarc.c \
        mfbmodule.c

libxf1bpp_la_SOURCES = $(libxf1bppmfb_a_SOURCES) $(libxf1bppgen_a_SOURCES)

AM_CFLAGS = -DXF86MONO -include mfbmap.h $(DIX_CFLAGS) $(XORG_CFLAGS)
INCLUDES = $(XORG_INCS) -I$(top_srcdir)/mfb

mfbseg.c:
	echo "#define POLYSEGMENT" > $@
	echo "#include \"$(top_srcdir)/mfb/mfbline.c\"" >> $@
mfbpgbwht.c:
	echo "#define MFBPOLYGLYPHBLT xf1bppPolyGlyphBltWhite" > $@
	echo "#define OPEQ |=" >> $@
	echo "#include \"$(top_srcdir)/mfb/mfbplygblt.c\"" >> $@
mfbpgbblak.c:
	echo "#define MFBPOLYGLYPHBLT xf1bppPolyGlyphBltBlack" > $@
	echo "#define OPEQ &=~" >> $@
	echo "#include \"$(top_srcdir)/mfb/mfbplygblt.c\"" >> $@
mfbpgbinv.c:
	echo "#define MFBPOLYGLYPHBLT xf1bppPolyGlyphBltInvert" > $@
	echo "#define OPEQ ^=" >> $@
	echo "#include \"$(top_srcdir)/mfb/mfbplygblt.c\"" >> $@
mfbigbwht.c:
	echo "#define MFBIMAGEGLYPHBLT xf1bppImageGlyphBltWhite" > $@
	echo "#define OPEQ |=" >> $@
	echo "#include \"$(top_srcdir)/mfb/mfbimggblt.c\"" >> $@
mfbigbblak.c:
	echo "#define MFBIMAGEGLYPHBLT xf1bppImageGlyphBltBlack" > $@
	echo "#define OPEQ &=~" >> $@
	echo "#include \"$(top_srcdir)/mfb/mfbimggblt.c\"" >> $@
mfbpawhite.c:
	echo "#define MFBSOLIDFILLAREA xf1bppSolidWhiteArea" > $@
	echo "#define MFBSTIPPLEFILLAREA xf1bppStippleWhiteArea" >> $@
	echo "#define OPEQ |=" >> $@
	echo "#define EQWHOLEWORD =~0" >> $@
	echo "#include \"$(top_srcdir)/mfb/mfbpntarea.c\"" >> $@
mfbpablack.c:
	echo "#define MFBSOLIDFILLAREA xf1bppSolidBlackArea" > $@
	echo "#define MFBSTIPPLEFILLAREA xf1bppStippleBlackArea" >> $@
	echo "#define OPEQ &=~" >> $@
	echo "#define EQWHOLEWORD =0" >> $@
	echo "#include \"$(top_srcdir)/mfb/mfbpntarea.c\"" >> $@
mfbpainv.c:
	echo "#define MFBSOLIDFILLAREA xf1bppSolidInvertArea" > $@
	echo "#define MFBSTIPPLEFILLAREA xf1bppStippleInvertArea" >> $@
	echo "#define OPEQ ^=" >> $@
	echo "#define EQWHOLEWORD ^=~0" >> $@
	echo "#include \"$(top_srcdir)/mfb/mfbpntarea.c\"" >> $@
mfbtewhite.c:
	echo "#define OP" > $@
	echo "#define CLIPTETEXT xf1bppImageGlyphBltWhite" >> $@
	echo "#define MFBTEGLYPHBLT xf1bppTEGlyphBltWhite" >> $@
	echo "#include \"$(top_srcdir)/mfb/mfbtegblt.c\"" >> $@
mfbteblack.c:
	echo "#define OP ~" > $@
	echo "#define CLIPTETEXT xf1bppImageGlyphBltBlack" >> $@
	echo "#define MFBTEGLYPHBLT xf1bppTEGlyphBltBlack" >> $@
	echo "#include \"$(top_srcdir)/mfb/mfbtegblt.c\"" >> $@
mfbplywhite.c:
	echo "#define MFBFILLPOLY1RECT xf1bppFillPolyWhite" > $@
	echo "#define OPEQ |=" >> $@
	echo "#define EQWHOLEWORD =~0" >> $@
	echo "#include \"$(top_srcdir)/mfb/mfbply1rct.c\"" >> $@
mfbplyblack.c:
	echo "#define MFBFILLPOLY1RECT xf1bppFillPolyBlack" > $@
	echo "#define OPEQ &=~" >> $@
	echo "#define EQWHOLEWORD =0" >> $@
	echo "#include \"$(top_srcdir)/mfb/mfbply1rct.c\"" >> $@
mfbplyinv.c:
	echo "#define MFBFILLPOLY1RECT xf1bppFillPolyInvert" > $@
	echo "#define OPEQ ^=" >> $@
	echo "#define EQWHOLEWORD ^=~0" >> $@
	echo "#include \"$(top_srcdir)/mfb/mfbply1rct.c\"" >> $@
mfbbltC.c:
	echo "#define MROP Mcopy" > $@
	echo "#include \"$(top_srcdir)/mfb/mfbblt.c\"" >> $@
mfbbltX.c:
	echo "#define MROP Mxor" > $@
	echo "#include \"$(top_srcdir)/mfb/mfbblt.c\"" >> $@
mfbbltCI.c:
	echo "#define MROP McopyInverted" > $@
	echo "#include \"$(top_srcdir)/mfb/mfbblt.c\"" >> $@
mfbbltO.c:
	echo "#define MROP Mor" > $@
	echo "#include \"$(top_srcdir)/mfb/mfbblt.c\"" >> $@
mfbbltG.c:
	echo "#define MROP M0" > $@
	echo "#include \"$(top_srcdir)/mfb/mfbblt.c\"" >> $@
mfbtileC.c:
	echo "#define MROP Mcopy" > $@
	echo "#include \"$(top_srcdir)/mfb/mfbtile.c\"" >> $@
mfbtileG.c:
	echo "#define MRop M0" > $@
	echo "#include \"$(top_srcdir)/mfb/mfbtile.c\"" >> $@

EXTRA_DIST = mfbmap.sh mfbunmap.sh 
