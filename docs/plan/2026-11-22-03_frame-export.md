# 画面の連番画像と動画の出力 計画

| 項目 | 内容 |
|---|---|
| 作成日 | 2026-11-22 |
| 最終更新日 | 2026-11-24 |
| 対象 | rcssmonitor develop (5affd90、20.0.0-beta.1) |
| ブランチ | `feature/frame-export` (URL とログの一覧 (F5) を取り込んだ develop から。ユーザが GitHub 上で作る) |
| 状態 | 計画を作った (2026-11-22)。ユーザが develop (b3762c3) から `feature/frame-export` を作った。段階 1 を済ませ、段階 2 を始めた (2026-11-24)。総合の計画の最後の機能。段階ごとの進み具合は、次の「進み具合」の表 |
| 英語版 | [2026-11-22-03_frame-export_en.md](2026-11-22-03_frame-export_en.md) (内容は同じ) |
| 関連 | 総合の計画 [2026-11-22-00_v20-features.md](2026-11-22-00_v20-features.md) の F6。soccerwindow2 の `src/qt/image_save_dialog.cpp` (2D の連番画像) |

## 進み具合

段階ごとの作業と完了条件は §5、経緯は §10 にある。

| 段階 | 内容 | 状態 | ユーザの確認 | CI | コミット | プッシュ |
|---|---|---|---|---|---|---|
| 1 | 2D の 1 枚の画像 (File > Save Image) (§4.1、§4.10。D4、D5) | 完了 | 済み | 通過 | e9a9e96 | 済み |
| 2 | 範囲と連番画像の基本、出力のダイアログ (§4.3〜§4.6、§4.11。D6) | 作業中 | 済み | 未 | — | 未 |
| 3 | 範囲の便利な指定 (§4.4) | 未着手 | 未 | 未 | — | 未 |
| 4 | ffmpeg による動画 (§4.5) | 未着手 | 未 | 未 | — | 未 |
| 5 | 3D の描画の整理 (リファクタだけ。§4.2) | 未着手 | 未 | 未 | — | 未 |
| 6 | 3D のオフスクリーンの描画 (§4.2) | 未着手 | 未 | 未 | — | 未 |
| 7 | バッチのコマンドライン (§4.7) | 未着手 | 未 | 未 | — | 未 |
| 8 | Qt Multimedia の試作 (§4.8) | 未着手 | 未 | 未 | — | 未 |
| — | develop への取り込み (PR、Squash Merge) | 未着手 | — | — | — | — |

## 1. 目的と方針

- 画面に描かれている内容を、そのまま連番画像として、または動画として出力する。2D 表示と 3D 表示の両方。
- 連番画像と、外部の ffmpeg による動画を主にする (Qt5 と Qt6 の両方。ビルドの依存は増えない)。加えて、Qt6 (6.8 以上) に限って、Qt Multimedia で直接動画にする方法を試作し、残すかどうかを測って決める (ユーザ、2026-11-22)。
- **オフラインで描く**: 再生しながら画面を録るのではなく、出力の各コマを (フレームの番号、補間の位置) と固定の時間の刻みから描き直す。機械の速さによらず、同じ結果になる。対話しながらの画面の録画は、OBS などの画面の録画のソフトに任せる (対象外)。
- 「画面のとおり」: 出力の論理の大きさを画面に近く保ち、解像度は devicePixelRatio (dpr) で変える。ズーム、フォーカス、文字やラベルや HUD の大きさが、画面と同じ見た目になる。

## 2. 現状 (develop 5affd90)

### 2.1 2D 表示

- `FieldCanvas` (`src/field_canvas.{h,cpp}`) は `QWidget`。`paintEvent` (250 行) が private の `draw( QPainter & )` (312 行) を呼ぶ。`draw` は次の順に動く:
  1. `updateFocus()` (267 行): ボールに近い選手の自動の選択 (`Options` の選択の番号) と、フォーカス点を変える。
  2. `Options::updateFieldSize( width(), height() )` (options.cpp:2251): キャンバスの大きさ、フィールドの中心、(ズームしていなければ) 尺度を変える。
  3. `FieldPainter` の後に、`ScoreBoardPainter`、`TeamGraphicPainter`、`PlayerPainter`、`BallPainter`、`DrawInfoPainter`。
- フィールドの座標の変換は、`Options` (シングルトン) の大域の状態。保存と復元の手本がある: `Options::FieldTransform` と `setFieldTransform()` (options.h:719)。`GroundTexture` (`src/view3d/ground_texture.cpp:97〜104`) が、`FieldPainter` を `QImage` に描くときに使っている。
- 描き手は、`painter.window()` (field_painter、score_board_painter) と `opt.canvasWidth()` (team_graphic_painter) で位置を決める。
- 画像の保存の機能はない。コメントアウトされた `M_save_image_act` が残っている (main_window.cpp:1998)。

### 2.2 3D 表示

- `FieldView3D` (`src/view3d/field_view_3d.{h,cpp}`) は `QOpenGLWindow` (`NoPartialUpdate`) で、`QWidget::createWindowContainer()` で埋め込む。別の native の窓なので、メインの窓の `grab()` には写らない。
- `paintGL()` (190 行): 補間の位置を決め (ライブは壁時計 `M_newest_clock`、`M_live_tick`、ログは `LogPlayer::alpha()`)、`FrameBuilder3D` で中身を作り、カメラを動かし (グライドの時間の刻みは壁時計 `M_frame_clock`、313 行)、`Scene3D::render( defaultFramebufferObject(), … )` で描き、QPainter で HUD (スコアボード、チームのロゴ、重ね描きの文字、方位、ミニマップ、FPS) を描く。
- `Scene3D::render` は任意の framebuffer に描ける。テストには、`QOffscreenSurface` と `QOpenGLFramebufferObject` でオフスクリーンに描く例がある (`test/test_scene_3d.cpp`)。
- `test/test_gl_isolation.cpp` は、`src/view3d/gl/` の外での生の GL の呼び出し (`glXxx(`、`GL_XXX`) を禁じている。
- `FieldView3D::unavailableReason()` (120 行) は、offscreen のプラットフォームなどでは 3D を使わせない。

### 2.3 再生とフレーム

- 1 フレーム (`DispHolder::dispCont()` の 1 つ) は、シミュレーションの 1 ステップ (既定 100 ms)。止まっているとき (`stime`) のフレームも 1 つと数える。
- `DispHolder::getIndex( cycle )` (disp_holder.cpp:672) はサイクルの最初の番号を返す。`setIndex()` (635 行) は番号を変える。
- ライブで 65535 フレームを超えると、古いフレームを捨てて番号がずれる (`trimLiveFrames`、261 行)。

### 2.4 この環境

Ubuntu 24.04、Qt 6.4.2 と Qt 5.15.13。Qt Multimedia の開発パッケージも ffmpeg も入っていない (`qt6-multimedia-dev` 6.4.2 はパッケージとしてはある)。`QVideoFrameInput` は Qt 6.8 からなので、この環境の Qt では試作を作れない。

### 2.5 soccerwindow2 の連番画像の保存 (参考)

soccerwindow2 (`../../rctools/soccerwindow2`) の File > Save Image (`src/qt/image_save_dialog.cpp`、`ImageSaveDialog`):

