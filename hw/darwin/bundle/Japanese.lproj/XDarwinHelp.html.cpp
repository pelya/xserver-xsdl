<!-- $XFree86: xc/programs/Xserver/hw/darwin/bundle/Japanese.lproj/XDarwinHelp.html.cpp,v 1.5 2002/05/04 01:25:40 torrey Exp $ -->

#include "xf86Version.h"
#ifndef PRE_RELEASE
#define PRE_RELEASE XF86_VERSION_SNAP
#endif

<html>
<head>
<META http-equiv="Content-Type" content="text/html; charset=EUC-JP">
<title>
XFree86 for Mac OS X</title></head>
<body>
<center>
    <h1>XFree86 on Darwin and Mac OS X</h1>
    XFree86 XF86_VERSION<br>
    Release Date: XF86_REL_DATE
</center>
<h2>目次</h2>
<ol>
    <li><A HREF="#notice">注意事項</A></li>
    <li><A HREF="#usage">使用法</A></li>
    <li><A HREF="#path">パスの設定</A></li>
    <li><A HREF="#prefs">環境設定</A></li>
    <li><A HREF="#license">ライセンス</A></li>
</ol>
<center>
        <h2><a NAME="notice">注意事項</a></h2>
</center>
<blockquote>
#if PRE_RELEASE
これは，XFree86 のプレリリースバージョンであり，いかなる場合においてもサポートされません。 
バグの報告やパッチが SourceForge の <A HREF="http://sourceforge.net/projects/xonx/">XonX プロジェクトページ</A>に提出されているかもしれません。
プレリリースバージョンのバグを報告する前に，<A HREF="http://sourceforge.net/projects/xonx/">XonX</A> プロジェクトページまたは <A HREF="http://www.XFree86.Org/cvs">XFree86 CVS リポジトリ</A>で最新版のチェックをして下さい。
#else
もし，サーバーが 6 -12 ヶ月以上前のものか，またはあなたのハードウェアが上記の日付よりも新しいものならば，問題を報告する前により新しいバージョンを探してみてください。
バグの報告やパッチが SourceForge の <A HREF="http://sourceforge.net/projects/xonx/">XonX プロジェクトページ</A>に提出されているかもしれません。
#endif
</blockquote>
<blockquote>
本ソフトウェアは，<A HREF="#license">MIT X11/X Consortium License</A> の条件に基づき，無保証で，「そのまま」の形で供給されます。
ご使用になる前に，<A HREF="#license">ライセンス条件</A>をお読み下さい。
</blockquote>

<h2><a NAME="usage">使用法</a></h2>
<p>XFree86 は，<a HREF="http://www.XFree86.Org/">XFree86 Project, Inc.</a>によって作成された，再配布可能なオープンソースの <a HREF="http://www.x.org/">X Window System</a> の実装です。
XFree86 によって提供される Darwin と Mac OS X のための X Window サーバーを XDarwin と呼びます。
XDarwin は，Mac OS X 上でフルスクリーンモードまたはルートレスモードで動作します。</p>

<p>フルスクリーンモードでは，X Window System がアクティブな時，それは全画面を占有します。
あなたは，Command-Option-A キーを押すことによって Mac OS X デスクトップへ切り替えることができます。このキーの組み合わせは，環境設定で変更可能です。
Mac OS X デスクトップから X Window System へ切り替える場合は，ドックに表示された XDarwin アイコンをクリックして下さい。
（環境設定で，フローティング・ウィンドウに表示された XDarwin アイコンをクリックするように変更することができます。）</p>

<p>ルートレスモードでは，X Window System と Aqua は画面を共有します。
X11 が表示するルートウィンドウは画面のサイズであり，他の全てのウィンドウを含んでいます。
Aqua がデスクトップの背景を制御するので，X11 のルートウィンドウはルートレスモードでは表示されません。</p>

<h3>複数ボタンマウスのエミュレーション</h3>
<p>多くの X11 アプリケーションは，3 ボタンマウスを必要とします。
あなたはマウスボタンのクリックと同時にいくつかの修飾キーを押すことによって，一つのボタンで 3 ボタンマウスをエミュレーションすることができます。
これは，環境設定の「一般設定」の「複数ボタンマウスのエミュレーション」セクションで設定します。
デフォルトでは，エミュレーションは有効で，コマンドキーを押しながらマウスボタンをクリックすることは第 2 マウスボタンのクリックに相当します。
オプションキーを押しながらクリックすることは第 3 マウスボタンのクリックに相当します。
あなたは，環境設定でボタン 2 と 3 をエミュレートするために使用する修飾キーの組合せを変更することができます。
注：修飾キーを xmodmap で他のキーに割り当てている場合でも，複数ボタンマウスのエミュレーションでは本来のコマンドキーやオプションキーを使わなければなりません。</p>

<h2><a NAME="path">パスの設定</a></h2>
<p>パスは， 実行可能なコマンドを検索するディレクトリのリストです。
X11 バイナリは，<code>/usr/X11R6/bin</code> に置かれます。あなたはそれをパスに加える必要があります。
XDarwin は，これをデフォルトで行います。また，あなたがコマンドライン・アプリケーションをインストールした追加のディレクトリを加えることができます。</p>

<p>経験豊かなユーザーは，すでに自らのシェルのために初期化ファイルを使用してパスを設定しているでしょう。
この場合，あなたは環境設定で XDarwin があなたのパスを変更しないように設定することができます。
XDarwin は，ユーザーのデフォルトのログインシェルで最初の X11 クライアントを開始します。
（環境設定で代わりのシェルを指定することができます。）
パスを設定する方法は，あなたが使用しているシェルに依存します。
これは，シェルのマニュアルページドキュメントに記載されています。

