if XSDLSERVER
XSDL_SUBDIRS = sdl
endif

if XEPHYR
XEPHYR_SUBDIRS = ephyr
endif

SERVER_SUBDIRS = 		\
	$(XSDL_SUBDIRS)		\
	$(XEPHYR_SUBDIRS)

SUBDIRS =			\
	src			\
	$(SERVER_SUBDIRS)

DIST_SUBDIRS = sdl ephyr src

relink:
	$(AM_V_at)for i in $(SERVER_SUBDIRS) ; do $(MAKE) -C $$i relink || exit 1 ; done