- 範囲は、始めと終わりのサイクル (整数) の 2 つのスピンボックスと「Select All」のボタン。終わりの既定値は最初のサイクル (1 サイクルだけ)。
- サイクルから番号へは `ViewHolder::getIndexOf( cycle )` (サイクルの最初のフレーム。`lower_bound`)。終わりも同じなので、**終わりのサイクルの止まっているフレーム (2 つ目以降) は含まれない**。
- ダイアログはモーダルで、開くと再生を止める。ダイアログを開いたまま再生の位置を動かして範囲を決めることはできない。
- 出力の前に、コマの数や長さ、ディスクの量は出ない。同じ名前のファイルがあると、1 枚ごとに上書きを尋ねる (No / Yes / Yes to All)。
- 名前はログの番号 (`image-00042.png`)。画像の大きさ (幅と高さ)、接頭辞、形式、ディレクトリを選べる。
- 進捗のダイアログで中止でき、20 枚ごとに `processEvents` を呼ぶ。動画と 3D はない。コマンドライン (`--auto-image-save`) で全体を保存して終わることもできる。

## 3. 決定事項

### 3.1 ユーザの決定 (2026-11-22)

| # | 内容 |
|---|---|
| D1 | 連番画像と外部の ffmpeg による動画を主にする |
| D2 | Qt6 に限ってよいので、Qt Multimedia による直接の動画化も試作する |
| D3 | 出力するフレームの範囲を指定できるようにする。soccerwindow2 の連番画像の保存を参考にしてよいが、よりよくする (§4.4) |
| D4 | 設計上の判断 S1〜S9 は案のとおりとする (2026-11-24、段階 1 の初めにユーザが決めた) |
| D5 | File > Save Image... のショートカットは Ctrl+Shift+I (段階 2 の Export Images / Video... は Ctrl+Shift+E の予定)。3D 表示を出している間は、3D の保存を足す段階 6 まで無効にする (見えている画面と違う画像を保存しない) (同上) |
| D6 | File のメニューの 1 枚の保存と、範囲の出力は、別の項目のままにし、名前で役割をはっきりさせる (案 A): 「Save Image of This Frame...」(Ctrl+Shift+I。今のフレームを、ダイアログの欄を通さずに 1 手で保存する) と「Export Images / Video...」(Ctrl+Shift+E。範囲、大きさ、fps を選ぶ、モードレスのダイアログ)。1 つにまとめる案 (soccerwindow2 のように、始めと終わりを同じにすると 1 枚) と比べて、よく使う 1 枚の保存が 1 手で済み、既定の値とダイアログの性質の違いを分けられるため (2026-11-24、段階 2 の初めにユーザが決めた) |

### 3.2 設計上の判断 (Claude。段階の初めにユーザが確認する)

| # | 内容 | 理由 |
|---|---|---|
| S1 | オフラインで描く (§1)。サーバに接続している間は出力を無効にし、切断した後に残ったフレームは出力できる | 結果が機械の速さによらない。ライブでは番号がずれる (2.3) |
| S2 | 2D は、描くときに変わる大域の状態 (`Options` のフィールドの変換、キャンバスの大きさ、フォーカス点、選択の番号と、`DispHolder` の今の番号) を、1 コマごとに保存して戻す | 画面の状態を壊さない。コマの間に画面が描き直されても安全 |
| S3 | 3D は、描く処理を `paintGL` から別のクラスに切り出し、画面の `FieldView3D` と、出力用の `OffscreenView3D` (自前の GL のコンテキストとオフスクリーンの面) がそれぞれ持つ | 画面の 3D の状態 (カメラ、キャッシュ) を壊さない。3D の画面がなくても、xvfb の下でも描ける |
| S4 | `grabFramebuffer()` は使わない | `NoPartialUpdate` では `paintGL` の中でしか正しくない (Qt の文書) |
| S5 | ffmpeg には `QProcess` の標準入力で生のフレームを渡す。引数はリストで渡し、シェルを通さない | 依存が増えない。安全 |
| S6 | Qt Multimedia の試作は CMake のオプション (既定 OFF) の内側に作る。autotools では作らない | 試作で、最小の環境のビルドを壊さない |
| S7 | 出力の設定は設定ダイアログのタブにせず、出力のダイアログに置く。設定は新しい群 `Export` に保存する | `test_config_dialog.cpp` がタブの一覧を固定している |
| S8 | 出力のダイアログはモードレスにし、開いたまま再生の位置を動かして範囲の始めと終わりを取れるようにする。出力している間だけ、進捗のダイアログをアプリケーションのモーダルにする | soccerwindow2 はモーダルで、範囲を決めるのにサイクルの数を先に調べておく必要がある (2.5) |
| S9 | 範囲はログの番号の区間として持ち、表示と入力は「サイクル」か「サイクル.止まった番号」(画面のフレームの表示 `1234.3` と同じ書き方) で行う | サイクルだけでは、止まっているフレームを区別できない |

### 3.3 段階の初めに決めること

- 中止したとき、途中までのファイルを消すか、短い動画として残すか (段階 4)。
- Qt 6.8 以上で試作を作る方法 (段階 8): aqtinstall で `~/Qt` に入れる、ubuntu:26.04 のコンテナ、`continue-on-error` の CI のジョブ、のどれにするか。
- メニューの項目のショートカット (既存のショートカットとぶつからないものを選ぶ)。範囲の始めと終わりの印を付ける操作のショートカットも (段階 3)。
- 得点の場面の既定の秒数 (案: 得点の 10 秒前から 5 秒後) と、今の位置の前後の既定の秒数 (段階 3)。
- スライダーに、選んだ範囲の帯のほかに、得点の位置の印も出すか (段階 3)。

## 4. 設計

### 4.1 2D: オフスクリーンでの描画

- `Options::ViewState` (新規。`FieldTransform` の隣): フィールドの尺度、フィールドの中心、キャンバスの幅と高さ、フォーカス点、選択の番号。`viewState()`、`setViewState()`。ズームの有無とフォーカスの種類は描くときに変わらないので含めない。
- `src/frame_scope.{h,cpp}` (新規): RAII のガード。`DispHolder` の今の番号と `Options::viewState()` を保存し、指定の番号にし、消えるときに両方を戻す。2D と 3D で共用する。1 コマを囲む。
- `FieldCanvas`:
  - `draw( QPainter & )` を `draw( QPainter &, const QSize & logical )` にし、`paintEvent` は `size()` を渡す。
  - `QImage renderFrame( std::size_t index, const QSize & pixels, qreal dpr )` (新規、public): `QImage( pixels, Format_RGB32 )` をフィールドの色で塗り、`setDevicePixelRatio( dpr )`、`FrameScope` を開き、QPainter の window と viewport を論理の大きさで明示して、`draw` を呼ぶ。
  - ズームしているときは、論理の高さの比で尺度を合わせ、画面と同じ範囲を写す (段階 1 で確かめる)。
  - 中ボタンでの計測の線は、画面の操作の一時的な表示なので含めない。
- 限界: `Options::scale()` は論理の整数の画素に丸めるので、dpr 2 では位置が 2 画素ずつ動く (画面と同じ)。ダイアログに「文字も画像に合わせて大きくする」の選択を置き、外すと dpr 1 で描く (動きはなめらか、文字は小さい)。

### 4.2 3D: オフスクリーンでの描画

`paintGL` の中で、壁時計や積み重なる状態に依存するもの:

