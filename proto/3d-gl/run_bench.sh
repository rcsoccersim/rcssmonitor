#!/bin/bash
# Measure the 3D prototypes (docs/PLAN_3D-VIEW.md, Phase 0, D1) one by one,
# each for 10 s after 2 s, at 1920x1080 device pixels, and print the results.
#
# usage: proto/3d-gl/run_bench.sh <build directory (Qt6)> <game log> [<build directory (Qt5)>]
#
# The windows appear one after another; do not touch them while measuring.

set -u
build6=$1
log=$2
build5=${3:-}
seconds=10
# the prototypes do not read ~/.rcssmonitor.conf, but keep it safe anyway
export HOME=$(mktemp -d)

run() {
    echo "== $*" >&2
    "$@" --bench $seconds --size 1920x1080 "$log" 2>/dev/null | grep -E '^(BENCH|GL_RENDERER|GL_VERSION)'
    sleep 1
}

run "$build6/proto_gl"
run "$build6/proto_gl" --no-vsync
run "$build6/proto_gl" --no-vsync --no-shadows
run "$build6/proto_gl" --no-vsync --cylinders
if [ -n "$build5" ]; then
    run "$build5/proto_gl" --no-vsync
fi
if [ -x "$build6/proto_quick3d" ]; then
    run "$build6/proto_quick3d"
    run "$build6/proto_quick3d" --no-vsync
    run "$build6/proto_quick3d" --no-vsync --rotations-only
    run "$build6/proto_quick3d" --no-vsync --window
    run "$build6/proto_quick3d" --no-vsync --no-shadows
    run "$build6/proto_quick3d" --no-vsync --cylinders
fi
