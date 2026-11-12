# Prototypes of the 3D view

Phase 0 of `docs/PLAN_3D-VIEW.md`: two prototypes to decide D1, the way to
draw the 3D view. They draw the same scene: the pitch, 22 human figures
walking and running by a game log, the ball, and the shadows of the sun.
Not part of the application; this branch (`proto/3d-gl`) is not merged.

- `proto_gl`: OpenGL 3.3 core in a `QOpenGLWidget`, with our own shaders
  (the recommended way). Qt 5.15 and Qt 6. The skin matrices of the 22
  figures are in a float texture, and the figures are drawn with one
  instanced call per material. The shadow map is 4096 px with a 5 x 5 PCF.
- `proto_quick3d`: Qt Quick 3D in a `QQuickWidget` (or a `QQuickView` with
  `--window`). Qt 6 only. The model is converted by balsam at build time
  (`make_quick3d_model.py`), and its joint nodes are moved every frame
  through their properties.

Both compute the poses from the log every frame, the same way
(`gltf_rig.cpp`, `proto_log.cpp`; plan §5.8): the idle, walk, run and
sprint clips mixed by the speed, each at the time of the distance walked.
The model is `resources/models/Mannequin.glb`.

## Build

```bash
cmake -S . -B build-proto -DCMAKE_BUILD_TYPE=Release -DBUILD_PROTO_3D=ON -DQT_VERSION_MAJOR=6
cmake --build build-proto --target proto_gl proto_quick3d
# proto_gl with Qt 5 too
cmake -S . -B build-proto5 -DCMAKE_BUILD_TYPE=Release -DBUILD_PROTO_3D=ON -DQT_VERSION_MAJOR=5
cmake --build build-proto5 --target proto_gl
```

`proto_quick3d` needs `qt6-declarative-dev`, `qt6-quick3d-dev`,
`qt6-quick3d-dev-tools` (balsam), `qt6-shadertools-dev` and the QML modules
(`qml6-module-qtquick`, `qml6-module-quick3d`, ...), and Python 3.

## Run

```bash
build-proto/proto_gl 1-json.rcg.gz
build-proto/proto_quick3d 1-json.rcg.gz
```

Options (both): `--no-vsync`, `--bench <seconds>` (measure after 2 s, print
a `BENCH` line, and quit), `--size <W>x<H>` (device pixels, 1920x1080),
`--cylinders`, `--no-shadows`, `--start <frame>`, `--screenshot <png>`.
`proto_gl`: F switches figures and cylinders, S the shadows, Space pauses,
and the mouse moves the camera. `proto_quick3d`: `--window`,
`--rotations-only` (write only the rotations of the joints).

`run_bench.sh <build-proto> <log> [<build-proto5>]` measures them one after
another.

Without a window (to check the drawing on a machine without a display):
`--offscreen <frames>` draws into an offscreen framebuffer, e.g.

```bash
QT_QPA_PLATFORM=offscreen LIBGL_ALWAYS_SOFTWARE=1 build-proto/proto_gl --offscreen 30 --screenshot gl.png 1-json.rcg.gz
QT_QUICK_BACKEND=rhi QT_QPA_PLATFORM=offscreen LIBGL_ALWAYS_SOFTWARE=1 build-proto/proto_quick3d --offscreen 30 --screenshot q3d.png 1-json.rcg.gz
```
