# アイコン

日本語 | [English](icons_en.md)

この文書では、rcssmonitor のアイコンの作り方と埋め込み方、アイコンを追加・修正する手順を説明する。
仕組みは soccerwindow2 と同じである (計画書 `docs/plan/2026-11-22-01_icon-redesign.md`)。

- [概要](#概要)
- [ファイル](#ファイル)
- [アイコンの追加・修正](#アイコンの追加修正)
- [描き方の決まり](#描き方の決まり)
- [アプリのアイコン](#アプリのアイコン)
- [palette.txt](#palettetxt)
- [generate.sh](#generatesh)
- [Qt のリソースシステム (.qrc)](#qt-のリソースシステム-qrc)
- [テスト](#テスト)

## 概要

- アイコンは 1 つずつ、明るい配色用の色で SVG ファイル (`icons/svg/NAME.svg`) に描く。
- `icons/generate.sh` が Inkscape を使い、SVG ファイルから 8 つの PNG ファイルを作る。
  明るい配色用と暗い配色用の、それぞれ 16×16、24×24、32×32、48×48 ピクセルの画像である。
  暗い配色用の色は `icons/palette.txt` から取る。
  24×24 と 48×48 は、24 ピクセルで描くボタン (Live Mode と再生) のためにある。
- PNG ファイルはリポジトリにコミットするので、Inkscape が必要なのは PNG ファイルを作るときだけである。
  ビルドと CI には Inkscape は要らない。
- PNG ファイルは、Qt のリソースシステム (`icons/icons.qrc`) でプログラムに埋め込む。
- プログラムは `UIIcon::fromName( "NAME" )` (`src/ui_icon.h`) でアイコンを作る。
  アイコンを描くたびに、アプリのパレットの配色 (明るい・暗い。`UIPalette::isDark()`) に合った画像を使うので、View > UI Color Scheme を切り替えるとアイコンも切り替わる。
  画像の大きさは、描く大きさと画面の device pixel ratio で選ぶ (例: 16 ピクセルのアイコンは、device pixel ratio が 2 の画面では 32×32 の画像で描く)。
- アプリのアイコン (ウィンドウのアイコン、インストールするアイコン) は `icons/app/` にあり、配色では切り替えない ([アプリのアイコン](#アプリのアイコン))。
- ツールバーのアイコンのうち 10 個 (open、live_mode、play、stop、step_back、step_forward、slower、faster、prev_score、next_score) は、soccerwindow2 の SVG ファイルの写しである (中の説明に元のファイルの名前を書いた)。
  2 つの道具で同じ働きのボタンが同じ見た目になるよう、直すときは soccerwindow2 と合わせることを考える。

## ファイル

```
icons/
  svg/NAME.svg          アイコンの元のファイル (このファイルを編集する)
  light/SIZE/NAME.png   明るい配色用のアイコン (SIZE: 16、24、32、48)   } generate.sh
  dark/SIZE/NAME.png    暗い配色用のアイコン (SIZE: 16、24、32、48)     } が作る
  palette.txt           色の対応表 (明るい配色用 → 暗い配色用)
  palette.awk           色の検査と置き換え (generate.sh とテストが使う)
  generate.sh           PNG ファイルと下の一覧を作る
  app/svg/NAME.svg      アプリのアイコンの元のファイル (32×32 の格子)
  app/SIZE/NAME.png     アプリのアイコン (SIZE: 16、32、48、64、128、256)  } generate.sh が作る
  app/make_rcssmonitor_svg.py  rcssmonitor のアイコンの SVG を作るスクリプト
  app/robocup_logo.png  RoboCup の公式のロゴ (rcssmonitor のアイコンに埋め込む)
  icons.qrc             Qt のリソースファイル (PNG ファイルの一覧)      } generate.sh
  icons.am              Automake 用のファイルの一覧                    } が作る
  generated.sha256      SVG ファイルと palette.txt のチェックサム      }
  rcss.ico              Windows 用のアイコン (src/rcssmonitor.rc が参照する。今のビルドでは使わない)
  legacy/               古いアイコン (XPM など)。記録として残しているだけで、ビルドでは使わない
test/check_icons.sh     上のファイルを検査する
```

「generate.sh が作る」ファイルは、手で編集しない。

## アイコンの追加・修正

1. Inkscape (またはテキストエディタ) で `icons/svg/NAME.svg` を描く・編集する。[描き方の決まり](#描き方の決まり)に従うこと。
   NAME に使える文字は `a-z`、`0-9`、`_` だけである。
2. PNG ファイルを作り、一覧を更新する。
   ```
   icons/generate.sh NAME
   ```
3. ファイルを検査し、PNG ファイルを目で確かめる。
   ```
   test/check_icons.sh
   ```
4. (新しいアイコンの場合) ソースコードでアイコンを使う。
   ```cpp
   #include "ui_icon.h"

   act->setIcon( UIIcon::fromName( "NAME" ) );
   ```
5. ビルドしてテストを実行する (`./cmake-helper.sh test`)。
   CMake は新しいファイルを自動で埋め込む。
   autotools では、`icons/icons.am` が変わるので、`make` が automake を自動で実行し直す。
6. `icons/svg/NAME.svg`、8 つの PNG ファイル、`icons/icons.qrc`、`icons/icons.am`、`icons/generated.sha256` をコミットする。

アイコンを削除するときは、SVG ファイルと 8 つの PNG ファイルを削除し (`git rm`)、`icons/generate.sh --lists` を実行する。
`generated.sha256` からその行を手で消す (または `icons/generate.sh` ですべてを作り直す)。

## 描き方の決まり

アイコンの線の太さと色合いをそろえるため、次の決まりに従う (soccerwindow2 と同じ)。

- 大きさ: `width="16" height="16" viewBox="0 0 16 16"`。
  24×24、32×32、48×48 の画像は、同じ SVG ファイルを 1.5 倍、2 倍、3 倍の大きさで出力して作る。
- 画素の格子: 1 ピクセルの線は画素の中心 (例: `x="1.5"`) に、塗りの図形の端は画素の境界 (例: `x="2"`) に置き、16×16 の画像がぼやけないようにする。
  そうすれば、32×32 と 48×48 の画像もぼやけない (24×24 は 1.5 倍なので、少しぼやける)。
- 余白: ほとんどのアイコンは 1〜15 の範囲 (14×14 ピクセル) に描く。
  図形に必要なら、輪郭が端に触れてもよい。
- 線: 輪郭は 1 ピクセルで、色は `fg`。
  記号 (矢印、×など) は 1 か 2 ピクセルの線で描く。
  線はなるべく水平・垂直・45° にする。
- 小さな丸 (外径 7 ピクセル以下): `circle` で描くと 16×16 でぼやけるので、画素にそろえた角丸の四角で描く。
  `rx` は幅の半分より 0.5 小さくする。
  例: 外径 5 ピクセルの丸は `<rect x="1.5" y="1.5" width="4" height="4" rx="1.5"/>`、外径 7 ピクセルの丸は `width="6" height="6" rx="2.5"`。
  大きな丸 (kick_off のボールなど) は `circle` でよい。
- ログプレイヤーの記号 (再生、停止、コマ送りなど): 三角は高さ 10 ピクセル (y 3〜13)・幅 5 ピクセル (45° の辺)、棒は幅 2 ピクセルにそろえる。
  並べて使うので、大きさをそろえる。
- 塗り: 部品のある物 (フォルダ、ボールなど) には `fg` の輪郭を付ける。
  単純な色の印には輪郭を付けない。
  黒い記号 (再生の三角など) は `fg` で塗り、輪郭を付けない。
- 色: `palette.txt` の明るい配色用の色 (LIGHT) だけを使う。
  色を薄くするために透明度を使わず、`palette.txt` に色を加える。
- 選手やチームを表すアイコンには、フィールドの描画で同じ意味を持つ色を使う。

Inkscape で編集したときは、`palette.txt` にない色 (グラデーションや、スポイトで拾った色など) が加わっていないか確かめる。
`generate.sh` と `test/check_icons.sh` がそのような色を報告する。

## アプリのアイコン

アプリのアイコン (ウィンドウのアイコンなど) は、ツールバーのアイコンとは次の点が違う。

- 元のファイルは `icons/app/svg/NAME.svg`。32×32 の格子 (`viewBox="0 0 32 32"`) で描く。
- `generate.sh` が `icons/app/SIZE/NAME.png` (SIZE: 16、32、48、64、128、256) を作る。
- 小さな大きさで細部がつぶれるときは、その大きさ専用の元のファイル `icons/app/svg/NAME.SIZE.svg` (例: `NAME.16.svg`。16×16 の格子で描く) を置ける。その大きさだけ、そちらから作る。
- ウィンドウマネージャやデスクトップ環境が表示するので、配色では切り替えない (暗い配色用は作らない)。そのため、色は `palette.txt` に縛られない。
- プログラムは `UIIcon::appIcon( "NAME" )` で、6 つの大きさの画像を持つアイコンを作る (`MainWindow` の `setWindowIcon( UIIcon::appIcon( "rcssmonitor" ) )`)。

今のアプリのアイコンは `rcssmonitor` だけである。
細い縁 (黒いベゼルと銀色の縁) のモニタの画面に、RoboCup の公式のロゴを置いた絵である。
16 ピクセル用の SVG はなく、6 つの大きさとも同じ SVG から作る。

- ロゴは `icons/app/robocup_logo.png` で、手を加えない。
  RoboCup Federation の画像 `https://www.robocup.org/system/static_contents/images/000/000/010/list/robocup_logo_ext.jpg` (750×330) のロゴの部分 (灰色の枠と緑の帯を含む 397×252 ピクセル) を切り出して、PNG にしたものである。
  ロゴの使い方の決まりは `https://robocup.org/logo_guidelines` にある (ベクタ版や高解像度版は載っていない)。
- SVG (`icons/app/svg/rcssmonitor.svg`) は `icons/app/make_rcssmonitor_svg.py` (Python 3 の標準ライブラリだけを使う) が作る。
  ロゴを data URI で埋め込むので、SVG は約 100 KB になる。
  画面はロゴの縦横比で、縁、支柱、台の大きさと色は、スクリプトの先頭の定数で決める。
  外枠を変えるときは、このスクリプトを直して実行し、`icons/generate.sh rcssmonitor` で PNG を作り直す。

```
python3 icons/app/make_rcssmonitor_svg.py
icons/generate.sh rcssmonitor
```

CMake は、48×48 の画像を freedesktop.org の hicolor テーマの場所 (`PREFIX/share/icons/hicolor/48x48/apps/rcssmonitor.png`) にインストールする (トップの `CMakeLists.txt`)。

Windows 用のアイコン `icons/rcss.ico` (`src/rcssmonitor.rc` が参照する。今の CMake と autotools のビルドでは使わない) は、16、32、48、256 ピクセルの PNG から作った。
作り直すときは、Python の Pillow で次のように作る。

```
python3 -c "from PIL import Image; s = [16, 32, 48, 256]; im = {n: Image.open(f'icons/app/{n}/rcssmonitor.png') for n in s}; im[256].save('icons/rcss.ico', sizes=[(n, n) for n in s], append_images=[im[n] for n in s[:-1]])"
```

macOS 用のアイコン (`.icns`) は作っていない (古いものは `icons/legacy/rcss.icns`)。

## palette.txt

各行は、役割、明るい配色用の色 (LIGHT)、暗い配色用の色 (DARK) の 3 つの欄からなる。

```
# role      light    dark
fg          #303030  #e0e0e0
paper       #ffffff  #505050
```

- SVG ファイルでは LIGHT の色だけを使う。
  `generate.sh` は、LIGHT の色をそれぞれ DARK の色に置き換えて、暗い配色用のアイコンを作る。
- 色は `#rrggbb` の形 (小文字) で書く。役割と LIGHT の色は重複してはいけない。
- 両方の配色で同じ色にしたいとき (例: フィールドの白線 `field_line`) は、LIGHT と DARK に同じ色を書く。
  ただし、LIGHT の色はほかの役割と重複できないので、例えば白は `paper` (#ffffff) と区別して #f8f8f8 にしている。
- DARK の色は、暗い背景 (例: `#323232`) に対して、LIGHT の色が明るい背景 (例: `#efefef`) に対して持つのと同じくらいのコントラストになるように選ぶ。
- `palette.txt` を変えたら、`icons/generate.sh` (名前を指定しない) を実行して、すべてのアイコンを作り直す。
  名前を指定しても、`generate.sh` が自動ですべてを作り直す。
- `palette.txt` と `palette.awk` は soccerwindow2 のものと同じである。

## generate.sh

```
icons/generate.sh [NAME...]     アイコンの PNG ファイルを作る (省略するとすべてのアイコン。アプリのアイコンも同じ)
icons/generate.sh --lists       icons.qrc と icons.am だけを更新する (Inkscape は使わない)
icons/generate.sh --help        使い方を表示する
```

- Inkscape 1.0 以降が必要である。必要なら、環境変数 `INKSCAPE` でコマンドを指定する。
  例: `INKSCAPE=/snap/bin/inkscape`、`INKSCAPE="flatpak run org.inkscape.Inkscape"`。
- 実行するコマンドを、実行する前に表示する。例:
  ```
  + inkscape --pipe --export-type=png --export-filename=- --export-background-opacity=0 -w 16 -h 16 < svg/open.svg > light/16/open.png
  + awk -v mode=dark -f palette.awk palette.txt svg/open.svg | inkscape --pipe ... -w 16 -h 16 > dark/16/open.png
  ```
  SVG のデータを標準入力から渡し、PNG のデータを標準出力で受け取るので、Inkscape はファイルを自分では読み書きしない
  (snap でインストールした Inkscape は隠しディレクトリのファイルを読めず、シェルのカレントディレクトリも使わないため)。
- Inkscape を実行する前に、SVG ファイルの色を `palette.txt` で検査する。
- PNG ファイルを作った後、`icons.qrc`、`icons.am`、`generated.sha256` を更新する。
  `generated.sha256` には、PNG ファイルを作ったときの `palette.txt` と SVG ファイルのチェックサムを記録する。
  `test/check_icons.sh` はこれを使って、`generate.sh` を実行せずに編集した SVG ファイルを見つける。
- soccerwindow2 の `generate.sh` との違いは、ツールバーのアイコンの大きさ (16、24、32、48。soccerwindow2 は 16 と 32) だけである。

## Qt のリソースシステム (.qrc)

Qt のリソースシステムは、ファイルをプログラムに埋め込む仕組みである。
埋め込むファイルをリソースファイル (`.qrc`) に並べると、リソースコンパイラ `rcc` がそれらを C++ のソースファイルに変換し、プログラムにコンパイル・リンクされる。
プログラムは、`:/` で始まるパスで、ディスク上のファイルと同じように読み込める
(例: `QPixmap( ":/icons/light/16/open.png" )`)。
rcssmonitor では、アイコンのほかに、3D 表示のシェーダ (`src/view3d/shaders/view3d_shaders.qrc`) と同梱の人型パック (`resources/figures/view3d_figures.qrc`) も同じ仕組みで埋め込んでいる。

### 書式

`icons/icons.qrc` は XML のファイルである。

```xml
<!DOCTYPE RCC>
<RCC version="1.0">
<qresource prefix="/icons">
    <file>light/16/open.png</file>
    <file>light/24/open.png</file>
    ...
    <file>app/16/rcssmonitor.png</file>
</qresource>
</RCC>
```

- `<file>` には、`.qrc` ファイルのあるディレクトリからの相対パスでファイルを書く。
- `<qresource>` の `prefix` は、プログラムで使うパスの前に付く。
  ファイル `icons/light/16/open.png` は `:/icons/light/16/open.png` で読み込む。
- `rcc` は、サイズが小さくなる場合にファイルを圧縮する。PNG ファイルは、ふつうそのまま格納される。

`icons/icons.qrc` は `generate.sh` が SVG ファイルの一覧から作るので、手で編集する必要はない。
アプリのアイコンの SVG ファイルは埋め込まない (PNG ファイルだけ)。

### ビルド

CMake (`src/CMakeLists.txt`): `.qrc` ファイルを静的ライブラリ `rcssmonitor_core` のソースに加え、`AUTORCC` で `rcc` を実行する。
CMake は `.qrc` ファイルに載っているファイルを知っているので、それらが変わると `rcc` を実行し直す。

```cmake
add_library(rcssmonitor_core STATIC
  ...
  ${PROJECT_SOURCE_DIR}/icons/icons.qrc
  )
set_target_properties(rcssmonitor_core PROPERTIES AUTORCC ON)
```

autotools (`src/Makefile.am`): `configure` が見つけた `rcc` (`QT_RCC`) で、`make` が `qrc_icons.cpp` を作る。

```make
ICONS_DIR = $(top_srcdir)/icons
include $(top_srcdir)/icons/icons.am

qrc_icons.cpp: $(ICONS_DIR)/icons.qrc $(ICONS_PNG_FILES)
	$(QT_RCC) --name icons --output $@ $(ICONS_DIR)/icons.qrc
```

- `qrc_icons.cpp` は `nodist_rcssmonitor_SOURCES` に載せている。
  トップの `Makefile.am` でも `icons/icons.am` を `include` し、SVG と PNG のファイルを `make dist` の tarball に加えている (`EXTRA_DIST`)。
- `configure` (`m4/ax_have_qt.m4`) は、`moc` と同じく `rcc` を PATH から探す (qmake の場所からではない)。
  PATH に別の Qt (anaconda など) があるとそちらを使うので、使う Qt の `bin` を PATH の先頭に置いて `configure` を実行するか、`make QT_RCC=/usr/lib/qt5/bin/rcc` のように指定する。

### 初期化

静的ライブラリに入れたリソースは、使う前に `Q_INIT_RESOURCE( NAME )` で登録しなければならない。
NAME は `.qrc` ファイルの名前から拡張子を除いたもの (または `rcc --name` で指定した名前) である。
登録しないと、リンカがリソースを取り除いてしまうことがある。CMake のビルドでは、アイコンは静的ライブラリ `rcssmonitor_core` にリンクされる。

`UIIcon::fromName()` と `UIIcon::appIcon()` は、`Q_INIT_RESOURCE( icons )` を呼ぶ `UIIcon::initResources()` を自動で呼ぶ。
`:/icons/...` のパスでファイルを直接読み込む場合 (テストなど) は、先に `UIIcon::initResources()` を呼ぶこと。
`Q_INIT_RESOURCE` は、名前空間の外の関数から呼ばなければならない。

## テスト

- `test/check_icons.sh` (CTest の `check_icons`) は、Inkscape を使わずに次のことを検査する。
  - `palette.txt` が正しく、SVG ファイルがその LIGHT の色だけを使っていること。
  - 各 SVG ファイルに、正しい大きさの 8 つの PNG ファイルがあり、SVG ファイルのない PNG ファイルがないこと。
  - 各アプリのアイコンに、正しい大きさの 6 つの PNG ファイルがあり、大きさ専用の SVG ファイルが正しい大きさのものであること。
  - `icons.qrc` と `icons.am` に、ちょうどこれらのファイルが載っていること。
  - PNG ファイルが、今の SVG ファイルと `palette.txt` から作られていること (`generated.sha256`)。
- `test/test_ui_icon.cpp` (CTest の `ui_icon`) は、アイコンが埋め込まれていること、
  `UIIcon::fromName()` が今の配色、描く大きさ、device pixel ratio に合った画像を使うこと、
  アプリのアイコンが 6 つの大きさの画像を持ち、ウィンドウのアイコンになっていること、
  MainWindow のボタンが今の配色に合った画像で描かれることを検査する。
- `test/test_toolbar.cpp` も、ツールバーのボタンが今の配色の画像で描かれることを検査する。
