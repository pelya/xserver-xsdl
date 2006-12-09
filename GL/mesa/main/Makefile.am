noinst_LTLIBRARIES = libmain.la

AM_CFLAGS = \
	$(DIX_CFLAGS) \
	-DXFree86Server \
	@GLX_DEFINES@

INCLUDES = -I@MESA_SOURCE@/include \
           -I../X \
           -I../array_cache \
           -I../glapi \
           -I../main \
           -I../math \
           -I../shader \
           -I../swrast \
           -I../swrast_setup \
           -I../tnl \
           -I.. \
           -I$(top_srcdir)/hw/xfree86/os-support

if NEED_VSNPRINTF
VSNPRINTF_SOURCES = vsnprintf.c
endif

nodist_libmain_la_SOURCES = accum.c \
                    api_arrayelt.c \
                    api_loopback.c \
                    api_noop.c \
                    api_validate.c \
                    arrayobj.c \
                    attrib.c \
                    blend.c \
                    bufferobj.c \
                    buffers.c \
                    clip.c \
                    colortab.c \
                    context.c \
                    convolve.c \
                    debug.c \
                    depth.c \
		    depthstencil.c \
                    dlist.c \
                    drawpix.c \
                    enable.c \
                    enums.c \
                    eval.c \
                    execmem.c \
                    extensions.c \
                    fbobject.c \
                    feedback.c \
                    fog.c \
                    framebuffer.c \
                    get.c \
                    getstring.c \
                    hash.c \
                    hint.c \
                    histogram.c \
                    image.c \
                    imports.c \
                    light.c \
                    lines.c \
                    matrix.c \
                    mipmap.c \
                    mm.c \
                    occlude.c \
                    pixel.c \
                    points.c \
                    polygon.c \
                    rastpos.c \
                    rbadaptors.c \
                    renderbuffer.c \
                    state.c \
                    stencil.c \
                    texcompress.c \
                    texcompress_fxt1.c \
                    texcompress_s3tc.c \
                    texenvprogram.c \
                    texformat.c \
                    teximage.c \
                    texobj.c \
                    texrender.c \
                    texstate.c \
                    texstore.c \
                    varray.c \
                    $(VSNPRINTF_SOURCES) \
                    vtxfmt.c
