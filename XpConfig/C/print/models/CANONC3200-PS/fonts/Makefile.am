xpcdir = @xpconfigdir@/C/print/models/SPSPARC2/fonts

parentdir = @xpconfigdir@/C/print/models/PSdefault/fonts

XPFONTS = \
        AvantGarde-Book.pmf \
        AvantGarde-BookOblique.pmf \
        AvantGarde-Demi.pmf \
        AvantGarde-DemiOblique.pmf \
        Courier-Bold.pmf \
        Courier-BoldOblique.pmf \
        Courier-Oblique.pmf \
        Courier.pmf \
        Helvetica-Bold.pmf \
        Helvetica-BoldOblique.pmf \
        Helvetica-Oblique.pmf \
        Helvetica.pmf \
        LubalinGraph-Book.pmf \
        LubalinGraph-BookOblique.pmf \
        LubalinGraph-Demi.pmf \
        LubalinGraph-DemiOblique.pmf \
        NewCenturySchlbk-Bold.pmf \
        NewCenturySchlbk-BoldItalic.pmf \
        NewCenturySchlbk-Italic.pmf \
        NewCenturySchlbk-Roman.pmf \
        Souvenir-Demi.pmf \
        Souvenir-DemiItalic.pmf \
        Souvenir-Light.pmf \
        Souvenir-LightItalic.pmf \
        Symbol.pmf \
        Times-Bold.pmf \
        Times-BoldItalic.pmf \
        Times-Italic.pmf \
        Times-Roman.pmf \
        ZapfDingbats.pmf

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
