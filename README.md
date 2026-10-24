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

Systems receive a mutable `SystemContext &`, declared in
[SystemContext.h](engine/core/SystemContext.h). It contains descriptor-checked
`SystemEntities entities`, `const FrameContext &frame`, and `editMode`, plus
narrow physics operations. It exposes no raw registry, physics world, scene,
or job-submission handle.

The game/editor loop captures one
presentation frame after GLFW event polling. Local accumulator
logic in `BaseGame::runLoop()` and `EditorLayer::run()` derives zero or more ticks
with fixed `deltaTime` and simulation
`elapsedTime`; all systems in one tick share its input and window values through
a const reference. The scheduler keeps each update's context and tick frame in
scope until the update finishes.

```cpp
#include "core/ISystem.h"
#include "core/SystemContext.h"

class MovementSystem : public ISystem
{
public:
    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("MovementSystem")
            .writes<Transform>()
            .nonReentrant();
    }

    void update(SystemContext &context) override
    {
        auto view = context.entities.view<Transform>();
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
instance. A dependency graph orders their updates. With workers configured,
independent systems can run together during play; each tick finishes before
rendering resumes. Edit mode and zero-worker execution use a serial path.

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

Animation maps define a texture, named sprite frames, and animations that play
those frames in a specified order. Each frame can also override existing
collider and sensor shapes.

#### Example JSON

```json
{
  "meta": { "texture": "character.png" },
  "frames": {
    "idle_0": { "x": 0, "y": 0, "w": 32, "h": 32, "duration": 0.2 },
    "attack_0": {
      "x": 32, "y": 0, "w": 32, "h": 32, "duration": 0.08,
      "colliders": {
        "body": { "size": [12, 22], "offset": [0, -4], "rotation": 0 }
      },
      "sensors": {
        "attack": { "size": [30, 22], "offset": [13, 1], "active": false }
      }
    },
    "attack_1": {
      "x": 64, "y": 0, "w": 32, "h": 32, "duration": 0.08,
      "sensors": {
        "attack": { "size": [30, 22], "offset": [13, 1], "active": true }
      }
    }
  },
  "animations": {
    "idle": { "frames": ["idle_0"] },
    "attack": { "frames": ["attack_0", "attack_1"] }
  }
}
```

#### JSON Fields

| Field | Meaning |
| --- | --- |
| `meta.texture` | Texture path. Relative paths resolve under `GAME_ASSETS_DIR/texture`, independently of the JSON file's location. |
| `frames` | Dictionary of frame names and their sprite rectangles. |
| `frames.<name>.x`, `y`, `w`, `h` | Rectangle position and dimensions in atlas pixels. |
| `frames.<name>.duration` | Seconds to display this frame; defaults to `0.1`. Use a positive value. |
| `frames.<name>.colliders` | Optional overrides for named `BoxCollider2D` shapes. |
| `frames.<name>.sensors` | Optional overrides for named `Sensor2D` shapes. |
| `animations.<name>.frames` | Ordered array of frame names. Frames can be reused across animations. |

Every listed frame name should exist, and every animation should contain at
least one valid frame. The loader skips unknown frame names; empty animations
are not playable. Frame-level sprite `rotation` is not parsed.

#### Collider and Sensor Overrides

Shape names such as `body` and `attack` must already exist on the entity's
`BoxCollider2D` or `Sensor2D` component. Unmatched shape names are skipped;
animation frames do not create shapes or add these components.

Both shape types support optional `size`, `offset`, and `active` overrides.
`size` is full width and height, and `offset` is relative to the physics body,
using scene physics units. Box colliders also support `rotation` in degrees.
Sensors have no rotation override.

Each update resets shape size, offset, and collider rotation to their authored
values before applying the current frame's overrides. Collider activation
resets to `true`; sensor activation resets to the shape's authored `isActive`.
See [Box2D and physics](docs/box2d-and-physics.md) for shape setup and physics behavior.

#### Configuring and Playing Animations

`Animator::animationSourceMap` maps each animation name to a JSON source that
defines that same name. Relative JSON paths resolve from the game's assets
directory (`GAME_ASSETS_DIR`):

```cpp
gameObject.addComponent<Animator>();
Animator &animator = gameObject.getComponent<Animator>();
animator.animationSourceMap["idle"] = "texture/character.json";
animator.animationSourceMap["attack"] = "texture/character.json";
animator.play("idle"); // Looping defaults to true.

// Later, when an attack starts:
animator.play("attack", false); // Stop on the final frame when finished.
```

`play()` returns `false` if the source mapping, file, or requested animation
cannot be found. Looping is chosen through `play(name, loop)`; the loader does
not read a JSON `loop` setting. Any animation name is valid. A newly added
`Animator` has no selected animation; choose one in the inspector or call
`play()`. Scene loading resumes a nonempty serialized `CurrentAnimation` with
looping enabled. An animation named `idle` is optional.

Register, add, and enable `AnimationSystem` in the scene. It advances playback
with `context.frame.deltaTime`, updates the sprite, and applies shape overrides
before `PhysicsSystem`. Editor previews supply their own presentation delta.
`AnimationPlayer` also supports pause and stop; stop resets playback to the
first frame. Playback currently advances at most one frame per update and
discards leftover elapsed time when advancing.

The first `Animator::play()` for an uncached source can load its texture through
OpenGL. Systems that can reach that loading path must run on the main thread.

### Contributing

Feel free to fork and contribute via pull requests! Bug fixes, improvements, and new features are all welcome.
