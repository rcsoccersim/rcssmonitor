# 画面の連番画像と動画の出力 計画

| 項目 | 内容 |
|---|---|
| 作成日 | 2026-11-22 |
| 最終更新日 | 2026-11-25 |
| 対象 | rcssmonitor develop (5affd90、20.0.0-beta.1) |
| ブランチ | `feature/frame-export` (URL とログの一覧 (F5) を取り込んだ develop から。ユーザが GitHub 上で作る) |
| 状態 | 計画を作った (2026-11-22)。ユーザが develop (b3762c3) から `feature/frame-export` を作った。段階 1〜6 をコミットした。段階 6 の CI が 3D のテストで落ち、テストを直している (2026-11-25)。総合の計画の最後の機能。段階ごとの進み具合は、次の「進み具合」の表 |
| 英語版 | [2026-11-22-03_frame-export_en.md](2026-11-22-03_frame-export_en.md) (内容は同じ) |
| 関連 | 総合の計画 [2026-11-22-00_v20-features.md](2026-11-22-00_v20-features.md) の F6。soccerwindow2 の `src/qt/image_save_dialog.cpp` (2D の連番画像) |

## 進み具合

段階ごとの作業と完了条件は §5、経緯は §10 にある。

| 段階 | 内容 | 状態 | ユーザの確認 | CI | コミット | プッシュ |
|---|---|---|---|---|---|---|
| 1 | 2D の 1 枚の画像 (File > Save Image) (§4.1、§4.10。D4、D5) | 完了 | 済み | 通過 | e9a9e96 | 済み |
| 2 | 範囲と連番画像の基本、出力のダイアログ (§4.3〜§4.6、§4.11。D6) | 完了 | 済み | 通過 | d7adf47 | 済み |
| 3 | 範囲の便利な指定 (§4.4、§4.12。D7〜D12) | 完了 | 済み | 通過 | d7a5cd1 | 済み |
| 4 | ffmpeg による動画 (§4.5、§4.13。D13〜D16) | 完了 | 済み | 通過 | 83ab35e | 済み |
| 5 | 3D の描画の整理 (リファクタだけ。§4.2、§4.14) | 完了 | 済み | 通過 | ad3687b | 済み |
| 6 | 3D のオフスクリーンの描画 (§4.2、§4.15。D17、S10〜S15) | 完了 | 済み | 失敗 | 89b5168 | 済み |
| — | CI の修正 (段階 6 のテスト `sameInput`: 2 つの描き手の絵を、色の差 2 までで比べる) | 作業中 | — | 未 | — | 未 |
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
| D7 | 範囲の始めと終わりの印を付けるショートカットは `[` と `]` (今の位置に付ける)。印を消す操作は、使う機会が少ないのでメニューだけにする (2026-11-24、段階 3 の初めにユーザが決めた) |
| D8 | よく使う範囲の既定の秒数: 得点の場面は得点の 10 秒前から 5 秒後、今の位置の前後は 5 秒前から 5 秒後。ダイアログで変えられ、設定に保存する (同上) |
| D9 | 再生のスライダーに、選んだ範囲の帯のほかに、得点の位置の印も出す (同上) |
| D10 | 得点の位置は、`DispHolder::scoreChangedIndex()` (チームの記録が届いたときに覚える、新しいスコアの直前のフレームの番号。ログの再生の「次の得点へ」も使う) にそろえる。チームとスコアは、その次のフレームから読む。初めはフレームの列で得点が増えたところを数え直していたが、ユーザの問いを受けて、「得点」の決め方を 1 か所にするために変えた (B 案。2026-11-24、段階 3 の確認の前にユーザが決めた) |
| D11 | File のメニューでは、出力にかかわる項目を「Export Images / Video」の中にまとめる: Open the Export Dialog... (Ctrl+Shift+E)、Start the Range Here (`[`)、End the Range Here (`]`)、Clear the Range。初めの「Export Images / Video...」と「Export Range」(Mark the Start、Mark the End、Clear the Marks) では、2 つがつながっていることと、できることが分かりにくかった。メニューから印の項目を消してダイアログだけを入口にする案もユーザが出したが、メニューはショートカットを見つける手がかりで、1 つの項目の中にまとまるとつながりが見えるので、こちらにした (2026-11-24、段階 3 の確認でユーザが求め、Claude の勧めで決めた) |
| D12 | 出力のダイアログの「All」(今の範囲をログ全体にする) を「Clear」に改め、メニューの Clear the Range とそろえる。Now のボタンの簡易ヘルプ (ツールチップ) に、メインの窓の `[` と `]` (と、そのメニューの項目) を書く (同上) |
| D13 | 動画の出力を途中で止めたときは、書いていたファイルを消す (最後まで書かれない mp4 は再生できないことがあるため)。画像の連番は 1 枚ずつ完成しているので、今までどおり残す。範囲ごとの動画では、書き終えたファイルは残す (2026-11-25、段階 4 の初めにユーザが決めた) |
| D14 | 動画の設定としてダイアログに置くのは、画質 (CRF) と圧縮の速さ (preset) だけにする。コーデックは H.264 (libx264)、形式はファイルの拡張子 (.mp4、.mov、.mkv) で決まる。ffmpeg に libx264 がないときは、使えるもの (mpeg4 など) に替えて知らせる (同上) |
| D15 | 動画の既定の画質は CRF 18、preset medium (計画の案。見た目でほとんど劣化しない。2D の画面は平らな色が多く、ファイルは大きくなりにくい) (同上) |
| D16 | 出力のダイアログの「Scale the texts with the image」は、動き (画像の dpr を「画像の高さ ÷ 2D 表示の高さ」にして、画面を拡大した見た目にする) を変えず、名前を「Enlarge the texts, logos and lines with the image」に改める。ツールチップと README に、文字のほかにチームのロゴと線の太さ (キックの白い円など) も大きくなること、外すと画素の大きさのまま (大きな画像では小さく細い) で、動きがなめらかなことを書く。ユーザが画面で、チームのロゴとキックの円も大きく太くなることに気づき、仕様かを尋ねた。文字だけを大きくする案 (dpr 1 でフォントだけを拡大する) と、文字と線を別のチェックにする案は採らなかった。スコアボードの高さや背番号の位置などは画素の値で組んであり、描き手を何か所も直す必要があって、ずれが出やすいため。設定のキー `scale_texts` は変えない (A 案。2026-11-25、段階 4 の確認でユーザが決めた) |
| D17 | 出力のダイアログで 2D / 3D の表示を選ぶ。開いたときに画面に出ている表示に合わせ、その後はダイアログで変えられる。前回の選択は覚えない (A 案。2026-11-25、段階 6 の初めにユーザが決めた) |

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
| S10 | 出力は、描き手の型 `ExportRenderer` (2D は `FieldCanvas::renderFrame()` の包み、3D は `OffscreenView3D`) を通して描く。`ExportPart` に、3D の位置 (`positions`) と範囲の始めの画像 (`range_starts`) を持たせる (段階 6) | `ExportJob` は描き手を知らずに済み、範囲の始めをどの部分でも同じに扱える。2D の今までの呼び方とテストはそのまま |
| S11 | 範囲の始めでは、カメラを `snap()` するだけにし、ほかのキャッシュは消さない (計画の「補間と動きのキャッシュを消す」は要らなかった) (段階 6) | 選手の動き (`GaitTable`)、ボールの回転 (`BallSpin`)、宙を飛ぶボール (`BallFlight`) は、フレームだけから決まり、描く順によらない (3D の計画 §5.2)。ラベルの画像も選手ごとの固定の枠。ほかのフレームを描いた後には、影などで色が 1〜2 違う画素が少しあるだけだった (§4.15) |
| S12 | 出力用のフレームバッファは Qt の `QOpenGLFramebufferObject` を使い、計画の `gl/offscreen_target` は作らない。読み戻した画像のアルファは使わない (段階 6) | 生の GL の呼び出しが要らない (`test_gl_isolation`)。画面の窓にはアルファがないので、半透明のものが書いたアルファを捨てると画面と同じ色になる |
| S13 | メインの窓が `OffscreenView3D` を 1 つ持ち、初めて要るときに作って持ち続ける。Save Image of This Frame と出力で共用する (段階 6) | 人型を読み、画像を GPU に送るのは 1 回で済む。出力ごとに作ると 1 秒ほどかかる |
| S14 | fps は表示ごとに持つ (設定の `fps` と `fps_3d`。3D の既定は 30)。「As the view」の大きさと文字の拡大の比は、画面に出ている表示 (2D のキャンバスか 3D の窓) から取る。3D の動画には `-tune animation` を付けない (段階 6) | 2D は 1 フレーム 1 枚の 10、3D はフレームの間も描く 30 が自然 (4.3)。3D の画面は平らな色ではない |
| S15 | 画像が画面より小さく、dpr が 1 より小さいときは、画像の dpr を 1 にし、論理の大きさの window を画像の画素の viewport に写す。2D の `FieldCanvas::renderFrame()` も同じに直した (段階 6) | QPainter は 1 より小さい dpr を 1 として描く (確かめた)。2D でも、画面より小さい画像 (たとえば高さ 900 の画面からの 1280×720) では、配置の左上だけが写っていた (段階 1 からの不具合) |

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
  - 画像: 大きさ (画面のとおり、1280×720、1920×1080、3840×2160、任意)。「Scale the texts with the image」(段階 4 で「Enlarge the texts, logos and lines with the image」に改めた。D16) は、dpr を「画像の高さ ÷ 2D 表示の高さ」にする (文字が画面と同じ割合)。外すと dpr 1。速さと fps (既定 10 images/s、×1)。「モニタの再生と同じ速さ」のボタンは、まだ作っていない。
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