| もの | 出力では |
|---|---|
| `M_newest_clock`、`M_live_tick` (ライブ) | 使わない。`Interpolate::positionAt( count, t )` で位置を決める |
| `LogPlayer::alpha()` | 出力のコマの時刻 `t` の小数部 |
| カメラのグライドの `M_frame_clock` | 固定の `dt = 1 / fps` |
| `CameraController` の状態 | 画面のものを写し、最初のコマで `snap()` (すぐに目標の姿勢にする) |
| FPS の計測と表示 | 使わない (出力には FPS を描かない) |
| GPU のキャッシュ (地面、スタジアム、観客席、人型パック、ミニマップ、ラベルの atlas、MSAA の描き先) | 出力用に別に持つ |
| `Options` の選択とフィールドの大きさ、`DispHolder` の今の番号 | `FrameScope` |

- **リファクタ (振る舞いは変えない)**: `src/view3d/view_3d_renderer.{h,cpp}` (新規) に、描き手 (`FieldPainter`、`ScoreBoardPainter`、`TeamGraphicPainter`)、`LabelAtlas`、`FrameBuilder3D`、`Scene3D`、`SceneContent`、画像のキャッシュを移す。`initialize()`、`release()`、`build( at, live, delay, mode )`、`subject()`、`render( fbo, pose, logical, dpr, samples )`、`drawHud( painter, … )`。`FieldView3D` は、時計、カメラ、マウスの操作、FPS を持ち続け、`paintGL` は「位置を決める → build → カメラを動かす → render → HUD」になる。
- `CameraController::snap()` (新規)、`FieldView3D::camera()` (新規)。
- `TeamGraphicPainter::update()` を `draw()` から分ける。今はロゴの画像が HUD を描くときにしか作られないので、出力の最初のコマの看板にロゴが出ない。`build()` の初めに呼ぶ。
- `src/view3d/offscreen_view_3d.{h,cpp}` (新規):
  - `unavailableReason()`: `FieldView3D::unavailableReason()` の確認から、プラットフォームの確認を除いたもの。
  - `start( const CameraController & from, … )`: `FieldView3D::surfaceFormat()` で GL のコンテキストと `QOffscreenSurface` を作り、描き手を初期化し、カメラを写して `snap()`。3D の画面がなければ、`Options` のカメラで新しく作る。
  - `render( at, pixels, dpr, dt, samples )`: `makeCurrent`、`FrameScope`、build、カメラの更新、`render`、画像に読み戻して `Format_RGB32` に、HUD をラスタの QPainter で画像に描く、`doneCurrent`。
  - `renderStill( at, pose, … )`: 3D の「画像を保存」用。画面の今の状態をそのまま使う。
  - 範囲の始め (§4.4。複数の範囲を 1 つの動画にまとめるときは、範囲ごとの始め) では、補間と動きのキャッシュ (`FrameBuilder3D` の歩きの表、ボールの回転、宙を飛ぶボール) を消し、カメラを `snap()` する。前の範囲から続けて描くと、離れた場面の間でカメラが滑ったり、ボールが飛んだりするため。
- `src/view3d/gl/offscreen_target.{h,cpp}` (新規): 出力用の framebuffer (大きさが変われば作り直す、深さとステンシル)、読み戻し。生の GL の呼び出しは `gl/` の中だけ (`test_gl_isolation`)。
- 大きさの上限は 3840×2160 (MSAA の記憶)。

### 4.3 時間の模型

- `src/export_timing.{h,cpp}` (新規、純粋な関数)。1 つの範囲 (番号の `first`〜`last`。§4.4) ごとに:
  - `per_image = speed × 1000 / (step_ms × fps)` (出力の 1 コマで進むフレームの数)
  - `N = floor( (last − first) / per_image + ε ) + 1`、`t_n = first + n × per_image` (足し算を重ねず n から計算する)
  - 2D: 番号 `= min( last, floor( t_n + ε ) )`。3D: `Interpolate::positionAt( frames.size(), t_n )`
  - 動画の長さ `= N / fps`
- 既定: 2D は 10 fps (1 フレームが 1 コマ)、3D は 30 fps (60 も選べる)。「モニタの再生と同じ速さ」のボタン (`speed = step / 再生のタイマーの間隔`)。
- 複数の範囲を 1 つの動画にまとめるときは、範囲ごとのコマを順に並べる (長さは各範囲の和)。
- 画像の名前の既定は、出力のコマの番号 (0 から): `PREFIX%0Wd.ext`、`W = max( 5, N−1 の桁数 )`。連続しているので、ffmpeg の image2 で読める (soccerwindow2 はログの番号で名付けるが、それが一致するのは 2D の 10 fps、1 倍速のときだけ)。サイクルで名付ける選択は §4.4。

### 4.4 出力する範囲

soccerwindow2 (2.5) を土台に、次のようにする。

**位置の書き方と、範囲の意味**
- 位置は「サイクル」(`1234`) か「サイクル.止まった番号」(`1234.3`。画面のフレームの表示と同じ書き方) で書く。試合の時刻 (分:秒。サイクル × ステップ) も並べて出す。
- 始めの `1234` はそのサイクルの最初のフレーム、終わりの `1234` はそのサイクルの**最後の**フレーム (止まっているフレームも含む)。`1234.3` はちょうどそのフレーム。始めと終わりの両方を含む。
- ログにないサイクル (ログの外、ライブで欠けたところ): 始めは次にあるフレーム、終わりは前にあるフレームに寄せ、寄せたことをダイアログに出す。
- `src/export_range.{h,cpp}` (新規、純粋な関数): 位置の解釈と書き出し、始めと終わりの番号への変換 (`DispHolder::getIndex( cycle )` と、新しい `DispHolder::lastIndex( cycle )`)、範囲の検査、下のよく使う範囲。範囲は `{ first, last, 名前 }` (番号の区間) で持つ。

**今の位置から取る**
- ダイアログはモードレス (S8)。開いたまま、スライダー、コマ送り、サイクルの欄で位置を動かし、「始めを今の位置に」「終わりを今の位置に」で取る。
- メインの窓からも、範囲の始めと終わりの印を付けられる (メニューの項目とショートカット。ダイアログを開かなくてよい)。印を消す操作も置く。ダイアログは印を読み、ダイアログで変えた範囲は印にも返す。
- 選んだ範囲を、再生のスライダー (`LogPlayerSlider`) の溝に色の帯で出す。帯は配色 (F1) に合わせる。
- 印と範囲は開いているログについてだけ覚え、別のログを開くか、サーバに接続したら消す (設定には保存しない)。

**よく使う範囲 (ボタンかメニューで選ぶ)**
- 全体。
- 区間: 前半、後半、延長の各ハーフ、PK 戦。プレイモードの遷移から求める (`first_half_over` と `time_over` で区切り、最初の `penalty_setup_*` から後を PK 戦とする)。ログにない区間は選べなくする。
- 得点の場面: 得点ごとに、得点の N 秒前から M 秒後 (`DispHolder::scoreChangedIndex()` の番号から。ログの端で切る)。得点の一覧 (時刻、得点したチーム、そのときのスコア) から選ぶか、「すべての得点」で得点ごとの範囲を一度に作る。
- 今の位置の前後: 今の位置の N 秒前から M 秒後。
- N と M の秒数は設定に保存する。

