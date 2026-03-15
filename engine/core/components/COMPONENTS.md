# Component System Overview

This document describes the built‑in engine components, how they interact with each other and with editor tooling, and how to define game‑specific components so that they participate in serialization and the editor UI.

---

## 1. Core Components

Core components live in `engine/core/components/` and are available to all games. Most are directly surfaced in the editor's **Properties** panel and can be added/removed at edit time.

### 1.1 Spatial & Rendering Basics

- **Transform** (`Transform.h`)
  - Stores position, rotation, and scale of a game object.
  - Used by almost every system (rendering, physics, gizmos, text layout, etc.).
  - Updated by editor move tools (selection gizmos, snap‑to‑grid movement).

- **EntityInfo** (`EntityInfo.h`)
  - Stores metadata such as a tag/name, logical width/height, and a **UUID** (`uint64_t`).
  - The UUID is auto-generated on creation via `EntityInfo::generateUUID()` and persists across save/load.
  - Used when inferring default collider sizes, when exporting models, and as a stable identity for cross-entity references (see `EntityRef`).

- **RenderLayer** (`RenderLayer.h`)
  - Encodes which render layer an object belongs to (e.g., background, gameplay, UI).
  - Used by the renderer to draw objects in the correct order/group.

- **Sprite** (`Sprite.h`)
  - Holds UVs, size, and texture reference for 2D sprite rendering.
  - When a sprite is present, `Renderer2D` will draw a textured quad for the object.
  - Often paired with `Animator` (for animated sprites) and `RenderLayer`.

- **Animator** (`Animator.h`)
  - Manages a mapping from animation name → JSON spritesheet file and plays animations.
  - Produces a current `Sprite` for rendering.
  - Supports:
    - Play / pause / stop.
    - Drag & drop of animations from the **Assets → Animations** panel (payload type `ANIMATION`).

- **Text** (`Text.h`)
  - Represents simple bitmap text attached to a game object.
  - Fields: `content`, `fontSize`, `color`, and a font reference.
  - In the editor, you can:
    - Edit content and styling in the **Properties** panel.
    - Drag & drop a font asset (payload type `FONT`) from **Assets → fonts** onto the font area to assign the font.

- **TextAnchor** (`TextAnchor.h`)
  - Stores alignment/anchoring information for text.
  - Used by text layout systems to position text relative to the gameobject that owns the component.

- **ParticleEmitter** (`ParticleEmitter.h`)
  - Encapsulates parameters for GPU/CPU particle systems and a list of live particles.
  - Typically used with `Renderer2D`'s particle system.

- **PostProcessSettings** (`PostProcessSettings.h`)
  - Stores post‑processing configuration (blur, bloom, color‑grading, etc.).
  - Intended primarily for camera / full‑screen effects.

### 1.2 Physics Components

- **RigidBody2D** (`RigidBody2D.h`)
  - Represents a dynamic or static 2D body used by the physics system.
  - Registered with physics when added via the **Add Component** combo.
  - Inspector exposes body type, mass, damping, etc. (see implementation for exact fields).

- **BoxCollider2D** (`BoxCollider2D.h`)
  - Axis‑aligned box collider used with `RigidBody2D`.
  - When added from the Properties panel, its default size is initialized from `EntityInfo.width/height`.

- **CircleCollider2D** (`CircleCollider2D.h`)
  - Circular collider.
  - Registered with physics upon addition.

- **PolygonCollider2D** (`PolygonCollider2D.h`)
  - Generic polygon collider for more complex shapes.

- **EdgeCollider2D** (`EdgeCollider2D.h`)
  - A line/edge collider, often used for one‑way platforms or boundaries.

- **Sensor2D** (`Sensor2D.h`)
  - Trigger collider: participates in overlap queries but not physical responses.
  - Registered separately (via `registerSensor2D`) when added.

- **JointComponent** (`JointComponent.h`)
  - Holds configuration for physics joints/constraints between bodies.

These physics components cooperate with the engine's 2D physics system. Game objects that should participate in physics generally have a `RigidBody2D` plus one or more colliders.

### 1.3 Camera & Visual Effects

- **Camera** (`Camera.h`)
  - Describes a gameplay camera: logical width/height, primary flag, etc.
  - Used by the runtime to determine the main view; the editor can optionally visualize the camera bounds.

- **PostProcessSettings** (also listed above)
  - Typically attached to the camera object that owns the screen‑space effects.

### 1.4 Audio & Gameplay Logic

