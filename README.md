# MementoGE

Memento is a lightweight 2D game engine built using OpenGL, GLFW, and ImGui, aimed at game prototyping and editor-based level design. It supports rendering, scene editing, input handling, and more.

<img width="1392" alt="image" src="https://github.com/user-attachments/assets/526b1df0-9aef-474b-8b5b-d8f67353ac2e" />
<img width="1392" height="860" alt="image" src="https://github.com/user-attachments/assets/61faf826-368c-4d3d-a6b0-b5a6f2516182" />







---

## Getting Started

### Prerequisites

- **OpenGL** core.
- **GLEW** must be installed **locally** on your system. *(Will be removed as a requirement soon — stay tuned!)*
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
- [ ] Proper logging system

Contributing
Feel free to fork and contribute via pull requests! Bug fixes, improvements, and new features are all welcome.

License
MIT License © 2025 hello_world_was_taken
