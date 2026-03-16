# OpenGL Abstraction Layer

This directory contains thin C++ wrappers around core OpenGL objects. Each class maps one-to-one to an OpenGL concept and follows the same **bind → operate → unbind** state-machine pattern that OpenGL uses.

---

## Components

### Vertex (`Vertex.h`)

A plain data struct representing a single vertex sent to the GPU.

| Field      | Type   | Purpose                                 |
| ---------- | ------ | --------------------------------------- |
| `position` | `vec3` | World-space position (x, y, z)          |
| `color`    | `vec4` | RGBA vertex color                       |
| `texture`  | `vec2` | UV texture coordinates                  |
| `texSlot`  | `int`  | Index into the bound texture unit array |

### VertexAttribute (`VertexAttribute.h`)

Describes how the GPU should interpret raw bytes inside a vertex buffer — essentially the metadata passed to `glVertexAttribPointer`.

| Field             | Meaning                                         |
| ----------------- | ----------------------------------------------- |
| `m_vector_length` | Number of components (e.g. 3 for vec3)          |
| `m_type`          | GL type (`GL_FLOAT`, `GL_INT`, etc.)            |
| `m_normalized`    | Whether to normalize fixed-point data           |
| `m_stride`        | Byte distance between consecutive vertices      |
| `m_offset`        | Byte offset of this attribute within one vertex |

### VertexBuffer / VBO (`VertexBuffer.h/.cpp`)

Allocates a block of GPU memory for vertex data.

| OpenGL call                         | When                                   |
| ----------------------------------- | -------------------------------------- |
| `glGenBuffers(1, &id)`              | Constructor — create                   |
| `glBindBuffer(GL_ARRAY_BUFFER, id)` | `bind()`                               |
| `glBufferData(..., nullptr, type)`  | Constructor — allocate                 |
| `glBufferSubData(..., data)`        | `updateBufferData()` — upload vertices |
| `glDeleteBuffers(1, &id)`           | Destructor                             |

The buffer is allocated once at a fixed size and then updated per-frame via `updateBufferData()` with the current batch of `Vertex` structs.

### IndexBuffer / IBO (`IndexBuffer.h/.cpp`)

Stores triangle indices so vertices can be reused (e.g. two triangles forming a quad share two vertices).

| OpenGL call                                     | When                           |
| ----------------------------------------------- | ------------------------------ |
| `glGenBuffers(1, &id)`                          | Constructor — create           |
| `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id)`     | `bind()`                       |
| `glBufferData(GL_ELEMENT_ARRAY_BUFFER, ...)`    | Constructor — allocate         |
| `glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, ...)` | `updateIndicesData()` — upload |
| `glDeleteBuffers(1, &id)`                       | Destructor                     |

### VertexArray / VAO (`VertexArray.h/.cpp`)

Records the association between vertex attributes and the currently bound VBO/IBO. Once a VAO is bound, all subsequent `glVertexAttribPointer` and element buffer binds are captured inside it. Rebinding the VAO later replays those associations automatically.

| OpenGL call                                                   | When                                           |
| ------------------------------------------------------------- | ---------------------------------------------- |
| `glGenVertexArrays(1, &id)`                                   | Constructor                                    |
| `glBindVertexArray(id)`                                       | `bind()`                                       |
| `glEnableVertexAttribArray(idx)`                              | `attachVertexAttribute()`                      |
| `glVertexAttribPointer(...)` or `glVertexAttribIPointer(...)` | `attachVertexAttribute()` — float vs int types |
| `glDeleteVertexArrays(1, &id)`                                | Destructor                                     |

### Shader (`Shader.h/.cpp`)

Compiles a vertex + fragment shader pair into a GPU program and exposes uniform setters.

| OpenGL call                          | When                                              |
| ------------------------------------ | ------------------------------------------------- |
| `glCreateShader(type)`               | Constructor — per stage                           |
| `glShaderSource` + `glCompileShader` | Constructor — compile                             |
| `glCreateProgram` + `glLinkProgram`  | Constructor — link                                |
| `glUseProgram(id)`                   | `use()`                                           |
| `glUniform1i/1f/4fv/4f`              | `setUniform*()`                                   |
| `glUniform1iv`                       | `setMultipleTextureUnits()` — binds sampler array |
| `glDeleteProgram(id)`                | Destructor                                        |

Shader source is read from file at construction time via `parseShader()`.

### Texture (`Texture.h/.cpp`)

