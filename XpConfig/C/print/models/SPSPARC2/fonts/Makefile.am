xpcdir = @xpconfigdir@/C/print/models/SPSPARC2/fonts

parentdir = @xpconfigdir@/C/print/models/PSdefault/fonts

XPFONTS = \
        Courier-Bold.pmf \
        Courier-BoldOblique.pmf \
        Courier-Oblique.pmf \
        Courier.pmf \
        Helvetica-Bold.pmf \
        Helvetica-BoldOblique.pmf \
        Helvetica-Oblique.pmf \
        Helvetica.pmf \
        Symbol.pmf \
        Times-Bold.pmf \
        Times-BoldItalic.pmf \
        Times-Italic.pmf \
        Times-Roman.pmf

install-data-hook:
	mkdir -p $(xpcdir) ;								\
											\
	for x in $(XPFONTS) ; do							\
		rm -f $(xpcdir)/$$x	;						\
		ln -s $(parentdir)/$$x $(xpcdir)/$$x	;				\
	done

	@rm -f $(xpcdir)/fonts.scale
	$(MKFONTSCALE) $(xpcdir)
	@rm -f $(xpcdir)/font.dir
	$(MKFONTDIR) $(xpcdir)

distuninstallcheck:
	@:
