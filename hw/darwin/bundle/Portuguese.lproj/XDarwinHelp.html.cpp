<!-- $XFree86: xc/programs/Xserver/hw/darwin/bundle/English.lproj/XDarwinHelp.html.cpp,v 1.2 2001/11/04 07:02:28 torrey Exp $ --><html><body>

<head>
<title>XDarwin Help</title>
</head>

<center>
    
  <h1>XDarwin X Server para Mac OS X</h1>
    X_VENDOR_NAME X_VERSION<br>
    Release Date: X_REL_DATE
</center>
<h2>&Iacute;ndice</h2>
<ol>
    <li><A HREF="#notice">Notas importantes</A></li>
    <li><A HREF="#usage">Uso</A></li>
    <li><A HREF="#path">Ajustando seu Path</A></li>
    
  <li><A HREF="#prefs">Prefer&ecirc;ncias do usu&aacute;rio</A></li>
  <li><A HREF="#license">Licen&ccedil;a</A></li>
</ol>
<center>
    <h2><a NAME="notice">Notas importantes</a></h2>
</center>
<blockquote>
#if PRE_RELEASE
  Essa &eacute; uma vers&atilde;o pr&eacute;-lancamento 
  do XDarwin, e ela n&atilde;o &eacute; suportada de nenhuma forma. Bugs podem 
  ser reportados e corre&ccedil;&otilde;es podem ser enviadas para <A HREF="http://sourceforge.net/projects/xonx/">P&aacute;gina 
  do projeto XonX</A> no SourceForge. Antes de informar bugs em vers&otilde;es 
  pr&eacute;-lancamento, por favor verifique a þltima vers&atilde;o em <A HREF="http://sourceforge.net/projects/xonx/">XonX</A> 
  or X_VENDOR_LINK. 
#else
Se o servidor &eacute; mais velho que 6-12 semanas, ou seu hardware &eacute; 
  mais novo que a data acima, procure por uma nova vers&atilde;o antes de informar 
  problemas. Bugs podem ser reportados e corre&ccedil;&otilde;es podem ser enviadas 
  para a <A HREF="http://sourceforge.net/projects/xonx/">P&aacute;gina do projeto 
  XonX</A> na SourceForge.
#endif
</blockquote>
<blockquote> Este software &eacute; distribu&iacute;do sob os termos da <a href="#license">licen&ccedil;a 
  MIT X11 / X Consortium</a> e &eacute; provido, sem nenhuma garantia. Por favor 
  leia a <a href="#license">Licen&ccedil;a</a> antes de come&ccedil;ar a usar 
  o programa.</blockquote>

<h2><a NAME="usage">Uso</a></h2>
<p>O XDarwin &eacute; uma X server &quot;open-source&quot; livremente 
  redistribu&iacute;da do <a HREF
="http://www.x.org/">Sistema X Window</a>. This version of XDarwin was produced by the X_VENDOR_LINK.
  XDarwin roda sobre Mac OS X no modo Tela Cheia ou no modo Compartilhado.</p>
<p>No modo Tela Cheia, quando o sistema X window est&aacute; ativo, ele ocupa 
  a tela toda. Voc&ecirc; pode voltar ao desktop do Mac OS X clicando Command-Option-A. 
  Essa combina&ccedil;&atilde;o de teclas pode ser mudada nas prefer&ecirc;ncias. 
  Pelo desktop Mac OS X, clique no &iacute;cone XDarwin no Dock para voltar ao 
  sistema X window. (Voc&ecirc; pode mudar esse comportamento nas prefer&ecirc;ncias 
  da&iacute; voc&ecirc; dever&aacute; clicar no &iacute;cone XDarwin na janela 
  flutuante que aparecer&aacute;.)</p>
<p>No modo Compartilhado, o sistema X window e Aqua dividem a mesma tela. A janela 
  raiz da tela X11 est&aacute; do tamanho da tela (monitor) e cont&eacute;m todas 
  as outras janelas. A janela raiz do X11 no modo compartilhado n&atilde;o &eacute; 
  mostrada pois o Aqua controla o fundo de tela.</p>
