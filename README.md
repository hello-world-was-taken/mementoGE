# Memento Game Engine

Memento is a lightweight 2D game engine built using OpenGL, GLFW, and ImGui, aimed at game prototyping and editor-based level design. It supports rendering, scene editing, input handling, and more.

<img width="1279" alt="image" src="https://github.com/user-attachments/assets/bfe9ef84-afdf-4ab2-90bb-c9424ae12489" />


---

## ✨ Features

- 🖼️ **Framebuffer-based rendering**: Render the game world into a texture and display it inside an ImGui window.
- 🧭 **Camera system**: Supports panning and zooming via mouse and scroll input.
- 🖱️ **Mouse input**: Drag to pan the camera, click to select objects, move with snapping.
- 🧱 **Grid renderer**: Infinite-feeling grid aligned with camera for intuitive editing.
- 🧩 **Modular engine**: Organized by core, rendering, editor, and scene systems.
- 💡 **ImGui-based editor**: Custom editor layer with live tools and viewport.
- 🎯 **High-DPI support**: Uses framebuffer size for accurate mouse and rendering behavior.

---

## 🚀 Getting Started

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

## 🧪 Controls

| Action           | Input                            |
|------------------|----------------------------------|
| Pan camera       | Left mouse drag                  |
| Zoom in/out      | Scroll wheel / Touchpad pinch    |
| Select object    | Left click on object             |
| Move object      | Drag selected object (snaps to grid) |

## 🛠️ Remaining Features / TODO

- [ ] Zoom centering on mouse cursor
- [ ] Support for touchpad gestures (pinch-to-zoom, two-finger pan)
- [ ] (`inprogress`) Dedicated event system (abstract input handling from ImGui + GLFW)
- [x] Scene serialization (saving/loading scenes)
- [x] Prefab system for reusable GameObjects
- [ ] Remove GLEW requirement by bundling or replacing
- [ ] (`inprogress`) Gizmo tools - (translate/rotate/scale handles)
- [ ] Custom shader and material editing support
- [ ] (`inprogress`) Physics integration (Box2D)
- [ ] Audio system (OpenAL or FMOD)
- [x] Hot-reload shaders and assets during runtime
- [x] Cross-platform support (Window/MacOS)
- [ ] Sprite based animation
- [ ] Proper logging system

🤝 Contributing
Feel free to fork and contribute via pull requests! Bug fixes, improvements, and new features are all welcome.

📄 License
MIT License © 2025 hello_world_was_taken