- **AudioSource** (`AudioSource.h`)
  - Encapsulates a single sound source: audio file path, looping, gain, pitch.
  - In the editor inspector you can:
    - Assign an audio clip by dragging a `.wav`/`.ogg` asset from the **Assets** panel (payload type `AUDIO`).
    - Adjust gain, pitch, and looping.
    - Preview playback with **Play** / **Stop** buttons.

### 1.5 Entity Relationships

- **EntityRef** (`EntityRef.h`)
  - Stores a reference from one entity to another using `targetUuid` (persistent, serialized) and a cached `target` (`entt::entity`, runtime-only).
  - Allows entities to be associated (e.g., a health bar pointing to the player it tracks, a projectile referencing its owner).
  - **Key fields:**
    - `targetUuid` (`uint64_t`) — the UUID of the target entity (from `EntityInfo::uuid`). Serialized to YAML.
    - `target` (`entt::entity`) — runtime cache resolved lazily. Not serialized.
  - **`resolve(registry)`** — looks up the target entity by UUID in the registry. Caches the result for O(1) subsequent access. Returns `true` if found. Call this before accessing `target`.
  - **Editor UI:** a combo dropdown listing all entities in the scene (by tag) lets you pick or clear the target. The `drawInspector(GameObject &go)` method takes a `GameObject` reference (matching the standard component pattern) and internally extracts the registry and entity ID from it.
  - **Serialization:** only the `targetUuid` is saved. On scene load, call `resolve()` to re-establish the runtime entity handle.

  **Usage in systems:**

  ```cpp
  auto view = registry.view<EntityRef>();
  for (auto entity : view)
  {
      auto &ref = view.get<EntityRef>(entity);
      if (ref.resolve(registry))
      {
          // Access any component on the referenced entity
          auto &health = registry.get<Health>(ref.target);
          // e.g., update a health bar display based on health.value
      }
  }
  ```

  **Resolving after scene load:**

  EntityRef stores the UUID, not the runtime entity handle. After deserialization the `target` field is `entt::null`. The first call to `resolve()` scans the registry to find the entity with the matching UUID and caches it. Subsequent calls return immediately if the cached entity is still valid.

### 1.6 Animation & Particles (Data Helpers)

- **Animation** / **Frame** (`Animation.h`)
  - Data structs representing animations built from spritesheet JSON.
  - Used by `AnimationMap` / `AnimationPlayer` and `Animator`.

- **Particle** / **ParticleEmitter** (`ParticleEmitter.h`)
  - `Particle` is the per‑instance data; `ParticleEmitter` configures emission and holds live particles.

---

## 2. Editor Interactions & Workflows

This section focuses on how components are created, edited, and wired together inside the editor.

### 2.1 Properties Panel

The **Properties** panel is the main place where you view and edit components on the currently selected game object.

- When a single object is selected, the panel:
  - Draws inspectors for all built‑in components present on that object.
  - Invokes `ComponentRegistry` to draw inspectors for game‑specific components.

- **Editing fields**
  - Numeric values (e.g., Transform position, RigidBody settings, Health value) are edited via ImGui widgets (`DragFloat`, `SliderFloat`, etc.) and take effect immediately.
  - Text values (e.g., Text content, tags) are edited via ImGui input fields.

- **Adding components**
  - The **Add Component** combo in the Properties panel lets you add components to the selected object.
  - Built‑in options include (among others): `Rigidbody2D`, `BoxCollider2D`, `CircleCollider2D`, `Sensor2D`, `Animator`, `Audio Source`, `Enemy Stats`, `Patrol`, `Text`, `Text Anchor`, `Particle Emitter`, `Post Processing Settings`.
  - For some components, the editor also performs additional wiring:
    - Adding `RigidBody2D` or collider components registers them with the physics system.
    - Adding `BoxCollider2D` initializes its size from `EntityInfo`.

- **Game‑specific components**
  - Any entries registered via `ComponentRegistry` appear at the bottom of the **Add Component** combo.
  - Selecting one of these entries calls the registered `AddComponentFn` to attach the component to the object.

- **Deleting objects**
  - The bottom of the Properties panel includes a **Delete Object** button which removes the entire game object from the scene (and clears selection).

- **Exporting models**
  - The **Export Model** button serializes a subset of components (e.g., `EntityInfo`, `Transform`, `RenderLayer`, `Sprite`, physics, text, particles, post‑processing, etc.) as a YAML model under `assets/models/`.
  - These models can later be instantiated by drag & drop from the **Assets** panel (see below).

### 2.2 Drag & Drop From Assets

Several components are configured via drag & drop from the **Assets** window.