<h3>Emula&ccedil;&atilde;o de Mouse Multi-Bot&otilde;es</h3>
<p>Muitas aplica&ccedil;&otilde;es X11 insistem em usar um mouse de 3 bot&otilde;es. 
  Voc&ecirc; pode emular um mouse de 3 bot&otilde;es com um simples bot&atilde;o, 
  mantendo pressionando teclas modificadoras enquanto voc&ecirc; clica no bot&atilde;o 
  do mouse. Isto &eacute; controlado pela configura&ccedil;&atilde;o da &quot;Emula&ccedil;&atilde;o 
  de Mouse Multi-Bot&otilde;es&quot; da prefer&ecirc;ncia &quot;Geral&quot;. Por 
  padr&atilde;o, a emula&ccedil;&atilde;o est&aacute; habilitada e mantendo pressionada 
  a tecla Command e clicando no bot&atilde;o do mouse ele simular&aacute; o clique 
  no segundo bot&atilde;o do mouse. Mantendo pressionada a tecla Option e clicando 
  no bot&atilde;o do mouse ele simular&aacute; o terceiro bot&atilde;o. Voc&ecirc; 
  pode mudar a combina&ccedil;&atilde;o de teclas modificadoras para emular os 
  bot&otilde;es dois e tr&ecirc;s nas prefer&ecirc;ncias. Nota, se a tecla modificadora 
  foi mapeada para alguma outra tecla no xmodmap, voc&ecirc; ainda ter&aacute; 
  que usar a tecla atual especificada nas prefer&ecirc;ncias para a emula&ccedil;&atilde;o 
  do mouse multi-bot&otilde;es.</p>
<h2><a NAME="path">Ajustando seu Path</a></h2>
<p>Seu path &eacute; a lista de diret&oacute;rios a serem procurados por arquivos 
  execut&aacute;veis. O comando X11 est&aacute; localizado em <code>/usr/X11R6/bin</code>, 
  que precisa ser adicionado ao seu path. XDarwin faz isso para voc&ecirc; por 
  padr&atilde;o e pode-se tamb&eacute;m adicionar diret&oacute;rios onde voc&ecirc; 
  instalou aplica&ccedil;&otilde;es de linha de comando.</p>
<p>Usu&aacute;rios experientes j&aacute; ter&atilde;o configurado corretamente 
  seu path usando arquivos de inicializa&ccedil;&atilde;o de seu shell. Neste 
  caso, voc&ecirc; pode informar o XDarwin para n&atilde;o modificar seu path 
  nas prefer&ecirc;ncias. O XDarwin inicia o cliente inicial X11 no shell padr&atilde;o 
  do usu&aacute;rio corrente. (Um shell alternativo pode ser tamb&eacute;m expecificado 
  nas prefer&ecirc;ncias.) O modo para ajustar o path depende do shell que voc&ecirc; 
  est&aacute; usando. Isto &eacute; descrito na man page do seu shell.</p>
<p>Voc&ecirc; pode tamb&eacute;m querer adicionar as man pages do X11 para 
  a lista de p&aacute;ginas a serem procuradas quando voc&ecirc; est&aacute; procurando 
  por documenta&ccedil;&atilde;o. As man pages do X11 est&atilde;o localizadas 
  em <code>/usr/X11R6/man</code> e a vari&aacute;vel de ambiente <code>MANPATH</code> 
  cont&eacute;m a lista de diret&oacute;rios a buscar.</p>
<h2><a NAME="prefs">Prefer&ecirc;ncias do Usu&aacute;rio</a></h2>
<p>V&aacute;rias op&ccedil;&otilde;es podem ser ajustadas nas prefer&ecirc;ncias 
  do usu&aacute;rio, acess&iacute;vel pelo item &quot;Prefer&ecirc;ncias...&quot; 
  no menu &quot;XDarwin&quot;. As op&ccedil;&otilde;es listadas como op&ccedil;&otilde;es 
  de inicializa&ccedil;&atilde;o, n&atilde;o ter&atilde;o efeito at&eacute; voc&ecirc; 
  reiniciar o XDarwin. Todas as outras op&ccedil;&otilde;es ter&atilde;o efeito 
  imediatamente. V&aacute;rias das op&ccedil;&otilde;es est&atilde;o descritas 
  abaixo:</p>
