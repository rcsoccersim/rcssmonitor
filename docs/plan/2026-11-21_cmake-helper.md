# cmake-helper.sh の追加 実装計画

| 項目 | 内容 |
|---|---|
| 作成日 | 2026-11-21 |
| 対象 | rcssmonitor develop (a6df084, 19.0.1) |
| ブランチ | `feature/cmake-helper` (develop から。ユーザが GitHub 上で作る) |
| 状態 | 実装とローカルでの試験は済 (2026-11-21)。コミット、CI、develop への PR はこれから |
| 英語版 | [2026-11-21_cmake-helper_en.md](2026-11-21_cmake-helper_en.md) (内容は同じ) |
| 関連 | 3D 表示の計画書 (`feature/3d-view` の `docs/plan/2026-11-11_3d-view.md`) の §14「2026-11-21」と Phase 10 |

## 1. 目的と方針

- CMake でよく使う操作 (configure、ビルド、テスト、実行、インストール、アンインストール、ソース tarball の作成と検査) を 1 つのコマンドで行えるスクリプト `cmake-helper.sh` を追加する。
- soccerwindow2 (`rctools/soccerwindow2`) にはすでに `cmake-helper.sh` がある。目的はそれと同じにし、使い方もそろえる。
- 実行する cmake コマンドを表示してから実行する。使う人が実際のコマンドを覚えられるようにするためである。
- 3D 表示とは独立した変更なので、3D 表示の計画には含めない。別のブランチと、この小さな計画書で扱う。

## 2. 決定事項

### 2.1 ユーザの決定 (2026-11-21)

| # | 内容 |
|---|---|
| D1 | develop から作るブランチ `feature/cmake-helper` で作業し、この計画書を置いて develop へ PR を出す。機能のまとまりなので Squash Merge が適する |
| D2 | CMake の既定のインストール先を `~/.local` にする |
| D3 | `dist` で作る tarball の名前はリリース番号にする: `rcssmonitor-VERSION.tar.gz` (VERSION は `project()` の版)。展開先は `rcssmonitor-VERSION/` |
| D4 | リリース用の tarball を `dist` で作るかどうかは、3D 表示の計画の Phase 10 で改めて検討する。それまで autotools の `make dist` はそのまま残す |

### 2.2 設計上の判断 (Claude。PR の前にユーザが確認する)

| # | 内容 | 理由 |
|---|---|---|
| S1 | インストールした `rcssmonitor` に RPATH `$ORIGIN/../lib` を埋め込む | rcg ライブラリ (`librcssrcg.so`) は `PREFIX/lib` に入る。`~/.local/lib` は既定のライブラリ検索パスにないので、RPATH がないと起動できない |
| S2 | デスクトップエントリの `TryExec` と `Exec` に、インストール先の `bin` の絶対パスを埋め込む。`data/rcssmonitor.desktop` を `data/rcssmonitor.desktop.in` に改名し、インストールのときに生成する (`install(CODE)`) | `~/.local/bin` が PATH にない環境でも、メニューから起動できるようにする。インストールのときに生成するので、`cmake --install --prefix DIR` で指定した先にも追従する。DESTDIR はパスに含めない |
| S3 | `uninstall` は `install_manifest.txt` にあるファイルを消し、ディレクトリは空になった `include/rcss/rcg` と `include/rcss` だけを消す | `bin`、`lib` などは他のパッケージと共用なので消さない |
| S4 | `dist` は `git archive` で HEAD から作る。`.gitattributes` の `export-ignore` で `.github`、`.gitignore`、`.gitattributes`、`docs/plan` を除く。未コミットの変更があれば警告する。ソースが git 作業ツリーのトップでなければエラーにする。tarball は既定でソースのトップに作る (キャッシュ変数 `RCSSMONITOR_DIST_DIR` で変えられる) | soccerwindow2 と同じ方式。展開した tarball を別の git 作業ツリーに置いても、その別のリポジトリを固めてしまわない |
| S5 | `distcheck` は tarball を作って展開し、ソースを読み取り専用にしてから configure、ビルド、ctest を行う。続けて作業ディレクトリへインストールし、主なファイル、デスクトップエントリの `TryExec`/`Exec` を確かめる (`desktop-file-validate` があれば検証も)。インストールした実行ファイルを、LD_LIBRARY_PATH なし、HOME を作業ディレクトリにして `--version` で起動し、アンインストール後にファイルが残らないことを確かめる。全部通れば作業ディレクトリを消す | autotools の `make distcheck` に相当する。HOME を替えるのは、`rcssmonitor` が `--version` でも終了時に `~/.rcssmonitor.conf` を書くため |
| S6 | Qt の選択は既存の `QT_VERSION_MAJOR` を使う (`auto`/`qt6`/`qt5` → ビルドディレクトリ `build`/`build-qt6`/`build-qt5`) | soccerwindow2 は独自のキャッシュ変数を使うが、rcssmonitor にはすでに `QT_VERSION_MAJOR` がある |
| S7 | `.gitignore` を追加する。master にある `.gitignore` の 2 行 (`*.o`、`build/`) を先頭にそのまま置き、`/build-qt5/`、`/build-qt6/`、`/rcssmonitor-*.tar.gz` を加える | develop には `.gitignore` がない。develop を master にマージするときの add/add の衝突を小さくする。なお、ユーザの `~/.gitignore` は `.gitignore` を無視するので、コミットには `git add -f` が要る |
| S8 | CI に `cmake-distcheck` ジョブ (ubuntu-26.04、Qt6、`./cmake-helper.sh distcheck qt6`) を加える | tarball に要るファイルが漏れていないか、インストールとアンインストールが壊れていないかを毎回確かめる |
| S9 | cmake のスクリプト (`cmake/*.cmake`) の先頭で `cmake_minimum_required(VERSION 3.16)` を呼ぶ | `cmake -P` のスクリプトモードでは政策が設定されず、`while(TRUE)` が偽になる (CMP0012)。§6 の試験で見つかった |
| S10 | autotools は変更しない (`make install`、`make dist` はそのまま) | D4 |