### 4.12 段階 3 の実装 (2026-11-24)

4.4 の範囲の便利な指定を、次のように実装した。計画から決めたことも書く。

- 区間の求め方を変えた: rcssserver 19 は、ハーフの終わりにプレイモードを `before_kick_off` にし (`Stadium::callHalfTime()`)、`first_half_over` は使っていない。さらに、実際のログには、ハーフの終わりの `before_kick_off` も記録されていなかった (2022 年の延長のある試合で確かめた)。そこで計画の「`first_half_over` と `time_over` で区切る」をやめ、ログのサーバのパラメータ (`half_time`、`nr_normal_halfs`、`extra_half_time`、`nr_extra_halfs`、`simulator_step`) から求めたサイクルの境で分ける。前半は 0〜3000、後半は 3001〜6000、延長は 1000 サイクルずつ。PK 戦は、最初の PK のプレイモードから後。フレームのある区間だけを出す。ハーフに時間がなければ、試合全体を 1 つの区間にする。
- `ExportRange` (追加): `periods()`、`goals()` (`DispHolder::scoreChangedIndex()` の番号ごとに、その次のフレームのチームとスコア。得点した側は、1 つ前の得点のスコアと比べて決める。スコアが増えない変わり目は除く。D10)、`around()` (あるサイクルの前後の秒数。ログの端で切る)、`cyclesOf()`。
- `src/export_range_list.{h,cpp}` (新規): `ExportRangeList` (範囲の一覧のモデル。`QObject`) と `ExportRangeSpec` (始めと終わりの位置の文と名前)。1 つ以上の範囲と、今の範囲。`resolve()` (文からフレームの区間)、`newName()`、`safeName()` (ディレクトリの名前にする)。メインの窓が持ち、印、ダイアログ、スライダーが同じ一覧を見る。別のログを開くか、サーバに接続すると消す。
- `MainWindow`: File > Export Images / Video の中に、Open the Export Dialog... (Ctrl+Shift+E)、Start the Range Here (`[`)、End the Range Here (`]`)、Clear the Range (D7、D11。初めは File > Export Range の Mark the Start、Mark the End、Clear the Marks だった)。メニューの項目にもツールチップを出す。今の範囲の始めと終わりを今の位置にし、ステータスに出す。有効になる条件は Export と同じ。スライダーをメンバにし、範囲の一覧が変わるたびに、帯と得点の印を渡す (`updateRangeMarks()`)。全体の範囲 (始めも終わりも空) は帯にしない。接続している間は出さない。
- `LogPlayerSlider`: `setBands()` と `setGoals()`。溝を描いた後に、範囲の帯 (今の範囲は濃く) と得点の印を描き、最後につまみを描く (`QSlider::paintEvent()` と同じく、描く部分を明示する)。帯の色は配色のハイライト、得点の印は配色に合わせた橙 (D9)。
- `ExportDialog`:
  - 範囲の一覧 (`QListWidget`。名前、始めと終わり、フレームの数か誤り) と、Add、Remove、Up、Down。今の範囲をログ全体にする Clear (初めは All。D12)。Now のボタンのツールチップに、メインの窓の `[` と `]` とそのメニューの項目を書く (D12)。今の範囲の名前、始め、終わりの欄は、一覧と双方向に同じにする。
  - Presets (メニュー): 全体、区間 (前半、後半、延長、PK 戦。試合の時刻つき)、得点ごと (時刻、チーム、スコア)、すべての得点 (得点ごとの範囲。一覧が全体の 1 つならそれに替え、そうでなければ後ろに足す)、今の位置の前後。名前は `first-half`、`goal1-c6713`、`around-c1234` など。秒数の欄 (得点の前と後、今の位置の前と後。D8) は設定に保存する。
  - 範囲が複数のとき: 範囲ごとに別のディレクトリ (名前を `safeName()` にし、同じ名前には `-2` などを付ける) か、1 つの連番 (番号は範囲をまたいで続ける。サイクルの名前が重なるときは誤り)。範囲が 1 つのときは、今までどおり 1 つのディレクトリ。
  - 要約は全部の範囲の合計。誤りは範囲の名前つきで出す。
