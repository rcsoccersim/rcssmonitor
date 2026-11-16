# Figure packs

The 3D view of rcssmonitor can draw the players as human figures. The
models and the motion of the figures come in *figure packs*. Two packs are
bundled with rcssmonitor:

- **Mannequin** (`mannequin`): the Mannequin of Quaternius's Universal
  Animation Library, all in the team color but its joints. Its motion has
  clips of walking and jogging backward and sideways too, cut from the
  100STYLE dataset (Ian Mason, CC BY 4.0).
- **Human** (`human`): men and women of Quaternius's Universal Base
  Characters in a uniform. Each player has its own body, hairstyle, and
  skin and hair colors. They move with the Mannequin's motion.

You can add packs of your own, for example with models that cannot be
bundled because of their licenses.

This document describes format 1 of `figure.json`. In this version a pack
can use glTF 2.0 models (`.glb`, `.gltf`, and VRM 0.x and 1.0 `.vrm`) and
FBX models (`.fbx`, as Mixamo gives them) with their textures, whose bones
are named as Quaternius's, Mixamo's or VRoid's rigs or 100STYLE's motion
capture, or have VRM's roles, or are mapped by the pack. The clips of the
motion may come from other files, of other skeletons too, including motion
capture (`.bvh`) and VRM Animation (`.vrma`) files.

To make a pack of a character and animations of Mixamo, see
[FIGURE_PACKS_MIXAMO.md](FIGURE_PACKS_MIXAMO.md).

## Choosing a pack

In the settings dialog (View > Preference, Ctrl+V), open the 3D tab. Under
Players, choose the pack in **Model**. The line below the list shows the
pack's credits; the full credits are in its tooltip. The **Packs** button
does the following:

- **Choose a Folder of Packs...**: adds a folder of packs of your choice.
- **No Folder of Packs**: removes that folder.
- **Open the User's Folder of Packs**: opens your data folder of packs,
  and creates it if it does not exist.
- **Look for Packs Again**: reads the packs again, after you change one.
- **Move a Pack to the Trash**: lists your packs (not the bundled ones).
  After you confirm, the chosen pack's folder, with all the files in it,
  is moved to the trash, from where you can restore it. If it was the
  pack in **Model**, the players are drawn as cylinders; if it was the
  folder of your choice, that folder is no longer read.

The dialog also reads the packs again each time it is shown.

rcssmonitor reads a pack's files where they are and keeps no copy of
them. Once its folder is gone (moved to the trash, or deleted by hand),
nothing of the pack is left but its name in the settings, which a
**Model** of another pack replaces.

The same can be set in `~/.rcssmonitor.conf` (group `View3D`) or on the
command line:

| Setting | Command line | Value |
|---|---|---|
| `player_model_3d` | `--player-model-3d` | `simple` (cylinders), or the name of a pack: `mannequin`, `human`, or one of yours |
| `figure_folder_3d` | `--figure-folder-3d` | a folder of packs of your choice (empty: none) |

## Where the packs are

rcssmonitor looks for packs in these places, in this order:

1. The bundled packs.
2. The folder of your choice (`figure_folder_3d`).
3. The user's data folders: `rcssmonitor/figures` in each XDG data folder.
   On Linux, the first one is `~/.local/share/rcssmonitor/figures`.

Each pack is a folder that holds a `figure.json`. The folder's name is the
pack's name in the settings. A folder of your choice may also be a pack
itself.

Only the first pack of a name is used. A later pack with the same name
cannot be read: rename its folder. The name `simple` belongs to the
cylinders and cannot be a pack's name.

A pack that cannot be read is shown in the list but cannot be chosen. Its
tooltip tells why. Some problems are found only when the models are read,
the first time the pack is drawn: for example a missing bone, clip or
material. Then the players are drawn as cylinders, the list marks the pack
"cannot be drawn", and its tooltip and the line below the list tell why.
rcssmonitor also prints the reason to the standard error.

## figure.json

Here is the bundled Human pack's `figure.json`:

