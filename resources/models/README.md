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

# The human body

- `BaseCharacter.glb`: the male body (`Superhero_Male_FullBody`) of the
  Universal Base Characters [Standard] by
  [Quaternius](https://quaternius.com/packs/universalbasecharacters.html),
  CC0 1.0 (`License_Standard.txt` in the pack), with its eyebrows and eyes,
  and the hairstyle `Hair_SimpleParted` of the same pack (rigged to the
  head bone). The 3D view draws it with the Mannequin's clips: it has the
  same 65 joints (the same names and order), and no clips of its own.
- `pack_base_character.py`: the script that wrote it from the pack's glTF
  files (Python's standard library only):

  ```bash
  python3 pack_base_character.py \
      "Universal Base Characters[Standard]/Base Characters/Godot - UE/Superhero_Male_FullBody.gltf" \
      "Universal Base Characters[Standard]/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/Hair_SimpleParted.gltf" \
      BaseCharacter.glb
  ```

  It keeps the skeleton and, of the meshes, the positions, the normals,
  the joints, the weights and the triangles (720 kB of the body and 51 kB
  of the hair, with the textures, to 524 kB). The textures, the other
  texture coordinates and the vertex colors are left out: the 3D view
  paints the body's material (`MI_Superhero_Male`) with a uniform and the
  skin (`FigureUniform`), and the hair, the eyebrows (`MI_Hair_1`) and the
  eyes (`MI_Eyes`) in the colors that the script sets.

The body has four meshes (the body, the eyebrows, the eyes, the hair) in
three materials, about 15,600 triangles, and a skin of 65 joints.
