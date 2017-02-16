if XEPHYR
XEPHYR_SUBDIRS = ephyr
endif

if KDRIVELINUX
LINUX_SUBDIRS = linux
endif

SERVER_SUBDIRS = 		\
	$(XEPHYR_SUBDIRS)

SUBDIRS =			\
	src			\
	$(LINUX_SUBDIRS)	\
	$(SERVER_SUBDIRS)

DIST_SUBDIRS = ephyr src linux

relink:
	$(AM_V_at)for i in $(SERVER_SUBDIRS) ; do $(MAKE) -C $$i relink || exit 1 ; done