<h3>Geral</h3>
<ul>
  <li><b>Usar o Beep do Sistema para o X11: </b>Quando habilitado som de alerta 
    padr&atilde;o do Mac OS X ser&aacute; usado como alerta no X11. Quando desabilitado 
    (padr&atilde;o) um tom simples ser&aacute; usado.</li>
  <li><b>Permitir o X11 mudar a acelera&ccedil;&atilde;o do mouse: </b>Por implementa&ccedil;&atilde;o 
    padr&atilde;o no sistema X window, o gerenciador de janelas pode mudar a acelera&ccedil;&atilde;o 
    do mouse. Isso pode gerar uma confus&atilde;o pois a acelera&ccedil;&atilde;o 
    do mouse pode ser ajustada diferentemente nas prefer&ecirc;ncias do Mac OS 
    X e nas prefer&ecirc;ncias do X window. Por padr&atilde;o, o X11 n&atilde;o 
    est&aacute; habilitado a mudar a acelera&ccedil;&atilde;o do mouse para evitar 
    este problema.</li>
  <li><b>Emula&ccedil;&atilde;o de Mouse de Multi-Bot&otilde;es: </b>Esta op&ccedil;&atilde;o 
    est&aacute; escrita acima em <a href="#usage">Uso</a>. Quando a emula&ccedil;&atilde;o 
    est&aacute; habilitada as teclas modificadoras selecionadas tem que estar 
    pressionadas quando o bot&atilde;o do mouse for pressionado, para emular o 
    segundo e terceiro bot&otilde;es.</li>
</ul>
<h3>Inicial</h3>
<ul>
  <li><b>Modo Padr&atilde;o: </b>Se o usu&aacute;rio n&atilde;o indicar qual modo 
    de exibi&ccedil;&atilde;o quer usar (Tela Cheia ou Compartilhado) o modo especificado 
    aqui ser&aacute; usado .</li>
  <li><b>Mostrar o painel de escolha na inicializa&ccedil;&atilde;o: </b> Por 
    padr&atilde;o, uma painel &eacute; mostrado quando o XDarwin &eacute; 
    iniciado para permitir que o usu&aacute;rio escolha ente o modo tela cheia 
    ou modo compartilhado. Se esta op&ccedil;&atilde;o estiver desligada, o modo 
    padr&atilde;o ser&aacute; inicializado automaticamente.</li>
  <li><b>N&uacute;mero do Monitor X11: </b>O X11 permite ser administrado em multiplos 
    monitores por servidores X separados num mesmo computador. O usu&aacute;rio 
    pode indicar o n&uacute;mero do monitor para o XDarwin usar se mais de um 
    servidor X se estiver rodando simultaneamente.</li>
  <li><b>Habilitar suporte a m&uacute;ltiplos monitores pelo Xinerama: </b>o XDarwin 
    suporta m&uacute;ltiplos monitores com o Xinerama, que trata todos os monitores 
    como parte de uma grande e retangular tela. Voc&ecirc; pode desabilitar o 
    Xinerama com est&aacute; op&ccedil;&atilde;o, mas normalmente o XDarwin n&atilde;o 
    controla m&uacute;ltiplos monitores corretamente sem est&aacute; op&ccedil;&atilde;o. 
    Se voc&ecirc; s&oacute; tiver um monotor, Xinerama &eacute; automaticamente 
    desabilitado. </li>
  <li><b>Arquivo de Mapa de Teclado: </b> O mapa de teclado &eacute; lido na inicializa&ccedil;&atilde;o 
    e traduzido para um mapa de teclado X11. Arquivos de mapa de teclado, est&atilde;o 
    dispon&iacute;veis numa grande variedade de l&iacute;nguas e s&atilde;o encontradas 
    em <code>/System/Library/Keyboards</code>.</li>
  <li><b>Iniciando Clientes X11 primeiro: </b>Quando o XDrawin &eacute; inicializado 
    pelo Finder, ele ir&aacute; rodar o <code>xinit</code> para abrir o controlador 
    X window e outros clientes X. (Veja o manual "<code>man xinit</code>" para 
    mais informa&ccedil;&otilde;es.) Antes do XDarwin rodar o <code>xinit</code> 
    ele ir&aacute; adicionar espec&iacute;ficos diret&oacute;rios a seu path. 
    Por padr&atilde;o somente o <code>/usr/X11R6/bin</code> &eacute; adicionado. 
    separado por um ponto-e-v&iacute;rgula. Os clientes X s&atilde;o inicializados 
    no shell padr&atilde;o do usu&aacute;rio e os arquivos de inicializa&ccedil;&atilde;o 
    do shell ser&atilde;o lidos. Se desejado, um shell alternativo pode ser especificado.</li>