- `ImageSequenceSink`: 名前にディレクトリを含められる (範囲のディレクトリを、最初の画像を書くときに作る)。
- `Options::ExportSettings` (追加): `separate_ranges`、`goal_before`、`goal_after`、`around_before`、`around_after`。
- テスト:
  - `export_range` (追加): 区間 (2 つのハーフ、延長と PK 戦、後半から始まるログ、最後の境より後、ハーフの時間なし、フレームなし)、得点 (`DispHolder` の番号の付け方、新しいスコアのフレームを指す番号、増えない変わり目、範囲の外の番号)、前後 (止まっているフレーム、ログの端、50 ms のステップ)。
  - `export_range_list` (新規): 編集、足す・消す・動かす (今の範囲が保たれる)、名前、文からフレームの区間。
  - `log_player_slider` (新規): 位置の順と端、帯 (今の範囲は濃い)、得点の印の色 (明暗)、帯の外は変わらない。
  - `export_dialog` (追加): 印 (`[` と `]`、メニュー、ダイアログとスライダーへの反映、印を消す、別のログで消える)、Presets (JSON のライブのフレームで、2 つのハーフと 2 つの得点の試合を作る)、複数の範囲 (別のディレクトリ、同じ名前、1 つの連番、サイクルの名前の重なり、範囲の誤り、一覧のボタン)。
- 確かめたこと (2026-11-24): 2022 年の延長のある試合のログで、区間は前半 (0〜3147 番)、後半、延長 1、延長 2 に分かれ、得点 (6713、HELIOS2022) を見つけた。すべての得点の場面 (6613〜6763、止まっているフレームを含む 201 枚) を 1280×720 で 3.1 秒で出した。D10 の後、2023 年のログ (Oxsy 対 YuShan2023) で 4 つの得点を見つけ、サイクル (437、797、1075、3536) はログのプレイモードの `goal_l`、`goal_r` と一致した。4 つの範囲 (804 枚) を範囲ごとのディレクトリに 12.9 秒で出した。ダイアログとスライダーを画像にして見た。

### 4.13 段階 4 の実装 (2026-11-25)

4.5 の ffmpeg による動画と、4.6 の動画の欄を、次のように実装した。計画から決めたことも書く。

- `src/ffmpeg_sink.{h,cpp}` (新規): `FfmpegSink`。
  - 引数は 4.5 のとおり (静的な `arguments()`)。フレームは `QImage::Format_RGB32` の行をそのまま標準入力に書く (`pixelFormat()`: リトルエンディアンでは `bgr0`)。動画の大きさが奇数なら開かない。
  - 背圧: 書き待ちが 2 コマ分以上のとき、`write()` は BUSY を返す。`bytesWritten` で下回ったら `readyForFrame()` を出す。
  - 終わり: `finish()` で標準入力を閉じ、ffmpeg が 0 で終わったら、ファイルのパスを添えて終わりを知らせる。0 でない終了、落ちたとき、入力の途中で終わったときは、書きかけのファイルを消し、標準エラーの末尾 (16 KB を覚え、1500 字を出す) を添えて知らせる。起動の失敗は `open()` の戻りだけで知らせる (二重に知らせない)。
  - `abort()`: ffmpeg を止め、ファイルを消す (D13)。
  - 場所 (`findProgram()`): 空なら PATH の `ffmpeg`、パスなら実行できるファイル、`/` のない名前は PATH から探す。
  - 検出 (`probe()`): `-hide_banner -version` (1 行目) と `-encoders` (映像のエンコーダの名前) を、それぞれ 5 秒まで待って調べる。libx264、libopenh264、mpeg4 の順に選ぶ (D14 の代わりのエンコーダ)。mpeg4 には `-q:v 2`、libopenh264 には `-b:v 10M` を渡す。