Wraps a 2D texture. Supports two creation paths:

- **From file** — loads an image (PNG/JPG) via stb_image
- **From memory buffer** — used for font glyph atlases (single-channel `GL_RED` with swizzle mask)

| OpenGL call                           | When                                  |
| ------------------------------------- | ------------------------------------- |
| `glGenTextures(1, &id)`               | `generateTexture()`                   |
| `glActiveTexture(GL_TEXTURE0 + slot)` | `bind()`                              |
| `glBindTexture(GL_TEXTURE_2D, id)`    | `bind()`                              |
| `glTexImage2D(..., format, data)`     | `generateTexture()` — upload          |
| `glTexParameteri` (wrap, filter)      | `generateTexture()` — sampling config |
| `glDeleteTextures(1, &id)`            | Destructor                            |

The `texSlot` field in `Vertex` indexes into the array of texture units, allowing a single draw call to sample from multiple textures.

### FrameBuffer / FBO (`FrameBuffer.h/.cpp`)

Off-screen render target with a color texture attachment and a depth/stencil renderbuffer. Used by `Renderer2D` to render the scene into a texture that can then be displayed in the editor viewport or used for post-processing.

| OpenGL call                                                               | When                         |
| ------------------------------------------------------------------------- | ---------------------------- |
| `glGenFramebuffers(1, &fbo)`                                              | `init()`                     |
| `glGenTextures(1, &colorTex)` + `glTexImage2D`                            | `init()` — color attach      |
| `glFramebufferTexture2D(COLOR_ATTACHMENT0)`                               | `init()` — attach color      |
| `glGenRenderbuffers(1, &rbo)` + `glRenderbufferStorage(DEPTH24_STENCIL8)` | `init()` — depth/stencil     |
| `glFramebufferRenderbuffer(DEPTH_STENCIL_ATTACHMENT)`                     | `init()` — attach RBO        |
| `glCheckFramebufferStatus`                                                | `init()` — validate          |
| `glBindFramebuffer(GL_FRAMEBUFFER, fbo)`                                  | `bind()`                     |
| `glBindFramebuffer(GL_FRAMEBUFFER, 0)`                                    | `unbind()` — back to default |
| `glDeleteFramebuffers/Textures/Renderbuffers`                             | `destroy()`                  |

Supports deferred resize via `queueSizeUpdate()` / `resize()` to handle editor viewport resizing without recreating every frame.

---

## How They Fit Together — Rendering Pipeline

```
                                         ┌─────────────┐
                                         │ FrameBuffer  │
                                         │ (off-screen) │
                                         └──────┬───────┘
                                                │ bind()
                                                ▼
┌────────┐    ┌──────────────┐    ┌──────────────────────────┐
│ Vertex │───▶│ VertexBuffer │    │       VertexArray        │
│ (data) │    │   (VBO)      │◀───│ (VAO — records layout)   │
└────────┘    └──────────────┘    └──────────────────────────┘
                                          ▲
┌──────────────────┐                      │
│ VertexAttribute  │──────────────────────┘
│ (layout metadata)│    attachVertexAttribute()
└──────────────────┘

┌──────────────┐
│ IndexBuffer  │    Defines triangle winding order
│   (IBO)      │    Bound while VAO is active
└──────────────┘

┌──────────┐   ┌─────────┐
│ Texture  │   │ Shader  │
│ (image)  │   │ (GPU    │
│          │   │ program)│
└──────────┘   └─────────┘
```

A typical frame follows this sequence:

1. **FrameBuffer** `bind()` — redirect rendering off-screen
2. **Shader** `use()` — activate the GPU program
3. **Shader** `setUniform*()` — upload camera matrices, texture samplers
4. **Texture** `bind()` — activate textures in their assigned slots
5. **VertexBuffer** `updateBufferData()` — upload this frame's vertex batch
6. **IndexBuffer** `updateIndicesData()` — upload triangle indices
7. **VertexArray** `bind()` — replay attribute layout + buffer associations
8. `glDrawElements()` — issue the draw call
9. **FrameBuffer** `unbind()` — switch back to the default framebuffer
10. The FBO's color texture (`getColorTexture()`) is rendered to screen or passed to post-processing

This is orchestrated by `RenderBatch` (which owns a VAO + VBO + IBO triplet) and coordinated by `Renderer2D` (which owns the FBO and delegates to specialized renderers like `SpriteRenderer` and `GridRenderer`).
