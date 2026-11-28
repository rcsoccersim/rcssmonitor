# 得点の場面などの秒数を再生の秒で数える 計画

| 項目 | 内容 |
|---|---|
| 作成日 | 2026-11-28 |
| 最終更新日 | 2026-11-28 |
| 対象 | rcssmonitor develop (f2898ed。出力の機能 (https://github.com/rcsoccersim/rcssmonitor/pull/76) を取り込んだ後) |
| ブランチ | `fix/goal-scene-length` (develop の f2898ed から。ユーザが GitHub 上で作る) |
| 計画 ID | MON-01 |
| 計画 Issue | https://github.com/rcsoccersim/rcssmonitor/issues/77 |
| 英語版 | [2026-11-28-00_goal-scene-length_en.md](2026-11-28-00_goal-scene-length_en.md) (内容は同じ) |
| 関連 | 出力の計画 [2026-11-22-03_frame-export.md](2026-11-22-03_frame-export.md) (§4.3 時間の模型、§4.4 出力する範囲、D8、D10)。soccerwindow2 への出力の機能の移植 (この不具合を見つけたところ) |

## Phase と Issue

| Phase | 内容 | 実装の記録 | Issue |
|---|---|---|---|
| 1 | 再生の秒で数える範囲 (§4.1。D1、D2、S1〜S8) | — | https://github.com/rcsoccersim/rcssmonitor/issues/78 |

## 1. 目的と方針

- 出力のダイアログのよく使う範囲「得点の場面」と「今の位置の前後」、コマンドラインの `--export-range goals[:BEFORE:AFTER]` の秒数を、サイクルではなく再生の秒 (フレームの数) で数える。
- 出力の時間の模型 (出力の計画 §4.3。1 倍速で 1 フレームが 1 コマ) に合わせ、止まっているフレームも 1 ステップと数える。既定の秒数 (得点の 10 秒前から 5 秒後) の得点の場面が、1 倍速の動画で 15 秒になる。

## 2. 現状 (develop f2898ed)

- `ExportRange::around( frames, cycle, before_s, after_s, step_ms )` (`src/export_range.cpp`) は、中心のサイクルから `cyclesOf()` のサイクルを引き、また足して、`startIndex()` と `endIndex()` でフレームの番号に直す。終わりのサイクルの止まっているフレームはすべて含む。止まっているフレームは秒に数えない。
- 呼び出し元は 4 つ。
  - `ExportDialog::chooseGoal()` と `chooseAllGoals()`: 得点の `ExportGoal::cycle` の前後
  - `ExportDialog::chooseAround()`: 表示中のフレームのサイクルの前後
  - `ExportBatch::resolveRanges()` の `goals[:BEFORE:AFTER]`: 得点の `ExportGoal::cycle` の前後
- `ExportGoal::index` は `DispHolder::scoreChangedIndex()` の番号 (新しいスコアの直前のフレーム。出力の計画の D10)、`ExportGoal::cycle` はその次のフレームのサイクル。
- 得点の後、キックオフまでに止まっているフレームが 50 ある。2023 年のログ `202307090956-Oxsy_1-vs-YuShan2023_3.rcg.gz` の最初の得点では、`goal_r` のサイクル 437 に 51 フレーム (437、437.1〜437.50) があり、438 で `kick_off_l` になる。
  - 今の数え方では、「得点の 5 秒後」は 487 (キックオフの 5 秒後) までになる。得点の場面は 337〜487 の 201 フレーム (得点の前の 100、得点の 437、止まっている 50、キックオフの後の 50) で、10 枚/秒の動画では得点の後が 10 秒になる。
  - `docs/export.md` の例 (`goal1-c437: 337 - 487 (201 frames)`、4 つの得点で 804 枚) も、この数え方による。
- 出力の時間の模型 `ExportTiming` は、1 倍速で 1 フレームを 1 コマにする (2D の 10 枚/秒)。止まっているフレームもコマになる (止まっているフレームを飛ばす出力は TODO の T3)。

## 3. 決定事項

### 3.1 ユーザの決定

| # | 決定 | 決めた日 | いつ |
|---|---|---|---|
| D1 | 得点の場面と今の位置の前後の秒数は、再生の秒で数える。中心のフレームの前と後のフレームを、その数 (秒 × 1000 / ステップ) で取り、止まっているフレームも 1 ステップと数える (出力のコマ `ExportTiming` の 1 倍速と同じ)。得点の場面は得点のフレーム (新しいスコアの最初のフレーム) の前後、今の位置の前後は表示中のフレームの前後 | 2026-11-28 | 不具合の報告 (計画の作成の前) |
| D2 | 既定の秒数 (得点の 10 秒前から 5 秒後、今の位置の 5 秒前から 5 秒後。出力の計画の D8) は変えない。既定の得点の場面は 151 フレームになり、キックオフの前の最後の止まっているフレーム (2023 年のログの最初の得点では 437.50) で終わる | 2026-11-28 | 同上 |

### 3.2 設計上の判断 (Claude)

| # | 判断 | 理由 |
|---|---|---|
| S1 | `ExportRange::around()` は、中心をサイクルではなくフレームの番号で受ける: `around( frames, center, before_s, after_s, step_ms )`。中心の前後のフレームの数だけ取り、ログの端で切る。中心がログの外なら無効な範囲を返す | フレームを数えるので、番号で受けるのが素直。どのフレームを中心にするかは呼び出し元が知っている |
| S2 | 前と後のフレームの数は `cyclesOf()` (秒 × 1000 / ステップを丸める) で求め、コメントに「1 倍速のフレームの数でもある」と書く | 同じ式なので、関数を増やさない |
| S3 | 得点のフレームを `ExportGoal` の新しいメンバー `frame` に持つ。`index` の次のフレーム (`cycle` のフレーム) で、`index` がログの最後のフレームなら `index` 自身。`goals()` で決める | `cycle` と同じフレームを指し、範囲の名前 (`goal1-c437`) と合う。得点のフレームの決め方を 1 か所にする (出力の計画の D10 と同じ考え) |
| S4 | 今の位置の前後は、`DispHolder::currentIndex()` のフレームの前後にする。範囲の名前は今のまま `around-c<サイクル>` | 止まっているフレームを表示していても、そのフレームが中心になる |
| S5 | ライブで欠けたサイクルのあるログでも、フレームの数で数える | 出力のコマと、ログの再生 (1 フレームずつ進む) に合う |
| S6 | 設定のキー (`goal_before`、`goal_after`、`around_before`、`around_after`) と既定の値は変えない。値の意味が再生の秒になる | 値はそのまま使え、設定の移行は要らない |
| S7 | `docs/export.md` には、秒が再生の秒であること (止まっているフレームも数えること) を書き、一覧の例とバッチの標準エラーの例を、実際のログで出し直した値に直す。大きさの表の「a goal scene (20 s)」は、測った値を残し、「a scene of 20 s」に直す | 既定の得点の場面は 15 秒になる。表の値は 20 秒の場面を測ったもので、そのまま正しい |
| S8 | 出力の計画書 (2026-11-22-03_frame-export.md) は直さない。`around()` などのソースのコメントは、この計画書を指す | 出力の計画書は記録として残す。D8 の秒数の値は変わらない |

## 4. 設計

### 4.1 Phase 1: 再生の秒で数える範囲

#### 4.1.1 設計

**`ExportRange::around()` (S1、S2)**

```
n_before = cyclesOf( before_s, step_ms )
n_after  = cyclesOf( after_s, step_ms )
first = center − min( center, n_before )
last  = min( frames.size() − 1, center + n_after )
```

- フレームが空か、`center >= frames.size()` なら無効な範囲を返す。
- ヘッダのコメントを、「サイクルの前後」から「フレームの前後 (再生の秒。止まっているフレームも 1 ステップ)」に直す。

**`ExportGoal::frame` (S3)**

- `goals()` で、`after` (今の `frames[min( index + 1, size − 1 )]`) の番号を `frame` に入れる。

**呼び出し元**

| 呼び出し元 | 今の中心 | 直した後の中心 |
|---|---|---|
| `ExportDialog::chooseGoal()`、`chooseAllGoals()` | `goal.cycle` | `goal.frame` |
| `ExportDialog::chooseAround()` | 表示中のフレームのサイクル | `M_disp_holder.currentIndex()` (S4) |
| `ExportBatch::resolveRanges()` の `goals[:BEFORE:AFTER]` | `goals[i].cycle` | `goals[i].frame` |

- `--export-range` のヘルプに、秒が再生の秒であること (止まっているフレームも数えること) を足す。

**2023 年のログの最初の得点 (437) で、既定の秒数のとき**

| | 今 | 直した後 |
|---|---|---|
| 得点の場面 | 337〜487 の 201 フレーム | 337〜437.50 の 151 フレーム |
| 1 倍速の動画 (10 枚/秒) | 20.1 秒 | 15.1 秒 |

**`docs/export.md` (S7)**

- Presets の「Goal N」「Around the position shown」と、`--export-range` の `goals`、`goals:BEFORE:AFTER` の行に、秒が再生の秒であることを書く。既定の得点の場面がキックオフの前で終わることも書く。
- 一覧の例 (`goal1-c437: 337 - 487 (201 frames)`) と、バッチの標準エラーの例 (804 枚) を、直した後に実際のログで出した値にする。
- 大きさの表の「a goal scene (20 s)」を「a scene of 20 s」にする。

## 5. Phase

| # | 作業 | 完了条件 |
|---|---|---|
| 1 | `ExportRange::around()` をフレームの番号で数え、`ExportGoal::frame` を足し、ダイアログのよく使う範囲と `goals[:BEFORE:AFTER]` を直す。テスト、`--export-range` のヘルプ、`docs/export.md` を直す | Qt6 と Qt5 のビルドですべてのテストが通る。2023 年のログで、既定の得点の場面が 151 フレーム (最初の得点は 337〜437.50) になる (ダイアログとコマンドライン)。CI が通る |

## 6. テスト

- `test_export_range`
  - `around()`: 中心の番号の前後を、フレームの数で取る。中心の後に止まっているフレームがあると、そのぶん終わりのサイクルが早くなる。ログの端で切る。ステップが 50 ms なら 2 倍のフレームを取る。フレームが空のときと、中心がログの外のときは無効。
  - `goals()`: `frame` が `index` の次のフレームになる。`index` が最後のフレームなら `index` 自身。
- `test_export_dialog` の `presets`: 得点の場面、すべての得点、今の位置の前後の期待値を、フレームの数で数えた値に直す。得点の後に止まっているフレームがある試合で、既定の秒数の得点の場面がキックオフの前で終わることを確かめる。
- `test_export_batch` の `ranges`: `goals` と `goals:BEFORE:AFTER` の範囲を、フレームの数で確かめる。
- 実際の動作: 2023 年のログで、コマンドラインの `--export-range goals` が 151 フレームの範囲を 4 つ作ること、ダイアログの Presets の「Goal 1」が `337 - 437.50 (151 frames)` になることを確かめる。

## 7. 変更するファイル (見込み)

| ファイル | 変更 |
|---|---|
| `src/export_range.h`、`src/export_range.cpp` | `around()` をフレームの番号で数える。`ExportGoal::frame` |
| `src/export_dialog.cpp` | 得点の場面、すべての得点、今の位置の前後の中心 |
| `src/export_batch.cpp` | `goals[:BEFORE:AFTER]` の中心、`--export-range` のヘルプ |
| `test/test_export_range.cpp`、`test/test_export_dialog.cpp`、`test/test_export_batch.cpp` | §6 |
| `docs/export.md` | §4.1.1 |
| `docs/plan/2026-11-28-00_goal-scene-length.md`、`_en.md` | 実装の記録 |

## 8. 対象外

- 止まっているフレームを飛ばす出力 (TODO の T3 のまま)。

## 9. 記録

### 2026-11-28: 計画の作成

- soccerwindow2 への出力の機能の移植の際に見つけた不具合の報告 (得点の場面と今の位置の前後の秒数がサイクルで数えられ、止まっているフレームが数えられないため、得点の場面が思ったより長くなる) をもとに、計画を作った。報告の変更の案を D1、D2 とした。
- 2023 年のログで、得点 (437) のサイクルに 51 フレームがあり、438 でキックオフになることを確かめた (§2)。
- この後: 計画の確認を受けて計画 ID を振り、計画 Issue と Phase 1 のサブ Issue を作る。

### 2026-11-28: 計画の確定

- ユーザが計画書を確認した。ユーザが develop (f2898ed) から `fix/goal-scene-length` を作った。
- 計画 ID を MON-01 とし (このリポジトリの最初の計画 Issue)、計画 Issue https://github.com/rcsoccersim/rcssmonitor/issues/77 と、Phase 1 のサブ Issue https://github.com/rcsoccersim/rcssmonitor/issues/78 を作った。
- この後: ユーザの確認を受けて Phase 1 を始める。
