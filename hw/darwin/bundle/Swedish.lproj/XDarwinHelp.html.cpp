<!-- $XFree86: xc/programs/Xserver/hw/darwin/bundle/Swedish.lproj/XDarwinHelp.html.cpp,v 1.2 2001/11/07 22:43:27 torrey Exp $ -->

#include "xf86Version.h"
#ifndef PRE_RELEASE
#define PRE_RELEASE XF86_VERSION_SNAP
#endif

<html>
<head>
<title>XFree86 f&ouml;r Mac OS X</title>
</head>
<body>
<center>
    <h1>XFree86 f&ouml;r Darwin och Mac OS X</h1>
    XFree86 XF86_VERSION<br>
    F&auml;rdigst&auml;llt: XF86_REL_DATE
</center>
<h2>Inneh&aring;ll</h2>
<ol>
    <li><A HREF="#notice">Viktigt!</A></li>
    <li><A HREF="#usage">Anv&auml;ndande</A></li>
    <li><A HREF="#path">Att st&auml;lla in sin s&ouml;kv&auml;g</A></li>
    <li><A HREF="#prefs">Inst&auml;llningar</A></li>
    <li><A HREF="#license">Licens</A></li>
</ol>
<center>
    <h2><a NAME="notice">Viktigt!</a></h2>
</center>
<blockquote>
#if PRE_RELEASE
Detta &auml;r en testversion av XFree86, och du kan inte garranteras n&aring;gon som helst support f&ouml;r den. Buggar och fel kan rapporteras och f&ouml;rslag till fixar kan skickas till <A HREF="http://sourceforge.net/projects/xonx/">XonX-projektets sida</A> p&aring; SourceForge.  Innan du rapporterar buggar i testversioner, var god pr&ouml;va den senaste versionen fr&aring;n <A HREF="http://sourceforge.net/projects/xonx/">XonX</A> eller i <A HREF="http://www.XFree86.Org/cvs">XFree86 CVS-arkiv</A>.
#else
Om servern &auml;r &auml;ldre &auml;n 6-12 m&aring;nader, eller om din h&aring;rdvara &auml;r nyare &auml;n datumet ovan, leta efter en nyare version innan du rapporterar fel. Buggar och fel kan rapporteras och f&ouml;rslag till fixar kan skickas till <A HREF="http://sourceforge.net/projects/xonx/">XonX-projektets sida</A> p&aring; SourceForge.
#endif
</blockquote>
<blockquote>
Denna programvara distrubueras i enlighet med <A HREF="#license">MIT X11 / X Consortium License</A> och tilhandh&aring;lls som den &auml;r, helt utan garantier. Var god l&auml;s igenom <A HREF="#license">licensdokumentet (engelska)</A> innan du anv&auml;nder programmet.</blockquote>

<h2><a NAME="usage">Anv&auml;ndande</a></h2>
<p>XFree86 &auml;r en fritt spridd implemenation av <a HREF
="http://www.x.org/">X Window-systemet</a> producerad av <a HREF="http://www.XFree86.Org/">XFree86 Project, Inc</a>, med &ouml;ppen k&auml;llkod. Den X Window-server f&ouml;r Darwin och Mac OS X som tillhandah&aring;lls av XFree86 kallas XDarwin. XDarwin kan k&ouml;ras p&aring; Mac OS X i fullsk&auml;rmsl&auml;ge eller rotl&ouml;st l&auml;ge.</p>
<p>I fullsk&auml;rmsl&auml;ge kommer X window-systemet att ta &ouml;ver hela sk&auml;rmen n&auml;r det &auml;r aktivt. Du kan byta tillbaka till Mac OS Xs skrivbord genom att trycka Kommando-Alt-A. Denna tangentkombination kan &auml;ndra i inst&auml;llningarna. N&auml;r du &auml;r p&aring; Mac OS Xs skrivbord kan du klicka p&aring; XDarwin-ikonen i dockan f&ouml;r att byta tillbaka till X Window-systemet. (Du kan f&ouml;r&auml;ndra detta beteende i inst&auml;llningarna s&aring; att du ist&auml;llet m&aring;ste klicka i det fltande bytesf&ouml;nstret ist&auml;llet.)</p>
<p>I rotl&ouml;stl&auml;ge delar X11 och Aqua p&aring; din sk&auml;rm. Rotf&ouml;nstret p&aring; X11-sk&auml;rmen &auml;r av samma storlek som hela sk&auml;rmen och inneh&aring;ller alla andra f&ouml;nster - det fungerar som bakgrund. I rotl&ouml;stl&auml;ge visas inte detta rotf&ouml;nster, eftersom Aqua hanterar skrvbordbakgrunden.</p>

