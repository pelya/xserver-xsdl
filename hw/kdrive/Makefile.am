if KDRIVEVESA
VESA_SUBDIRS = vesa mach64 mga nvidia r128 smi chips pm2 via neomagic
endif

if KDRIVEFBDEV
FBDEV_SUBDIRS = fbdev
endif

if XSDLSERVER
XSDL_SUBDIRS=sdl
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
	ati			\
	fake			\
	ephyr                   \
	i810			\
	epson
