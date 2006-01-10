if KDRIVEVESA
VESA_SUBDIRS = vesa ati chips epson i810 mach64 mga neomagic nvidia pm2 r128 \
               smi via
endif

if KDRIVEFBDEV
FBDEV_SUBDIRS = fbdev
endif

if XSDLSERVER
XSDL_SUBDIRS = sdl
endif

if XEPHYR
XEPHYR_SUBDIRS = ephyr
endif

SUBDIRS =			\
	src			\
	linux			\
	$(XSDL_SUBDIRS)		\
	$(FBDEV_SUBDIRS)	\
	$(VESA_SUBDIRS)		\
	$(XEPHYR_SUBDIRS)       \
	fake

DIST_SUBDIRS = vesa ati chips epson i810 mach64 mga neomagic nvidia pm2 r128 \
               smi via fbdev sdl ephyr src linux fake sis300
