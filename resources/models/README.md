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

# The human bodies

- `BaseCharacterMale.glb`, `BaseCharacterFemale.glb`: the male and the
  female bodies (`Superhero_Male_FullBody`, `Superhero_Female_FullBody`)
  of the Universal Base Characters [Standard] by
  [Quaternius](https://quaternius.com/packs/universalbasecharacters.html),
  CC0 1.0 (`License_Standard.txt` in the pack), with their eyebrows and
  eyes, and the hairstyles of the same pack (rigged to the head bone) made
  for each body's head: `Hair_SimpleParted`, `Hair_Buzzed` and `Hair_Beard`
  for the man, `Hair_Buns`, `Hair_Long` and `Hair_BuzzedFemale` for the
  woman. The 3D view draws them with the Mannequin's clips: they have the
  same 65 joints (the same names and order), and no clips of their own.
  Each player wears one of its body's hairstyles (and a man a beard or
  not; `FigureLooks`).
- `pack_base_character.py`: the script that wrote them from the pack's glTF
  files (Python's standard library only):

  ```bash
  B="Universal Base Characters[Standard]/Base Characters/Godot - UE"
  H="Universal Base Characters[Standard]/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)"
  python3 pack_base_character.py BaseCharacterMale.glb "$B/Superhero_Male_FullBody.gltf" \
      "$H/Hair_SimpleParted.gltf" "$H/Hair_Buzzed.gltf" "$H/Hair_Beard.gltf"
  python3 pack_base_character.py BaseCharacterFemale.glb "$B/Superhero_Female_FullBody.gltf" \
      "$H/Hair_Buns.gltf" "$H/Hair_Long.gltf" "$H/Hair_BuzzedFemale.gltf"
  ```

  It keeps the skeleton and, of the meshes, the positions, the normals,
  the joints, the weights and the triangles (585 kB and 858 kB). The
  textures, the other texture coordinates and the vertex colors are left
  out. The materials are named for the 3D view: `Body` (painted with a
  uniform and the player's skin, `FigureUniform`), `Eyebrows` and
  `Hair_<style>` (in the player's hair color), and `Eyes` (in the color
  that the script sets).

The man's body has six meshes (the body, the eyebrows, the eyes, three
hairstyles), about 17,500 triangles; the woman's about 22,100.