**複数の範囲**
- 範囲の一覧を持ち、足す、消す、並べ替える (例: 「すべての得点」で得点ごとの範囲が並ぶ)。
- 出し方を選ぶ: (a) 範囲ごとに別のファイル (動画) か別のディレクトリ (画像)。名前に範囲の名前を付ける (例 `PREFIX-goal1-c1234`)。(b) 1 つの動画にまとめる (ハイライト)。範囲の間のつなぎの効果 (フェードなど) は付けない (§8)。
- 1 つの範囲だけのときは、今までどおり 1 つのファイルかディレクトリ。

**出力の前の確認**
- 範囲ごとと合計の、フレームの数、出力のコマの数、動画の長さ、試合の時刻の区間を出す。
- 連番画像は、最初のコマを描いて圧縮した大きさから、およそのディスクの量を見積もり、出力先の空き (`QStorageInfo`) と比べる。足りなければ警告する。
- 始めが終わりより後、空の範囲は、その場で知らせ、出力のボタンを押せなくする。
- 作る名前のファイルがすでにあるかを、出力の前に一度に調べ、1 回だけ確かめる (上書きする / 別の場所を選ぶ / やめる)。soccerwindow2 のように 1 枚ごとには尋ねない。

**画像の名前**
- 既定は出力のコマの番号 (§4.3)。
- 2D で 1 フレームを 1 コマにするとき (10 fps、1 倍速) は、サイクルで名付けることも選べる (`PREFIX-c01234.03.png`)。解析で、場面とファイルを対応づけるときに使う。

### 4.5 出力先

- `src/frame_sink.h` (新規): インタフェース。`open( pixels, fps )`、`write( QImage )` (受け付けた / 今は受けられない / 誤り)、`finish()`、`abort()`、シグナル `readyForFrame()`、`finished( ok, message )`。
- `src/image_sequence_sink.{h,cpp}` (新規): `QImageWriter`。形式は `QImageWriter::supportedImageFormats()` から (PNG、JPG など)。
- `src/ffmpeg_sink.{h,cpp}` (新規):
  - 引数 (静的な関数で組み、単体テストする):
    ```
    -hide_banner -loglevel error -nostats
    -f rawvideo -pixel_format bgr0 -video_size WxH -framerate F -i pipe:0
    -an -c:v libx264 -preset P -crf C [2D なら -tune animation]
    -vf scale=out_color_matrix=bt709:out_range=tv,format=yuv420p
    -colorspace bt709 -color_primaries bt709 -color_trc bt709
    [.mp4/.mov なら -movflags +faststart] -y OUT
    ```
    フレームは `QImage::Format_RGB32` (リトルエンディアンでは `bgr0`。ビッグエンディアンでは `0rgb`)。動画の大きさは偶数に丸める (yuv420p)。既定は CRF 18、preset `medium`。
  - 背圧: `QProcess::bytesToWrite()` が 2 コマ分を下回るまで、次のコマを描かない (`bytesWritten` を待つ)。`waitForBytesWritten` はイベントループを止めるので使わない。
  - ffmpeg の場所: 設定 (空なら `QStandardPaths::findExecutable( "ffmpeg" )`)。ダイアログを開いたときに `-version` (1 行目を出す) と `-encoders` (libx264 の有無) を調べ、使えるエンコーダだけを出す。
  - 誤り: 標準エラーの末尾 (16 KB) を覚え、起動の失敗、0 でない終了、途中の終了で、それを詳細として出す。
  - 終わり: `closeWriteChannel()` の後、`finished` を待つ (「動画を仕上げています」)。中止: `kill()` と、途中のファイルの扱い (§3.3)。
  - 音声は入れない。
- `src/export_job.{h,cpp}` (新規): 描き手 (2D は `FieldCanvas`、3D は `OffscreenView3D`) と出力先を受け取り、イベントループの中で 1 コマずつ描いて渡す `QObject`。進捗と終わりのシグナル。出力の前に再生を止める。

### 4.6 UI

- File > Save Image of This Frame... (D6。計画のときの名前は Save Image...): 今の表示を 1 枚、PNG または JPG で保存する。2D は `renderFrame( 今の番号, 画面の大きさ × dpr, dpr )`、3D は `renderStill`。
- File > Export Images / Video...: `src/export_dialog.{h,cpp}` (新規)。モードレス (S8)。
  - 表示: 2D / 3D (3D は `HAVE_3D_VIEW` で、`OffscreenView3D::unavailableReason()` が空のときだけ)。
  - 範囲 (§4.4): 範囲の一覧と、選んだ範囲の始めと終わり (サイクルかサイクル.止まった番号、時刻)。「始めを今の位置に」「終わりを今の位置に」、よく使う範囲 (全体、区間、得点の場面、今の位置の前後)、範囲を足す・消す、範囲ごとか 1 つにまとめるかの選択。範囲ごとと合計のコマの数と長さ、ディスクの見積もり。
  - 大きさ: 画面のとおり (W×H)、1280×720、1920×1080、3840×2160、任意。「文字も画像に合わせて大きくする」。
  - fps と速さ、「モニタの再生と同じ速さ」のボタン。
  - 出力: 動画 (ファイル、CRF、preset、ffmpeg の場所と状態、試作を作ったときはエンコーダの選択「ffmpeg (外部) / Qt Multimedia (試作)」) か、画像 (ディレクトリ、接頭辞、形式)。
  - 進捗のダイアログ (アプリケーションのモーダル): n/N、残りの時間、中止。
- メインの窓の範囲の印: 「範囲の始めにする」「範囲の終わりにする」「範囲の印を消す」(メニューの項目とショートカット)。スライダーに範囲の帯を出す (§4.4)。
- コマがあり、サーバに接続していないときだけ有効にする (ログを開いたとき、接続と切断のときに更新する)。

### 4.7 コマンドライン (バッチ)

- `--export-video FILE` | `--export-images DIR`、`--export-view 2d|3d`、`--export-range RANGE` (何度でも指定できる)、`--export-join` (範囲を 1 つの動画にまとめる)、`--export-fps`、`--export-speed`、`--export-size WxH`、`--export-scale R`、`--export-image-format`、`--export-image-prefix`、`--export-crf`、`--export-preset`、`--ffmpeg PATH`。保存しない設定として持つ。
- `RANGE` の書き方: `START-END` (`1234-1500`、`1234.3-1500` など。§4.4 の位置の書き方)、`all`、`first-half`、`second-half`、`extra-1`、`extra-2`、`penalties`、`goals[:N:M]` (得点ごと。N 秒前から M 秒後)。指定がなければ `all`。
- バッチのときは、窓を出さずにログを読み (メッセージのダイアログを出さない経路で)、出力して、終了コードを返す。進捗は標準エラーに出す。
- 2D は `QT_QPA_PLATFORM=offscreen` で動く。3D は GL のコンテキストが要るので `xvfb-run` の下で動かす (`OffscreenView3D` は `QOpenGLWindow` を作らないので xvfb でよい)。offscreen のプラットフォームでは 3D の画面が使えないので、3D の選択は `init()` の前に読む。

### 4.8 Qt Multimedia の試作 (Qt 6.8 以上)

