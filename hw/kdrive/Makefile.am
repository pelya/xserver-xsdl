if KDRIVEVESA
VESA_SUBDIRS = vesa mach64 mga nvidia r128 smi chips pm2 via
endif

if KDRIVEFBDEV
FBDEV_SUBDIRS = fbdev
endif

if XSDLSERVER
XSDL_SUBDIRS=sdl
endif

SUBDIRS =			\
	src			\
	linux			\
	$(XSDL_SUBDIRS)		\
	$(FBDEV_SUBDIRS)	\
	$(VESA_SUBDIRS)		\
	ati			\
	fake
