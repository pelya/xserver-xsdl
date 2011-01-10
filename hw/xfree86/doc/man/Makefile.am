appmandir = $(APP_MAN_DIR)
appman_PRE = Xorg.man
appman_DATA = $(appman_PRE:man=$(APP_MAN_SUFFIX))

filemandir = $(FILE_MAN_DIR)
fileman_PRE = xorg.conf.man
fileman_DATA = $(fileman_PRE:man=$(FILE_MAN_SUFFIX))

EXTRA_DIST = $(appman_PRE) $(fileman_PRE)
CLEANFILES = $(appman_DATA) $(fileman_DATA)
SUFFIXES = .$(APP_MAN_SUFFIX) .$(FILE_MAN_SUFFIX) .man

# String replacements in MAN_SUBSTS now come from xorg-macros.m4 via configure
MAN_SUBSTS += 	-e 's|__logdir__|$(logdir)|g' \
		-e 's|__datadir__|$(datadir)|g' \
		-e 's|__mandir__|$(mandir)|g' \
		-e 's|__sysconfdir__|$(sysconfdir)|g' \
		-e 's|__xconfigdir__|$(__XCONFIGDIR__)|g' \
		-e 's|__xkbdir__|$(XKB_BASE_DIRECTORY)|g' \
		-e 's|__modulepath__|$(DEFAULT_MODULE_PATH)|g'

.man.$(APP_MAN_SUFFIX):
	$(AM_V_GEN)$(SED) $(MAN_SUBSTS) < $< > $@
.man.$(FILE_MAN_SUFFIX):
	$(AM_V_GEN)$(SED) $(MAN_SUBSTS) < $< > $@