- CMake: `option( ENABLE_QT_MULTIMEDIA_EXPORT "..." OFF )`。Qt6 で 6.8 以上のときだけ `find_package( Qt6 QUIET COMPONENTS Multimedia )`。`cmake-config.h.in` に `#cmakedefine HAVE_QT_MULTIMEDIA_EXPORT`。autotools では作らない。
- `src/qt_multimedia_sink.{h,cpp}` (新規、`FrameSink` の 1 つ):
  - `QVideoFrameFormat` (`Format_RGB32` に当たる画素の形式、`setStreamFrameRate`)、`QVideoFrameInput`、`QMediaCaptureSession`、`QMediaRecorder` (`QMediaFormat::MPEG4` と H.264、`isSupported( Encode )` を先に確かめる、`ConstantQualityEncoding`、`VeryHighQuality`、解像度と fps、`setAutoStop( true )`)。
  - 背圧: `sendVideoFrame()` が偽なら、`readyToSendVideoFrame` を待って同じ画像を送り直す。
  - 終わり: 空の `QVideoFrame` を送る。
- 作り方 (§3.3 で決める): aqtinstall (`aqt install-qt linux desktop 6.8.x linux_gcc_64 -m qtmultimedia qtwebsockets -O ~/Qt`、`CMAKE_PREFIX_PATH`)、ubuntu:26.04 のコンテナ (先に `qt6-multimedia-dev` の版が 6.8 以上か、FFmpeg のプラグインがどのパッケージにあるかを確かめる)、`continue-on-error` の CI のジョブ。
- **残すかを決めるために測ること**:
  1. 速さ: 2D の 1 試合 (1280×720、10 fps) と、3D の 1 分 (1920×1080、60 fps) の、全体の時間、CPU の時間、記憶の最大。ffmpeg の 8 割以上の速さか。
  2. 画質と大きさ: 同じ大きさで、連番画像に対する SSIM と、ファイルの大きさ。
  3. 正しさ: ffprobe でコマの数、fps、画素の形式と色空間、faststart。Firefox、Chrome、VLC での再生。
  4. 失敗の扱い: エンコーダがない、途中の中止、ディスクがいっぱい。
  5. 依存の重さと届く範囲: Qt Multimedia と FFmpeg のライブラリ、Qt 6.8 以上だけ、Windows と macOS で ffmpeg が要らない利点。
- 結果を本書の記録に書き、残すか、取り込まないか (試作のコミットを別のブランチに移す) を決める。

### 4.9 autotools

2D の部分 (段階 1〜4 と 7) だけを作る。3D と Qt Multimedia の部分は `HAVE_3D_VIEW`、`HAVE_QT_MULTIMEDIA_EXPORT` の内側に置く。

### 4.10 段階 1 の実装 (2026-11-24)

4.1 と、4.6 の File > Save Image (2D) を、次のように実装した。計画から決めたことも書く。

- `Options::ViewState` (`options.h`): 尺度、フィールドの中心、キャンバスの大きさ、尺度を合わせたときのチームのロゴとスコアボードの表示の有無、フォーカス点、選択の番号。`viewState()` と `setViewState()`。
  - `updateFieldSize()` は、尺度を合わせたときのロゴとスコアボードの表示の有無を、関数の中の static 変数に覚えていた。別のフレームを描くとそれが今の表示に変わるので、表示を変えた直後に保存すると、戻した画面の尺度が古いまま使われた (テストで見つけた)。`Options` のメンバ (`M_sized_with_team_graphic`、`M_sized_with_score_board`) にして、`ViewState` に入れた。
- `DispHolder::restoreIndex()` (新規): 今の番号を、無効な番号 (`INVALID_INDEX`。最新のフレームを描く) もそのまま戻す。`setIndex()` は無効な番号を受けないため。
- `src/frame_scope.{h,cpp}` (新規): `FrameScope`。作るときに今の番号と `viewState()` を覚えて番号を変え、消えるときに両方を戻す。フレームの外の番号は無視する (今のフレームを描く)。
- `FieldCanvas`:
  - `draw( QPainter &, const QSize & logical )`。`paintEvent` は `size()` を渡す。
  - `renderFrame( index, pixels, dpr )`: `Format_RGB32` の画像をフィールドの色で塗り、`setDevicePixelRatio( dpr )`、`FrameScope` を開く。ズームしていて論理の高さが画面と違えば、尺度を高さの比で変える。QPainter の window と viewport を論理の大きさにして `draw` を呼ぶ。中ボタンの計測の線は描かない。大きさが空なら空の画像を返す。
  - 描き手を調べた: どれも `DispHolder::currentDisp()` だけを見て、フレームをまたぐ状態を持たない (チームのロゴのキャッシュはチームの名前ごと)。
  - 文字のアンチエイリアス: Qt は、ウィジェットに描くとき (`paintEvent` を通るとき。`QWidget::render()` で画像に描いても) だけ、フォントの設定に従ってサブピクセルのアンチエイリアスを使い、画像に直接描くときは灰色にする。このため、画像の文字は画面の文字と画素では一致せず、Qt 5 では字の送り幅も少し違う。出力の画像や動画には、拡大や圧縮で色のにじみが出ない灰色のほうが向くので、このままにした。
- `MainWindow`:
  - File > Save Image... (Ctrl+Shift+I、`save_image`。段階 2 の初めに、D6 で「Save Image of This Frame...」に名前を変えた)。File のメニューの Save a Copy of Game Log の後に置いた。2D 表示の間だけ有効 (D5)。`updateSaveImageAction()` を、`toggleView3D()` で表示を変えた後に呼ぶ。3D 表示の間は、理由をステータスとツールチップに出す。
  - 選んだときの今のフレームを先に画像にしてから、ファイルのダイアログを出す (ダイアログの間も再生が進むため)。画像は、2D 表示の大きさ × 画面の dpr (`renderImage()`)。
  - 既定の名前は、ログの名前 (`.rcg`、`.rcg.gz` を除く。ライブでは `rcssmonitor`) とサイクル (止まっているときは `1234.3`)。例: `head_v5-1.png` (`defaultImageName()`)。拡張子がなければ PNG。形式は拡張子で決まる (`QImageWriter`)。保存したディレクトリは、起動している間だけ覚える (Save a Copy と同じ)。
  - ライブの間も使える。1 枚の画像は、オフラインで描く理由 (S1: 結果が機械の速さによらない、番号がずれる) に当たらないため。フレームがなければ、ステータスに「There is no frame to save.」。
  - `saveImage( path, error )` を公開した (テストのため)。
- テスト `field_canvas_render` (新規):
  - 文字 (背番号、選手の種類、スコアボード) を消すと、`QWidget::render()` とも `grab()` とも画素で一致する。文字ありでは、文字が描かれ、同じ条件で同じ画像になる。
  - 別のフレームを描いた後も画面は変わらず、そのフレームを表示すれば同じ画像になる。
  - ボールのフォーカス、ズーム、自動の選択の状態と、今の番号 (`INVALID_INDEX` も) が戻る (`FrameScope` だけでも)。
  - dpr 2: 画像の大きさと dpr、スコアボードが下にある、ゴールラインが 2 倍の位置にある。ズームしているときの尺度。空の大きさ。
  - メインの窓の Save Image: ショートカット、File のメニュー、フレームがないとき、画像の大きさと中身、名前、PNG と JPEG、書けない場所。
  - メニューの項目のテストは、計画の `test_toolbar` ではなく、同じログと画像を使うこのテストに入れた。`test_view_switch` には、3D を使えずに 2D に戻ったとき Save Image が有効なままであることを足した。3D 表示の間に無効になることは、offscreen では 3D を作れないので、手で確かめる。
- 確かめたこと (2026-11-24): 実際の試合のログ (RoboCup 2026) で、offscreen の `QT_SCALE_FACTOR` 1 と 2 で保存した。2 では 2000×1226 の画像になり、文字とチームのロゴは 2 倍の解像度で、スコアボードは下にあった。