```json
{
    "format": 1,
    "name": "Human",
    "description": "Men and women of the Universal Base Characters in a uniform, each player with its own body, hairstyle and colors.",
    "credits": [
        {
            "title": "Universal Base Characters [Standard]",
            "author": "Quaternius",
            "license": "CC0 1.0",
            "url": "https://quaternius.com/packs/universalbasecharacters.html"
        }
    ],
    "motion": "mannequin",
    "bodies": [
        {
            "model": "BaseCharacterMale.glb",
            "skeleton": "quaternius",
            "sex": "male",
            "paint": { "Body": "uniform", "Eyebrows": "hair" },
            "hairstyles": [ "Hair_SimpleParted", "Hair_Buzzed" ],
            "extras": [ { "material": "Hair_Beard", "chance": 0.3 } ]
        },
        {
            "model": "BaseCharacterFemale.glb",
            "skeleton": "quaternius",
            "sex": "female",
            "paint": { "Body": "uniform", "Eyebrows": "hair" },
            "hairstyles": [ "Hair_Buns", "Hair_Long", "Hair_BuzzedFemale" ],
            "standard_hairstyle": "Hair_BuzzedFemale"
        }
    ]
}
```

Both bundled packs are in `resources/figures/` of the source tree.

### The keys

| Key | Required | Value |
|---|---|---|
| `format` | yes | `1`. A pack of a later format is not read. |
| `name` | no | The name shown in the list. Without it, the folder's name is shown. |
| `description` | no | Shown in the tooltip. |
| `credits` | no | A list of where the models and the motion come from. Each item is an object with `title`, `author`, `license` and `url` (all optional), or a line of text. |
| `motion` | yes | The pack's own motion (an object, below), or the name of the pack whose motion it uses, such as `"mannequin"`. The credits of that pack are shown with this pack's. |
| `bodies` | yes | The bodies, one or more (below). |

Keys that rcssmonitor does not know are ignored, so a pack can also be
written for a later version.

### The motion

| Key | Required | Value |
|---|---|---|
| `model` | yes | The model of the motion's skeleton (a glTF, FBX or BVH file, or a VRM Animation), in the pack's folder. It needs no mesh, but its rest should be the skeleton's T pose (see below). |
| `skeleton` | no | Which bones play which roles (below). Without it, they are found from the model. |
| `clips` | yes | The clips: `idle` (standing), `walk`, `jog` and `sprint`, all four required; and, if you have them, `walk_back`, `jog_back`, `walk_left`, `walk_right`, `jog_left` and `jog_right` (below). |

Each clip is one of these:

- The name of a clip of the motion's model: `"Walk_Loop"`.
- The name of a file of the pack, told by its extension (`.fbx`, `.glb`,
  `.gltf`, `.vrm`, `.vrma`, `.bvh`): its first clip. `"walk.fbx"`. A BVH
  file has one clip, named after the file (`walk` for `walk.bvh`).
- An object of a `file` and the name of its `clip`:
  `{ "file": "anims.glb", "clip": "Jog_Fwd_Loop" }`. Without `clip`, the
  file's first clip.

A clip of another file is put on the motion's skeleton through the bones'
roles, so the file may be of another skeleton (another rig, other
proportions). Its bones are found by the motion's `skeleton`, or else from
its own names. A file of clips without a skin (an FBX file of Mixamo
downloaded "Without Skin") does not tell its rest pose: its bones of the
same names as the motion's model's (the prefix before a `:` aside, as in
`mixamorig:Hips`) are taken at rest as the motion's model's are.

The 3D view moves the figure by the log, so the clips play in place: a
clip whose hips go forward across it (a "root motion", as a Mixamo
animation downloaded without "In Place") is put in place, its hips'
horizontal drift taken off. The clips of walking and running face +z.
rcssmonitor reads from the clips how fast each one goes (the feet on the
ground), so that the feet do not slide.

#### Walking backward and sideways

The six optional clips are of walking and jogging backward (`walk_back`,
`jog_back`), to the figure's left (`walk_left`, `jog_left`) and to its
right (`walk_right`, `jog_right`), facing +z as the others (a clip to the
left goes toward +x). Each is told as the four are, and any of them may
be left out:

- With none of them, a figure going backward plays the forward clips
  backward, and going diagonally turns its hips toward the way it goes.
- A side of which a clip is left out takes the other side's clip of the
  same speed (`walk_left` and `walk_right`, `jog_left` and `jog_right`)
  mirrored. One clip of a side is enough for both sides.
- Faster than the backward clips, a figure going backward plays the
  forward clips backward; without backward clips, it does so at any speed.

