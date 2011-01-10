drivermandir = $(DRIVER_MAN_DIR)
driverman_PRE = exa.man
driverman_DATA = $(driverman_PRE:man=$(DRIVER_MAN_SUFFIX))

EXTRA_DIST = $(driverman_PRE)
CLEANFILES = $(driverman_DATA)
SUFFIXES = .$(DRIVER_MAN_SUFFIX) .man

# String replacements in MAN_SUBSTS now come from xorg-macros.m4 via configure
.man.$(DRIVER_MAN_SUFFIX):
	$(AM_V_GEN)$(SED) $(MAN_SUBSTS) < $< > $@
