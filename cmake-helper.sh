#!/bin/sh
#
# cmake-helper.sh -- shortcuts for the frequently used CMake commands of rcssmonitor
#
# Run "./cmake-helper.sh help" for the usage.
# The commands are printed before they are run, so that you can learn the
# actual cmake commands.

set -e

usage()
{
    cat <<'EOF'
Usage: ./cmake-helper.sh COMMAND [QT] [OPTIONS]

Commands:
  configure [QT]                 (re)configure the build directory
  build [QT]                     configure (if needed) and build
  test [QT]                      build and run the tests
  run [QT] [-- ARGS...]          build and run rcssmonitor with ARGS
  install [QT] [--prefix DIR]    build and install (default prefix: $HOME/.local)
  uninstall [QT]                 remove the files installed from the build directory
  dist [QT]                      create the source tarball rcssmonitor-VERSION.tar.gz
                                 in the project root (from the committed files of HEAD)
  distcheck [QT]                 create the source tarball and check that it can be
                                 built, tested, installed, and uninstalled
  clean [QT|all]                 remove the build directory
  help                           show this help

QT selects the Qt version and the build directory:
  auto (default)  Qt6 if available, otherwise Qt5   build directory: build
  qt6             Qt6                                build directory: build-qt6
  qt5             Qt5                                build directory: build-qt5

Environment variables:
  CMAKE_ARGS  additional options for configure, e.g.
              CMAKE_ARGS="-DENABLE_WEBSOCKET=OFF" ./cmake-helper.sh build
              (configure is run again when CMAKE_ARGS is set)
  JOBS        the number of parallel build jobs (default: the number of CPUs)

The main commands that are run (for QT=qt6):
  configure   cmake -S . -B build-qt6 -DQT_VERSION_MAJOR=6
  build       cmake --build build-qt6 --parallel JOBS
  test        cd build-qt6 && ctest --output-on-failure
  run         build-qt6/rcssmonitor ARGS...
  install     cmake --install build-qt6 [--prefix DIR]
  uninstall   cmake --build build-qt6 --target uninstall
  dist        cmake --build build-qt6 --target dist
  distcheck   cmake --build build-qt6 --target distcheck
  clean       rm -rf build-qt6
EOF
}

die()
{
    echo "cmake-helper.sh: $*" >&2
    exit 1
}

# print a command and run it
run_cmd()
{
    printf '+'
    for arg in "$@"; do
        case "$arg" in
            *[[:space:]]*|'') printf " '%s'" "$arg" ;;
            *) printf ' %s' "$arg" ;;
        esac
    done
    printf '\n'
    "$@"
}

# move to the project root (the directory of this script)
cd "$(dirname "$0")"

command="${1:-help}"
[ $# -gt 0 ] && shift

case "$command" in
    help|-h|--help)
        usage
        exit 0
        ;;
    configure|build|test|run|install|uninstall|dist|distcheck|clean)
        ;;
    *)
        usage >&2
        die "unknown command: $command"
        ;;
esac

# Qt version and build directory
qt=auto
case "${1:-}" in
    qt5|qt6|auto|all)
        qt="$1"
        shift
        ;;
esac
case "$qt" in
    qt5) build_dir=build-qt5; qt_option=-DQT_VERSION_MAJOR=5 ;;
    qt6) build_dir=build-qt6; qt_option=-DQT_VERSION_MAJOR=6 ;;
    auto) build_dir=build; qt_option= ;;
    all)
        [ "$command" = clean ] || die "'all' can be used only with clean"
        ;;
esac

if [ -z "${JOBS:-}" ]; then
    JOBS=$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
fi

# configure if the build directory is not configured, or if forced or CMAKE_ARGS is set
configure()
{
    if [ "${1:-}" = force ] || [ -n "${CMAKE_ARGS:-}" ] || [ ! -f "$build_dir/CMakeCache.txt" ]; then
        # CMAKE_ARGS and qt_option are split into words intentionally.
        # shellcheck disable=SC2086
        run_cmd cmake -S . -B "$build_dir" $qt_option ${CMAKE_ARGS:-}
    fi
}

build()
{
    configure
    run_cmd cmake --build "$build_dir" --parallel "$JOBS"
}

# remove a build directory only if it is a CMake build directory
clean_dir()
{
    dir="$1"
    if [ ! -e "$dir" ]; then
        echo "$dir does not exist."
    elif [ ! -f "$dir/CMakeCache.txt" ]; then
        echo "$dir is not a CMake build directory (no CMakeCache.txt). It is not removed." >&2
        return 1
    else
        run_cmd rm -rf -- "$dir"
    fi
}

case "$command" in
    configure)
        configure force
        ;;
    build)
        build
        ;;
    test)
        build
        echo "+ cd $build_dir && ctest --output-on-failure"
        (cd "$build_dir" && ctest --output-on-failure)
        ;;
    run)
        [ "${1:-}" = "--" ] && shift
        build
        run_cmd "$build_dir/rcssmonitor" "$@"
        ;;
    install)
        prefix=
        while [ $# -gt 0 ]; do
            case "$1" in
                --prefix) [ $# -ge 2 ] || die "--prefix requires a directory"; prefix="$2"; shift 2 ;;
                --prefix=*) prefix="${1#--prefix=}"; shift ;;
                *) die "unknown option for install: $1" ;;
            esac
        done
        build
        if [ -n "$prefix" ]; then
            run_cmd cmake --install "$build_dir" --prefix "$prefix"
        else
            run_cmd cmake --install "$build_dir"
        fi
        ;;
    uninstall)
        # The files listed in install_manifest.txt of the build directory are removed.
        [ -f "$build_dir/install_manifest.txt" ] \
            || die "$build_dir/install_manifest.txt is not found. Nothing has been installed from $build_dir."
        run_cmd cmake --build "$build_dir" --target uninstall
        ;;
    dist|distcheck)
        configure
        run_cmd cmake --build "$build_dir" --target "$command"
        ;;
    clean)
        if [ "$qt" = all ]; then
            status=0
            for dir in build build-qt5 build-qt6; do
                clean_dir "$dir" || status=1
            done
            exit $status
        fi
        clean_dir "$build_dir"
        ;;
esac
