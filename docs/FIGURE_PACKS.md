# Figure packs

The 3D view of rcssmonitor can draw the players as human figures. The
models and the motion of the figures come in *figure packs*. Two packs are
bundled with rcssmonitor:

- **Mannequin** (`mannequin`): the Mannequin of Quaternius's Universal
  Animation Library, all in the team color but its joints.
- **Human** (`human`): men and women of Quaternius's Universal Base
  Characters in a uniform. Each player has its own body, hairstyle, and
  skin and hair colors. They move with the Mannequin's motion.

You can add packs of your own, for example with models that cannot be
bundled because of their licenses.

This document describes format 1 of `figure.json`. In this version a pack
can use glTF binary models (`.glb`) whose bones are named as Quaternius's
rig. Later versions will read more formats (glTF with textures, VRM, FBX,
BVH) and more skeletons (Mixamo, VRM).

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

The dialog also reads the packs again each time it is shown.

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
| `model` | yes | The glTF binary file with the clips, in the pack's folder. |
| `skeleton` | no | The names of its bones: `quaternius`. Without it, the skeleton is found from the bones' names. |
| `clips` | yes | The names of the clips: `idle` (standing), `walk`, `jog` and `sprint`. All four are required. |

The clips of walking and running must play in place, facing +z: the 3D
view moves the figure by the log. rcssmonitor reads from the clips how fast
each one goes (the feet on the ground), so that the feet do not slide.

### A body

| Key | Required | Value |
|---|---|---|
| `model` | yes | The glTF binary file of the body, in the pack's folder. It may be the motion's own file. |
| `skeleton` | no | As for the motion. A body may have another skeleton than the motion's, as long as both are humanoid: the motion goes through the bones' roles (hips, spine, upper arm, and so on). |
| `sex` | no | `male` or `female`. A pack with both men's and women's bodies can be limited to one of them (the **Bodies** setting). Without it, the body is anyone's. |
| `paint` | no | How each material of the model is painted: `team` (the player's color, as the Mannequin's body), `uniform` (the skin, in a footballer's uniform of the player's color and the player's skin), `hair` (the player's hair color, on all players: eyebrows) or `model` (the material's own color, the default). |
| `hairstyles` | no | The materials of the hairstyles. Each player wears one of them, each as often as the others, in its hair color. |
| `standard_hairstyle` | no | The hairstyle of a player whose number the player camera does not know for sure. Without it, the first hairstyle. |
| `extras` | no | Materials that a player wears by a chance over its hairstyle, such as a beard: a list of objects with `material` and `chance` (0 to 1). |

A body can have at most four hairstyles and extras together. Each player's
body, hairstyle and colors are drawn by lot, but always the same for the
same player of the same team: a team looks the same in every game.

### Paths

The files are named relative to the pack's folder. A pack that uses
another pack's motion names that pack, not its files.

## Making a pack

1. Make a folder, for example `~/.local/share/rcssmonitor/figures/robot`.
2. Put the model of the body (`.glb`) in it.
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