- `ExportJob`: 出力を「部分」(出力先とそのコマの列。`ExportPart`) の列にした。部分を順に開き、書き、終わらせてから次を開く。中止すると、今の部分の出力先だけを中止し、終わった部分は残す (D13)。進捗は全体の数で、`done == total` は「出力先が仕上げている」。終わりのメッセージは、部分ごとに 1 行。
- `ExportDialog`:
  - 出力: 「Images」と「Video (ffmpeg)」のラジオボタンで、画像の欄 (ディレクトリ、名前、形式、サイクルでの名前) と動画の欄を切り替える。動画の欄は、ファイルと Browse、Quality (CRF。0〜51)、Encoding speed (preset)、ffmpeg のパスと Browse、ffmpeg の状態。範囲の選択の文も替わる (A video for each, by its name / All in one video)。ラジオボタンは 2 つずつ `QButtonGroup` に入れた (同じ枠の中で、出力の選択と範囲の選択が互いに排他になり、設定を読むと範囲の選択が消えていたため)。
  - ffmpeg は、動画を選んだときに初めて調べる (画像だけを出す人には走らせない)。ffmpeg のパスを直すか、選ぶと調べ直す。状態の欄には版と使うエンコーダを出す。見つからないときは、赤で理由と「インストールするか、選ぶ」を出す。libx264 がないときは、使うエンコーダと、CRF と preset を使わないことを出し、その 2 つの欄を無効にする。
  - 動画のファイル: 拡張子がなければ `.mp4` を足す。`.mp4`、`.mov`、`.mkv` のほかは誤りにする (要約に出し、Export を無効にする)。範囲ごとの動画は `FILE-名前.ext` (範囲の名前を `safeName()` にし、重なれば `-2` などを付ける。画像のディレクトリと同じ `rangeFileNames()`)。範囲が 1 つなら `FILE` のまま。
  - 大きさ: 動画では偶数に切り下げる (yuv420p)。
  - 「As the Playback」のボタン (4.6): 速さを、ステップ / 再生の間隔 (`Options::timerInterval()`) にする (再生は 1 つの間隔に 1 フレーム進む)。
  - 要約: 動画では「a video of N s at F images/s」か「K videos, N s in all at F images/s」。ffmpeg や拡張子の誤りも要約に出す。
  - 出力の前: 動画のファイルがあれば 1 回だけ尋ねる (上書き、別のファイル、取り消し)。動画のディスクの見積もりはしない。2D なので `-tune animation` を付ける。
  - 進捗: 題は Export Video。2 秒を過ぎたら、それまでの速さからの残りの時間を出す (4.6 の「残りの時間」。画像にも)。全部を渡した後は「Finishing the video...」。
  - 終わり: 「Exported the video FILE.」か「Exported K videos in DIR.」。ステータスバーも同じ 1 行にした。
  - 出力の準備を、画像 (`prepareImages()`) と動画 (`prepareVideos()`) に分けた。
  - 文字の拡大のチェック (D16): 名前を「Enlarge the texts, logos and lines with the image」に改め、ツールチップに、画面を拡大した見た目になること (文字、チームのロゴ、線の太さが画像とともに大きくなる)、外すと画素の大きさのまま (大きな画像では小さく細い) で動きがなめらかなことを書いた。動き、オブジェクトの名前、設定のキー (`scale_texts`) は変えない。
- `Options::ExportSettings` (追加): `video`、`video_file`、`crf`、`preset`、`ffmpeg`。
- CI: CMake の Qt6 のジョブに `ffmpeg` を入れ、`RCSSMONITOR_REQUIRE_FFMPEG=1` で本物の ffmpeg のテストを必ず走らせる (なければ失敗にする)。ほかのジョブでは飛ばす。
- 文書: README (動画の節)、`docs/build.md` (実行時の任意の ffmpeg)、NEWS、ChangeLog。
- テスト:
  - `ffmpeg_sink` (新規): 引数 (libx264 の全体、tune なし、`.mkv`、12.5 fps、mpeg4、libopenh264)。場所の検出。検出 (版、3 つのエンコーダの選び方、音声の「libx264」は選ばない、エンコーダがない、失敗する、ない)。偽の ffmpeg (テストの中でシェルのスクリプトを作る) への書き込み (引数、バイトの数、色)。背圧 (BUSY と `readyForFrame()`。遅い偽の ffmpeg は作らず、イベントループを回さずに書いて確かめた)。開けないとき (奇数、ディレクトリ、起動できない。あったファイルは残る)。誤り (すぐに、入力の途中で。標準エラーの末尾、ファイルが消える)。中止 (ファイルが消え、終わりを知らせない)。複数の動画 (順に書く。2 つめを中止すると 1 つめは残る)。本物の ffmpeg (ffprobe で h264、大きさ、yuv420p、コマの数、長さ 0.4 秒。mp4 と mkv。間違った preset の誤り)。
  - `export_dialog` (追加): 動画 (欄の切り替え、既定の CRF と preset、状態の文、`.mp4` の補い、CRF と preset が引数に入る、偶数の大きさ、要約、ファイルの大きさ、設定、間違った拡張子、あるファイルの確認)、複数の動画 (範囲ごとのファイルの名前、同じ名前、1 つにまとめる、設定の読み直し)、ffmpeg なし (状態、要約、Export が無効、画像は出せる、mpeg4 に替えたときの表示と引数)、再生の速さのボタン。
- 確かめたこと (2026-11-25): 2023 年のログ (Oxsy 対 YuShan2023) の 4 つの得点の場面を、1280×720 で範囲ごとの 4 本の mp4 に 1.9 秒で出した。ffprobe で、H.264 (High)、yuv420p、bt709 (tv の範囲)、10 fps、各 201 コマ、20.1 秒だった。1 本にまとめると 804 コマ、80.4 秒 (和) だった。前半全体 (止まっているフレームを含む 3299 コマ) を 1920×1080 で 14.3 秒で出し、24 MB だった。動画から取り出したコマを見て、画面と同じ絵であることを確かめた。ダイアログを画像にして見た。
### 4.14 段階 5 の実装 (2026-11-25)

4.2 のリファクタを、次のように実装した。画面の 3D の見た目は変えない (チームのロゴが初めのフレームから看板に出ることだけが変わる)。計画から決めたことも書く。

- `src/view3d/view_3d_renderer.{h,cpp}` (新規): `View3DRenderer`。
  - 持つもの: 2D の描き手 (`FieldPainter`、`ScoreBoardPainter`、`TeamGraphicPainter`)、`LabelAtlas`、`FrameBuilder3D`、`Scene3D`、`SceneContent`、画像のキャッシュ (地面、両端の看板と横断幕、観客席、ミニマップのピッチ) と、その鍵、人型パックの体を送った印。
  - `initialize()` (今の文脈に GPU の資源を作り、人型を 1 回読む。新しい文脈では画像を送り直す)、`release()`、`ready()`、`setTiming()`、`takeGpuTime()`。
  - `build( at, live, delay_frames, mode )`: 選手の自動の選択、画像の更新 (地面、両端、観客席。変わったときだけ)、`FrameBuilder3D::build()`、人型の体を GPU へ。
  - `subject()`: カメラが追うもの。
  - `render( fbo, camera, logical, dpr, samples )`: カメラと表示の高さに合わせた背番号のラベル、カード、ボールの補助を足し、ラベルの画像を送り、`Scene3D::render()` で描く (計画の `render` の引数のとおり。ラベルはカメラが決まってから作るので、`build()` ではなくここに置いた)。
  - `drawHud( painter, camera, logical, dpr, mode )`: スコアボード、チームのロゴ、重ね描きの文字、方位とミニマップ。QPainter の window と viewport は呼ぶ側が論理の大きさにする。
  - `minimapShown( mode )` (静的): FPS の表示の位置のため (FPS は画面だけのものなので `FieldView3D` に残した)。
  - `buildTime()`、`submitTime()`: CPU の時間を描き手の中で測る (`RCSSMONITOR_3D_TIMING`)。build の時間は「`build()` の作る部分 + `render()` のラベルの部分」で、カメラの更新 (マイクロ秒の単位) は含まなくなった。
  - `state( camera )`、`targetAt( camera, size, point )`: テストの状態と、右クリックの的。
  - 標準エラーの文言 (`(FieldView3D) OpenGL 3.3 core is not available` など) は変えない。
