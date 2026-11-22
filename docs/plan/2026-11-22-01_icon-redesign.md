# アイコンの刷新 (XPM から PNG へ、明るい配色用と暗い配色用) 計画

| 項目 | 内容 |
|---|---|
| 作成日 | 2026-11-22 |
| 対象 | rcssmonitor develop (5affd90、20.0.0-beta.1) |
| ブランチ | `feature/icon-redesign` (配色 (F1) を取り込んだ develop から。ユーザが GitHub 上で作る) |
| 状態 | 計画を作った (2026-11-22)。段階 0 (デザインとアプリのアイコンの案) を済ませた。段階 1 (仕組みと再生のアイコン) を実装し、ユーザが画面で確かめた (2026-11-22)。次は段階 2 |
| 英語版 | [2026-11-22-01_icon-redesign_en.md](2026-11-22-01_icon-redesign_en.md) (内容は同じ) |
| 関連 | 総合の計画 [2026-11-22-00_v20-features.md](2026-11-22-00_v20-features.md) の F2。soccerwindow2 の `docs/plan/2026-11-21_icon-redesign.md` と `docs/icons.md` |

## 1. 目的と方針

- アイコンを XPM (ソースに `#include` する C の配列) ではなく、PNG で持つ。PNG は Qt のリソース (`.qrc`) で実行ファイルに埋め込む。
- soccerwindow2 と同じ方法をとる (ユーザ、2026-11-22): アイコンを SVG で描き、開発のときに Inkscape で明るい配色用と暗い配色用の PNG を作ってコミットする。ビルドと CI には Inkscape は要らない。実行時は、アイコンを描くたびに配色を調べて、明るい用と暗い用を選ぶ。
- ツールバーのアイコンはすべて (XPM の 9 個と、コードで描いている 6 個) を、統一したデザインで描き直す。アプリのアイコンも SVG で描き直し、各サイズの PNG を作る (ユーザ、2026-11-22。総合の計画の U4)。
- 配色の判定 (`isDark(palette)`) は配色 (F1) の後に意味を持つので、F1 の後に行う。アプリのアイコンの各サイズは、デスクトップ (F3) でインストールに使う。

## 2. 現状 (develop 5affd90)

### 2.1 アイコンとその使われ方

| 種類 | アイコン | 使われ方 |
|---|---|---|
| XPM (使っている) | rcss (48×48、アプリのアイコン) | `main_window.cpp:105` の `setWindowIcon( QIcon( QPixmap( rcss_xpm ) ) )` |
| XPM (使っている) | live、play、stop、plus、minus、decelerate、accelerate、prev_score、next_score (16×16、白地に黒、透明なし) | `main_window.cpp:71〜84` で include。`MainWindow::setToolIcon()` (1432 行) で `toolIcon(xpm, palette)` に渡す。`togglePlayOrStopIcon()` (3123 行) が play と stop を入れ替える |
| XPM (使っていない) | blank、ff、go、open、quit、rec、rev、rew | include していない (rev、rew、rec はコメントアウト) |
| コードで描く | Open、Connect、Disconnect、KickOff、Loop、LiveDelay | `src/toolbar_icons.{h,cpp}` の `DrawnIcon`。16 単位の正方形に QPainter で描き、16、24、32、48 px の pixmap を作る。`MainWindow::setDrawnIcon()` (1458 行) |
| コードで描く (色の塗り) | 黄色と赤のカード | `toolbar_icons.cpp` の塗りつぶし。配色によらない |

- XPM とコードで描いた形は、どちらも `src/tool_icon.cpp` の `tintIcon()` で `QPalette::ButtonText` の色に塗る (黒 → その色、白 → 透明、灰色 → 半透明)。パレットが変わると `MainWindow::changeEvent()` から `updateToolIcons()` (1470 行) で塗り直す。
- アプリのアイコンの PNG は 48×48 の `icons/rcssmonitor.png` だけ。CMake でインストールする (CMakeLists.txt:167)。
- `icons/rcss.ico` (48×48)、`icons/rcss.icns`、`src/rcssmonitor.rc` は、どのビルドも使っていない。autotools の `EXTRA_DIST` には XPM と `rcss.ico`、`rcss.icns` がある (Makefile.am)。
- `.qrc` は 3D 表示の 2 つ (`src/view3d/shaders/view3d_shaders.qrc`、`resources/figures/view3d_figures.qrc`) だけ。静的ライブラリ `rcssmonitor_core` に入るので `Q_INIT_RESOURCE` が要る (`gl/shader_program.cpp:56`、`figure_pack.cpp:64`)。`AUTORCC` は 3D の部分でだけ ON (src/CMakeLists.txt:114)。
- autotools の `m4/ax_have_qt.m4` は `QT_RCC` を `AC_SUBST` しているが、`src/Makefile.am` は使っていない。
- チームのロゴの XPM (`team_graphic*.cpp`、`rcss/rcg/parser*`) はログのデータなので、この計画とは関係ない。

