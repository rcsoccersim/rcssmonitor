# リリースの手順 (メンテナ向け)

| 項目 | 内容 |
|---|---|
| 対象 | rcssmonitor を公式にリリースするメンテナ |
| 英語版 | [release_en.md](release_en.md) (内容は同じ) |
| 配布 | この文書は配布物 (tarball) に含めない |

公式のリリース (ベータを含む) のたびに、この順に確かめる。

## 0. 次のリリースで忘れないこと (2026-11-22 の時点)

- 版は 20.0.0-beta.1 にしてあるが、まだリリースしていない。`feature/3d-view` は develop に取り込むところまでで、公式のリリースの前に機能を足す予定である。
- **20.0.0 には、`docs/plan/2026-11-22-00_v20-features.md` の機能 (F1〜F6: 配色、アイコン、デスクトップ、審判の笛、URL とログの一覧、画像と動画) を入れる**。どこまで入れてリリースするかは、途中でユーザが決めてよい (同書 §4)。
- **ChangeLog の先頭の 20.0.0-beta.1 の項目の日付 (2026-11-22) は仮**。リリースの日付に直す。
- **ブラウザ版 (rcssmonitor-web) の URL** を README と NEWS に足す。rcsoccersim の Organization に移した後の `https://rcsoccersim.github.io/rcssmonitor-web/` になる見込み。公開を確かめてから書く。
- **master への初めての取り込み (19.0.1 の後)** では、AppImage のワークフロー `.github/workflows/main.yml` と `utils/appimage/` を消す (AppImage をやめた。計画書の D10)。`.gitignore` は master と develop の両方で足されているので add/add で衝突する。develop の方 (master の 2 行を含む) をとる。

## 1. 版を決める

- `CMakeLists.txt`:
  - `project(rcssmonitor VERSION X.Y.Z)` の数字。
  - `RCSSMONITOR_VERSION_SUFFIX`: ベータなら `beta.N`、正式なリリースなら空 (`""`)。`--version`、窓の題名、tarball の名前は `X.Y.Z-beta.N` (空なら `X.Y.Z`) になる。
- `configure.ac`: `AC_INIT` の版を、上と同じ文字列にする (例: `20.0.0-beta.1`)。
- rcg ライブラリ (`librcssrcg`): インストールするヘッダの構造体や関数が、前のリリースと互換でなくなったら (構造体のメンバを足した、型を変えたなど)、soname を上げる。
  - `rcss/rcg/CMakeLists.txt` の `VERSION` と `SOVERSION`。
  - `rcss/rcg/Makefile.am` の `-version-info CURRENT:REVISION:AGE` (soname は CURRENT − AGE)。
  - CMake と autotools で soname をそろえる。20.0.0 では 20 (19.0.1 から `BallT` と `ServerParamT` にメンバが増えた)。

## 2. 表示と文書

- `src/main.cpp` の Copyright の西暦 (`2009 - YYYY`) を、リリースの年にする。
- `NEWS`: 見出しを `[版]` にし、変更点がそろっているかを見る。
- `ChangeLog`: リリースの日付で項目を書く (今までと同じ形: 日付、名前、ファイル、版)。
- `README.md`: ビルドの手順と案内 (ブラウザ版など) が今のものか。
- `docs/`: 利用者向けの文書 (`build.md`、`3d-view.md`、`FIGURE_PACKS.md` など) が今の動きと合っているか。

## 3. 確かめる

- develop の CI がすべて通っている。
- `./cmake-helper.sh distcheck qt6` と `./cmake-helper.sh distcheck qt5`: tarball をビルド、テスト、インストール、アンインストールできる。インストールしたものが `rcssmonitor-版` と名乗る。
- 画面で: 2D と 3D、ログの再生、ライブ (UDP、TCP、WebSocket)、遅延再生、人型パック。

## 4. 取り込みとタグ

- develop を master へ、GitHub の PR で取り込む (Merge Commit)。§0 の初めての取り込みの手当てを忘れない。
- master にタグを付ける: `rcssmonitor-X.Y.Z` (ベータは `rcssmonitor-X.Y.Z-beta.N`)。今までのタグ (`rcssmonitor-19.0.1` など) と同じ形。

## 5. リリースを作る

- タグのコミットで、作業ツリーをきれいにして `./cmake-helper.sh dist` を動かし、`rcssmonitor-版.tar.gz` を作る (HEAD のコミットから `git archive` で作る。`docs/plan/` と `docs/dev/`、CI の設定は入らない)。
- GitHub のリリースを、タグから作る。説明は NEWS から書き、ソースからのビルド (`docs/build.md`) とブラウザ版を案内する。tarball を添付する。AppImage はもう作らない。
- ベータは「pre-release」にする。