### 4.11 段階 2 の実装 (2026-11-24)

4.3、4.4 の基本、4.5 の連番画像、4.6 の出力のダイアログ (2D、画像、範囲は 1 つ) を、次のように実装した。計画から決めたことも書く。

- `src/export_timing.{h,cpp}` (新規): static だけのクラス `ExportTiming`。`framesPerImage()`、`imageCount()`、`position()`、`frameAt()`、`frames2D()` (2D の画像ごとのフレーム)、`duration()`、`nameDigits()`、`imageName()`。位置は n から計算する (足し算を重ねない)。
- `src/export_range.{h,cpp}` (新規): `ExportPosition` (サイクルと止まった番号) と `ExportRange` (番号の区間と、static の関数)。`parsePosition()`、`positionText()`、`startIndex()`、`endIndex()`、`gameTime()`。
  - 計画の `DispHolder::lastIndex( cycle )` は作らず、`DispCont` (サイクルと止まった番号の順) に対する純粋な関数にした (テストでフレームの列を作れるため)。
  - 始めはその位置以後の最初のフレーム、終わりはその位置以前の最後のフレーム (サイクルなら、止まっているフレームまで)。ないときは寄せて、寄せたことを返す。
- `src/frame_sink.{h,cpp}` (新規): インタフェース `FrameSink` (`open`、`write` の ACCEPTED / BUSY / FAILED、`finish`、`abort`、`readyForFrame`、`finished`)。`frame_sink.cpp` は moc のためのもの (CMake の AUTOMOC は、対の .cpp のないヘッダを拾わないことがある)。
- `src/image_sequence_sink.{h,cpp}` (新規): `ImageSequenceSink`。ディレクトリ (なければ作る) に、与えた名前で 1 枚ずつ `QImageWriter` で書く。中止したとき、書いた画像は残す (中止したときの扱いの問い (§3.3) は動画の段階 4 で)。形式は PNG と JPEG (`formats()`)。
- `src/export_job.{h,cpp}` (新規): `ExportJob`。`start()` の後、イベントループの中で 1 枚ずつ `FieldCanvas::renderFrame()` で描いて出力先に渡す (`QTimer::singleShot( 0 )`)。BUSY なら `readyForFrame` を待つ。進み具合と終わりのシグナル。`abort()` は終わりをすぐ知らせる。
- `src/export_dialog.{h,cpp}` (新規): `ExportDialog` (モードレス)。
  - 範囲: 始めと終わりの欄 (空なら最初と最後のフレーム)、「Now」(今の位置)、「All」(両方を空に)。欄の右に、寄せた先の位置と試合の時刻、寄せたこと、誤りを出す。
  - 画像: 大きさ (画面のとおり、1280×720、1920×1080、3840×2160、任意)。「Scale the texts with the image」は、dpr を「画像の高さ ÷ 2D 表示の高さ」にする (文字が画面と同じ割合)。外すと dpr 1。速さと fps (既定 10 images/s、×1)。「モニタの再生と同じ速さ」のボタンは、まだ作っていない。
  - 出力: ディレクトリ、名前の頭、形式。「Name the images by the cycles」は、1 フレームが 1 枚のときだけ選べる。名前は `PREFIXc01234.00.png` (止まった番号も常に 2 桁以上にして、名前の順をフレームの順にそろえる)。
  - 要約: 画像とフレームの数、試合の時刻の区間、大きさ、動画の長さ。誤りのときは理由を出し、Export を押せなくする。
  - Export: すでにある名前のファイルを一度に数え、1 回だけ尋ねる (上書き / 別のディレクトリ / やめる)。最初の画像を描いて圧縮した大きさ × 枚数 × 1.1 を見積もり、空き (`QStorageInfo`) より多ければ警告する。設定を保存し、再生を止めて (`stopRequested`)、`ExportJob` を始める。進み具合はアプリケーションのモーダルの `QProgressDialog` (Stop で中止)。モーダルの `setValue()` はイベントを処理するので、文を先に、値を後に変え、終わりのときは `deleteLater()` で消す (F5 の段階 1 で見つけた落ち方を避ける)。
  - 接続している間は出力しない (S1。ボタンを無効にして理由を出す)。
  - 3D 表示の間も、2D の画像を出す (欄の見出しを「Images (2D view)」にした)。3D は段階 6。
- `Options::ExportSettings` と `writeExportSettings()` (群 `Export`。S7): ディレクトリ、名前の頭、形式、大きさ、任意の大きさ、文字の拡大、fps、速さ、サイクルでの名前。出力を始めるときに書く。
- `MainWindow`: File > Export Images / Video... (Ctrl+Shift+E、`export`。Save Image of This Frame の後)。フレームがあり、接続していないときだけ有効 (`updateExportAction()`。ログを開いたとき、接続と切断のときに呼ぶ)。ダイアログは初めて開くときに作り、`viewUpdated` で要約を更新する。
- テスト:
  - `export_timing` (新規): 1 枚の進むフレーム (2D、3D、速さ、ステップ)、枚数、2D のフレームの列 (半分の速さ、30 fps、長い区間の最後)、長さ、名前の桁。
  - `export_range` (新規): 位置の解釈 (誤りも)、書き出し、始めと終わりの番号 (止まっているフレーム、欠けたサイクル、ログの外、寄せ)、区間、試合の時刻。
  - `image_sequence_sink` (新規): 名前どおりのファイル (PNG、JPEG、ディレクトリを作る)、作れないディレクトリ、名前より多い画像。`ExportJob`: 描いた画像が `renderFrame()` と同じ、`start()` の後に描く、画面の状態が変わらない、中止 (書いた画像は残る)、出力先の誤り、フレームなし。
  - `export_dialog` (新規): メニュー (ショートカット、Save Image of This Frame の次、ログを開くと有効、モードレス、1 つだけ)、範囲 (空の欄、Now、寄せ、誤りで押せない)、出力 (ファイルの名前と大きさ、再生を止める、画面が変わらない、設定の保存、ステータスの知らせ)、サイクルでの名前 (選べる条件、半分の速さ)、大きさ (画面のとおり、1920×1080、文字の拡大)、すでにあるファイル (1 回だけ尋ね、やめれば書かない)、接続しているとき。
  - 画面が変わらないことのテストでは、ログを開いた後のツールバーの行の決め直し (0 ms のタイマー) を待ってから比べる。待たないと、出力の間に 2D 表示の大きさが変わる。
- 確かめたこと (2026-11-24): 実際の試合のログ (RoboCup 2026) の 1000〜1299 (止まっているフレームを含む 330 フレーム) を 1920×1080 の PNG で出した。10.3 秒 (1 枚 31 ms)、記憶は約 110 MB、ディスクは 35 MB (1 枚 106 KB)。画像の文字とロゴは、画面と同じ割合の大きさだった。ダイアログを画像にして見て、欄の幅を直した (始めと終わりの欄は同じ幅、右の説明は 1 行で、長いときは切る)。

## 5. 段階

どの段階も、ビルドでき、CI が通る。