- `FieldView3D`: 時計 (補間の位置、ライブの時計、カメラのグライドの時間)、カメラ、マウス、FPS、時間の計測を持ち続け、`paintGL` は「位置を決める → `build` → カメラを動かす → `render` → HUD (`drawHud`、FPS)」になった。`camera()` (新規) は段階 6 の出力がカメラを写すため。
- `TeamGraphicPainter::update()` (新規): タイルからロゴの画像を作る部分を `draw()` から分けた。`draw()` はこれを呼んでから描く (2D は変わらない)。`View3DRenderer::build()` が両端の看板を作る前に呼ぶので、看板のロゴが初めのフレームから出る (今までは HUD がロゴを描いた次のフレームから)。
- `CameraController::snap()` (新規): 次の `update()` で目標の姿勢に跳ぶ (カメラを替えたときと同じ。選手のカメラは初めから)。自由なカメラと真上のカメラはそのまま。
- 作業中に見つけて直したこと: `DispHolder` のコンストラクタが `M_playmode` を初期化していなかった (`clear()` だけが `PM_Null` にしていた)。プレイモードが来る前のフレームでは、スコアボードがプレイモードの名前の配列を未定義の番号で読む。新しいテストが落ちて分かった。コンストラクタで `PM_Null` にした (アプリは `clear()` を呼んでから読むので、画面の動きは変わらない)。
- テスト:
  - `view_3d_renderer` (新規。OpenGL 3.3 の文脈がなければ飛ばす): 初期化、`build` → カメラ → `render` の手順、同じフレームは同じ画像、別のフレームは別の画像、状態 (選手 22、位置、ラベル)、地面の色。`drawHud` がスコアボードを下に描き、真ん中を描かない。ロゴが初めのフレームから看板に出る (右のゴールの後ろのカメラで、初めのフレームと、HUD を描いた後の次のフレームが同じ。`update()` を呼ばないと落ちることを確かめた)。
  - `team_graphic_painter` (新規): `update()` だけでロゴができる (大きさ、色、タイルが増えると作り直す)、`draw()` が左上に描く、設定が切れているときとフレームがないときは作らない。
  - `cameras_3d` (追加): `snap()` (放送のカメラと寄りのカメラは跳び、その後はまた滑る。自由なカメラは動かない)。
  - `disp_holder` (追加): 新しい `DispHolder` のプレイモードは `PM_Null`。
- 確かめたこと (2026-11-25): Xvfb (llvmpipe) の上で、変更の前 (83ab35e) と後のモニタに同じ 2023 年のログを開き、3D の画面 (1100×624) をフレームバッファから取り出して画素で比べた。放送のカメラのサイクル 1、437、1000、2500 と、寄り (ボール)、真上、選手のカメラ (方位とミニマップを出す) のサイクル 437、1000 で、違う画素は 0 だった。

### 4.15 段階 6 の実装 (2026-11-25)

4.2 の 3D のオフスクリーンの描画と、4.6 のダイアログの 3D、3D の Save Image を、次のように実装した。計画から決めたことは S10〜S15 にも書いた。

- `src/view3d/offscreen_view_3d.{h,cpp}` (新規): `OffscreenView3D` (`ExportRenderer`)。
  - 自前の `QOpenGLContext` と `QOffscreenSurface` と `View3DRenderer` を持つ。`initialize()` で 1 回だけ作る (誤りは理由を返す)。窓は要らないので、プラットフォームの offscreen でも描ける。
  - `unavailableReason()` (静的): OpenGL 3.3 core の文脈と関数を試す。`FieldView3D::unavailableReason()` は、プラットフォームの確認の後にこれを呼ぶ形にした。
  - 1 枚の描き方 (`renderAt( position, pixels, dpr, dt )`): `FrameScope` (今の番号と `Options::viewState()` を戻す) の中で、`Interpolate::positionAt()` の位置、`View3DRenderer::build()`、カメラの `update( dt )`、`render()` を画像の大きさの `QOpenGLFramebufferObject` (大きさが変われば作り直す) に描き、読み戻し、アルファを捨てて `Format_RGB32` にし、HUD を QPainter で描く (S12、S15)。
  - 出力: `start( camera, fps )` で画面のカメラを写し、1 枚ごとに 1/fps 秒動かす。`startRange()` はカメラの `snap()` (S11)。`render( part, i, … )` は `ExportPart::positions` の位置で描く。
  - `renderStill( index, camera, pixels, dpr )`: Save Image of This Frame 用。カメラを写して動かさずに (`dt = 0`) 描く。
  - 大きさの上限 `MAX_WIDTH` × `MAX_HEIGHT` (3840×2160)。