### 2.2 テスト

- `test/test_tool_icon.cpp`: `tintIcon` の塗り分けと、Live Mode のアイコンが `ButtonText` に従うこと。
- `test/test_toolbar.cpp`: `icons()` (`DrawnIcon` の形の画素)、`buttons()` (ボタンの順、ツールチップ、アイコンがあること、`ButtonText` が変わったときの Loop と Open の塗り直し)、`liveDelay()` など。
- `cmake/DistCheck.cmake`: インストールされた 48×48 のアイコン。

### 2.3 soccerwindow2 の実装 (移植の元)

soccerwindow2 (`../../rctools/soccerwindow2`、develop 1df28c1) の計画書 `docs/plan/2026-11-21_icon-redesign.md`、文書 `docs/icons.md`。主なコミット: db7c68b (SVG、`palette.txt`/`palette.awk`、`generate.sh`、最初の PNG)、6e6d971 (`.qrc` の埋め込み、CMake と autotools)、62e92fd と c2ae992 (アプリのアイコンと hicolor へのインストール)、9dd965f と 13dc45d (古い仕組みの削除、XPM を legacy へ)。

| 部品 | 内容 |
|---|---|
| `icons/svg/NAME.svg` | 原画。16×16 の格子。`palette.txt` の明るい色だけを使う |
| `icons/palette.txt` | 役割ごとの明るい色と暗い色 (例 `fg #303030 #e0e0e0`、`paper #ffffff #505050`) |
| `icons/palette.awk` | `check` (明るい色だけか)、`dark` (明るい色を暗い色に置き換える)、`palette` |
| `icons/generate.sh` | Inkscape (`--pipe --export-type=png -w N -h N`) で `light/{16,32}/NAME.png`、`dark/{16,32}/NAME.png`、`app/{16,32,48,64,128,256}/NAME.png` を作り、`icons.qrc`、`icons.am`、`generated.sha256` も作り直す |
| `icons/app/svg/NAME.svg` | アプリのアイコン (32×32 の格子)。`NAME.16.svg` で 16 px 用を簡単にできる |
| `src/qt/ui_icon.{h,cpp}` | `UIIcon::fromName()` (`SchemeIconEngine`: 描くたびに `isDark(QGuiApplication::palette())` で明暗を選ぶ。HiDPI では 32 px から縮める。Qt 6.8 の論理の大きさの変更、Qt5 の `virtual_hook` の違いを扱う)、`UIIcon::appIcon()` |
| `test/check_icons.sh` | 色、大きさ、一覧、sha256 の確認 |
| `icons/legacy/xpm/` | 古い XPM。ビルドでは使わず、`export-ignore` |

## 3. 決定事項

### 3.1 ユーザの決定 (2026-11-22)

| # | 内容 |
|---|---|
| D1 | soccerwindow2 と同じ方法 (SVG → 明るい用と暗い用の PNG → `.qrc`) |
| D2 | ツールバーのアイコンはすべて (XPM の 9 個とコードで描く 6 個) 描き直す。アプリのアイコンも描き直す |
| D3 | デザインの決まり (線、格子、余白、色の役割、`palette.txt`) は soccerwindow2 と同じにする。15 個のうち 10 個 (open、live_mode、play、stop、step_back、step_forward、slower、faster、prev_score、next_score) は soccerwindow2 の SVG (`open_rcg`、`logplayer_*`) を使い、5 個 (connect、disconnect、kick_off、loop、live_delay) は同じ決まりで新しく描く (段階 0。§4.1) |
| D4 | PNG は 16、24、32、48 px を作る。LIVE と再生のボタンは 24 px で描くので、24 px (1x) と 48 px (2x) も要る。soccerwindow2 (16 と 32 だけ) からの変更 |
| D5 | アプリのアイコンは、今の意匠の描き直し (画面の中の RoboCup のロゴ)。ロゴは公式のものを使う (段階 0。§4.4) |

