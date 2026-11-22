# Playing Logs from URLs, and Log Lists (GitHub, archive): Plan

| Item | Content |
|---|---|
| Created | 2026-11-22 |
| Target | rcssmonitor develop (5affd90, 20.0.0-beta.1) |
| Branch | `feature/remote-logs` (from develop after the referee's whistle (F4) is merged; the user creates it on GitHub) |
| Status | Plan written (2026-11-22). Stage 3 (archive) is on hold until the site can be reached |
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
| `src/log_url.{h,cpp}`, `src/log_downloader.{h,cpp}` (new) | Stage 1 |
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