- `ExportJob`: 描き手の型 `ExportRenderer` (`startRange()`、`render( part, i, pixels, dpr )`) を通して描く (S10)。2D は `FieldCanvas` を包んだもので、今までのコンストラクタのまま。範囲の始め (`ExportPart::startsRange()`: 部分の初めと `range_starts`) で `startRange()` を呼ぶ。描けなかった (空の画像) ときは止めて知らせる (「Could not draw the image N.」)。
- `ExportTiming`: `positions3D()` (フレームの間の位置。フレームに `EPSILON` まで近ければそのフレーム。範囲の終わりを越えない)、`DEFAULT_FPS_3D` (30)。
- `ExportDialog`:
  - 枠の名前を「Images」にし、初めの行に「View:」(2D view / 3D view。`QButtonGroup`) を置いた。開いたときに画面に出ている表示を選ぶ (D17。`MainWindow::showExportDialog()` が `chooseView3D()` を呼ぶ)。
  - `setView3DSources( view, offscreen )`: 3D の窓 (大きさとカメラ) と描き手。描き手が変わったときだけ `OffscreenView3D::unavailableReason()` を調べ、描けなければ 3D の選択を無効にして、理由を簡易ヘルプに出す。3D なしのビルドでは「This rcssmonitor is built without the 3D view.」。
  - fps は表示ごと (S14): 表示を替えると欄の値と、もう一方の値を入れ替える。設定の `fps` と `fps_3d`。
  - 「As the view」の大きさと文字の拡大の比は、画面に出ている表示から取る (`viewPixels()`、`viewHeight()`。3D の窓が出ていれば、その大きさと dpr)。
  - 3D の大きさが上限を越えると、要約に「A 3D image is 3840 x 2160 pixels at most.」を出し、Export を無効にする。
  - 出力の前に、3D なら描き手の `initialize()` を確かめる (出力先を作る前)。部分に、範囲ごとの位置と範囲の始めを入れる。カメラは画面の 3D のものを写す (3D の窓がなければ、設定のカメラで新しく作る)。3D の動画は `-tune animation` なし。
  - 文字の拡大のチェックの簡易ヘルプの「動きがなめらか」は 2D のことなので、「in 2D」を足した。
- `MainWindow`:
  - `OffscreenView3D` を 1 つ持ち、初めて要るときに作る (S13)。閉じるときは出力を止めてから消す。
  - Save Image of This Frame: 3D が出ていれば、`renderStill()` で 3D の窓の大きさ × dpr に描く。3D の間も有効にした (D5 の「段階 6 まで無効」の終わり)。
  - 3D の窓を作ったとき、出力のダイアログに渡す。
- 2D の直し (S15): `FieldCanvas::renderFrame()` は、dpr が 1 より小さいとき、画像の dpr を 1 にし、論理の window を画像の画素の viewport に写す。
- 文書: README (Save Image の 3D、出力の View、fps)、`docs/3d-view.md`、NEWS、ChangeLog。
- テスト:
  - `offscreen_view_3d` (新規。OpenGL 3.3 がなければ飛ばす): 初期化の前とフレームがないときは空、フレームの間の位置 (番号、次、割合、選手)、今の番号と表示の状態が戻る、スコアボードが下にある (dpr 1、2、0.5。高さの比も)、同じ入力で同じ画像 (2 つの描き手、範囲の始めを含む)、範囲の始めでカメラが跳び、そこから始めた描き手と同じ姿勢と絵 (色の差は 2 まで)、`renderStill()` はカメラを動かさず、出力のカメラを変えない、`ExportJob` での出力。
  - `image_sequence_sink` (追加): 別の描き手での `ExportJob` (位置、範囲の始めを知らせる、部分ごと、描けないとき)。
  - `export_timing` (追加): `positions3D()`。
  - `export_dialog` (追加): 3D の画像の出力 (開いたときは 2D、fps の入れ替え、7 枚、上限、設定、開き直すと 2D、動画の引数に `animation` がない)、3D を使えないとき。
  - `field_canvas_render` (追加): dpr 0.5 (スコアボードが下、ゴールラインが半分の位置)。直す前のコードで落ちることを確かめた。
- 確かめたこと (2026-11-25、Xvfb と Mesa):
  - 画面の 3D は、変更の前 (ad3687b) と後で画素まで同じだった (X サーバから窓を読んだ)。
  - 3D の Save Image の画像と画面: 違うのはスコアボードの文字だけ (画面は OpenGL の描画エンジン、画像はラスタの描画エンジンで、文字のアンチエイリアスが違う。2D の 4.11 と同じ)。3D の絵は画素まで同じだった。2 回目からは 5 ms ほどで描ける (初めは 150〜180 ms)。
  - この環境では、画面の `grabFramebuffer()` は真っ黒を返した (S4 のとおり)。画面と比べるときは、描き直させた後に X サーバから窓を読んだ。
  - 2023 年のログ (Oxsy 対 YuShan2023) の 4 つの得点の場面:
    - 320×180、30 fps の画像を、範囲ごとと 1 つの列とで出すと、2404 枚 (各 601 枚) が画素まで同じだった。範囲の境目では絵が大きく変わり、範囲の初めの 2 枚の間はほとんど変わらない (カメラが跳んでいる)。
    - 1280×720、30 fps で範囲ごとの 4 本の mp4 を 15.4 秒で出した。H.264、yuv420p、各 601 コマ、20.03 秒。
    - 1 つの得点の場面を 1920×1080、60 fps で、13.9 秒 (1 秒に 86 枚) で出した。1201 コマ、20.02 秒、13.5 MB。
  - 画面より小さい画像 (320×180) で、HUD が画面と同じ割合になることを見て確かめた (S15 の前は、ロゴが大きくスコアボードが出なかった)。

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

### 2026-11-24: 段階 2 のコミットと CI、段階 3 の開始

- 段階 2 を d7adf47 としてコミットし、プッシュした。ユーザの指示で CI は見張らず、作業を休止した。再開のときに、ユーザが CI が通ったことを伝えた。
- 作業の環境のスクラッチパッドの場所が変わり、Qt 6 と Qt 5 のビルドのディレクトリを作り直した (Qt 6.4.2 と Qt 5.15.13、笛あり。警告なし)。
- ユーザの指示で段階 3 を始めた。段階の初めにユーザが決めた: D7 (印は `[` と `]`、消すのはメニューだけ)、D8 (得点の 10 秒前から 5 秒後、今の位置の 5 秒前から 5 秒後)、D9 (スライダーに得点の印も出す)。キーを選ぶために調べたこと: 1 文字のキーでは F、J、M、Q、W、Y と `[` `]` が空いていた (I と O は使われている)。

### 2026-11-24: 段階 3 の実装