### 3.2 設計上の判断 (Claude。段階の初めにユーザが確認する)

| # | 内容 | 理由 |
|---|---|---|
| S1 | ファイルの置き方、`palette.txt`、`palette.awk`、`generate.sh`、`UIIcon` は soccerwindow2 のものを移植し、名前と置き場所をそろえる | 2 つのプロジェクトで保守の仕方を同じにする |
| S2 | カード (黄と赤) は、色そのものを表すので、コードの塗りつぶしのまま残す (段階 0 で確かめる) | 配色によって変える必要がない |
| S3 | 古い XPM は消さず、`git mv` で `icons/legacy/xpm/` に移す。`icons/legacy/README.md` に一覧と新旧の比較の画像を置く。`.gitattributes` の `export-ignore` で tarball から除く | soccerwindow2 と同じ。記録として残す |
| S4 | `.qrc` は `rcssmonitor_core` に入れ、`AUTORCC` を常に ON にし、`Q_INIT_RESOURCE( icons )` を呼ぶ | 3D の `.qrc` と同じ形 |
| S5 | autotools では `QT_RCC` で `qrc_icons.cpp` を作る規則を足し、`generate.sh` が作る `icons.am` を include する | soccerwindow2 と同じ |
| S6 | 使っていない XPM (blank、ff など) に当たるアイコンは作らない | 使うところがない |

### 3.3 段階の初めに決めること

- アプリのアイコンの意匠: 今の意匠 (画面の中の RoboCup のロゴ) を描き直すか、rcsoccersim のロゴ (黄と赤の 2 人の選手とボール。`rcsoccersim.github.io/static/img/logo.png`) を基にするか、別の案か。段階 0 で案を並べた画像を作り、ユーザが選ぶ。 → 今の意匠の描き直し、ロゴは公式のもの (D5)。
- `rcss.ico`、`rcss.icns`、`src/rcssmonitor.rc` (どのビルドも使っていない): 新しいアイコンから作り直すか、legacy に移すか (段階 3)。
- 文書の名前: `docs/icons.md` と英語版 (`docs/icons_en.md`。このリポジトリの `_en` の決まりにそろえる)。

## 4. 設計

### 4.1 アイコンの一覧 (名前は段階 0 で、この案のとおりに決めた)

| 今のアイコン | 新しい名前 (案) | 使うところ |
|---|---|---|
| live.xpm | `live_mode` | Live Mode |
| play.xpm / stop.xpm | `play`、`stop` | 再生と停止の切り替え (`togglePlayOrStopIcon`) |
| plus.xpm / minus.xpm | `step_forward`、`step_back` | コマ送り |
| accelerate.xpm / decelerate.xpm | `faster`、`slower` | 速さ |
| next_score.xpm / prev_score.xpm | `next_score`、`prev_score` | 得点へのジャンプ |
| DrawnIcon::Open | `open` | Open Game Log |
| DrawnIcon::Connect / Disconnect | `connect`、`disconnect` | 接続と切断 |
| DrawnIcon::KickOff | `kick_off` | 審判のキックオフ |
| DrawnIcon::Loop | `loop` | ループ再生 |
| DrawnIcon::LiveDelay | `live_delay` | 遅延ライブ |
| rcss.xpm | `rcssmonitor` (アプリ) | 窓のアイコン、インストール |

### 4.2 コード

- `src/ui_icon.{h,cpp}` (新規): soccerwindow2 の `UIIcon` を移す。`fromName( name )`、`appIcon( name )`。配色の判定は、配色 (F1) で `src/ui_palette.h` に置いた `UIPalette::isDark( palette )` を使う (2026-11-22。soccerwindow2 の `UIIcon::isDark()` と同じ判定)。`fromNumber` (soccerwindow2 のデバッグの番号のアイコン) は要らない。
- 消す: `src/tool_icon.{h,cpp}`、`src/toolbar_icons.{h,cpp}` の `DrawnIcon` (カードの塗りは S2 のとおり残す場所を決める)、`MainWindow` の `M_tool_icons`、`M_drawn_icons`、`setToolIcon`、`setDrawnIcon`、`updateToolIcons`、`changeEvent` の中のパレットの変化での塗り直し。
- `togglePlayOrStopIcon` は `UIIcon::fromName( "play" / "stop" )` に替える。
- 窓のアイコンは `UIIcon::appIcon( "rcssmonitor" )` (各サイズ)。

