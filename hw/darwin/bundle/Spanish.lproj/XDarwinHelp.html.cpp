<!-- $XFree86: xc/programs/Xserver/hw/darwin/bundle/English.lproj/XDarwinHelp.html.cpp,v 1.2 2001/11/04 07:02:28 torrey Exp $ -->

<html>
<head>
<title>XDarwin Ayuda</title>
</head>
<body>
<center>
    <h1>XDarwin X Server for Mac OS X</h1>
    X_VENDOR_NAME X_VERSION<br>
    Fecha de release: X_REL_DATE
</center>
<h2>Contenido</h2>
<ol>
    <li><A HREF="#notice">Aviso Importante</A></li>
    <li><A HREF="#usage">Modo de uso</A></li>
    <li><A HREF="#path">Configurando su Path</A></li>
    <li><A HREF="#prefs">Preferencias del Usuario</A></li>
    <li><A HREF="#license">Licencia</A></li>
</ol>
<center>
    <h2><a NAME="notice">Aviso Importante</a></h2>
</center>
<blockquote>
#if PRE_RELEASE
Esta es una versi&oacuten pre-release de XDarwin, y no tiene ning&uacuten soporte. Patches y reportes de error pueden ser enviados a la <A HREF="http://sourceforge.net/projects/xonx/">p&aacutegina del proyecto XonX</A> en SourceForge.  Antes de reportar errores en versiones pre-release, por favor verifique la ultima versi&oacuten en <A HREF="http://sourceforge.net/projects/xonx/">XonX</A> o bien el X_VENDOR_LINK.
#else
Si el server el m&aacutes antiguo que 6 a 12 meses, o si su hardware es posterior a la fecha indicada m&aacutes arriba, por favor verifique la &uacuteltima versi&oacuten antes de reportar problemas. Patches y reportes de error pueden ser enviados a la <A HREF="http://sourceforge.net/projects/xonx/">p&aacutegina del proyecto XonX</A> en SourceForge.
#endif
</blockquote>
<blockquote>
Este software es distribuido bajo los t&eacuterminos de la <A HREF="#license">Licencia MIT X11 / X Consortium</A> y es provisto sin garant&iacutea alguna y en el estado en que se encuentra. Por favor lea la <A HREF="#license">Licencia</A> antes de utilizarlo.</blockquote>

<h2><a NAME="usage">Modo de uso</a></h2>
<p>XDarwin es una X server open-source de distribuci&oacuten libre del <a HREF
="http://www.x.org/">X Window System</a>. This version of XDarwin was produced by the X_VENDOR_LINK. XDarwin funciona en Mac OS X en modo pantalla completa o en modo rootless (integrado al escritorio).</p>
<p>En modo pantalla completa, el X window system toma control total de la pantalla mientras esta activo. Presionando Command-Option-A  puede regresar al Escritorio de Mac OS X. Esta combinaci&oacuten de teclas puede cambiarse en las Preferencias de Usuario. Desde el Escritorio de Mac OS X, haga click en &iacutecono de XDarwin en el Dock para volver al X window system.  (Puede cambiar esta comportamiento en las Preferencias de Usuario y configurar que XDarwin vuelva al X window system haciendo click en la ventana flotante con el logo X.)</p>
<p>En modo rootless, el X window system comparte la pantalla con Aqua. La ventana root de X11 es del tama&ntildeo de la pantalla y contiene a todas las dem&aacutes ventanas. La ventana root de X11 no se muestra en este modo, ya que Aqua maneja el fondo de pantalla.</p>
<h3>Emulaci&oacuten de mouse multi-bot&oacuten</h3>
<p>Muchas aplicaciones X11 requieren del uso de un mouse de 3 botones. Es posible emular un mouse de 3 botones con un mouse de solo un bot&oacuten presionando teclas modificadoras mientras hace click. Esto es controlado en de la seccion "Emulaci&oacuten mouse" dentro de la secci&oacuten "General" de las Preferencias del Usuario. Por defecto, la emulaci&oacuten est&aacute activa y utiliza la tecla Command para simular el 2do bot&oacuten y la tecla Option para simlar el 3er bot&oacuten. La conbinaci&oacuten para simular el 2do y 3er bot&oacuten pueden ser modificada por cualquier combinaci&oacuten de teclas modificadoras dentro de las Preferencias del Usuario. Tenga en cuenta que aunque las teclas modificadoras hayan sido mapeadas a otras teclas con xmodmap, las teclas configuradas en las Preferencias del Usuario seguir&aacuten siendo las utilizadas por la emulaci&oacuten de mouse multi-bot&oacuten.</p>