- 4.12 のとおり実装した。テスト `export_range_list`、`log_player_slider` を足し、`export_range`、`export_dialog` に段階 3 の分を足した。
- 調べて計画を変えたこと: 区間を、プレイモードではなくサーバのパラメータのサイクルで分ける (4.12。rcssserver 19 はハーフの終わりに `first_half_over` を使わず、ログには `before_kick_off` も記録されていなかった)。
- テストで見つけて直したこと: スライダーの帯を描くとき、`initStyleOption()` は描く部分を空にするので溝が描かれなかった (描く部分を明示した)。`ServerParamT` はコピーできない (テストの補助関数を直した)。
- 笛のある Qt 6 (6.4.2) と Qt 5 (5.15.13)、`ENABLE_SOUND=OFF` の CMake のビルドで警告はなく、`ctest -j8` をそれぞれ 3 回走らせて、テスト 77 個がすべて通った。autotools (Qt 5) のビルドも、コンパイルの警告なしで通った。
- 実際の試合のログで、区間、得点、得点の場面の出力、ダイアログとスライダーを確かめた (4.12)。README、NEWS、ChangeLog を更新した。
- この後: ユーザが、実際のログで得点の場面をまとめて出せることと、スライダーの帯を確かめる (段階 3 の完了条件)。その後、コミットとプッシュ、CI。

### 2026-11-24: 得点の位置を DispHolder にそろえる (D10)

- ユーザが、`DispHolder` が持つ得点の番号を使わないと判断したのかを尋ねた。理由 (メニューにチームとスコアが要る、純粋な関数にしてテストする) と、調べたこと (`scoreChangedIndex()` は新しいスコアの直前のフレームを指し、`goals()` とは 1 フレームずれる。ログの再生の「次の得点へ」はその 50 フレーム前に移る) を伝え、A (今のまま) と B (`scoreChangedIndex()` にそろえる) を示した。ユーザが B を選んだ (D10)。
- `ExportRange::goals()` が番号の一覧を受け取るようにし、ダイアログとメインの窓は `scoreChangedIndex()` を渡す。テストを直した (4.12)。Qt 6 で、出力のテストがすべて通った。実際の 2 つのログで確かめた (4.12)。
- 笛のある Qt 6 と Qt 5、`ENABLE_SOUND=OFF` の CMake のビルドで警告はなく、`ctest -j8` をそれぞれ 3 回走らせて、テスト 77 個がすべて通った。autotools (Qt 5) のビルドも通った。
- この後: ユーザが段階 3 を確かめる (段階 3 の完了条件)。その後、コミットとプッシュ、CI。

### 2026-11-24: 段階 3 の確認と、メニューとダイアログの直し (D11、D12)

- ユーザが動作を確かめた (段階 3 の完了条件)。そのうえで、次を求めた: Export Range の印の付け方がメニューだけでは分かりにくく、メニューの名前からできることが想像しにくい。Now のボタンの簡易ヘルプにショートカットを書く。ダイアログにも範囲のクリアのボタンを付ける。
- ダイアログの「All」が今の範囲のクリアと同じ働きであることを伝え、ユーザが「Clear」に改めることを選んだ (D12)。メニューは、ユーザが 2 つの案 (メニューから印の項目を消してダイアログだけを入口にする、Export Images の中にダイアログ、範囲の始めと終わり、クリアをまとめる) を出し、Claude が後者を勧めた (D11)。
- 直したこと: メニュー (File > Export Images / Video の中に 4 つの項目、ツールチップつき)、ダイアログの Clear と Now のツールチップ、README のメニューの道筋。テスト `export_dialog` を、メニューの構成、項目の名前、Clear のボタン、Now のツールチップに合わせて直した。Qt 6 で出力のテストがすべて通った。メニューとダイアログを画像にして見た。
- 笛のある Qt 6 と Qt 5、`ENABLE_SOUND=OFF` の CMake のビルドで警告はなく、`ctest -j8` をそれぞれ 3 回走らせて、テスト 77 個がすべて通った。autotools (Qt 5) のビルドも通った。
- この後: ユーザがメニューとダイアログを確かめる。その後、コミットとプッシュ、CI。

### 2026-11-25: メニューの区切りと、段階 3 の確認の終わり

- ユーザがメニューとダイアログを確かめた。File のメニューの項目がすべて並んで読みにくかったので、ユーザが区切りを足した (開く、URL と一覧、コピーの保存、画像と出力、終了の間)。笛のある Qt 6 と Qt 5、`ENABLE_SOUND=OFF` の CMake のビルドで、テスト 77 個がすべて通った。
- この後: 段階 3 のコミットとプッシュ、CI。

### 2026-11-25: 段階 3 のコミットと CI

- 段階 3 を d7a5cd1 としてコミットし、プッシュした。CI の 5 つのジョブがすべて通った。
- この後: 段階 4 (ffmpeg による動画) を、始める前にユーザに確かめる (§3.3 の段階 4 の項目を決める)。

### 2026-11-25: 段階 4 の開始

- ユーザの指示で段階 4 を始めた。段階の初めにユーザが決めた: D13 (止めたときは動画のファイルを消す)、D14 (ダイアログの動画の設定は画質と速さだけ)、D15 (既定は CRF 18、preset medium)。ユーザは、mp4 のオプションを後で決めるのかを尋ねた。計画 (4.5) の案を示し、ダイアログの作りに効く 2 点 (D14、D15) をここで決めた。
- この作業の環境には ffmpeg と ffprobe が入っている (2.4 の時点ではなかった)。本物の ffmpeg のテストもここで走らせる。

### 2026-11-25: 段階 4 の実装

- §4.13 のとおり実装した。計画から決めたこと: ffmpeg は動画を選んだときに初めて調べる。動画の拡張子は `.mp4`、`.mov`、`.mkv` に限る。範囲ごとの動画の名前は `FILE-名前.ext` にする。進捗に残りの時間を出す (画像にも)。preset の欄の名前を Encoding speed にした (Rate の Speed と区別する)。CI の Qt6 のジョブで、本物の ffmpeg のテストを必ず走らせる。
- 確かめたこと: 笛のある Qt 6 と Qt 5、`ENABLE_SOUND=OFF` の CMake のビルドで、警告なしにビルドでき、テスト 78 個 (新しい `ffmpeg_sink` を含む。本物の ffmpeg のテストも走った) が 3 回ともすべて通った。autotools (Qt 5) でもビルドできた (警告は前からある `configure.ac` の 1 つだけ)。実際のログでの確かめは §4.13。
- この後: ユーザが画面で確かめる (動画の出力と再生、ffmpeg がないときの表示、中止)。その後、段階 4 のコミットとプッシュ、CI。

### 2026-11-25: 段階 4 の確認 (文字の拡大のチェック)

