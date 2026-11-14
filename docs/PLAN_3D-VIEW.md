# rcssmonitor 3D 表示・Qt6 対応 開発計画

| 項目 | 内容 |
|---|---|
| 作成日 | 2026-11-11 |
| 対象 | rcssmonitor。作業の土台は `develop` (01f880a、19.0.1)。TCP / WebSocket 対応 (`feature/support-tcp-ws`) は含めない (D7) |
| 参照実装 | rcssmonitor-web 20.0.0-beta.2 (タグ `v20.0.0-beta.2`、ed36941)。作業ツリーは、このリポジトリと並べて置いた `../rcssmonitor-web` |
| 参照したサーバ | asio 版 rcssserver 19.x。作業ツリーは、このリポジトリと並べて置いた `../rcssserver` (`feature/asio-migration` ブランチ) |
| 英語版 | [PLAN_3D-VIEW.en.md](PLAN_3D-VIEW.en.md) (内容は同じ。§0.1) |
| 状態 | Phase 6 (2026-11-13 着手、`feature/3d-view`)。区切り 6-1〜6-6 (§14)。Phase 5 は済 |

## 0. この計画書の使い方

この計画書を読み込ませて rcssmonitor の開発を進める。web 版 (rcssmonitor-web) で 3D 表示を作ったときの設計と経緯は、web 版の次の資料にある。部品を作る前に、対応する資料とソースを読むこと。

| 資料 | 内容 |
|---|---|
| `rcssmonitor-web/docs/DEVELOPMENT_PLAN.md` §5「Phase 6: 3D 表示」 | 3D 表示の仕様と設計 (カメラ、選手視点、重ね描き、人型、影、景色、ボールの円と軌跡など) |
| 同 §9「Phase 6」区切り 1〜16 | 作った順の記録。ユーザの決定、見つけた不具合とその原因、計測の結果 |
| `rcssmonitor-web/docs/parity.md` | rcssmonitor の機能との対応表。§7 に rcssmonitor の不具合、§8 にキーの互換表 |
| `rcssmonitor-web/docs/protocol.md` | モニタプロトコルと rcg 形式のメモ (JSON の項目、状態ビット、プレイモード) |
| `rcssmonitor-web/src/view/render3d/` ほか | 3D 表示の実装 (§4.2 の対応表) |
| `rcssmonitor-web/test/unit/`、`test/e2e/view3d.spec.ts` | 単体テスト (移植の手本) と E2E テスト |

### 0.1 作業の取り決め (ユーザとの約束)

- 報告は日本語で行う。実装を報告するときは、コミットメッセージの案を添える。
- コミットとプッシュは Claude が行う。その前に、コミットメッセージの案 (と含めるファイル) をユーザに示し、確認を受ける (2026-11-11 から。それまではユーザが行っていた)。コミットメッセージは英語で書き、末尾に指定の `Co-Authored-By` 行を付ける。
- PR が向く取り込み (master への取り込みなど) は、ユーザが行う。PR を作るときは、Claude がタイトルと本文の案 (英語) を出し、勧めるマージの方法 (基本は Merge Commit。Squash Merge が向くときはその理由) を添える (2026-11-11 から。§9.1)。
- 動かして試すときは、ユーザの設定ファイル `~/.rcssmonitor.conf` を書き換えない。`HOME` を一時ディレクトリにして起動する (例: `HOME=$(mktemp -d) ./src/rcssmonitor`)。`--version` だけの起動でも、終了時に設定ファイルを書くので、同じようにする (2026-11-11 に確かめた。§14)。単体テストは CTest が `HOME` をビルドディレクトリの下にする (§10.1)。
- rcssserver を動かすときは、`RCSS_CONF_DIR` を一時ディレクトリにする (`~/.rcssserver` を汚さない)。
- GPU を使うテストを並列で回しすぎない (web 版の作業中に、GPU のページフォルトで作業マシンが落ちたことがある)。
- 区切りごとに、§14 に記録 (決めたこと、見つけた不具合、計測、試験) を足す。
- 計画書は、日本語版 (この文書) と英語版 (`PLAN_3D-VIEW.en.md`) を並行して保つ。どちらかを直したら、同じ区切りでもう一方も同じ内容に直す。
- ブランチはユーザが GitHub 上で作り、手元ではそれを取ってきて切り替える (§9.1)。

### 0.2 web 版に反映すること

この計画で決めたことのうち、web 版 (rcssmonitor-web) にも後で入れるもの。web 版を直したら、ここに印を付ける。

- [ ] **スタミナ容量のバーを太くする** (5-3、2026-11-12、ユーザ): web 版の容量のバー (`render3d.ts` の `BAR.capacity`、0.1 m) は細くて見にくい。rcssmonitor と同じ 0.15 m にする。
- [ ] **キーパーのキャッチ範囲に、キャッチの向きの制限を反映する** (2026-11-13、ユーザ): rcssserver 17.0.0 から、キャッチの向きは `min_catch_angle`〜`max_catch_angle` (既定 -90〜+90) に限られ、キーパーは後ろにはキャッチできない。web 版 (`overlay-math.ts` の `catchAreaRadii()`) は、rcssmonitor の前と同じく円を描く。rcssmonitor と同じ形 (`OverlayMath::catchOutline()`) にする。2 つのパラメータがないログ (サーバ 17 より前) は ±180 (制限なし) として読む (web 版の `param-defaults.ts` は 19.0.0 の ±90)。
- [ ] **操作を 2D にそろえる** (§5.6、5-1b、2026-11-12、ユーザ): 3D でもホイールはフレームを進め戻し、Z と X (と Ctrl+Z) でカメラを寄せる・離す。web 版は 3D でホイールがカメラを寄せる (three.js の OrbitControls の zoom と、寄りのカメラの距離 `closeupWheel`) ので、rcssmonitor と同じく 2D (web 版の 2D のホイールとキー) にそろえる。追うカメラは追ったまま距離が変わる、真上のカメラは寄るとボールのまわり、カメラを変えると倍率が戻る、I で戻す、も rcssmonitor に合わせる。
- [ ] **フォーカス点の表示** (5-5、2026-11-13、ユーザ): web 版のフォーカス点は、rcssmonitor の前と同じく、選手からの線と円だけ。rcssmonitor と同じく、2D と 3D で、誤差が同じになる線 (選手とフォーカス点を焦点とする楕円を視野で切ったもの。`OverlayMath::focusNoiseLines()`)、照準とスポットライト (`OverlayMath::FocusLook`、`focusTicks()`)、3D の目の高さからの線、全員のフォーカス点の設定 (`show_all_focus_points`) を入れる。

## 1. 目的と範囲

1. **3D 表示**: rcssmonitor-web の 3D 表示と同等の機能を rcssmonitor に実装する (§4、§5)。
2. **不具合の修正**: web 版の移植で見つけた rcssmonitor の不具合 14 件を直す (§6)。
3. **JSON パーサの改善**: JSON のログとライブのメッセージを、web 版と同じく頑健に読めるようにする (§7)。
4. **Qt6 対応**: Qt6 でビルドできるようにし、非推奨の API を置き換える (§8)。
5. あわせて、CI (GitHub Actions) とテストを整える (§10)。

範囲外は §13。

## 2. 決定事項と、作業の初めに決めること

### 2.1 決まっていること (ユーザ)

- 3D 表示は、rcssmonitor-web の 3D 表示と同等にする。カメラ、選手視点、重ね描き、人型、影、景色、ボールの円と軌跡、設定、キーを含む。
- 不具合の修正、JSON パーサの改善、Qt6 対応もこの計画で行う。
- 計画書は日本語で書き、英語版を並行して保つ (§0.1)。
- 作業は `develop` から派生させたブランチで進める。TCP / WebSocket 対応は、公式に採用するかが決まっていないので、今は取り込まない (D7、2026-11-11)。
- Phase 0〜3 (Qt6 対応まで) は、終わったものから develop に取り込む。3D 表示 (Phase 4〜8) は、完成して公式に採用されることが決まってから develop に取り込む (§9.1、2026-11-11)。
- qmake のビルドは廃止する (D11、2026-11-11)。

### 2.2 作業の初めに決めること

Phase 0 でユーザに確かめてから進める。推奨案は、この計画の以下の節の前提にしている。

2026-11-11 の状態: D7、D10、D11 はユーザが決めた (表の「決定」。D11 は Phase 0 で見つけた問題)。D1 は Phase 0 の試作の結果で決める。D2〜D6、D8、D9 は推奨案で進める。2026-11-12: D1 は試作の結果で推奨案に決めた (§14)。

