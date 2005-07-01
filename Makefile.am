AUTOMAKE_OPTIONS=dist-bzip2 foreign

if COMPOSITE
COMPOSITE_DIR=composite
endif
if XTRAP
XTRAP_DIR=XTrap
endif

SUBDIRS=	\
	include	\
	dix 	\
	fb	\
	mi	\
	Xext	\
	miext	\
	os	\
	randr	\
	render  \
        Xi	\
        mfb	\
        xkb	\
	afb	\
	cfb	\
	cfb16   \
	cfb24   \
	cfb32   \
	record	\
	xfixes	\
	damageext \
	$(XTRAP_DIR) \
        $(COMPOSITE_DIR) \
	hw

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xorg-server.pc
EXTRA_DIST = xorg-server.pc.in