- ユーザが画面で確かめ、「Scale the texts with the image」を入れると、スコアボードや選手の文字だけでなく、チームのロゴも大きくなり、キックのときの白い円 (幅 2 画素のペン) も太くなることに気づき、仕様かを尋ねた。答え: 作りどおり (画像の dpr を「画像の高さ ÷ 2D 表示の高さ」にして、画面を拡大した見た目にするので、画面の画素の単位で大きさが決まるものはすべて同じ割合で大きくなる)。名前が文字だけを言っていて不正確だった。
- ユーザが A 案を選んだ (D16): 動きは変えず、名前を「Enlarge the texts, logos and lines with the image」に改め、ツールチップ、README、ソースのコメントを直した。
- ユーザが直した名前と説明を確かめ、コミットとプッシュを求めた。
- この後: 段階 4 のコミットとプッシュ、CI。

### 2026-11-25: 段階 4 のコミットと CI

- 段階 4 を 83ab35e としてコミットし、`feature/frame-export` にプッシュした (初めのプッシュはコマンドが止められて届かず、プッシュだけをやり直した)。
- CI はすべて通った (autotools Qt5、CMake の Qt6、Qt6 (Ubuntu 24.04)、Qt5、distcheck)。CMake の Qt6 のジョブでは ffmpeg が入り、`RCSSMONITOR_REQUIRE_FFMPEG=1` で `ffmpeg_sink` のテスト (本物の ffmpeg を含む) が通った。
- この後: 段階 5 (3D の描画の整理。リファクタだけ)。始める前にユーザに確かめる。

### 2026-11-25: 段階 5 の開始と実装

- ユーザの指示で段階 5 を始めた。段階の初めに決めることはなかった (§3.3)。
- §4.14 のとおり実装した。計画から決めたこと: ラベルとボールの補助はカメラが決まってから作るので `render()` に置く。FPS は画面だけのものなので `FieldView3D` に残し、その位置のために `minimapShown()` を足した。CPU の時間は描き手の中で測る (build の時間にカメラの更新を含まなくなった)。
- 作業中に、`DispHolder` のコンストラクタが `M_playmode` を初期化していないことを見つけて直した (§4.14)。
- 確かめたこと: 笛のある Qt 6 と Qt 5、`ENABLE_SOUND=OFF` の CMake のビルドで、警告なしにビルドでき、テスト 80 個 (新しい `view_3d_renderer` と `team_graphic_painter` を含む。3D のテストは飛ばされずに走った) が 3 回ともすべて通った。autotools (Qt 5。3D なし) でもビルドできた (警告は前からある `configure.ac` の 1 つだけ)。Xvfb の上で、変更の前と後の 3D の画面を画素で比べ、違いがなかった (§4.14)。
- ユーザが画面で 3D を確かめ、コミットとプッシュを求めた。ユーザは作業の環境を移るので、ここで作業を休む。CI の結果はユーザが後で見る。
- この後: 段階 5 の CI の結果を確かめ、進み具合の表に段階 5 のコミットのハッシュと CI を書く (次のコミットに含める)。その後、段階 6 (3D のオフスクリーンの描画。始める前にユーザに確かめる)。

### 2026-11-25: 段階 5 の CI と再開

- 段階 5 (ad3687b) の CI は、すべてのジョブで通った (autotools Qt5、CMake の Qt6、Qt6 (Ubuntu 24.04)、Qt5、distcheck)。ユーザが確かめて伝え、Claude も GitHub で確かめた。
- ユーザが作業の環境を移して、作業を再開した。
- この後: 段階 6 (3D のオフスクリーンの描画)。始める前にユーザに確かめる。

### 2026-11-25: 段階 6 の開始と実装

- ユーザの指示で段階 6 を始めた。段階の初めにユーザが決めた: D17 (出力のダイアログの 2D / 3D は、開いたときに画面に出ている表示に合わせる)。
- §4.15 のとおり実装した。計画から決めたこと: S10 (描き手の型 `ExportRenderer`)、S11 (範囲の始めはカメラの `snap()` だけ)、S12 (`QOpenGLFramebufferObject`。`gl/offscreen_target` は作らない)、S13 (メインの窓が描き手を 1 つ持つ)、S14 (fps は表示ごと、大きさは画面に出ている表示から)、S15 (dpr が 1 より小さいとき)。
- 作業中に見つけて直したこと: QPainter は 1 より小さい dpr を 1 として描くので、画面より小さい画像では配置の左上だけが写っていた。段階 1 からの 2D の不具合でもあった (S15)。
- 確かめたこと: 笛のある Qt 6 と Qt 5、`ENABLE_SOUND=OFF` の CMake のビルドで、警告なしにビルドでき、テスト 81 個 (新しい `offscreen_view_3d` を含む。3D のテストは飛ばされずに走った) が 3 回ともすべて通った。autotools (Qt 5。3D なし) でもビルドできた (警告は前からある `configure.ac` の 1 つだけ)。Xvfb の上での画面との比べ方と、実際のログでの出力は §4.15。
- ユーザが画面で確かめ、コミットメッセージを求めた。
- この後: 段階 6 のコミットとプッシュ、CI。

### 2026-11-25: 段階 6 のコミットと CI の失敗

- 段階 6 を 89b5168 としてコミットし、`feature/frame-export` にプッシュした。
- CI は、CMake の 4 つのジョブ (Qt6、Qt6 (Ubuntu 24.04)、Qt5、distcheck) で落ちた。autotools は通った。落ちたのは `offscreen_view_3d` の `sameInput` だけで、同じ入力の 2 つの描き手の 1 枚目の画像が同じでなかった。ほかの 3D のテスト (`rangeStart` も 2 つの描き手を色の差 2 までで比べる) は通った。
- ここの環境では、ハードウェアでも Mesa のソフトウェアの描画 (`LIBGL_ALWAYS_SOFTWARE=1`) でも、同じになり、再現しなかった。時計や乱数で絵が変わるところと、塗らずに使う画像は見つからなかった。
- 直し: `sameInput` と `rangeStart` は、2 つの画像を同じ関数で比べ、色の差 2 までを同じ絵とする (GPU の丸め)。画素まで同じでないときは、違う画素の数、最大の差、場所をログに出し、落ちたときもそれを出す。CI でも差が大きければ、そのログで調べる。
- この後: 直しのコミットとプッシュ、CI。
