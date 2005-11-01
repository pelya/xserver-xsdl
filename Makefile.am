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

if XPRINT
XPDDX_DIR=Xprint
XPCONFIG_DIR=XpConfig
endif

if LBX
LBX_DIR=lbx
endif

SUBDIRS = \
	doc \
	include \
	dix  \
	fb \
	mi \
	Xext \
	miext \
	os \
	randr \
	render  \
	Xi \
	xkb \
	dbe \
	$(MFB_DIR) \
	$(AFB_DIR) \
	$(CFB_DIR) \
	$(CFB16_DIR) \
	$(CFB24_DIR) \
	$(CFB32_DIR) \
	record \
	xfixes \
	damageext \
	$(XTRAP_DIR) \
	$(COMPOSITE_DIR) \
	$(GLX_DIR) \
	$(LBX_DIR) \
	hw \
	$(XPDDX_DIR) \
	$(XPCONFIG_DIR)

aclocaldir = $(datadir)/aclocal
aclocal_DATA = xorg-server.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xorg-server.pc

EXTRA_DIST = xorg-server.pc.in xorg-server.m4

DIST_SUBDIRS = \
	doc \
	include \
	dix  \
	fb \
	mi \
	Xext \
	miext \
	os \
	randr \
	render  \
	Xi \
	xkb \
	dbe \
	mfb \
	afb \
	cfb \
	cfb16 \
	cfb24 \
	cfb32 \
	ilbm \
	iplan2p4 \
	record \
	xfixes \
	damageext \
	XTrap \
	composite \
	GL \
	hw \
	Xprint \
	XpConfig \
	lbx
