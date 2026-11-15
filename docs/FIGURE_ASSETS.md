# The figures' material: for maintainers

How the models and motions of the 3D view's figures are made, converted
and tested. For users, see [FIGURE_PACKS.md](FIGURE_PACKS.md).

## The bundled packs

Both are in `resources/figures/`, each with a README telling its source
and license (CC0 1.0):

- `mannequin/`: `Mannequin.glb`, the Mannequin and four clips of
  Quaternius's Universal Animation Library [Standard], written without
  mesh compression by `decompress.mjs` (glTF-Transform).
- `human/`: `BaseCharacterMale.glb` and `BaseCharacterFemale.glb`, bodies
  and hairstyles of Quaternius's Universal Base Characters [Standard],
  written by `pack_base_character.py` (Python's standard library only).

A bundled model must look and move the same in every release: the tests
compare what is drawn (`test_scene_3d`, `test_frame_builder_3d`).

## Formats read

| Format | Read by | Notes |
|---|---|---|
| glTF 2.0 (`.glb`, `.gltf`), VRM 0.x and 1.0 (`.vrm`) | cgltf (`third_party/cgltf`), `src/view3d/gltf_model.cpp` | No mesh compression (Draco, meshopt) |
| FBX (`.fbx`, binary or ASCII) | ufbx (`third_party/ufbx`), `src/view3d/fbx_model.cpp` | Converted to y up, +z ahead, meters |

Both are read into the same plain data (`GltfModel`). The bones go
through the humanoid roles (`HumanoidRig`): the poses of one skeleton play
on another, and a clip of another file is put on the motion's skeleton
(`retargetClip()`), in place.

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
