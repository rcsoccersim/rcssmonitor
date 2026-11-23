# Playing Logs from URLs, and Log Lists (GitHub, archive): Plan

| Item | Content |
|---|---|
| Created | 2026-11-22 |
| Target | rcssmonitor develop (5affd90, 20.0.0-beta.1) |
| Branch | `feature/remote-logs` (from develop after the referee's whistle (F4) is merged; the user creates it on GitHub) |
| Status | Plan written (2026-11-22). Stage 1 (opening from URLs) started (2026-11-24, `feature/remote-logs`). Stage 3 (archive) is on hold until the site can be reached |
| Japanese version | [2026-11-22-02_remote-logs.md](2026-11-22-02_remote-logs.md) (same content) |
| Related | F5 of the overall plan [2026-11-22-00_v20-features_en.md](2026-11-22-00_v20-features_en.md). Reference implementation: `src/io/fetch-log.ts`, `src/ui/dialogs/open-url-dialog.ts` and `docs/protocol.md` of rcssmonitor-web |

## 1. Goal and approach

- Given a URL, download the rcg file and play it.
- Show the list of the logs in GitHub repositories, and play the one chosen.
- Also show the list of the logs on archive.robocup.info. However, the site cannot be reached as of 2026-11-22, so GitHub comes first and the archive later (the user, 2026-11-22).
- QtNetwork is already a required dependency (used for the UDP and TCP connections), so `QNetworkAccessManager` adds no new dependency.

## 2. Current state (develop 5affd90)

- Logs can only be opened from a file path. File > Open Game Log (`MainWindow::openGameLogFile()`, main_window.cpp:2145) → `openGameLogFile( const QString & )` (line 1759; requires `QFile::exists`) → `openGameLogFileImpl()` (line 1859) → `loadGameLogFile()` (`src/game_log_file.cpp`).
- The positional argument of the command line (options.cpp:1597) is taken as a local path and opened 100 ms after the start (main_window.cpp:191).
- Loading runs synchronously on the GUI thread. `RCGHandler` shows a progress dialog after 500 ms (with no cancel button).
- gzip is inflated in memory when libdeflate is available (`inflateWithLibdeflate`, in an anonymous namespace of game_log_file.cpp). zlib reads only from files by `gzopen(path)`. The parser itself can read from memory (`rcss/rcg/parser.h`).
- `QNetworkAccessManager` is used nowhere. The TLS plugin (OpenSSL) of Qt6 is available locally.
- There is no drag and drop onto the window (`dragEnterEvent`/`dropEvent` are commented out), and no list of recent files.

### 2.1 The external sites (checked on 2026-11-22)

**GitHub (`rcsoccersim/RoboCup2026`)**
- The default branch is `main`. `logs/Finals/`, `logs/GroupA/`, ... have `*.rcg.gz` and `*.rcl.gz`. Of the 313 entries, 144 have rcg in the name.
- `GET https://api.github.com/repos/rcsoccersim/RoboCup2026/git/trees/HEAD?recursive=1` (unauthenticated): 200, about 85 KB of JSON with all the files (`truncated` is false). `HEAD` can be used as `REF`. It has an `ETag`.
- The unauthenticated limit is 60 requests per hour (`x-ratelimit-limit: 60`). Even when `If-None-Match` got a 304, the remaining count went down.
- The files: `https://raw.githubusercontent.com/rcsoccersim/RoboCup2026/HEAD/logs/Finals/<name>.rcg.gz` returns 200 (`application/octet-stream`, about 4.8 MB). This repository does not use Git LFS (its `.gitattributes` only has `logs/ export-ignore`).
- File names: `YYYYMMDDhhmm-LEFTTEAM_SCORE-vs-RIGHTTEAM_SCORE.rcg.gz`. With a penalty shootout, `_SCORE_PKSCORE` (e.g. `202607050025-FRA-UNIted_0_4-vs-NexusPrime_0_2.rcg.gz`). Team names may contain `-` and `_` (e.g. `RoboTech_Coach_base_0`, `FC-AIT_2`).

**The archive**
- `archive.robocup.org` (the name the user first wrote; a mistake): HTTPS gives a TLS error (`tlsv1 unrecognized name`). HTTP gives the default page of Nginx Proxy Manager.
- `archive.robocup.info` (the right name): cannot be connected over either HTTPS or HTTP.

### 2.2 rcssmonitor-web's implementation (the source to port)

- `src/io/fetch-log.ts`: `parseLogUrl` (trims spaces, only `http`/`https`, rejects URLs with a user name or password), `githubRawUrl` (`github.com/OWNER/REPO/blob|raw/REF/PATH` → `raw.githubusercontent.com/OWNER/REPO/REF/PATH`), `githubLfsUrl` (→ `media.githubusercontent.com/media/OWNER/REPO/REF/PATH`), `lfsPointerSize` (under 1024 bytes, starting with `version https://git-lfs.github.com/spec/v1\n`, with a `size N` line), `logNameFromUrl`, and a download with progress and abort.
- `test/unit/fetch-log.test.ts`: unit tests of the functions above (to be ported).
- `docs/protocol.md`: a table of how each GitHub URL behaves.
- The browser's restrictions (CORS, mixed content) do not apply to a desktop application.

## 3. Decisions

### 3.1 The user's decisions (2026-11-22)

| # | Content |
|---|---|
| D1 | GitHub first, the archive later (the archive is `archive.robocup.info`; it cannot be reached now) |
| D2 | The GitHub lists include a default repository (`rcsoccersim/RoboCup2026`), and the user can add `owner/repo` in the settings |
| D3 | Downloaded logs are kept in the cache (under `QStandardPaths::CacheLocation`, e.g. `~/.cache/rcssmonitor/logs/`), one per URL. The same URL is reused if it was downloaded completely before. A way to clear the cache is provided (2026-11-24, at the start of stage 1) |
| D4 | Drag and drop onto the window opens both URLs and local files (`.rcg`, `.rcg.gz`) (same) |
| D5 | A way to save a copy of the file of the open log (File > Save a Copy of Game Log...) is added. It works for local logs too (same). It was named Save Game Log As... at first, but it copies the file, so the user chose Save a Copy of Game Log... (2026-11-24) |
| D6 | The design decisions S1 to S7 are as proposed. The download size limit (S4) is 200 MB (same) |
| D7 | The URL functions ported from the web version (global functions in `log_url.h` in stage 1) are put in classes: the static-only class `LogUrl` for reading URLs, which has no state, and the class `LogCache` (`log_cache.{h,cpp}`) for the cache, which has its directory. Not statics of `LogDownloader` (the dialog and the command line only check URLs, without depending on a `QObject` of Qt Network; the cache is not the download's job) (2026-11-24; decided by the user when stage 1 was revised) |
| D8 | The rules of the referee's whistle (F4; global functions in `whistle_rules.h`) are put in the static-only class `WhistleRules` too, in another commit on this branch (same) |

### 3.2 Design decisions (Claude; the user confirms them at the start of the stages)

| # | Content | Reason |
|---|---|---|
| S1 | A downloaded file is put on disk and opened by the existing `openGameLogFile( path )`. The parser and the loading are not changed | zlib can only read from files. Keep a single way of loading |
| S2 | Follow redirects with `QNetworkRequest::NoLessSafeRedirectPolicy` (set explicitly) | Qt5 does not follow them by default. A redirect from https to http is not followed |
| S3 | Reject URLs other than `http` and `https`, and URLs with a user name or password (as the web version) | Do not read unintended schemes |
| S4 | Limit the size of a download (the default value is decided in stage 1; a game is a few MB in gzip) | Do not read a huge file from a wrong URL |
| S5 | The `User-Agent` of the requests to GitHub is `rcssmonitor/VERSION`, with no personal information | GitHub requires a `User-Agent` |
| S6 | The GitHub lists are cached on disk and fetched again only on the user's "Reload" or after some time (e.g. one hour) | 60 requests per hour unauthenticated, and even a 304 counts |
| S7 | A downloaded file is untrusted input for the parser, so tests check that broken files do not crash it (the parser was hardened in Phase 2 of the 3D view plan) | Files from outside are read |

### 3.3 To decide at the start of the stages

- **Where downloads are kept (stage 1)**. Recommended: under `QStandardPaths::CacheLocation` (e.g. `~/.cache/rcssmonitor/logs/`), one per URL. The same URL is reused if it was downloaded completely before. A way to clear the cache (a button in the dialog) is provided. The other option: a temporary file deleted when closed (no reuse).
- **Drag and drop of local files (stage 1)**: whether to add dropping local files along with dropping URLs.
- **Saving a downloaded log (stage 1)**: whether to provide "Save as" (if the cache is chosen, whether opening its folder is enough).
- **Columns and filter of the list (stage 2)**: date and time, teams, scores, size, directory. Filter by team name.
- **Default repositories (stage 2)**: whether to add others than `rcsoccersim/RoboCup2026` (as of 2026-11-22, RoboCup2026 is the only repository of the rcsoccersim organization whose name starts with RoboCup).

## 4. Design

### 4.1 Stage 1: open from a URL

- Menu File > Open Game Log from URL...: a dialog to enter a URL, with a combo box of the recent URLs (saved in the settings). GitHub file page URLs are accepted.
- Command line: a positional argument starting with `http://` or `https://` is opened as a URL (`rcssmonitor https://…/game.rcg.gz`).
- Dropping a URL onto the window (`setAcceptDrops`, `dragEnterEvent`, `dropEvent`).
- `src/log_url.{h,cpp}` (new): port `parseLogUrl`, `githubRawUrl`, `githubLfsUrl`, `lfsPointerSize` and `logNameFromUrl` of the web version to C++ (QUrl), as pure functions with unit tests.
- `src/log_downloader.{h,cpp}` (new): a `QObject` that downloads one URL with `QNetworkAccessManager`.
  - Progress (`downloadProgress`), abort (`QNetworkReply::abort`), HTTP errors (status code and reason), network errors, TLS errors.
  - If the first response is an LFS pointer, read it again from media.githubusercontent.com (using the pointer's `size` as the total of the progress).
  - The size limit (S4). If `Content-Length` exceeds it, stop before reading.
  - If `QSslSocket::supportsSsl()` is false, an https URL shows why TLS cannot be used and what to do (how to install OpenSSL).
  - Write to a temporary name, and rename it at the end (an interrupted file is never reused).
- `MainWindow`: shows a `QProgressDialog` (with a cancel button) during the download. When done, it opens the file by `openGameLogFile( path )` and shows the log's name (`logNameFromUrl`) in the window title. Opening another log or connecting to a server stops the download.

### 4.2 Stage 2: GitHub log lists

- Menu File > Browse Game Logs...: a dialog with the list of logs (`src/log_browser_dialog.{h,cpp}`, new).
  - On the left, the repositories (default `rcsoccersim/RoboCup2026`; the user adds `owner/repo` or `owner/repo@ref`, and removes them; saved in the settings).
  - On the right, the tree of logs: grouped by directory (Finals, GroupA, ...), with the columns date and time, left team, scores, right team, size. A field to filter by team name.
  - Double-click or "Open" passes it to the download of stage 1.
  - "Reload" fetches the list again.
- `src/github_log_index.{h,cpp}` (new):
  - `GET https://api.github.com/repos/OWNER/REPO/git/trees/REF?recursive=1` (`REF` defaults to `HEAD`), `Accept: application/vnd.github+json`.
  - From the JSON of the response (`QJsonDocument`, in QtCore), take the entries whose `type` is `blob` and whose name ends with `.rcg` or `.rcg.gz` (path, size). If `truncated` is true, show that the list is incomplete.
  - The URL of a file is `https://raw.githubusercontent.com/OWNER/REPO/REF/PATH`.
  - Cache of the lists (S6): the JSON and the time it was fetched, under `CacheLocation`.
  - Errors: on 403/429 with `x-ratelimit-remaining: 0`, show when the limit is lifted, from `x-ratelimit-reset`. 404 means no such repository or branch.
- `src/log_file_name.{h,cpp}` (new): parsing of file names (pure functions).
  - Split into the date and time and the two sides by `^(\d{12})-(.+)-vs-(.+)\.rcg(\.gz)?$`, then each side into the team name, the score and the penalty score by `^(.*?)_(\d+)(?:_(\d+))?$` (the shortest team name).
  - Examples: `FRA-UNIted_0_4` → `FRA-UNIted`, 0, 4. `RoboTech_Coach_base_0` → `RoboTech_Coach_base`, 0.
  - If it cannot be parsed, show the name as is.

### 4.3 Stage 3: archive.robocup.info (on hold)

- Starts when the site can be reached. Until then, the status of this document says "on hold".
- Make the way to get the list a generic source of "directory listings" (the autoindex HTML of Apache or nginx), as one of the sources of the stage 2 dialog (the same interface as the GitHub source).
- If the logs are bundled in `.tar.gz` etc., decide then how to extract them (our own tar reader, or another dependency).
- Before starting, check whether it is public, the form of the URLs, and the rules for automated access (robots.txt etc.).

### 4.4 The implementation of stage 1 (2026-11-24)

4.1 was implemented as follows. What was decided beyond the plan is written too.

- `src/log_url.{h,cpp}` (new; QtCore only): the static-only class `LogUrl`, which has no state (D7). `parseLogUrl` of the web version was ported as `LogUrl::parse` (`QUrl` in StrictMode; http and https only, a host needed, a URL with a user name or a password refused; the reason of an error returned), `githubRawUrl`, `githubLfsUrl` and `lfsPointerSize` under the same names, and `logNameFromUrl` as `LogUrl::name`. Added: `LogUrl::isUrl` (telling a URL from a local path on the command line and in a drop).
- `src/log_cache.{h,cpp}` (new; QtCore only): the class `LogCache`, which has its directory (D7). `pathOf` (`DIR/KEY/NAME` in the cache; KEY is the first 16 hex digits of the SHA-1 of the URL, NAME the log's name made safe for a file name; the file has the log's name, used as it is by the window's title and Save a Copy), `contains`, `size`, `clear` and `defaultDir` (`QStandardPaths::CacheLocation/logs`, `~/.cache/rcssmonitor/logs`). The main window has a `LogCache` of the default place, and gives it to the dialog.
- In the first version of stage 1 (d56fad4), these were global functions in `log_url.h`. At the user's remark, they were made into the two classes above (D7).
- `src/log_downloader.{h,cpp}` (new): `LogDownloader` downloads one URL by `QNetworkAccessManager`.
  - Written through `QSaveFile` (the plan's "written under a temporary name, then renamed", by Qt's own means; two rcssmonitors downloading the same URL at once do not mix).
  - Redirects by `NoLessSafeRedirectPolicy` (S2), at most 10. `User-Agent: rcssmonitor/VERSION` (S5).
  - The size limit of 200 MB (S4, D6): stopped before the body if `Content-Length` is over it, or when that much has been read without it. The content of an LFS pointer whose `size` is over it is not read.
  - A transfer timeout of 30 s without data (`setTransferTimeout`; Qt 5.15 or later), told as such.
  - An LFS pointer: read again from `githubLfsUrl` of `reply->url()` (the URL after the redirects), with the pointer's `size` as the total of the progress. Where it goes can be replaced for the tests (`setLfsResolver`).
  - The progress is told each time the body is read (`QNetworkReply::downloadProgress` was not emitted each time data came).
  - https without TLS (`QSslSocket::supportsSsl()`): told to install OpenSSL or to use http.
  - A failure is told after `start()` returns, and not after `abort()` or the next `start()`.
- `src/open_url_dialog.{h,cpp}` (new): the dialog of File > Open Game Log from URL. The recent URLs (an editable combo box), why the text is not a URL (in red in the dialog, which stays open), where the cache is (the home as `~`) and its size, and Clear Cache.
- `MainWindow`:
  - "Open Game Log from URL..." (Ctrl+Shift+O) and "Save a Copy of Game Log..." (Ctrl+Shift+S) in the File menu.
  - A URL is opened from the cache if it is there; otherwise it is downloaded with a `QProgressDialog` (with Cancel; how many MB of how many, when the size is known), and then opened by `openGameLogFile( path )` (S1). A failure is told in a message box with its reason (and the program ends with `--auto-quit-mode`). The progress dialog is not modal (a modal `QProgressDialog` processes the events in `setValue()`, where the download ended and closed the dialog, and the call after it crashed). Opening the log and telling a failure are done after the call of `LogDownloader` returns (`QTimer::singleShot( 0 )`; reading a log and a message box both process the events). Opening another log, connecting to a server or opening the next URL stops the download.
  - A positional argument of the command line that starts with http:// or https:// is opened as a URL.
  - Drops (D4): the first URL; a local file is opened, an http or https URL downloaded. A dropped text that is a URL is taken too. A local file is taken whatever its extension (a file that cannot be opened is told as when chosen in the file dialog). The 3D view is a window of its own (`QOpenGLWindow`), so its drags and drops go to the same handling through the main window's event filter.
  - Saving a copy (D5): the file of the log open is copied as it is (compressed if it is). Enabled when a log is opened, and disabled when connected to a server. If the file is not there any more (the cache cleared), that is told.
- `Options`: the recent URLs (at most 10; `recent_urls` in `[GameLog]` of the settings), written as soon as a URL is opened (`writeRecentLogUrls()`; not at the end).
- The tests' HOME has the cache too (`XDG_CACHE_HOME`), so that they never touch the user's cache.

## 5. Stages

| # | Work | Completion criteria |
|---|---|---|
| 1 | Open from a URL (4.1). Decide the stage 1 items of §3.3 | Unit tests and the CI pass. The user checks that real GitHub URLs (file pages, raw links, LFS files) and URLs of other servers can be opened |
| 2 | GitHub log lists (4.2) | Unit tests and the CI pass. The user checks opening from the list and the message when the limit is hit |
| 3 | Archive lists (4.3; on hold) | Decided when the site can be reached |

## 6. Tests

- `test_log_url`: parsing URLs (port the web version's `fetch-log.test.ts`): GitHub blob, raw and raw.githubusercontent.com, other hosts, relative URLs, rejecting `ftp:` etc., rejecting URLs with a user name, taking the name, detecting LFS pointers.
- `test_log_downloader`: a small HTTP server with `QTcpServer` in the test; success, redirects (http → http; https → http is not followed), 404, abort in the middle, the size limit, LFS pointer → the real file. The CI uses no external network.
- `test_github_log_index`: parsing a tree JSON in `test/data` (a part of RoboCup2026's), `truncated`, parsing the rate-limit response.
- `test_log_file_name`: parsing file names (penalty scores, `-` and `_` in team names, names that cannot be parsed).
- Downloading and opening a broken file (not gzip, truncated, not an rcg) does not crash.

## 7. Files to change (expected)

| File | Content |
|---|---|
| `src/log_url.{h,cpp}`, `src/log_cache.{h,cpp}`, `src/log_downloader.{h,cpp}`, `src/open_url_dialog.{h,cpp}` (new) | Stage 1 |
| `src/github_log_index.{h,cpp}`, `src/log_file_name.{h,cpp}`, `src/log_browser_dialog.{h,cpp}` (new) | Stage 2 |
| `src/main_window.{h,cpp}` | Menus, dropping, connecting the download to opening |
| `src/options.{h,cpp}` | Recent URLs, the repository list, cache settings, the command line |
| `src/CMakeLists.txt`, `src/Makefile.am` | Sources (the moc list of autotools is written by hand) |
| `test/` | §6 |
| README, NEWS, `docs/build.md` (the run-time requirement of TLS) | Documents |

## 8. Out of scope

- Authentication by a GitHub token (it raises the limit to 5000 per hour, but the token would have to be stored).
- Private repositories.
- Playing while downloading (streaming).
- Downloading rcl (command logs).

## 9. Record

### 2026-11-22: Writing the plan

- Written together with the overall plan (2026-11-22-00_v20-features_en.md). The user's decisions D1 and D2 were given.
- The state of the external sites in §2.1 was checked on this day with curl and the GitHub API.

### 2026-11-24: Stage 1 started

- The user made `feature/remote-logs` from develop (6b3b4f8) after the referee's whistle (F4) was merged.
- Decided by the user at the start of the stage: D3 (kept in the cache), D4 (dropping URLs and local files), D5 (Save As), D6 (S1 to S7 as proposed; the size limit is 200 MB).
- The source to port (`src/io/fetch-log.ts` and `test/unit/fetch-log.test.ts` of rcssmonitor-web, 8c2b375) is in `../rcssmonitor-web` here.

### 2026-11-24: Stage 1 implemented

- Stage 1 was implemented (4.4). Tests: `test_log_url` (ported from `fetch-log.test.ts` of the web version; telling URLs on the command line; the cache's paths, size and clearing), `test_log_downloader` (with a small HTTP server in the test, `test/http_test_server.h`: success and progress, replacing, redirects, 404, no server, abort, the next start, the size limit (by the length, while reading), LFS (the content, the limit, not GitHub), the timeout, a place not writable), `test_remote_log` (in the main window: what a drop has, dropping a file, opening a URL and reusing the cache, a URL on the command line, Cancel, telling a failure, Save As, the dialog, the recent URLs).
- Checked (Ubuntu 24.04):
  - The three CMake builds, Qt 6.4.2, Qt 5.15.13 (the system's Qt5) and Qt6 without the whistle, had no warnings, and `ctest -j8`, run 3 times in each, passed all 64 tests. The autotools build (Qt5) passed without warnings too.
  - From the real GitHub (a public repository; only the User-Agent is sent): a log of RoboCup2026 (4.8 MB) was downloaded from its file page (Qt6) and its raw link (Qt5), the same as the local copy of that log (https with TLS worked).
  - Under Xvfb, the 3D view was shown, a drop was sent to its window, and the log opened (the offscreen tests cannot make the 3D view, so this is not in ctest).
  - The File menu and the URL dialog were saved as images offscreen and looked at. A long path of the cache cut the last line, so the dialog now grows with its content, and shows the home as `~`.
- Documents: a section "Opening a game log from a URL" in README, OpenSSL (at run time) and a note in `docs/build.md`, and items in NEWS and ChangeLog.
- Next: the user checks opening real GitHub URLs (a file page, a raw link, a file of LFS) and URLs of other servers, drops (2D and 3D), Save As and the cache (the completion criteria of stage 1). Then the commit and push, and the CI.

### 2026-11-24: Stage 1 revised (after the user checked)

- The user checked: opening the example URL crashed with Segmentation Fault. After starting again, the same URL (in the cache) opened. After Clear Cache, it crashed again. Drops from the file manager and the browser worked, but a drop from the browser crashed too at the first download. Save Game Log As saved the file. Its name: it copies rather than saves, so "copy" fits better.
- The cause: the progress dialog was modal (`Qt::WindowModal`). A modal `QProgressDialog` processes the events in `setValue()`. When the end of the download came there, the dialog was closed (`M_download_dialog` cleared) and the log opened, and `setLabelText()` after `setValue()` returned used the null pointer and crashed. Opening from the cache shows no progress, so it did not crash. The tests missed it, as the log was small and no progress came in the middle. A test that serves the log in two parts (`openUrlInParts`) reproduced it (Segmentation Fault all 3 times).
- Fixed: the dialog is not modal; the text of the progress is set first and `setValue()` last, with the dialog watched by `QPointer`; opening the downloaded log and telling a failure are done after the call of `LogDownloader` returns (4.4). The new test crashed before the fix and passes after it.
- The name: the user chose "Save a Copy of Game Log..." (D5). The menu, the function (`saveGameLogCopy`), the messages, the tests, README, NEWS and ChangeLog were changed.
- Checked: the three CMake builds had no warnings, and `ctest -j8`, run 3 times in each, passed all 64 tests. The autotools build (Qt5) passed without warnings too.
- Next: the user checks again (the completion criteria of stage 1). Then the commit and push, and the CI.

### 2026-11-24: Deciding to revise the design

- The user pointed out that the functions ported from the web version are global functions, which feels odd as C++ code, and asked whether they can be put in classes (statics of `LogDownloader`, perhaps). The functions were sorted by their roles and where they are used, and two classes were proposed: `LogUrl` (reading URLs) and `LogCache` (the cache). The user decided D7 and D8.
- The order of the commits (decided by the user): the present state (the crash fixed, all the tests passing) is committed and pushed first, and the revision is the next commit.

### 2026-11-24: The design revised (D7)

- The first version of stage 1 was committed as d56fad4 and pushed.
- The URL functions were made into the static-only class `LogUrl` (`log_url.{h,cpp}`) and the class `LogCache` (`log_cache.{h,cpp}`, new), which has its directory (4.4). The main window has a `LogCache` (`gameLogCache()`), and gives it to the dialog. `MainWindow::gameLogCacheDir()` was removed. The tests were split into `test_log_url` (`LogUrl`) and the new `test_log_cache` (the place, `pathOf`, `contains`, the size and clearing). The behavior is not changed.
- Checked: the three CMake builds had no warnings, and `ctest -j8`, run 3 times in each, passed all 65 tests. autotools (Qt5): the builds with the whistle and with `--disable-sound` passed (no warnings).
- The user checked with the revised version that the crash at the first opening is fixed.

### 2026-11-24: The class of the whistle's rules (D8)

- After the first commit ce0870e (D7), the rules of the referee's whistle (global functions in `whistle_rules.h`) were put in the static-only class `WhistleRules` (`whistleFor` → `WhistleRules::whistle`, `isWhistleStopMode` → `isStopMode`, `strongerWhistle` → `stronger`, `whistleAtFirstFrame` → `atFirstFrame`). The rules are not changed. The name in C.3.4 and the record in §6 of the overall plan were changed too.
- Next: the push and the CI. Then stage 2 (the lists of logs on GitHub), confirmed with the user before it starts.

### 2026-11-24: The timeout with Qt 6.10 of the CI

- In the CI of d56fad4, two jobs failed: CMake with Qt6, and the distcheck (Qt6). Only `timeout()` of `test_log_downloader` failed; Qt5, Qt6 (Ubuntu 22.04, Qt 6.2) and autotools passed. The CI's Qt6 is Qt 6.10.2 (Ubuntu 26.04). Qt 6.4 and Qt 5.15 here tell the transfer timeout as `OperationCanceledError` (Operation canceled), but Qt 6.10 as `TimeoutError` (Operation timed out), so `LogDownloader` did not tell it was the timeout, and the message was not "No data came for ... seconds.".
- Fixed: `TimeoutError` is the timeout too (`LogDownloader::describe()`). The tests pass with Qt6 and Qt5 here. Qt 6.10 is checked by the CI.

### 2026-11-24: Replacing the CI job of Ubuntu 22.04 with 24.04

- At the user's request, the CI job of Ubuntu 22.04 (which checked Qt 6.2 as the oldest Qt supported) is removed. Without it, the CI's Qt6 would be only Qt 6.10 of Ubuntu 26.04, so replacing it with a container of 24.04 was proposed, and the user chose the replacement (the job is named "CMake, Qt6 (Ubuntu 24.04)", with the packages of 24.04). The user decided to raise the oldest Qt 6 to 6.4 (Ubuntu 24.04; the oldest Qt6 checked here), in another commit on this branch before stage 2 (U20 of the overall plan). `QT_MIN_VERSION` of `CMakeLists.txt`, the version and the row of Ubuntu 22.04 in README and `docs/build.md`, and NEWS were changed. CMake configures with Qt 6.4.2 and Qt 5.15.13 here.