With clips of other ways, the clips of the two ways nearest to the way a
player goes are mixed: forward and to its right going forward and to the
right, to its right and backward going backward and to the right, and so
on, each way's clips by the speed as the forward ones. They are mixed by
how far each way's feet go in a cycle, so that the feet on the ground go
the way the player goes. Going a little off forward or backward (up to 30
degrees), the figure turns its hips that way instead, playing the forward
(or backward) clips: a player running a little off its body direction
runs forward. A player going faster sideways than the clips of walking
sideways go takes quicker steps. With backward clips and no sideways
ones, the forward and backward clips are mixed about sideways.

The clips should be of steady walking and jogging, a cycle or more each,
so that they loop: one cycle (two steps) is enough. rcssmonitor reads how
fast each one goes, as the forward ones. A clip that does not go its way
(its feet on the ground do not go against it) is told and the pack is
not drawn.

### A body

| Key | Required | Value |
|---|---|---|
| `model` | yes | The model of the body (`.glb`, `.gltf`, `.vrm` or `.fbx`), in the pack's folder. It may be the motion's own file. |
| `skeleton` | no | As for the motion. A body may have another skeleton than the motion's, as long as both are humanoid: the motion goes through the bones' roles (hips, spine, upper arm, and so on). |
| `sex` | no | `male` or `female`. A pack with both men's and women's bodies can be limited to one of them (the **Bodies** setting). Without it, the body is anyone's. |
| `paint` | no | How each material of the model is painted (below): `model`, `team`, `tint`, `uniform`, `skin` or `hair`. A material not named is painted `model`. |
| `hairstyles` | no | The materials of the hairstyles. Each player wears one of them, each as often as the others, in its hair color. |
| `standard_hairstyle` | no | The hairstyle of a player whose number the player camera does not know for sure. Without it, the first hairstyle. |
| `extras` | no | Materials that a player wears by a chance over its hairstyle, such as a beard: a list of objects with `material` and `chance` (0 to 1). |

A body can have at most four hairstyles and extras together. Each player's
body, hairstyle and colors are drawn by lot, but always the same for the
same player of the same team: a team looks the same in every game.

### The skeleton

The 3D view moves the bones by their roles, the same as VRM's humanoid
bones: `hips`, `spine`, `chest`, `upperChest`, `neck`, `head`,
`leftShoulder`, `leftUpperArm`, `leftLowerArm`, `leftHand`,
`leftUpperLeg`, `leftLowerLeg`, `leftFoot`, `leftToes`, and the same on the
right. All but `chest`, `upperChest`, `neck`, the shoulders and the toes
are required.

`skeleton` is one of these presets:

| Preset | The bones |
|---|---|
| `quaternius` | Quaternius's rig (the bundled models): `pelvis`, `spine_01`, `upperarm_l`, ... |
| `mixamo` | Mixamo's: `Hips`, `Spine`, `LeftArm`, `LeftForeArm`, `LeftUpLeg`, ... |
| `vroid` | VRoid Studio's: `J_Bip_C_Hips`, `J_Bip_L_UpperArm`, ... |
| `xsens` | XSens MVN's motion capture (as 100STYLE's BVH files): `Hips`, `Chest`, `LeftCollar`, `LeftShoulder` (the upper arm), `LeftHip` (the upper leg), ... |
| `vrm` | The roles that a VRM or VRM Animation file tells |

A preset's names may also follow a prefix that ends with `:` or `_`, such
as `mixamorig:Hips`. Or `skeleton` is the pack's own map from the roles to
the bones' names:

```json
"skeleton": { "hips": "Root_Pelvis", "spine": "Spine1", "head": "Head", "leftUpperArm": "L_Arm", ... }
```

Without `skeleton`, a VRM file's roles are used; otherwise the first
preset whose bones the model has.

The figure's rest pose should be a T pose (the arms straight out
sideways), y up, facing +z. A model whose arms are more than 10° off that
(an A pose) is taken as if they were straight out, a model with another
axis up (a BVH file written z up) is stood upright (the axis nearest to
the way from its hips to its head taken as up), and a model facing
another way (a VRM 0.x model faces -z) is turned to face +z, all by
itself.

### The materials

The color of each material comes from its base color and its base color
texture (glTF's metallic-roughness model, or the diffuse of the older
specular-glossiness one). `paint` tells how the player's colors go over
them:

