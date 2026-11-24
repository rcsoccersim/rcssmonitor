# Adding cmake-helper.sh: Implementation Plan

| Item | Content |
|---|---|
| Created | 2026-11-21 |
| Last updated | 2026-11-24 |
| Target | rcssmonitor develop (a6df084, 19.0.1) |
| Branch | `feature/cmake-helper` (from develop; the user creates it on GitHub) |
| Status | Done. Merged into develop by PR #69 (Squash Merge) (c392514, 2026-11-21). The progress is in the table of "Progress" below |
| Japanese version | [2026-11-21_cmake-helper.md](2026-11-21_cmake-helper.md) (same content) |
| Related | The 3D view plan (`docs/plan/2026-11-11_3d-view.md` on `feature/3d-view`): §14 "2026-11-21" and Phase 10 |

## Progress

The steps are in §5, and the history in §6. The merge into develop, and then into `feature/3d-view`, are recorded in §14 (2026-11-21) of the 3D view plan.

| Stage | Content | Status | Checked by the user | CI | Commits | Pushed |
|---|---|---|---|---|---|---|
| 1 to 4 | `cmake-helper.sh` and the documents, the local tests, the CI (steps 1 to 4 of §5) | Done | Done | Passed | a7e6880 | Done |
| 5 | Merging into develop (PR #69, Squash Merge) | Done | Done | Passed | c392514 | Done |
| 6 | Merging develop into `feature/3d-view` | Done | — | — | §14 (2026-11-21) of the 3D view plan | Done |

## 1. Goal and approach

- Add a script, `cmake-helper.sh`, that does the frequently used CMake operations with one command: configure, build, test, run, install, uninstall, and create and check the source tarball.
- soccerwindow2 (`rctools/soccerwindow2`) already has a `cmake-helper.sh`. The goal is the same, and the usage is kept the same.
- The script prints each cmake command before running it, so that the user can learn the actual commands.
- The change does not depend on the 3D view, so it is not part of the 3D view plan. It is done on its own branch with this small plan.

## 2. Decisions

### 2.1 The user's decisions (2026-11-21)

| # | Content |
|---|---|
| D1 | Work on the branch `feature/cmake-helper` made from develop, with this plan, and open a PR to develop. It is one feature, so Squash Merge fits |
| D2 | The default CMake install prefix is `~/.local` |
| D3 | The tarball made by `dist` is named by the release number: `rcssmonitor-VERSION.tar.gz` (VERSION is the version of `project()`). It extracts to `rcssmonitor-VERSION/` |
| D4 | Whether the release tarballs are made by `dist` is decided again in Phase 10 of the 3D view plan. Until then, autotools' `make dist` stays as it is |

### 2.2 Design choices (by Claude; the user checks them before the PR)

| # | Content | Reason |
|---|---|---|
| S1 | The installed `rcssmonitor` has the RPATH `$ORIGIN/../lib` | The rcg library (`librcssrcg.so`) is installed into `PREFIX/lib`. `~/.local/lib` is not in the default library search path, so without the RPATH the program would not start |
| S2 | The desktop entry's `TryExec` and `Exec` have the absolute path of the installed `bin`. `data/rcssmonitor.desktop` is renamed to `data/rcssmonitor.desktop.in`, and the entry is generated at installation (`install(CODE)`) | The application menu starts it even where `~/.local/bin` is not in PATH. Because the entry is generated at installation, it also follows the prefix given by `cmake --install --prefix DIR`. DESTDIR is not part of the path |
| S3 | `uninstall` removes the files listed in `install_manifest.txt`, and of the directories only `include/rcss/rcg` and `include/rcss`, when they become empty | `bin`, `lib` and so on are shared with other packages, so they are kept |
| S4 | `dist` makes the tarball from HEAD by `git archive`. `export-ignore` in `.gitattributes` leaves out `.github`, `.gitignore`, `.gitattributes` and `docs/plan`. Uncommitted changes give a warning. It is an error if the sources are not the top of a git working tree. The tarball is made in the top source directory by default (the cache variable `RCSSMONITOR_DIST_DIR` changes it) | The same as soccerwindow2. If an extracted tarball is placed inside another git working tree, the other repository is not archived by mistake |
| S5 | `distcheck` makes the tarball, extracts it, and makes the sources read-only. It then configures, builds and runs ctest. It installs into the work directory and checks the main files and the desktop entry's `TryExec`/`Exec` (and validates it with `desktop-file-validate` if available). It starts the installed program with `--version`, without LD_LIBRARY_PATH and with HOME in the work directory. After uninstalling, no file may remain. When all of this passes, the work directory is removed | The counterpart of autotools' `make distcheck`. HOME is replaced because `rcssmonitor` writes `~/.rcssmonitor.conf` on exit, even with `--version` |
| S6 | The Qt version is chosen by the existing `QT_VERSION_MAJOR` (`auto`/`qt6`/`qt5` → build directories `build`/`build-qt6`/`build-qt5`) | soccerwindow2 has its own cache variable, but rcssmonitor already has `QT_VERSION_MAJOR` |
| S7 | Add a `.gitignore`. It starts with master's two lines (`*.o`, `build/`) unchanged, and adds `/build-qt5/`, `/build-qt6/` and `/rcssmonitor-*.tar.gz` | develop has no `.gitignore`. This keeps the add/add conflict small when develop is merged into master. The user's `~/.gitignore` ignores `.gitignore` files, so the commit needs `git add -f` |
| S8 | Add a CI job `cmake-distcheck` (ubuntu-26.04, Qt6, `./cmake-helper.sh distcheck qt6`) | Each run checks that no file needed in the tarball is missing, and that install and uninstall still work |
| S9 | The cmake scripts (`cmake/*.cmake`) call `cmake_minimum_required(VERSION 3.16)` at the top | In the script mode of `cmake -P` no policies are set, and `while(TRUE)` is false (CMP0012). Found by the tests in §6 |
| S10 | autotools is not changed (`make install` and `make dist` stay as they are) | D4 |

## 3. Files to change

| File | Content |
|---|---|
| `cmake-helper.sh` (new) | Commands: `configure`, `build`, `test`, `run [-- ARGS]`, `install [--prefix DIR]`, `uninstall`, `dist`, `distcheck`, `clean [QT\|all]`, `help`. Environment variables `CMAKE_ARGS` (configure runs again when it is set) and `JOBS` |
| `cmake/Uninstall.cmake` (new) | The script of the `uninstall` target (S3) |
| `cmake/Dist.cmake` (new) | The script of the `dist` target (S4) |
| `cmake/DistCheck.cmake` (new) | The script of the `distcheck` target (S5). It refuses to remove a work directory that is not named `distcheck` |
| `CMakeLists.txt` | The default prefix (D2), the RPATH (S1), the generated desktop entry (S2), and the `uninstall`/`dist`/`distcheck` targets |
| `data/rcssmonitor.desktop` → `data/rcssmonitor.desktop.in` | `TryExec=@RCSSMONITOR_BINDIR@/rcssmonitor`, `Exec="@RCSSMONITOR_BINDIR@/rcssmonitor"` |
| `.gitattributes` (new) | `export-ignore` (S4) |
| `.gitignore` (new) | S7 |
| `README.md` | The "cmake-helper.sh" section. The default of `CMAKE_INSTALL_PREFIX`; install and uninstall |
| `.github/workflows/ci.yml` | The `cmake-distcheck` job (S8) |

## 4. Differences from soccerwindow2

| Item | soccerwindow2 | rcssmonitor |
|---|---|---|
| Tarball name | `soccerwindow2-YYYYMMDD.tar.gz` (the commit date of HEAD), extracted to `soccerwindow2/` | `rcssmonitor-VERSION.tar.gz`, extracted to `rcssmonitor-VERSION/` (D3) |
| Choosing Qt | `SOCCERWINDOW2_QT_VERSION` | `QT_VERSION_MAJOR` (S6) |
| Directories removed by uninstall | `share/soccerwindow2` | `include/rcss/rcg`, `include/rcss` (S3) |
| Finding libraries | `CMAKE_INSTALL_RPATH_USE_LINK_PATH` | `$ORIGIN/../lib` (S1; it installs its own shared library) |
| Desktop entry | `Exec` is replaced | `TryExec` and `Exec` are replaced |
| Policies in the scripts | Not set (no boolean constants are used, so no problem arises) | Set by `cmake_minimum_required` (S9) |

## 5. Steps

1. Make a working tree from develop and write the files above (done).
2. Test locally (done; §6).
3. Put the work on `feature/cmake-helper`, which the user creates. Commit and push only after the user approves (done; §6).
4. Check CI (done; §6).
5. Propose the PR title and description. The user opens the PR to develop on GitHub and Squash Merges it.
6. After the merge, merge develop into `feature/3d-view` and record it in the 3D view plan (the renamed desktop entry; the tarball item of Phase 10).

## 6. Record

### 2026-11-21: Implementation and local tests

The work was done in a working tree made from develop (a6df084). The tests ran in a throwaway git repository where the changes were committed. To leave the user's environment unchanged, they ran with HOME set to a temporary directory and without `LD_LIBRARY_PATH` (§0.1 of the 3D view plan).

- `test qt6`: 15/15 passed.
- `run qt6 -- --version`: printed `rcssmonitor-19.0.1`.
- `install qt6 --prefix <temporary directory>`: 15 files were installed. RUNPATH is `$ORIGIN/../lib`. Without LD_LIBRARY_PATH, `librcssrcg.so.18` is found in the installed `lib`. The desktop entry's `TryExec`/`Exec` have the absolute installed path, and it passed `desktop-file-validate`.
- `install qt6` (the default prefix): installed into `$HOME/.local`. After `uninstall qt6` no file remained, and `include/rcss/rcg` and `include/rcss` were removed too.
- `dist qt6`: made `rcssmonitor-19.0.1.tar.gz`, which extracts to `rcssmonitor-19.0.1/`. The `export-ignore` files are not in it. Uncommitted changes give a warning. Running `dist` in an extracted tarball (not under git) is an error.
- `distcheck qt6`, `distcheck qt5` (the system's Qt5), and `distcheck qt6` without DISPLAY and with offscreen (as on CI): all passed.
- `auto` with `CMAKE_ARGS="-DENABLE_WEBSOCKET=OFF"`: configured `build` with Qt6 and without WebSocket.
- Errors: an unknown command, `all` with a command other than `clean`, `uninstall` in a build directory that has installed nothing, and an unknown option of `install` all exit with code 1. `clean all` keeps a `build` that is not a CMake build directory and exits with code 1.
- The syntax was checked by `dash -n` (shellcheck is not installed).

Problems found and their fixes:

- **An empty `include/rcss` was left after `uninstall`**: in the script mode of `cmake -P`, the policy CMP0012 is not set, so `TRUE` in `while(TRUE)` was taken as a variable name and was false. The three scripts now call `cmake_minimum_required(VERSION 3.16)` at the top (S9).
- **`.gitignore` does not show up as a file to commit**: the user's `~/.gitignore` ignores `.gitignore` files. The commit uses `git add -f .gitignore` (S7).
- (Environment) When an anaconda environment is first in PATH, CMake finds its Qt5. This is not a problem of the scripts. The test with the system's Qt5 ran with anaconda removed from PATH.

### 2026-11-21: Commit and CI

- The user created `feature/cmake-helper` on GitHub (at a6df084, the same as develop). The working tree was switched to this branch, and the work was committed after the user approved, and pushed (a7e6880 "Add cmake-helper.sh for the frequently used CMake commands"). At the user's request, the commit message does not mention soccerwindow2.
- All five CI jobs passed (CMake Qt6, CMake Qt6 (Ubuntu 22.04), CMake Qt5, autotools Qt5, and the new CMake distcheck Qt6). The distcheck job went through making the tarball, extracting, building, testing (15/15), installing, validating the desktop entry, starting the installed program, and uninstalling.

### Still to do

- The user opens the PR to develop on GitHub and Squash Merges it.
- On develop, the 3D view plan is still `docs/PLAN_3D-VIEW*.md`, which is not `export-ignore`d. When `feature/3d-view` is merged into develop it moves into `docs/plan`, and is left out from then on.
- When develop is merged into master, `.gitignore` is expected to conflict (add/add). The new file contains master's lines, so take the new file.

### 2026-11-24: The header table and the table of the progress (the unified format)

- Following the unified format of the plans ("計画書の書式" of CLAUDE.md; the record of 2026-11-24 of the plan of the URLs and the lists of logs), the last update was added to the header table, the status was shortened, and the table of the progress was put. The chapters are not changed.
- The PR and the merge of "What remains" were done on 2026-11-21. The user merged PR #69 into develop by Squash Merge (c392514), and develop into `feature/3d-view` (§14 of the 3D view plan).