<h3>Emulering av flerknapparsmus</h3>
<p>M&aring;nga X11-program utnyttjar en treknapparsmus. Du kan emulera en treknapparsmus med en knapp genom att h&aring;lla ner olika knappar p&aring; tangentbordet medan du klickar med musens enda knapp. Denna funktion styrs av inst&auml;llningarna i  "Emulera flerknapparsmus" under fliken "Diverse" i inst&auml;llningarna. Grundinst&auml;llningen &auml;r att emulationen &auml;r aktiv  och att ett kommando-klick (H&aring;ll ner kommando och klicka) simulerar den andra musknappen. Den tredje musknappen f&aring;s genom att h&aring;lla ner alt och klicka. Du kan &auml;ndra detta till n&aring;gon annan kombination av de fem tangenterna kommando, alt, kontrol, skift och fn (Powerbook/iBook). Notera att om dessa knappar har flyttats med hj&auml;lp av kommandot xmodmap kommer denna f&ouml;r&auml;ndring inte att p&aring;verka vilka knappar som anv&auml;nds vid flerknappsemulationen.</p>

<h2><a NAME="path">Att st&auml;lla in sin s&ouml;kv&auml;g</a></h2>
<p>Din s&ouml;kv&auml;g &auml;r en lista av kataloger som s&ouml;ks igenom n&auml;r terminalen letar efter kommandon att exekvera. Kommandon som h&ouml;r till X11 ligger i <code>/usr/X11R6/bin</code>, en katalog som inte ligger i din s&ouml;kv&auml;g fr&aring;n b&ouml;rjan. XDarwin l&auml;gger till denna katalog &aring;t dig, och du kan ocks&aring; l&auml;gga till ytterligare kataloger i vilka du lagt program som skall k&ouml;ras fr&aring;n kommandoraden.</p>
<p>Mer erfarna anv&auml;ndare har antagligen redan st&auml;llt in sin s&ouml;kv&auml;g i skalets inst&auml;llningsfiler. Om detta g&auml;ller dig  kan st&auml;lla in XDarwin s&aring; att din s&ouml;kv&auml;g inte modifieras. XDarwin startar de f&ouml;rsta X11-klienterna i anv&auml;ndarens inloggningsskal (Vill du anv&auml;nda ett alternativt skall, kan du specificera detta i inst&auml;llningarna). Hur du st&auml;ller in din s&ouml;kv&auml;g beror p&aring; vilket skal du anv&auml;nder. Exakt hur beskrivs i skalets man-sidor.</p>

<p>Ut&ouml;ver detta kan du ocks&aring; vilja l&auml;gga till XFree86s man-sidor (dokumentation) till listan &auml;ver sidor som som skall s&ouml;kas n&auml;r du vill l&auml;sa efter dokumentationen. X11s man-sidor ligger i <code>/usr/X11R6/man</code> och listan &auml;ver kataloger att s&ouml;ka best&auml;mms av variabeln<code>MANPATH</code>.</p>

