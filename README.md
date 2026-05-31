# MementoGE

Memento is a lightweight 2D game engine built using OpenGL, GLFW, and ImGui, aimed at game prototyping and editor-based level design. It supports rendering, scene editing, input handling, and more.

<img width="3024" height="1898" alt="image" src="https://github.com/user-attachments/assets/5e664b67-557c-46f1-8650-c24b620aab86" />

---

## Getting Started

### Prerequisites

- **OpenGL** core.
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

---

## Creating a New Game with the Template Script

Helper script that bootstraps a minimal game project wired up to the engine.

Download just the script via `curl` and run it:

```bash
# Download and run the script directly in an empty folder
curl -o create_game.sh https://raw.githubusercontent.com/hello-world-was-taken/mementoGE/main/create_game.sh
chmod +x create_game.sh
./create_game.sh MyNewGame
```

The script will:

- Create a new folder `MyNewGame/`.
- Initialize a git repository inside it (if one does not already exist).
- Create a `game/` subfolder with `assets/`, `components/`, `lib/`, and `systems/` directories.
- Add MementoGE as a git submodule under `game/lib/memento`.
- Generate `main.cpp`, `CMakeLists.txt`, `build.sh`, `run.sh`, a basic `.clang-format`, and VS Code launch/tasks configs.

To build and run the new game:

```bash
cd MyNewGame
./build.sh
./run.sh
```

You can then start adding your own game code under `game/` (e.g. new systems, components, and assets) and wire it into `main.cpp` and the generated CMakeLists.

---

## Integrating MementoGE into a Game CMake Project

When you embed MementoGE into a larger game project, the top-level CMakeLists (the _game_ CMake) is expected to provide a small set of configuration variables and options that the engine uses.

### Required: asset directory

Before adding the MementoGE subdirectory, define the game asset root as a CMake variable:

```cmake
# Path where your game's assets live (textures, audio, scenes, etc.).
set(GAME_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/game/assets/" CACHE INTERNAL "")
```

MementoGE's CMake will read this variable and define a preprocessor macro `GAME_ASSETS_DIR` for the `engine` target so that engine code can locate shared assets (fonts, default shaders, etc.).

### Optional: editor vs. runtime builds

The _game_ controls whether editor-only code (ImGui editor, panels, gizmos, etc.) is compiled by setting a CMake option and defining the `EDITOR_BUILD` macro accordingly:

```cmake
# At the top level (game CMakeLists.txt)
option(EDITOR_BUILD "Build with editor tooling enabled" ON)

add_subdirectory(path/to/mementoGE)

add_executable(MyGame
    # your game sources here
)

target_link_libraries(MyGame PRIVATE engine)

if(EDITOR_BUILD)
    target_compile_definitions(MyGame PUBLIC EDITOR_BUILD)
endif()
```

Inside MementoGE, the engine's CMake checks the same `EDITOR_BUILD` option and, when it is ON, defines `EDITOR_BUILD` for the `engine` target as well. This keeps the game and engine in sync: turning editor support on or off at the game level automatically enables or disables editor-only code inside the engine.

### Adding a System

Systems implement `ISystem`, declare their access through the required `descriptor()` override, and receive scene services through `SystemContext`:

Systems receive a mutable `SystemContext &`. It contains the scene's mutable registry and physics references,
`const FrameContext &frame`, and `editMode`. The game/editor loop captures one
frame after `beginFrame()` and GLFW event polling. Its variable `deltaTime`,
`elapsedTime`, input snapshot, and window metrics are shared by all
updates through a const reference. Keep the frame alive until updates finish.

```cpp
class MovementSystem : public ISystem
{
public:
    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("MovementSystem")
            .writes<Transform>()
            .nonReentrant();
    }

    int executionOrder() const override
    {
        return 150;
    }

    void update(SystemContext &context) override
    {
        auto view = context.registry.view<Transform>();
        // Update matching entities using context.frame.deltaTime as needed.
    }
};
```

Register the type once, then add it to any scene that needs it:

```cpp
SystemRegistry::instance().registerSystem<MovementSystem>("MovementSystem");
scene.addSystem("MovementSystem");
scene.setSystemEnabled("MovementSystem", true);
```

