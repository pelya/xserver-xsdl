AUTOMAKE_OPTIONS=dist-bzip2 foreign nostdinc
DISTCHECK_CONFIGURE_FLAGS=--disable-xorgcfg

if COMPOSITE
COMPOSITE_DIR=composite
endif

if XTRAP
XTRAP_DIR=XTrap
endif

if CFB
CFB_DIR=cfb
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

if XINPUT
XINPUT_DIR=Xi
endif

if DBE
DBE_DIR=dbe
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
	$(XINPUT_DIR) \
	xkb \
	$(DBE_DIR) \
	$(MFB_DIR) \
	$(AFB_DIR) \
	$(CFB_DIR) \
	$(CFB32_DIR) \
	record \
	xfixes \
	damageext \
	$(XTRAP_DIR) \
	$(COMPOSITE_DIR) \
	$(GLX_DIR) \
	exa \
	hw

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
	cfb32 \
	record \
	xfixes \
	damageext \
	XTrap \
	composite \
	GL \
	exa \
	hw

# gross hack
relink: all
	$(MAKE) -C hw relink