</ul>
<h3>Tela Cheia</h3>
<ul>
  <li><b>Bot&atilde;o de Combina&ccedil;&atilde;o de Teclas: </b> Clique no bot&atilde;o 
    e pressione qualquer quantidade de teclas modificadoras seguidas por uma tecla 
    padr&atilde;o para modificar a combina&ccedil;&atilde;o quando se quer mudar 
    entre o Aqua e X11.</li>
  <li><b>Clique no &Iacute;cone no Dock para mudar para o X11: </b>Habilitando 
    esta op&ccedil;&atilde;o voc&ecirc; ir&aacute; ativar a mudan&ccedil;a para 
    o X11 clicando no &iacute;cone do XDarwin no Dock. Em algumas vers&otilde;es 
    do Mac OS X, mudando pelo clique no Dock pode causar o desaparecimento do 
    cursor quando retornar ao Aqua.</li>
  <li><b>Mostrar a Ajuda na inicializa&ccedil;&atilde;o: </b>Isto ir&aacute; mostrar 
    uma tela introdut&oacute;ria quando o XDarwin for inicializado no modo Tela 
    Cheia. </li>
  <li><b>Profundidade de Cores em bits: </b> No modo Tela Cheia, a tela do X11 
    pode usar uma profundiadde de cor diferente da usada no Aqua. Se a op&ccedil;&atilde;o 
    &quot;Atual&quot; est&aacute; especificada, a profundidade usada pelo Aqua 
    quando o XDarwin iniciar ser&aacute; a mesma. Al&eacute;m das op&ccedil;&otilde;es 
    8, 15 ou 24 bits que podem ser especificadas.</li>
</ul>

<h2><a NAME="license">Licen&ccedil;a</a></h2>
<p>A licen&ccedil;a 
  principal n&oacute;s por XDarwin baseada na licen&ccedil;a tradicional MIT X11 
  / X Consortium, que n&atilde;o imp&otilde;e nenhuma condi&ccedil;&atilde;o sobre 
  modifica&ccedil;&otilde;es ou redistribui&ccedil;&atilde;o do c&oacute;digo-fonte 
  ou dos bin&aacute;rios desde que o copyright/licen&ccedil;a sejam mantidos intactos. 
  Para mais informa&ccedil;&otilde;es e not&iacute;cias adicionais de copyright/licensing 
  em algumas se&ccedil;&atilde;o do c&oacute;digo, por favor refer to the source code.</p>
<H3><A NAME="3"></A>Licen&ccedil;a do X Consortium</H3>
<p>Copyright (C) 1996 X Consortium</p>
<p>Permiss&otilde;es s&atilde;o em virtude garantidas, livre de mudan&ccedil;as, 
  para qualquer pessoa que possua uma c&oacute;pia deste software e aos arquivos 
  de documenta&ccedil;&atilde;o associada (o &quot;Software&quot;), para lidar 
  com o software sem restri&ccedil;&otilde;es, incluindo limita&ccedil;&otilde;es 
  dos direitos de uso, c&oacute;pia, modifica&ccedil;&atilde;o, inclus&atilde;o, 
  publica&ccedil;&atilde;o, distribui&ccedil;&atilde;o, sub licen&ccedil;a, e/ou 
  venda de c&oacute;pias deste Software, e permitir pessoas to whom o Software 
  &eacute; fornecido para ser desta forma, verifique as seguintes condi&ccedil;&otilde;es:</p>
<p>O nota de copyright abaixo e a permiss&atilde;o dever&atilde;o ser inclu&iacute;das 
  em todas as c&oacute;pias ou substanciais por&ccedil;&otilde;es do Software.</p>
<p>O SOFTWARE 'E PROVIDO &quot;COMO TAL&quot;, SEM GARANTIAS DE NENHUM TIPO, EXPLICITA 
  OU IMPLICITA, INCLUINDO MAS N&Atilde;O LIMITADO NOS AVISOS DE COM&Eacute;RCIO, 
  TAMANHO OU PARA PROPOSTAS PARTICULARES E N&Atilde;O INFRA&Ccedil;&Atilde;O. 
  EM NENHUM ACONTECIMENTO O X CONSORTIUM SER&Aacute; RESPONSAV&Eacute;L POR NENHUMA 
  RECLAMA&Ccedil;&Atilde;O, DANOS OU OUTRAS RESPONSABILIDADES, SE NUMA A&Ccedil;&Atilde;O 
  DE CONTRATO, OU OUTRA COISA, SURGINDO DE, FORA DE OU EM CONEX&Atilde;O COM O 
  SOFTWARE OU O USO OU OUTRO MODO DE LIDAR COM O SOFTWARE.</p>
<p>Exceto o contido nesta nota, o nome do X Consortium n&atilde;o pode ser usado 
  em propagandas ou outra forma de promo&ccedil;&atilde;o de vendas, uso ou outro 
  modo de lidar com este Software sem ter recebido uma autoriza&ccedil;&atilde;o 
  escrita pelo X Consortium.</p>
<p>O Sistema X Window &eacute; marca registrada do X Consortium, Inc.</p>
</body>
</html>

