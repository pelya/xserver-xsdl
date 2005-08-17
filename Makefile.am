AUTOMAKE_OPTIONS=dist-bzip2 foreign nostdinc

if COMPOSITE
COMPOSITE_DIR=composite
endif
if XTRAP
XTRAP_DIR=XTrap
endif
if CFB
CFB_DIR=cfb
CFB16_DIR=cfb16
CFB24_DIR=cfb24
CFB32_DIR=cfb32
endif
if AFB
AFB_DIR=afb
endif
if MFB
MFB_DIR=mfb
endif
if GLX
GLX_DIR=GL
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
	xkb	\
        dbe     \
	$(MFB_DIR)	\
	$(AFB_DIR)	\
	$(CFB_DIR)	\
	$(CFB16_DIR)   \
	$(CFB24_DIR)   \
	$(CFB32_DIR)   \
	record	\
	xfixes	\
	damageext \
	$(XTRAP_DIR) \
	$(COMPOSITE_DIR) \
        $(GLX_DIR) \
	hw	\
	Xprint

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xorg-server.pc
EXTRA_DIST = xorg-server.pc.in