- **Sprites → Scene viewport**
  - Source:
    - **Assets → texture → Sprites** panel: each sprite in the current spritesheet is shown as an `ImageButton`.
    - Dragging a sprite creates a payload of type `SPRITE` containing its UVs, size, and texture.
  - Target:
    - Scene viewport (Scene panel): when the drop target accepts `SPRITE`, the editor:
      - Creates a new `GameObject` with appropriate size (based on sprite aspect ratio).
      - Adds a `Sprite` component initialized from the payload.
      - Positions the new object at the mouse world position (via `Transform`).

- **Models → Scene viewport**
  - Source:
    - **Assets → models** folder: each `.yaml` file represents a serialized model.
    - Dragging a model produces a payload of type `MODEL` containing the model asset path.
  - Target:
    - Scene viewport: accepting `MODEL` calls `Scene::addGameObjectFromModel`, which deserializes the components defined in the model and positions the new object under the mouse cursor.

- **Fonts → Text component**
  - Source:
    - **Assets → fonts**: each `.ttf` file has a drag source that emits a `FONT` payload containing the font path.
  - Target:
    - `Text` component inspector in the Properties panel: accepting `FONT` sets the `font` on the `Text` component and triggers a text rebuild.

- **Audio → AudioSource component**
  - Source:
    - **Assets → audio**: `.wav` or `.ogg` assets emit an `AUDIO` payload.
  - Target:
    - `AudioSource` component inspector: accepting `AUDIO` sets the `filePath` of the `AudioSource`.

- **Animations → Animator component**
  - Source:
    - **Assets → Animations** panel: each animation in the selected spritesheet emits an `ANIMATION` payload containing the animation name and JSON path.
  - Target:
    - `Animator` inspector: accepting `ANIMATION` registers the animation in `animationSourceMap` under its name. You can then select it and play/pause/stop from the inspector.

### 2.3 Scene View & Movement

While not components by themselves, some editor behaviours are tightly coupled with component data.

- **Transform & grid snapping**
  - The editor supports both free‑move and snap‑to‑grid modes.
  - When snapping is enabled, object centers are aligned to the grid such that `Transform.position` (x, y) matches the center of the corresponding grid cell.

- **Camera bounds overlay**
  - The editor can draw a camera frame overlay representing the logical game viewport.
  - This helps you visualize what will be visible at runtime relative to object positions, sprites, and UI components.

---

## 3. Game‑Specific Components

Game-specific components live under `game/components/`. Today they integrate with the engine primarily through:

- Direct serialization hooks implemented on the component itself (YAML `serialize` / `deserialize`).
- Gameplay systems under `game/systems/` that read/write those components.

There is also a generic `ComponentRegistry` in the engine that lets game code plug in extra behavior (custom inspectors, add/remove hooks, and optional extra serialization) by registering free functions/lambdas that all take a `GameObject &` as their first parameter. The current game components do **not** use this yet, but you can adopt it incrementally for more advanced editor workflows.

### 3.1 Expected Structure for Game Components

When adding a new game‑specific component, there are two layers you can opt into:

1. **Baseline pattern**
   - Define a plain struct under `game/components/`:

     ```cpp
     // game/components/MyComponent.h
     #pragma once

     struct MyComponent
     {
         // Runtime data
         float someValue = 0.0f;

     #ifdef EDITOR_BUILD
         void serialize(YAML::Emitter &out);
         void deserialize(const YAML::Node &node);
         void drawInspector();
     #endif
     };
     ```

   - In `MyComponent.cpp`, implement:
     - `serialize` writing a top‑level map keyed by the component name (e.g. `"MyComponent"`).
     - `deserialize` reading from that map if present.
     - `drawInspector` to expose fields via ImGui.
   - Use the component directly from your systems (e.g. `registry.view<MyComponent, Transform>()`).

   This is sufficient for full runtime use and for scenes/models to persist your component as long as you participate in serialization where appropriate.

