<!-- $XFree86: xc/programs/Xserver/hw/darwin/bundle/English.lproj/XDarwinHelp.html.cpp,v 1.2 2001/11/04 07:02:28 torrey Exp $ -->

<html>
<head><META HTTP-EQUIV="content-type" CONTENT="text/html; charset=iso-8859-1">
<title>XDarwin Help</title>
</head>
<body>
<center>
    <h1>XDarwin X Server pour Mac OS X</h1>
    X_VENDOR_NAME X_VERSION<br>
    Date : X_REL_DATE
</center>
<h2>Sommaire</h2>
<ol>
    <li><A HREF="#notice">Avertissement</A></li>
    <li><A HREF="#usage">Utilisation</A></li>
    <li><A HREF="#path">Chemins d'accès</A></li>
    <li><A HREF="#prefs">Préférences</A></li>
    <li><A HREF="#license">Licence</A></li>
</ol>
<center>
    <h2><a NAME="notice">Avertissement</a></h2>
</center>
<blockquote>
#if PRE_RELEASE
Ceci est une pré-version de XDarwin et ne fait par conséquent l'objet d'aucun support client. Les bogues peuvent être signalés et des patches peuvent être soumis sur la 
<A HREF="http://sourceforge.net/projects/xonx/">page du projet XonX</A> chez SourceForge. Veuillez prendre connaissance de la dernière version sur <A HREF="http://sourceforge.net/projects/xonx/">XonX</A> ou le X_VENDOR_LINK avant de signaler un bogue d'une pré-version.
#else
Si le serveur date de plus de 6-12 mois ou si votre matériel est plus récent que la date indiquée ci-dessus, veuillez vous procurer une version plus récente avant de signaler toute anomalie. Les bogues peuvent être signalés et des patches peuvent être soumis sur la <A HREF="http://sourceforge.net/projects/xonx/">page du projet XonX</A> chez SourceForge.
#endif
</blockquote>
<blockquote>
Ce logiciel est distribué sous la 
<A HREF="#license">Licence du Consortium X/X11 du MIT</A> et est fourni TEL QUEL, sans garanties. Veuillez prendre connaissance de la <A HREF="#license">Licence</A> avant toute utilisation.</blockquote>

<h2><a NAME="usage">Utilisation</a></h2>
<p>XDarwin est une X server libre et distribuable sans contrainte du <a HREF
="http://www.x.org/">X Window System</a>. This version of XDarwin was produced by the X_VENDOR_LINK. XDarwin fonctionne sous Mac OS X en mode « rootless » ou plein écran.</p>
<p>Lorsque le système X window est actif en mode plein écran, il prend en charge la totalité de l'écran. Il est possible de revenir sur le bureau de Mac OS X en appuyant sur Commande-Option-A. Cette combinaison de touches peut être modifiée dans les préférences. Pour revenir dans X window, cliquer sur l'icône de XDarwin dans le Dock de Mac OS X.  (Un réglage des préférences permet d'effectuer cette opération en cliquant dans une fenêtre flottante au lieu de l'icône du Dock)</p>
<p>En mode « rootless », X window system et Aqua utilisent le même affichage. La fenêtre-mère de l'affichage X11 est de la taille de l'écran et contient toutes les autre fenêtres. En mode « rootless » cette fenêtre-mère n'est pas affichée car Aqua gère le fond d'écran.</p>
<h3>Émulation de souris à plusieurs boutons</h3>
<p>Le fonctionnement de la plupart des applications X11 repose sur l'utilisation d'une souris à 3 boutons. Il est possible d'émuler une souris à 3 boutons avec un seul bouton en appuyant sur des touches de modification. Ceci est réglé dans la section "Émulation de souris à plusieurs boutons" de l'onglet "Général" des préférences. L'émulation est activée par défaut. Dans ce cas, cliquer en appuyant simultanément sur la touche "commande" simulera le bouton du milieu. Cliquer en appuyant simultanément sur la touche "option" simulera le bouton de droite. Les préférences permettent de régler n'importe quelle combinaison de touches de modification pour émuler les boutons du milieu et de droite. Notez que même si les touches de modifications sont mises en correspondance avec d'autres touches par xmodmap, ce sont les touches originelles spécifiées dans les préférences qui assureront l'émulation d'une souris à plusieurs boutons.

<h2><a NAME="path">Réglage du chemin d'accès</a></h2>
<p>Le chemin d'accès est une liste de répertoires utilisés pour la recherche d'exécutables. Les commandes X11 sont situées dans <code>/usr/X11R6/bin</code>, qui doit être ajouté à votre chemin d'accès. XDarwin fait cela par défaut, et peut également ajouter d'autres répertoires dans lesquels vous auriez installé d'autre commandes unix.</p>
<p>Les utilisateurs plus expérimentés auront déjà réglé leur chemin d'accès correctement par le biais des fichiers d'initialisation de leur shell. Dans ce cas, il est possible de demander à XDarwin de ne pas modifier le chemin d'accès initial. XDarwin lance les premiers clients X11 dans le shell d'ouverture de session par défaut. (Un shell de remplacement peut être spécifié dans les préférences.) La façon de régler le chemin d'accès dépend du shell utilisé. Ceci est documenté dans les pages "man" du shell.</p>
<p>De plus, il est possible d'ajouter les pages "man" de X11 à la liste des pages recherchées pour la documentation "man". Les pages "man" X11 se trouvent dans <code>/usr/X11R6/man</code>  et la variable d'environnement <code>MANPATH</code> contient la liste des répertoires dans lesquels chercher.</p>


