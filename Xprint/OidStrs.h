/* $Xorg: OidStrs.h,v 1.4 2001/03/14 18:45:40 pookie Exp $ */
/*
(c) Copyright 1996 Hewlett-Packard Company
(c) Copyright 1996 International Business Machines Corp.
(c) Copyright 1996 Sun Microsystems, Inc.
(c) Copyright 1996 Novell, Inc.
(c) Copyright 1996 Digital Equipment Corp.
(c) Copyright 1996 Fujitsu Limited
(c) Copyright 1996 Hitachi, Ltd.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the copyright holders shall
not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from said
copyright holders.
*/
/* $XFree86: xc/programs/Xserver/Xprint/OidStrs.h,v 1.5 2001/12/21 21:02:05 dawes Exp $ */

static int XpOidStringMapCount = 127;

#define OIDATA(name) { name, sizeof(name)-1, 0, 0, 0 }

static const XpOidStringMapEntry XpOidStringMap[] = {
    OIDATA(""),
    OIDATA(""),
    OIDATA("descriptor"),
    OIDATA("content-orientation"),
    OIDATA("copy-count"),
    OIDATA("default-printer-resolution"),
    OIDATA("default-input-tray"),
    OIDATA("default-medium"),
    OIDATA("document-format"),
    OIDATA("plex"),
    OIDATA("xp-listfonts-modes"),
    OIDATA("job-name"),
    OIDATA("job-owner"),
    OIDATA("notification-profile"),
    OIDATA("xp-setup-state"),
    OIDATA("xp-spooler-command-options"),
    OIDATA("content-orientations-supported"),
    OIDATA("document-formats-supported"),
    OIDATA("dt-pdm-command"),
    OIDATA("input-trays-medium"),
    OIDATA("medium-source-sizes-supported"),
    OIDATA("plexes-supported"),
    OIDATA("printer-model"),
    OIDATA("printer-name"),
    OIDATA("printer-resolutions-supported"),
    OIDATA("xp-embedded-formats-supported"),
    OIDATA("xp-listfonts-modes-supported"),
    OIDATA("xp-page-attributes-supported"),
    OIDATA("xp-raw-formats-supported"),
    OIDATA("xp-setup-proviso"),
    OIDATA("document-attributes-supported"),
    OIDATA("job-attributes-supported"),
    OIDATA("locale"),
    OIDATA("multiple-documents-supported"),
    OIDATA("available-compression"),
    OIDATA("available-compressions-supported"),
    OIDATA("portrait"),
    OIDATA("landscape"),
    OIDATA("reverse-portrait"),
    OIDATA("reverse-landscape"),
    OIDATA("iso-a0"),
    OIDATA("iso-a1"),
    OIDATA("iso-a2"),
    OIDATA("iso-a3"),
    OIDATA("iso-a4"),
    OIDATA("iso-a5"),
    OIDATA("iso-a6"),
    OIDATA("iso-a7"),
    OIDATA("iso-a8"),
    OIDATA("iso-a9"),
    OIDATA("iso-a10"),
    OIDATA("iso-b0"),
    OIDATA("iso-b1"),
    OIDATA("iso-b2"),
    OIDATA("iso-b3"),
    OIDATA("iso-b4"),
    OIDATA("iso-b5"),
    OIDATA("iso-b6"),
    OIDATA("iso-b7"),
    OIDATA("iso-b8"),
    OIDATA("iso-b9"),
    OIDATA("iso-b10"),
    OIDATA("na-letter"),
    OIDATA("na-legal"),
    OIDATA("executive"),
    OIDATA("folio"),
    OIDATA("invoice"),
    OIDATA("ledger"),
    OIDATA("quarto"),
    OIDATA("iso-c3"),
    OIDATA("iso-c4"),
    OIDATA("iso-c5"),
    OIDATA("iso-c6"),
    OIDATA("iso-designated-long"),
    OIDATA("na-10x13-envelope"),
    OIDATA("na-9x12-envelope"),
    OIDATA("na-number-10-envelope"),
    OIDATA("na-7x9-envelope"),
    OIDATA("na-9x11-envelope"),
    OIDATA("na-10x14-envelope"),
    OIDATA("na-number-9-envelope"),
    OIDATA("na-6x9-envelope"),
    OIDATA("na-10x15-envelope"),
    OIDATA("monarch-envelope"),
    OIDATA("a"),
    OIDATA("b"),
    OIDATA("c"),
    OIDATA("d"),
    OIDATA("e"),
    OIDATA("jis-b0"),
    OIDATA("jis-b1"),
    OIDATA("jis-b2"),
    OIDATA("jis-b3"),
    OIDATA("jis-b4"),
    OIDATA("jis-b5"),
    OIDATA("jis-b6"),
    OIDATA("jis-b7"),
    OIDATA("jis-b8"),
    OIDATA("jis-b9"),
    OIDATA("jis-b10"),
    OIDATA("simplex"),
    OIDATA("duplex"),
    OIDATA("tumble"),
    OIDATA("top"),
    OIDATA("middle"),
    OIDATA("bottom"),
    OIDATA("envelope"),
    OIDATA("manual"),
    OIDATA("large-capacity"),
    OIDATA("main"),
    OIDATA("side"),
    OIDATA("event-report-job-completed"),
    OIDATA("electronic-mail"),
    OIDATA("xp-setup-mandatory"),
    OIDATA("xp-setup-optional"),
    OIDATA("xp-setup-ok"),
    OIDATA("xp-setup-incomplete"),
    OIDATA("xp-list-glyph-fonts"),
    OIDATA("xp-list-internal-printer-fonts"),
    OIDATA("0"),
    OIDATA("01"),
    OIDATA("02"),
    OIDATA("03"),
    OIDATA("012"),
    OIDATA("013"),
    OIDATA("023"),
    OIDATA("0123")
};

#undef OIDATA