| # | 作業 | 完了条件 |
|---|---|---|
| 1 | `Options::ViewState`、`FrameScope`、`FieldCanvas::renderFrame`、File > Save Image (2D) | 画面の `grab()` と同じ画像になる、状態が戻る、のテストが Qt5、Qt6、autotools で通る。HiDPI をユーザが画面で確かめる |
| 2 | `export_timing`、`export_range` の基本 (位置の書き方、始めと終わりの意味、寄せ、検査)、`FrameSink`、`ImageSequenceSink`、`ExportJob`、`ExportDialog` (モードレス。2D、画像。範囲は 1 つ: 始めと終わり、今の位置から、全体)、出力の前の確認 (コマの数、長さ、ディスク、上書きを 1 回)、`Export` の設定 | 1 試合と、選んだ範囲の PNG の連番を出せる。終わりのサイクルの止まっているフレームも入る。中止できる。出力の後も画面が変わらない |
| 3 | 範囲の便利な指定: メインの窓の印とスライダーの帯、よく使う範囲 (区間、得点の場面、今の位置の前後)、複数の範囲 (範囲ごとに別、または 1 つにまとめる)、サイクルでの名付け | 範囲のテストが通る。ユーザが実際のログで、得点の場面をまとめて出せることと、スライダーの帯を確かめる |
| 4 | `FfmpegSink`、ffmpeg の検出、引数、ダイアログの動画の部分、CI の 1 つのジョブに ffmpeg | 偽の ffmpeg と本物の ffmpeg のテストが通る。mp4 が再生でき、長さが N/fps (複数の範囲をまとめたときは和)。分かりやすい誤りの表示 |
| 5 | リファクタだけ: `View3DRenderer`、`TeamGraphicPainter::update()`、`CameraController::snap()`、`FieldView3D::camera()` | すべてのテストが通る。画面の 3D が変わらない (ユーザが確かめる) |
| 6 | `OffscreenView3D`、`gl/offscreen_target`、ダイアログの 3D、3D の Save Image、範囲の始めでのキャッシュの消去とカメラの `snap()` | 同じ入力で同じ画像になるテストが CI (llvmpipe) で通る。1080p、60 fps で出力できる。範囲をまとめた動画で、範囲の境目にカメラが滑らない。画面の 3D に影響しない |
| 7 | バッチのコマンドライン (`--export-range` など)、`docs/build.md` と README | offscreen での 2D のバッチのテスト。xvfb での 3D をユーザが確かめる |
| 8 | Qt Multimedia の試作 (OFF)、そのテスト、§4.8 の計測 | 残すかを決めて記録する |

段階 5 は、段階 2〜4 と並行して進めてもよい。

## 6. テスト

- `test_export_timing`: 2D の 10 fps、3D の 30/60 fps、速さ 0.5 と 2、100 ms でないステップでの N、`t_n`、長さ。複数の範囲をまとめたときの長さ。画像の名前と桁数。
- `test_export_range`: 位置の書き方の解釈と書き出し (`1234`、`1234.3`、誤り)。止まっているフレームのある `DispCont` での、始めと終わりの番号 (終わりのサイクルの止まっているフレームを含む)。ログの外と欠けたサイクルの寄せ。始めが終わりより後の検査。区間 (前半、後半、延長、PK 戦) を、合成したプレイモードの列から求める (区間のないログも)。得点の場面 (前後の秒、ログの端で切る、近い得点で重なる範囲)。今の位置の前後。サイクルでの名前。
- ダイアログ: 開いたまま位置を動かして「始めを今の位置に」で取れる。メインの窓の印とダイアログの範囲が同じになる。別のログを開くと印が消える。上書きの確認が 1 回だけ出る。
- `test_field_canvas_render`: `renderFrame( 今の番号, 大きさ, 1.0 )` と `grab().toImage()` が同じ (文字のアンチエイリアスで違えば、小さな許容)。別の番号、別の大きさで描いた後も `Options::viewState()` と今の番号が変わらない (ボールのフォーカス、ズーム、自動の選択)。dpr 2 でスコアボードが下の行にある (window が論理の大きさ)。
- `test_image_sequence_sink`: 一時ディレクトリに書いて読み戻す。
- `test_ffmpeg_sink` (Unix): 偽の ffmpeg のスクリプトを、テストの中で一時ディレクトリに作る (実行ファイルをコミットしない): 引数を書き出して標準入力を最後の引数のファイルに書くもの、少し読んで誤りで終わるもの、遅いもの、存在しないパス。バイトの数 = N×w×h×4、渡した引数、誤りの標準エラー、起動の失敗、中止でファイルが消える、`bytesToWrite()` が上限を超えない。本物の ffmpeg のテスト (なければ QSKIP) は ffprobe でコマの数を確かめる。
- `test_offscreen_view_3d` (GL 3.3 がなければ QSKIP。`test_scene_3d.cpp` と同じ): 同じ (番号、位置) を描き直すと同じ画像、画像の大きさ、`Options` と `DispHolder` の状態が戻る、同じ固定の刻みの列で 2 回同じ画像。
- `test_cameras_3d`: `snap()` で目標の姿勢に跳ぶ。
- メニューの項目 (`test_view_switch` か `test_toolbar` に足す)。
- `test_qt_multimedia_sink` (試作を作ったときだけ): 320×240 で 30 コマを出し、空でないファイルができる。H.264 のエンコーダがなければ QSKIP。
- 手で確かめること: HiDPI (`QT_SCALE_FACTOR` 1.25、1.5、2) で出力と画面を比べる。3D の各カメラ (中継のグライド、選手視点、自由、真上) と、4K での方位、ミニマップ、ラベル。ブラウザと VLC での再生と色。1 試合の出力で記憶が増え続けない。中止。ffmpeg がないとき。リファクタの後の画面の 3D。xvfb での 3D のバッチ。

## 7. 変更するファイル (見込み)

| ファイル | 内容 |
|---|---|
| `src/options.{h,cpp}` | `ViewState`、`Export` の設定、コマンドライン |
| `src/frame_scope.{h,cpp}`、`src/export_timing.{h,cpp}`、`src/export_range.{h,cpp}`、`src/frame_sink.h`、`src/image_sequence_sink.{h,cpp}`、`src/ffmpeg_sink.{h,cpp}`、`src/export_job.{h,cpp}`、`src/export_dialog.{h,cpp}` (新規) | 出力の仕組み |
| `src/field_canvas.{h,cpp}` | `renderFrame` |
| `src/disp_holder.{h,cpp}` | `lastIndex( cycle )` |
| `src/log_player_slider.{h,cpp}` | 範囲の帯 (と得点の印。§3.3) |
| `src/view3d/view_3d_renderer.{h,cpp}`、`src/view3d/offscreen_view_3d.{h,cpp}`、`src/view3d/gl/offscreen_target.{h,cpp}` (新規) | 3D のオフスクリーンの描画 |
| `src/view3d/field_view_3d.{h,cpp}`、`src/view3d/cameras_3d.{h,cpp}`、`src/team_graphic_painter.{h,cpp}` | リファクタ、`snap()`、`update()` |
| `src/qt_multimedia_sink.{h,cpp}` (新規、試作) | §4.8 |
| `src/main_window.{h,cpp}`、`src/main.cpp` | メニュー、範囲の印、バッチ |
| `CMakeLists.txt`、`src/CMakeLists.txt`、`cmake-config.h.in`、`src/Makefile.am` | ビルド |
| `test/`、`.github/workflows/ci.yml` | §6、CI の ffmpeg |
| README、`docs/build.md`、NEWS | 文書 |

## 8. 対象外