## 3. 変更するファイル

| ファイル | 内容 |
|---|---|
| `cmake-helper.sh` (新規) | コマンド: `configure`、`build`、`test`、`run [-- ARGS]`、`install [--prefix DIR]`、`uninstall`、`dist`、`distcheck`、`clean [QT\|all]`、`help`。環境変数 `CMAKE_ARGS` (設定すると configure をやり直す) と `JOBS` |
| `cmake/Uninstall.cmake` (新規) | `uninstall` ターゲットのスクリプト (S3) |
| `cmake/Dist.cmake` (新規) | `dist` ターゲットのスクリプト (S4) |
| `cmake/DistCheck.cmake` (新規) | `distcheck` ターゲットのスクリプト (S5)。作業ディレクトリの名前が `distcheck` でなければ消さない |
| `CMakeLists.txt` | 既定のインストール先 (D2)、RPATH (S1)、デスクトップエントリの生成 (S2)、`uninstall`/`dist`/`distcheck` ターゲット |
| `data/rcssmonitor.desktop` → `data/rcssmonitor.desktop.in` | `TryExec=@RCSSMONITOR_BINDIR@/rcssmonitor`、`Exec="@RCSSMONITOR_BINDIR@/rcssmonitor"` |
| `.gitattributes` (新規) | `export-ignore` (S4) |
| `.gitignore` (新規) | S7 |
| `README.md` | 「cmake-helper.sh」の節。`CMAKE_INSTALL_PREFIX` の既定値、インストールとアンインストール |
| `.github/workflows/ci.yml` | `cmake-distcheck` ジョブ (S8) |

## 4. soccerwindow2 との違い

| 項目 | soccerwindow2 | rcssmonitor |
|---|---|---|
| tarball の名前 | `soccerwindow2-YYYYMMDD.tar.gz` (HEAD のコミット日)、展開先 `soccerwindow2/` | `rcssmonitor-VERSION.tar.gz`、展開先 `rcssmonitor-VERSION/` (D3) |
| Qt の選択 | `SOCCERWINDOW2_QT_VERSION` | `QT_VERSION_MAJOR` (S6) |
| アンインストールで消すディレクトリ | `share/soccerwindow2` | `include/rcss/rcg`、`include/rcss` (S3) |
| ライブラリの検索 | `CMAKE_INSTALL_RPATH_USE_LINK_PATH` | `$ORIGIN/../lib` (S1。自前の共有ライブラリを入れるため) |
| デスクトップエントリ | `Exec` を置き換える | `TryExec` と `Exec` を置き換える |
| スクリプトの政策 | 設定しない (真偽の定数を使っていないので問題は出ない) | `cmake_minimum_required` で設定する (S9) |

