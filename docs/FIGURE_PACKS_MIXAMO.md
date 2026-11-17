# A figure pack of Mixamo

[Mixamo](https://www.mixamo.com) gives characters and animations for free
(with an Adobe account). rcssmonitor reads Mixamo's FBX files as they are,
so a character and four animations make a figure pack of the 3D view
([FIGURE_PACKS.md](FIGURE_PACKS.md)).

Mixamo's files may be used in your own work but not given away as they
are. Keep the pack for yourself: do not put it in a public repository or
share it. For the same reason rcssmonitor bundles none of them, and gives
only the template [figure_packs/mixamo/figure.json](figure_packs/mixamo/figure.json).

## 1. Download the character

On Mixamo, choose a character (or upload your own and rig it). Download it:

- Format: **FBX Binary (.fbx)**
- Pose: **T-pose**

Save it as `Character.fbx`. Its textures are in the file.

## 2. Download the animations

With the same character chosen, find four animations: standing, walking,
jogging and sprinting. Search for them with words such as "idle",
"walking", "jogging" or "running", and "sprint" or "fast run". Choose
animations that go straight forward and loop; check **In Place** if the
animation has it (if not, rcssmonitor takes the forward motion off by
itself).

Download each:

- Format: **FBX Binary (.fbx)**
- Skin: **Without Skin** (smaller; the character's file gives the rest
  pose), or With Skin
- Frames per second: 30

Save them as `idle.fbx`, `walk.fbx`, `jog.fbx` and `sprint.fbx`.

If you like, find also animations of walking and jogging backward and
sideways (search with words such as "backward" and "strafe"), and save
them as `walk_back.fbx`, `jog_back.fbx`, `walk_left.fbx`,
`walk_right.fbx`, `jog_left.fbx` and `jog_right.fbx`, the same way. Each
is optional, and one side is enough (the other side is mirrored); add
them to `clips` of `figure.json` (`"walk_back": "walk_back.fbx"`, ...).
Without them, the figures play the forward animations backward going
backward. See "Walking backward and sideways" in
[FIGURE_PACKS.md](FIGURE_PACKS.md).

The actions can be animations too, each optional (without it, the
figures keep the poses rcssmonitor makes): a kick (search "kick", "soccer
pass", "penalty kick"), the sliding tackle ("slide", "tackle"), falling
("fall", "trip"), getting up ("getting up", "stand up"), the goalkeeper's
catch and dive ("goalkeeper", "catch", "dive", "save"), turning on the
spot ("left turn", "right turn"; In Place if it has it). Download them the
same way, and add them to `clips`: `"kick": "kick.fbx"`, `"tackle":
"tackle.fbx"`, `"get_up": "get_up.fbx"`, `"catch_low": "catch.fbx"`,
`"dive_right": "dive.fbx"`, `"turn_right": "right_turn.fbx"`, and so on
(the keys are in "The actions" of FIGURE_PACKS.md). A kick of either foot
will do, and one side of a dive or a turn.
rcssmonitor finds when the foot meets the ball, the body is down, or the
hands reach for the ball; if it finds the wrong time, tell it in seconds:
`"kick": { "file": "kick.fbx", "moment": 0.8 }`.

## 3. Make the pack

1. Make a folder for the pack in your folder of packs. Its name is the
   pack's name in the settings, for example
   `~/.local/share/rcssmonitor/figures/mixamo-character`. (The **Packs**
   button of the settings dialog opens your folder of packs.)
2. Put the five files in it.
3. Copy `figure.json` of the template into it, and change its `name`.
4. Open the settings dialog (View > Preference, Ctrl+V), the Players & Ball
   tab, and choose the pack in **Model**.

rcssmonitor reads from the animations how fast each one goes, so that the
feet do not slide. If something is wrong, the pack is marked "cannot be
drawn" and its tooltip tells why.

Mixamo's characters often have many more triangles than the bundled
bodies (one has about 55,000, three times the Human's), and take more of
the GPU. If the 3D view is slow, keep the shadows simple, or make the
character lighter: see "How heavy a pack may be" in
[FIGURE_PACKS.md](FIGURE_PACKS.md).

## 4. The team colors

The template draws the character in its own colors. To dress the players
in their team's colors, tell how the character's materials are painted in
`paint` of the body.

The materials' names differ from character to character, so find them
first. Write a name that is not one, such as `"?"`:

```json
"bodies": [
    { "model": "Character.fbx", "skeleton": "mixamo",
      "paint": { "?": "team" } }
]
```

The pack is then marked "cannot be drawn", and its tooltip (or the line
below **Model**) lists the character's materials, for example:

```
Character.fbx: no material "?" (its materials: "Alpha_Body_MAT", "Alpha_Joints_MAT")
```

Then paint each material that should take the team's color, and leave
the others out (they keep their own colors):

| Paint | Color |
|---|---|
| `team` | The team color, flat. |
| `tint` | The team color, with the texture's light and dark over it: clothes drawn in their own texture. |
| `uniform` | A footballer's uniform in the team color and the player's skin, cut by the bones. The texture's colors are not used, so a face drawn in it is gone too. |
| `skin`, `hair` | The player's skin or hair color, with the texture's light and dark over it. |

For example, Mixamo's Y Bot (the materials above) in the team colors,
like the bundled Mannequin, the joints in their own color:

```json
"paint": { "Alpha_Body_MAT": "team" }
```

If a character's whole body, its skin and its clothes, is one material
with one texture, the team color can go only over all of it: `tint`
colors its skin too, and `uniform` draws a uniform in place of its
clothes and its face.

## 5. When the pack cannot be drawn

| Reason | What to do |
|---|---|
| `walk.fbx: no clips` | The file has no animation: download it again with the animation. |
| `the clip walk.fbx is not of walking or running ...` | The animation does not walk in place facing forward: choose another (straight forward, looping). |
| `the clip walk_back.fbx is not of walking or running backward ...` | The animation does not go its way (backward, to the left, to the right) facing forward: choose another, or check that `walk_left` goes to the character's left. |
| `the clip kick.fbx is not of kicking ...`, `the clip tackle.fbx is not of falling ...` | No foot swings forward in the kick, or the hips do not come down in the tackle (or the fall): choose another, or tell its `moment`. |
| `the clip right_turn.fbx is not of turning ...` | The animation does not turn on the spot, or turns the other way: choose another, or tell it as the other side (`turn_left`). |
| `Character.fbx: the bones are of no skeleton known ...` | Not a Mixamo skeleton: use `skeleton` with a map of the roles to its bones (FIGURE_PACKS.md). |
| `Character.fbx: no material "..." (its materials: ...)` | A name in `paint` is not one of the character's materials: use the names listed (section 4). |
| `... is not found in the pack's folder` | A file named in `figure.json` is missing, or its name differs (upper and lower case count). |
