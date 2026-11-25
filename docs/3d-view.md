# The 3D view

rcssmonitor shows a game in 3D as well as in 2D: the pitch, the players (as cylinders or as human figures), the ball and a stadium, seen by cameras that follow the play.
The 3D view draws the same game as the 2D view, live or from a game log, and the 2D view stays as it was.

- [Requirements](#requirements)
- [Showing the 3D view](#showing-the-3d-view)
- [Cameras](#cameras)
- [Mouse and keys](#mouse-and-keys)
- [The settings](#the-settings)
  - [The 3D tab](#the-3d-tab)
  - [The Players & Ball tab](#the-players--ball-tab)
  - [The Camera tab](#the-camera-tab)
- [The flying ball (Flight)](#the-flying-ball-flight)
- [Running smoothly](#running-smoothly)
- [The options](#the-options)

## Requirements

- rcssmonitor built with CMake, with the 3D view: it is built when Qt's OpenGL modules are found ([build.md](build.md)). The tooltip of the `3D View` button tells when it is not.
- OpenGL 3.3 at run time. Without it, the monitor runs with the 2D view only, and tells why when the 3D view is asked for.

## Showing the 3D view

- `D`, the `3D View` button in the tool bar, or `View` -> `3D View` in the menu bar switches between the 2D and 3D views.
- `--view-3d on` on the command line starts the monitor in 3D.
- `File` -> `Save Image of This Frame...` saves the frame shown, and `File` -> `Export Images / Video` exports ranges of the frames as images or videos, drawn by the 3D view offscreen ([export.md](export.md#the-3d-view)).

## Cameras

The 3D view has these cameras, chosen in the tool bar's list of cameras (shown with the 3D view), in `View` -> `3D Camera`, or one after another with `E`:

| Camera | What it shows |
|---|---|
| Broadcast | The pitch from the side, high in the stand, following the ball as a television camera does (the default) |
| Free | Where you put it with the mouse |
| Close-up: Ball | Near the ball, following it |
| Close-up: Selected Player | Near the selected player, following them |
| Top | The pitch from above, as a map |
| Behind the Left Goal / Behind the Right Goal | From behind a goal |
| Selected Player | What the selected player sees, from their eyes or shoulder |

The player is selected as in the 2D view (`Help` -> `Shortcut Keys` lists the keys).

The free camera can also be put at viewpoints of the whole pitch: `I` puts it at the side of the pitch, `Ctrl+I` at the next viewpoint (the far side, behind the goals, high above, the corners), and the `Whole Pitch` button in the tool bar lists them all.

## Mouse and keys

The mouse and the keys work as in the 2D view where they can:

| Mouse or key | In the 3D view |
|---|---|
| Wheel | Steps the frames, as in the 2D view |
| `Z` / `X` (or `Ctrl+Z`) | Brings the camera nearer / farther. A camera following the ball or a player keeps following at the new distance |
| Left drag | Turns the camera around what it looks at. In a camera other than Free, the camera becomes Free there |
| `Shift` or `Ctrl` + left drag | Moves the camera |
| Middle drag | Brings the camera nearer (up) or farther (down) |
| Right click | The referee's menu, at the player or the place clicked (its commands need the connection to the server) |
| `D`, `E`, `I`, `Ctrl+I` | The 3D view, the next camera, the viewpoints ([Cameras](#cameras)) |
| `S` | The players' stamina: none, stamina, capacity, both (also the bars over the players in 3D) |

`Help` -> `Shortcut Keys` prints the keys.

## The settings

`View` -> `Preference` (`Ctrl+V`) opens the settings dialog. Three of its tabs are for the 3D view; each has a `Default` button that sets its items back.
Each item tells what it does in its tooltip.
The settings are kept in `~/.rcssmonitor.conf` when the monitor quits.

### The 3D tab

| Item | Values | |
|---|---|---|
| Compass | Off / Player Camera / All Cameras | A strip at the top: the way the camera looks, the goals and the ball. Player Camera: only in the Selected Player camera |
| Minimap | Off / Player Camera / All Cameras | A small map at the bottom right: the players, the ball, the camera and what it sees |
| FPS | on / off | The frames drawn per second and the GPU's time of a frame, at the bottom right |
| Smooth Edges | on / off | Smooth edges (multisampling). Off: less work for the GPU |
| Shadows | Off / Simple / Detailed | The sun's shadows. Simple: a soft shadow under each player. Detailed: the shapes of the players and the goals (more work for the GPU) |
| Boards, Banners | on / off | The boards around the pitch, and the banners with the team names and logos |
| Stands | on / off | The stands, with the fans of both teams in their colors |
| Sky | on / off | The sky and the fog far away |
| Grass | Plain / Stripes / Realistic | The grass: one color, light and dark stripes, or stripes that change with the way the camera looks, with the blades, patches and wear of the grass |

### The Players & Ball tab

| Item | Values | |
|---|---|---|
| Model | Simple, or a figure pack | How the players are drawn: Simple, as cylinders and spheres with the marks of the body and face directions. The bundled figure packs are `Mannequin` and `Human`; packs of your own can be added ([FIGURE_PACKS.md](FIGURE_PACKS.md)) |
| Bodies | Mixed / Male / Female | The bodies of the `Human` pack |
| Figure Scale | 0.8 to 1.5 | How many times a person's height the figures are, so that they can be seen from afar |
| Pattern | on / off | The ball's pattern, which shows it roll |
| Flight | on / off | The ball flies in the air ([The flying ball](#the-flying-ball-flight)). Off by default |
| Ring | on / off | A ring on the ground around the ball, to find it where it is small |
| Trail | on / off | The ball's way in the last cycles |
| Size | 0 to 1 m | The ball's radius in 3D; 0: the real size. The 2D view's ball size is set apart |
| Ring Radius, Trail Cycles | 0.1 to 5 m, 1 to 30 | The ring's radius and the trail's length |

### The Camera tab

| Item | Values | |
|---|---|---|
| Close-up Camera: Distance, Elevation | 3 to 60 m, 5 to 85° | Where the close-up cameras are, from the ball or the player |
| Player Camera: Position | Eyes / Over the Shoulder | The Selected Player camera at the player's eyes, or behind their shoulder |
| Direction | Head / Body / Between | Where it looks: the way of the player's head, of their body, or between |
| Motion | Instant / Smooth | How it turns with the player |
| Field of View | View Width / Eased / Fixed | As wide as the player's view (which changes with the view width), easing into it, or a fixed width |
| Time Constant, Max Turn, Fixed Field of View | | How smooth the motion is, and the fixed width |
| Player Sight | Everything / View Cone / Server Rules | What the Selected Player camera shows: everything, only what is in the player's view cone, or what the server's rules let the player see |

## The flying ball (Flight)

Today's rcssserver is two-dimensional: the ball never leaves the ground.
With `Flight` on (the `Players & Ball` tab, or `--ball-flight-3d on`), the 3D view lets the ball fly in the air, virtually: from the game log, it guesses which kicks a real player would loft, and adds only the height to the ball's way on the ground.

- A kick is lofted when the log gives a reason: a long ball, a corner kick or a long goal kick, a cross from a wing into the penalty area, a shot from afar (a little), or opponents near the ball's way that did not touch it (it went over them). Some of the long ones land halfway and bounce on to the receiver.
- The ball flies under gravity and bounces, low enough for the next player to touch it (a foot, a goalkeeper's hands), and under the crossbar into a goal.
- Which kicks fly and how high vary from kick to kick, but a frame looks the same every time: paused, stepped back or replayed.
- It is a guess from the log, not what the server computed. The 2D view and the overlays stay on the ground.
- In a live game, the ball flies only with `Live Delay` ([README](../README.md#drawing-a-live-game-some-seconds-behind-live-delay)): the next touch of each kick has to be known when it is drawn. A kick whose next touch has not come when it is drawn stays on the ground.
- A game log of a server with the ball's height (a future 2.5D server) is drawn with its own heights.

The idea of the flying ball comes from a paper of Osaka Metropolitan University, which lofts the ball between the kicks of a 2D game log in its 3D log viewer:
Hatakeyama et al., "サッカー分析のためのログ可視化システム" (a log visualization system for soccer analysis), 人工知能学会第二種研究会資料 (JSAI SIG technical reports), SAI-047, 2024 (in Japanese only). <https://doi.org/10.11517/jsaisigtwo.2023.SAI-047_05>
rcssmonitor decides which kicks fly and how high in its own way.

## Running smoothly

- The `FPS` item shows how fast the 3D view is drawn. With an integrated GPU or a large screen, turn `Smooth Edges` off and set `Shadows` to `Simple` (the default) for fewer frames dropped. Figure packs with many triangles take more time ([FIGURE_PACKS.md](FIGURE_PACKS.md)).
- In a Wayland session with a screen of high resolution such as 4K, start the monitor with `QT_QPA_PLATFORM=wayland` for a smooth 3D view:
  ```bash
  QT_QPA_PLATFORM=wayland rcssmonitor
  ```
  Qt may run the monitor through XWayland even in a Wayland session, and XWayland can slow the 3D view down at high resolutions. rcssmonitor does not choose Wayland by itself (Qt's own choice is kept). Qt's Wayland plugin is needed (`qt6-wayland` on Ubuntu).

## The options

Each setting of the 3D view has a command line option of the same name with hyphens, e.g. `--camera-3d closeup-ball`, `--shadows-3d detailed`, `--player-model-3d human`.
`rcssmonitor --help` lists them with their values and defaults.

| Option | Setting |
|---|---|
| `--view-3d on` | Start in the 3D view |
| `--camera-3d NAME` | The camera: `broadcast`, `free`, `closeup-ball`, `closeup-player`, `top`, `goal-left`, `goal-right`, `player` |
| `--player-model-3d NAME` | `simple` (cylinders), or a figure pack's name (`mannequin`, `human`, ...) |
| `--ball-flight-3d on` | The flying ball |
| `--live-delay SECONDS` | Draw a live game some seconds behind (0: off) |
