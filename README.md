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
`const FrameContext &frame`, and `editMode`.
The game/editor loop captures one
presentation frame after GLFW event polling. Local accumulator
logic in `BaseGame::runLoop()` and `EditorLayer::run()` derives zero or more ticks
with fixed `deltaTime` and simulation
`elapsedTime`; all systems in one tick share its input and window values through
a const reference. The scheduler keeps each update's context and tick frame in
scope until the update finishes.

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
instance. `Scene` builds a `SystemGraph` directly from a vector of descriptors
when systems are added or deserialized, then runs systems sequentially in its
topological order.
`orderedSystems()` lists descriptor names in execution order. `successors()`
maps each descriptor name to the names of systems that directly depend on it;
systems with no dependents have an empty list. Scene registration names can be
aliases: graph dependencies always use the descriptor names.
Descriptors must remain constant while installed.
Descriptor `before()`/`after()` requirements establish system dependencies.
Component/resource write conflicts and structural changes also produce ordering
edges, oriented along the semantic topological order. Among ready systems,
scene insertion order breaks ties; it cannot override an explicit or transitive
dependency. The map's alphabetical key order does not affect execution order.
Built-in animation and physics systems declare their dependency through
descriptors and must be registered and added like any other system.

Graph construction rejects empty or duplicate descriptor names, empty dependency
targets, self-dependencies, contradictory dependencies, and cycles, identifying the systems involved. A
named dependency on a system absent from the scene is ignored, so scenes can
omit optional systems. Disabled systems remain in the graph: their update is
skipped, while dependencies through them still order the enabled systems.
The editor's Systems panel displays graph order. The game and editor each own
a persistent `SystemScheduler`. It runs `MainThread` updates on the caller and
`AnyWorker` updates through its worker pool, waiting for each whole update
before starting the next system. `Scene::update(frame, scheduler, editMode)` returns
only after the tick's work finishes. The overload without a scheduler uses a
persistent zero-worker fallback.
While editing, every enabled system runs. Systems start disabled, and entering
Play enables every system on the disposable runtime scene. Use the panel
checkboxes for temporary overrides. Runtime Pause still stops scene updates
entirely.

The scheduler dispatches complete systems; `SystemContext` has no job-submission
handle. A system that touches GLFW, OpenGL, or ImGui must declare
`.onMainThread()`, including paths that may lazily load GPU resources. Other
systems can use the default `AnyWorker` affinity once their called services
support it. With workers configured, these updates run on a worker; with zero
workers, they run inline.

For direct CPU work, the pool exposes one synchronous operation:

```cpp
jobs.runAndWait([&system, &context]
{
    system.update(context);
});
// The whole update has finished; its outputs can now be consumed.
```

Ordinary scene execution uses `SystemScheduler::run(system, context)` to apply
affinity centrally. Exceptions propagate after completion, and the scene does
not start a successor after failure. System timings include dispatch and wait.
Grouped child jobs and automatic range splitting are deferred.

Running systems one at a time preserves exclusive access between updates. It
does not make arbitrary overlapping registry, physics, audio, asset, or platform
access safe; ownership work is still needed before updates can overlap.

Systems and editor tools traverse the same read-only
`context.frame.input.events` vector. `InputSnapshot` has no key/button maps or
input-query helpers. GLFW callbacks feed `EventHandler` on the main thread;
gameplay and editor consumers handle the captured engine events rather than
polling GLFW directly. Reading the batch never removes another reader's input.

| Event | Use |
| --- | --- |
| `KeyPress`, `KeyRelease`, `MouseButtonPress`, `MouseButtonRelease` | One-shot transitions, including a press and release before the next tick |
| `KeyHeld`, `MouseButtonHeld` | Continuous input, emitted once per currently held input in each tick |
| `KeyRepeat` | Native keyboard repeat; separate from continuous held input |
| `MouseMove`, `MouseScroll` | Ordered movement deltas and scroll amounts |
| `MousePosition` | Latest cursor position, retained even when the cursor has not moved |

`Event` exposes engine-defined input values: `KeyType`, `MouseButton`, and
modifier flags. `EventType` identifies press, release, repeat, or held input;
there is no separate `isPressed` flag. `KeyType` covers letters, digits,
navigation, function keys, punctuation, keypad keys, and left/right modifiers.
Unmapped keys use `KeyType::Unknown`. `MouseButton` distinguishes left, right,
middle, buttons 4–8, and unknown or absent buttons.
Native key/button codes and scancodes stay in `EventHandler`'s private platform
state to match callback transitions; consumers never compare GLFW input constants.
Callbacks maintain held input and modifiers. Losing window focus queues releases
and clears that retained state. Cursor position is seeded once when window
callbacks are installed, then updated by cursor callbacks. `captureSnapshot()`
publishes the retained values without polling key, button, cursor, or focus state;
the run loop still calls `glfwPollEvents()` to dispatch platform callbacks.
Callbacks queue one-shot events until capture drains them; there is no
frame-start input clearing that could discard events delivered between captures.
Pointer events carry their coordinates, drag origin, and `insideSceneViewport`.
Presses outside the viewport do not start scene interactions; releases still
end active drags outside it.
Input event storage is uncapped for now because normal frame batches are small.
Consumers use `context.frame.window` for pure viewport and screen-to-world calculations.
Snapshot reads need no Input/Window resource declarations, and Time is no longer
a scheduling resource. Platform capture and window operations remain on the
main thread. `AnimationPlayer::update(float)` and `Animator::update(float)` take
explicit delta time. Local loop constants set `1/60`-second ticks, at most eight
per presentation frame. Any remaining whole-tick backlog is discarded, keeping
the fractional remainder for the next frame. Ordered one-shot events survive
frames without a tick and appear only in the first catch-up tick. Accumulation
replaces prior held/pointer records with the latest capture, so rendering faster
than simulation does not duplicate continuous actions. Later catch-up ticks
retain only `KeyHeld`, `MouseButtonHeld`, and `MousePosition`.
`InputSnapshot::accumulate()` and `clearTransientState()` share input buffering
between the loops; the accumulator, simulation clock, and pending input remain
local run-loop variables.
Rendering draws the latest simulation transforms once per presentation frame;
UI, previews, and shader clocks use real presentation time. Render interpolation
is deferred in the [project TODO](../../../TODO.md).
See the [fixed-step rendering diagrams](docs/box2d-and-physics.md#fixed-ticks-and-latest-pose-rendering)
for examples of faster rendering and faster simulation.
Scene systems remain sequential; dependency scheduling is later work.
See [Engine Concurrency Design](../../../docs/concurrency-design.md)
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
