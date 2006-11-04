if KDRIVEVESA
VESA_SUBDIRS = vesa ati chips epson i810 mach64 mga neomagic nvidia pm2 r128 \
               smi via
endif

if BUILD_KDRIVEFBDEVLIB
FBDEV_SUBDIRS = fbdev
endif

if XFAKESERVER
XFAKE_SUBDIRS = fake
endif

if XSDLSERVER
XSDL_SUBDIRS = sdl
endif

if XEPHYR
XEPHYR_SUBDIRS = ephyr
endif

if KDRIVELINUX
LINUX_SUBDIRS = linux
endif

SUBDIRS =			\
	src			\
	$(LINUX_SUBDIRS)	\
	$(XSDL_SUBDIRS)		\
	$(FBDEV_SUBDIRS)	\
	$(VESA_SUBDIRS)		\
	$(XEPHYR_SUBDIRS)       \
	$(XFAKE_SUBDIRS)

DIST_SUBDIRS = vesa ati chips epson i810 mach64 mga neomagic nvidia pm2 r128 \
               smi via fbdev sdl ephyr src linux fake sis300