| Paint | Color |
|---|---|
| `model` | The material's own color and texture (the default). |
| `team` | The player's color, flat (the Mannequin's body). Of a texture, only its transparency is used. |
| `tint` | The player's color, with the texture's light and dark over it (clothes drawn in a texture). |
| `uniform` | A footballer's uniform of the player's color and the player's skin, cut by the bones (the Human's body). Of a texture, only its transparency is used. |
| `skin` | The player's skin color, with the texture's light and dark over it. |
| `hair` | The player's hair color on all players (eyebrows), with the texture's light and dark over it. The `hairstyles` and the `extras` are painted so too. |

For `tint`, `skin` and `hair`, the texture's brightness is taken relative
to its mean: a texture shades the color, and does not make it darker or
lighter as a whole.

A material whose alpha mode is `MASK` is not drawn, and casts no shadow,
where it is more transparent than its alpha cutoff (hair cards,
eyelashes). One whose mode is `BLEND` is drawn the same, with the cutoff
0.5 (the figures are not sorted from far to near). A double-sided material
is drawn from behind too. A texture larger than 1024 pixels is made
smaller.

### The models

A model can be a glTF binary (`.glb`), a glTF JSON file (`.gltf`) with its
buffers and images in files beside it or inside it, a VRM (`.vrm`, 0.x
or 1.0), or an FBX file (`.fbx`, binary or ASCII). All its meshes are drawn: the skinned ones move with their bones,
and one fixed to a bone (a hat on the head) moves with it. A mesh without
normals gets smooth ones. Texture coordinates moved by
KHR_texture_transform are read.

A model that cannot be read without an extension that rcssmonitor does
not read, such as a compression of its meshes or textures
(KHR_draco_mesh_compression, EXT_meshopt_compression, KHR_texture_basisu),
cannot be read; the reason names the extension. Export it without the
compression.

An FBX file is converted to glTF's space: y up, a person facing +z, in
meters (an FBX file may be in centimeters, z up, or turned by its root).
Its rest is the bind pose of its skin. Its materials give their diffuse
(or base) color and its texture: embedded in the file, or a file beside it
(as the file names it, or by its name alone, also in `<name>.fbm`). A
texture with transparent pixels is taken as a mask (cut off at 0.5) and
drawn from both sides; a material with an opacity below 1 is taken as
`BLEND`. Its takes are its clips.

Two formats hold motions only, for the motion's `model` or its clips:

- A BVH file (`.bvh`, motion capture): its joints are the bones, at rest
  where their `OFFSET`s put them, not turned (the skeleton's pose at rest
  should be a T pose); an End Site is a bone named `<its joint>_End`. Its
  motion is one clip. The rotation channels turn a bone in the order
  written (`Zrotation Xrotation Yrotation`: about z, then x, then y, in
  the bone's frame), and the position channels place it in its parent in
  place of its `OFFSET`. Its unit does not matter (the figure takes lengths
  relative to the height of the hips), and its axes are found as above. A
  file cut short in its motion keeps the frames it has whole.

  Motion capture places the skeleton in the room it was taken in: the
  root's `OFFSET` often at the origin with the legs below it, the motion
  meters away from it, going any way. So a BVH skeleton is settled: its
  rest stands on the ground (the lowest bone of the hips and below)
  above the origin, and its clip is moved and turned about the vertical
  so that it starts above the origin, faces +z on the whole (the mean way
  its hips face) and comes down to the ground (the lowest bone across the
  clip). What is moved is the root that moves the skeleton: the hips, or
  a root above them (a reference node, or Blender's `root`). A clip of walking
  or running should go straight; a clip going forward is then put in
  place as any other.
- A VRM Animation (`.vrma`): a glTF file of a skeleton whose bones' roles
  it tells (`VRMC_vrm_animation`), at rest in the VRM T pose. Its first
  clip is read; its expressions and gaze are not.

### Paths

The files are named relative to the pack's folder. A pack that uses
another pack's motion names that pack, not its files.

## Making a pack

1. Make a folder, for example `~/.local/share/rcssmonitor/figures/robot`.
2. Put the model of the body (`.glb`, `.gltf` with its files, or `.vrm`)
   in it.
3. Write `figure.json`. To start, you can use the bundled motion:

   ```json
   {
       "format": 1,
       "name": "Robot",
       "credits": [ { "title": "Robot", "author": "...", "license": "..." } ],
       "motion": "mannequin",
       "bodies": [
           { "model": "Robot.glb", "paint": { "Robot_Body": "team" } }
       ]
   }
   ```

4. Open the settings dialog and choose **Robot** in **Model**.

The materials named in `paint`, `hairstyles` and `extras` must be the
model's. If one is not, the reason lists the model's materials.
