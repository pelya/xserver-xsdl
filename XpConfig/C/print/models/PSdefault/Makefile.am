SUBDIRS = fonts

xpcdir = @xpconfigdir@/C/print/models/PSdefault

xpc_DATA = model-config

install-data-hook:
	@rm -f $(xpcdir)/fonts.scale
	$(MKFONTSCALE) $(xpcdir)
	@rm -f $(xpcdir)/font.dir
	$(MKFONTDIR) $(xpcdir)

distuninstallcheck:
	@:
