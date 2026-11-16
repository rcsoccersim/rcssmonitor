# The Mannequin: a figure pack of the 3D view

A figure pack bundled with rcssmonitor (`docs/FIGURE_PACKS.md`): the
Mannequin, all in the player's color but its joints, with the motion of the
human figures. `figure.json` tells the 3D view how to draw it; the Human
pack (`../human`) moves with the same motion.

- `Mannequin.glb`: the Mannequin and four animations (Idle_Loop, Walk_Loop,
  Jog_Fwd_Loop, Sprint_Loop) of the Universal Animation Library [Standard] by
  [Quaternius](https://quaternius.com/packs/universalanimationlibrary.html),
  CC0 1.0. It is the `Mannequin.glb` of rcssmonitor-web 20.0.0-beta.2
  (`src/view/render3d/figures/models/`, trimmed from `UAL1_Standard.glb`
  with glTF-Transform), written again without EXT_meshopt_compression and
  KHR_mesh_quantization, which a desktop application does not need to save
  space for: 346 kB to 684 kB.
- `decompress.mjs`: the script that wrote it, with glTF-Transform 4.5.1 and
  meshoptimizer:

  ```bash
  npm install @gltf-transform/core@4 @gltf-transform/extensions@4 @gltf-transform/functions@4 meshoptimizer
  node decompress.mjs <rcssmonitor-web>/src/view/render3d/figures/models/Mannequin.glb Mannequin.glb
  ```

The model has one mesh with two materials (M_Main, M_Joints), a skin of
65 joints, and the four animations, each with the translation, rotation
and scale of every joint.

- `WalkBack.bvh`, `JogBack.bvh`, `WalkRight.bvh`, `JogLeft.bvh`: one cycle
  each of walking and jogging backward and sideways, from the style
  Neutral of [the 100STYLE dataset](https://www.ianxmason.com/100style/)
  by Ian Mason, [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/):
  cut from `Neutral_BW.bvh`, `Neutral_BR.bvh`, `Neutral_SW.bvh` and
  `Neutral_SR.bvh`, looped (the end made to meet the start), and resampled
  to about 30 frames a second. The figures mirror them for the other side
  (walking to the left, jogging to the right).
- `cut_100style.py`: the script that cut them from the dataset's BVH files
  (<https://zenodo.org/records/8127870>), with Python's standard library:

  ```bash
  python3 cut_100style.py 100STYLE.zip .
  ```

  The cycles it cuts are in the script, and in `docs/FIGURE_ASSETS.md`.
- `Actions.glb`: the skeleton (no mesh) and three animations of the
  [Universal Animation Library 2](https://quaternius.com/packs/universalanimationlibrary2.html)
  [Standard] by Quaternius, CC0 1.0, from its file without root motion
  (`UAL2_Standard.glb`): `Slide_Start` (the sliding tackle), `Slide_Exit`
  (getting up from it) and `LayToIdle` (getting up from the ground, after a
  foul). Their skeleton is the Mannequin's. A channel that keeps its node at
  rest is left out.
- `pack_ual2_actions.py`: the script that wrote it (Python's standard
  library only):

  ```bash
  python3 pack_ual2_actions.py "Unreal-Godot/UAL2_Standard.glb" Actions.glb
  ```