<p>また，あなたはドキュメントを探している時，XFree86 のマニュアルページを検索されるページのリストに追加したいと思うかもしれません。
X11 のマニュアルページは <code>/usr/X11R6/man</code> に置かれます。そして <code>MANPATH</code> 環境変数は検索するディレクトリのリストを含んでいます。</p>

<h2><a NAME="prefs">環境設定</a></h2>
<p>「XDarwin」メニューの「環境設定...」メニュー項目からアクセスできる環境設定パネルで，いくつかのオプションを設定することができます。
「起動オプション」の内容は，XDarwin を再起動するまで有効となりません。
他の全てのオプションの内容は，直ちに有効となります。
以下，それぞれのオプションについて説明します:</p>

<h3>一般設定</h3>
<ul>
    <li><b>X11 でシステムのビープ音を使用する:</b> オンの場合，Mac OS X のビープ音が X11 のベルとして使用されます。オフの場合（デフォルト），シンプル トーンが使われます。</li>
    <li><b>X11 のマウスアクセラレーションを有効にする:</b> 標準的な X Window System の実装では，ウィンドウマネージャーはマウスの加速度を変更することができます。
    マウスの加速度に Mac OS X のシステム環境設定と X ウィンドウマネージャーが異なる値を設定した場合，これは混乱を招きます。
    この問題を避けるため，デフォルトでは X11 のマウスアクセラレーションを有効としません。</li>
    <li><b>複数ボタンマウスのエミュレーション:</b> <a HREF="#usage">使用法</a>を参照して下さい。オンの場合，マウスボタンが第 2 または第 3 のマウスボタンをエミュレートする時に，選択した修飾キーを同時に押します。</li>
</ul>

<h3>起動オプション</h3>
<ul>
    <li><b>画面モード:</b> ユーザーがフルスクリーンモードまたはルートレスモードのどちらを使用するかを指定しない場合，ここで指定されたモードが使われます。</li>
    <li><b>起動時にモード選択パネルを表示する:</b> デフォルトでは，XDarwin の起動時にユーザーがフルスクリーンモードまたはルートレスモードのどちらを使用するかを選択するパネルを表示します。このオプションがオフの場合，画面モードで指定したモードで起動します。</li>
    <li><b>X11 ディスプレイ番号:</b> X11は，一つのコンピュータ上で別々の X サーバーが管理する複数のディスプレイが存在することを許します。複数の X サーバーが同時に実行している時，XDarwin が使用するディスプレイの番号を指定することができます。</li>
    <li><b>Xinerama マルチモニタサポートを有効にする:</b> XDarwin は，Xinerama マルチモニタをサポートします。それは全てのモニタを一つの大きな画面の一部とみなします。あなたはこのオプションで Xinerama を無効にすることができます。ただし，現在 XDarwin はそれ無しで正しく複数のモニタを扱うことができません。もし，あなたが一つのモニタを使うだけならば，Xinerama は自動的に無効となります。</li>
    <li><b>キーマッピングファイル:</b> キーマッピングファイルは起動時に読み込まれ，X11 キーマップに変換されます。他言語に対応したキーマッピングファイルは <code>/System/Library/Keyboards</code> にあります。（訳注：キーマッピングで Japanese を選択すると，一部のキーが効かない等の不具合が発生することがあります。この場合は USA を選択した上で ~/.Xmodmap を適用して下さい。）</li>
    <li><b>最初の X11 クライアントの起動:</b> XDarwin が Finderから起動する時，X ウィンドウマネージャーと X クライアントの起動は <code>xinit</code> を実行します。（詳細は "<code>man xinit</code>" を参照して下さい。）XDarwin は <code>xinit</code> を実行する前に，指定されたディレクトリをユーザーのパスに追加します。デフォルトでは <code>/usr/X11R6/bin</code> だけを追加します。他のディレクトリを追加したい場合は，コロンで区切って指定します。ユーザーのシェル初期化ファイルを読み込むために，X クライアントはユーザーのデフォルトログインシェルで起動されます。必要であれば，代わりのシェルを指定することができます。</li>
</ul>

<h3>フルスクリーン</h3>
<ul>
    <li><b>キー設定ボタン:</b> X11 と Aqua を切り替えるために使用するボタンの組み合わせを指定します。
    ボタンをクリックして，任意の数の修飾キーに続いて通常のキーを押します。</li>
    <li><b>ドックのアイコンのクリックで X11 に戻る:</b> オンの場合，ドックに表示された XDarwin アイコンのクリックで X11 への切り換えが可能となります。Mac OS X のいくつかのバージョンでは，ドックのアイコンのクリックで Aqua に戻った時，カーソルが消失することがあります。</li>
    <li><b>起動時にヘルプを表示する:</b> XDarwin がフルスクリーンモードで起動する時，スプラッシュスクリーンを表示します。</li>
    <li><b>色深度:</b> フルスクリーンモードでは，X11 ディスプレイが Aqua と異なる色深度を使うことができます。「変更なし」が指定された場合，XDarwin は Aqua によって使用される色深度を使います。これ以外に 8，15 または24 ビットを指定することができます。</li>
</ul>

<h2>
<a NAME="license">ライセンス</a>
</h2>
XFree86 Project は，自由に再配布可能なバイナリとソースコードを提供することにコミットしています。
私たちが使用する主なライセンスは，伝統的な MIT X11/X Consortium License に基づくものです。
そして，それは修正または再配布されるソースコードまたはバイナリに，その Copyright/ライセンス告示がそのまま残されることを要求する以外の条件を強制しません。
より多くの情報と，コードの一部をカバーする追加の Copyright/ライセンス告示のために，<A HREF="http://www.xfree86.org/legal/licence.html">XFree86 の License ページ</A>を参照して下さい。
<H3>
<A NAME="3"></A>
X  Consortium License</H3>
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
