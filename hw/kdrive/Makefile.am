if KDRIVEVESA
VESA_SUBDIRS = vesa mach64 mga nvidia r128 smi chips
endif

if KDRIVEFBDEV
FBDEV_SUBDIRS = fbdev
endif

XSDL_SUBDIRS=@XSDL_DIRS@

SUBDIRS =			\
	src			\
	linux			\
	$(XSDL_SUBDIRS)		\
	$(FBDEV_SUBDIRS)	\
	$(VESA_SUBDIRS)		\
	ati
