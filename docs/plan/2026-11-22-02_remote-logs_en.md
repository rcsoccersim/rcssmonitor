# Playing Logs from URLs, and Log Lists (GitHub, archive): Plan

| Item | Content |
|---|---|
| Created | 2026-11-22 |
| Last updated | 2026-11-24 |
| Target | rcssmonitor develop (5affd90, 20.0.0-beta.1) |
| Branch | `feature/remote-logs` (from develop after the referee's whistle (F4) is merged; the user creates it on GitHub) |
| Status | Plan written (2026-11-22). Stages 1 to 5 are done on `feature/remote-logs` (2026-11-24). The progress of each stage is in the table of "Progress" below |
| Japanese version | [2026-11-22-02_remote-logs.md](2026-11-22-02_remote-logs.md) (same content) |
| Related | F5 of the overall plan [2026-11-22-00_v20-features_en.md](2026-11-22-00_v20-features_en.md). Reference implementation: `src/io/fetch-log.ts`, `src/ui/dialogs/open-url-dialog.ts` and `docs/protocol.md` of rcssmonitor-web |

## Progress

The work and the conditions of the end of each stage are in §5, and the history in §9.

| Stage | Content | Status | Checked by the user | CI | Commits | Pushed |
|---|---|---|---|---|---|---|
| 1 | Opening from URLs, drops, saving a copy (4.1; D3 to D8) | Done | Done | Passed | d56fad4, ce0870e (D7), f1bccac (D8) | Done |
| — | The CI: the timeout of Qt 6.10, Ubuntu 24.04 and Qt 6.4 (U20 of the overall plan) | Done | — | Passed | 4d08f2b, 26b4935 | Done |
| 2 | The lists of logs on GitHub (4.2; D9 to D11) | Done | Done | Passed | ec341e2 | Done |
| 3 | The list of the archive, and editing the sources (4.3; D12, D13) | Done | Done | Passed | 42e08a1 (the test environment: bec0f96) | Done |
| 4 | Filters of the list of logs (4.4; D14) | Done | Done | Passed | 96eb9a2 | Done |
| 5 | Lists of files of web servers (4.5; D15) | Done | Done | Passed | b76e4bf | Done |
| — | Merging into develop (PR, Squash Merge) | Not started | — | — | — | — |

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
- Checked again on 2026-11-24: `archive.robocup.info` answers over HTTPS and HTTP (200). Its robots.txt is empty (no rules). It is made with h5ai (which shows listings of directories), and the lists can be read from the table of the listing without JavaScript (`fb-n`, `fb-d`, `fb-s`) and from its JSON API (`POST /`, `action=get&items[href]=DIR&items[what]=1`; sizes in bytes, times in ms). The 2D logs are under `/Soccer/Simulation/2D/logs/`: `RoboCup/` (2004 to 2023; no 2020) and `RCAP/` (2018), with rounds and groups nested under a year (how deep depends on the year). The files are `.rcg.gz`, one a game (with `.rcl.gz`, and `.log.gz` in old years). The names have the form `DATE-LEFT_SCORE-vs-RIGHT_SCORE.rcg.gz` since 2004, with a date of 12 or 14 digits (to the second).
- On the same day, the rcsoccersim organization has 7 public repositories, and only RoboCup2026 has a name starting with RoboCup. The tree of RoboCup2026 comes in one request (`truncated` false): 144 logs in 21 directories. The plan's reading of the file names (4.2) read all 144 (3 with a penalty shootout; 13 teams).

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
| D9 | The list of logs is a tree of the directories (Finals, GroupA and so on). Its columns: the date and time, the left team, the score (the scores of a penalty shootout in parentheses), the right team, the size. A field to filter by team names, and sorting by the column headers (2026-11-24; decided by the user at the start of stage 2) |
| D10 | The dialog of the list of logs (File > Browse Game Logs...) is modeless, and stays open after a log is opened (to open one game after another; as Player Type List) (same) |
| D11 | The default sources: `rcsoccersim/RoboCup2026` on GitHub and all the 2D logs of archive.robocup.info (`/Soccer/Simulation/2D/logs/`; RoboCup and RCAP). The archive is made on this branch as stage 3 after stage 2. The logs are to be gathered on GitHub in the future (the user), so the archive can be taken out of the sources then (same) |
| D12 | The lists of the archive are used from the cache for a day, a directory at a time (the archive has the logs of past competitions, which hardly change; Reload reads them again) (2026-11-24, decided by the user at the start of stage 3) |
| D13 | The user can add and remove any source, the default ones (RoboCup2026 and the archive) too (it is handier that the user can change the default sources too). This replaces the default sources that cannot be removed of D2 and D11. When the archive is to be dropped in the future, it is dropped from the default list (same) |
| D14 | More filters of the list of logs: the left team, the right team, both teams (a match), a range of dates, and the win or loss of the team given. The fields are "Team (a team's name) + Side (either, left, right) + vs (the opponent)", and Result (win, draw, loss) is seen from the team of Team. In a penalty shootout, its winner wins (a draw is only a tie without a shootout). No search across the sources. Made as stage 4 on this branch (2026-11-24, decided by the user) |
| D15 | Besides h5ai, a plain list of files of a web server (the autoindex of Apache and so on: a page of links) can be a source too. Whether it is h5ai or a plain list is found out by itself (the API of h5ai is tried first, and if the answer is not of h5ai, the page is read by GET; the kind found is remembered). The example server the user gave (the table form of Apache's FancyIndexing; now the logs of RoboCup2025) is not one of the default sources (the user adds it by Add...). Made as stage 5 on this branch, and included in the PR of F5 (2026-11-24, decided by the user) |
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

## 4. Design and implementation of each stage

For each stage, the design when it was planned (4.N.1) is followed by what was implemented (4.N.2).

### 4.1 Stage 1: open from a URL

#### 4.1.1 Design

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

#### 4.1.2 Implementation (2026-11-24)

4.1.1 was implemented as follows. What was decided beyond the plan is written too.

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

### 4.2 Stage 2: GitHub log lists

#### 4.2.1 Design

- Menu File > Browse Game Logs...: a dialog with the list of logs (`src/log_browser_dialog.{h,cpp}`, new).
  - On the left, the repositories (default `rcsoccersim/RoboCup2026`; the user adds `owner/repo` or `owner/repo@ref`, and removes them; saved in the settings).
  - On the right, the tree of logs: grouped by directory (Finals, GroupA, ...), with the columns date and time, left team, scores, right team, size. A field to filter by team name.
  - Double-click or "Open" passes it to the download of stage 1.
  - "Reload" fetches the list again.
- `src/github_log_index.{h,cpp}` (new; implemented as `github_log_source.{h,cpp}`, one of the sources of the common interface `LogSource`; 4.2.2):
  - `GET https://api.github.com/repos/OWNER/REPO/git/trees/REF?recursive=1` (`REF` defaults to `HEAD`), `Accept: application/vnd.github+json`.
  - From the JSON of the response (`QJsonDocument`, in QtCore), take the entries whose `type` is `blob` and whose name ends with `.rcg` or `.rcg.gz` (path, size). If `truncated` is true, show that the list is incomplete.
  - The URL of a file is `https://raw.githubusercontent.com/OWNER/REPO/REF/PATH`.
  - Cache of the lists (S6): the JSON and the time it was fetched, under `CacheLocation`.
  - Errors: on 403/429 with `x-ratelimit-remaining: 0`, show when the limit is lifted, from `x-ratelimit-reset`. 404 means no such repository or branch.
- `src/log_file_name.{h,cpp}` (new): parsing of file names (pure functions).
  - Split into the date and time and the two sides by `^(\d{12})-(.+)-vs-(.+)\.rcg(\.gz)?$`, then each side into the team name, the score and the penalty score by `^(.*?)_(\d+)(?:_(\d+))?$` (the shortest team name).
  - Examples: `FRA-UNIted_0_4` → `FRA-UNIted`, 0, 4. `RoboTech_Coach_base_0` → `RoboTech_Coach_base`, 0.
  - If it cannot be parsed, show the name as is.

#### 4.2.2 Implementation (2026-11-24)

4.2.1 was implemented as follows. What was decided beyond the plan is written too.

- `src/log_file_name.{h,cpp}` (new; QtCore only): the value class `LogFileName` (in line with D7). `parse()` reads the date and time, the left and right teams, the scores and the penalty scores (-1 if none), and `score()` returns "2 - 1" or "0 (4) - 0 (2)". Dates of 12 digits and of 14 digits (to the second, e.g. the archive's of 2023) are accepted. A date that does not exist, an empty team name, and a name without scores (the archive's old logs) cannot be parsed, and the list shows such a name as it is. The penalty scores use `capturedStart()` (`hasCaptured()` is from Qt 6.3, not in Qt 5).
- `src/log_source.{h,cpp}` (new): the abstract class of the sources of logs, `LogSource` (a `QObject`). `list( dir, reload )` reads the entries under a directory (`LogSourceEntry`: the path, whether it is a directory, whether it is a directory not read yet, the size, the URL), and tells them by `listed` or `failed` after `list()` returns. `noted` tells a note (e.g. the list is from the cache). Nothing is told after `abort()`. GitHub returns the whole tree at once, and the archive (stage 3) returns a directory at a time (a directory not read yet is read when its branch is opened).
- `src/github_log_source.{h,cpp}` (new): `GitHubLogSource`.
  - A repository is given as `OWNER/REPO`, `OWNER/REPO@REF` (REF may have `/`), or `https://github.com/OWNER/REPO` (also with `.git` or `/tree/REF`, or without the scheme) (`parseRepository`). It is shown as `OWNER/REPO` or `OWNER/REPO@REF` (`repositoryText`).
  - One request to the Git Trees API; only the `blob`s whose names are game logs (`.rcg`, `.rcg.gz`) are taken (`parseTree`). The URL of a file is `https://raw.githubusercontent.com/OWNER/REPO/REF/PATH` (stage 1 reads the content of a file of LFS).
  - The cache (S6): the JSON of the response is written to `~/.cache/rcssmonitor/github/OWNER/REPO/REF.json` with `QSaveFile`, and the time of the file is the time it was read. For an hour (`CACHE_SECONDS`), the cache is used, with a note of it and the time. Reload always reads again.
  - When GitHub cannot be read (the limit, 404, a timeout, the network), the cache is shown if there is one, however old, with a note of the reason and the time of the list. Without one, the failure is told.
  - The limit (403 or 429 with `x-ratelimit-remaining: 0`): "60 requests an hour are used up until TIME". 404: the repository is not public, or the branch is not there. The timeout is 30 seconds (both `TimeoutError` of Qt 6.10 and `OperationCanceledError` before it).
  - If `truncated` is true, a note says the list is a part.
  - `User-Agent: rcssmonitor/VERSION` (S5). The places of the API and of the raw files can be replaced for the tests (`setApiBase`, `setRawBase`).
- `src/log_browser_dialog.{h,cpp}` (new): the dialog of File > Browse Game Logs (D9, D10).
  - On the left, the sources, Add... (a repository entered with `QInputDialog`; an error in a message box), and Remove (only those the user added). On the right, the field of a team name (Team:), Reload, the tree of logs, and the status line. At the bottom, Open and Close.
  - The tree is grouped by directory, with the columns Date, Left, Score, Right and Size. Sorting compares the date as `yyyyMMddHHmmss` and the size as a number, and puts the directories first in either order. A log whose name cannot be parsed shows its name in the Date column. A directory alone at the top (`logs` of GitHub) is expanded (and one alone in it, and so on).
  - The filter matches the left and right team names (case-insensitively), or the name of a log whose name cannot be parsed, and expands the directories with a matching log. A directory not read yet is not hidden (it may have a matching log).
  - The status line shows "N game logs." ("M of N game logs." while filtering) and the source's note. With no logs and a note (while reading, a failure), only the note is shown. A failure is shown in red. The plural is written by hand (with no translation file, the `(s)` of `%n` would be shown as it is).
  - A source is read when it is chosen for the first time (when the dialog is made, only the first source). Choosing another and coming back does not read it again.
  - A double click (Enter) or Open tells the log's URL by `openRequested`, and the dialog stays open (D10). Open is not the default button: as the default button, Enter in the tree triggered both `itemActivated` and the default button, and opened the same log twice (`QDialogButtonBox` makes the first AcceptRole button the default one when shown, so Open has ActionRole).
- `MainWindow`: "Browse Game Logs..." (Ctrl+Shift+B) after Open Game Log from URL in the File menu. The dialog is made when it is first shown, with the default repositories (`GitHubLogSource::DEFAULT_REPOSITORIES`, not removable) and the repositories saved in the settings (removable). `openRequested` goes to `openGameLogUrl()`, and `repositoriesChanged` to the settings, written at once.
- `Options`: the repositories added (`github_repositories` in `[GameLog]` of the settings; `writeLogRepositories()`).
- Checked with the real GitHub (2026-11-24): the list of RoboCup2026 has 144 games, read by one request in 0.7 s; the second time from the cache in 4 ms. A game of the Finals (4.6 MB) was opened from the list.
- In stage 3, the handling of the sources (the default ones, Remove, Add..., the settings) was changed after D13 (4.3.2).

### 4.3 Stage 3: archive.robocup.info

#### 4.3.1 Design

- The site can be reached since 2026-11-24 (§2.1). It is made after stage 2, on this branch (D11).
- `src/archive_log_source.{h,cpp}` (new): `ArchiveLogSource`, one of the `LogSource`s of stage 2. Its root is the URL of a directory listed by h5ai (by default `https://archive.robocup.info/Soccer/Simulation/2D/logs/`, D11).
  - The lists are read with the JSON API of h5ai (`POST /`, `action=get&items[href]=DIR&items[what]=1`), a directory at a time. As the tree is deep and large, a directory is read when its branch is opened. The response has, besides the entries right under the directory, the siblings of each level up to the root (checked on 2026-11-24), so only those right under it are taken. The `href` of a directory ends with `/`. A file has its `size` (bytes).
  - The directories right under it are returned as directories not read yet. Of the files, only `.rcg` and `.rcg.gz` (not `.rcl.gz` nor `.log.gz`). The logs are a `.rcg.gz` a game, not bundled (no tar to extract). The URL of a file is made from the server's `href`.
  - The JSON of the response is cached a directory at a time (`~/.cache/rcssmonitor/archive/`) and used for a day (D12). When it cannot be read, the cache is shown, however old, with a note (as for GitHub). Reload reads the root again, and the directories opened after it are read without the caches from before the Reload.
  - A source can read several directories at once (branches opened one after another).
- The list of sources (D13): the settings keep the list of sources as the texts the user enters (`log_sources` in `[GameLog]`; `OWNER/REPO[@REF]` for GitHub, the URL for the archive). Without it in the settings, the default list (`Options::DEFAULT_LOG_SOURCES`: `rcsoccersim/RoboCup2026` and the archive). `github_repositories` of stage 2 is replaced, as it has not been released.
  - Remove removes any source.
  - Add... takes a repository on GitHub (`OWNER/REPO`, `OWNER/REPO@REF`, a URL) and the URL of a directory listed by h5ai (e.g. a year of the archive). The field is an editable combo box, whose items are the default sources not in the list (a default source removed can be chosen again).
  - The dialog makes a source from a text (`LogBrowserDialog::createSource`). `LogSource` gets the text that makes the source again (`text()`).

#### 4.3.2 Implementation (2026-11-24)

4.3.1 was implemented as follows. What was decided beyond the plan is written too.

- `src/archive_log_source.{h,cpp}` (new): `ArchiveLogSource`.
  - Reading a URL (`parseUrl`): http or https with a host, without a user name or a password, and not github.com (GitHub is added as a repository). `/` is added to the end of the path, and the query and the fragment are dropped.
  - Reading an answer (`parseItems`): the `href`s are decoded, and only the entries right under the directory asked for are taken. A directory is one not read yet, and of the files only the game logs. The URL of a file is its `href` (as encoded) resolved against the URL of the directory. What is not JSON (e.g. a page of a server without h5ai) is an error.
  - The request is a `POST` to the URL of the directory (checked on 2026-11-24 that the archive answers a `POST` to a directory, not only to `/` of the server; it works with a server whose h5ai is only in a part of the site). `items[href]` in the body is the encoded path of the directory (encoded again for the form). `User-Agent: rcssmonitor/VERSION`. A timeout of 30 seconds.
  - The cache: under `~/.cache/rcssmonitor/archive/HOST/`, named by the first 16 digits of the SHA-1 of the directory's URL. Used for a day (`CACHE_SECONDS`, D12). After a Reload (the root read again), only the caches of the directories read from the server after it are used. At first the time of the file was compared with the time of the Reload, but within the same millisecond an old cache was used, so the directories read are remembered instead.
  - One request at a time for a directory (reading the same directory again stops the earlier request). Different directories can be read at once.
  - Errors: 404 (no such directory), other HTTP errors, a timeout (30 seconds), not an answer of h5ai. If there is a cache, it is shown, however old, with a note.
- The sources (D13):
  - `LogSource::text()` (the text that makes the source again) was added: `OWNER/REPO[@REF]` for GitHub, the URL (encoded) for the archive.
  - `Options::DEFAULT_LOG_SOURCES` (`rcsoccersim/RoboCup2026` and the archive's URL) is the default list, and the list is kept in `log_sources` of `[GameLog]` in the settings (`logSources()`, `setLogSources()`, `writeLogSources()`). An empty list is written as an empty text (`QSettings` does not read an empty list back, and the defaults would come back). `GitHubLogSource::DEFAULT_REPOSITORIES` and `github_repositories` were removed.
  - `LogBrowserDialog`: `createSource()` (a source from a text: a repository on GitHub if it reads as one, a directory of h5ai if it is http or https), `setSources()` (the list of the settings; the texts that are not sources are skipped), `addSource( text, error )` (not the same one twice), `sourceTexts()`, `missingDefaultSources()`, and the signal `sourcesChanged`. Remove removes any source. Add... is an editable combo box offering the default sources not in the list.
  - `MainWindow`: gives the dialog `setSources( Options::logSources() )`, and writes the settings on `sourcesChanged`.
- Changes to the dialog: with only directories not read yet (e.g. the root of the archive), the status line says "Open a directory to list its game logs." In the list of sources, a long name (the archive's URL) is cut in the middle, without a horizontal scroll bar (the whole in the tooltip). The left side is 240 wide.
- Found by the tests and fixed: removing a source removed it from the internal list before `takeItem()`, so the change of the current row inside `takeItem()` read the source of a shifted index and crashed (a latent error since stage 2, which came out when the default sources became removable). Now the item of the list is taken first, and the current row is chosen again with the signals blocked, then shown.
- The test environment: with the plugin of the plastique style of Qt 5 (`qt5-style-plugins`) installed, the "unknown style name" (plastique) of `test_ui_color_scheme` was a known style, and the test failed. There, a name that no style has is used.
- Checked with the real archive (2026-11-24): the root is listed in 1.9 s, and each directory in about 1 s. 19 years under RoboCup, 6 directories under 2023, 25 games in Finals. A game of the Finals (4.6 MB) was opened from the list. The second time from the cache within 50 ms.

### 4.4 Stage 4: filters of the list of logs (D14)

#### 4.4.1 Design

- `src/log_filter.{h,cpp}` (new; QtCore only): the value class `LogFilter`: a team's name (`team`), its side (`Side`: either, left, right), the opponent (`opponent`), the result (`Result`: any, win, draw, loss), and a range of dates (`from`, `to`; an invalid date is no limit; both ends included). `matches( LogFileName, file name )` decides.
  - A team's name matches a part, case-insensitively (as in stage 2).
  - With the side "either", either way round may match. Team matches the left (right) team, vs the other side, and the result is seen from Team's side. An empty Team is any team on its side.
  - The result: by the goals, and in a tie with a penalty shootout by its goals. A tie without a shootout is a draw.
  - A log whose name cannot be parsed: it matches if its file name has the text of Team (as in stage 2). It does not match if a condition other than Team is given (Side, vs, Result, dates).
- The dialog: Team, Side, vs and Result in the first row; in the second, From and To (dates enabled by their check boxes, chosen from a calendar, at first the oldest and the newest days of the logs listed of the source), Clear (all back), and Reload. The filter is applied to the tree whenever a condition changes (the directories not read yet are not hidden, as in stage 2). `setFilter( LogFilter )` and `filter()` are public (for the tests).

#### 4.4.2 Implementation (2026-11-24)

4.4.1 was implemented as follows. What was decided beyond the plan is written too.

- `src/log_filter.{h,cpp}` (new): `LogFilter`. `matches()` looks at the date (the day of the log's date; both ends included), then, for each way round the Side allows (either, left, right), at Team and vs (a part, case-insensitively) and Result. The result is the static `resultOf( goals, penalties, the opponent's goals, the opponent's penalties )`: by the goals if they differ, by the penalties in a tie with a shootout whose goals differ, a draw otherwise. An empty Team is any team on its side (without Team, Side=left and Result=win is "the games the left team won").
- `LogBrowserDialog`: the field of a team of stage 2 (`filter`) was replaced with Team (`team`), Side (`side`), vs (`opponent`), Result (`result`), From and To (`from_check`, `from`, `to_check`, `to`), and Clear (`clear_filter`). Whenever a field changes, `applyFilter()` applies it to the tree. `setFilter( LogFilter )` sets the fields with their signals blocked, and applies it once. A log's item of the tree keeps its file name (`NAME_ROLE`), read with `LogFileName` when applied.
  - The fields of the dates are `QDateEdit`s enabled by their check boxes (with a calendar, `yyyy-MM-dd`). A field not checked follows the oldest and the newest days of the logs in the tree (whenever the list is shown, and on Clear).
  - The public slot `setFilter( QString )` of stage 2 was removed.
- Checked with the real list of RoboCup2026 (144 games): HELIOS's wins from 4 July are 6 games (with a win in a shootout), and the games of FRA-UNIted on the left against NexusPrime are 2.

### 4.5 Stage 5: lists of files of web servers (D15)

#### 4.5.1 Design

- `ArchiveLogSource` reads both h5ai and plain lists (not another class: both are directories of a web server, with the same handling of the URL, the cache, the reading a directory at a time, and the failures). Each source has the kind of its list, `Listing` (not known yet, h5ai, a page of links).
- Finding out the kind: while it is not known, a request to the API of h5ai (POST) is sent. If the answer is of h5ai, the kind is h5ai. If the server answered by HTTP but not by h5ai (an error such as 404, 405 or 501, or a 200 that is not JSON), the same URL is read by GET as a page. If the server cannot be reached, or does not answer in time, it fails without a GET. The kind found is remembered, and the directories after it are read by one request. Reload (of the root) forgets the kind and finds it out again. On 2026-11-24, the example Apache server was checked to answer a POST with 404.
- Reading a page (`parseIndex`):
  - Only the `<a href>`s that point right under the directory are taken: the same scheme, host and port, without a query (`?C=N;O=D` and so on) or `#`. One that ends with `/` is a directory.
  - The parent directory, the links to sort, and the links to other sites are not taken. The same link is taken once.
  - The character references of an `href` (`&amp;` and so on) are decoded, and then it is resolved against the URL of the directory (relative, absolute from `/`, or a full URL). Of the files, only `.rcg` and `.rcg.gz`.
  - The size: the last word that looks like a size (a number, and a unit such as K, M, G, T, KB or MiB; a unit is 1024 times) in the text after the link (up to the next link, the end of the line, or the row of the table). Apache gives a rounded value (`5.0M`), nginx the bytes by default, the page of h5ai `5123 KB`, and Python's http.server no size (-1). A rounded value is taken as an approximation (the column shows the MB with one decimal, so it looks the same).
  - An answer that does not look like HTML (no `<a`, `<html` and so on) is an error.
- The cache: a page is kept as `.html`, apart from the answers of h5ai (`.json`). While the kind is not known, a cache found gives the kind. Kept for a day (D12), the same.
- The texts: "a list given by h5ai" in the dialog (Add... and the description of a source), README, NEWS and ChangeLog becomes "a list of files of a web server (h5ai, or a page of links such as Apache's)".
- The example server is not one of the defaults (D15). The test data copies the form of the example server's page, with the host name and the paths changed.

#### 4.5.2 Implementation (2026-11-24)

4.5.1 was implemented as follows. What was decided beyond the plan is written too.

- `ArchiveLogSource` (`archive_log_source.{h,cpp}`):
  - The kind of the list, `Listing` (`UNKNOWN_LISTING`, `H5AI_LISTING`, `INDEX_LISTING`), and `listing()` were added. Each request remembers its directory and its kind (`Request`).
  - `request( dir, listing )`: a POST for h5ai (`Accept: application/json`), a GET for a page (`Accept: text/html`). Both with `User-Agent: rcssmonitor/VERSION`, a timeout of 30 seconds, and `NoLessSafeRedirectPolicy`.
  - Handling an answer: if a request for h5ai failed or was not answered by h5ai, the kind is not known yet, and the server answered by HTTP (with a status code), the same directory is read by GET. When an answer is read, its kind is remembered.
  - `parseIndex()`: as in 4.5.1. The links are taken from `<a ... href=...>` (in double quotes, single quotes, without quotes, and in capitals; not `data-href` and so on). If two links go to the same file (`IconsAreLinks` of Apache), the size known of them is used.
  - `parseSize()`: the last word of a number that is not a part of a date or a time. K, M, G and T (with B or iB too) are 1024 times. When a word that is not a unit follows, as in `5 X`, the number alone is the size (bytes).
  - The character references decoded are `&amp;`, `&lt;`, `&gt;`, `&quot;`, `&apos;`, `&nbsp;` (as a space), and the numeric ones.
  - The cache: `cachePath( dir, listing )` is `.json` for h5ai, and `.html` for a page. `cachedListing()` uses the file of the kind if it is known, and the newer one if not.
  - The description of a source (`description()`) became "listed by the web server: h5ai, or a page of links such as Apache's".
- `LogBrowserDialog`: the tooltip of Add..., its input dialog, and the text for a text that is not a source were changed.
- `test/http_test_server.h`: responses by method (`route( method, path, response )`), the counts by method (`requests( method, path )`), and a response of an error (`error()`) were added.
- The test data:
  - `test/data/index_apache_2025.html` and `index_apache_finals.html`: the pages of the example server, with the host name changed to example.org and the paths to `/logs/`; 4 games of the 9 of Finals kept.
  - `h5ai_finals.html`: the page of Finals of the archive, with 4 games kept.
  - The pages of nginx, of the preformatted form of Apache, and of Python's http.server (really given by 3.12) are written in the test.
- Checked (2026-11-24): the example server (Apache), added as a source, was listed at the root in 55 ms (a POST answered with 404, and a GET), and RoboCup2025 (13 directories) and Finals (9 games, with sizes such as 5.0 MB) in about 50 ms each. A game (4.2 MB) was opened from the list. The archive was found to be h5ai, and read down to Finals of 2023 (25 games).

## 5. Stages

| # | Work | Completion criteria |
|---|---|---|
| 1 | Open from a URL (4.1). Decide the stage 1 items of §3.3 | Unit tests and the CI pass. The user checks that real GitHub URLs (file pages, raw links, LFS files) and URLs of other servers can be opened |
| 2 | GitHub log lists (4.2) | Unit tests and the CI pass. The user checks opening from the list and the message when the limit is hit |
| 3 | Archive lists (4.3) | Unit tests and the CI pass. The user checks opening from the list of the archive |
| 4 | Filters of the list of logs (4.4, D14) | Unit tests and the CI pass. The user checks narrowing the games down by the filters |
| 5 | Lists of files of web servers (4.5, D15) | Unit tests and the CI pass. The user checks opening logs from the list of the example Apache server |

## 6. Tests

- `test_log_url`: parsing URLs (port the web version's `fetch-log.test.ts`): GitHub blob, raw and raw.githubusercontent.com, other hosts, relative URLs, rejecting `ftp:` etc., rejecting URLs with a user name, taking the name, detecting LFS pointers.
- `test_log_downloader`: a small HTTP server with `QTcpServer` in the test; success, redirects (http → http; https → http is not followed), 404, abort in the middle, the size limit, LFS pointer → the real file. The CI uses no external network.
- `test_github_log_source` (`test_github_log_index` in the plan): parsing `test/data/github_tree.json` (a part of RoboCup2026's tree: 21 logs, `.rcl.gz`, README and so on), parsing the repositories given, reading from the test's HTTP server, the cache (no request for an hour, Reload, an old cache), the rate-limit response (without a cache, with an old cache), 404, `truncated`, `abort()`.
- `test_log_browser_dialog`: the tree from a source of the test (the columns, the directories first, a directory not read yet read when opened, read again when opened after a failure), the filter, sorting (the size as a number), Open and Enter (opened once), a failure, Reload and notes, switching the sources, adding and removing repositories (with the test's HTTP server), and File > Browse Game Logs of the main window (the default and saved repositories, modeless, a log downloaded and opened from the list, a repository added saved in the settings).
- `test_archive_log_source` (stage 3): parsing `test/data/h5ai_2023.json` and `h5ai_finals.json` (real answers of the archive; Finals cut down to 4 games) (only those right under, only the logs, the sizes and URLs), encoded names, answers not of h5ai, parsing URLs, the POST to the test's HTTP server (its body), the cache (a day, after a Reload), several directories at once, 404, HTTP errors, an old cache, `abort()`.
- `test_log_browser_dialog` (added in stage 3): adding and removing sources (GitHub, a directory of h5ai, the same one, not a source, a file on github.com, the default sources offered by Add..., the one chosen after removing the first and the last, removing all), the tree of a directory of h5ai (read when opened, Reload), and the main window (the sources of the settings, texts that are not sources skipped, a default one removable and written to the settings, an empty list written as an empty text).
- `test_log_filter` (stage 4): combinations of a team, a side, an opponent, a result and dates (both ends, a reversed range) on 7 names (wins on the left and the right, a loss, a draw, a shootout won and lost, 2023, a name not parsed); `resultOf()`, `isEmpty()`.
- `test_log_browser_dialog` (changed in stage 4): the fields of the filter (Team, Side, vs, Result, From and To, Clear, `setFilter()` and `filter()`), the first values of the dates, and their change with a directory read later.
- `test_archive_log_source` (stage 5): reading pages (the table and the preformatted forms of Apache, nginx, Python's http.server, the page of h5ai), the links not taken (the parent, sorting, other hosts, outside the directory, the same one, not game logs), the forms of the sizes, answers that are not HTML, finding out the kind (a GET after a POST answered with 404, no GET with h5ai, only a GET for the second directory, no GET when the server cannot be reached, found out again by Reload), and the cache of `.html`.
- `test_log_browser_dialog` (stage 5): the tree of a source of a page of links (read when opened, the sizes).
- `test_log_file_name`: parsing file names (penalty scores, `-` and `_` in team names, names that cannot be parsed).
- Downloading and opening a broken file (not gzip, truncated, not an rcg) does not crash.

## 7. Files to change (expected)

| File | Content |
|---|---|
| `src/log_url.{h,cpp}`, `src/log_cache.{h,cpp}`, `src/log_downloader.{h,cpp}`, `src/open_url_dialog.{h,cpp}` (new) | Stage 1 |
| `src/log_source.{h,cpp}`, `src/github_log_source.{h,cpp}` (`github_log_index` in the plan), `src/log_file_name.{h,cpp}`, `src/log_browser_dialog.{h,cpp}` (new) | Stage 2 |
| `src/archive_log_source.{h,cpp}` (new) | Stage 3 |
| `src/log_filter.{h,cpp}` (new) | Stage 4 |
| `src/archive_log_source.{h,cpp}`, `test/http_test_server.h` | Stage 5 (pages of links, responses by method of the test server) |
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

- Stage 1 was implemented (4.1.2). Tests: `test_log_url` (ported from `fetch-log.test.ts` of the web version; telling URLs on the command line; the cache's paths, size and clearing), `test_log_downloader` (with a small HTTP server in the test, `test/http_test_server.h`: success and progress, replacing, redirects, 404, no server, abort, the next start, the size limit (by the length, while reading), LFS (the content, the limit, not GitHub), the timeout, a place not writable), `test_remote_log` (in the main window: what a drop has, dropping a file, opening a URL and reusing the cache, a URL on the command line, Cancel, telling a failure, Save As, the dialog, the recent URLs).
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
- Fixed: the dialog is not modal; the text of the progress is set first and `setValue()` last, with the dialog watched by `QPointer`; opening the downloaded log and telling a failure are done after the call of `LogDownloader` returns (4.1.2). The new test crashed before the fix and passes after it.
- The name: the user chose "Save a Copy of Game Log..." (D5). The menu, the function (`saveGameLogCopy`), the messages, the tests, README, NEWS and ChangeLog were changed.
- Checked: the three CMake builds had no warnings, and `ctest -j8`, run 3 times in each, passed all 64 tests. The autotools build (Qt5) passed without warnings too.
- Next: the user checks again (the completion criteria of stage 1). Then the commit and push, and the CI.

### 2026-11-24: Deciding to revise the design

- The user pointed out that the functions ported from the web version are global functions, which feels odd as C++ code, and asked whether they can be put in classes (statics of `LogDownloader`, perhaps). The functions were sorted by their roles and where they are used, and two classes were proposed: `LogUrl` (reading URLs) and `LogCache` (the cache). The user decided D7 and D8.
- The order of the commits (decided by the user): the present state (the crash fixed, all the tests passing) is committed and pushed first, and the revision is the next commit.

### 2026-11-24: The design revised (D7)

- The first version of stage 1 was committed as d56fad4 and pushed.
- The URL functions were made into the static-only class `LogUrl` (`log_url.{h,cpp}`) and the class `LogCache` (`log_cache.{h,cpp}`, new), which has its directory (4.1.2). The main window has a `LogCache` (`gameLogCache()`), and gives it to the dialog. `MainWindow::gameLogCacheDir()` was removed. The tests were split into `test_log_url` (`LogUrl`) and the new `test_log_cache` (the place, `pathOf`, `contains`, the size and clearing). The behavior is not changed.
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

### 2026-11-24: Stage 2 started

- In the CI of stage 1 (26b4935), all 5 jobs passed (the new "CMake, Qt6 (Ubuntu 24.04)" ran 65 tests with Qt 6.4.2). Stage 2 was started at the user's instruction.
- GitHub and the archive were checked again (§2.1). archive.robocup.info can be reached now.
- Decided by the user at the start of the stage: D9 (a tree of the directories), D10 (a modeless dialog), D11 (the default sources are RoboCup2026 and all the 2D logs of the archive; the archive after stage 2, on this branch). 4.3 was rewritten.

### 2026-11-24: Stage 2 implemented

- Implemented as in 4.2.2. The tests `log_file_name`, `github_log_source` and `log_browser_dialog` were added.
- Found by the tests and fixed: Enter in the tree opened the same log twice (Open is not the default button); the filter hid the directories not read yet; the plural of the status line was shown as "log(s)"; `setFilter()` did not change the filter field; Qt 5 has no `QRegularExpressionMatch::hasCaptured()`.
- With the CMake builds of Qt 6 (6.4.2), Qt 5 (5.15.13) and without Qt Multimedia, all 68 tests passed. The autotools build (Qt 5) passed too.
- Checked with the real GitHub (4.2.2). README, NEWS, ChangeLog and `docs/build.md` (the lists added to the requirement of TLS) were updated.

### 2026-11-24: Stage 2 committed, and its CI

- Stage 2 was committed as ec341e2 and pushed. All 5 jobs of the CI passed.

### 2026-11-24: Stage 3 started

- The user resumed, and stage 3 was started. The API of h5ai was checked again (4.3: the response has the siblings of each level up to the root too; a file has its size and time).
- Decided by the user at the start of the stage: D12 (the archive's cache for a day), D13 (the user can change the list of sources, the default ones too). 4.3 was rewritten.

### 2026-11-24: Stage 3 implemented

- Implemented as in 4.3.2. The test `archive_log_source` was added, and `log_browser_dialog` and `github_log_source` were changed after D13.
- The tests found a crash when a source is removed, and it was fixed (4.3.2).
- This environment had no development package of Qt Multimedia, so the CMake builds of Qt 6 (6.4.2) and Qt 5 (5.15.13) were both without the whistle (the CI checks the builds with it). With them and the build of `ENABLE_SOUND=OFF`, all 69 tests passed. The autotools build (Qt 5) passed too.
- Checked with the real archive (4.3.2). README, NEWS, ChangeLog and `docs/build.md` were updated.

### 2026-11-24: Stage 3 committed, and stage 4 started

- The user checked, and stage 3 was committed as 42e08a1, with the fix of the test environment (`test_ui_color_scheme`) apart as bec0f96, and pushed.
- The user installed Qt Multimedia again. With the builds of Qt 6 and Qt 5 with the whistle, all 69 tests passed.
- Answered the user's question: supporting directory listings other than h5ai (Apache's autoindex, nginx, Python's http.server and so on) would be half to two thirds of stage 3 (a source taking the entries right under a directory from `<a href>` of the HTML; the API of h5ai tried first, and HTML read if the answer is not JSON; the sizes exact, rounded or none by the server). Whether to support it is not decided.
- The only filter planned in stage 2 was a team's name. The user decided D14, as stage 4 on this branch. 4.4.1 was written.

### 2026-11-24: Stage 4 implemented

- In the CI of stage 3 (42e08a1), all 5 jobs passed.
- Implemented as in 4.4.2. The test `log_filter` was added, and the test of the filter of `log_browser_dialog` was rewritten.
- With the CMake builds of Qt 6 (6.4.2) and Qt 5 (5.15.13) with the whistle, and of `ENABLE_SOUND=OFF`, all 70 tests passed. The autotools build (Qt 5) passed too.
- README, NEWS and ChangeLog were updated.

### 2026-11-24: Stage 4 committed, and stage 5 started

- The user checked on the screen that the filters narrow down the games (the end of stage 4). Stage 4 was committed as 96eb9a2 and pushed. In the CI, all 5 jobs passed. `./cmake-helper.sh distcheck qt6` (under Xvfb) passed locally too (with the whistle; 70 tests in the build of the tarball).
- The lines of the item of F5 in NEWS were wrapped evenly (the text is not changed).
- When the title and the description of the PR were proposed, the user decided to support plain lists of files of Apache besides h5ai (D15; the estimate is in the record "Stage 3 committed, and stage 4 started").
- The example server was checked. It is the table form of FancyIndexing of Apache 2.4: it answers a POST to the API of h5ai with 404, and a GET with the page of the list (the sizes rounded, as `5.0M`). The GET page of the archive (h5ai) has a table of links too, shown without JavaScript (the sizes as `5123 KB`).
- Decided by the user: made as stage 5 on this branch, and included in the PR of F5. The kind is found out by itself. The example server is not one of the default sources. 4.5.1 was written. The PR is opened after stage 5.

### 2026-11-24: Stage 5 implemented

- Implemented as in 4.5.2. Reading pages (the example server's pages, the forms of other servers, the links not taken, the sizes) and finding out the kind were added to the test `archive_log_source`, and the test of the failures of h5ai was changed for the behavior after the kind is found out. The tree of a source of pages of links was added to `log_browser_dialog`.
- With the CMake builds of Qt 6 (6.4.2) and Qt 5 (5.15.13) with the whistle, and of `ENABLE_SOUND=OFF`, there were no warnings, and all 70 tests passed. The autotools build (Qt 5) passed too, without warnings of the compiler.
- Checked with the real example server and the archive (4.5.2). README, NEWS and ChangeLog were updated.
- The user checked opening logs from the list of the example server (the end of stage 5).
- Next: the commit and the push, the CI, and the proposal of the PR.

### 2026-11-24: Stage 5 committed, the CI, and the table of the progress

- Stage 5 was committed as b76e4bf and pushed. In the CI, all 5 jobs passed. `./cmake-helper.sh distcheck qt6` (under Xvfb) passed locally too (the new test data in the tarball; 70 tests).
- At the user's request, a table of the stages and their progress ("Progress": the status, the user's check, the CI, the commits) was put near the top of the plan. The "Status" of the header was shortened to point to the table. The plans from now on have the same table. Whether to add it to the other existing plans is checked with the user.

### 2026-11-24: §4 rearranged

- At the user's request, §4, where the designs and the implementations of the stages were mixed and hard to read, was arranged by stage. In each stage (4.N), the design when it was planned (4.N.1) is followed by what was implemented (4.N.2). Splitting it into two chapters was another way, but it would change the numbers of §5 to §9 and many references, so the numbers of the chapters were kept.
- The numbers of the sections (old → new): 4.1 → 4.1.1, 4.2 → 4.2.1, 4.3 → 4.3.1, 4.4 → 4.1.2, 4.5 → 4.2.2, 4.6 → 4.3.2, 4.7 → 4.4.1, 4.8 → 4.4.2, 4.9 → 4.5.1, 4.10 → 4.5.2. The references in this plan (in the records before this one too) and in the comments of the sources and the tests (old 4.7 → 4.4, old 4.9 → 4.5) were changed.
- The plans from now on also give the design and the implementation of each stage one after the other.

### 2026-11-24: The format of the plans unified

- Decided by the user: the other existing plans (the overall plan, transport, 3d-view, cmake-helper, icon-redesign, frame-export) also get the table of the progress near the top. The design and the implementation by stage are not forced into a plan whose design is not in sections by stage.
- At the user's request, a unified format based on the plans so far was written in "計画書の書式" (the format of the plans) of CLAUDE.md: the items of the header table and their order, the columns of the table of the progress (the status as Not started, In progress or Done; a column of the push), the chapters, the numbers and the references, and how to write the record.
- This plan was aligned to the format: the last update was added to the header table, the words of the status in the table of the progress were changed, and a column of the push was added.
- Decided by the user (Claude's proposal): the other plans are changed in the first commit of the branch of F6, after F5 is merged, together with the record of the merge of F5 in the overall plan.
- Next: the commit and the push of the changes of this plan, and the CI. Then the PR of F5.
