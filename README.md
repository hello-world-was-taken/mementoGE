# MementoGE

Memento is a lightweight 2D game engine built using OpenGL, GLFW, and ImGui, aimed at game prototyping and editor-based level design. It supports rendering, scene editing, input handling, and more.

https://drive.google.com/file/d/1Xxlj8qKeTMeP6VCOT5qz2uikZ4h5nwnv/view?usp=sharing









---

## Getting Started

### Prerequisites

- **OpenGL** core.
- **GLEW** must be installed **locally** on your system. *(Will be removed as a requirement soon)*
- A C++17-compatible compiler (e.g., GCC 9+, Clang 10+, MSVC 2019+)

---

### Cloning the Repository

This project uses Git submodules. Clone it with:

```bash
git clone --recurse-submodules https://github.com/hello-world-was-taken/mementoGE.git
cd mementoGE
```
If you forgot --recurse-submodules, you can fix it with:

```bash
git submodule update --init --recursive
```

### Building
```bash
cd mementoGE
./build.sh
```
You will need CMake 3.26.4+.

### Running
After compiling, you can run the engine like so:

```bash
./run.sh
```

## Controls

| Action           | Input                            |
|------------------|----------------------------------|
| Pan camera       | Left mouse drag                  |
| Zoom in/out      | Scroll wheel / Touchpad pinch    |
| Select object    | Left click on object             |
| Move object      | Drag selected object (snaps to grid) |

## Remaining Features / TODO

- [ ] Zoom centering on mouse cursor
- [ ] Support for touchpad gestures (pinch-to-zoom, two-finger pan)
- [x] Dedicated event system (abstract input handling from ImGui + GLFW)
- [x] Scene serialization (saving/loading scenes)
- [x] Prefab system for reusable GameObjects
- [ ] Remove GLEW requirement by bundling or replacing
- [ ] (`inprogress`) Gizmo tools - (translate/rotate/scale handles)
- [ ] Custom shader and material editing support
- [x] Physics integration (Box2D)
- [ ] Audio system (OpenAL or FMOD)
- [x] Hot-reload shaders and assets during runtime
- [x] Cross-platform support (Window/MacOS)
- [x] Sprite based animation
- [x] Proper logging system

## Development Notes
### Animation Support

The engine now supports basic 2D sprite animations via JSON-defined animation maps. Each animation map JSON file defines the texture, sprite layout, and animations.

#### Example JSON

```json
{
  "meta": { "texture": "name.png" },
  "frames": {
    "sprite-1-1": { "x": 0, "y": 0, "rotation": 0, "w": 13, "h": 4 },
    "sprite-1-2": { "x": 13, "y": 0, "rotation": 0, "w": 15, "h": 5 },
    "sprite-1-3": { "x": 28, "y": 0, "rotation": 0, "w": 8, "h": 5 },
    "sprite-1-4": { "x": 36, "y": 0, "rotation": 0, "w": 24, "h": 6 },
    "sprite-1-5": { "x": 60, "y": 0, "rotation": 0, "w": 11, "h": 6 },
    "sprite-1-6": { "x": 71, "y": 0, "rotation": 0, "w": 5, "h": 7 },
    "sprite-1-7": { "x": 76, "y": 0, "rotation": 0, "w": 18, "h": 7 },
    "sprite-1-8": { "x": 94, "y": 0, "rotation": 0, "w": 5, "h": 7 },
  },
  "animations": {
    "idle": {
      "frames": [
        "sprite-1-1",
        "sprite-1-2",
        "sprite-1-3",
        "sprite-1-4",
        "sprite-1-5",
        "sprite-1-6",
        "sprite-1-7",
        "sprite-1-8"
      ]
    }
  }
}

```

#### JSON Fields

- **texture**: Path to the sprite sheet image.
- **spriteWidth / spriteHeight**: Dimensions of a single frame in pixels.
- **spriteGapX / spriteGapY**: Horizontal and vertical spacing between frames in the sheet.
- **spriteCount**: Total number of frames in the sheet.
- **columns**: Number of columns in the sprite sheet.
- **animations**: Dictionary of animation definitions:
  - **startFrame**: Index of the first frame in the animation.
  - **frameCount**: Number of frames in the animation.
  - **duration**: Time in seconds per frame.
  - **loop**: Whether the animation should repeat.

#### Important Note

Currently, each sprite’s JSON **must include an animation named `idle`**, as this is used as the default animation when an Animator component is added to a GameObject.


### Contributing
Feel free to fork and contribute via pull requests! Bug fixes, improvements, and new features are all welcome.