<h2><a NAME="prefs">Préférences</a></h2>
<p>Un certain nombre d'options peuvent être réglées dans les préférences. On accède aux préférences en choisissant "Préférences..." dans le menu "XDarwin". Les options décrites comme options de démarrage ne prendront pas effet avant le redémarrage de XDarwin. Les autres options prennent immédiatement effet. Les différentes options sont détaillées ci-après :</p>
<h3>Général</h3>
<ul>
    <li><b>Utiliser le bip d'alerte Système dans X11 :</b> Cocher cette option pour que le son d'alerte standard de Mac OS X soit utilisé à la place du son d'alerte de X11. L'option n'est pas cochée ar défaut. Dans ce cas, un simple signal sonore est utilisé.</li>
    <li><b>Autoriser X11 à changer la vitesse de la souris :</b> Dans une implémentation classique du sytème X window, le gestionnaire de fenêtres peut modifier la vitesse de la souris. Cela peut s'avérer déroutant puisque le réglage de la vitesse de la souris peut être différent dans les préférences de Mac OS X et dans le gestionnaire X window. Par défaut, X11 n'est pas autorisé à changer la vitesse de la souris.</li>
    <li><b>Émulation de souris à plusieurs boutons :</b> Ceci est décrit ci-dessus à la rubrique <a HREF="#usage">Usage</a>. Lorsque l'émulation est activée, il suffit d'appuyer simultanément sur les touches modificatrices sélectionnées et sur le bouton de la souris afin d'émuler les boutons du milieu et de droite.</li>
</ul>
<h3>Démarrage</h3>
<ul>
    <li><b>Mode par défaut :</b> Le mode spécifié à cet endroit sera utilisé si l'utilisateur ne l'indique pas au démarrage.</li>
    <li><b>Choix du mode d'affichage au démarrage</b> Par défaut, une fenêtre de dialogue est affichée au démarrage de XDarwin pour permettre à l'utilisateur de choisir entre le mode plein écran et le mode « rootless ». Si cette option est désactivée, le mode par défaut sera automatiquement utilisé.</li>
    <li><b>Numéro d'affichage (Display)</b> X11 offre la possibilité de plusieurs serveurs X sur un ordinateur. L'utilisateur doit spécifier ici le numéro d'affichage utilisé par XDarwin dans le cas où plusieurs serveurs X seraient en service simultanément.</li>
    <li><b>Autoriser la prise en charge Xinerama de plusieurs moniteurs :</b> XDarwin peut être utilisé avec plusieurs moniteur avec Xinerama, qui considère les différents moniteurs comme des parties d'un écran rectugulaire plus grand. Cette option permet de désactiver Xinerama mais XDarwin ne prend alors pour l'instant pas correctement en charge l'affichage sur plusieurs écrans. Si il n'y a qu'un seul moniteur, Xinerama est automatiquement désactivé.</li>
    <li><b>Fichier clavier :</b> Un fichier de correspondance de clavier est lu au démarrage puis transformé en un fihcier de correspondance clavier pour X11. Les fichiers de correspondance clavier, disponibles pour de nombreuses langues, se trouvent dans <code>/System/Library/Keyboards</code>.</li>
    <li><b>Démarrage des premiers clients X11 :</b> Lorsque XDarwin est démarré à partir du Finder, il lance <code>xinit</code> qui lance à son tour le gestionnaire X window ainsi que d'autres clients X. (Voir "<code>man xinit</code>" pour plus d'informations.) Avant de lancer <code>xinit</code>, XDarwin ajoute les répertoires ainsi spécifiés au chemin d'accès de l'utilisateur. Par défaut, seul <code>/usr/X11R6/bin</code> est ajouté. Il est possible d'ajouter d'autres répertoires en les séparants à l'aide de deux points (<code>:</code>). Les clients X sont démarrés à partir du shell par défaut de l'utilisateur. Ainsi, le fichier d'initialisation de shell de l'utilisateur est lu. Un autre shell peut éventuellement être spécifié.</li>
</ul>
<h3>Plein écran</h3>
<ul>
    <li><b>Combinaison de touches :</b> Appuyer sur ce bouton, puis appuyer sur une ou plusieurs touches modificatrices suivies d'une touche ordinaire. Cette combinaison de touche servira à commuter entre Aqua et X11.</li>
    <li><b>Basculer dans X11 en cliquant sur l'icône du Dock :</b> Cette option permet de passer dans X11 en cliquant dans l'icône de XDarwin dans le Dock. Sur certaines versions de Mac OS X, la commutation en utilisant le Dock peut faire disparaître le curseur lors du retour dans Aqua.</li>
    <li><b>Afficher l'aide du mode plein écran au démarrage :</b> Permet l'affichage d'une fenêtre d'introduction lorsque XDarwin est démarré en mode plein écran.</li>
    <li><b>Profondeur de couleur :</b> En mode plein écran, l'affichage X11 peut utiliser une autre profondeur de couleur que celle employée par Aqua. Si "Actuelle" est choisi, XDarwin utilisera la même profondeur de couleur qu'Aqua. Les autres choix sont 8 (256 couleurs), 15 (milliers de couleurs) et 24 bits (millions de couleurs). </li>
</ul>

<h2><a NAME="license">Licence</a></h2>
The main license for XDarwin is one based on the traditional MIT X11 / X Consortium License, which does not impose any conditions on modification or redistribution of source code or binaries other than requiring that copyright/license notices are left intact. For more information and additional copyright/licensing notices covering some sections of the code, please refer to the source code.
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