### 4.3 ビルド

- CMake (`src/CMakeLists.txt`): `${PROJECT_SOURCE_DIR}/icons/icons.qrc` を `rcssmonitor_core` のソースに足し、`AUTORCC ON` を 3D の外に出す。
- autotools (`src/Makefile.am`、`Makefile.am`): `include $(top_srcdir)/icons/icons.am`、`qrc_icons.cpp: $(ICONS_DIR)/icons.qrc $(ICONS_PNG_FILES)` の規則 (`$(QT_RCC) --name icons`)、`nodist_rcssmonitor_SOURCES`、`EXTRA_DIST` に SVG と PNG。XPM を `EXTRA_DIST` から除く。
- CMake の dist (`git archive`) は `.gitattributes` に従うので、`icons/legacy/` の `export-ignore` を足す。

### 4.4 アプリのアイコン (段階 0 で決めた。D5)

- 32×32 の格子で、画面 (濃い灰色の縁、角丸) と台を描き、画面に RoboCup の公式のロゴを、手を加えずに置く。画面の縦横比はロゴ (397×252) に合わせる。
- ロゴは robocup.org の `robocup_logo_ext.jpg` (750×330) のロゴの部分を切り出した PNG。SVG には data URI で埋め込む。
- アプリのアイコンは配色で切り替えないので、色は `palette.txt` に縛られない (soccerwindow2 と同じ)。
- 16 px と 24 px の扱い、ロゴの PNG とスクリプトの置き場所は段階 3 で決める (§9 の段階 0 の記録)。

## 5. 段階

各段階で、明るい配色と暗い配色の比較の画像をユーザが見て確かめてから、コミットする。

| # | 作業 | 完了条件 |
|---|---|---|
| 0 | デザインの決まり (線の太さ、色の役割、余白) と見本 (再生のアイコンなど数個)。明暗の比較の画像。アプリのアイコンの案 (§3.3) | ユーザがデザインの決まりとアプリのアイコンの案を決める |
| 1 | 仕組み: `palette.txt`、`palette.awk`、`generate.sh`、`UIIcon`、`icons.qrc`、CMake と autotools。再生のアイコン (play、stop、コマ送り、速さ、得点、live) を置き換える | 両方のビルドと CI が通る。ユーザが画面で確かめる |
| 2 | 残りのツールバーのアイコン (open、connect、disconnect、kick_off、loop、live_delay) | 同上 |
| 3 | アプリのアイコン (SVG、各サイズの PNG、窓のアイコン)。`rcss.ico` などの扱い | 同上。各サイズで見て確かめる |
| 4 | 古いコード (`tool_icon`、`DrawnIcon` など) の削除、XPM を `icons/legacy/xpm/` へ、`icons/legacy/README.md`、`export-ignore` | XPM の include と塗り直しのコードがなくなる。distcheck が通る |
| 5 | 文書 (`docs/icons.md`、`docs/icons_en.md`、`docs/build.md` からの参照、README、NEWS) | ユーザが確かめる |

## 6. テスト

- `test_tool_icon.cpp` を `test_ui_icon.cpp` に替える: 明るいパレットと暗いパレットで違う画像が選ばれる、HiDPI (dpr 2) では 32 px の画像から描く、アプリのアイコンの各サイズがある、無い名前のときの扱い。
- `test_toolbar.cpp`: `icons()` (DrawnIcon の画素の検査) を消し、`buttons()` の塗り直しの検査を「暗いパレットで暗い用の画像が選ばれる」に替える。ボタンの順とツールチップの検査は残す。
- `test/check_icons.sh` (soccerwindow2 から移す): SVG が明るい色だけを使う、PNG の大きさ、`icons.qrc` と `icons.am` の一覧が SVG と合う、`generated.sha256`。CTest に登録する (Inkscape は要らない)。
- `cmake/DistCheck.cmake` のアイコンの確認は、デスクトップ (F3) で各サイズに直す。

## 7. 変更するファイル (見込み)