- 再生しながらの画面の録画 (OBS などに任せる)。
- 動画への音声 (審判の笛を含む)。
- 止まっているフレームを飛ばす出力 (補間が隣り合う番号を前提にしているため)。
- 複数の範囲を 1 つの動画にまとめるときの、つなぎの効果 (フェード、字幕など)。
- 範囲の一覧をファイルに保存すること。

## 9. リスク

- dpr のある `QImage` への QPainter の扱い: window と viewport を明示し、テストで確かめる。
- 2D の整数の画素への丸め (dpr が 1 より大きいとき)。
- libx264 のない ffmpeg: `-encoders` にあるものだけを出す。
- 2 つの GL のコンテキストと、4K の MSAA の記憶。
- PNG の連番のディスクの量 (2D の 1080p で 1 試合 1〜2 GB)。

## 10. 記録

### 2026-11-22: 計画の作成

- 総合の計画 (2026-11-22-00_v20-features.md) と一緒に作った。ユーザの決定 D1、D2 を受けた。
- 2D と 3D の描画の仕組みと、soccerwindow2 の画像の保存 (`ImageSaveDialog`: オフスクリーンの `QImage` に `FieldCanvas::draw` で描き、ログの番号で名付ける。動画はない) を調べて、設計を立てた。

### 2026-11-22: 出力する範囲の指定 (ユーザの求め)

- ユーザの求め (D3): 出力するフレームの範囲を指定できるようにする。soccerwindow2 の連番画像の保存を参考にしてよいが、よりよくする。
- soccerwindow2 の範囲の指定を調べ (2.5)、位置の書き方 (サイクル.止まった番号)、終わりのサイクルの止まっているフレームを含めること、モードレスのダイアログとメインの窓の印、スライダーの帯、よく使う範囲 (区間、得点の場面、今の位置の前後)、複数の範囲、出力の前の確認、サイクルでの名付けを足した (§4.4、S8、S9)。
- 範囲の便利な指定を段階 3 として足し、後の段階の番号を 1 つずつ後ろにずらした (ffmpeg は段階 4、Qt Multimedia の試作は段階 8)。

### 2026-11-24: ブランチの作成と書式の統一

- F5 (PR #75) が develop に取り込まれた (b3762c3) 後、ユーザが GitHub 上で `feature/frame-export` を作った。
- 計画書の書式の統一 (CLAUDE.md の「計画書の書式」。URL とログの一覧の計画書の 2026-11-24 の記録) に合わせて、冒頭の表に最終更新日を足して、進み具合の表を置いた。§4 (設計) は段階ごとの節に分かれていないので組み替えず、各段階の実装は設計の節の後に足していく。
- この後: 段階 1 を、始める前にユーザに確かめる (§3.3 の段階 1 の項目を決める)。

### 2026-11-24: 段階 1 の開始

- F6 のブランチの最初のコミット 9a1853b (計画書の書式の統一) をプッシュした。CI の 5 つのジョブがすべて通った。
- ユーザの指示で段階 1 を始めた。段階の初めにユーザが決めた: D4 (S1〜S9 は案のとおり)、D5 (Save Image のショートカットは Ctrl+Shift+I。3D 表示の間は段階 6 まで無効)。
- ショートカットを選ぶために調べたこと: File のメニューは Ctrl+O、Ctrl+Shift+O、Ctrl+Shift+B、Ctrl+Shift+S、Ctrl+Q を使っている。Ctrl+I は Next Viewpoint、Ctrl+S は Show Score Board。Ctrl+Shift+I と Ctrl+Shift+E は空いていて、3D 表示のキー操作 (修飾キーつきのキーは使わない) ともぶつからない。
- 2D の描き手を調べたこと: どの描き手も `DispHolder::currentDisp()` だけを見て、フレームをまたいで覚えている状態はない (チームのロゴのキャッシュはチームの名前ごと)。描くときに変わる大域の状態は、`updateFocus()` が変えるフォーカス点と選択の番号、`Options::updateFieldSize()` が変えるキャンバスの大きさ、尺度、フィールドの中心 (§4.1 の `ViewState`)。

### 2026-11-24: 段階 1 の実装

- 4.10 のとおり実装した。テスト `field_canvas_render` を足し、`test_view_switch` に Save Image の確認を足した。
- テストで見つけて直したこと: `updateFieldSize()` の static 変数のため、表示を変えた直後に別のフレームを描くと、画面の尺度が古いまま戻った (4.10)。画面との比べ方は、文字のアンチエイリアスが画面 (サブピクセル) と画像 (灰色) で違うため、文字を消して画素で比べる形にした (Qt 5 では字の送り幅も違った)。
- 笛のある Qt 6 (6.4.2) と Qt 5 (5.15.13)、`ENABLE_SOUND=OFF` の CMake のビルドで警告はなく、`ctest -j8` をそれぞれ 3 回走らせて、テスト 71 個がすべて通った。autotools (Qt 5) のビルドも、コンパイルの警告なしで通った。
- 実際の試合のログで、拡大率 1 と 2 の保存を確かめた (4.10)。README (「Saving an image of the view」)、NEWS、ChangeLog を更新した。
- ユーザが画面で Save Image を確かめた (段階 1 の完了条件)。
- この後: コミットとプッシュ、CI。

### 2026-11-24: 段階 1 のコミットと CI

- 段階 1 を e9a9e96 としてコミットし、プッシュした。CI の 5 つのジョブがすべて通った。
- この後: 段階 2 (範囲と連番画像の基本、出力のダイアログ) を、始める前にユーザに確かめる。

### 2026-11-24: 段階 2 の開始と、メニューの名前 (D6)

- ユーザが、1 枚の保存と範囲の出力を別のメニューにする理由を尋ねた。分ける利点 (1 枚が 1 手で済む、既定の値が違う、ダイアログの性質が違う) と欠点 (似た項目が 2 つ並ぶ、soccerwindow2 の 1 つの入り口と違う) を示し、3 つの案 (A: 分けて名前で区別、B: 1 つにまとめる、C: まとめて 1 手の保存も残す) を出した。ユーザが A を選んだ (D6)。Save Image... を「Save Image of This Frame...」に変えた (メニュー、ダイアログの題、README、NEWS、ChangeLog)。
- ユーザの指示で段階 2 を始めた。Export Images / Video... のショートカットは、D5 の予定のとおり Ctrl+Shift+E にした。

### 2026-11-24: 段階 2 の実装

- 4.11 のとおり実装した。テスト `export_timing`、`export_range`、`image_sequence_sink`、`export_dialog` を足した。
- テストと画面で見つけて直したこと: 出力のダイアログの終わりの欄が、長い誤りの文に押されて狭くなった (欄の幅を決め、説明を 1 行にした)。画面が変わらないことのテストが、ログを開いた後のツールバーの行の決め直しで 2D 表示の大きさが変わって失敗した (テストで配置を待つ)。
- 笛のある Qt 6 (6.4.2) と Qt 5 (5.15.13)、`ENABLE_SOUND=OFF` の CMake のビルドで警告はなく、`ctest -j8` をそれぞれ 3 回走らせて、テスト 75 個がすべて通った。autotools (Qt 5) のビルドも、コンパイルの警告なしで通った。
- 実際の試合のログで、1920×1080 の PNG の連番を確かめた (4.11)。README (「Saving images of the view」)、NEWS、ChangeLog を更新した。
- ユーザが出力を確かめた (段階 2 の完了条件)。
- この後: コミットとプッシュ、CI。