2. **Optional: integrate with `ComponentRegistry` for editor wiring**
   - If you want the generic editor UI to know how to add/remove and draw your component, you can also register it with `ComponentRegistry`:
     - Include `core/ComponentRegistry.h` in your game startup code.
     - Early in initialization (before scenes are deserialized), call:

       ```cpp
         ComponentRegistry::instance().registerComponent(
           "MyComponent",
           // Deserialize callback: signature matches
           //   void(GameObject &, const YAML::Node &)
           /* deserialize = */ [](GameObject &go, const YAML::Node &node) {
             go.deserializeComponent<MyComponent>(node, "MyComponent");
           },
           // Serialize callback: signature matches
           //   void(GameObject &, YAML::Emitter &)
           /* serialize = */ [](GameObject &go, YAML::Emitter &out) {
             go.serializeComponent<MyComponent>(out);
           }
         #ifdef EDITOR_BUILD
           ,
           // Editor-only callbacks: each takes GameObject & and
           // delegates to your component's member functions.
           /* drawInspector = */ [](GameObject &go) {
             go.getComponent<MyComponent>().drawInspector();
           },
           /* addComponent = */ [](GameObject &go) {
             go.addComponent<MyComponent>();
           },
           /* removeComponent = */ [](GameObject &go) {
             go.removeComponent<MyComponent>();
           }
         #endif
         );
       ```

   - What the registry hooks do (and their expected shapes):
     - `DeserializeFn` – `void(GameObject &, const YAML::Node &)`; used by `GameObject` to let you deserialize your component when loading scenes/models.
     - `SerializeFn` – `void(GameObject &, YAML::Emitter &)`; used by `GameObject` to let you serialize your component when saving.
     - `DrawInspectorFn` – `void(GameObject &)`; used by `PropertiesPanel` to draw extra inspectors for registered components.
     - `AddComponentFn` – `void(GameObject &)`; used by the **Add Component** combo in `PropertiesPanel` to attach your component.
     - `RemoveComponentFn` – `void(GameObject &)`; used by `SceneHierarchyPanel` to show a removable row and remove your component when requested.

3. **Use in systems**
   - In your gameplay systems (e.g. under `game/systems/`), access components via `entt::registry` or `GameObject` helpers:
     - `registry.view<MyComponent, Transform>()` or
     - `go.getComponent<MyComponent>()`.
   - Combine with core components where appropriate.

### 3.2 Serialization & Models

- When a scene or model is serialized:
  - Core components are serialized directly by the engine.
  - Game‑specific components are serialized/deserialized via `ComponentRegistry` entries.

- For models exported from the Properties panel:
  - Only a known subset of components is serialized by default.
  - You can extend this behaviour by adding `serialize` entries in `ComponentRegistry` for your own components.

---

## 4. Adding a New Engine Component — Checklist

When adding a new component to the engine (under `engine/core/components/`), update **all** of the following locations:

| #   | File                                           | What to do                                                                                                                                                                                                                                                                                              |
| --- | ---------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1   | `engine/core/components/NewComponent.h/.cpp`   | Create the component struct with `serialize`, `deserialize`, and `drawInspector` (gated by `EDITOR_BUILD`).                                                                                                                                                                                             |
| 2   | `CMakeLists.txt` (engine)                      | Add the `.cpp` to the engine's `add_library` source list.                                                                                                                                                                                                                                               |
| 3   | `engine/core/GameObject.cpp`                   | Add `#include`, add `deserializeComponent<NewComponent>(...)` in the deserialization constructor, add `serializeComponent<NewComponent>(out)` in `serialize()`.                                                                                                                                         |
| 4   | `engine/editor/panels/PropertiesPanel.cpp`     | Add `#include`, add `drawComponentInspector<NewComponent>(go)` in `renderPropertiesPanel()` (or a manual draw call if the inspector has a non-standard signature), add an `ImGui::Selectable("NewComponent")` entry in `drawAddComponentCombo()`, add `serializeComponent` call in `drawExportModel()`. |
| 5   | `engine/editor/panels/SceneHierarchyPanel.cpp` | Add `#include`, add `drawComponentRemoveRow<NewComponent>(go, "Display Name")` in `drawGameObjectNode()`.                                                                                                                                                                                               |
| 6   | `engine/core/components/COMPONENTS.md`         | Document the component in the appropriate section.                                                                                                                                                                                                                                                      |

> **Note:** If the component requires special wiring (e.g., physics registration), add the appropriate calls in `PropertiesPanel::drawAddComponentCombo` and in `GameObject`'s deserialization constructor.

---

## 5. Summary

- Core components in `engine/core/components/` cover common 2D game needs: transforms, sprites, text, physics, audio, camera, particles, and simple AI.
- The editor builds on top of these components with:
  - The **Properties** panel for inspecting and editing.
  - The **Add Component** combo for attaching new components.
  - Drag & drop from **Assets** into the scene viewport or individual inspectors.
- Game‑specific components live under `game/components/` and are integrated via `ComponentRegistry` plus optional editor hooks.
- Following the expected structure for game components ensures they can be:
  - Serialized and deserialized with scenes and models.
  - Edited via the same unified editor workflows as core components.
