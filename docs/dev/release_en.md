# Releasing rcssmonitor (for maintainers)

| Item | Content |
|---|---|
| For | Maintainers who make an official release of rcssmonitor |
| Japanese version | [release.md](release.md) (same content) |
| Distribution | This document is not in the distribution (tarball) |

Check these in this order at every official release (betas included).

## 0. Not to forget at the next release (as of 2026-11-22)

- The version is 20.0.0-beta.1, but it has not been released. `feature/3d-view` goes as far as develop, and more features are to come before the official release.
- **20.0.0 gets the features of `docs/plan/2026-11-22-00_v20-features_en.md`** (F1 to F6: the color scheme, the icons, the desktop, the referee's whistle, the URL and the log lists, the images and the video). The user may decide on the way how many of them go into the release (§4 there).
- **The date of the 20.0.0-beta.1 entry at the top of ChangeLog (2026-11-22) is a placeholder.** Set it to the release date.
- **Add the URL of the browser version (rcssmonitor-web)** to the README and NEWS. Expected to be `https://rcsoccersim.github.io/rcssmonitor-web/` after it moves to the rcsoccersim organization. Write it after checking that it is published.
- **In the first merge into master (after 19.0.1)**, delete the AppImage workflow `.github/workflows/main.yml` and `utils/appimage/` (the AppImage was dropped; D10 of the plan). `.gitignore` was added on both master and develop, so it conflicts (add/add): take develop's (which contains master's two lines).

## 1. Deciding the version

- `CMakeLists.txt`:
  - The numbers of `project(rcssmonitor VERSION X.Y.Z)`.
  - `RCSSMONITOR_VERSION_SUFFIX`: `beta.N` for a beta, empty (`""`) for a release. `--version`, the window title and the tarball's name become `X.Y.Z-beta.N` (`X.Y.Z` if empty).
- `configure.ac`: the version of `AC_INIT`, the same string as above (e.g. `20.0.0-beta.1`).
- The rcg library (`librcssrcg`): when the structs or functions of its installed headers are no longer compatible with the last release (a struct got members, a type changed and so on), raise its soname:
  - `VERSION` and `SOVERSION` in `rcss/rcg/CMakeLists.txt`.
  - `-version-info CURRENT:REVISION:AGE` in `rcss/rcg/Makefile.am` (the soname is CURRENT - AGE).
  - Keep the soname the same in CMake and autotools. It is 20 for 20.0.0 (`BallT` and `ServerParamT` got members since 19.0.1).

## 2. What is shown, and the documents

- The copyright year in `src/main.cpp` (`2009 - YYYY`): the year of the release.
- `NEWS`: the heading `[version]`, and whether the changes are all there.
- `ChangeLog`: an entry with the release date (in the same form as before: date, name, files, version).
- `README.md`: whether the build steps and the pointers (the browser version and so on) are current.
- `docs/`: whether the documents for users (`build.md`, `3d-view.md`, `FIGURE_PACKS.md` and others) match what it does now.

## 3. Checking

- The CI of develop passes in all its jobs.
- `./cmake-helper.sh distcheck qt6` and `./cmake-helper.sh distcheck qt5`: the tarball builds, tests, installs and uninstalls; the installed program tells `rcssmonitor-version`.
- On screen: 2D and 3D, log replay, live (UDP, TCP, WebSocket), the delayed live view, the figure packs.
- By ear: the referee's whistle, with the Qt6 and the Qt5 builds (with `libqt5multimedia5-plugins`), in log replay and live (the kick-offs, a foul and its restart, the three long whistles at the end of the game, after going back, Test of the settings dialog). The tests cannot check the sound.

## 4. Merging and the tag

- Merge develop into master by a PR on GitHub (Merge Commit). Do not forget the care of the first merge in §0.
- Tag master: `rcssmonitor-X.Y.Z` (a beta `rcssmonitor-X.Y.Z-beta.N`), in the form of the tags so far (`rcssmonitor-19.0.1` and others).

## 5. Making the release

- At the tagged commit, with a clean working tree, run `./cmake-helper.sh dist` to make `rcssmonitor-version.tar.gz` (made from the commit of HEAD by `git archive`; without `docs/plan/`, `docs/dev/` and the CI's settings).
- Make the GitHub release from the tag. Write its notes from NEWS, and point to building from the source (`docs/build.md`) and to the browser version. Attach the tarball. No AppImage is made any more.
- Mark a beta as a "pre-release".