## 5. 実装手順

1. develop から作業ツリーを作り、上のファイルを書く (済)。
2. ローカルで試験する (済。§6)。
3. ユーザが作った `feature/cmake-helper` に載せ、コミットとプッシュはユーザの確認を受けてから行う。
4. CI を確かめる。
5. PR のタイトルと説明の案を示す。ユーザが GitHub 上で develop への PR を作り、Squash Merge する。
6. マージの後、develop を `feature/3d-view` に取り込み、3D 表示の計画書に記録する (デスクトップエントリの改名、Phase 10 の tarball の項目)。

## 6. 記録

### 2026-11-21: 実装とローカルでの試験

作業は develop (a6df084) から作った作業ツリーで行った。試験は、変更をコミットした使い捨ての git リポジトリで行った。ユーザの環境を変えないよう、HOME を一時ディレクトリにし、`LD_LIBRARY_PATH` を外して実行した (3D 表示の計画書 §0.1)。

- `test qt6`: 15/15 が通った。
- `run qt6 -- --version`: `rcssmonitor-19.0.1` を出した。
- `install qt6 --prefix 一時ディレクトリ`: 15 個のファイルが入った。RUNPATH は `$ORIGIN/../lib`。LD_LIBRARY_PATH なしで、`librcssrcg.so.18` はインストール先の `lib` から見つかる。デスクトップエントリの `TryExec`/`Exec` はインストール先の絶対パスで、`desktop-file-validate` を通った。
- `install qt6` (既定のインストール先): `$HOME/.local` に入った。`uninstall qt6` の後にファイルは残らず、`include/rcss/rcg` と `include/rcss` も消えた。
- `dist qt6`: `rcssmonitor-19.0.1.tar.gz` ができ、展開先は `rcssmonitor-19.0.1/`。`export-ignore` のファイルは含まれない。未コミットの変更があると警告が出る。展開した tarball (git 管理外) で `dist` を実行するとエラーになる。
- `distcheck qt6`、`distcheck qt5` (システムの Qt5)、`distcheck qt6` を DISPLAY なし・offscreen で (CI と同じ条件): どれも通った。
- `auto` と `CMAKE_ARGS="-DENABLE_WEBSOCKET=OFF"`: `build` に Qt6 で、WebSocket なしで構成された。
- エラーの扱い: 未知のコマンド、`clean` 以外での `all`、インストールしていないビルドディレクトリでの `uninstall`、`install` の未知のオプションは、どれも終了コード 1。`clean all` は CMake のビルドディレクトリでない `build` を消さずに残し、終了コード 1。
- `dash -n` で構文を確かめた (shellcheck は入っていない)。

見つかった問題と対処:

- **`uninstall` の後に空の `include/rcss` が残った**: `cmake -P` のスクリプトモードでは政策 CMP0012 が設定されず、`while(TRUE)` の `TRUE` が変数名と解釈されて偽になった。3 つのスクリプトの先頭で `cmake_minimum_required(VERSION 3.16)` を呼ぶようにした (S9)。
- **`.gitignore` がコミットの対象に出てこない**: ユーザの `~/.gitignore` が `.gitignore` を無視している。コミットでは `git add -f .gitignore` とする (S7)。
- (環境) PATH の先頭に anaconda の環境があると、CMake はその Qt5 を見つける。スクリプトの問題ではない。システムの Qt5 での試験は、PATH から anaconda を外して行った。

### 残っていること

- `feature/cmake-helper` が GitHub 上に見つからない (2026-11-21 の時点で、リモートのブランチは develop、feature/3d-view、feature/qt6、feature/support-tcp-ws、master、proto/3d-gl)。ユーザに確かめる。
- develop の時点では、3D 表示の計画書は `docs/PLAN_3D-VIEW*.md` にあり、`export-ignore` されない。`feature/3d-view` を develop に取り込むと `docs/plan` に移るので、そのとき除かれる。
- develop を master にマージするときは、`.gitignore` が add/add で衝突する見込み。新しい方が master の内容を含むので、新しい方をとればよい。