| # | 項目 | 推奨 | 代わりの案 | 理由 |
|---|---|---|---|---|
| D1 | 3D の描画の技術 | **決定**: `QOpenGLWidget` と自前の描画 (OpenGL 3.3 core、シェーダを自分で書く。2026-11-12、試作の結果。§14) | Qt Quick 3D (`QQuickWidget` で埋め込む) | 下の §2.3 |
| D2 | 対応する Qt の版 | Qt 5.15 と Qt 6.2 以降の両方 (1 つのソースで切り替える) | Qt6 だけにする | Ubuntu 22.04 には Qt 5.15 と Qt 6.2 の両方がある。両対応の期間を経て、次の版で Qt5 をやめられる |
| D3 | ビルドの仕組み | CMake を主にする。Qt6 は CMake だけで対応し、autotools は Qt5 のまま残す (3D は CMake でだけビルドする)。qmake (`.pro`) は D11 で廃止 | 3 つとも Qt6 に対応させる / autotools と qmake をやめる | 3 つを保守する手間が大きい。autotools の `AX_HAVE_QT` は qmake を通して Qt を探すので、Qt6 への対応が難しい |
| D4 | 3D をビルドの任意にするか | CMake の `ENABLE_3D` (既定 ON)。OpenGL 3.3 が使えない環境では、実行時に 3D を無効にして 2D だけで動く | 必須にする | 古い GPU や仮想マシンでも、2D の監視は今までどおり使えるようにする |
| D5 | glTF の読み込み | cgltf (1 つのヘッダ、MIT) と、アニメーションの評価を自前で書く | tinygltf (MIT)、Assimp (BSD、大きい) | §5.8 のとおり、姿勢はログから毎回求めるので、アニメーションの再生機は要らず、キーフレームを読むだけでよい |
| D6 | 人型のモデル | web 版の `Mannequin.glb` (CC0) を、圧縮なしで書き出し直して Qt のリソース (qrc) に入れる | meshoptimizer のデコーダ (MIT) を同梱して、web 版のファイルをそのまま読む | web 版のファイルは `EXT_meshopt_compression` と `KHR_mesh_quantization` を必須にしている。デスクトップのアプリでは大きさより依存の少なさを取る |
| D7 | 作業の土台 | **決定**: `develop` から派生させる。`feature/support-tcp-ws` は取り込まずに残し、採用が決まってから取り込む (§3.5) | (当初の推奨) `feature/support-tcp-ws` を先に取り込み、そこから切る / `feature/support-tcp-ws` から直接切る | TCP / WebSocket 対応を公式に採用するかがまだ決まっていない (ユーザ)。3D 表示の差分とも分けて見られる |
| D8 | 版 | 20.0.0 (asio 版 rcssserver 20、rcssmonitor-web 20 にそろえる) | 19.1.0 など | Qt6 対応と 3D 表示は大きな変更 |
| D9 | ブランチの方針 | web 版と同じ (master: リリース、develop: 取り込み先、feature/*: 作業、hotfix/*)。取り込みは Merge Commit を基本にする (2026-11-11 にユーザが決めた。それまでは Squash Merge も使っていた)。Squash Merge が向く条件と、フェーズごとのブランチは §9.1 | 今の rcssmonitor の運用に合わせる | web 版の `docs/release.md` |
| D10 | master の AppImage ワークフロー (§3.2) | **決定**: AppImage を廃止する (§2.4 の案 B、2026-11-11)。今のワークフローは保守の負担になっていて (ユーザが GitHub の画面で無効にした)、ブラウザ版のモニタがあるので AppImage の必要性も低い (ユーザ)。案 A は試して動いたが、採用しない | 今のまま残す (Qt 5.12 でもビルドできる範囲にとどめることになり、D2 と合わない) | Ubuntu 20.04 (サポート終了) の Docker イメージ、`qt5-default`、自前のシェルと Python のスクリプトを保守し続ける必要がある。autotools でビルドするので 3D が入らない (D3) |
| D11 | qmake のビルド (§3.2) | **決定**: 廃止する。`rcssmonitor.pro`、`src/src.pro` と、qmake のビルドだけが使っていた古いパーサの写し `src/rcsslogplayer/` を消す (`feature/ci`)。NEWS に書く (Phase 9)。autotools は Qt を探すのに qmake を使うので、`qt5-qmake` のパッケージは引き続き要る | 直して (定義 1 つ、版、boost) CI に入れる | qmake を使う機械がなくなる (ユーザ)。develop の時点ですでに壊れていて、誰も気づいていなかった。CMake と autotools の 2 つで足りる |

### 2.3 D1 (3D の描画の技術) の比較

| 案 | よい点 | 気になる点 |
|---|---|---|
| **QOpenGLWidget と自前の描画** (推奨) | Qt5 と Qt6 の両方で同じコードが動く。QML を持ち込まない (rcssmonitor は QWidget だけのアプリ)。web 版でやったこと (インスタンス描画、影の地図、自作のシェーダ、骨の姿勢をログから毎回決めること、背番号の大きさの上限、描くたびの状態の管理) をすべて自分で決められる。依存は Qt の OpenGL モジュールと cgltf だけ | 描画の基盤を書く量が多い (シェーダ 10 種ほど、影の地図、骨つきメッシュ、線、ビルボード)。macOS の OpenGL は非推奨 (4.1 core までは動く) |
| Qt Quick 3D | 材質、影、霧、骨つきメッシュ、インスタンス描画、ピッキングがそろっている | Qt6 が前提 (骨のアニメーションやインスタンス描画は Qt 6 の機能)。シーンは QML で書き、C++ からはデータを渡す形になる。骨つきの glTF は、ビルド時の変換 (Balsam) を通す道が案内されていて、実行時の読み込み (RuntimeLoader) で骨のアニメーションを扱えるかは確かめられていない。骨を毎フレーム自分で動かす使い方 (§5.8) が素直にできるかも、試す必要がある |
| Qt 3D | C++ の API がある | Qt 6.8 で非推奨になり、標準のリリースから外れた。新しく使わない |

Phase 0 の試作 (§9) で、推奨案で「人型 22 体と影を 1920×1080 で 60 fps」を確かめてから決める。 → 2026-11-12 に両方の案を試作して測り、推奨案に決めた (§14)。

### 2.4 D10 (AppImage) の置き換え先の案

2026-11-11 に案 B (廃止) に決めた。案 A は Phase 0 で試して動いた (§14) が、採用しない。案 A の作り方は、コミット 1fe26c6 の `.github/workflows/appimage.yml` と、下の記述と §14 に残す。

| 案 | 内容 | 保守するもの |
|---|---|---|
| A. タグで動く `release.yml` で、CMake と linuxdeploy で作る (試して動いたが、採用しない) | web 版の `release.yml` と同じ形。タグ (`rcssmonitor-X.Y.Z`) を push したときと、手で実行したときだけ動く。ランナーの上の `container: ubuntu:22.04` (Qt 5.15.3 / Qt 6.2.4、glibc 2.35) で CMake でビルドし、`DESTDIR` にインストールして、linuxdeploy と linuxdeploy-plugin-qt で AppImage にまとめ、`gh release` でリリースに添付する。デスクトップのファイルとアイコンは CMake の `install()` で入れる | ワークフロー 1 つ (50 行ほど)。Ubuntu の LTS が終わるときに基盤の版を上げる (22.04 → 24.04)。linuxdeploy の版 (固定する) を上げる |
| **B. AppImage をやめる** (決定) | ワークフローと `utils/appimage/` を消す。配布はソースと、各ディストリビューションでのビルドの手順 (README) だけにする。ログを見るだけならブラウザ版を案内する | なし |

- 案 B に決めた理由 (2026-11-11): (1) rcssserver のリリースはソースだけで、ライブで使う人はサーバをビルドする環境を持っている。(2) ログを見るだけなら、インストールの要らないブラウザ版で足りる。(3) 22.04 の標準サポートが 2027 年 4 月に終わり、Qt6 と 3D (OpenGL) が入ると、利用者の環境のドライバとの組み合わせを確かめる手間が増える。一方で、19.0.1 のダウンロード数は AppImage 257、ソース 1,154 (2026-11-11) で、AppImage の利用者もいた。リリースノートと README で、ソースからのビルドとブラウザ版を案内する。なお、ブラウザ版のライブ接続は WebSocket に対応したサーバ (asio 版の 20) が前提で、今の rcssserver 19 (UDP だけ) とつなぐにはネイティブ版が要る。
- どちらの案でも、今の `main.yml` は無効にする (2026-11-11 にユーザが無効にした)。このワークフローは任意のブランチとタグへの push で動き、`CMakeLists.txt` の版のリリースがなければ作る。そのため、作業のブランチで版を上げただけでリリースができてしまう。GitHub の画面 (Actions → ci → Disable workflow) で無効にすれば、コミットなしで止められ、戻すこともできる。ファイルは、20.0.0 を master に取り込むときに消す (案 A なら置き換える)。
- 案 A は、Phase 0 で CI を作るときに試す。手で実行する形 (`workflow_dispatch`) は、既定のブランチ (master) に同じファイルがないと動かせないので、試すあいだは `feature/ci` への push で動かし、採用するときにタグの push と手での実行で動く形にする。できた AppImage が別の環境 (Ubuntu 22.04、24.04、26.04) で起動することを確かめてから、採用を決める。3D の入った AppImage が利用者の環境の OpenGL で動くか (linuxdeploy は libGL をまとめない) は、Phase 4 以降に確かめる。
- 基盤を 22.04 にするのは、AppImage はビルドした環境より古い glibc では動かないため。22.04 の Qt (5.15.3、6.2.4) は D2 の下限と同じなので、下限の版でビルドできることの確認にもなる。
- 今のワークフローも linuxdeploy (版を固定しない `continuous`) を使っている。案 A で変わるのは、autotools の代わりに CMake、自前の Dockerfile の代わりに `container: ubuntu:22.04`、どの push でも動くのをやめてタグだけにすること、linuxdeploy の版を固定すること。
- master の `utils/appimage/rcssmonitor.png` は、中身が 512×512 の JPEG (RoboCup のロゴ) で、拡張子が合っていない。試す版では、アプリの窓のアイコン `icons/rcss.xpm` (48×48) を PNG にした `icons/rcssmonitor.png` を使う。

## 3. 現状の rcssmonitor

### 3.1 構成

| ファイル | 役割 |
|---|---|
| `src/main_window.cpp` | メニュー、ツールバー、キー (`createActions*`)、審判操作。`createFieldCanvas()` で `FieldCanvas` を中央のウィジェットにする |
| `src/field_canvas.cpp` | 2D の描画先。`QWidget` (定義 `USE_GLWIDGET` があれば `QGLWidget`。Phase 3 で `USE_GLWIDGET` を消した)。painter を順に呼ぶ。マウス操作、右クリックのメニュー |
| `src/field_painter.cpp`、`player_painter.cpp`、`ball_painter.cpp`、`draw_info_painter.cpp` | 2D の描画。選手の状態の色、キャッチ・タックル・キックの範囲、オフサイドラインなどの計算もここにある |
| `src/disp_holder.cpp` | 受け取ったフレームを持つ。ライブとログの両方 |
| `src/log_player.cpp` | `QTimer` で 1 フレームずつ進める。フレームのあいだの補間はない |
| `src/monitor_client.cpp` | ライブ接続。develop では UDP だけ (TCP / WebSocket は `feature/support-tcp-ws` の `*_monitor_transport.cpp`) |
| `src/options.cpp` | 設定。`QSettings` で `~/.rcssmonitor.conf` に保存。コマンドラインのオプションもここ |
| `src/config_dialog.cpp` | 設定ダイアログ (Show、Canvas、Color、Font のタブ) |
| `src/player_type_dialog.cpp` | 選手タイプの一覧 |
| `rcss/rcg/` | ログとメッセージの解析。`parser_simdjson.cpp` (JSON、同梱の simdjson を使う)、`parser_v1〜v4.cpp`、`types.cpp` (パラメータの既定値)、`util.cpp` (プレイモードの名前など)。`nlohmann/json.hpp` も同梱されていた (使われておらず、Phase 2 で消した) |
| `test/` | develop にはない (単体テストがない)。`feature/support-tcp-ws` には Qt Test の `test_message_splitter.cpp` がある。この計画では `feature/ci` で `test_rcg_parser.cpp` とフィクスチャ (`test/data/`) を足した (§14) |

### 3.2 ビルド

- CMake (`CMakeLists.txt`、`src/CMakeLists.txt`)、autotools (`configure.ac`、`m4/ax_have_qt.m4`)、qmake (`rcssmonitor.pro`) の 3 つがある。qmake は D11 で廃止した (`feature/ci`)。
- どれも Qt5 に決め打ち (`find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)`)。`feature/support-tcp-ws` では、WebSockets と Test を任意で探す。
- C++17、CMake 3.5 以上。
- qmake のビルドは develop で通らない (2026-11-11 に確かめた)。`src/src.pro` に `HAVE_ARPA_INET_H` の定義がなく、`disp_holder.cpp` の `ntohs` で止まる。版は 16.0.0 のまま、使っていない boost にリンクする。D11 で廃止した。CMake と autotools は通る。
- develop には CI がない (`.github/workflows` がない)。
- master にだけ、PR #59 の AppImage のワークフロー (`.github/workflows/main.yml`、名前は `ci`) と `utils/appimage/` がある。任意のブランチとタグへの push で動き、Ubuntu 20.04 の Docker イメージ (`qt5-default` で Qt 5.12.8) の中で autotools でビルドし、`CMakeLists.txt` の版のリリースがなければ作る。ビルドとテストの確認はしない。D10 で廃止に決め、ユーザが 2026-11-11 に GitHub の画面で無効にした。ファイルは 20.0.0 を master に取り込むときに消す。

### 3.3 Qt6 で変わる箇所 (2026-11-11 の `src/` と `rcss/rcg/` で数えた。moc の生成物と同梱ライブラリを除く。develop で数え直しても同じ数。`feature/support-tcp-ws` では `QT_VERSION_CHECK` が 1 つ多い)

| 内容 | 箇所数 | Qt6 での書き方 |
|---|---|---|
| `QKeySequence` を `Qt::CTRL + Qt::Key_X` / `Qt::ALT + …` で作る | 19 + 5 | `Qt::CTRL \| Qt::Key_X` (Qt 6 で `+` は非推奨。Qt 5.15 でも `\|` は使える) |
| `QLayout::setMargin()` | 34 | `setContentsMargins()` (Qt6 で削除) |
| `QMouseEvent::pos()`、`globalPos()` | 13、1 | Qt6 では `position().toPoint()`、`globalPosition().toPoint()` (`pos()` は Qt 6 で非推奨)。`position()` は Qt 5.15 にないので、両対応では小さな関数で `QT_VERSION_CHECK` を分ける。 → 訂正 (Phase 3): `QMouseEvent::pos()` は Qt 6.2〜6.10 で非推奨ではない (非推奨なのは `globalPos()`、`x()`、`y()` と、ほかのイベントの `pos()`)。使っているのは `QMouseEvent::pos()` と `QContextMenuEvent::globalPos()` だけなので、直さなくてよい |
| `QWheelEvent::delta()`、`orientation()` | 2、1 | `angleDelta()` (Qt6 で削除) |
| `QSignalMapper` | 2 | ラムダの接続 |
| `QFontMetrics::width()` | 数か所 (一部はすでに `QT_VERSION_CHECK` で `horizontalAdvance` と分けてある) | `horizontalAdvance()` |
| `QTime::start()` / `elapsed()` | 1 | `QElapsedTimer` |
| `QGLWidget` (`USE_GLWIDGET` のとき) | 3 | `QOpenGLWidget` (Qt6 では `Qt6::OpenGLWidgets`)。2D でこの定義を使っている人がいないなら、定義ごと消す |
| `QT_VERSION_CHECK` の分岐 | 22 | Qt 5.15 より古い分岐を消して整理する |
| `QAction` (93 か所) | — | Qt6 では QtGui に移ったが、`#include <QAction>` のままで両方で動く。`#include <QtWidgets/QAction>` があれば直す |

`std::endl` は Qt と関係ないのでそのままでよい。`QRegExp`、`QDesktopWidget`、`QTextCodec` は使われていない。

### 3.4 ブランチ (2026-11-11)

- `develop` (01f880a、19.0.1): 作業の土台。
- `master` (42848cd、19.0.1): develop に、PR #59 の AppImage のワークフローと `utils/appimage/`、`.gitignore` を足したもの (§3.2)。それ以外は develop と同じ。
- `feature/support-tcp-ws` (45bca98): develop (01f880a) に TCP / WebSocket 対応の 5 コミット。未取り込み (D7)。計画書 `docs/TRANSPORT_PLAN.md` (リポジトリ未登録) がある。
- `feature/ci`: この計画の最初のブランチ (§9.1)。develop (01f880a) から 2026-11-11 に作った。
- `proto/3d-gl`: 3D の試作 (取り込まない)。develop (95965dc) から 2026-11-12 に作った (§14)。
- `fix/qt-selection`: CMake の Qt の選び方の修正と、試作の記録。develop (95965dc) から 2026-11-12 に作った (§14)。
- `origin/feature/json-reformat` (2024-03): 旧パーサ `rcss/rcg/parser_json.cpp` と `parser.cpp` を新しい JSON 形式に合わせた 2 コミット。今の develop は `parser_simdjson.cpp` を使っているので、内容を確かめて、§7 の作業に取り込むか捨てるかを決める。 → 取り込まない (Phase 2、§14)。
- `origin/develop_observation` (2022-11)、`origin/fix/qt-asprintf` (master に取り込み済み) は、この計画と関係しない。

### 3.5 TCP / WebSocket 対応を後で取り込むとき

D7 のとおり、この計画のブランチは `feature/support-tcp-ws` を含まない。採用が決まって取り込むときは、次のことが起きる。

- 両方が変えるファイル: `src/main_window.cpp`、`src/options.{h,cpp}`、`CMakeLists.txt`、`src/CMakeLists.txt`、`configure.ac`、`src/Makefile.am`、`src/src.pro`、`README.md`、`NEWS`、`test/CMakeLists.txt` (どちらも新しく作る)。機能が別なので、衝突の解決は難しくない見込み。
- テストの仕組みは、この計画でも `feature/support-tcp-ws` と同じ形にする (最上位の `CMakeLists.txt` で Test を探し、見つかれば `enable_testing()` と `add_subdirectory(test)`。`test/CMakeLists.txt` にテストごとの `add_executable` と `add_test`)。取り込むときは、両方のテストを並べるだけで済むようにする。Phase 1 で `test/CMakeLists.txt` はテストを足す関数 `rcssmonitor_add_test` を使う形になったので、`message_splitter` もこの関数で足す。
- Phase 1 で、アプリのソース (`main.cpp` 以外) は `src/CMakeLists.txt` のライブラリ `rcssmonitor_core` にまとめた。TCP / WebSocket 対応で足したソースは、`add_executable` ではなく `rcssmonitor_core` に足す。
- Phase 3 (Qt6 対応) は、TCP / WebSocket 対応で足したファイル (`connect_dialog.cpp`、`*_monitor_transport.cpp`、`message_splitter.cpp`) を含まない。取り込むときは、develop を `feature/support-tcp-ws` にマージし (プッシュ済みなので rebase はしない)、そこでこれらのファイルの Qt6 対応と、WebSockets を探す CMake の Qt6 対応 (`Qt${QT_VERSION_MAJOR}::WebSockets`) を行ってから、PR にする。Qt6 で WebSocket を使うには `qt6-websockets-dev` が要る (作業マシンには未導入)。
- 計画書の行番号 (§6) のうち、`main_window.cpp` の行は、取り込んだ後にずれる。

## 4. 参照実装 (rcssmonitor-web) の 3D 表示

### 4.1 機能の一覧

web 版の README の「3D view」と、リリースノート `docs/release-notes/v20.0.0-beta.2.md` の「New: the 3D view」がまとまった説明になっている。

- **切り替え**: View > 3D View、ツールバーの 3D ボタン、キー D。2D と 3D を切り替えても、フレーム、選手の選択、審判メニューの状態が保たれる。
- **カメラ** (View > 3D Camera、ツールバーの選択欄、キー E で次へ): free (左ドラッグで回す、Shift / Ctrl + ドラッグで動かす、ホイールで寄る)、broadcast、close-up (ボール)、close-up (選択した選手)、top、ゴール裏 (左、右)、選択した選手の視点。ほかのカメラもドラッグすると free になる。
- **ピッチ全体の視点**: I でピッチ脇から、Ctrl+I で次の視点 (9 個)。メニューとツールバーのボタンからも選べる。2D では I は今までどおりフィールドを画面に合わせ、Ctrl+I は「3D 用」と知らせるだけ。
- **選手視点**: 肩越し (既定) か目の高さ。向きは頭 (体 + 首) か体かその中間。動きは滑らか (角速度に上限) か即時。画角は一定 (既定 120°) か視野の幅に合わせるか、なめらかに追うか。描くものはすべてか、視野の中だけか、サーバの視覚の規則か (既定)。設定ダイアログの Player Camera タブ。
- **2D と同じ描画**: 選手の状態の色、カード、視野、足元の円 (キック可能範囲か Player Size) の中の体と顔の向きの線、Show タブの重ね描き (注視点、キャッチ・タックル・キックの加速範囲、指差し、オフサイドライン、ボールの予測軌道、選手の種類、不正な守備の背番号の色)。範囲の数値 (タックルの確率など) は HUD に書く。
- **背番号とスタミナ**: 背番号は頭の上 (画面上で Player フォントの 2 倍までに抑える)。真上のカメラでは 2D と同じくキック可能範囲の右に、2D と同じ大きさで書く。スタミナとスタミナ容量は頭の上のバー (S キーで、なし → スタミナ → 容量 → 両方)。
- **審判操作**: 右クリックで、選手を狙えばその選手 (カード) とその足元、地面ならピッチと周りの点。ピッチの外や地平線より上では位置なし。
- **人型** (3D タブの Players): CC0 のマネキン。速さで歩き・走り、ログからキック、タックル、キャッチ、指差し、ファウルを受けて座り込む。止めても戻しても同じ姿勢。真上のカメラと、向きのわからない選手は円柱。
- **両端の区別**: ゴールのネット、ピッチのまわりの広告板、ゴール裏の空中の看板をチームの色にする (広告板と看板は切り替え可)。方位の帯とミニマップを重ねられる。
- **見た目** (3D タブ、どれも切り替え可、既定でオン): 太陽の影、芝の刈り目の縞、空、両タッチライン沿いの観客席、五角形の模様で転がるボール。
- **ボールを見つける助け** (3D タブの Ball): 地面の円 (既定 0.7 m) と、最近のサイクルの軌跡 (飛行機雲)。
- **ボールの高さ**: show に `ball.z` があれば空中に描き、真下に影を出す (計画中の 2.5 次元シミュレータ用。今のサーバとログにはない)。
- **性能**: 1920×1080、デスクトップの GPU で、すべてオンでも 60 fps (Firefox で計測)。

### 4.2 web 版のモジュールと、rcssmonitor での担当 (仮の名前)

| web 版 (`src/view/` 以下) | 行数 | 内容 | rcssmonitor での担当 |
|---|---|---|---|
| `render3d/render3d.ts` | 1927 | 3D の描画の中心。シーンの組み立て、選手、ラベル、バー、カメラの更新、審判操作の当たり判定、点検用の状態 | `FieldView3D` (QOpenGLWidget) と `Scene3D` に分ける |
| `render3d/cameras.ts` | 230 | カメラの種類、中継風・ゴール裏・寄りの位置、視点、なめらかな追従 | `Cameras3D` (三次元の計算だけ。GL に依存しない) |
| `render3d/player-camera.ts` | 204 | 選手視点のスタイルと動き | `PlayerCamera3D` |
| `render3d/sight.ts` | 111 | 選手が知覚するもの (視野、サーバの規則) | `Sight` |
| `render3d/interpolate.ts` | 81 | フレームのあいだの補間 | `Interpolate` |
| `render3d/scene-math.ts` | 279 | 座標の対応、ピッチ全体を収める距離、スタミナの色、色の明暗など | `SceneMath` |
| `render3d/overlays.ts`、`../overlay-math.ts` | 435、235 | 地面の重ね描きと、2D と共有する計算 | 2D の painter から計算を切り出して共有する (`OverlayMath`) |
| `render3d/stadium.ts` | 270 | 広告板と看板 | `Stadium3D` |
| `render3d/scenery.ts` | 402 | 芝の縞、空、観客席 | `Scenery3D` |
| `render3d/ball.ts`、`ball-aids.ts` | 189、338 | ボールの模様と転がり、円と軌跡 | `Ball3D`、`BallAids3D` |
| `render3d/field-texture.ts` | 55 | 2D の field painter で地面の絵を描く | `FieldPainter` を `QImage` に描いて使う |
| `render3d/figures/` (`figures.ts`、`pose.ts`、`gltf.ts`) | 152、270、413 | 人型。動作の決め方、姿勢、glTF の読み込みと骨の操作 | `Figures3D`、`Pose`、`GltfModel` |
| `hud/guides.ts`、`hud/hud.ts`、`hud/score-board.ts` | 261、183、62 | 方位の帯、ミニマップ、HUD の文字、スコアボード | QPainter で 3D の上に描く (§5.5) |
| `player-style.ts` | 107 | 2D と同じ選手の色 | 2D の `PlayerPainter` の色の決め方を共有する |

### 4.3 主な定数 (web 版の値。迷ったら web 版のソースを正とする)

| 項目 | 値 | web 版の場所 |
|---|---|---|
| 座標 | ワールドの X = フィールドの x、Z = フィールドの y、Y が上。フィールドの角度 d は `rotation.y = -d` (度→ラジアン)。glTF のモデルの前は +Z なので `rotation.y = π/2 - body` | `scene-math.ts`、`figures/gltf.ts` |
| 縦の画角 | 35° (選手視点以外) | `cameras.ts` `CAMERA_FOV` |
| 中継風 | ピッチ全体が収まる距離の 0.55 倍 (16:9 で約 72 m)、仰角 32°。ボールの地面上の位置を、タッチラインとゴールラインから 10 m 内側までで追う | `broadcastPose` |
| なめらかな追従 | 時定数 0.35 s。1 フレームに進める時間は 0.1 s まで。0.02 m 以内で止まったとみなす | `smoothToward`、`SETTLED` |
| ゴール裏 | x = ±(52.5 + 18)、高さ 10 m。見る点はゴールとボールの中間 (横は 0.6 倍)。画角 45° | `goalPose` |
| 真上 | 正射影。地面の絵の範囲を、高さの 15% を HUD に残して収める | `topHalfSize` |
| 寄り | 距離 12 m (3〜60)、仰角 20° (5〜85)。選手は高さ 1 m を見る。時定数 = 0.0089 s/m × 距離 (0.05〜0.35 s)。ホイール 1 ノッチで約 1.1 倍 | `closeupPose`、`closeupTimeConstant` |
| 視点 | 9 個の方位と仰角 (`VIEWPOINT_ANGLES`)。ピッチと周り 2 m の四隅が、横 97%、縦 85% に収まる最短の距離を式で求める | `fitView` |
| 選手視点 | 既定は肩越し (6 m 後ろ、3 m 上)、頭、滑らか、一定の画角 120° (横)、サーバの規則。τ 0.5 s、ω 90°/s。目の高さ 1.65 m | `player-camera.ts` |
| 視覚の規則 | 視野の中は `player_max_observation_length` まで。`unum_far_length` 20 m〜`unum_too_far_length` 40 m で背番号が薄れ、背番号がなければ向きもない。`team_far_length` 40 m〜`team_too_far_length` 60 m でチームが灰色に。視野の外は `visible_distance` 3 m 以内を感じる | `sight.ts` |
| 背番号 | 1.3 m の高さのラベル (文字は 40/64)。画面上の文字の高さは Player フォントの 2 倍まで。縮めるときは頭の上に向かって縮める | `fitLabels`、`LABEL_MAX_FONT` |
| 地面の層の高さ | 影の面 0.01、足元の円 0.02、ボールの円 0.028、ボールの影 0.03、視野 0.035〜0.04、重ね描きの線 0.045 m | 各ファイル |
| 広告板 | タッチラインとゴールラインから 5.5 m、高さ 1 m、10 m ごとにチーム名、明るさはチームの色の 0.32 倍 | `BOARDS`、`BOARD_SHADE` |
| 看板 | ゴールラインの 22 m 後ろ、幅 50 m、高さ 8 m、下端 12 m | `BANNER` |
| 影 | 太陽の向き (-60, 65, -30) (左奥、仰角 44°)。地図は 4096 px (GPU が 8192 を扱えれば) か 2048 px。地面の絵の範囲 + 4 m、高さ 3 m までを覆う。PCF、半径 2、bias -0.0005、normalBias 0.02。影だけを描く面の濃さ 0.4。影を落とすのは選手とゴールの枠。真上のカメラでは描かない | `render3d.ts` `fitSunShadow` |
| 芝の縞 | 幅 5.25 m (ピッチに 20 本)。芝の色の 1.05 倍と 0.92 倍 | `GRASS` |
| 空 | 天頂 #3d7ccc、地平線 #d4e4f0、地平線より下 #6f9f7f。半径 1900 m の球をカメラに付ける。霧 600〜1500 m。外の地面は 3000 m 四方 | `SKY` |
| 観客席 | 広告板の 2 m 外から 16 段 (奥行き 0.85 m、高さ 0.42 m、最前列の下に 1.2 m の壁)。長さは広告板と同じ。観客の 45% がその半分のチームの色 | `STANDS` |
| ボール | 模様は正二十面体の頂点に黒い五角形 12 個 (角は隣の頂点への 1/3)。転がりは 1 サイクルに 0.8π まで。5 m を超える移動は置き直し | `ball.ts` |
| ボールの円 | 半径 0.7 m (0.1〜5)。線は 0.06 m か画面上 2 px の太いほう。内側の濃さ 0.12 | `ball-aids.ts` |
| 軌跡 | 5 サイクル (1〜30)。頭の幅はボールの直径の 1.5 倍か画面上 6 px の太いほう。濃さ 0.75。古いほど細く薄い | `ball-aids.ts` |
| 人型 | 背の高さ 1.85 m × 1.2。クリップ Idle_Loop、Walk_Loop、Jog_Fwd_Loop、Sprint_Loop (それぞれ 1.4、3.6、6.5 m/s)。骨 Head、spine_02、thigh_l/r、calf_l/r、upperarm_l/r (65 本のうち姿勢で動かすもの)。ユニフォームの材質 M_Main | `figures/gltf.ts` |

### 4.4 ログと rcssserver の仕様で効くこと

- **タックルのビット**: rcssserver は、選手を止めているあいだ (`tackle_cycles`、既定 10) Tackle / TackleFault のビットを残す。ファウルになったタックルは止めないので、ビットは 1 フレームだけ。人型は、ビットが続くフレームのあいだだけ滑らせる (`Player::tackle`、`resetState`)。
- **FoulCharged のビット**: ファウルを受けた選手は `foul_cycles` (既定 5) のあいだ止まり、そのあいだビットが残る。
- **Catch のビット**: 1 フレームだけ。キャッチの姿勢は `catch_ban_cycle` のあいだ保つ。
- **Kick のビット**: キックのあいだ (`kick_cycles`) 残る。ふつうは 1 フレーム。
- ライブの show には、接続していない選手も `state: 0` で入り、x = -(unum × 3 × side)、y = -37 に置かれる。
- 詳しくは `rcssmonitor-web/docs/protocol.md` §5。

## 5. 設計

### 5.1 全体の構成

```
MainWindow
 └ QStackedWidget (中央)
     ├ FieldCanvas      (2D。今のまま)
     └ FieldView3D      (3D。QOpenGLWidget)
          ├ Scene3D      シーンの組み立てと描画 (GL)
          ├ Cameras3D / PlayerCamera3D / Sight / Interpolate  (計算だけ。GL に依存しない)
          ├ Figures3D    人型 (cgltf、姿勢の計算、骨の行列)
          └ HUD          QPainter で 3D の上に描く
DispHolder / LogPlayer / Options  (2D と共有)
```

- 2D と 3D は `DispHolder` と選手の選択を共有する。切り替えはウィジェットを入れ替えるだけで、再生の位置と選択はそのまま。
- 3D の描画に要る計算 (カメラ、補間、知覚、姿勢、軌跡など) は、GL に依存しないクラスに置き、単体テストで確かめる。web 版でも同じ分け方にした。
- `ENABLE_3D` が OFF、または実行時に OpenGL 3.3 core の文脈が作れないときは、3D のメニューとキーを無効にし、理由をステータスバーに出す。

### 5.2 時間と補間

- 今の `LogPlayer` は `QTimer` で 1 フレームずつ進めるだけで、フレームのあいだの位置を持たない。3D は、今のフレーム、次のフレーム、その間の割合 (alpha) で描く。`LogPlayer` に「最後に進めてからの経過時間 / 間隔」を返す関数を足す (`QElapsedTimer`)。止めているときは alpha = 0。
- 補間は、位置は線形、角度は短い回り方向。プレイモードが変わる前後と、大きな瞬間移動 (選手 1 サイクルに 2 m 超、ボール 5 m 超など) の前後では補間しない (web 版 `interpolate.ts`)。
- ライブでは 1 サイクル遅れて描き、最新と 1 つ前のあいだを補間する (動きをなめらかにするため)。
- 描画の呼び出し: 再生中、カメラが動いているあいだ、人型の姿勢が変わるあいだは、毎フレーム `update()` する (`QOpenGLWidget::frameSwapped` に続けて次を頼む)。止まって何も動かなくなったら呼ばない。
- **姿勢、ボールの回転、軌跡は、ログのフレームと alpha だけから決める** (描くたびに状態を積み上げない)。止めても、コマ送りで戻しても、同じフレームなら同じに見える。web 版では、描くたびに時間を積み上げたせいで、戻すと脚が止まったまま滑り、同じフレームでも姿勢が変わった (§11)。

### 5.3 描画の基盤

OpenGL 3.3 core (`QSurfaceFormat` で要求。macOS は 4.1 core まで)。`QOpenGLFunctions_3_3_Core` を使う。マルチサンプリング 4×。

| 描き方 | 使う所 |
|---|---|
| 光の当たらないテクスチャ | 地面の絵、外の地面、看板、広告板、観客席 (光あり) |
| 光の当たる単色 (Lambert) のインスタンス描画 | 選手の円柱と頭、向きの印、ゴールの枠 |
| 半透明の単色のインスタンス描画 | 足元の円、減衰の輪、選択の輪、視野の扇形 |
| 骨つきメッシュ (Standard に近い光) | 人型 (ユニフォームの色を材質ごとに変える) |
| 線 (頂点に色) | 重ね描き、視野の大きな扇形、ネット |
| 点 | ボールの予測軌道の点 |
| ビルボード (画面を向く四角にテクスチャ) | 背番号、カード、スタミナのバー |
| 影の深さ (インスタンスと骨つき) | 太陽の影の地図 |
| 影だけを描く面 | 地面に落ちる影 |
| 空のグラデーション | 空の球 |
| ボールの模様 | ボール (方向から五角形を求める) |
| ボールの円 | ボールの円 (距離から線と内側を求める) |
| 霧 | 外の地面を地平線に溶かす |

- フレームごとに中身が変わるバッファは、大きさを決めて一度だけ作り、使い回す (`glBufferSubData`)。web 版では、毎フレーム新しいバッファを作って GPU のメモリが漏れたことがある。
- 影の地図は、真上のカメラでは描かない (影の面も隠す)。影の設定がオフなら描かない。
- 描く順: 不透明 → 地面の層 (高さで重ねる) → 半透明 → 空 → ビルボード (深さの検査なし)。

描画の基盤の構成 (4-3b、§14)。

- **方針 (ユーザ、2026-11-12)**: OpenGL は `src/view3d/gl/` の部品の中に隠す。OpenGL の関数 (`glXxx()`) と定数 (`GL_XXX`) を直に使うのは `gl/` の中だけにし、ふだんの実装 (`Scene3D`、`FieldView3D` など) は `gl/` の部品を使う。`Scene3D` は「どのシェーダとどの描き方で、どの形を描くか」を並べるだけにする。新しい描き方が要るときは、`gl/` に部品を足すか、部品に関数を足す。
- テスト `gl_isolation` が、`src/` の C++ のソース (`src/view3d/gl/` を除く) から、コメントと文字列を除いて、OpenGL の関数の呼び出しと定数を探し、あれば失敗する。Qt の型や関数 (`GLuint`、`QOpenGLWidget`、`initializeOpenGLFunctions()`) はよい。テストのコードは対象外 (画面外の描画の準備で OpenGL を直に使う)。

| 部品 | 役割 |
|---|---|
| `shaders/*.vert`、`*.frag` (`view3d_shaders.qrc`) | GLSL のソース。`#version` の行と属性の番号 (`A_POSITION` など) は `ShaderProgram` が先頭に足す |
| `gl/attributes.h` | 頂点の属性の番号。頂点のバッファとシェーダで共有する |
| `gl/shader_program` | シェーダを読んでリンクし、uniform の場所を一度だけ引いて覚える (`set( "uViewProj", m )`) |
| `gl/instanced_mesh` | 形の頂点のバッファと、インスタンス (行列と色) のバッファ。1 回の描画命令で全部を描く。インスタンスのバッファは大きくなるときだけ作り直す |
| `gl/texture_2d` | 画像のテクスチャ (ミップマップ、異方性フィルタ) |
| `gl/billboard_mesh` | カメラを向く四角 (ビルボード) のインスタンス描画。背番号とカード (4-5) |
| `gl/render_state` | 描き方 (深さの検査と書き込み、裏面を描かない、半透明)。どの段も自分の描き方をすべて当て、前の段の残した状態に頼らない |
| `gl/vertex_batch` | 頂点ごとに色を持ち、毎フレーム作り直す頂点の列を、線、点、三角形で描く。点線は頂点の距離でシェーダが間を抜く。重ね描きと視野 (5-1) |
| `gl/frame` | フレームの始めと終わり (`GLFrame::begin()`、`end()`)。描き先のフレームバッファと消去、QPainter (HUD) が残しうる状態の戻し、QPainter に渡す状態 (4-5) |
| `meshes_3d` | 形の頂点 (GL に依存しない。単体テストで、三角形の巻き方向と法線を確かめる) |

### 5.4 各部品

web 版の対応するファイルを読み、同じ値と同じ考え方で作る。値は §4.3。

- **地面**: 2D の `FieldPainter` で、1 m を 24 px、ペンを 3 倍にした `QImage` に描き、テクスチャにする。ゴールは描かない (3D で作る)。芝の縞は、芝の塗りの後、線の前に描く (`FieldPainter` に「芝の上、線の下に描く」引数を足す)。地面の絵は、色やオプションが変わったときだけ描き直す。
- **ゴール**: 枠 (柱とクロスバー、高さ 2.44 m) と半透明のネット (チームの色)。
- **選手**: 円柱 (胴)、球 (頭)、足元の円、体の向きの黒い円錐、顔の向きの赤い円錐、カード。色は 2D の `PlayerPainter` と同じ決め方 (状態の色、スタミナで暗く、選択)。部品ごとにインスタンス描画。足元の円の中に、体の向き (選手の線の色) と顔の向き (首の色。視野を描くときは描かない) の線。
- **ボール**: 半径はオプションの Ball Size か `ball_size`。高さがあれば空中に描き、影は真下 (高さで薄く)。模様と転がりは `ball.ts`。
- **ラベル**: 背番号 (と選手の種類) を `QImage` に書いてテクスチャにし、ビルボードで描く。大きさの上限と、真上のカメラでの置き方は `fitLabels`、`labelPlace`。
- **スタミナのバー**: カメラの下向きにずらして、背番号の下に並べる。色は `staminaHue` (残りが `recover_dec_thr` と `effort_dec_thr` を下回ると赤)。選手視点のサーバの規則では、選んだ選手のバーだけ。
- **重ね描き**: 2D の `PlayerPainter` と `BallPainter` から計算 (キャッチ範囲の半径、タックルの確率、キックの加速、予測軌道、オフサイドライン) を切り出して共有し、地面に線で描く。範囲の数値は HUD に書く。
- **視野**: 選手ごとの扇形 (視野の幅、`visible_distance`)、選択した選手は 60 m の線の扇形。
- **広告板と看板**: `stadium.ts`。チーム名とロゴ (Team Graphic がオンのとき) をテクスチャに書く。
- **景色**: `scenery.ts`。観客席の面はピッチ側と上だけに向け、裏と端は外に向ける (片面だけ描く)。寄りのカメラがタッチライン際で席の中に入っても、ピッチが隠れない。
- **ボールの円と軌跡**: `ball-aids.ts`。線の太さの下限は、カメラからの奥行きで 1 px の長さを求めて決める。
- **影**: §4.3。
- **人型**: §5.8。
- **審判操作**: 画面の点から光線を出し、選手 (見えない当たり判定用の円柱。人型は 1.2 倍) か地面との交点を求める。ピッチと周りの外、地平線より上は位置なしにして、Drop Ball と Free Kick を無効にする (理由をツールチップに)。今の `FieldCanvas` の右クリックのメニューを 3D でも使う。

### 5.5 HUD

- `QOpenGLWidget` の `paintGL` の最後に、`QPainter` で 2D の上書きを描く: スコアボード (2D と同じ painter)、チームのロゴ、範囲の数値、方位の帯、ミニマップ、メッセージ。
- 方位の帯: 画面の上の中央、幅 480 px まで。カメラの向きを中央に、時計回りを右に。両ゴール (チームの色の L、R) とボールの向き (`guides.ts`)。
- ミニマップ: 右下に 2D と同じピッチ。選手、ボール、カメラの位置と見ている範囲。
- どちらも設定で「オフ / 選手視点だけ / すべてのカメラ」。真上のカメラでは方位の帯を出さない。

### 5.6 入力

- マウス: free カメラは、左ドラッグで回す、Shift (または Ctrl) + 左ドラッグで動かす、ホイールと中ボタンで寄る (web 版は three.js の OrbitControls。同じ操作を自分で書く)。ほかのカメラでドラッグすると free になる。寄りのカメラではホイールで距離を変え、設定に残す。右クリックは審判のメニュー。 → 4-4: 2D ではホイールでフレームを進め戻すので、3D でホイールを受けないカメラ (真上) では、今までどおりフレームを動かす。
- **2D と操作をそろえる (ユーザ、2026-11-12、5-1b)**: 3D でホイールがカメラを寄せると、2D (ホイールでフレームを進め戻し、Z と X で拡大と縮小) と操作が違って混乱する。一般の 3D ビューワの操作と違っても、2D にそろえる。3D でも、ホイールはフレームを進め戻す。Z で寄り、X と Ctrl+Z で離れる (2D と同じ 1.5 倍)。追うカメラは追ったまま距離が変わり、真上のカメラは見える高さが変わって、寄ったときはボールのまわりを (地面の絵の中で) 映す。free カメラは見ている点に向かって寄る。カメラを変えると倍率は戻る。I は倍率を戻す (6-2 でピッチ全体の視点に置き換えた)。寄りのカメラ (Phase 6) の距離も、ホイールではなく Z と X で変える。マウスのドラッグ (回す、動かす、中ボタンで寄る) は今のまま。web 版も後で同じ操作にそろえる (§0.2)。
- キー (web 版の `docs/parity.md` §8 と `src/ui/shortcuts.ts`):

| キー | 機能 | rcssmonitor の今 |
|---|---|---|
| D | 2D / 3D の切り替え | 空き |
| E | 次のカメラ | 空き |
| Z、X、Ctrl+Z | 3D ではカメラを寄せる・離す。2D では今までどおり (5-1b) | Zoom In、Zoom Out |
| I | 3D ではピッチ脇の視点 (free)。2D では今までどおり (5-1b から 6-1 まで、3D では倍率を戻した。6-2 で視点に置き換えた) | Fit Field Size |
| ホイール | 2D と 3D の両方で、フレームを進め戻す (5-1b) | 同じ (2D) |
| Ctrl+I | 3D では次の視点。2D では「3D 用」と知らせる | 空き |
| S | スタミナの表示を、なし → スタミナ → 容量 → 両方で回す (3D のバーにも効く) | 同じ (2D) |

Shift を使うショートカットは作らない。関係のある別の操作は Ctrl を付ける (G / Ctrl+G、Z / Ctrl+Z と同じ形)。

### 5.7 設定

`Options` に足し、`~/.rcssmonitor.conf` に保存する。名前は web 版 (URL のパラメータ名と同じ) にそろえ、コマンドラインのオプションにも同じ名前で足す。範囲の外の値は読み込まない。

| 名前 | 既定 | 値 |
|---|---|---|
| `view_3d` | off | 3D で始める (コマンドラインは、今のオプションに合わせて `--view-3d`。Phase 4 の 4-1) |
| `camera_3d` | broadcast | free / broadcast / closeup-ball / closeup-player / top / goal-left / goal-right / player (コマンドラインは `--camera-3d`。4-4 では寄りと選手視点を除く 5 つ、6-1 で寄りの 2 つ、6-3 で選手視点を足した。知らない名前は読まない) |
| `closeup_distance_3d`、`closeup_elevation_3d` | 12、20 | 3〜60 m、5〜85° (設定ファイルのグループ `View3D`。コマンドラインは `--closeup-distance-3d`、`--closeup-elevation-3d`。6-1) |
| `player_camera_position` | shoulder | eye / shoulder |
| `player_camera_direction` | head | head / body / middle |
| `player_camera_motion` | smooth | instant / smooth |
| `player_camera_fov` | fixed | view / eased / fixed |
| `player_camera_sight` | server | all / view / server |
| `player_camera_time_constant`、`player_camera_max_turn`、`player_camera_fixed_fov` | 0.5、90、120 | 0〜5 s、1〜3600 °/s、20〜120° (選手視点の 8 つは、グループ `View3D`。コマンドラインは `--player-camera-position` などの同じ名前。6-3) |
| `compass_3d`、`minimap_3d` | off | off / player / all (グループ `View3D`。コマンドラインは `--compass-3d`、`--minimap-3d`。6-5) |
| `boards_3d`、`banners_3d` | on | (グループ `View3D`。コマンドラインは `--boards-3d`、`--banners-3d`。6-4) |
| `player_model_3d` | simple | simple / figure |
| `shadows_3d`、`grass_stripes_3d`、`sky_3d`、`stands_3d`、`ball_pattern_3d` | on | |
| `ball_ring_3d`、`ball_ring_radius_3d` | on、0.7 | 0.1〜5 m |
| `ball_trail_3d`、`ball_trail_cycles_3d` | on、5 | 1〜30 (整数) |

設定ダイアログ:

- **3D タブ**: Close-up Camera (距離、仰角)、Guides (方位の帯、ミニマップ)、Players (Model、Shadows)、Ball (Pattern、Ring、Trail、Ring Radius、Trail cycles)、Stadium (Boards、Banners、Stands、Grass Stripes、Sky)、Default ボタン。
- **Player Camera タブ**: 4 つのスタイル、時定数、最大角速度、一定の画角、Player Sight、Default ボタン。
- どの項目にもツールチップで詳しい説明を付ける。数値の欄は、ツールチップに範囲を書き、範囲の外は端に寄せる (入力中はエラーの色)。選んだスタイルが使わない数値の欄は薄くする。
- web 版では、3D の項目が増えて 1 つのタブが縦に長くなりすぎたので、Player Camera を別のタブに分けた。項目がさらに増えたら、同じく分ける。
- → 6-6 で、6-1〜6-5 の設定 (Close-up Camera、Guides、Stadium の Boards と Banners、Player Camera タブのすべて) を作った。Players、Ball と、Stadium の残りの項目は、Phase 7 と 8 で足す。2 つのタブは 3D の入ったビルドにだけある。

### 5.8 人型

- モデル: `Mannequin.glb` (Quaternius の Universal Animation Library [Standard]、CC0) の、メッシュ、骨 65 本、クリップ 4 つ。web 版の `src/view/render3d/figures/models/README.md` に出典と削り方がある。D6 のとおり、圧縮なしで書き出し直して qrc に入れる。
- 読み込み: cgltf でメッシュ (位置、法線、骨の番号と重み)、骨の階層と逆バインド行列、クリップのキーフレームを読む。
- **姿勢は毎フレーム、ログから決める** (再生機を持たない):
  1. 歩いた距離の表 (`gaitAt`): 選手ごとに、フレームまでに歩いた距離 (体の向きと逆に動いた分は引く。1 サイクルに 2 m を超える移動は数えない) を、ログから一度だけ求めて覚えておく。
  2. クリップの時刻: 歩き・走り・全力疾走は「歩いた距離 / そのクリップの速さ」、待機は試合の時刻。速さで重みを決めて混ぜる (全力疾走 (v-5)/1.5、走り (v-2)/1.5、歩き v/0.8 を 0〜1 に切る)。
  3. 骨の局所の回転を、キーフレームから求めて重みで混ぜる。
  4. 動作の姿勢を重ねる (`pose.ts`): 首 (頭の骨を首の角度で回す)、キック (前後のフレームのビットから、振りかぶり、蹴る瞬間、振り抜き。蹴る脚はボールの側)、タックル (ビットが続くあいだ滑り、最後から 0.3 サイクル保って 1.5 サイクルで起きる)、ファウルを受けた (座り込んで手を後ろにつく)、キャッチ (両手を上げる)、指差し (右腕)。動作は、ビットが続くフレームの範囲 (`actionRun`) で決める。
  5. 骨の行列を求めて、骨つきの描画に渡す。
- 回転は、クリップの姿勢の上に「毎回はじめから」重ねる。web 版 (three.js) では、アニメーションの再生機が値の変わらない骨を書き直さないため、重ねた回転が描くたびにたまり、首や体が回り続けた (§11)。自前で毎回求めれば起きない。
- 大きさは実物の 1.2 倍。真上のカメラでは円柱で描く。選手視点のサーバの規則で、向きがわからない選手も円柱で描く (人型は形で向きを見せてしまうため)。人型には向きの円錐を付けない。
- 当たり判定は、全員分の見えない円柱で行う (人型は 1.2 倍)。

## 6. 不具合の修正

web 版の `docs/parity.md` §7。行番号は 2026-11-11 の develop (01f880a) で確かめた (`feature/support-tcp-ws` と違うのは B11 だけ。あちらでは 2394-2429)。直し方は web 版の実装を参考にする。

| # | 不具合 | 場所 | 直し方 |
|---|---|---|---|
| B1 | 65,535 フレームを超えると、ライブ表示が止まる | `src/disp_holder.cpp:65` (`MAX_SIZE = 65535`)、`:249` | 上限で受け付けをやめない。上限を大きくするか、古いフレームから捨てる (どちらにするかは実装のときに決め、§14 に記録する)。web 版は上限なし。→ ライブ接続では、65,535 を超えたら古いものから 4,096 フレームをまとめて捨てる。ログファイルは全部持つ (Phase 1、§14) |
| B2 | JSON の `vq` を bool として入れている。`view_quality_` は `'h'` / `'l'` の char なので、JSON のログでは視覚の質がいつも低いと判定される | `rcss/rcg/parser_simdjson.cpp:478` | `'h'` / `'l'` を入れる |
| B3 | JSON の未知のキーで、解析全体が中断する | `rcss/rcg/parser_simdjson.cpp:85-164` | 未知のキーは飛ばす (§7) |
| B4 | JSON の選手を、配列の順に格納している | `rcss/rcg/parser_simdjson.cpp:447-525` | `side` と `unum` で格納する (§7) |
| B5 | `foul_push_r` が PM_Null になる (`{ "foul_push_l", PM_Foul_Push_Right }` の打ち間違い) | `rcss/rcg/util.cpp:854-855` | `foul_push_r` にする |
| B6 | テキスト rcg のチームグラフィックで、16 文字以上の文字列が壊れる (`char buf[16]` に `%15[^"]` で読むので、16 文字以上では閉じの `"` と合わず、読み取りの位置がずれる) | `rcss/rcg/parser_v4.cpp:625-709` (`:696-697`) | 引用符の中を、長さを決め打ちにせずに読む (`std::string` で次の `"` まで) |
| B7 | パラメータの既定値が古い (`player_speed_max` 1.2、`stamina_max` 4000 など約 40 項目)。`effort_inc` が対応表にない | `rcss/rcg/types.cpp:547-760` | asio 版 rcssserver 19.0.0 の既定値にそろえる (web 版 `src/core/param-defaults.ts` の値) |
| B8 | テキスト rcg に、`(名前` で始まらない行が 1 行でもあると、ログ全体の読み込みが止まる | `rcss/rcg/parser_v4.cpp:95-102, 123-128` | その行を飛ばして続ける |
| B9 | オフサイドラインが x = 0 とボールの間にしか引かれない (選手の side を列挙値と比べている) | `src/player_painter.cpp:1034, 1061` | 比べ方を直す |
| B10 | 左のキーパーのキャッチ範囲に、右のキーパーの色を使う (B9 と同じ比較の誤り) | `src/player_painter.cpp:545, 563` | 同上 |
| B11 | Change Playmode の back_pass で、位置を計算した後に何も送られない | `src/main_window.cpp:2359-2394` | `back_pass_l` / `back_pass_r` を送る (web 版 `src/io/live/commands.ts` `changePlayModeCommands`) |
| B12 | 設定ダイアログの "Fix" が Auto-right になる (`setPlayerSelectType( SELECT_AUTO_RIGHT )`) | `src/config_dialog.cpp:2441` | `SELECT_FIX` にする |
| B13 | 色の一覧の Illegal Defense の項目が、初期値に Pointto の色 (`pointtoPen().color()`) を使う | `src/config_dialog.cpp:1258-1260` | Illegal Defense の色を使う |
| B14 | 選手タイプ一覧で、数値が 2 つある列が並べ替わらない (セルの文字列を 1 つの数値として読む) | `src/player_type_dialog.cpp:69-75` | 最初の数値で比べる |

- 1 件ずつ、できれば単体テストを付けて直す (Qt Test。解析の不具合はフィクスチャで、描画の不具合は計算を切り出して確かめる)。
- Phase 1 (2026-11-11、`fix/parity-bugs`) で B1、B5、B9〜B14 を直した。B10 のほかはテストがある (§14)。
- Phase 2 (2026-11-12、`feature/json-parser`) で B2〜B4、B6〜B8 を直した。どれもテストがある (§14)。
- master への取り込みを急ぐなら、不具合の修正だけを先に別の PR にできる。

## 7. JSON パーサの改善

対象は `rcss/rcg/parser_simdjson.cpp` (ログ) と、ライブの JSON メッセージ (同じパーサを `disp_holder.cpp` から使う)。web 版の `src/io/rcg-json.ts` と `docs/protocol.md` §3、§7 に合わせる。

1. 未知のキーは飛ばす (B3)。今後サーバが項目を足しても読めるように。 → Phase 2 の B-1 で済 (§14)。
2. `vq` を `'h'` / `'l'` で入れる (B2)。 → B-2 で済。
3. 選手を `side` と `unum` で格納する (B4)。配列の要素が欠けても、順が違ってもよいように。 → B-3 で済。
4. ログを 1 行 1 レコードとして読む。サーバが異常終了すると末尾の `]` が欠けるので、`[` と `]` の行を飛ばし、行末の `,` を取り除いて読む。1 行が壊れていたら、その行だけ飛ばして警告を出す。 → Phase 2 の A4 で済 (§14)。
5. 省略されうる項目を受け付ける: `stime` (0 より大きいときだけ)、`px` / `py` (指差し中だけ)、`fside` / `fnum` (attentionto のあるときだけ)、`pen_score` / `pen_miss` (PK を蹴ったチームだけ)、チーム名の `null` (接続前)、`capacity`。 → B-4 で済。
6. ボールの高さ `ball.z` と鉛直方向の速度 `ball.vz` を、あれば読む (2.5 次元シミュレータ用。単位は m と m/サイクル、上が正。キー名は仮で、サーバの形式が決まったら合わせる)。`types.h` の `BallT` に足す。 → B-5 で済。
7. `timestamp` (`%FT%T%z`、オフセットにコロンなし) を壊さずに扱う。今のパーサは `version` と `timestamp` を読み飛ばし、Handler に渡していない (`parseVersion`、`parseTimestamp` が空。2026-11-11 に確かめた)。両方を Handler (`handleServerVersion`、`handleTimestamp`) に渡す。 → B-6 で済。
8. 速さを測る: 全試合の JSON ログ (作業ツリーの直下に置いた `1-json.rcg.gz`、6,209 フレーム。リポジトリには入れない) の読み込み時間を、改善の前後で比べて §14 に記録する。web 版はブラウザで約 0.3〜0.5 秒。 → Phase 2 で測った (§14)。
9. `origin/feature/json-reformat` の内容 (旧パーサの新形式対応) を確かめ、上と重ならないものがあれば取り込む。 → 取り込むものはなかった (§14)。
10. テキスト rcg の B6、B8 もこの段階で直す。 → B-7、B-8 で済。B7 (パラメータの既定値) は B-9。
11. 同梱の simdjson (3.8.0。GCC 15 でビルドできるよう、3.10.0 の修正を 3 行だけ入れた。§14) を、新しい版にするかを決める (2026-11 の最新は 5.1.1)。 → 5.1.1 に更新した (Phase 2 の A2、§14)。

テスト: フィクスチャ (`1-json.rcg.gz` から短く切り出したもの、壊れた行、未知のキー、選手の順の入れ替え、末尾の `]` の欠け) で、フレーム数、特定のフレームの値、プレイモードの並びを確かめる。web 版の `test/unit/rcg-json.test.ts`、`rcg-text.test.ts`、`full-logs.test.ts` が手本。

## 8. Qt6 対応

1. CMake: `find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)` で版を選び、`Qt${QT_VERSION_MAJOR}::Widgets` などでリンクする。Test も同じ形にする (WebSockets は、取り込むときに §3.5 で)。Qt6 では `OpenGL` と `OpenGLWidgets` を足す (3D 用)。Qt5 の `QOpenGLWidget` は Widgets にある。 → Phase 3 で済 (OpenGL は Phase 4 で足す。§14)。
2. §3.3 の置き換えを行う。Qt 5.15 でも動く書き方を選ぶ (両対応、D2)。 → Phase 3 で済。
3. `USE_GLWIDGET` の `QGLWidget` は、使われていなければ消す。使うなら `QOpenGLWidget` にする。 → 消した。
4. `QT_VERSION_CHECK` の分岐のうち、Qt 5.15 より古いものを消す。 → 済。
5. 非推奨の警告をなくす: Qt5 では `QT_DISABLE_DEPRECATED_BEFORE=0x050F00` でビルドし、Qt6 では非推奨の警告が出ないことを確かめる。 → CMake で常に `QT_DISABLE_DEPRECATED_BEFORE` (Qt5 は 0x050F00、Qt6 は 0x060000) を付け、CI は `-Werror=deprecated-declarations` でビルドする。
6. 高 DPI: Qt6 では既定で有効。2D の描画 (線の太さ、文字の大きさ) が、倍率 1、1.5、2 で崩れないことを確かめる。3D は `devicePixelRatio` に合わせて描く。 → 画面外で倍率 1、1.5、2 を描いて確かめた。Qt5 も Qt6 と同じく画面の倍率で拡大するようにした (§14)。
7. autotools は D3 のとおり Qt5 のまま残し (qmake は D11 で廃止)、README に「Qt6 と 3D は CMake で」と書く。 → README に「autotools は Qt5 だけ」と書いた (3D は Phase 4 で書く)。
8. CI で Qt5 と Qt6 の両方をビルドする (§10)。 → 済。下限の Qt 6.2 (Ubuntu 22.04 のコンテナ) も。
9. README のインストールの手順を、Ubuntu 22.04 / 24.04 / 26.04 の Qt5 と Qt6 の両方について書き直す。 → 済。
10. `cmake_minimum_required(VERSION 3.5)` を上げる。CI の CMake が「3.10 より古い版との互換は将来なくなる」と警告している (§14)。Qt6 は CMake 3.16 以上を要るので、3.16 にする。 → 済。

## 9. フェーズ計画

各フェーズは feature ブランチで進め、終わったら develop に取り込む (D9)。ただし 3D 表示 (Phase 4〜8) は、完成して公式に採用されることが決まってから develop に取り込む。区切りごとにユーザに確かめる。

### 9.1 ブランチ (2026-11-11 にユーザが了承)

| フェーズ | ブランチ |
|---|---|
| Phase 0: CI とテストの仕組み | `feature/ci` |
| Phase 0: 3D の試作 | `proto/3d-gl` (取り込まない。作り方は試作に入るときに相談する) |
| Phase 1: 不具合の修正 | `fix/parity-bugs` |
| Phase 2: パーサの改善 | `feature/json-parser` |
| Phase 3: Qt6 対応 | `feature/qt6` |
| Phase 4〜8: 3D 表示 | `feature/3d-view` |

- Phase 0〜3 は、終わったものから develop に取り込む (取り込みはユーザが行う)。次のフェーズのブランチは、前のフェーズを取り込んだ develop から作る。取り込む前に始めるときは、前のフェーズのブランチから作る (積み重ね)。Phase 1 と Phase 3 は、どちらも `main_window.cpp`、`config_dialog.cpp`、`player_painter.cpp` を変えるので、並行させずに順に積む。
- `feature/3d-view` は、Qt6 対応 (Phase 3) を取り込んだ develop から作る。完成して公式に採用されることが決まるまで、develop に取り込まない。そのあいだに develop に入ったもの (TCP / WebSocket 対応など) は、develop を `feature/3d-view` にマージして取り込む。
- ブランチはユーザが GitHub 上で作る (2026-11-11 から)。手元では `git fetch origin` の後に `git switch <ブランチ>` とし、GitHub のブランチを追跡するブランチを作って切り替える。`feature/ci` だけは手元で先に作ったので、`git push -u origin feature/ci` で GitHub に作る。
- `proto/3d-gl` (試作) の作り方は、試作に入るときに相談する (2026-11-11)。
- PR の取り込みは Merge Commit を基本にする (D9)。PR 単位の履歴は `git log --first-parent`、PR 全体の取り消しは `git revert -m 1 <マージコミット>`、原因探しは `git bisect start --first-parent` で PR を絞ってから中のコミットに進む。途中のコミットもビルドできるように保ち、試して消しただけのコミットはプッシュの前にまとめる。
- Squash Merge が向くのは、次をすべて満たす PR だけで、そのときは PR の案を出すときに伝える: (1) 目的が 1 つの小さな変更、(2) 途中のコミット (直し、試行錯誤) にそれぞれの意味がない、(3) ほかのブランチがその上に積まれていない、(4) 計画書などからそのコミット ID を参照していない。develop から master への取り込み、`feature/3d-view` の取り込み、独立した変更をいくつも含む PR は、Merge Commit にする。

### Phase 0: 土台と検証

**作業**
- §2.2 の決めることを、ユーザに確かめる。
- D7 に従って、develop からブランチを作る (§9.1)。
- CI (GitHub Actions) を用意する。Ubuntu 26.04 (`runs-on: ubuntu-26.04`。web 版の CI と同じ) で、Qt5 のビルドと単体テスト。Qt6 のジョブは、Qt6 でビルドが通るようになる Phase 3 で足す。アクションは Node.js 24 で動く版 (web 版と同じく checkout@v5、upload-artifact@v7 など) を使う。
- テストの仕組み (Qt Test、CTest) を develop の上に作る。形は §3.5 のとおり `feature/support-tcp-ws` にそろえる。
- D10 の案 A (§2.4) を試す (試すあいだは `feature/ci` への push で動かす)。→ 試して動いたが、D10 は案 B (廃止) に決めた。
- 3D の試作 (D1 の確認): `QOpenGLWidget` で、骨つきの人型 22 体 (クリップを混ぜて歩かせる) と、4096 px の影の地図と、インスタンス描画の円柱を描き、1920×1080 で 60 fps を保てるかを測る。Qt 5.15 と Qt 6 の両方で動かす。結果を §14 に記録し、D1 を決める。 → 済。Qt Quick 3D も試作して比べた (§14)。
- 人型のモデルを D6 のとおりに用意する。 → 済 (`proto/3d-gl` の `resources/models/`。§14)。

**完了条件**: §2.2 がすべて決まる。CI が通る。試作で 60 fps を確かめる (D1 が推奨案のとき)。D10 の案 A を採用するかが決まる (決まった: 採用せず、案 B)。 → 満たした (2026-11-12)。

### Phase 1: 不具合の修正

**作業**: §6 の B1、B5、B9〜B14 (パーサ以外)。単体テストを付ける。

**完了条件**: 直した不具合ごとにテストがあり、CI が通る。ユーザが実際のログで確かめる。

### Phase 2: JSON パーサとテキストパーサの改善

**作業**: §7 (B2〜B4、B6〜B8 を含む)。

**完了条件**: フィクスチャのテストが通る。全試合のログ 2 本 (JSON とテキスト v5) を読めて、件数がファイルと一致する。読み込み時間を記録する。

### Phase 3: Qt6 対応

**作業**: §8。CI に Qt6 のジョブを足す。

**完了条件**: Qt 5.15 と Qt 6 の両方でビルドでき、非推奨の警告がない。CI の両方の版が通る。ユーザが 2D の表示、ライブ接続 (UDP。TCP / WebSocket は取り込んだ後に §3.5 で)、ログの再生、審判操作、設定ダイアログを、Qt6 で確かめる。

### Phase 4: 3D の土台

**作業**
- `FieldView3D` と、2D との切り替え (D キー、メニュー、ツールバー)。`ENABLE_3D` と、GL が使えないときの扱い。
- 時間と補間 (§5.2)。ライブの 1 サイクル遅れ。
- 地面の絵、ゴール、選手 (円柱。状態の色、カード、向きの印、足元の円と向きの線、視野)、ボール (高さと影)。 → カードは背番号のラベルと一緒に 4-5 で、足元の向きの線と視野は線の描画と一緒に Phase 5 で描く (4-3、§14)。
- カメラ: free、broadcast、top、ゴール裏 2 つ。E キー。マウスの操作。 → 4-4 で済 (§14)。
- HUD (スコアボード、ロゴ)。 → 4-5 で済 (§14)。
- 背番号のラベル (大きさの上限、真上のカメラでの置き方)。 → 4-5 で済 (カードも)。

**完了条件**: 2D と 3D を切り替えても、フレームと選択が保たれる。`ball.z` のある show (テスト用に作る) で、ボールが空中に描かれ、影が地面に出る。カメラごとの位置の単体テストが通る。

### Phase 5: 2D と同じ情報と、審判操作

**作業**: 重ね描き (§5.4。2D の painter から計算を切り出して共有する)、範囲の数値の HUD、スタミナのバーと S キー、不正な守備の色、審判操作の当たり判定 (§5.4)。 → 区切り: 5-1 線の描画の部品、足元の向きの線、視野 (Phase 4 から移したもの)。5-2 2D と同じ重ね描きと範囲の数値の HUD。5-3 スタミナのバーと S キー。5-4 審判操作の当たり判定。5-5 フォーカス点の表示 (2D と 3D。ユーザが足した)。不正な守備の色は 4-5 の背番号で済。

**完了条件**: Show タブの切り替えが 3D でも効く。3D で右クリックした選手と地面の点に、審判のコマンドが送られる (擬似サーバで確かめる。web 版の `tools/replay-server` が参考)。

### Phase 6: 選手視点と、ほかのカメラ

**作業**: 選手視点の 4 つのスタイルと知覚 (§4.3、`player-camera.ts`、`sight.ts`)、寄りのカメラ 2 つ (ホイールで距離)、ピッチ全体の視点 9 個 (I、Ctrl+I、メニュー、ツールバー)、方位の帯とミニマップ、両端のチームの色 (ネット、広告板、看板)、設定ダイアログの 3D タブと Player Camera タブ。 → 区切り: 6-1 寄りのカメラ 2 つ (距離は 2D と同じく Z と X。§5.6)。6-2 ピッチ全体の視点 9 個。6-3 選手視点と知覚。6-4 両端のチームの色。6-5 方位の帯とミニマップ。6-6 設定ダイアログの 3D タブと Player Camera タブ (設定とコマンドラインのオプションは各区切りで足す)。

**完了条件**: 選手視点の 4 つの項目のすべての組み合わせを選べる。`smooth` が角速度の上限を超えない (単体テスト)。`view` と `server` の描き分け (単体テスト)。どの視点でもピッチの四隅が画面に収まる (単体テスト)。

### Phase 7: 人型

**作業**: §5.8。

**完了条件**: 人型で、歩く、走る、キック、タックル (ファウルでは一瞬)、ファウルを受けて座り込む、キャッチ、指差しを、実際のログで確かめる。コマ送りで進んで戻ったときと、跳んで戻ったときに、同じ姿勢になる (単体テストと、描画の点検用の状態で確かめる)。同じフレームを何度描いても、首と脚が変わらない。

### Phase 8: 見た目とボールの助け

**作業**: 影 (§4.3)、芝の縞、空と霧、観客席、ボールの模様と転がり、ボールの円と軌跡。設定とダイアログ。

**完了条件**: どれも設定で切り替えられる。真上のカメラに切り替えた最初のフレームが、次のフレームと同じ (§11 の 4)。1920×1080、デスクトップの GPU で、すべてオンでも 60 fps を保つ (計測を §14 に記録)。

### Phase 9: 文書とリリース

**作業**: README (3D、キー、設定、Qt6 のビルド)、ChangeLog と NEWS (qmake の廃止 (D11) も書く)、版を D8 にする。autotools の `make dist` で作る tarball に、CMake でだけビルドするファイル (`src/view3d/`、`test/`、`data/` など。2026-11-12 の時点で入っていない) を入れるか、tarball の作り方を変える (19.0.1 の tarball は `make dist` で作られている)。master の `main.yml` と `utils/appimage/` を消す (D10。master への取り込みで行う)。NEWS と README に、AppImage をやめたこと、ソースからのビルドの手順、ブラウザ版への案内を書く。ユーザが master に取り込み、タグを付ける。

**完了条件**: リリースされる。

## 10. テスト計画

### 10.1 単体テスト (Qt Test)

GL に依存しない計算を、web 版の単体テストを手本に移す。

- テストは `test/CMakeLists.txt` の `rcssmonitor_add_test(<名前> <ライブラリ>...)` で足す (`test_<名前>.cpp` をビルドし、`<名前>` として登録する)。アプリのクラスを使うテストは `rcssmonitor_core` をリンクする。
- `gl_isolation` は、OpenGL を `src/view3d/gl/` の外で使っていないことを確かめる (§5.3)。
- CTest はどのテストも `HOME` をビルドディレクトリの下 (`test/home`) にし、`QT_QPA_PLATFORM=offscreen` で動かす。`Options` は作られたときに `~/.rcssmonitor.conf` を読み (なければ書き)、終了時に書くので、利用者の設定ファイルを守るため。

| web 版のテスト | 確かめること |
|---|---|
| `view3d-motion.test.ts` | 補間、カメラの位置 (中継風、ゴール裏、真上、寄り)、追従、カメラの順、設定 |
| `viewpoints.test.ts` | 視点の式 (四隅が収まり、どこかが端に接する) |
| `player-camera.test.ts` | 選手視点のスタイル、角速度の上限、知覚の規則 |
| `figures.test.ts` | 動作の範囲 (タックルのビットが 10 フレーム続くときと 1 フレームのとき)、ファウルを受けた姿勢、歩いた距離の表 (描く順によらず同じ、後ろ向きで減る、瞬間移動を数えない) |
| `overlays.test.ts` | 重ね描きの計算、向きの線の端と色、スタミナのバー |
| `scenery.test.ts` | 芝の縞の帯、観客席の面の向きと範囲、ボールの五角形と転がり |
| `ball-aids.test.ts` | 軌跡の点の年齢と長さ、置き直しで切れること、帯の向き・幅・濃さ |
| `guides.test.ts`、`scene3d.test.ts` | 方位の帯とミニマップの座標、ボールの高さと影の濃さ、色 |
| `rcg-json.test.ts`、`rcg-text.test.ts`、`full-logs.test.ts` | パーサ (§7) |

### 10.2 描画のテスト

- `FieldView3D` に、点検用の状態を返す関数を作る (web 版の `Render3DState`: 描いた選手と人型の数、カメラの位置と向き、ボールの位置と回転、影、景色、ラベルの位置と大きさ、重ね描きの数、描画命令の数など)。描画のテストは、これを見て確かめる。
- 画面外の描画 (`QOffscreenSurface` と `QOpenGLFramebufferObject`、または `QT_QPA_PLATFORM=offscreen`) と、CI ではソフトウェアの OpenGL (Mesa llvmpipe、`LIBGL_ALWAYS_SOFTWARE=1`) を使う。必要なら `xvfb-run`。
- 画素で確かめるのは、色に頼らない比べ方にする (例: 切り替えた最初のフレームと次のフレームが同じ)。web 版では、黒い画素だけを数えたテストが、白くなった同じ不具合を見逃した。
- **ソフトウェアの描画は遅い**。テストでは影を切って描く (影を確かめるテストだけ明示してオンにする)、なめらかに動くカメラの到着を待たない、人型を使うテストは時間の制限に余裕を持たせる。web 版の CI では、ランナーが遅いときにこの 3 点で時間切れが起きた。

### 10.3 手で確かめること

- 実際の GPU での fps (描画時間をログに出す仕組みを作る)。
- ログの読み込みの速さは `test/bench_load` で測る (CI では動かさない。§14)。conda などが `PATH` や `LD_LIBRARY_PATH` に入った環境では、別の Qt や zlib でビルドされ、動くので、それらを外してビルドし、測る。
- asio 版 rcssserver とのライブ接続 (UDP。TCP / WebSocket は取り込んだ後)。`RCSS_CONF_DIR` と `HOME` を一時ディレクトリにする (§0.1)。
- 全試合のログ (作業ツリーの直下に置いた `1-json.rcg.gz` と `202207160432-HELIOS2022_1-vs-CYRUS_0.rcg.gz`。リポジトリには入れない) での見た目。web 版と並べて見比べる。

## 11. web 版で得た教訓

1. **ログから決める**: 人型の姿勢、ボールの回転、軌跡は、フレームと alpha だけから毎回求める。描くたびに時間や回転を積み上げると、コマ送りや巻き戻しで崩れる (web 版の区切り 12)。
2. **ビットの続く範囲を見る**: タックルとファウルは、ビットが続くフレームの範囲で姿勢を決める (§4.4)。最初は「ビットのあったフレームから tackle_cycles」としたため、止められたタックルが 2 倍の長さ倒れ、ファウルでは滑ったまま走った。作ったデータ (ビットが 1 フレームだけ) のテストでは気づけなかったので、実際のログで確かめる。
3. **重ねる回転は毎回はじめから**: 骨の回転をクリップの姿勢の上に重ねるとき、前のフレームの結果の上に重ねない。
4. **状態の切り替えの最初のフレームを確かめる**: web 版では、three.js が GPU にデータを送る時期の都合で、真上のカメラに切り替えた最初のフレームだけ、円柱が初期値 (原点) のまま描かれた。カメラや設定を切り替えた直後のフレームを、次のフレームと比べるテストを入れる。
5. **画面上の大きさに上限と下限**: 背番号とバーは、近くで大きくなりすぎないよう上限 (Player フォントの 2 倍)。ボールの円の線と軌跡の頭は、遠くで消えないよう下限 (2 px、6 px)。
6. **追従の遅れ**: 近いカメラほど遅れを小さくする (寄りの時定数は距離に比例)。1 フレームに進める時間に上限を設ける。
7. **観客席は片面だけ**: カメラが席の中や後ろに入っても、ピッチを隠さない。
8. **太陽の向き**: 手前のカメラから影が見えるよう、左奥から照らす。真上のカメラでは影を描かない。ボールは高さを読むための真下の影のまま。
9. **補間しない場面**: プレイモードの切り替えと瞬間移動の前後。ライブは 1 サイクル遅れて描く。
10. **ソフトウェアの描画は遅い**: CI のテストは軽く保つ (§10.2)。
11. **キーの決め方**: rcssmonitor の空いているキーを使い、Shift は使わず、関係のある操作は Ctrl を付ける。ブラウザと違い、ネイティブ版はキーの横取りの問題がない。
12. **設定の見せ方**: 項目ごとのツールチップ、範囲の明示、範囲の外の扱い、使わない欄を薄くする。タブが長くなったら分ける。
13. **ユーザが決めた既定値**: 選手視点 (肩越し、頭、滑らか、120° 一定、サーバの規則、τ 0.5、ω 90)、人型の既定は円柱 (simple)、影と景色とボールの助けは既定でオン。

## 12. 変更・追加するファイル (見込み)

**追加** (`src/view3d/` にまとめる案)

| ファイル | 内容 |
|---|---|
| `field_view_3d.{h,cpp}` | QOpenGLWidget。入力、時間、描画の呼び出し、点検用の状態 |
| `scene_3d.{h,cpp}` | シーンの組み立てと描画 |
| `gl/*.{h,cpp}`、`shaders/*.vert`、`*.frag`、`meshes_3d.{h,cpp}` | 描画の基盤 (シェーダ、バッファ、テクスチャ、描き方、形。§5.3。4-3b で作った。影の地図は Phase 8) |
| `cameras_3d.{h,cpp}`、`player_camera_3d.{h,cpp}`、`sight.{h,cpp}`、`interpolate.{h,cpp}`、`scene_math.{h,cpp}` | 計算 (GL に依存しない) |
| `overlay_math.{h,cpp}` | 2D と 3D で共有する重ね描きの計算 (2D の painter から移す)。Phase 1 で `src/` に作り、オフサイドラインを移した |
| `play_mode_position.{h,cpp}` | Change Playmode のボールの位置 (Phase 1。3D の審判操作でも使う) |
| `player_type_sort_proxy.h` | 選手タイプ一覧の並べ替え (Phase 1) |
| `referee_menu.{h,cpp}`、`referee_target.h` | 右クリックのメニュー (審判操作) と、狙った先 (点と選手)。2D と 3D で共有する (5-4) |
| `player_camera_options.h` | 選手視点のスタイルの種類と名前と範囲 (GL に依存しない。設定が使う。6-3) |
| `view3d/guides_3d.{h,cpp}` | HUD の方位の帯とミニマップ (QPainter で描く。GL に依存しない。6-5) |
| `view3d/stadium_3d.{h,cpp}`、`shaders/panel.*` | ネット以外の両端のチームの色: 広告板と看板の画像と配置 (GL に依存しない)、板のシェーダ (6-4) |
| `view3d/picking.{h,cpp}` | 3D の右クリックの当たり判定 (カメラの光線と、選手の胴と頭、地面。GL に依存しない。5-4) |
| `game_log_file.{h,cpp}` | ログのファイルを 1 つのバッファに読み、解析する。libdeflate があれば、gzip をそれで展開する (Phase 2) |
| `rcss/rcg/line_parser.{h,cpp}` | ログの行を並列に解析し、順に Handler に渡す (Phase 2) |
| `test/bench_load.cpp` | 読み込みの速さを測る (Phase 2。CI では動かさない) |
| `stadium_3d`、`scenery_3d`、`ball_3d`、`ball_aids_3d` | 部品 |
| `figures_3d`、`pose`、`gltf_model` | 人型 |
| `hud_3d`、`guides` | HUD、方位の帯、ミニマップ |
| `config_dialog` の 3D タブ、Player Camera タブ | 設定 (6-6。Phase 7、8 の項目はその区切りで足す) |
| `third_party/cgltf/cgltf.h` | glTF の読み込み (MIT)。v1.15 を試作のブランチに入れた (§14) |
| `resources/models/Mannequin.glb`、`.qrc` | 人型のモデル (CC0) と出典。圧縮を外したものを試作のブランチに入れた (§14) |
| `test/test_*.cpp` | 単体テストと描画のテスト |
| `.github/workflows/ci.yml` | CI |
| `data/rcssmonitor.desktop`、`icons/rcssmonitor.png` | デスクトップのファイルとアイコン。CMake の `install()` で入れる (Phase 0 で AppImage の試行のために足した。普通のインストールでもアプリのメニューに出るので、AppImage をやめた後も残す) |

**変更**: `main_window.cpp` (切り替え、メニュー、ツールバー、キー)、`field_canvas.cpp` (右クリックのメニューを共有)、`log_player.cpp` (alpha)、`options.{h,cpp}` (設定)、`config_dialog.cpp`、`player_painter.cpp` と `ball_painter.cpp` と `field_painter.cpp` (計算の切り出し、芝の縞の引数)、`rcss/rcg/*` (§6、§7)、`CMakeLists.txt` と `src/CMakeLists.txt` (Qt6、3D)、README。

**削除**: `rcssmonitor.pro`、`src/src.pro`、`src/rcsslogplayer/` (D11。Phase 0 で消した)。AppImage の試行の `.github/workflows/appimage.yml` (D10。Phase 0 で消した)。master の `.github/workflows/main.yml` と `utils/appimage/` (D10。20.0.0 を master に取り込むときに消す)。

## 13. 対象外

- web 版に固有のこと: ブラウザでの配布 (単一 HTML、GitHub Pages)、file:// と https の制約、ブラウザのキーの横取り、URL のパラメータ (rcssmonitor ではコマンドラインと設定ファイル)。
- バイナリの rcg (v1〜v3) の 3D での扱いの改善 (今の 2D と同じく読めればよい)。
- 2.5 次元シミュレータのサーバ側の形式に合わせること (形式が決まってから)。
- rcssmonitor-web の将来の拡張 (ドラッグで選手を動かす、msg の一覧など)。

## 14. 記録

区切りごとに、日付、決めたこと、作ったもの、見つけた不具合とその原因、計測、試験を書き足す。

### Phase 0

#### 2026-11-11: 方針の確認とブランチ

**決めたこと (ユーザ)**
- D7: 作業は develop から派生させる。`feature/support-tcp-ws` は、TCP / WebSocket 対応の採用が決まってから取り込む (§3.5)。
- ブランチは §9.1 のとおり。最初に `feature/ci` を develop (01f880a) から作った。
- D10: master の AppImage のワークフローは保守の負担になっている。無効にするか、保守のやさしい方式に置き換える。置き換え先の案は §2.4 (推奨は案 A。Phase 0 で試してから決める)。
- 計画書をリポジトリに入れる。英語版 `PLAN_3D-VIEW.en.md` を並行して保つ (§0.1)。
- D2〜D6、D8、D9 は推奨案で進める。D1 は試作の結果で決める。

**確かめたこと**
- ローカルの develop は origin/develop より 2 コミット遅れていたが、ユーザが更新した (01f880a)。`feature/support-tcp-ws` も 01f880a から分かれている。
- master は develop より進んでいる (PR #59 の AppImage のワークフロー。§3.2、§3.4)。計画書の初版の「master と develop は同じ内容」「CI はない」を直した。
- `test/` は `feature/support-tcp-ws` で足したもので、develop にはない (§3.1)。
- §6 の行番号を develop で確かめ直した。違うのは B11 だけ。§3.3 の箇所数も develop で同じ。
- 作業環境: Ubuntu 24.04、CMake 3.28、Qt 5.15.13 と Qt 6.4.2 (Core、Gui、Widgets、Network、OpenGL、OpenGLWidgets、Test がある。Qt6 の WebSockets はない)。 → 訂正 (Phase 3): Qt6 は実行用のライブラリだけで、開発用のパッケージはなかった。Phase 3 でユーザが `qt6-base-dev` と `libqt6opengl6-dev` を入れた。GPU は AMD (PCI ID 13c0)。
- rcssmonitor-web の作業ツリーは `v20.0.0-beta.2` (ed36941) の 1 コミット後 (bcab423。計画書の更新だけ)。
- asio 版 rcssserver の作業ツリーは、リポジトリ直下 (`rcssserver-19.x-asio/`) から `../rcssserver` (`feature/asio-migration`、19.0.0) に移っていた。冒頭の表を直した。
- 既存のリリースのタグは `rcssmonitor-X.Y.Z` の形。19.0.1 のリリースには AppImage が添付されている。

#### 2026-11-11: テストの仕組みと CI (`feature/ci`)

**作ったもの**
- テストの仕組み: 最上位の `CMakeLists.txt` で Qt5 Test を探し、`test/` を足す (§3.5 のとおり `feature/support-tcp-ws` と同じ書き方)。最初のテスト `test/test_rcg_parser.cpp` は、テキスト rcg (v5) と JSON のパーサが、ヘッダ、パラメータ、チーム、プレイモード、show をどう Handler に渡すかを確かめる。フィクスチャ `test/data/head_v5.rcg` (20 KB) と `head_json.rcg` (41 KB) は、全試合ログ 2 本の先頭から、ヘッダとパラメータ、最初のプレイモード、チーム、show 3 つを切り出したもの (チームグラフィックは除いた)。
- `.github/workflows/ci.yml`: ubuntu-26.04 で、CMake (Qt5) のビルドと ctest、autotools (Qt5) のビルド。master、develop、`feature/**`、`fix/**`、`hotfix/**` への push と PR で動く。
- `.github/workflows/appimage.yml`: D10 の案 A を試す版 (§2.4)。`feature/ci` への push で動く。`ubuntu:22.04` のコンテナで CMake でビルドし、linuxdeploy `1-alpha-20251107-1` と linuxdeploy-plugin-qt `1-alpha-20250213-1` で AppImage にまとめ、`ubuntu:22.04`、`24.04`、`26.04` のコンテナで `xvfb-run` の下で `--version` を実行して起動を確かめる。
- `data/rcssmonitor.desktop` と `icons/rcssmonitor.png` (48×48。`icons/rcss.xpm` から作った)。CMake の `install()` で `share/applications` と `share/icons/hicolor/48x48/apps` に入れる。

**試験 (手元、Ubuntu 24.04、Qt 5.15.13)**
- HEAD のきれいなコピーに今回の変更を載せて、`ci.yml` の 2 つのジョブと同じ手順を実行した。CMake のビルドと ctest (テスト関数 `textV5` と `json` がどちらも通る)、autotools のビルドが通った。
- テストが失敗を拾うことも確かめた (最初にパーサの作りと違う期待値を書いて、失敗した。下の 3 つ目)。
- `DESTDIR` へのインストールで、デスクトップのファイル (`desktop-file-validate` で問題なし) とアイコンが入り、入れた `rcssmonitor --version` が 0 で終わる。
- linuxdeploy の動き (固定した版のソースで確かめた): `--desktop-file` を渡さなければ AppDir の最初のデスクトップファイルを使い、アイコンは AppDir の hicolor から探し、AppDir にすでにある実行ファイルの依存も入れる。
- `appimage.yml` は手元では動かしていない (linuxdeploy を手元で動かしていない)。push した後の CI で確かめる。

**見つけたこと**
- qmake のビルドが develop で通らない (§3.2、D11)。
- JSON パーサは `version` と `timestamp` を読み飛ばしている (§7 の 7)。
- テキスト rcg の v4、v5、v6 はどれも `ParserV4` が読み、`Parser::version()` は 4 を返す。ログの版は Handler の `logVersion()` に渡る (仕様。テストはこれに合わせた)。
- Ubuntu 26.04 (resolute) には、Qt 5.15.18 (`qtbase5-dev`、`qt5-qmake`、`libqt5websockets5-dev`) と Qt 6.10.2 (`qt6-base-dev`、`qt6-websockets-dev`) がある (packages.ubuntu.com で確かめた)。
- web 版の CI は ubuntu-26.04 のランナーで通っているので、同じ指定を使った。
- GNUInstallDirs: 一度別の prefix で構成したビルドディレクトリで `CMAKE_INSTALL_PREFIX=/usr` に変えると、`-DCMAKE_INSTALL_LIBDIR=lib` を渡しても `lib/x86_64-linux-gnu` に計算し直される。CI はまっさらなディレクトリで構成するので問題ない。

#### 2026-11-11: qmake の廃止とブランチの運用

**決めたこと (ユーザ)**
- D11: qmake のビルドを廃止する。qmake を使う機械がなくなるため。
- ブランチは、今後はユーザが GitHub 上で作ってから作業する (§9.1)。
- Phase 0〜3 (Qt6 対応まで) は、終わったものから develop に取り込む。3D 表示は、完成して公式に採用されることが決まってから develop に取り込む。
- 3D の試作は、`feature/ci` を GitHub に上げた後に、改めて相談する。

**作ったもの**
- `rcssmonitor.pro`、`src/src.pro`、`src/rcsslogplayer/` (qmake のビルドだけが使っていた古い rcsslogplayer のパーサの写し。CMake と autotools は `rcss/rcg/` を使う) を消した。

**試験**
- 消した後の作業ツリーの写しで、CMake のビルドと ctest、autotools のビルドが通った。
- 手元のリモートは SSH (`git@github.com:`) なので、ワークフローのファイルを含むプッシュに、トークンの `workflow` 権限は要らない。

#### 2026-11-11: CI の 1 回目 (5fa9729) の失敗と修正

**CI の結果**
- CI: CMake と autotools の両方が、同梱の simdjson 3.8.0 のコンパイルで止まった。ランナーの GCC 15.2.0 が、`base_formatter` の `this->print_newline()` など 3 か所を `-Wtemplate-body` でエラーにする (このクラスにその関数がない)。今回の変更ではなく、develop がもともと GCC 15 でビルドできない (新しいディストリビューションでビルドできない) という不具合。
- AppImage: ビルドとまとめる段階は通った (26 MB)。3 つの版すべての起動確認で、`libharfbuzz.so.0` がないために失敗した。linuxdeploy は harfbuzz などを、利用者の環境のものを使うようにまとめない。最小のコンテナに、それが入っていなかった (AppImage の不具合ではない)。

**直したこと**
- `rcss/rcg/simdjson/simdjson.h` の 3 か所を、上流が 3.10.0 で直したのと同じ形 (`static_cast<formatter*>(this)->print_newline()` など) にした。上流から持ってきた修正であることを注釈に書いた。同じ形の誤りは、ほかにはない。新しい版にするかは §7 の 11。
- `appimage.yml` の起動確認で入れるパッケージを、AppImage が環境に求めるものにそろえた。CI の AppImage をダウンロードして展開し、ELF の NEEDED から、まとめられていないライブラリを洗い出した: libc などの基本のもののほか、libGL、libEGL、fontconfig、freetype、harfbuzz、libX11、libX11-xcb、libxcb、libICE、libSM、libcom_err、libgpg-error、zlib。xcb の拡張、xkbcommon、glib、dbus は AppImage に入っている。パッケージ名は、どれも Ubuntu 26.04 にある (packages.ubuntu.com で確かめた)。

**試験**
- 手元 (GCC 13) で、ビルドと ctest が通った。GCC 15 は手元にないので、次の CI で確かめる。1 回目の CI は `rcss/rcg` で止まり、`src/` はまだ GCC 15 でコンパイルされていない。次の CI で、`src/` に GCC 15 のエラーが出る可能性がある。
- 展開した AppImage には、Qt のプラットフォームのプラグインとして xcb だけが入っている (`offscreen` では起動できない。起動確認は `xvfb-run` の下で xcb を使う)。

#### 2026-11-11: CI の 2 回目 (1fe26c6) が通った

**CI の結果**
- CI: CMake (GCC 15.2.0) のビルドと ctest (`rcg_parser` が通る)、autotools のビルドが通った。`src/` も GCC 15 でエラーなくコンパイルできた。警告は、同梱の `simdjson.cpp` の `-Wtemplate-id-cdtor` (C++20 で許されない書き方) 9 件だけ。
- CMake が `cmake_minimum_required(VERSION 3.5)` に「3.10 より古い版との互換は将来なくなる」と警告する。Phase 3 で 3.16 に上げる (§8 の 10)。
- AppImage: 27 MB。`ubuntu:22.04`、`24.04`、`26.04` のコンテナで、`xvfb-run` の下で `--version` を実行し、どれも `rcssmonitor 19.0.1` と表示して終わった (D10 の案 A の試行として成功)。

**決めたこと (ユーザ)**
- git の操作の分担を変えた: コミットとプッシュは Claude が行う。その前に、コミットメッセージの案をユーザが確認する (§0.1)。ブランチは引き続きユーザが GitHub 上で作り、PR と取り込みはユーザが行う。

#### 2026-11-11: AppImage の廃止 (D10)

**決めたこと (ユーザ)**
- D10: AppImage を廃止する (§2.4 の案 B)。ブラウザ版のモニタがあるので、AppImage の必要性が低い。master の古いワークフロー (`ci`) は、ユーザが GitHub の画面で無効にした (`gh workflow list` で `disabled_manually` を確かめた)。

**確かめたこと**
- 19.0.1 のリリースのダウンロード数: AppImage 257、ソース (`rcssmonitor-19.0.1.tar.gz`) 1,154。19.0.0 と 18.0.0 はソースだけ (142、852)。
- rcssserver のリリース (19.0.0、18.1.x) はソースだけで、AppImage はない。
- rcssmonitor-web のライブ接続は WebSocket だけ (README)。asio 版の rcssserver (20) が前提。

**作ったもの**
- AppImage の試行の `.github/workflows/appimage.yml` を消した (動いた形はコミット 1fe26c6 に残る)。
- デスクトップのファイルとアイコン (`data/rcssmonitor.desktop`、`icons/rcssmonitor.png`) と、その `install()` は残した。

#### 2026-11-11: PR の取り込み方

**決めたこと (ユーザ)**
- PR を作るときは、Claude がタイトルと本文の案を出す (§0.1)。
- PR の取り込みは Merge Commit を基本にする (D9)。Squash Merge が向く PR は、Claude が PR の案を出すときに伝える (条件は §9.1)。

**確かめたこと**
- これまでの取り込み: develop へは Merge Commit (#60、#57、#53 など) と Squash Merge (#51、#54、#46) が混ざっている。develop から master へは Merge Commit (#61、#58、#55 など)。リポジトリの設定では、Merge Commit、Squash、Rebase のどれも使える。
- `feature/ci` の PR は Merge Commit を勧めた。独立した変更 (GCC 15 でのビルドの修正、qmake の廃止など) を含み、計画書がコミット 1fe26c6 を参照しているため。

### Phase 1

#### 2026-11-11: 不具合の修正 (`fix/parity-bugs`)

PR #63 (`feature/ci`) を Merge Commit で develop に取り込んだ後、ユーザが GitHub 上で `fix/parity-bugs` を作った。

**決めたこと**
- B1: ライブ接続のフレームは、65,535 (`MAX_LIVE_FRAMES`。前と同じ数) を超えたら、古いものから 4,096 フレーム (`LIVE_TRIM_FRAMES`。約 7 分) をまとめて捨てる。ログファイルは、ファイルが大きさの上限なので全部持つ。設定で変えられるようにはしない (ユーザ)。初めは上限をなくす案にしたが、ユーザの指摘で改めた: rcssserver は 32 ビットの時刻の範囲で試合を続けられるので、全部をメモリに持つのは現実的でない。1 フレームは約 2.3 KB (`sizeof(DispInfoT)` = 2312) で、65,535 フレームで約 160 MB。捨てるのはポインタの `vector` の先頭なので、まとめて捨てれば性能の問題はない。気をつけるのは添字で、今の位置 (`M_current_index`。捨てたフレームを見ていたら残っている最も古いフレームにする) と得点の位置 (`M_score_changed_index`) を、捨てた数だけずらす。時刻ごとに持つ描画命令 (点、円、線) も、残っている最も古いフレームより前のものを捨てる。ライブのフレームは `DispHolder` のライブ用の入口 (`addJSON`、`addDispInfoV1`〜`V3`) だけを通るので、そこで捨てる (`trimLiveFrames()`)。
- テストのため、アプリのソース (`main.cpp` 以外) を CMake の静的ライブラリ `rcssmonitor_core` にまとめ、アプリとテストがそれをリンクする。
- 不具合ごとに 1 コミットにする。

**作ったもの (順に 1 コミットずつ)**
1. `rcssmonitor_core` と、テストを足す関数 `rcssmonitor_add_test` (`HOME` と `QT_QPA_PLATFORM` を設定する。§10.1)。
2. B5: `util.cpp` の `foul_push_r`。テスト `rcg_util`: `PLAYMODE_STRINGS` のすべての名前が元のプレイモードに戻る。
3. B1: ライブ接続で古いフレームから捨てる。テスト `disp_holder`: ライブの入口から 65,536 フレームを入れると 61,439 フレームになり、最新のフレームを表示できる (もう一度同じ数を入れても同じ)。見ていたフレームの位置が保たれ、捨てられたら最も古いフレームに移る。得点の位置がずれ、捨てたものは除かれる。古い描画命令が捨てられる。ログ (`handleShow`) は 65,537 フレームを全部持つ。
4. B9: オフサイドラインの計算を `src/overlay_math.{h,cpp}` (`OverlayMath::offsideLines`) に移し、`side_` (文字) ではなく `side()` で比べる。テスト `overlay_math`: 後ろから 2 人目、ボールのほうが近いとき、自陣に 2 人いないとき、ピッチの端、接続していない選手。
5. B10: キーパーのキャッチ範囲の色 (2 か所、B9 と同じ原因)。テストはない (描画の色の選び方だけ)。
6. B11: プレイモードごとのボールの位置を `src/play_mode_position.{h,cpp}` (`playModeBallPosition`) に移し、`MainWindow::changePlayMode` は受け付けたすべてのモードで、ボールの移動とモードの変更の 2 つを送る。テスト `play_mode_position`: back_pass の位置、ほかのモードの位置 (前と同じ)、ピッチの外の点、受け付けないモード。
7. B12: `ConfigDialog::clickSelectFix` が `SELECT_FIX` を入れる。テスト `config_dialog`: スロットを呼んで確かめる (「Fix」のボタンは視点と選手の選択の 2 つあるため、スロットを名前で呼ぶ)。
8. B13: 色の一覧の Illegal Defense の初期値。テスト `config_dialog`: 一覧の項目の色が Illegal Defense の色 (Pointto の色ではない)。
9. B14: 並べ替えのクラスを `src/player_type_sort_proxy.h` に移し、セルの先頭の数値で比べる (web 版の `parseFloat` と同じ)。テスト `player_type_sort`: 数値が 2 つある列、1 つの列、先頭の数値の読み方。

**試験 (手元、GCC 13、Qt 5.15.13)**
- B5、B1、B9、B12、B13、B14 のテストは、直す前のコードに一時的に戻すと失敗し、直すと通ることを確かめた。B1 は、添字をずらす処理だけを一時的に止めた場合も、2 つのテストが失敗することを確かめた。B11 の元の不具合は「送らない」ことなので、関数のテストでは直接は確かめられない。位置の計算はすべてのモードで前と同じで、受け付けたモードは必ず 2 つを送る形にした。
- 1〜9 のどの段階も、きれいなコピーで CMake のビルドとテストが通る (段階ごとに 1〜7 個のテスト)。autotools でファイルを足した段階 (4、6、9) は autotools のビルドも通る。新しい警告はない。

**見つけたこと**
- `DispHolder::handleTeam` は、得点が変わると `M_disp_cont.size() - 1` を得点の位置にする。フレームがまだないとき (試合の途中で接続して、最初の show の前にチームの情報が来たとき) は `size_t(-1)` になる。`trimLiveFrames()` は、捨てる前の数より大きい位置を除くので、これも除かれる。
- `player_painter.cpp:610` にも `side_ == LEFT` があるが、`#if 0` の中で使われていないので直していない。
- `PLAYMODE_STRINGS` は、最後のプレイモードの後に空の文字列が 2 つあり、`PM_MAX` (52) より 2 つ多い (54)。
- `Options` は `instance()` の中の静的な変数で、作られたときに `~/.rcssmonitor.conf` を読み (なければ書き)、プログラムの終了時に書く (`writeSettings(false)`: 窓の大きさ以外)。`--version` で終わるときも書く。
- 2026-11-11 15:27 に、AppImage の確かめのために入れた `rcssmonitor --version` を、`HOME` を一時ディレクトリにせずに動かし、ユーザの `~/.rcssmonitor.conf` を書き直してしまった (§0.1 に反する)。値は読んだものをそのまま書き戻すので、中身は変わらない見込み (ファイルになかった項目が既定値で足された可能性と、並びの変化はある)。ユーザに報告し、§0.1 に「`--version` でも同じ」と書き足した。

#### 2026-11-12: ユーザの確認と取り込み

- PR #64 (`fix/parity-bugs`) を Merge Commit で develop に取り込んだ (c269eee)。develop の CI も通った。
- ユーザが実際のログとライブで確かめた: B9、B10、B11、B12、B13、B14 はどれも OK。B1 は synch_mode で速く回し、65,535 フレームを超えたあたりで、古いフレームがまとめて消えることを確かめた。
- Phase 1 の完了条件 (不具合ごとのテスト、CI、実際のログでの確認) を満たした。

### Phase 2

#### 2026-11-12: 読み込みの高速化 (`feature/json-parser`、まとまり A)

**決めたこと (ユーザ)**
- Phase 2 では、パーサの改善に加えて、simdjson を更新し、読み込みをさらに速くする (ユーザ: web 版より体感で遅い)。
- 方針: (1) ファイルを 1 つのバッファに読み、メモリから解析する、(2) JSON を 1 行 1 レコードで読む、(3) 行を並列に解析する、(4) 進捗表示を間引く、(5) simdjson を 5.x にする。libdeflate (速い展開) は zlib のまま測ってから決める。別スレッドでの読み込み (読み込み中も画面を固めない) は後回し。
- 作業は、まとまり A (高速化) と B (頑健化: B2〜B4、B6〜B8 と §7 の残り) に分け、まとまりごとにコミットメッセージを確かめる。

**測ったこと (直す前)**
- アプリの `parsing elapsed` (展開と解析): offscreen で JSON 約 378 ms、テキスト約 222 ms。ユーザが実際の画面で JSON 371 ms、テキスト 220 ms。
- 内訳 (JSON 53.6 MB、6,209 フレーム / テキスト v5 28.7 MB、8,406 フレーム): `gzifstream` での展開 139 / 81 ms (zlib の `gzread` で 1 MB ずつなら 65 / 46 ms)。JSON パーサの中の `istreambuf_iterator` での全文のコピー 99 ms。解析と格納 約 100 / 80 ms。進捗表示 約 150 / 90 ms (32 サイクルごとの更新と、512 サイクルごとの `processEvents()` でアプリの窓が描き直される)。
- web 版は Worker で解析し、進捗を状態行に出す (画面は固まらない)。読み込み中に途中のフレームは見せない。

**作ったもの (順に 1 コミットずつ。時間は `bench_load` の 5 回の最良、進捗表示なし)**

| 段階 | 内容 | JSON | テキスト |
|---|---|---|---|
| A1 | 測定用のプログラム `test/bench_load` (アプリと同じ道で読み、最良の時間を出す。`--progress` で進捗表示も) | 235 ms | 130 ms |
| A2 | simdjson を 5.1.1 にした (3.8.0 に入れた GCC 15 の修正は上流にある) | 222 ms | 130 ms |
| A3 | ファイルを `gzread` で 1 つのバッファに読む (`src/game_log_file`)。メモリから解析する入口 `Parser::create( data, size )`、`parse( data, size, handler )` を足した。JSON は simdjson への 1 回のコピーだけ | 179 ms | 121 ms |
| A4 | JSON を 1 行 1 レコードで、その場で解析する (コピーなし)。`[` と `]` の行と行末の `,` を飛ばすので、末尾の `]` が欠けても読める。解析できない行は警告を出して飛ばす (§7 の 4) | 151 ms | — |
| A5 | テキストもメモリから 1 行ずつ読む (`std::getline` を使わない) | — | 122 ms |
| A6 | 行を並列に解析する (`rcss/rcg/line_parser`)。最大 8 スレッド、1 スレッドに 1,024 行以上。各スレッドは記録用の Handler に渡し、終わってから行の順に本当の Handler に渡す | 84 ms | 60 ms |
| A7 | 進捗表示 (`RCGHandler`) を 100 ms ごとに間引き、0.5 秒以上かかるときだけ出す | — | — |

**結果 (アプリの `parsing elapsed`、offscreen、3 回)**: JSON 275〜365 ms → 85〜118 ms、テキスト 214〜216 ms → 93〜102 ms。アプリは 1 回だけ読むので、2 回目以降より 30〜35 ms 遅い (大きなメモリの初めての確保、スレッドの起動など。`bench_load -n 1` で 94〜110 / 91〜96 ms)。残りの時間の多くは展開 (zlib、並列にできない)。

**試験**
- テスト `rcg_parser`: メモリからの入口 (テキスト、JSON)、末尾の `]` の欠け、壊れた行、1 行の JSON (文書全体として読む)、CR LF、大きなログ (5,000 フレーム。並列の道) で show とプレイモードの順が保たれること、テキストの不正な行で止まること、JSON の壊れた行を飛ばすこと。
- テスト `line_parser` (新): `split_lines`、2 万行の順、失敗した行 (例外を含む) の番号、最初の失敗で止まるとき後のブロックを渡さないこと。ブロックを逆順に渡すと失敗することも確かめた。
- テスト `game_log_file` (新): 圧縮していないファイル、1 MB を超える gzip、ないファイル、gzip のログを読んで解析する。
- 9 個のテストがすべて通る。A4 のテストは、文書全体として読む A3 の動きに戻すと失敗することを確かめた。

**見つけたこと**
- この作業の端末では conda (`anaconda3/envs/catboost`) が `PATH` に、`~/local/lib` などが `LD_LIBRARY_PATH` に入っていて、CMake が conda の Qt と zlib を拾っていた。速さは、それらを外した環境 (システムの Qt 5.15.13 と zlib) でビルドし、測り直した (§10.3)。値はほぼ同じだった。
- `perf` は `perf_event_paranoid=4` のため使えない。段階ごとの時間は測定用のプログラムで測った。
- simdjson 5.1.1 は `simdjson.h` が 10 MB (3.8.0 は 3.8 MB)。コンパイルの時間は `simdjson.cpp` 3.5 → 9.6 秒、`parser_simdjson.cpp` 6.4 → 7.7 秒。`get_string()` の戻り値を使わないと警告が出るので直した。ライセンスの違いは年だけ。
- アプリは `gzfstream` (`gzifstream`) を使わなくなった。ファイルは残してある (消すかは後で決める)。 → 後で消した (下の記録)。
- ファイルのパスは `toLatin1()` ではなく `QFile::encodeName()` で渡す (日本語を含むパスも開ける)。
- テキストのヘッダの版を `std::stoi` ではなく `std::atoi` で読む (`ULGx` のような壊れたヘッダで例外が出ない)。
- JSON を 1 行ずつ読むようにしたので、解析できないレコード (未知のキーを含む。B3) は、その行だけ飛ばす (前は解析全体が止まった)。B3 では、未知のキーだけを飛ばすようにする。
- `Handler::handleTimestamp()` は仮想関数でないので、並列のときの記録用の Handler では記録できない。§7 の 7 (timestamp を Handler に渡す) のときに直す。
- 並列のとき、Handler の関数の戻り値は使わない (アプリの Handler はいつも true を返す)。

#### 2026-11-12: libdeflate (`feature/json-parser`)

まとまり A の 8 コミット (d36e1a9〜8b9d510) をプッシュした。CI は CMake と autotools の両方が通った。同梱の simdjson の警告 (`-Wtemplate-id-cdtor` 9 件) は 5.1.1 でなくなり、コンパイラの警告はない。なお、ブランチを作ったときの CI (c269eee) は、ランナーが `security.ubuntu.com` の名前を引けずにパッケージを入れられなかったために失敗した (コードとは関係ない)。

**決めたこと (ユーザ)**
- 展開を速くするため、libdeflate を任意の依存として使う (見つからなければ zlib だけで動く)。

**測ったこと (直す前)**: 展開だけで、zlib (`gzread`、1 MB ずつ) が JSON 69 ms、テキスト 48 ms。libdeflate (ファイルを読み、一度に展開) が 34 ms、22 ms (読み込みの 7.5 ms、6.5 ms を含む)。

**作ったもの**
- `src/game_log_file.cpp`: libdeflate があれば、ファイルをそのまま読み、gzip なら一度に展開する。展開先の大きさは末尾の ISIZE で決め、足りなければ倍にする (deflate の最大の比 1032:1 を上限にする)。複数のメンバをつないだ gzip を読み、メンバの後の gzip でないバイトは zlib と同じく無視する。libdeflate が展開できないデータ (途中で切れたもの) は、zlib の道で読み直す。zlib は、途中で切れた gzip を切れたところまで読む (サーバが異常終了したログ。§7 の 4)。
- CMake: `ENABLE_LIBDEFLATE` (既定 ON)。見つかれば `HAVE_LIBDEFLATE` を定義する (古いディストリビューションの libdeflate には CMake の設定ファイルがないので、`find_path` と `find_library` で探す)。autotools: `--with-libdeflate` (既定は見つかれば使う。`--without-libdeflate` で使わない、`--with-libdeflate` で見つからなければエラー)。ライブラリは `LIBDEFLATE_LIBS` でアプリにだけ付ける (全体の `LIBS` に足すと、Qt を確かめる `AX_HAVE_QT` の試しのリンクにも付く)。
- CI: CMake のジョブに `libdeflate-dev` を入れ、autotools のジョブには入れない (両方の道をコンパイルする)。README に、任意の依存であることを書いた。

**結果 (アプリの `parsing elapsed`、offscreen、3 回)**

| ビルド | JSON | テキスト |
|---|---|---|
| CMake、zlib だけ | 91〜119 ms | 95〜100 ms |
| CMake、libdeflate | 63〜76 ms | 59〜60 ms |
| autotools、zlib だけ | 117〜119 ms | 76〜106 ms |
| autotools、libdeflate | 76〜77 ms | 59〜64 ms |

`bench_load` (5 回の最良) は、zlib だけで 84〜117 / 60 ms、libdeflate で 53〜74 / 33〜42 ms。

**試験**
- テスト `game_log_file` に足した: 複数のメンバの gzip (最後のメンバが小さいので、ISIZE から決めた大きさが足りず、広げる道を通る)、途中で切れた gzip (切れたところまで読む)、gzip の後の余計なバイト。
- CMake で libdeflate なしとありの両方、autotools で 3 つの場合 (見つかれば使う、`--without-libdeflate`、見つからないときの `--with-libdeflate`) を、きれいなコピーで確かめた。CMake の 2 つは 9 個のテストがすべて通る。
- 失敗しても zlib に戻るので、テストが通るだけでは libdeflate の道を通ったかがわからない。一時的に表示を入れて、ふつうの gzip、複数のメンバ、余計なバイトは libdeflate で、途中で切れたものは zlib で読むことを確かめた。
- 手元には `libdeflate-dev` がないので、`apt-get download` で取ってきたパッケージを一時ディレクトリに展開して使った (ライブラリはシステムの `libdeflate.so.0`)。

#### 2026-11-12: パーサの頑健化 (`feature/json-parser`、まとまり B)

libdeflate の 2 コミット (e0e21ef、6328ac9) をプッシュした後、§7 の残りと B2〜B4、B6〜B8 を直した。

**作ったもの (順に 1 コミットずつ)**

| 段階 | 内容 | テスト (`rcg_parser` ほか) |
|---|---|---|
| B-1 | JSON: 未知のレコードとキーを飛ばす (B3)。未知のキーは、プロセスの中で 1 回だけ報告する (ライブでは毎回のメッセージに入るため)。文書全体として読むときも、読めないレコードを飛ばして続ける。ライブの入口 (`ParserSimdJSON::parseData`) で simdjson の例外を受け止める (前は受け止めず、壊れたメッセージで落ちるおそれがあった。protocol.md §3.6: 文字列の中の改行などで、サーバが不正な JSON を送ることがある) | 未知のレコード、レコードの中の未知のキーと既知のキー、show と選手の中の未知のキー (1 行 1 レコードと文書全体)、文書全体での読めないレコード、ライブの壊れたメッセージ |
| B-2 | JSON: `vq` を `'h'` / `'l'` で入れる (B2) | `vq` が `"l"` の選手 |
| B-3 | JSON: 選手を `side` と `unum` で格納する (B4)。不正な選手は飛ばし、重なった選手は後のものにする | 逆順、欠けた選手、不正な side と unum、重なった選手 |
| B-4 | JSON: 省略されうる項目を受け付ける (§7.5)。必須は show の `time` と `ball`、選手の `side` と `unum` だけにし、ほかは欠けたら `PlayerT` の既定値 (値なしの印) のままにする。前は `capacity` などが 1 つ欠けると、その選手から後が読まれず、ボールの速度が欠けると show ごと捨てていた。show の中のチームに show の時刻を渡す (前は -1)。チームの読み込みに失敗しても show は捨てない | ライブの形の show (モードとチームを含み、コマンド回数がない)、`capacity`・`px`・`fside` などのない選手、`stime`、`pen_*`、`null` のチーム名、`time` のない msg |
| B-5 | JSON: ボールの `z` と `vz` を、あれば読む (§7.6)。`BallT` に `z_`、`vz_` (既定 0) を足した | 高さのある show とない show |
| B-6 | JSON: `version` と `timestamp` を Handler に渡す (§7.7)。並列のときの記録用の Handler で記録できるよう、`Handler::handleTimestamp()` を仮想関数にした | 逐次と並列の両方 |
| B-7 | テキスト: チームグラフィックの文字列を長さを決めずに読む (B6)。1 画素 2 文字の XPM (8 画素で 16 文字) が壊れていた | 16 文字の行のある XPM、サーバが書く形 |
| B-8 | テキスト: 読めない行を飛ばして続ける (B8)。show などの解析の失敗も「飛ばした行」に数え、最初の 10 行の番号と合計を出す (JSON と同じ `report_skipped`)。空行は飛ばすだけ。単独の team 行の `null` を空のチーム名にする。show のボールの 4 つの値が読めなければ行を飛ばす (前は 0 として読んでいた)。`unum` が 0 の選手で `player_[-1]` に書き込んでいたのを直した。`parse_lines` の「最初の失敗で止める」は使うものがなくなったので除いた | 不正な行、空行、未知のレコード、ボールの壊れた show、途中で切れた show、`unum` 0 の show、`null` のチーム名 |
| B-9 | パラメータの既定値を asio 版 rcssserver 19.0.0 にそろえる (B7。web 版の `param-defaults.ts`)。server_param 33 項目、player_param 8 項目、player_type 4 項目 (`dash_power_rate` 0.06 は 0.006 の打ち間違い。`team_far_length` は 2022 年にピッチの対角 125.096 に変わった)。TCP / WebSocket のサーバが送る `tcp_port`、`tcp_coach_port`、`tcp_olcoach_port`、`ws_port`、`ws_allowed_origins` を足した (前は「Unsupported parameter」と出ていた)。`PlayerTypeT` の書き出しで、v19 の 4 つのノイズの率がどれも `dist_noise_rate` の値になっていたのを直した | 新しいテスト `rcg_types` |

**試験**
- 10 個のテストがすべて通る。どの段階のテストも、直す前のファイルに戻すか、直した箇所だけを前の動きに戻すと失敗することを確かめた。最初は、1 行 1 レコードでは未知のレコードの行を丸ごと飛ばしても結果が同じになり、文書全体で読めないレコードのテストもなかったので、その 2 つのテストを足した。
- 全試合のログ 2 本を、まとまり A の終わりのパーサと今のパーサで読み、Handler に渡るものをすべて (show の全項目、プレイモード、チーム、メッセージ、チームグラフィック) 書き出して比べた。違いは、JSON の `vq` の値 (前は bool の 1 を文字として入れていた) と、`version` と `timestamp` が渡るようになったことだけ。
- B1〜B9 のどの段階も、きれいなコピーで CMake のビルドとテストが通る。B9 は autotools でも警告なしでビルドできる。

**見つけたこと**
- `origin/feature/json-reformat` (§7.9) は、消えた `parser_json.cpp` (nlohmann/json を使う古いパーサ) への変更で、今の `parser_simdjson.cpp` にない内容は残っていない。取り込まない。同梱の `rcss/rcg/nlohmann/json.hpp` は、どこからも使われていない。
- テキストの show の解析は、選手の部分の数値が読めたかを確かめていない (途中で切れた行は、選手の数の検査で捕まる)。ボールと `unum` だけを直した。
- 速さは変わらない。アプリの `parsing elapsed` (offscreen、3 回、libdeflate あり) は、CMake で JSON 69〜72 ms、テキスト 59〜61 ms、autotools で 67〜73 ms、59〜61 ms (まとまり B の前は 63〜76 / 59〜64 ms)。

#### 2026-11-12: ユーザの確認と、使われていないファイルの削除

- まとまり B の 10 コミット (0bf9ee3〜caf490b) をプッシュした。
- ユーザが実際の画面で確かめた: 読み込みの速さとライブ接続は OK。視界の質 (`vq`、B2) は、rcssmonitor のどの表示にも使われていない (`src/` に `view_quality_` を使う箇所がない) ので、画面では確かめられない。B2 はパーサのテストで確かめた。
- ユーザの決定: 使われていない `rcss/rcg/nlohmann/json.hpp` (古い JSON パーサ用) と `src/gzfstream.{h,cpp}` (まとまり A から使っていない) を消す。あわせて、`main_window.cpp` に残っていた中身のない `#ifdef HAVE_LIBZ` を消し、ファイルを開くダイアログの `*.rcg.gz` を、zlib がなく libdeflate だけのときにも出す。

#### 2026-11-12: 取り込み

- PR #65 (`feature/json-parser`) を Merge Commit で develop に取り込んだ (a6f2456)。develop の CI も通った。

### Phase 3

#### 2026-11-12: Qt6 対応 (`feature/qt6`)

ユーザが GitHub 上で develop (a6f2456) から `feature/qt6` を作った。

**準備**
- 作業マシンには Qt6 の実行用のライブラリしかなかった (Phase 0 の記録を訂正した)。ユーザが `qt6-base-dev` と `libqt6opengl6-dev` (6.4.2) を入れた。
- Qt5 で `QT_DISABLE_DEPRECATED_BEFORE=0x050F00` を付けてビルドすると、エラーは `setMargin` (27 か所) だけだった。ほかの多くは Qt 6 で非推奨になったものなので、Qt6 でビルドして洗い出した: 置き換える前のソース (Q1) を今の CMake で Qt6 でビルドすると、エラーは 49 件 (`setMargin` 27、`Qt::CTRL + Qt::Key_X` の `+` 21、`QKeySequence` の要素の `int` への変換 1)。

**決めたこと**
- CMake は、Qt6 が見つかればそれを使い、なければ Qt5 を使う。`-DQT_VERSION_MAJOR=5` で Qt5 を選べる。下限は Qt 6.2 と Qt 5.15 (D2)。
- 非推奨の API は、ビルドのときに使えなくする: `QT_DISABLE_DEPRECATED_BEFORE` を Qt5 では 0x050F00、Qt6 では 0x060000 にする。Qt 6.1 以降に非推奨になったものは、CI で `-Werror=deprecated-declarations` にして捕まえる。
- `QMouseEvent::pos()` は、Qt 6.2、6.4、6.10 のどれでも非推奨ではない (qtbase のヘッダで確かめた。非推奨なのは `globalPos()`、`x()`、`y()` と、ほかのイベントの `pos()`)。§3.3 の「`pos()` は Qt 6 で非推奨」は誤りだったので、版で分ける関数は作らない (一度作ったが取りやめた)。
- `QTime` は Phase 2 の A7 で `QElapsedTimer` に置き換え済み。

**作ったもの (順に 1 コミットずつ)**

| 段階 | 内容 | テスト |
|---|---|---|
| Q1 | Qt 5.15 より古い Qt 向けの分岐を消した (`QT_VERSION_CHECK` 22 か所、Qt 4 の版を 16 進で比べるもの 4 か所)。`QFontMetrics::width()`、`QWheelEvent::delta()`、`setResizeMode` などは、古い側にだけあった | — |
| Q2 | `setMargin( n )` を `setContentsMargins( n, n, n, n )` にした (Qt6 で削除) | — |
| Q3 | キーの組み合わせを `Qt::CTRL \| Qt::Key_X` にした (Qt6 で `+` は削除)。Qt 4 のマクロ `Q_WS_MAC` の分岐 (21 か所。Qt 5 からは定義されず、使われていなかった) と `Q_WS_WIN` を消した。選手をキーで選ぶアクションは、ショートカットのキーの値を `int` として読んでいた (Qt6 では `QKeyCombination` になる) ので、選手の番号を `setData()` で持たせた。10 番目の選手のアクションの名前が「Select Left 0」、右の選手のが「Selct Right」になっていたのも直した | `player_select_keys` (新): メインウィンドウのショートカット 1〜9、0、- と Ctrl 付き。番号を持たせない前の `main_window.cpp` に戻すと失敗する。メインウィンドウのすべてのアクションの名前とショートカットを書き出して前と比べ、選手を選ぶアクションの名前と番号のほかは同じ |
| Q4 | `QSignalMapper` (2 か所、Qt 5.15 で非推奨) をラムダの接続にした | `play_mode_actions` (新): フィールドのメニューの「Change Playmode」の項目が、そのモードを `FieldCanvas` に渡す。渡すモードを間違えると失敗する |
| Q5 | `USE_GLWIDGET` と `QGLWidget` を消した (どのビルドの設定も定義していなかった。`QGLWidget` は Qt6 にない) | — |
| Q6 | CMake を Qt5 と Qt6 の両方に対応させた (`find_package(QT NAMES Qt6 Qt5)`、`Qt${QT_VERSION_MAJOR}::…`)。`cmake_minimum_required` を 3.16 にした (§8 の 10)。`QT_DISABLE_DEPRECATED_BEFORE` を付ける | Qt6 と Qt5 で 12 個 |
| Q7 | CI: CMake のジョブを Qt5 と Qt6 の 2 つにし (`-Werror=deprecated-declarations`)、下限の Qt 6.2 を確かめるジョブ (`ubuntu:22.04` のコンテナ、CMake 3.22) を足した | — |
| Q8 | README のインストールの手順を書き直した: CMake を主にし、Ubuntu 22.04 / 24.04 / 26.04 の Qt6 と Qt5 のパッケージ、Qt の選び方、autotools は Qt5 だけ、CMake のオプション。C++14 を C++17 に直し、要らない boost などのパッケージを除いた | — |
| Q9 | Qt5 でも、画面の倍率で拡大して描く (`AA_EnableHighDpiScaling`、端数の倍率もそのまま)。Qt6 は常にそうするので、同じキャンバスの大きさで同じウィンドウになる (下のユーザの確認) | 手元の画面で、Qt5 の倍率と論理 DPI が Qt6 と同じになることを確かめた |
| Q10 | ツールバーのアイコン (黒い図柄の XPM) を、配色の文字の色で描く (白は透明、灰色は半透明)。配色が変わったら描き直す。暗い配色では黒い図柄が見えにくく、白い四角の背景が浮いていた | `tool_icon` (新): 色の付け直し、配色を変えたときの描き直し。描き直しを止めると失敗する。暗い配色と明るい配色で、画面外でツールバーを描いて見た |

**試験**
- Q1〜Q10 のどの段階も、きれいなコピーで CMake のビルドとテストが通る。Q6 以降は Qt 6.4.2 と Qt 5.15.13 の両方で、`-Werror=deprecated-declarations` を付けても警告なしで通る (テスト 12 個、Q10 から 13 個)。最後の段階は autotools (Qt5) でもビルドできる。
- 高 DPI (§8 の 6): Qt6 で、ログを読んで 1,000 フレーム目に進めたメインウィンドウを、画面外で倍率 1、1.5、2 で描いて画像にした。配置、文字、ボタンは崩れない。フィールドの線と選手の輪郭は 1 物理画素のままなので、倍率 2 では相対的に細い。チームのロゴ (XPM) は拡大されてぼやける。画面外の描画では、Qt6 のメニューと得点板の文字が Qt5 より少し小さい (既定のフォントの違いと見られる)。実際の画面での確認はユーザが行う。
- Ubuntu 22.04 (Qt 6.2) は手元に Docker がないので試していない。CI で確かめる。

**ユーザの確認 (Qt6、実際の画面)**
- ライブ接続、ログの再生、審判の操作、設定ダイアログは問題ない。
- Qt5 と Qt6 で、同じキャンバスの大きさでもウィンドウの大きさが違い、チームグラフィックの大きさも違う。原因: 作業マシンの画面は 3840×2160、DPI 124 (Wayland の GNOME。Qt5 と Qt6 はどちらも XWayland で動く)。Qt6 は常に画面の倍率 (124 / 96 = 1.29) で拡大して描き、Qt5 は画素を拡大せずに文字だけを DPI 124 で大きくしていた。Qt6 が一貫した描き方で、Qt5 では文字に比べてチームグラフィックや線が小さかった。ユーザの決定: Qt5 も Qt6 と同じく拡大する (Q9)。
- デスクトップが暗い配色のとき、Qt6 のウィンドウも暗くなり (背景 #323232。Qt5 は明るいまま)、ツールバーのアイコンが見えにくい。ユーザの決定: アイコンを配色に合わせる (Q10)。
- Qt6 で JSON の読み込みが 800 ms 以上、テキストが 70 ms 以上かかる。原因: ユーザのビルドのディレクトリが `CMAKE_BUILD_TYPE=Debug` だった (simdjson が最適化されない)。Qt6 の Debug のビルドで、JSON 692〜868 ms、テキスト 73 ms になることを確かめた。Release では Qt6 も Qt5 も JSON 70 ms、テキスト 60 ms で同じ。ユーザのマシンには `libdeflate-dev` もない (zlib だけ)。

#### 2026-11-12: 取り込み

- PR #66 (`feature/qt6`) を Merge Commit で develop に取り込んだ (95965dc)。develop の CI も通った。Phase 3 の完了条件 (Qt 5.15 と Qt 6 の両方でのビルド、非推奨の警告なし、CI、ユーザの確認) を満たした。

#### 2026-11-12: Qt の選び方の不具合 (`fix/qt-selection`)

- 3D の試作 (下) をビルドしたとき、`-DQT_VERSION_MAJOR` を付けずに構成すると、Qt5 と Qt6 の両方が入った作業マシンで Qt5 が選ばれることに気づいた (Phase 3 のときは Qt6 が選ばれていた)。原因: `find_package(QT NAMES Qt6 Qt5)` は、名前を並べた順ではなく、`/usr/lib/x86_64-linux-gnu/cmake/` の中のディレクトリを読み出した順で見つける。Qt Quick 3D のパッケージを入れてディレクトリが増え、順が変わった。README の「Qt6 があれば Qt6」が保証されていなかった。CI は版を明示しているので影響しない。
- 直したこと: 指定がなければ、まず Qt6 (6.2 以降) を探し、見つからなければ Qt5 にする。決まった版はキャッシュ (`QT_VERSION_MAJOR`) に残し、構成し直しても変わらない。指定なし、`5`、`6`、Qt6 を探さないとき (`-DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON`) で、それぞれ Qt 6.4.2、5.15.13、6.4.2、5.15.13 になることを確かめた。

### Phase 0 (続き)

#### 2026-11-12: 3D の試作と D1 (`proto/3d-gl`)

Phase 1〜3 を取り込んだので、3D の試作に入った。

**決めたこと (ユーザ)**
- 試作は develop (95965dc) から作った `proto/3d-gl` で行い、取り込まない (§9.1)。使える部分は、後で `feature/3d-view` に整えて持っていく。記録は develop の計画書に入れる (この区切りは `fix/qt-selection` で入れた)。
- 推奨案 (`QOpenGLWidget` と自前の描画) と Qt Quick 3D の両方を作って比べる。

**作ったもの (`proto/3d-gl` の bf554fe、e9a9b65、c54d791)**
- `resources/models/Mannequin.glb`: web 版のモデルを、glTF-Transform 4.5.1 と meshoptimizer で、EXT_meshopt_compression と KHR_mesh_quantization を外して書き出し直した (346 kB → 684 kB。D6)。書き出したスクリプト `decompress.mjs` と出典の README を添えた。メッシュ 1 つ (材質 M_Main と M_Joints)、骨 65 本、クリップ 4 つ (骨ごとに位置、回転、拡大)。
- `third_party/cgltf/`: cgltf 1.15 (MIT。D5)。
- `proto/3d-gl/` (CMake の `BUILD_PROTO_3D`、既定 OFF): 2 つの試作が、同じログ (`1-json.rcg.gz`) で、同じ場面 (地面とライン、人型 22 体、ボール、太陽の影) を描く。姿勢は共通のコード (`gltf_rig`、`proto_log`) で、ログから毎フレーム求める (§5.8。速さで待機・歩き・走り・全力疾走を混ぜ、それぞれ歩いた距離 / そのクリップの速さの時刻)。
  - `proto_gl`: OpenGL 3.3 core、`QOpenGLWidget`、自前のシェーダ。Qt5 と Qt6。骨の行列を RGBA32F のテクスチャに入れ、材質ごとに 1 回のインスタンス描画で 22 体を描く。影の地図は 4096 px、5×5 の PCF。描画の本体 (`SceneGL`) をウィジェットから分けたので、画面のない環境でも `QOffscreenSurface` と FBO で描ける (`--offscreen`)。
  - `proto_quick3d`: Qt Quick 3D、`QQuickWidget` (`--window` で `QQuickView`)。Qt6 だけ。モデルはビルド時に balsam で QML に変換し、クリップの `Timeline` を除く (`make_quick3d_model.py`)。骨のノードを QML の id で見つけ、毎フレーム `position`、`rotation`、`scale` を書き込む。
  - `run_bench.sh`: 2 つを順に、1920×1080 で 10 秒ずつ測る。

**計測 (ユーザのマシン。GPU は Ryzen 9 9950X3D の内蔵 GPU (radeonsi、raphael)、Mesa 25.2.8、OpenGL 4.6 core。1920×1080、10 秒)**

| | fps (垂直同期あり) | fps (なし) | CPU / フレーム | GPU / フレーム |
|---|---|---|---|---|
| OpenGL (Qt6)、人型、影 4096 px | 59.9 | 60.0 (下の注) | 0.29 ms (姿勢 0.26 ms) | 4.44 ms |
| OpenGL (Qt6)、人型、影なし | — | 60.0 | 0.25 ms | 1.02 ms |
| OpenGL (Qt6)、円柱、影あり | — | 60.0 | 0.03 ms | 3.81 ms |
| OpenGL (Qt5)、人型、影 4096 px | — | 158 | 0.27 ms | 4.42 ms |
| Qt Quick 3D (`QQuickWidget`)、人型、影 | 59.9 | 128.6 | 5.3〜5.9 ms (p99 9.7 ms) | 測れない |
| Qt Quick 3D、骨の回転だけを書く | — | 128.6 | 4.2 ms | — |
| Qt Quick 3D (`QQuickView`) | — | 128.9 | 5.4 ms | — |
| Qt Quick 3D、影なし | — | 161 | 5.2 ms | — |
| Qt Quick 3D、円柱 | — | 181 | 0.01 ms | — |

**わかったこと**
- どちらも内蔵 GPU で 60 fps を保つ。OpenGL は GPU 4.4 ms (うち影 3.4 ms。影なしで 1.0 ms)、CPU 0.3 ms で、約 4 倍の余裕がある。
- Qt Quick 3D は、骨をプロパティで書くのに、CPU を毎フレーム 5〜6 ms (p99 で約 10 ms) 使う (22 体 × 65 本 × 3 つ。回転だけでも 4.2 ms)。60 fps の 1 フレーム (16.7 ms) の 3〜6 割にあたる。
- Qt Quick 3D 6.4 では、骨つきのモデルが影をほとんど落とさない (円柱は落とす。元の姿勢の範囲で影の対象を絞っているなどが考えられる。新しい Qt では確かめていない)。表の Qt Quick 3D の値は、人型の影を描かずに出た値。
- Qt6 の `QOpenGLWidget` は、`swapInterval` を 0 にしても 60 fps に止まる (窓の合成が RHI を通るためと見られる。Qt5 では 158 fps)。余裕は GPU の時間 (タイマーのクエリ) で測る (§10.3)。
- Qt Quick 3D の準備で詰まった所: Ubuntu 24.04 の `qt6-quick3d-dev` は `qt6-shadertools-dev` (と `qsb` の入った `qt6-shader-baker`) に依存していないので、別に入れる。balsam の出力 (5,332 行) の大半はクリップの `Timeline`。Qt 6.4 の `QQuickWidget` には `setInitialProperties` がない。光源 (`Node`) の `lookAt` は C++ から呼べない (カメラは呼べる)。`QQuick3D::idealSurfaceFormat()` は、アプリケーションを作る前には呼べない。Qt 6.4 の影の地図の最高は VeryHigh。
- 画面のない環境 (`QT_QPA_PLATFORM=offscreen`) では、`QOpenGLWidget` も Qt Quick の窓も OpenGL を使えない (Qt Quick はソフトウェアの描画に切り替わる)。試作では、`QOffscreenSurface` と FBO (`proto_gl`)、`QQuickRenderControl` (`proto_quick3d`。`QT_QUICK_BACKEND=rhi`) で描いて確かめた。§10.2 の描画のテストも、描画の本体をウィジェットから分けて、この形にする。
- ソフトウェアの描画 (llvmpipe、1280×720) では、`proto_gl` の人型と影が 1 フレーム約 36 ms (姿勢の計算は 0.2 ms)、`proto_quick3d` が約 75〜80 ms。

**D1 の決定 (ユーザ)**: 推奨案 (`QOpenGLWidget` と自前の描画、OpenGL 3.3 core) に決めた。理由: 内蔵 GPU でも余裕を持って 60 fps (GPU 4.4 ms、CPU 0.3 ms)。Qt5 と Qt6 の両方で同じコードが動く (D2)。ログから毎フレーム姿勢を決める作り (§5.8) が安く書ける。依存が少ない (Qt の OpenGL と cgltf だけ)。Phase 4 では、試作の `gltf_rig` (glTF の読み込みと姿勢)、`SceneGL` の分け方、影の地図、骨つきのインスタンス描画を、整えて持っていく。

### Phase 4

#### 2026-11-12: 区切り 4-1、3D の土台 (`feature/3d-view`)

PR #67 (`fix/qt-selection`) を Merge Commit で develop に取り込んだ (2a88791)。ユーザが develop から `feature/3d-view` を作った。

**決めたこと**
- Phase 4 を 5 つの区切りに分ける: 4-1 土台 (`ENABLE_3D`、`FieldView3D`、地面、2D と 3D の切り替え)、4-2 時間と補間、4-3 選手、ボール、ゴール、4-4 カメラ、4-5 HUD と背番号のラベル。区切りごとにユーザに確かめる。
- 3D のコードは `src/view3d/` に置き、CMake でだけビルドする (D3)。`ENABLE_3D` (既定 ON。Qt6 では `OpenGL` と `OpenGLWidgets` が見つかったときだけ) で `config.h` に `HAVE_3D_VIEW` を定義する。autotools と `-DENABLE_3D=OFF` では、3D なしでビルドする。
- 描画の本体 (`Scene3D`) をウィジェット (`FieldView3D`) から分ける (試作の `SceneGL` と同じ)。描き先のフレームバッファを受け取るので、テストでは画面外の FBO に描ける (§10.2)。
- 3D の表示は、初めて 3D にするときに作る (2D だけを使う人の窓に、OpenGL の部品を入れない)。

**作ったもの**
- `FieldPainter::drawGround()`: ゴールを除いた地面を、ペンの太さを何倍かにして描く。`Options::fieldTransform()` / `setFieldTransform()`: フィールドの倍率と中心を一時的に差し替える (ズームの状態は変えない)。
- `src/view3d/ground_texture`: 2D の `FieldPainter` で、ピッチと余白 (115 m × 78 m) を 1 m = 24 px、ペン 3 倍 (12.5 cm) の画像に描く (web 版 `field-texture.ts`)。色、旗、グリッド、keepaway の設定が変わったときだけ描き直す (`key()`)。
- `src/view3d/camera_3d.h`: カメラの位置、見る点、上、画角 (または正射影の高さ) と、視野と射影の行列。GL に依存しない (4-4 のカメラの計算でも使う)。
- `src/view3d/scene_3d`: OpenGL 3.3 core の描画。今は地面 (光の当たらないテクスチャ、異方性フィルタ 8 倍まで) と空の色だけ。
- `src/view3d/field_view_3d`: `QOpenGLWidget` (OpenGL 3.3 core、マルチサンプリング 4 倍)。今のカメラは、中央を中継の位置 (72 m、仰角 32°) から見る固定のもの (4-4 で置き換える)。`unavailableReason()`: OpenGL 3.3 の文脈を作れるかと、プラットフォームが OpenGL のウィジェットを出せるか (offscreen、minimal、vnc、linuxfb は出せない) を確かめる。
- メインウィンドウ: 中央を `QStackedWidget` にし、2D の表示と 3D の表示を入れ替える。View > 3D View、ツールバーの「3D」、キー D (チェックつきのアクション)。3D を使えないときは、アクションを無効にし、理由をステータスバーとツールチップに出して 2D のままにする。設定 `view_3d` (`MainWindow` のグループ) とコマンドラインの `--view-3d` で、3D で始める。
- CI: Qt6 のジョブに `libqt6opengl6-dev` を足し、3D がビルドされたことを `config.h` で確かめる。テストは Xvfb と Mesa のソフトウェアの OpenGL (`LIBGL_ALWAYS_SOFTWARE=1`) で動かす。

**試験**
- テスト `ground_texture` (新): 画像の大きさ、芝・センターライン・ゴールのあった所の色、センターラインの太さ (約 3 px)、2D の倍率と中心が戻ること、鍵が色と keepaway で変わること。ペンの倍率を 1 にしたときと、ゴールも描いたときに失敗することを確かめた。
- テスト `scene_3d` (新): 画面外の OpenGL 3.3 の文脈と FBO で地面を真上から描き、センターラインと芝の色、2 回描いて同じになること。OpenGL 3.3 の文脈を作れない環境では飛ばす。
- テスト `view_switch` (新): 画面外のプラットフォームでは 3D を使えないので、D でアクションが無効になり、理由が出て、2D のまま、選手の選択も変わらないこと。実際の切り替えは、ユーザが画面で確かめる。
- 4-1 のどの段階も、きれいなコピーで、Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) でビルドとテストが通る (テスト 16 個)。`-DENABLE_3D=OFF` (13 個) と autotools も通る。
- 画面外で、中継の位置からの地面を描いて見た (2D と同じライン、ペナルティエリア、センターサークル)。

#### 2026-11-12: 区切り 4-2、4-3、時間と補間、選手、ボール、ゴール (`feature/3d-view`)

4-1 の 5 コミット (1956f13〜27f1100) をプッシュし、CI が通った後、ユーザが画面で確かめた。4-2 (時間と補間) だけでは画面で確かめられることが少ないので、4-3 (選手、ボール、ゴール) と合わせて 1 回で確かめてもらう。

**決めたこと**
- 何をどこに描くか (インスタンスの行列と色) は、GL に依存しない `SceneBuilder` が決め、単体テストで確かめる。`Scene3D` は、それを形ごとに 1 回のインスタンス描画で描くだけにする。
- 選手の色の決め方 (2D の `PlayerPainter::drawBody`) を関数 `playerStyle()` に切り出し、2D と 3D で共有する。
- カードは、ビルボード (背番号のラベル) と一緒に 4-5 で描く。足元の円の中の向きの線と、視野の扇形は、線の描画が要るので、Phase 5 の重ね描きと一緒に描く。
- 光は、three.js の値の写しではなく、太陽 (§4.3 の向き) の Lambert と空からの光の簡単な式にした (画面で見て合わせる。影は Phase 8)。

**作ったもの**
- テストの仕組み: Qt Test が飛ばしたテスト (`QSKIP`。OpenGL 3.3 の文脈を作れないときなど) を、CTest が「通った」ではなく「飛ばした」と報告する (`SKIP_REGULAR_EXPRESSION`)。4-1 の CI の Ubuntu 22.04 のジョブで `scene_3d` が 0.22 秒で通っていて、本当に描いたかがわからなかったため。
- `LogPlayer::alpha()`: 再生中に、最後に進めてからの経過時間 / 間隔 (0〜1)。止めているときは 0。`isPlaying()`。
- `src/view3d/interpolate`: フレームのあいだの補間 (web 版 `interpolate.ts`)。位置と速度は線形、体の向きは短い回り方向、首は体に対する角度なので線形。プレイモードが変わるとき、ボールが 4 m、選手が 3 m を超えて動いたとき、接続していない選手と入れ替わった選手は補間しない。`position()`: 描くフレームとその割合。ログでは今のフレームから次へ、ライブでは最新の 1 つ前から最新へ、最新が来てからの時間 / `simulator_step` で進む (1 サイクル遅れ)。
- `src/player_style`: `playerStyle()`。外の円の線 (選んだ選手、キック、タックル、不正な守備で太さ 2)、塗り (チームと状態の色)、体 (スタミナで 1/8 ずつ暗く)、減衰の輪 (effort、recovery)。`drawBody` はこれを使うようにした。
- `src/view3d/scene_builder`: ゴール (柱とクロスバー、半透明のネット、暗い床)、選手 (円柱の胴、球の頭、足元の円 (キック可能範囲か Player Size、不透明度 0.45)、強調の輪、減衰の輪、体の向きの黒い円錐、顔の向きの赤い円錐)、ボール (高さ `z` があれば空中に、真下に高さで薄くなる影)。値は web 版 (§4.3)。22 人をすべて描く (接続していない選手は 2D と同じく黒)。
- `Scene3D`: 光の当たるインスタンス描画 (円柱、球、円錐) と、半透明のインスタンス描画 (円、輪、ネット。深さを書かず、両面)。インスタンスのバッファは大きくなるときだけ作り直し、ほかは `glBufferSubData` で使い回す (§5.3)。
- `FieldView3D`: `LogPlayer` から再生の割合を受け取り、補間した show を描く。新しいフレームが来た時刻を覚え、ライブの割合にする。再生中と、ライブで次のフレームを待つあいだは、画面に出るたびに (`frameSwapped`) 次を描く。止まったら描かない。

**試験**
- テスト `player_style` (新): チームとキーパーの色、接続していない選手、選んだ選手、状態の色の重なりの順 (タックルがキックより後)、スタミナの暗さ、減衰の輪 (effort が先)。
- テスト `interpolate` (新): 角度の短い回り方向、中間の位置と向き、両端と範囲の外の割合、プレイモードの変化、瞬間移動、接続していない選手と入れ替わり、ログとライブの `position()`。
- テスト `scene_builder` (新): ゴールの柱、クロスバー、ネットの位置、選手の胴、頭、足元の円の大きさと色、Player Size の設定、向きの円錐の向き、強調と減衰の輪、ボールの高さと影の濃さ、選手とボールを描かない設定。
- テスト `scene_3d` に足した: 選手と空中のボールを真上から描き、頭、ボール、足元の円、ボールの影、芝の画素の色と、2 回描いて同じになること。
- どのテストも、直した箇所を一時的に壊すと失敗することを確かめた (向きの符号、足元の円の大きさ、ボールの高さ、角度の回り方向、プレイモードの変化、暗さの段階、状態の順、球と円を描かないこと)。
- 2D の描画が変わらないこと: 全試合のログの 7 つのフレームを、HEAD と今のビルドで画面外に描き、画素が一致することを確かめた。
- Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) でビルドとテストが通る (テスト 19 個)。`-DENABLE_3D=OFF` (14 個) と autotools も通る。
- 画面外で、全試合のログの 1,000 フレーム目を、中継の位置、ボールへの寄り、ゴールの近くから描いて見た。手元には Xvfb がないので、アプリの窓の中の 3D (再生中のなめらかさ、ライブ) は、ユーザが画面で確かめる。

#### 2026-11-12: 区切り 4-3b、描画の整理 (`feature/3d-view`)

4-2 と 4-3 の 5 コミット (81a8493〜c863113) をプッシュし、CI が通った (Ubuntu 22.04 のジョブの `scene_3d` も「飛ばした」ではなく「通った」で、本当に描いている)。

**決めたこと (ユーザ)**
- OpenGL の描画のコードがわかりにくいので、4-4 の前に、見た目を変えずに抽象化する。案は、薄い自前の層 (§5.3 の表)。Qt の `QOpenGLBuffer` などはインスタンスの属性や描き方の管理を助けないので一部の効果しかなく、Qt RHI は公開 API が Qt 6.6 以降で D2 と合わず、外部の描画エンジンは依存が大きいので、採らない。
- 実行効率: 1 フレームの仕事は描画命令 10〜20 回と数 KB の転送で、試作の計測では CPU 0.3 ms、GPU 4.4 ms (§14)。抽象化で増えるのは 1 フレームに数マイクロ秒の程度で、問題にならない。uniform の場所を覚えるので、名前で引く分はむしろ減る。

**作ったもの**
- `src/view3d/gl/` の部品と、`meshes_3d`、シェーダのファイル (§5.3)。`Instance3D` は `instance_3d.h` に移した。
- 頂点は位置、法線、テクスチャ座標の 1 つの形 (`Vertex3D`) にまとめ、地面もほかの形と同じく `InstancedMesh` で描く (インスタンスは 1 つ)。
- `Scene3D` は約 540 行から 177 行になった。`render()` は、段ごとに「描き方を当て、シェーダを選び、uniform を設定し、形を描く」を並べるだけになった。
- 静的ライブラリの中のリソースは、使う前に `Q_INIT_RESOURCE` で登録する (`ShaderProgram::load()` が初めに一度だけ行う)。
- OpenGL に詳しくない人も手で保守できるよう、関数と処理のまとまりごとにコメントを付けた (ユーザ)。GL の部品、シェーダ、`Scene3D`、`FieldView3D`、`CameraPose`、`Instance3D`、`Meshes3D`、`SceneBuilder` に、何をするか、OpenGL の用語 (バッファ、VAO、インスタンス描画、uniform、深さのバッファ、裏面の省略、混色、ミップマップ、行列)、新しく描くものを足す手順を書いた。コメントを除いたコードは前と同じ (コメントを除いて比べて確かめた)。

**試験**
- テスト `meshes_3d` (新): どの形も、三角形に面積があり、法線の向く側から見て反時計回りで、法線の長さが 1。円柱、球、円錐、輪、四角、地面の範囲、法線の向き、頂点の数、地面のテクスチャ座標。巻き方向をそろえないとき、球の極に面積のない三角形を作ったとき、地面のテクスチャ座標を間違えたときに失敗することを確かめた。シェーダの属性の名前を壊すと、読み込みの失敗が表示され、`scene_3d` のテストが失敗する。
- 見た目が変わらないこと: 全試合のログの 2 つのフレームを、3 つのカメラから画面外に描いた 6 枚が、直す前 (HEAD) と PNG のバイトまで同じ。
- Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) で、きれいなビルドとテストが通る (テスト 20 個)。`-DENABLE_3D=OFF` (14 個) と autotools も通る。

**見つけたこと**
- autotools の `make dist` で作る tarball には、CMake でだけビルドするファイル (`src/view3d/`、`test/` など) が入らない。19.0.1 のリリースの tarball は `make dist` で作られている (`configure` と `Makefile.in` を含み、`.github` を含まない)。Phase 9 の作業に足した。

#### 2026-11-12: 区切り 4-4、カメラ (`feature/3d-view`)

4-3b の 2 コミット (2859390、d279615) をプッシュし、CI が通った (シェーダをリソースから読む形でも、Qt 6.2 のジョブで描ける)。

**決めたこと**
- カメラの種類と名前 (`CameraMode`) は、GL に依存しないヘッダ `src/camera_mode.h` に置く。設定 (`Options`) は 3D なしのビルド (autotools、`ENABLE_3D=OFF`) にもあるため。4-4 では、free、broadcast、top、ゴール裏 2 つの 5 つ。寄りの 2 つと選手視点は Phase 6 で足す (E で回る順は web 版と同じ)。
- 位置の計算 (`Cameras3D`) と、どのカメラかと今の位置と追従 (`CameraController`) は、GL に依存しない `src/view3d/cameras_3d` に置き、単体テストで確かめる。値は web 版 (§4.3)。
- マウスの操作は three.js の OrbitControls と同じにした: 左ドラッグで回る (ビューの高さ分のドラッグで 1 周)、Shift / Ctrl / Meta + 左ドラッグで動かす (注視点の地面がマウスについてくる)、中ボタンのドラッグ (100 px で 5%) とホイール (1 ノッチで 5%) で寄る。距離 3〜400 m、地面の 0.03 rad 上まで。web 版と違う点: (1) 追従するカメラは、押しただけでは free にならず、動かしたときに free になる。(2) 真上に向けられる限りを 0.01 rad 手前にした (web 版は 1e-6。真上ではカメラの上の向きが視線と重なり、向きが不安定になるため)。
- 3D では、ホイールはカメラが受ける (計画どおり)。真上のカメラはマウスを受けないので、ホイールは 2D と同じくフレームを進め戻す。
- E は 2D でも効き、次に 3D を出すときのカメラを選ぶ (web 版と同じ)。ツールバーの選択欄は 3D のときだけ出す。

**作ったもの**
- `src/camera_mode.h`: カメラの一覧 (設定の名前、メニューの名前)、名前との変換、次のカメラ。
- `Options`: 設定 `camera_3d` (グループ `MainWindow`)、コマンドラインの `--camera-3d`。知らない名前は読まない。
- `src/view3d/cameras_3d`: `fitView()` (ピッチと周り 2 m の四隅を、横 97%、縦 85% に収める最短の距離)、`broadcast()`、`goal()`、`top()` (正射影、2D と同じくフィールドの -y が上)、`smoothToward()`、`orbit()`、`pan()`、`dolly()`。`CameraController`: 追従するカメラは、カメラを変えたときは跳び、ほかは時定数 0.35 s で追う (1 フレームに進める時間は 0.1 s まで、0.02 m 以内で止まる)。マウスで追従するカメラを動かすと、そこから free になる。free は、真上のカメラから移るときだけピッチ全体の位置から始める。
- `FieldView3D`: カメラ (`setCameraMode()`、マウスで free になったら `cameraModeChanged`)、マウスとホイールの操作、フレームのあいだの時間。カメラが追っているあいだは描き続ける。
- メインウィンドウ: View > 3D Camera (5 つのカメラと Next 3D Camera)、ツールバーの選択欄、キー E。3D が使えないときは、これらも無効にする。

**試験**
- テスト `cameras_3d` (新): 名前の変換と次のカメラ、9 つの視点の方向と 2 つの縦横比で四隅が収まり 1 つが端に接すること、横からの視点が web 版の閉じた式と同じこと、中継の距離と仰角とボールの追い方と端での止まり方、ゴール裏の位置と注視点、真上のカメラの範囲と向き、`smoothToward()` (時定数で 1 - 1/e、2 回に分けても同じ)、回転 (距離と注視点が変わらない、1 周で戻る、地面より下と真上に行かない)、移動 (カメラと注視点が同じだけ、ビューの高さ分のドラッグでビューの高さ分、向きが変わらない)、寄り (範囲で止まる)、`CameraController` (最初は跳ぶ、追う、止まる、長い間隔でも 0.1 s 分だけ、カメラを変えると跳ぶ、マウスで free になって位置が続く、真上のカメラはマウスを受けない、真上から free でピッチ全体)。
- テスト `view_switch` に足した: View > 3D Camera に設定のカメラが選ばれていること、E で次へ (メニュー、選択欄、ステータスバー)、最後の次は最初、3D が使えないとカメラも無効になること。
- テスト `scene_3d` に足した: 真上のカメラで描き、中央の線、両ゴール、フィールドの y = -20 の選手が上に描かれること。
- どのテストも、確かめる箇所を一時的に壊すと失敗することを確かめた (真上のカメラの上の向き、縦の 85%、中継の端での止まり方、地面より下に行かないこと、移動の向き、0.1 s の上限、真上のカメラがマウスを受けないこと、E でメニューが追うこと)。
- Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) で、きれいなビルドとテストが通る (テスト 21 個)。`-DENABLE_3D=OFF` (14 個) と autotools も通る。
- 画面外で、全試合のログの 3,000 フレーム目を、5 つのカメラ (free は回して寄せたもの) で描いて見た。

**見つけたこと**
- Qt5 では、`cameras_3d.h` で `M_PI` を使うのに `<cmath>` を読んでいなかった (Qt6 のヘッダは間接的に読む)。Qt5 のビルドで見つけて直した。
- 2D では、ホイールでフレームを進め戻す (`MainWindow::wheelEvent()`)。

#### 2026-11-12: 区切り 4-5、背番号のラベル、カード、HUD (`feature/3d-view`)

4-4 の 2 コミット (e05064b、fdc2a96) をプッシュし、CI が通った。ユーザが画面で確かめた。

**決めたこと**
- 背番号とカードは、web 版と同じく、カメラを向く四角 (ビルボード) で描く。全員の文字を 1 枚の画像 (`LabelAtlas`) に並べ、1 つのテクスチャ、1 回の描画命令で描く。文字は、文字が変わったときだけ描き直す。
- カードは web 版と同じく背番号の右に置く (2D では背番号の左)。
- HUD は、2D の painter (`ScoreBoardPainter`、`TeamGraphicPainter`) を、3D の上に QPainter で描く (§5.5)。方位の帯、ミニマップ、範囲の数値は、Phase 5 と 6 で足す。
- ユーザの指摘: `scene_3d.cpp` に、OpenGL の関数の呼び出しが戻っていた (フレームの始めの消去と状態の戻し、終わりのテクスチャの外し)。ユーザの決定: OpenGL は `gl/` の中に隠し、ふだんの実装は抽象化した層を使う (§5.3 に方針として書いた)。フレームの始めと終わりを `gl/frame` (`GLFrame`) に移し、方針が守られていることを確かめるテスト `gl_isolation` を足した。

**作ったもの**
- `src/view3d/label_atlas`: 22 人分と、カード用の白い枠を並べた画像 (1 枠 192×64 px、文字 40 px、白い文字に 6 px の黒い縁)。文字は中央に置き、枠より広ければ横に縮める。文字の幅 (m) を返す。
- `SceneBuilder::addLabels()` (web 版の `fitLabels()`、`labelPlace()`): 文字は背番号と選手の種類 (Show タブ。スタミナはバーで出すので、Phase 5)。不正な守備の色。真上のカメラでは、2D と同じく足元の円の右に、文字の高さを 2D の Player フォントと同じにする。ほかのカメラでは、頭の上 (2.4 m) に実寸で置き、画面上で Player フォントの 2 倍を超えるときは、頭の上に向かって縮める。カード (0.42 × 0.6 m) は背番号の右。遠いものから並べる (近い文字が上に重なる)。
- `gl/billboard_mesh` とシェーダ `billboard.vert`、`billboard.frag`: 四角をカメラの右と上の向きに広げて、カメラに向ける。`Scene3D` の 4 つ目の段として、深さを見ずに、すべての上に描く (`RenderState::overlay()`)。
- `FieldView3D`: 背番号を組み立て、文字が変わったときに画像を GPU に送る。3D の上に、QPainter でスコアボードと両チームのロゴを描く。右のロゴの位置は設定のキャンバスの大きさで決まるので、3D の大きさを入れる (2D と同じ大きさ)。
- `gl/frame` (`GLFrame::begin()`、`end()`): フレームの始めに、はさみ込みとステンシルを切り、色と深さの書き込みと深さの比べ方を戻してから画面を消す。QPainter (前のフレームの HUD) が変えて残すことがあり、消去もそれに従うため。終わりに、シェーダ、形、テクスチャを外し、QPainter の期待する描き方にする。`Scene3D::render()` は、これと `gl/` のほかの部品を呼ぶだけ。
- テスト `gl_isolation` (§5.3)。

**試験**
- テスト `label_atlas` (新): 枠が画像の中にあって重ならないこと、白い枠、文字が自分の枠の中に白と黒の縁で中央に描かれること、同じ文字では描き直さないこと、文字や書体が変わると描き直すこと、文字の幅、広い文字が枠に収まること。
- テスト `scene_builder` に足した: 遠い選手の背番号 (頭の上、1.3 m、白、自分の枠)、遠いものからの順、5 m 先の背番号が画面上で 24 px (フォント 12 px の 2 倍) に縮んで頭の上に寄ること、真上のカメラで足元の円の右に、文字の高さがフォントと同じこと、選手の種類で文字が広がること、背番号も種類も出さないと出ないこと、不正な守備の色、黄と赤のカードの色と位置と大きさ、カードを出さない設定、選手を出さない設定。
- テスト `scene_3d` に足した: 真上のカメラで描いた背番号が、選手の右に白と黒で、上下と左右が正しく描かれること (描いた背番号を格子に分けて白さを atlas の枠の画像と比べ、上下や左右に裏返したものより似ていること。下の「見つけたこと」)、反対の側には何もないこと、地面の下に置いても同じに見えること (すべての上)。QPainter が残しうる状態 (はさみ込み、色と深さの書き込みの止め、深さの比べ方) から描いても、前と同じ絵になること。
- テスト `gl_isolation` (新): `src/` の C++ のソース (`gl/` を除く) に OpenGL の関数の呼び出しと定数がないこと。見つけ方そのもの (呼び出しと定数を見つけ、コメント、文字列、`gl` を含むだけの名前は見逃し、行の番号を出す) も確かめる。`scene_3d.cpp` に呼び出しを戻すと失敗し、コメントの中の呼び出しでは失敗しないことを確かめた。`gl/frame` に移す前後で、画面外で描いた 5 枚の絵がバイトまで同じ。
- どのテストも、確かめる箇所を一時的に壊すと失敗することを確かめた (文字の上下と左右、すべての上に描くこと、画面上の上限、頭への寄せ、真上のカメラの大きさ、遠いものからの順、カードの位置と色、描き直さないこと、横に縮めること、画面を消す前の状態の戻し)。初めは「すべての上に描くこと」を壊しても通ったので、地面の下に置く確認を足した。
- Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) で、きれいなビルドとテストが通る (テスト 23 個)。`-DENABLE_3D=OFF` (15 個) と autotools も通る。
- 画面外で、全試合のログの 3,000 フレーム目を 5 つのカメラで描き、2D の painter で HUD を重ねて見た (背番号、カード、スコアボード、左チームのロゴ)。

**Phase 4 の完了条件**: 2D と 3D の切り替えでフレームと選択が保たれる (4-1、ユーザが確かめた)。`ball.z` のある show で、ボールが空中に描かれ、影が地面に出る (テスト `scene_builder`、`scene_3d`)。カメラごとの位置の単体テスト (テスト `cameras_3d`)。ユーザが 4-5 を画面で確かめた (問題なし)。Phase 4 の完了条件を満たした。

**見つけたこと (CI)**
- 4-5 の 3 コミット (2958c3f〜4ea0ff1) の CI で、`scene_3d` の背番号のテストが 3 つの CMake のジョブで失敗した (手元では通っていた)。原因: 背番号の幅は書体で決まり、CI の書体は手元と違う。「反対の側には何もない」を見る点が、CI ではセンターサークルの線にかかった。選手を線から離れた所 (x = 20、y = 20) に置き、点を選手を中心に背番号と反対の側に取るようにした。
- あわせて、手元で書体を 8 つに変えて試したところ、「7」の形 (横棒が上、下の線が左) で向きを見る確認が、2 つの書体で落ちた (字の形と縦の位置が書体で違う)。向きは、描いた背番号を 24 × 8 の格子に分けた白さを、atlas の枠の画像と相関で比べ、上下や左右に裏返したものより 0.1 以上似ていることで確かめるようにした。文字は、どの書体でも左右対称にならない「7,t0」(選手の種類つき) にした。8 つの書体のどれでも通り、上下と左右に裏返すと失敗することを確かめた。

### Phase 5

#### 2026-11-12: 区切り 5-1、線の描画、向きの線、視野 (`feature/3d-view`)

4-5 と CI の直しのコミット (2958c3f〜45614ca) をプッシュし、CI が通った。Phase 4 を終えた。

**決めたこと**
- Phase 5 を 4 つの区切りに分ける: 5-1 線の描画の部品、足元の向きの線、視野 (4-3 で Phase 5 に移したもの)。5-2 2D と同じ重ね描き (注視点、キャッチ・タックル・キックの範囲、指差し、オフサイドライン、ボールの予測軌道) と範囲の数値の HUD。5-3 スタミナのバーと S キー。5-4 審判操作の当たり判定。区切りごとにユーザに確かめる。
- 線、点、扇形のように毎フレーム形が変わるものは、頂点ごとに色を持つ頂点の列 (`ColorVertex3D`) にして、`gl/vertex_batch` が 1 回の描画命令で描く。何をどこに描くかは、GL に依存しない `OverlayBuilder` が決める (web 版の `overlays.ts`)。線の太さは 1 画素 (web 版と同じ。OpenGL 3.3 core は太い線を描けない)。
- 視野は web 版と同じく、半透明の扇形で塗る (2D は扇形の輪郭だけ)。

**作ったもの**
- `gl/vertex_batch` とシェーダ `colored.vert`、`colored.frag`: 線、点 (大きさはシェーダの `uPointSize`)、三角形。点線は、線に沿った距離を頂点に持たせ、シェーダが 0.3 m ごとに間を抜く (web 版と同じ)。属性 `A_DISTANCE` を足した。
- `src/view3d/overlay_builder`: `addLine()`、`addArc()` (弧に沿って距離が続く。点線の弧のため)、`addDirections()` (足元の円の中の向きの線。2D の `drawDir` と同じく、中心からキック可能範囲の端まで、体は選手の線の色、顔は首の色で、視野を描くときは描かない)、`addViewAreas()` (Show タブの View Area。選手ごとの半透明の扇形 (`visible_distance` まで、視野の幅、濃さ 0.25)、選んだ選手は大きな視野の線 (20、40、60 m の弧、60 m までの両端、`visible_distance` の円))。高さは、扇形 0.035、大きな視野 0.04、ほかの線 0.045 m。
- `SceneContent` に、扇形、線、点線、点の並びを足した。`Scene3D` は、半透明の段の中で、足元の円と輪の後、ネットの前に描く。
- 4-5 の不具合を直した: 青の 11 番の背番号の右端に細い縦の線が出ていた。atlas でその枠のすぐ右がカード用の白い枠で、テクスチャを読むときの補間で白がにじんでいた。白い枠の周りを 8 px 空け、カードは白の内側 (端から 16 px) だけを読む。

**試験**
- テスト `overlay_builder` (新): 線の頂点、高さ、色、距離、円と 4 分の 1 の弧 (円の上、つながり、距離が続く、向き)、向きの線 (体と顔の端と色、首がないとき、接続していない選手、視野を描くとき、選手を出さないとき)、扇形 (三角形の数、頂点と `visible_distance`、角度の範囲、高さ、色と濃さ、視野の幅がないとき)、大きな視野 (線の数、色、高さ、両端、20 と 40 m の弧、感じる円)。
- テスト `scene_3d` に足した: 真上から 10 m で、実線が端から端まで、点線がおよそ半分、2 つの点、半透明の三角形 (芝の色が変わり、覆わない) が描かれること。初めは三角形の確認が絵の外を見ていて、たまたま通っていた。画面の中に置き直し、見る点が絵の中にあることも確かめるようにした。
- テスト `label_atlas` に足した: どの枠の端も透明 (隣の枠ににじまない)、カードが読むところは白で、ほかの枠と重ならない。
- どのテストも、確かめる箇所を一時的に壊すと失敗することを確かめた (点線の間、点と扇形を描かないこと、点の大きさ、弧の距離、視野を描くときの顔の線、扇形の角度、選んだ選手の大きな視野、白い枠の周り)。
- Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) で、きれいなビルドとテストが通る (テスト 24 個)。`-DENABLE_3D=OFF` (15 個) と autotools も通る。
- 画面外で、全試合のログの 3,000 フレーム目を、視野を出し、左の 8 番を選んで描いて見た。

**ユーザの確認と、見つけた不具合**
- ユーザが画面で確かめた (向きの線、視野、大きな視野、背番号の横の線が消えたこと)。
- ユーザの指摘: 3D では、選手の自動選択 (ボールに最も近い選手) が効かない。原因: 2D は、自動選択をキャンバスを描くたびに `FieldCanvas::updateFocus()` の中で行っていて、3D を出しているあいだは 2D が描かれない。直したこと: 自動選択の計算を関数 `selectPlayerNearestBall()` (`src/player_selection`) に移し、2D と 3D の両方が描くときに呼ぶ。テスト `player_selection` (新): 両チームと片方のチームからの選び方、相手のチームの選手のほうが近いとき、接続していない選手、だれもいないとき、自動でないとき。範囲を両チームに広げると失敗することも確かめた (初めは、相手のほうが近い場合がなく、片方のチームの範囲を壊しても通った)。2D の絵は、自動選択をオンにして、4 つのフレームで HEAD と画素まで同じ。

#### 2026-11-12: 区切り 5-1b、操作を 2D にそろえる (`feature/3d-view`)

5-1 と自動選択の直しの 4 コミット (f4153d0〜2167e85) をプッシュし、CI が通った。

**決めたこと (ユーザ)**
- 2D は、ホイールでフレームを進め戻し、Z と X で拡大と縮小をする。3D でホイールがカメラを寄せると、操作が違って混乱する。一般の 3D ビューワの操作と違っても、2D にそろえる (§5.6)。web 版も後で同じにそろえるので、§0.2 に書いた。

**作ったもの**
- `CameraController::zoom()`、`resetZoom()`: 追うカメラ (中継、ゴール裏) は、追ったまま見ている点からの距離を倍率倍にする (なめらかに追う)。真上のカメラは見える高さを倍率倍にし、寄ったときはボールのまわりを、地面の絵からはみ出さない範囲で映す。free カメラは見ている点に向かって寄る。倍率は 0.04〜4 倍で、カメラを変えると 1 に戻る。`resetZoom()` は倍率を 1 にし、free カメラはピッチ全体の位置に戻す。
- `FieldView3D`: ホイールを受けなくした (メインウィンドウが、2D と同じくフレームを進め戻す)。`zoomCamera()`。
- メインウィンドウ: Z、X、Ctrl+Z、I は、3D を出しているときはカメラを寄せる、離す、倍率を戻す (2D と同じ 1.5 倍)。2D のときは今までどおり。

**試験**
- テスト `cameras_3d` に足した: 中継のカメラが、寄ってもボールを追い、距離が 1/1.5 になること、倍率の範囲、倍率を戻すこととカメラを変えたときに戻ること、真上のカメラの高さがボールのまわりで半分になること、隅では地面の絵の中に収まること、free カメラが寄ってピッチ全体に戻ること。
- テスト `view_switch` に足した: 2D を出しているとき、Z、X、I は今までどおり 2D の拡大と縮小と合わせを行うこと。
- どのテストも、確かめる箇所を一時的に壊すと失敗することを確かめた (追うカメラの距離、真上のカメラの高さ、地面の絵の中に収めること、カメラを変えたときに戻すこと、倍率の範囲、2D への振り分け)。初めは、真上のカメラのテストのボールが、倍率 0.5 で映せる範囲の外にあって失敗した (動きは正しく、テストの値を直した)。
- Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) で、きれいなビルドとテストが通る (テスト 25 個)。`-DENABLE_3D=OFF` (16 個) と autotools も通る。

#### 2026-11-12: 区切り 5-2、2D と同じ重ね描きと範囲の数値 (`feature/3d-view`)

5-1b の 2 コミット (3954aae、96af529) をプッシュし、CI が通った。ユーザが画面で確かめた。

**決めたこと**
- 重ね描きの計算 (注視点、キャッチ範囲、タックルの確率と文字、キックの加速範囲) は、計画どおり 2D の `PlayerPainter` から `OverlayMath` に移し、2D と 3D で共有する。幾何のクラス (`Vector2D`、`AngleDeg`、`Circle2D`) をそのまま使うので、2D の数値は変わらない。描く処理 (画面の座標、ペン、Qt の弧の角度の向き) は 2D に残す。
- ボールの予測軌道は共有しない。2D は画面の画素が変わらなくなったところで止める (画面の大きさで変わる)。3D は web 版と同じく、1 サイクルに 0.01 m 未満になったら止める (`OverlayMath::ballPath()`)。
- 範囲の数値の文字は、web 版と同じく HUD が Player フォントで書く。点を画面に写した位置の右下 (10, 2) に置き、ほかの文字と重なるときは 1 行ずつ下げる。指差しの印は web 版と同じく ±0.4 m の × (2D は ±2 px)。接続していない選手には描かない (web 版と同じ。2D は描く)。

**作ったもの**
- `OverlayMath`: `focusPoint()`、`catchArea()`、`tackleChances()`、`tackleText()`、`kickAccel()` (加速の上限、次の位置、2 つの円が交わるときの 2 つの弧か、交わらないときの円)、`ballPath()`。
- `PlayerPainter`: 注視点、キャッチ範囲、タックルの範囲、キックの加速範囲を、`OverlayMath` で計算するようにした。
- `OverlayBuilder::addOverlays()` (Show タブに従う): 選んだ選手の注視点 (線と円)、キーパーのキャッチ範囲 (円と、伸ばした範囲の点線の円 2 つ)、タックルの範囲 (体の向きに回した四角と確率の文字)、選んだ選手のキックの加速範囲 (次の位置への線、弧、加速の上限の文字)、指差し (線と ×)、オフサイドライン (チームの色でピッチの幅いっぱい)、ボールの予測軌道 (線と、サイクルごとの点)。`OverlayBuilder::placeTexts()`: 文字を画面に写し、重なりを避けて置く。カメラの後ろや画面の外の点の文字は書かない。
- `SceneContent::texts`。`FieldView3D` は、HUD (スコアボードとロゴ) の後に、範囲の数値を書く。

**見つけたこと**
- テストの仕組みの弱さ: `Options` は終了時に設定ファイルを書き、テストはどれも同じテスト用の HOME を使う。テストが設定を反転したまま途中で失敗すると、それが後のすべての実行に残っていた (Phase 1 からの弱さ。5-2 で壊して確かめたときに表に出た。`overlay_builder` のテストが、次の実行から失敗し続け、落ちた)。直したこと: どのテストも `main()` の前に、テスト用の HOME の設定ファイルを消して既定の設定から始める (`test/fresh_settings.cpp`)。CTest が渡す `RCSSMONITOR_TEST_HOME` が HOME と同じときだけ消すので、手で利用者の HOME のまま動かしても、利用者の設定ファイルは消さない (変数がないとき、別のとき、同じときの 3 通りで確かめた)。あわせて `overlay_builder` のテストは、関数ごとに重ね描きの設定を決まった値にしてから始め、並びを読む前に空でないことを確かめる。

**試験**
- 2D が変わらないこと: 重ね描きをすべて出し、自動選択にして、全試合のログの 200 フレーム (31 フレームごと。キックの加速範囲が出るもの 9、タックルの範囲が出るもの 86 を含む) を HEAD と今のビルドで描き、画素まで同じ。
- テスト `overlay_math` に足した: 注視点、キャッチ範囲 (伸ばしたときの最大と最小)、タックルとファウルの確率 (前、遠すぎ、後ろ、ファウルだけが成り立つ所)、文字の書式、キックできないとき、2 つの弧 (両端が 2 つの円の交点で、速さの円の弧が次の位置の側)、交わらないときの円、ボールの予測軌道 (減り方、止まり方、100 サイクルまで)。
- テスト `overlay_builder` に足した: どれも出さないと何も描かないこと、注視点 (選んだ選手だけ)、キャッチ範囲 (キーパーだけ、色)、タックルの範囲 (四角の角と文字、届かないとき)、キックの加速範囲 (選んだ選手だけ、線と文字の位置)、指差しとオフサイドライン、ボールの予測軌道、文字の置き方 (右下にずらすこと、重なると 1 行下げること、画面の外とカメラの真後ろの点を書かないこと)。
- どのテストも、確かめる箇所を一時的に壊すと失敗することを確かめた (弧の向き、軌道の止め方、伸ばした範囲、ファウルの判定、四角の回転、重なりの避け方、選んだ選手だけ、カメラの後ろ、オフサイドラインの色)。初めは、ファウルの判定と、カメラの後ろの点を壊しても通ったので、ファウルだけが成り立つ所とカメラの真後ろの点を足した。テストの丸めの期待値の誤り (Qt の書式は 0.8125 を 0.813 にする) も直した。
- Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) で、きれいなビルドとテストが通る (テスト 25 個)。`-DENABLE_3D=OFF` (16 個) と autotools も通る。
- 画面外で、全試合のログの 2,047 と 3,000 フレーム目を、重ね描きをすべて出して描いて見た。
- ユーザが画面で確かめた (Focus Point のほか)。Focus Point は、それが出る都合のよいログが見つからず、後で確かめる。

#### 2026-11-12: 区切り 5-3、スタミナのバー (`feature/3d-view`)

5-2 の 4 コミット (b4193af〜587456b) をプッシュし、CI が通った。

**決めたこと**
- S キーは、2D がすでに「なし → スタミナ → 容量 → 両方」と回していて (`ConfigDialog::toggleShowStamina()`)、その状態は 2D と 3D で共有する `Options` にある。3D はそれに従ってバーを描くので、キーには手を入れない。2D はスタミナを背番号の横に数字で書き、3D は web 版と同じくバーで出す。
- バーは、背番号やカードと同じビルボード (atlas の白い枠を色で塗る) で描く。新しい GL の部品は要らない。
- 容量のバーは、web 版 (0.1 m) より太い 0.15 m にする (ユーザ。画面で確かめたとき、細くて見にくかった)。web 版にも反映する (§0.2)。

**作ったもの**
- `SceneBuilder::staminaRate()`、`capacityRate()` (容量のない古いログでは -1)、`staminaHue()` (残りが `recover_dec_thr` と `effort_dec_thr` の大きいほうを下回ると赤、満タンで緑)。
- `SceneBuilder::addLabels()` にバーを足した (web 版の `updateBars()`): 背番号の文字の下に、カメラの下の向きへ、スタミナのバー (高さ 0.2 m) と容量のバー (0.15 m) を並べる。それぞれ、濃さ 0.6 の黒い背景 (縁 0.03 m) の上に、左端から残りの割合だけ塗る。スタミナの色は HSL (色相、0.85、0.5)、容量は #4aa3ff。幅 1.4 m、間 0.08 m。バーを出すときは、横から見てバーが頭の上に来るよう、背番号を 0.45 m 上げる (近くで縮めるのはそこから)。真上のカメラでは、2D のように背番号と同じ左端から並べる。スタミナのない show (古いログ) ではスタミナのバーを出さない。

**試験**
- テスト `scene_builder` に足した: 残りの割合 (上限 1)、容量のないとき、色相 (閾値以下で赤、満タンで緑、中間、2 つの閾値の大きいほう)、バー (出さないときは背番号だけ、両方のとき背番号が上がること、背景と塗りの大きさ・位置・色、容量のバーの位置、残り 0 では背景だけ、スタミナのない show ではバーなし)、真上のカメラでバーの左端が背番号の左端とそろうこと。
- テスト `view_switch` に足した: S で「スタミナ → 容量 → 両方 → なし」と回ること。
- どのテストも、確かめる箇所を一時的に壊すと失敗することを確かめた (背番号を上げること、塗りの向き、バーの間、真上のカメラの左端、スタミナのない show、残り 0、閾値の大きいほう)。初めは、閾値の大きいほうを片方に変えても通った (既定値は 2 つとも 0.3) ので、2 つが違う場合を足した。また、1 つのテスト関数が途中で失敗すると設定が反転したまま次の関数に響いたので、`scene_builder` のテストも関数ごとに設定を決まった値にしてから始める。
- Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) で、きれいなビルドとテストが通る (テスト 25 個)。`-DENABLE_3D=OFF` (16 個) と autotools も通る。
- 画面外で、全試合のログの 5,000 フレーム目を、スタミナと容量を出して描いて見た。

#### 2026-11-13: キーパーのキャッチ範囲の向きの制限 (`feature/3d-view`)

5-3 の 2 コミット (6a13654、b66d88d) をプッシュし、CI が通った。

**決めたこと (ユーザ)**
- rcssserver 17.0.0 から、サーバはキャッチの向きを `min_catch_angle`〜`max_catch_angle` (既定 -90〜+90) に切り詰める (`Player::goalieCatch()` の `NormalizeCatchAngle()`)。キーパーは後ろにはキャッチできないので、キャッチできる範囲は円ではない。2D と 3D の重ね描きのキャッチ範囲に反映する (soccerwindow2 の `PlayerPainter::drawCatchableArea()` を参考にした)。

**作ったもの**
- `OverlayMath::catchOutline()`: キャッチの箱 (長さ、幅 `catchable_area_w`) を体の向き + キャッチの向きに回したものを、向きの範囲で合わせた形の外形。遠い角が描く弧 (体の向き + min - 対角の角度 から、範囲 + 対角の角度の 2 倍)、そこから両端の向きの箱の横の辺でキーパーの後ろを閉じる。向きの幅が 180° なら、後ろに幅の半分の 1 本の線 (既定の ±90)。180° より狭ければ、両端の箱の近い角を通ってキーパーで会う。180° より広ければ、両端の箱の横の辺が後ろで交わる。弧が閉じれば円。向きの幅が対角の角度の 2 倍より狭いときは、遠い角が真ん中に隙間を残すので、弧を 2 つに分け、両端の箱の遠い辺でつなぐ (soccerwindow2 は、向きが対称で幅が 180° のときに正しい形を描く。これは一般の場合も扱う)。`CatchArea` に、3 つの半径に対応する箱の長さを足した。
- 2D (`PlayerPainter::drawCatchArea()`) と 3D (`OverlayBuilder::addOverlays()`) は、3 つの外形 (伸ばさない長さ、伸ばした最大と最小) をこれで描く。3D の点線は、弧から辺へと点線の間隔が続く。半径の選び方 (伸ばさない長さを実線、最大と最小を点線) は前のまま。soccerwindow2 は、確実にキャッチできる長さ (最小) と最大の 2 つを描く。
- パラメータの既定値: `max_catch_angle` と `min_catch_angle` は、サーバ 17.0.0 で足された。それより前のサーバのログにはこの 2 つがなく、キーパーは全方位にキャッチできた。既定値を、19.0.0 の ±90 (B7) から ±180 にした。17.0.0 以降のサーバのログとライブは、2 つを必ず含む (全試合のログ 2 本で確かめた)。

**試験**
- テスト `overlay_math` に足した: 制限のないとき (円)、±90 (弧の始まりと範囲、後ろの線の両端、弧の両端とつながる)、±45 (近い角とキーパーを通る)、±10 (弧が 2 つ、遠い辺が体の向きで交わる)、±135 (後ろで交わる点が体の向きの線の上と箱の辺の上)、±170 (円)。サーバと同じ判定 (向きを 0.05° ごとに試し、箱の中にあるか) と、外形の中かどうかが一致すること (8 通りの向きの範囲、体の向き、長さ。点を 0.1 m ごとに並べ、外形から 2 cm 以内を除く)。
- テスト `overlay_builder` に足した: ±90 で、外形が閉じ、幅の半分より後ろに出ず、後ろの線の両端が正しいこと。伸ばした型の 2 つの点線も同じで、点線の間隔が弧から辺へ続くこと。
- テスト `rcg_types`: 2 つの既定値を ±180 に直した。
- どのテストも、確かめる箇所を一時的に壊すと失敗することを確かめた (近い角の向き、弧を 2 つに分けること、後ろで交わる点、180° より狭いときの道、点線の間隔、3D で制限を無視すること、向きの範囲の計算)。
- 制限のないとき (古いログ) の 2D は、前と同じ計算 (同じ式の半径と、同じ整数の円) で描く。
- Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) で、ビルドとテストが通る (テスト 25 個)。`-DENABLE_3D=OFF` (16 個) と autotools も通る。
- 画面外で、全試合のログ (サーバ 17 以降、±90) の 3,000 フレーム目の両キーパーを、2D と 3D で描いて見た。パラメータを ±180 にしたときは前と同じ円になる。

#### 2026-11-13: 区切り 5-4、審判操作の当たり判定 (`feature/3d-view`)

キャッチ範囲の 2 コミット (d77a892、edad319) をプッシュし、CI が通った。ユーザが画面で確かめた (描く外形は前と同じ 3 つでよい)。

**決めたこと**
- 右クリックのメニューは、メインウィンドウが 1 つだけ持ち (`RefereeMenu`)、2D と 3D の表示は、右クリックで狙った先 (`RefereeTarget`: フィールドの点と、3D では選手) を知らせるだけにする。前は 2D の `FieldCanvas` がメニューを持ち、画面の座標を送って、メインウィンドウがフィールドの座標に直していた。メインウィンドウの審判操作の関数は、フィールドの座標を受け取るようにした。
- 3D の当たり判定は web 版 (`playerAt()`、`targetAt()`) と同じ: 画面の点からカメラの光線を出し、描いた選手の胴 (地面から BODY_HEIGHT までの円柱) と頭 (球) のうち最も近いものに当たれば、その選手とその足元。当たらなければ、地面との交点が地面の絵の範囲 (ピッチと周り 5 m) にあればその点。地平線より上や、範囲の外は「位置なし」。位置なしでは、Drop Ball と Free Kick を無効にし、理由をツールチップに出す (web 版と同じ文)。Change Playmode は、メニューバーの Referee と同じくボールの位置から行う。
- カードの選手を選ぶダイアログは、3D で選手を狙ったときは、その選手を選んだ状態で開く (2D と、メニューバーの Referee からは、前と同じく空)。web 版は 2D でも点に最も近い選手を選んでおくが、rcssmonitor の 2D は変えていない。
- 人型 (Phase 7) の当たり判定を 1.2 倍にするのは、Phase 7 で行う。

**作ったもの**
- `src/referee_target.h`、`src/referee_menu.{h,cpp}`: Drop Ball、Free Kick Left と Right、カード、Change Playmode の項目と信号。Change Playmode で選べるモードの一覧は `changePlayModeModes()` (`play_mode_position`) にまとめ、メニューバーの Referee と右クリックのメニューで共有する (前は同じ一覧が 2 か所にあった)。
- `FieldCanvas`: メニューと、点を送るスロットと信号をやめ、右ボタンを押した点を `refereeMenuRequested()` で知らせる。
- `src/view3d/picking.{h,cpp}`: `rayAt()` (カメラの行列の逆で、画面の点を近い面と遠い面の点に戻す。真上の正射影のカメラでも使える)、`groundAt()`、`playerAt()` (円柱の側面と上面、球)、`targetAt()`。`SceneContent::picks` に、描いた選手 (チーム、背番号、位置、胴の半径) を入れる。
- `FieldView3D`: 最後に描いたカメラを覚え、右クリックで `targetAt()` の結果を `refereeMenuRequested()` で知らせる。描く前は位置なし。
- `Options::setServerPort()` (テスト用)。

**試験**
- テスト `picking` (新): 光線が、画面の点に描かれた世界の点を通ること (透視と正射影)、真上のカメラの光線が平行で真下、地面との交点と地平線より上、選手の胴の 3 つの高さと頭に当たり、頭の上・胴の横・前の地面には当たらないこと、光線の上の 2 人のうち近いほう (並びの順によらない)、真上から胴の上面に当たること、狙った先 (選手の足元、余白の地面、範囲の外、地平線より上)。
- テスト `referee_menu` (新、`play_mode_actions` を置き換えた): 項目の並び、Change Playmode のモードがメニューバーと同じこと、点がないときに無効で理由のツールチップが出て、押しても信号を出さないこと、カードと Change Playmode が狙った先を渡すこと、2D の右クリックが押した点を知らせること (メニューのキーでは知らせない)、メインウィンドウが 2D の求めでメニューを開くこと。
- テスト `referee_commands` (新): 擬似サーバ (ローカルの UDP) に、メインウィンドウをつなぎ、右クリックのメニューの項目から送られるコマンドを確かめる。3D で選手を狙ったときの Drop Ball と Free Kick (`(dispfoul 168 -52 0)` など)、カードのダイアログがその選手を選んでいて、`(dispcard -1 7 yellow)` などが送られること、Change Playmode の点 (`(move (ball) 20 -10 0 0 0)` と `(change_mode play_on)`、kick_in でタッチラインへ)、位置なしでは何も送らないこと。サーバはコーチモードのパラメータを送る。
- テスト `scene_builder` に足した: 描いた選手だけが `picks` に入ること。
- どのテストも、確かめる箇所を一時的に壊すと失敗することを確かめた (13 通り: 上面、近いほう、範囲、画面の上下、頭、胴の高さ、2D の押した点、位置なしの守り、無効にすること、ダイアログの選手、Change Playmode の点、`picks`、モードの一覧)。位置なしの守りは、無効な項目を Qt が `trigger()` で動かさないので初めは確かめられず、信号を直に出して確かめるようにした。
- Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) で、ビルドとテストが通る (テスト 27 個)。`-DENABLE_3D=OFF` (17 個) と autotools も通る。
- 実際のサーバとの接続と、画面での右クリックは、ユーザが確かめる。

#### 2026-11-13: SceneContent を独立したヘッダに (`feature/3d-view`)

5-4 の 3 コミット (aa9868a〜a4cf9c1) をプッシュした。CI は、Ubuntu 22.04 (Qt 6.2) のジョブだけが、新しいテスト `referee_menu` と `referee_commands` のコンパイルで失敗した。原因: テストで、項目が開くサブメニューを `QAction::menu()` で得ていた。これは Qt 6.0 で非推奨になり (Qt 6.4 では非推奨でない)、CMake が付ける `QT_DISABLE_DEPRECATED_BEFORE=0x060000` で Qt 6.2 では使えない。サブメニューの項目はサブメニューが持つ (`QMenu::menuAction()`) ので、項目の親からサブメニューを得るようにした (Qt 5.15 と Qt 6 の両方で使える)。

**決めたこと (ユーザ)**
- 描くもののリスト `SceneContent` はデータだけにし、中身は builder (`SceneBuilder`、`OverlayBuilder`、後の区切りで足すもの) が決める。この分け方をファイルの上でもはっきりさせるため、`SceneContent` と、その要素の `OverlayText`、`PlayerPick` を `scene_builder.h` から `src/view3d/scene_content.h` に移す。データだけを使う `Scene3D`、`OverlayBuilder`、`Picking`、`FieldView3D` のヘッダは、`scene_builder.h` ではなくこれを読む。

**試験**
- 動きは変わらない。読み込みの連鎖に頼っていたテスト 2 つ (`picking`、`overlay_builder`) に、要るヘッダを足した。Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) で、ビルドとテストが通る (テスト 27 個)。

#### 2026-11-13: 区切り 5-5、フォーカス点の表示 (`feature/3d-view`)

CI の直しと SceneContent の分離の 3 コミット (fb41ec9〜b281ba2) をプッシュし、CI が通った (Ubuntu 22.04 のジョブも)。

**ユーザの確認**: rcssserver につなぎ、3D の右クリックから審判のコマンドが送られることと、Focus Point の動きを確かめた。Phase 5 の完了条件を満たした。ただし、Focus Point の表示 (選手から線を引き、フォーカス点のまわりに円を描くだけ) は、わかりにくかった。

**決めたこと (ユーザ)**
- 描画のコストを抑えたまま、フォーカス点の表示をわかりやすくする。Phase 6 の前に、区切り 5-5 として行う。採った案 (2D と 3D の両方):
  1. 誤差が同じになる線: rcssserver のフォーカス点は観測の誤差を決める (19.0.0 の Gaussian モデルでは、誤差の幅 = 選手からの距離 × `dist_noise_rate` + フォーカス点からの距離 × `focus_dist_noise_rate`。既定では 2 つの率が同じ)。誤差が同じになる点は、率が同じなら、選手とフォーカス点を焦点とする楕円になる。誤差が最も小さい所より、2 つの率の平均 × 10、20、40 m だけ大きい線を描く (率が同じなら、選手からその点を通ってフォーカス点へ行く道が、まっすぐより 10、20、40 m 長い線)。内側ほどよく見える。視野の外は観測されないので、視野の角度で切る。
  2. 印を照準の形にする: フォーカス点の大きさ (Focus Point Size) の円と、選手に向いた 1 本を含む十字の 4 本の目盛り。下に、中心ほど濃い半透明の円盤を 3 枚重ねる (スポットライト)。
  3. 3D では、選手からの線を目の高さ (頭の中心) から引く (視線として読める)。
  4. 全員のフォーカス点を、小さな照準 (0.6 倍) としてチームの色で出す。設定 (Show タブの「All Focus Points」、`show_all_focus_points`、`--show-all-focus-points`) で切り替え、Focus Point がオンのときだけ効く。

**作ったもの**
- `OverlayMath::focusNoiseLines()`: 誤差が最も小さい点 (フォーカス点の率が小さくなければフォーカス点、そうでなければ選手) から 96 本の光線を出し、重みつきの距離の和が線の値になる点を二分法で求めて、閉じた多角形にする (和は凸なので、各光線で 1 回だけ越える)。それを視野の両側の半平面で切り、視野の中の部分を折れ線にする。視野が切った線は 2 つに分かれることがある (選手が線の外にあるとき)。率は選手タイプの値。
- `OverlayMath::focusTicks()`、`OverlayMath::FocusLook` (照準、スポットライト、線の濃さ (内側から 0.8、0.55、0.3)、ほかの選手の照準の大きさ)。2D と 3D は同じ値で描く。
- 2D (`PlayerPainter::drawFocusPoint()`) と 3D (`OverlayBuilder::addOverlays()`)。3D のスポットライトは足元の円と同じ円盤のインスタンス描画で、描画命令は増えない。線は 1 フレームに数百の頂点。
- `Options::showAllFocusPoints()`、設定ダイアログのチェックボックス。

**試験**
- テスト `overlay_math` に足した: 目盛りの位置、線がないとき (フォーカス点なし、視野なし、率が 0)、率が同じときの楕円 (どの点も距離の和が値に合う、視野 180° で前だけ、両端が視野の端、前の端の位置)、狭い視野で両側が切られること、選手が外にあるときに線が閉じたまま (視野 90°) と 2 つに分かれること (視野 20°)、率が違うときの重みつきの和。
- テスト `overlay_builder`: 選んだ選手のスポットライト (位置、大きさ、濃さ)、目の高さからの線、線が楕円の上で視野の中にあり、外ほど薄いこと、照準の円と目盛り。全員の設定で、ほかの選手の照準がチームの色で、スポットライトも線もないこと。
- テスト `config_dialog`: チェックボックスが設定を変え、Focus Point がオフでは押せないこと (Focus Point のキーで切り替えたときも)。
- どのテストも、確かめる箇所を一時的に壊すと失敗することを確かめた (13 通り: 光線の中心、線の値、視野で切ること、視野の片側の向き、閉じた線のつなぎ、視野の幅の半分、目盛りの向き、目の高さ、ほかの選手の色、全員の設定、線の濃さの順、チェックボックスの有効・無効の 2 か所)。
- 手元の全試合のログ 2 本にはフォーカス点を動かした選手がいないので、ログの 1 フレームで選手にフォーカス点を与えて、画面外で 2D と 3D を描いて見た。
- Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) で、ビルドとテストが通る (テスト 27 個)。`-DENABLE_3D=OFF` (17 個) と autotools も通る。

### Phase 6

#### 2026-11-13: 区切り 6-1、寄りのカメラ (`feature/3d-view`)

5-5 の 2 コミット (220e3b7、72594bc) をプッシュし、CI が通った。Phase 5 を終えた。

**決めたこと**
- Phase 6 を 6 つの区切りに分ける (ユーザが了承): 6-1 寄りのカメラ 2 つ、6-2 ピッチ全体の視点 9 個、6-3 選手視点と知覚、6-4 両端のチームの色、6-5 方位の帯とミニマップ、6-6 設定ダイアログの 3D タブと Player Camera タブ。設定とコマンドラインのオプションは各区切りで足し、ダイアログのタブは 6-6 でまとめる。
- 寄りのカメラ (ボール、選んだ選手) は web 版と同じ: 中継風と同じくタッチライン側から、見る点 (ボールの地面上、選手は高さ 1 m) から距離 12 m、仰角 20°。見る点は地面の絵の範囲の中。選手が選ばれていないか、接続していなければ、ボールのもの。追う時定数は距離に比例 (0.0089 s/m、0.05〜0.35 s)。E の順は web 版と同じく、中継風の次。
- 距離は、2D にそろえて (§5.6)、ホイールではなく Z と X (と Ctrl+Z) で 1.5 倍ずつ変える (3〜60 m)。寄りのカメラの距離は設定なので、カメラを変えても保ち、設定ファイルに残す (web 版のホイールと同じ)。I は既定の 12 m に戻す (6-2 で I をピッチ全体の視点に置き換えるまで)。
- 3D の設定は、設定ファイルの新しいグループ `View3D` に書く。範囲の外の値は、設定ファイルからもコマンドラインからも読まない。

**作ったもの**
- `CameraMode::CloseupBall`、`CloseupPlayer` (`closeup-ball`、`closeup-player`)、`isCloseup()`。メニュー、ツールバーの選択欄、E は、カメラの一覧から作るので、そのまま 2 つが加わる。
- `Cameras3D::closeup()`、`closeupTimeConstant()`。`CameraController`: 追う先 (`CameraSubject`: ボールと選んだ選手)、`setCloseup()` (距離と仰角が変わればなめらかに移る)、寄りのカメラでの `zoom()` と `resetZoom()`。
- `Options`: `closeup_distance_3d`、`closeup_elevation_3d` (範囲に収める setter)。コマンドラインの `--closeup-distance-3d`、`--closeup-elevation-3d`。
- `FieldView3D`: 描くたびに、補間した show から選んだ選手を探してカメラに渡し、設定の距離と仰角を渡す。Z と X で寄りのカメラの距離が変わったら、設定に書き戻す。

**試験**
- テスト `cameras_3d` に足した: E の順 (中継風の次に寄りの 2 つ)、寄りのカメラの位置 (距離、仰角、タッチライン側、地面の絵の中に収める) と時定数、コントローラ (最初は跳び、ボールを寄りの時定数で追う、選手のものはその選手の高さ 1 m、選手がいなければボールへ移る、Z と X で距離が変わり範囲に収まり倍率は変わらない、カメラを変えても距離を保ち、I で既定に戻る、新しい距離と仰角へなめらかに移る)、設定の範囲。
- どのテストも、確かめる箇所を一時的に壊すと失敗することを確かめた (8 通り: タッチライン側、寄りの時定数、選手がいないとき、Z と X で距離を変えること、I で戻すこと、地面の絵の中に収めること、距離と仰角の変化で動き出すこと、設定の範囲)。
- Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) で、ビルドとテストが通る (テスト 27 個)。`-DENABLE_3D=OFF` (17 個) と autotools も通る。
- 画面外で、全試合のログの 3,000 フレーム目を、寄りのカメラ (ボール 12 m と 5 m、選手 12 m) で描いて見た。

#### 2026-11-13: 区切り 6-2、ピッチ全体の視点 (`feature/3d-view`)

6-1 の 2 コミット (b70c4c1、2fbdde9) をプッシュし、CI が通った。ユーザが画面で確かめた。web 版にフォーカス点の表示を入れることを §0.2 に足した (ユーザ)。

**決めたこと**
- 視点は web 版と同じ 9 個 (方位と仰角は §4.3。順は Ctrl+I の順)。選ぶと free カメラをそこに置き、そこからマウスで動かせる。ボールは追わない。free カメラの最初の位置は 1 つ目 (ピッチ脇)。
- 3D の I は、5-1b から続いた「倍率を戻す」をやめ、ピッチ脇の視点にする (§5.6)。Ctrl+I は、最後に選んだ視点の次 (初めは 1 つ目)。2D では、I は今までどおりフィールドを画面に合わせ、Ctrl+I は「3D 用」とステータスバーで知らせるだけ (web 版と同じ)。メニューとツールバーの視点も、2D では同じく知らせるだけ。
- 視点の位置は、選んだ次の描画のときの縦横比で決める (`CameraController::viewFrom()`)。
- 1 つのキーを 2 つのアクションのショートカットにはできない (Qt では、どちらも動かない) ので、I は今の「Fit Field Size」のアクションのまま、3D ではそれが視点を選ぶ。メニューの 1 つ目の視点には、文字だけ「I」を添える。

**作ったもの**
- `Viewpoint`、`viewpoints()`、`viewpointInfo()`、`nextViewpoint()` (`src/camera_mode.h`。3D なしのビルドでもメニューを作るため、GL に依存しない所に置く)。
- `Cameras3D::viewpoint()` (`fitView()` を視点の方位と仰角で)、`CameraController::viewFrom()` (free にし、倍率を 1 に戻し、次の `update()` で視点に置く)、`FieldView3D::viewFrom()`。
- メインウィンドウ: View > 3D Camera の下に、視点 9 個と「Next Viewpoint」(Ctrl+I)。ツールバーに「Whole Pitch」のボタン (押すと視点の一覧。3D のときだけ出す)。3D が使えないときは、これらも無効にする。視点を選ぶと、カメラの設定とメニューと選択欄が free になり、ステータスバーに視点の名前を出す。

**試験**
- テスト `cameras_3d` に足した: 視点の一覧 (9 個、1 つ目はピッチ脇で中継風と同じ仰角、名前が重ならない、Ctrl+I の順で回る、視点のカメラが方位と仰角の `fitView()`)、四隅が収まる確認をすべての視点と 3 つの縦横比 (16:9、4:3、21:9) に広げた、コントローラ (free になり倍率が戻る、次の描画の縦横比で視点に置かれる、ボールを追わずに止まる、マウスで動かせる、真上のカメラからも移れる)。
- テスト `view_switch` に足した: メニューに視点 9 個 (1 つ目に I の文字) と Ctrl+I があること、ツールバーのボタンが同じ視点を持ち、2D では出ないこと、2D の Ctrl+I とボタンの視点は知らせるだけでカメラが変わらないこと、3D が使えないと無効になること。
- どのテストも、確かめる箇所を一時的に壊すと失敗することを確かめた (8 通り: free にすること、視点に置くこと、倍率を戻すこと、視点の仰角、Ctrl+I の順、2D でカメラを変えないこと、ボタンを無効にすること、メニューの Ctrl+I)。
- Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) で、ビルドとテストが通る (テスト 27 個)。`-DENABLE_3D=OFF` (17 個) と autotools も通る。
- 3D での I、Ctrl+I、ツールバーのボタンは、画面のないテストの環境では動かせないので、ユーザが画面で確かめる。

#### 2026-11-13: 区切り 6-3、選手視点と知覚 (`feature/3d-view`)

6-2 の 2 コミット (dc172be、c7b493c) をプッシュし、ユーザが CI を確かめた (通った)。ユーザの指示で 6-3 から再開した。

**決めたこと**
- 選手視点は web 版と同じ (`player-camera.ts`、§4.3): 位置 (目の高さ 1.65 m / 肩越し: 6 m 後ろ、3 m 上から 15 m 先の高さ 1 m を見る)、向き (頭 = 体 + 首 / 体 / その中間)、動き (即時 / 滑らか: 時定数で近づき、角速度の上限を超えない)、画角 (視野の幅 (40〜120°。わからなければ 90°) / それになめらかに近づく / 一定)。既定は肩越し、頭、滑らか、一定の 120°、サーバの規則、時定数 0.5 s、最大 90°/s (ユーザが web 版で決めた値。§11 の 13)。別の選手になったとき、1 サイクルに動ける距離 (3 m) より大きく跳んだとき、スタイルを変えたときは、なめらかにせずにすぐその向きにする。
- 選手が選ばれていないときは、中継風のカメラ (web 版と同じ)。選手視点はマウスを受けない (web 版と同じ)。E で回る順の最後。
- 知覚は web 版と同じ (`sight.ts`、§4.3): すべて / 視野の中だけ / サーバの規則 (視野の中は観測の距離まで。背番号は `unum_far_length`〜`unum_too_far_length` で薄れ、背番号がなければ向きもない。チームは `team_far_length`〜`team_too_far_length` で灰色 (#9a9a9a) に近づく。視野の外は `visible_distance` の中だけ、何かがあることだけ)。決めるのはサイクルのフレームで、補間したフレームではない。サーバと違って本当の距離で決め、毎サイクル決める。
- 知覚の当て方も web 版と同じ: 見えない選手とボールは描かない。チームがわからない選手は灰色に寄せ、強調の輪とカードを出さない。向きがわからない選手は向きの円錐と、足元の向きの線、視野、Show タブの重ね描きを出さない。背番号はわかる確からしさを不透明度にする。スタミナのバーは自分のものだけ (サーバはほかの選手のスタミナを伝えない)。目の高さでは、選手自身を描かない (向きの線などは残る)。ボールが見えなければ予測軌道も出さない。
- 知覚の計算の名前空間は `PlayerSight` にした (選手視点のスタイルの `PlayerCameraStyle::Sight` と同じ名前にすると、両方を使う所で名前がぶつかるため)。
- Z と X は、選手視点では何もしない (選手がいないときの中継風のカメラは寄る)。

**作ったもの**
- `src/player_camera_options.h`: スタイルの種類 (`PlayerCameraStyle::Position`、`Direction`、`Motion`、`Fov`、`Sight`)、設定の名前、数値の範囲、`PlayerCameraOptions` (既定値)。設定 (`Options`) が使うので、GL に依存しない所に置く。
- `Options`: 選手視点の 8 つの設定 (グループ `View3D`、範囲の外と知らない名前は読まない) とコマンドラインのオプション。`setPlayerCamera()` は数値を範囲に収める。
- `CameraMode::Player` (`player`、「Selected Player」)。
- `src/view3d/player_camera_3d`: `referenceDirection()`、`viewFov()`、`turnToward()`、`easeToward()`、`pose()`、`PlayerCameraState` (フレームからフレームへ向きと画角を持つ)。
- `CameraController`: 選手視点 (`setPlayerCamera()`、`CameraSubject::player`)。マウスを受けない (`takeMouse()` が `takesMouse()` を見るようにした。前は真上のカメラだけを見ていて、テストで選手視点が free になることを見つけた)。
- `src/view3d/sight`: `PlayerSight::omitProbability()`、`perceivePlayer()`、`perceiveBall()`、`Perceptions` (ショーの選手ごとの知覚、ボール、描かない選手)、`fadeTeam()`。
- `SceneBuilder::addShow()`、`addLabels()`、`OverlayBuilder::addDirections()`、`addViewAreas()`、`addOverlays()` に、知覚 (`const Perceptions *`、なければすべて) を渡すようにした。
- `FieldView3D`: 選手視点で選手が選ばれているとき、サイクルのフレームで知覚を決めて組み立てに渡し、選手視点のスタイルをカメラに渡す。

**試験**
- テスト `player_camera` (新): 向き (頭、体、中間、首のないとき)、視野の幅の画角 (40〜120、わからなければ 90)、`turnToward()` (時定数なしならすぐ、短い回り方向で 180° を越える、時定数で 1 - 1/e、首を 180° 回しても 1 フレームに上限を超えない)、`easeToward()`、位置 (目の高さ、肩越し、16:9 での縦の画角)、状態 (最初はすぐ、滑らかは上限を超えずに向いて止まる、即時、画角のなめらかな変化と視野の幅、別の選手と跳んだときはすぐ、跳ばない一歩はなめらか、`reset()` の後はすぐ)、コントローラ (マウスを受けない、スタイルどおりの位置と画角、なめらかに回っている途中でスタイルを変えるとすぐその向き、選手がいなければ中継風)、設定 (範囲に収める、名前の変換、既定値)。
- テスト `sight` (新): 省かれる確からしさ、すべてと視野の中 (顔の向きで決まる、視野の端、狭い視野)、サーバの規則 (背番号が薄れる、チームが薄れる、チームが先に薄れるときは背番号も同じだけ、観測の距離の外、後ろの感じる距離、ボール、視野の幅がないときはサーバの `visible_angle`)、ショーの知覚 (自分はどちらを向いていても全部、目の高さでは描かないが重ね描きのための知覚は残る、すべてのときは描かない選手だけ)、灰色への寄せ方。
- テスト `scene_builder`、`overlay_builder` に足した: 知覚のあるときの描かない選手とボール、灰色、強調の輪、向きの円錐、当たり判定の選手、背番号の不透明度、カード、自分のバーだけ、向きの線・視野・重ね描き・ボールの予測軌道、目の高さでの自分の線。
- テスト `cameras_3d`、`view_switch`: E の順の最後に選手視点。
- どのテストも、確かめる箇所を一時的に壊すと失敗することを確かめた (22 通り: 背番号とチームの掛け合わせ、向き、顔の向き、感じる距離、自分の知覚、灰色、強調の輪、向きの円錐、ボール、背番号の不透明度、カード、自分のバー、重ね描きの向き、予測軌道、角速度の上限、短い回り方向、跳んだとき、別の選手、画角のなめらかな変化、スタイルの変化でのやり直し、マウス、画角の範囲)。自分の知覚は、選手が +x を向いているときは壊しても変わらなかったので、反対を向いたときを足した。
- Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) で、ビルドとテストが通る (テスト 29 個)。`-DENABLE_3D=OFF` (17 個) と autotools も通る。
- 画面外で、全試合のログの 3,000 フレーム目を、左の 9 番の選手視点 (肩越しのすべてとサーバの規則、目の高さのサーバの規則と視野の中) で描いて見た。設定ダイアログのタブは 6-6 で作る (それまでは設定ファイルとコマンドラインで変える)。

#### 2026-11-13: 区切り 6-4、両端のチームの色 (`feature/3d-view`)

6-3 の 3 コミット (389213e〜bd55734) をプッシュし、CI が通った。

**決めたこと**
- web 版と同じ (`stadium.ts`、§4.3): ゴールのネットを、そのゴールを守るチームの色にする (rcssserver はエンドを替えないので、左のチームが試合のあいだ左のゴールを守る)。ピッチのまわりの広告板 (タッチラインとゴールラインから 5.5 m、高さ 1 m。タッチライン沿いはハーフウェーラインで色が変わる。10 m ごとにチーム名) と、ゴールの 22 m 後ろの高さ 12〜20 m に浮かぶ看板 (幅 50 m。ロゴとチーム名) を、チームの色の 0.32 倍の暗い色に、縁と名前とロゴをチームの色で描く。看板のロゴは Team Graphic がオンのときだけ (2D と同じく、整数倍に拡大)。光は当てない。
- 板の裏は、web 版と同じく暗い色で塗りつぶす (画像を裏から見せない)。1 枚の板を両面描き、シェーダが表 (`gl_FrontFacing`) なら画像、裏ならインスタンスの色にする。
- 広告板の画像は、板の幅に合わせて 10 m ごとに繰り返す (テクスチャの繰り返し。シェーダが模型の行列の x の長さから回数を求める)。
- 広告板と看板は、設定 (`boards_3d`、`banners_3d`、既定はオン) で消せる。ダイアログは 6-6。
- 地面の絵は余白 5 m までなので、広告板はその外側の何もない所に立つ。外の地面は Phase 8 (空と霧) で足す。

**作ったもの**
- `SceneBuilder::addGoals()`: ネットの色 (左右のチームの色) を受け取る。
- `src/view3d/stadium_3d`: `TeamLook` (チームの色、名前、ロゴと、描き直しの判断に使う `key()`)、`shade()`、`boardImage()`、`bannerImage()` (QPainter で描く)、`addPanels()` (板の位置と向き)。GL に依存しない。
- `SceneContent::boards`、`banners` (端ごと)。`Texture2D::upload()` に繰り返しの指定。`RenderState::opaqueTwoSided()`。シェーダ `panel.vert`、`panel.frag`。`Scene3D::setStadium()` と、光の当たる固体の後に板を描く段。
- `TeamGraphicPainter::pixmap()`: HUD が作ったチームのロゴを看板に使う。
- `FieldView3D`: チームの色、名前、ロゴが変わったときだけ板の画像を描き直して GPU に送り、ネットと板を組み立てる。
- `Options`: `boards_3d`、`banners_3d` (グループ `View3D`) とコマンドラインの `--boards-3d`、`--banners-3d`。

**試験**
- テスト `stadium_3d` (新): `shade()`、`key()` (色、名前、ロゴとその画素で変わる)、広告板の画像 (大きさ、暗い色、上下の縁、真ん中の名前、名前がなければ文字なし)、看板の画像 (大きさ、枠、整数倍のロゴ、その右の名前、ロゴがなければ名前が真ん中)、板の配置 (端ごとに広告板 3 枚と看板 1 枚、どれもピッチの中心を向く、地面に立つ、裏の色、タッチライン沿いはハーフウェーラインから角まで、ゴールラインの後ろの幅、看板の位置と高さ)、それぞれを消す設定、設定の既定。
- テスト `scene_builder` に足した: ネットがゴールを守るチームの色 (左のゴールは左のチーム)、床は暗いまま。
- テスト `scene_3d` に足した: 画面外の描画で、板の表に画像が上下を正しく、10 m ごとに繰り返して描かれ、裏からは裏の色が見えること。
- どのテストも、確かめる箇所を一時的に壊すと失敗することを確かめた (11 通り: 画像の上下、繰り返し、繰り返しの幅、裏の色、両面、ネットの左右、ゴールラインの後ろの板の向き、タッチライン沿いの板の向き、名前、ロゴの倍率、ロゴの画素の変化)。
- Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) で、ビルドとテストが通る (テスト 30 個)。`-DENABLE_3D=OFF` (17 個) と autotools も通る。
- 画面外で、全試合のログの 3,000 フレーム目を、中継風、右のゴール裏、ピッチ全体の視点 (遠い左の角) から描いて見た。

#### 2026-11-14: 区切り 6-5、方位の帯とミニマップ (`feature/3d-view`)

6-4 の 2 コミット (06b5555、2a7b07f) をプッシュし、CI が通った。

**決めたこと**
- web 版と同じ (`hud/guides.ts`、§5.5): 方位の帯は画面の上の中央 (幅は画面の 60%、480 px まで、高さ 22 px)。カメラの向きを中央に、時計回りを右に 360° を並べ、15° ごとの目盛り (45° と 90° で長く)、見えている横の画角を明るく、両ゴールの向き (チームの色の丸に L、R)、ボールの向きを出す。ミニマップは右下 (幅は画面の 24%、140〜300 px) に、2D の field painter で描いたピッチと余白、選手 (チームとキーパーの色、選んだ選手に白い輪)、ボール、カメラの位置と見えている扇形を出す。ミニマップは選手視点の知覚によらずすべてを出す (試合の地図)。方位の帯のボールは、選手視点で見えていなければ出さない。
- どちらも設定で「オフ / 選手視点だけ / すべてのカメラ」(`compass_3d`、`minimap_3d`、既定はオフ)。真上のカメラでは出さない (地図そのもの)。ダイアログは 6-6。
- ミニマップのピッチは、地面の絵と同じ描き方を、ミニマップの大きさと画面の倍率で描き直す (`GroundTexture::draw()` に倍率とペンの太さを渡せるようにした)。地面の鍵と大きさが変わったときだけ描き直す。

**作ったもの**
- `GuideMode`、`guideModes()`、`guideModeName()`、`guideModeFromName()` (`src/camera_mode.h`。設定が使うので GL に依存しない所に置く)。
- `Options`: `compass_3d`、`minimap_3d` (グループ `View3D`、知らない名前は読まない) とコマンドラインの `--compass-3d`、`--minimap-3d`。
- `src/view3d/guides_3d`: `shown()`、`guideOf()` (カメラの位置、向き、横の画角)、`bearing()`、`compassOffset()`、`compassRect()`、`minimapRect()`、`minimapPoint()`、`drawCompass()`、`drawMinimap()` (QPainter で描く。GL に依存しない)。
- `FieldView3D`: HUD のスコアボード、ロゴ、範囲の数値の後に、設定とカメラに従って方位の帯とミニマップを描く。

**試験**
- テスト `guides_3d` (新): 出すカメラ (オフ、選手視点だけ、すべて、真上は出さない)、設定の名前と既定、カメラの向きと横の画角、方位の計算 (短い回り方向)、帯とミニマップの位置と大きさ、帯の印 (前のゴールが中央、後ろのゴールが端、時計回り 90° のボールが右の 4 分の 1、色、ボールがなければ印なし)、ミニマップ (選手とキーパーの色、ボール、カメラ、選んだ選手の輪、見えている扇形が明るく外は明るくない、ミニマップの外に描かない)。
- どのテストも、確かめる箇所を一時的に壊すと失敗することを確かめた (9 通り: 真上のカメラ、選手視点だけ、横の画角、短い回り方向、扇形の向き、キーパーの色、選んだ選手の輪、帯のボール、ミニマップの外を切ること)。初めは、選んだ選手の輪の確認が後ろの扇形の明るさに頼っていて、ミニマップの外の確認は扇形が届かない所を見ていたので、選ばないときと比べる確認と、カメラが向く上を見る確認に直した。
- Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) で、ビルドとテストが通る (テスト 31 個)。`-DENABLE_3D=OFF` (17 個) と autotools も通る。
- 画面外で、全試合のログの 3,000 フレーム目の左の 9 番の選手視点に、方位の帯とミニマップを重ねて描いて見た。

#### 2026-11-14: 区切り 6-6、設定ダイアログの 3D タブと Player Camera タブ (`feature/3d-view`)

6-5 の 2 コミット (11ff531、1fc390f) をプッシュし、CI が通った。

**決めたこと**
- web 版と同じ (`config-dialog.ts`、§5.7): 3D タブは Close-up Camera (距離、仰角)、Guides (方位の帯とミニマップ。それぞれオフ / 選手視点だけ / すべてのカメラ)、Stadium (Boards、Banners)、Default ボタン。Player Camera タブは、4 つのスタイル (位置、向き、動き、画角)、時定数 τ、最大角速度 ω、一定の画角、Player Sight、Default ボタン。Players、Ball と Stadium の残り (Stands、Grass Stripes、Sky) は、それを作る Phase 7 と 8 で足す。
- スタイルは web 版と同じくラジオボタンで並べ、行の下に選んだものの要約を出す (要約の欄は最も長い要約の高さにして、選び直してもダイアログの大きさが変わらないようにする)。どの項目にもツールチップで詳しい説明を付け、Default ボタンのツールチップに既定値を書く。スタイルの名前は web 版の短い名前にそろえた (`player_camera_options.h`)。
- 数値の欄は `QDoubleSpinBox` にした。Enter を押すか欄を離れたときに効く (キーを打つたびには効かない)。範囲の外は端に寄せる (下限より小さい値は Enter で下限に。上限を超える桁は Qt が受け付けない)。打っているあいだ、範囲の外ならエラーの色にする。選んだスタイルが使わない数値の欄 (τ は Smooth か Eased のとき、ω は Smooth のとき、一定の画角は Fixed のときだけ使う) は、ラベルと一緒に薄くする。
- 2 つのタブは 3D の入ったビルドにだけ作る (3D なしのビルドでは設定は残るが、使われない)。
- 3D で Z と X が寄りのカメラの距離を変えたら、開いているダイアログの欄も直す。欄に値を入れ直すときは、丸めた値を設定に書き戻さない。

**作ったもの**
- `ConfigDialog::create3DTab()`、`createPlayerCameraTab()`、`addChoices()` (ラジオボタンの行と要約)、`addNumber()` (数値の欄とラベル、範囲のツールチップ、エラーの色)、`update3D()` (設定から欄へ。`updateAll()` と、3D の Z と X の後に呼ぶ)。
- `Options`: 3D タブの既定値 (`BOARDS_3D`、`BANNERS_3D`、`COMPASS_3D`、`MINIMAP_3D`) と `setView3DDefaults()`。

**試験**
- テスト `config_dialog` に足した: タブの並び (3D なしのビルドでは 4 つ)、どの欄とボタンにもツールチップがあること、3D タブの各項目が設定を変えて `configured()` を出すこと、Default で設定と欄が戻ること、数値の欄 (下限より小さい値を打つとエラーの色で、Enter で下限になり、色が戻る。範囲の中の値は Enter まで効かない。上限を超えない)、`update3D()` が外で変わった設定を出し、丸めた値を書き戻さないこと、Player Camera タブの各スタイルと数値が設定の 1 つだけを変えること、要約が選んだものになること、Default、使わない数値の欄が薄くなる 5 つの組み合わせ。
- どのテストも、確かめる箇所を一時的に壊すと失敗することを確かめた (12 通り: 端に寄せること、Enter で効くこと、エラーの色、欄を入れ直すときの信号の止め、τ を薄くする条件、ラベルを薄くすること、要約、Default の後の欄、ミニマップの選択、`updateAll()` から呼ぶこと、τ の書き先、Player Sight の選択)。初めは「Enter で効くこと」を壊しても通ったので、範囲の中の値を打って Enter の前に効いていないことを足した。
- Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) で、ビルドとテストが通る (テスト 31 個)。`-DENABLE_3D=OFF` (17 個) と autotools も通る。
- 画面外でダイアログの 6 つのタブを描いて見た。

**ユーザの確認と、見つけた不具合**
- ユーザが画面で 2 つのタブを確かめた (問題なし)。
- ユーザの指摘: 3D の表示に、スコアボードが出ていなかった。原因: 画面の倍率が 1 でないとき、`QOpenGLWidget` に描く QPainter は、窓 (`window()`) を物理画素で持ち (倍率 1.29 で 1653×930)、描くのは論理画素 (1280×720) で行う。スコアボードは窓の下端に置くので、画面の外に出ていた (4-5 から)。ほかの HUD は窓を使わずに論理画素で置くので、ずれなかった。直したこと: HUD を描く前に、QPainter の窓とビューポートをウィジェットの論理画素の範囲にする (2D の表示の QPainter と同じ)。画面のないテストでは OpenGL のウィジェットを作れないので、表示に出さない窓 (`WA_DontShowOnScreen`) に実際の `FieldView3D` で描き、Qt6 (倍率 1.29) と Qt5 で、スコアボードが左下に出ること、ほかの HUD と場面が前と同じ所にあることを確かめた。

