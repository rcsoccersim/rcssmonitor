# The figures' material: for maintainers

How the models and motions of the 3D view's figures are made, converted
and tested. For users, see [FIGURE_PACKS.md](FIGURE_PACKS.md).

## The bundled packs

Both are in `resources/figures/`, each with a README telling its source
and license:

- `mannequin/`: `Mannequin.glb`, the Mannequin and four clips of
  Quaternius's Universal Animation Library [Standard] (CC0 1.0), written
  without mesh compression by `decompress.mjs` (glTF-Transform); and four
  clips of walking and jogging backward and sideways cut from the 100STYLE
  dataset (Ian Mason, CC BY 4.0: the credits tell it, and what was done to
  it), by `cut_100style.py` (below); and `Actions.glb`, three clips of
  Quaternius's Universal Animation Library 2 [Standard] (CC0 1.0) for the
  actions, by `pack_ual2_actions.py` (below).
- `human/`: `BaseCharacterMale.glb` and `BaseCharacterFemale.glb`, bodies
  and hairstyles of Quaternius's Universal Base Characters [Standard],
  written by `pack_base_character.py` (Python's standard library only).

A bundled model must look and move the same in every release: the tests
compare what is drawn (`test_scene_3d`, `test_frame_builder_3d`).

### The clips of 100STYLE

`WalkBack.bvh`, `JogBack.bvh`, `WalkRight.bvh` and `JogLeft.bvh` are one
cycle each of the style Neutral of 100STYLE
(<https://www.ianxmason.com/100style/>, the BVH files of
<https://zenodo.org/records/8127870>), cut by
`resources/figures/mannequin/cut_100style.py` (Python's standard library
only):

```bash
python3 resources/figures/mannequin/cut_100style.py 100STYLE.zip resources/figures/mannequin
```

| File | From | Frames (60 fps) | Cycle | Speed | Way from facing |
|---|---|---|---|---|---|
| `WalkBack.bvh` | `Neutral_BW.bvh` | 6514 to 6585 | 1.18 s | 0.56 m/s | 179° |
| `JogBack.bvh` | `Neutral_BR.bvh` | 2755 to 2795 | 0.67 s | 1.00 m/s | 174° |
| `WalkRight.bvh` | `Neutral_SW.bvh` | 5064 to 5133 | 1.15 s | 0.51 m/s | 88° to the right |
| `JogLeft.bvh` | `Neutral_SR.bvh` | 547 to 591 | 0.73 s | 1.44 m/s | 88° to the left |

Each cycle runs from the left foot coming down to it coming down again,
where the actor went straight the clip's way: of the cycles found from the
feet and the hips, the one nearest to the clip's way with the least
turning (the room is small, so the actor turns often). The script loops
it (what each channel is off at the end from the start, but where the
hips go across the floor, taken off in a line over the cycle), resamples
it at as many frames as fit the cycle near 30 a second, and writes it with
the file's own skeleton. The pack tells one clip of each side; the other
side's is mirrored (`mirrorClip()`). The speeds are of the actor
(normalized for a figure 1.85 m high, rcssmonitor reads 0.54, 1.00, 0.42
and 1.36 m/s): the actor backpedals and shuffles with the feet low (they
lift 3 to 10 cm), so a foot counts as lifted from 1 % of the figure's
height (`FOOT_LIFT` in `figures_3d.cpp`; 2 % before).

### The clips of the actions

`Actions.glb` is the skeleton (no mesh, no skin) and three clips of the
Universal Animation Library 2 [Standard]
(<https://quaternius.com/packs/universalanimationlibrary2.html>, its file
without root motion, `Unreal-Godot/UAL2_Standard.glb`), written by
`resources/figures/mannequin/pack_ual2_actions.py` (Python's standard
library only):

```bash
python3 resources/figures/mannequin/pack_ual2_actions.py \
    UAL2_Standard.glb resources/figures/mannequin/Actions.glb
```

| Clip | Length | Played as |
|---|---|---|
| `Slide_Start` | 0.83 s | `tackle`: from running, feet first onto the ground (on it at about 0.24 s, its moment) |
| `Slide_Exit` | 0.50 s | `tackle_up`: getting up from the slide |
| `LayToIdle` | 1.53 s | `get_up`: getting up from lying on the back (after a foul, from its last part) |

A channel keeping its node at rest all through a clip is left out
(120 kB). Its skeleton is the Mannequin's (the same 65 bones). Its
`Hit_Knockback` (a fall back) is not taken: the figure flies back with its
feet higher than a meter, unlike a player tripped by a foul, who keeps the
pose made by `Figures3D` (sitting down). The library has no kick, catch
or dive in its Standard part.

## Formats read

| Format | Read by | Notes |
|---|---|---|
| glTF 2.0 (`.glb`, `.gltf`), VRM 0.x and 1.0 (`.vrm`) | cgltf (`third_party/cgltf`), `src/view3d/gltf_model.cpp` | No mesh compression (Draco, meshopt) |
| FBX (`.fbx`, binary or ASCII) | ufbx (`third_party/ufbx`), `src/view3d/fbx_model.cpp` | Converted to y up, +z ahead, meters |
| BVH (`.bvh`) | `src/view3d/bvh_model.cpp` | A skeleton and one clip, no mesh; its unit and axes as written |
| VRM Animation (`.vrma`) | cgltf, `src/view3d/gltf_model.cpp` | The roles of `VRMC_vrm_animation`; no mesh |

All are read into the same plain data (`GltfModel`). The bones go
through the humanoid roles (`HumanoidRig`): the poses of one skeleton play
on another, and a clip of another file is put on the motion's skeleton
(`retargetClip()`), in place. A skeleton is stood upright (the axis
nearest to the way from its hips to its head taken as up) and turned to
face +z when its roles are found (`rigModel()` in `figures_3d.cpp`): a BVH
file may be written z up or facing any way.

BVH sets no unit and no axes. The rest of its joints is their `OFFSET`s,
not turned; its rotation channels are composed in the order written
(`Zrotation Xrotation Yrotation`: R = Rz Rx Ry), as Blender's BVH
importer reads them; a position channel replaces the `OFFSET`. A BVH
model is marked a capture (`GltfModel::capture`): its root is placed in
the room it was taken in (100STYLE: the root's `OFFSET` at the origin, the
hips about 0.97 m up and meters across in the positions). `settleCapture()`
(`humanoid.cpp`, after the model is stood upright) puts its rest on y = 0
above the origin, and moves and turns its clip to start above the origin,
facing +z (the mean heading of its hips), coming down to y = 0: by the
highest of the hips and the bones above them that the clip moves, the
ground measured on the hips and the bones below them.

The reading was checked against Blender's (5.3): 400 frames of
100STYLE's `Neutral_FW.bvh` imported by Blender and exported as glTF put
every joint within 0.1 cm of where rcssmonitor reads the BVH file, in
every frame (Blender's glTF starts its keys one frame later).

## Converting an FBX file to glTF with Blender

rcssmonitor reads FBX files as they are. To edit a model (its materials,
its mesh) as glTF, convert it with Blender (tested with Blender 5.3):

```bash
blender -b --factory-startup --python tools/fbx_to_glb.py -- Character.fbx Character.glb
```

The script imports with the leaf bones kept (`ignore_leaf_bones=False`:
a model's head or toes may be leaf bones, and losing them leaves the
skeleton without its roles) and the bones as they are
(`automatic_bone_orientation=False`), and exports a GLB with y up and the
animations, without Draco compression. In Blender's window, the same
options are under File > Import > FBX (Armature: Ignore Leaf Bones off,
Automatic Bone Orientation off) and File > Export > glTF 2.0 (Format glTF
Binary; Include: Animation; Compression off).

Keep a file of clips without a skin (a Mixamo animation downloaded
"Without Skin") as FBX: its rest pose is not in the file, and rcssmonitor
takes it from the motion's model only for FBX files (`restAs()`). A glTF
file written from it would keep the pose it was saved in as its rest.

## The test files

`test/data/fbx/` holds small FBX files made from the bundled Mannequin by
`test/data/fbx/make_fbx.py` with Blender:

```bash
blender -b --factory-startup --python test/data/fbx/make_fbx.py -- \
    resources/figures/mannequin/Mannequin.glb test/data/fbx
```

- `figure.fbx`: the body (fewer triangles, a texture embedded), its role
  bones named as Mixamo's, written as Blender writes (the root nodes
  scaled 100 times, the unit 1 cm).
- `walk.fbx`: the skeleton of the role bones only, the walk going forward
  (not in place), in centimeters as Mixamo writes.
- `plane.fbx`, `Holes.png`: a square on a moved node, its texture beside
  it with transparent pixels; `plane_zup.fbx`, the same written z up.

Blender writes a date into each file, so the files change whenever the
script runs; run it only when the files must change.

`test/data/bvh/walk.bvh` is the Mannequin's walk written by Blender's BVH
exporter, made by `test/data/bvh/make_bvh.py`:

```bash
blender -b --factory-startup --python test/data/bvh/make_bvh.py -- \
    resources/figures/mannequin/Mannequin.glb test/data/bvh
```

It has the role bones under Blender's `root`, z up (Blender's axes), in
centimeters, the rotations Y X Z, positions on every joint.

The other BVH and VRMA files of the tests are written by the tests
themselves from the Mannequin (`test/test_bvh.h`: its role bones, not
turned at rest, any unit, axes, rotation order and placing in a capture
room; `TestGltf::animationOf()` in `test/test_gltf.h`). Played on the
Mannequin, all of them must give its own clips back.
