INCLUDES =		    \
	@XGL_INCS@	    \
	@XSERVER_CFLAGS@    \
	@XGLXSERVER_CFLAGS@

bin_PROGRAMS = Xglx

Xglx_SOURCES = xglx.c

Xglx_LDADD =	          \
	@XGL_LIBS@        \
	@XSERVER_LIBS@    \
	@XGLXSERVER_LIBS@

Xglx_DEPENDENCIES = @XGL_LIBS@