<h2><a NAME="prefs">Inst&auml;llningar</a></h2>
<p>I inst&auml;llningarna finns ett antal alternativ d&auml;r du kan p&aring;verka hur XDarwin beter sig i vissa fall. Inst&auml;llningarna kommer du till genom att v&auml;lja "Inst&auml;llningar..." i menyn "XDarwin". De alternativ som finns under fliken "Starta" tr&auml;der inte i kraft f&ouml;rr&auml;n du startat om programmet. Alla andra alternativ tr&auml;der i kraft omedelbart. De olika alternativen beskrivs nedan:</p>
<h3>Diverse</h3>
<ul>
    <li><b>Anv&auml;nd Mac OS varningsljud i X11:</b> N&auml;r detta alternativ &auml;r valt anv&auml;nds Mac OS vanliga varningsljud &auml;r X11s varningsljud (bell). N&auml;r detta alternativ inte &auml;r valt (f&ouml;rvalt) anv&auml;nds en vanlig ton.</li>
    <li><b>Till&aring;t X11 att &auml;ndra musens acceleration:</b> I ett vanligt X11-system kan f&ouml;nsterhanteraren &auml;ndra musens acceleration. Detta kan vara f&ouml;rvirrande eftersom musens acceleration kan vara olika i Mac OS Xs System Preferences och i f&ouml;nsterhanteraren i X11. F&ouml;rvalet &auml;r att X11 inte kan &auml;ndra musens acceleration f&ouml;r att p&aring; detta s&auml;tt undvika detta problem.</li>
    <li><b>Emulera flerknapparsmus:</b> Detta beskrivs ovan under <a HREF="#usage">Anv&auml;ndande</a>. N&auml;r emulationen &auml;r aktiv m&aring;ste du h&aring;lla ner de valda knapparna f&ouml;r att emulera en andra eller tredje musknapp.</li>
</ul>
<h3>Starta</h3>
<ul>
    <li><b>F&ouml;rvalt l&auml;ge:</b> Om anv&auml;ndaren inte p&aring; annat s&auml;tt v&auml;ljer vilket l&auml;ge som skall anv&auml;ndas kommer alternativet h&auml;r att anv&auml;ndas.</li>
    <li><b>Visa val av sk&auml;rml&auml;ge vid start:</b> F&ouml;rvalet &auml;r att visa ett f&ouml;nster n&auml;r XDarwin startar som l&aring;ter anv&auml;ndaren v&auml;lja mellan fullsk&auml;rmsl&auml;ge och rotl&ouml;st l&auml;ge. Om detta alternativ inte &auml;r aktivt kommer XDarwin automatiskt att startas i det l&auml;ge som valts ovan.</li>
    <li><b>Sk&auml;rmnummer i X11:</b> X11 till&aring;ter att det finns flera sk&auml;rmar styrda av varsin X-server p&aring; en och samma dator. Anv&auml;ndaren kan ange vilket nummer XDarwin skall anv&auml;nda om mer &auml;n en X-server skall anv&auml;ndas samtidigt.</li>
    <li><b>Aktivera Xinerama (st&ouml;d f&ouml;r flera sk&auml;rmar):</b> XDarwin st&ouml;djer flera sk&auml;rmar genom Xinerama, vilket hanterar alla skr&auml;mar som delar av en enda stor rektangul&auml;r sk&auml;rm. Du kan anv&auml;nda detta alternativ f&ouml;r att st&auml;nga av Xinerama, men f&ouml;r n&auml;rvarande kan inte XDarwin hantera flera sk&auml;rmar utan det. Om du bara har en sk&auml;rm kommer Xinerama automatiskt att deaktiveras.</li>
    <li><b>Fil med tangentbordsupps&auml;ttning:</b> En fil som anger tangentbordsupps&auml;ttning l&auml;ses vid start och &ouml;vers&auml;tts till en tangentborsupps&auml;ttningsfil f&ouml;r X11. Filer med tangentbordsupps&auml;ttningar f&ouml;r ett stort antal spr&aring;k finns i <code>/System/Library/Keyboards</code>.</li>
    <li><b>Startar f&ouml;rsta X11-klienterna:</b> N&auml;r X11 startas fr&aring;n Finder kommer det att exekvera filen <code>xinit</code> f&ouml;r att starta f&ouml;nsterhanteraren i X11 och andra program. (Se "<code>man xinit</code>" f&ouml;r mer information.) Innan XDarwin k&ouml;r xinit kommer det att l&auml;gga till katalogern h&auml;r till anv&auml;ndarens s&ouml;kv&auml;g. F&ouml;rvalet &auml;r att endast l&auml;gga till katalogen <code>/usr/X11R6/bin</code>. Ytterligare kataloger kan l&auml;ggas till - separera dem med kolon. X11-klienterna startas i anv&auml;ndarens inloggningsskal s&aring; att anv&auml;ndarens inst&auml;llningsfiler i skalet l&auml;ses. Om s&aring; &ouml;nskas kan de startas i ett annat skal.</li>