`SystemRegistry` stores factories, so every scene owns an independent system
instance. A system controls its own execution order by overriding
`executionOrder()`; the default is `100`. The built-in animation and physics
systems use orders `200` and `300`, but must be registered and added like any
other system. Equal orders preserve insertion order. Descriptor `before`/`after`
requirements and read/write conflicts are metadata for a future dependency
scheduler; they do not change the current sequential loop. That scheduler will
use explicit dependencies first and numeric order as a deterministic fallback.
All systems in the scene appear in the editor's Systems panel.
While editing, every enabled system runs. Systems start disabled, and entering
Play enables every system on the disposable runtime scene. Use the panel
checkboxes for temporary overrides. Runtime Pause still stops scene updates
entirely.

Systems read `context.frame.input.isKeyHeld(...)`, key/mouse edge helpers, and
the broadcast `context.frame.input.events` vector. They use
`context.frame.window` for pure viewport and screen-to-world calculations.
Snapshot reads need no Input/Window resource declarations, and Time is no longer
a scheduling resource. Platform capture and window operations remain on the
main thread. `AnimationPlayer::update(float)` and `Animator::update(float)` take
explicit delta time; rendering receives frame elapsed time for shader effects.
The loop remains sequential and variable-step; a dependency scheduler and fixed
simulation ticks are later work. See [Engine Concurrency Design](../../../docs/concurrency-design.md)
and [Multithreading Migration Plan](docs/multithreading-plan.md).

## Controls

| Action        | Input                                |
| ------------- | ------------------------------------ |
| Pan camera    | Left mouse drag                      |
| Zoom in/out   | Scroll wheel / Touchpad pinch        |
| Select object | Left click on object                 |
| Move object   | Drag selected object (snaps to grid) |

## Remaining Features / TODO

- [ ] Zoom centering on mouse cursor
- [x] Support for touchpad gestures (pinch-to-zoom, two-finger pan)
- [x] Dedicated event system (abstract input handling from ImGui + GLFW)
- [x] Scene serialization (saving/loading scenes)
- [x] Prefab system for reusable GameObjects
- [x] Remove GLEW requirement by bundling or replacing
- [ ] (`inprogress`) Gizmo tools - (translate/rotate/scale handles)
- [ ] Custom shader and material editing support
- [x] Physics integration (Box2D)
- [x] Audio system (OpenAL)
- [ ] Hot-reload shaders and assets during runtime
- [x] Cross-platform support (Window/MacOS)
- [x] Sprite based animation
- [x] Proper logging system

## Forked Dependencies

- **[glew-cmake](https://github.com/hello-world-was-taken/glew-cmake)** — Fork of `Perlmint/glew-cmake`. The upstream version gates macOS AGL linking behind `CMAKE_SYSTEM_VERSION < 25.0.0`; on macOS 16+ (Darwin ≥ 25) where AGL was removed, this incorrectly falls through to the Linux/X11 code path. The fork restructures the condition so macOS always takes the Apple path, simply skipping AGL on newer versions.

  Upstream:

  ```cmake
  if(APPLE AND CMAKE_SYSTEM_VERSION VERSION_LESS "25.0.0")
      find_library(AGL_LIBRARY AGL REQUIRED)
      list(APPEND LIBRARIES ${AGL_LIBRARY})
  elseif(NOT WIN32)
  ```

  Fork:

  ```cmake
  if(APPLE)
      if(CMAKE_SYSTEM_VERSION VERSION_LESS "25.0.0")
          find_library(AGL_LIBRARY AGL REQUIRED)
          list(APPEND LIBRARIES ${AGL_LIBRARY})
      endif()
  elseif(NOT WIN32)
  ```

- **[Dear ImGui](https://github.com/hello-world-was-taken/imgui)** — Fork of `ocornut/imgui`. The fork keeps the GLFW backend from submitting raw mouse wheel input directly to ImGui, so MementoGE can route scroll events through its own event system and apply frame-based smoothing in `ImGuiWrapper::applySmoothScrolling()` before `ImGui::NewFrame()`. This avoids duplicate wheel input while making touchpad and mouse-wheel scrolling feel less abrupt in editor panels.

  Upstream:

  ```cpp
  io.AddMouseWheelEvent((float)xoffset, (float)yoffset);
  ```

  Fork:

  ```cpp
  // MementoGE handles wheel smoothing before ImGui::NewFrame().
  // io.AddMouseWheelEvent((float)xoffset, (float)yoffset);
  ```

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
    "sprite-1-8": { "x": 94, "y": 0, "rotation": 0, "w": 5, "h": 7 }
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