| ファイル | 内容 |
|---|---|
| `icons/svg/*.svg`、`icons/app/svg/*.svg`、`icons/palette.txt`、`icons/palette.awk`、`icons/generate.sh` (新規) | 原画と作り方 |
| `icons/{light,dark}/{16,32}/*.png`、`icons/app/*/rcssmonitor.png`、`icons/icons.qrc`、`icons/icons.am`、`icons/generated.sha256` (新規。生成物をコミット) | 埋め込む PNG |
| `icons/legacy/xpm/*.xpm` (移動)、`icons/legacy/README.md` (新規) | 古いアイコン |
| `src/ui_icon.{h,cpp}` (新規)、`src/tool_icon.{h,cpp}` と `src/toolbar_icons.{h,cpp}` (削除または縮小) | コード |
| `src/main_window.{h,cpp}` | アイコンの使い方 |
| `src/CMakeLists.txt`、`src/Makefile.am`、`Makefile.am`、`.gitattributes` | ビルドと配布 |
| `test/test_ui_icon.cpp` (新規)、`test/test_toolbar.cpp`、`test/check_icons.sh` (新規)、`test/CMakeLists.txt` | テスト |
| `docs/icons.md`、`docs/icons_en.md` (新規)、`docs/build.md`、README、NEWS | 文書 |

## 8. 対象外

- ツールバーやメニューの並びの変更。
- フィールドの描画の色。

## 9. 記録

### 2026-11-22: 計画の作成

- 総合の計画 (2026-11-22-00_v20-features.md) と一緒に作った。ユーザの決定 D1、D2 を受けた。
- Inkscape 1.4.4 (snap 版、`/snap/bin/inkscape`) が手元にある。

### 2026-11-22: 段階 0 (デザインとアプリのアイコンの案)

