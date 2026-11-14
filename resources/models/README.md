# The model of the human figures

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