</ul>
<h3>Fullsk&auml;rm</h3>
<ul>
    <li><b>Tangentkombinationsknappen:</b> Tryck p&aring; denna knapp och en tangentkombination f&ouml;r att &auml;ndra den tangentkombination som anv&auml;nds f&ouml;r att byta mellan X11 och Aqua.</li>
    <li><b>Klick p&aring; ikonen i dockan byter till X11:</b> Aktivera detta alternativ f&ouml;r att byta till X11 genom att klicka p&aring; ikonen i dockan. I vissa versioner av Mac OS X kommer ett bte p&aring; detta s&auml;tt att g&ouml;mma pekaren n&auml;r du &aring;terv&auml;nder till Aqua.</li>
    <li><b>Visa fullsk&auml;rmshj&auml;lp vid start:</b> Detta kommer att visa en informationsruta n&auml;r XDarwin startas i fullsk&auml;rmsl&auml;ge.</li>
    <li><b>F&auml;rgdjup:</b> I fullsk&auml;rmsl&auml;ge kan X11 anv&auml;nda ett annat f&auml;rgdjup &auml;n Aquas. Om du v&auml;jer "Nuvarande" kommer X11 att anv&auml;nda det f&auml;rgdjup som Aqua har just d&aring;.  Annars kan du v&auml;lja 8, 15, eller 24 bitare f&auml;rg.</li>
</ul>

<h2><a NAME="license">Licens (svenska)</a></h2>
<p>XFree86-projektet &aring;tar sig att tillhandah&aring;lla programvara och k&auml;llkod i format som fritt kan spridas vidare. Den huvudsakliga licens vi anv&auml;nder oss av &auml;r baserad p&aring; den traditionella MIT X11 / XConsortium-licensen, vilken inte p&aring; n&aring;got s&auml;tt begr&auml;nsar f&ouml;r&auml;ndringar eller vidarespridning av vare sig k&auml;llkod eller kompilerad programvara annat &auml;n genom att kr&auml;va att delarna som r&ouml;r copyright och licensiering l&auml;mnas intakta. F&ouml;r mer information och ytterligare copyright/licensieringsinfromation r&ouml;rande vissa speciella delar av koden, se <A HREF="http://www.xfree86.org/legal/licence.html">XFree86-licenssida</A> (engelska).</p>

<h3>Licence (english)</h3>
<p>The XFree86 Project is committed to providing freely redistributable binary and source releases. The main license we use is one based on the traditional MIT X11 / X Consortium License, which does not impose any conditions on modification or redistribution of source code or binaries other than requiring that copyright/license notices are left intact. For more information and additional copyright/licensing notices covering some sections of the code, please see the <A HREF="http://www.xfree86.org/legal/licence.html">XFree86 License page</A>.</p>

<H3><A NAME="3"></A>X Consortium License</H3>
<p>Copyright (C) 1996 X Consortium</p>
<p>Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without
limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to
whom the Software is furnished to do so, subject to the following conditions:</p>
<p>The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.</p>
<p>THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT
SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.</p>
<p>Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization from
the X Consortium.</p>
<p>X Window System is a trademark of X Consortium, Inc.</p>
</body>
</html>