<h2><a NAME="path">Configurando su Path</a></h2>
<p>El path es la lista de directorios donde se buscar&aacuten los comandos ejecutables. Los comandos de X11 se encuentran en  <code>/usr/X11R6/bin</code>, y &eacuteste necesita estar dentro de su path. XDarwin hace &eacutesto autom&aacuteticamente por defecto, y puede adem&aacutes agregar directorios adicionales donde tenga otros comandos de l&iacutenea.</p>
<p>Usuarios experimentados pueden tener su path correctamente configurado mediante los archivos de inicio de su interprete de comandos.  En este caso, puede informarle a XDarwin en las Preferencias de Usuario para que no modifique su path. XDarwin arrancar&aacute los clientes X11 iniciales usando el int&eacuterprete de comandos del usuario, seg&uacuten su configuraci&oacuten de login. Un int&eacuterprete de comandos alternativo puede ser especificado en las Preferencias del Usuario. La manera de configurar el path de su int&eacuterprete de comandos depende de cual est&aacute usando, y es generalmente descripta en las p&aacuteginas man del mismo.</p>
<p>Adem&aacutes, Ud. puede agregar las p&aacuteginas man de X11 a la lista de p&aacuteginas que son consultadas. Estas est&aacuten ubicadas en <code>/usr/X11R6/man</code> y <code>MANPATH</code> es la variable de entorno que contiene los directorios que son consultados.</p>

<h2><a NAME="prefs">Preferencias del Usuario</a></h2>
<p>Ciertas opciones pueden definirse dentro de "Preferencias...", en el men&uacute de XDarwin. Las opciones dentro de de "Inicio" no surtir&aacuten efecto hasta que la aplicaci&oacuten se reinicie. Las restantes opciones surten efecto inmediatamente. Las diferentes opciones se describen a continuaci&oacuten:</p>
<h3>General</h3>
<ul>
    <li><b>Usar beep del sistema en X11:</b> Cuando esta opci&oacuten est&aacute activa, el sonido de alerta est&aacutendar de Mac OS X se usar&aacute como alerta de X11. Cuando est&aacute desactivada, un simple tono es utilizado (esta es la opci&oacuten por defecto).</li>
    <li><b>Permitir que X11 cambie la aceleraci&oacuten del mouse:</b> En una implementaci&oacuten est&aacutendard de X11, el window manager puede cambiar la aceleraci&oacuten del mouse. Esto puede llevar a una gran confusi&oacuten si la aceleraci&oacuten es diferente en XDarwin y en Mac OS X. Por defecto, no se le permite a X11 alterar la aceleraci&oacuten para evitar este inconveniente.</li>
    <li><b>Emulaci&oacuten de mouse multi-bot&oacuten:</b> Esta opci&oacuten es descripta m&aacutes arriba bajo <a HREF="#usage">Modo de Uso</a>. Cuando esta emulaci&oacuten est&aacute activa los modificadores seleccionados deben ser presionados cuando se hace click para emular el bot&oacuten 2 o el bot&oacuten 3.</li>