- F1 (PR #71) が develop に取り込まれた (6c950f4) 後、ユーザが GitHub 上で `feature/icon-redesign` を作った。段階 0 を始める前に、S1〜S6 を計画書のとおりとすることをユーザが確かめた。
- 作った見本 (作業場所に置き、コミットしていない。段階 1 と 3 でリポジトリに入れる):
  - ツールバーの 15 個の SVG。10 個は soccerwindow2 の SVG の写しで、5 個は新しく描いた (connect と disconnect はプラグとコンセント、kick_off はボール、loop は輪の矢印、live_delay はストップウォッチ。どれも今の web 版の形を soccerwindow2 の決まりで描き直したもの)。soccerwindow2 の `palette.awk` で色を確かめ、暗い配色用を作り、Inkscape で 16、24、32 px にした。
  - 今のツールバーと、案のツールバー (1x と 2x、明暗) の比較の画像。16 px を 6 倍にした画像。
  - アプリのアイコンの 3 案 (A: 今の意匠の描き直し、B: rcsoccersim のロゴ、C: 2D 表示の画面) を 16〜128 px で、明るい背景と暗い背景に並べた画像。
- ユーザが決めたこと: D3 (デザインの決まりは soccerwindow2 と同じ)、D4 (16、24、32、48 px)、D5 (アプリのアイコンは案 A。RoboCup のロゴは公式のものを使う)。
- 公式のロゴ (§4.4):
  - ユーザが示した robocup.org の画像 `robocup_logo_ext.jpg` (750×330、JPG) を使う。ロゴの決まりのページ (`https://robocup.org/logo_guidelines`) を 2026-11-22 に確かめたが、ベクタ版や高解像度版はなかった (229×99 の PNG などだけ)。
  - ロゴの部分 (灰色の枠と緑の帯を含む 397×252 px。JPG の (175, 38) から) を手を加えずに切り出し、PNG にして SVG に埋め込んだ。画面の縦横比はロゴに合わせた。256 px でも画面の幅は約 200 px なので、ロゴを拡大しない。
  - ロゴの決まりのページによれば、RoboCup のロゴを使えるのは RoboCup Partners と参加チームで、学術と PR のためである (報道での使用は RoboCup の事務局に相談する)。rcssmonitor は RoboCup のサッカーシミュレーションの公式の道具で、今のアイコンもこのロゴを使っている。公式のロゴを使うことはユーザが決め、Claude がこの決まりの内容をユーザに伝えた。
- 段階 3 で決めること: 16 px と 24 px ではロゴが読めない (簡略版を作るか)。ロゴの PNG と、それを埋め込んだ SVG を作るスクリプトの置き場所 (soccerwindow2 の `make_soccerwindow2_svg.py` にならう)。デスクトップ (F3) で hicolor の scalable にインストールする SVG は、PNG を埋め込んだもの (約 100 KB) になる。

### 2026-11-22: 段階 1 (仕組みと再生のアイコン)

- ユーザの指示で段階 1 を始めた。
- 作ったもの:
  - `icons/palette.txt`、`icons/palette.awk`: soccerwindow2 のものをそのまま (D3)。
  - `icons/generate.sh`: soccerwindow2 のものを移し、大きさを 16、24、32、48 にした (D4)。大きさのディレクトリがなければ作る。`icons.am` の説明の `src/qt/Makefile.am` を `src/Makefile.am` に。
  - `icons/svg/` の 9 個 (live_mode、play、stop、step_back、step_forward、slower、faster、prev_score、next_score): soccerwindow2 の `logplayer_*.svg` の写し。中の説明に元のファイルの名前を足した。`generate.sh` で `icons/{light,dark}/{16,24,32,48}/*.png` (72 個)、`icons.qrc`、`icons.am`、`generated.sha256` を作った (Inkscape 1.4.4)。
  - `src/ui_icon.{h,cpp}` (新規): soccerwindow2 の `UIIcon::fromName()` と `SchemeIconEngine`。配色の判定は `UIPalette::isDark()`、大きさは `UIIcon::SIZES` (16、24、32、48)。アプリのアイコン (`appIcon`) は段階 3 で足す。数字のアイコン (`fromNumber`) は移さない。
  - `MainWindow`: 再生の 9 個のアクションのアイコンを `UIIcon::fromName()` に替えた。`togglePlayOrStopIcon` も。使わなくなった 9 個の XPM の `#include` を消した (XPM のファイルは段階 4 で legacy に移す。`setToolIcon` などの古い仕組みは、残りのアイコン (段階 2) の後、段階 4 で消す)。
  - CMake: `icons/icons.qrc` を `rcssmonitor_core` に入れ、`AUTORCC` を 3D の外に出して常に ON にした (S4)。
  - autotools: `src/Makefile.am` に `icons.am` の include と `qrc_icons.cpp` の規則 (`$(QT_RCC) --name icons`)、トップの `Makefile.am` の `EXTRA_DIST` に icons の仕組みと SVG、PNG を足した (S5)。
  - テスト: `test/check_icons.sh` (soccerwindow2 のものを移し、大きさを 4 つに) を CTest に登録 (Inkscape は要らない)。`test/test_ui_icon.cpp` (新規): 9 個のアイコンと各大きさの画像、配色に合わせた画像の選び方 (アプリのパレットを変えると同じアイコンが変わる)、描く大きさ (24 は 24 の画像、48 より大きくしない)、デバイスピクセル比 (2 で 16 は 32、24 は 48、1.5 で 16 は 24 の画像)、無効の表示、無い名前、MainWindow の再生のボタン (Play と Stop の切り替えを含む)。`test/test_tool_icon.cpp`: Live Mode の塗り直しの検査 (`followPalette`) を消した (`test_ui_icon` に移った。`tintIcon` の検査は段階 4 まで残す)。
- 分かったこと: autotools の `m4/ax_have_qt.m4` は、`moc` と `rcc` を PATH から探す (qmake の場所からではない)。PATH に別の Qt (anaconda など) があると、そちらが使われる。前からの動き (`moc` も同じ) で、この段階では直さない。PATH から外せば `/usr/lib/qt5/bin/rcc` が使われる。
- 確かめたこと (Ubuntu 24.04):
  - Qt 6.4.2 と Qt 5.15.13 の CMake のビルドで警告はなく、`ctest` の 59 個 (新しい `ui_icon` と `check_icons` を含む) が通った。
  - autotools (Qt5): 作業ツリーの写しで `./bootstrap`、`./configure`、`make` が通った。システムの `rcc` (`/usr/lib/qt5/bin/rcc`) で `qrc_icons.cpp` を作り、`--help` で起動できた。
  - offscreen で、ツールバーを 1x と 2x、明るい配色と暗い配色で画像にして見た。
- ユーザが画面で確かめた (Qt6 と Qt5)。
- この後: コミットとプッシュ、CI、`./cmake-helper.sh distcheck qt6`。