</ul>
<h3>Inicio</h3>
<ul>
    <li><b>Modo inicial:</b> Si el usuario no indica si desea utilizar la Pantalla Completa o el modo Rootless, el modo especificado aqu&iacute ser&aacute el usado.</li>
    <li><b>Mostrar panel de selecci&oacuten al inicio:</b> Por defecto, un di&aacutelogo permite al usuario elegir entre Pantalla Completa o Rootless al inicio. Si esta opci&oacuten esta desactivada, XDarwin arrancar&aacute utilizando el modo por defecto sin consultar al usuario.</li>
    <li><b>N&uacutemero de display X11:</b> X11 permite que existan m&uacuteltiples pantallas manejadas por servidores X11 separados funcionando en una misma computadora. El usuario puede especificar aqui un n&uacutemero entero para indicar el n&uacutemero de pantalla (display) que XDarwin utilizar&aacute si m&aacutes de un servidor X funciona en forma simult&aacutenea.</li>
    <li><b>Habilitar soporte Xinerama para m&uacuteltipes monitores:</b> XDarwin suporta m&uacuteltiple monitores con Xinerama, que maneja todos los monitores como si fueran parte de una gran pantalla rectangular. Puede deshabilitar Xinerama con esta opci&oacuten, pero XDarwin no maneja m&uacuteltiples monitores en forma correcta sin esta opci&oacuten habilitada. Si tiene solo un monitor, Xinerama es autom&aacuteticamente deshabilitado.</li>
    <li><b>Archivo de mapa de teclado:</b> Un archivo de mapa de teclas es le&iacutedo al inicio y es traducido a un keymap X11 (un archivo est&aacutendard de X11 para especificar la funci&oacuten de cada tecla). Estos archivos, disponibles para una amplia variedad de lenguajes, pueden encontrarse en  <code>/System/Library/Keyboards</code>.</li>
    <li><b>Al iniciar clientes X11:</b> Cuando XDarwin arranca desde el Finder, &eacuteste ejecutar&aacute <code>xinit</code> para a su vez arrancar el window manager y otros clientes. (Vea en "<code>man xinit</code>" para mayor informaci&oacuten). Antes de ejecutar <code>xinit</code> XDarwin agregar&aacute los directorios especificados al path del usuario. Por defecto, solo <code>/usr/X11R6/bin</code> es agregado. Otros directorios adicionales puede agregarse separados por dos puntos (:). Los clientes X son ejecutados con el int&eacuterprete de comandos del usuario, por lo que los archivos de inicio de &eacuteste son le&iacutedos. Si se desea, un int&eacuterprete de comandos diferente puede ser especificado.</li>
</ul>
<h3>Pantalla Completa</h3>
<ul>
    <li><b>Bot&oacuten para definir combinaci&oacuten de teclas:</b> Haga click en este bot&oacuten y luego presione cualquier combinaci&oacuten de modificadores seguidos de una tecla convencional para definir que combinaci&oacuten usar&aacute para intercambiar entre X11 y Aqua.</li>
    <li><b>Click en el &iacutecono del Dock cambia a X11:</b> Habilite esta opci&oacuten para volver a X11 al hacer click en &iacutecono de XDarwin en el Dock. En algunas versiones de Mac OS X, al volver haciendo click en el Dock puede causar al desaparci&oacuten del cursor al volver a Aqua.</li>
    <li><b>Mostrar ayuda al inicio:</b> Esta opci&oacuten habilitada har&aacute que una pantalla inicial de introducci&oacuten aparezca cuando XDarwin es arrancado en modo Pantalla Completa.</li>
    <li><b>Profundidad de color (bits):</b> En modo Pantalla Completa, el display X11 puede utilizar una profundidad de color diferente de la utilizada por Aqua. Si se especifica "Actual", la misma profundidad de color que Aqua utiliza ser&aacute adoptada por X11. Al contrario, puede especificar 8, 15, o 24 bits.</li>
</ul>

<h2><a NAME="license">Licencia</a></h2>
La licencia principal de XDarwin es basada en la Licencia MIT X11 tradicional, que no impone condiciones a la modificaci&oacuten o redistribuci&oacuten del c&oacutedigo fuente o de archivos binarios m&aacutes all&aacute de requerir que los mensajes de Licencia y Copyright se mantengan intactos. Para mayor informaci&oacuten y para mensajes adicionales de Licencia y Copyright que cubren algunas secciones del c&oacutedigo fuente, por favor consulte the source code.
<H3><A NAME="3"></A>Licencia del X Consortium</H3>
<p>Copyright (C) 1996 X Consortium</p>
<p>Se otorga aqui permiso, libre de costo, a toda persona que obtenga una copia de este Software y los archivos de documentaci&oacuten asociados (el "Software"),
para utilizar el Software sin restricciones, incluyendo sin l&iacutemites los derechos de usar, copiar, modificar, integrar con otros productos, publicar, distribuir, sub-licenciar y/o comercializar copias del Software, y de permitir a las personas que lo reciben para hacer lo propio, sujeto a las siguientes condiciones:</p>
<p>El mensaje de Copyright indicado m&aacutes arriba y este permiso ser&aacute inclu&iacutedo en todas las copias o porciones sustanciales del Software.</p>
<p>THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT
SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.</p>
<p>Excepto lo indicado en este mensaje, el nombre del X Consortium no ser&aacute utilizado en propaganda o como medio de promoci&oacuten para la venta, utilizaci&oacuten u otros manejos de este Software sin previa autorizaci&oacuten escrita del X Consortium.</p>
<p>X Window System es una marca registrada de X Consortium, Inc.</p>
<H3><A NAME="3"></A>X Consortium License (English)</H3>
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
