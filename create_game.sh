#!/usr/bin/env bash
set -euo pipefail

# Usage: ./create_game.sh MyNewGame

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <GameName>" >&2
  exit 1
fi

GAME_NAME="$1"
ROOT_DIR="$(pwd)"
GAME_ROOT="${ROOT_DIR}/${GAME_NAME}"

if [ -e "${GAME_ROOT}" ]; then
  echo "Error: ${GAME_ROOT} already exists" >&2
  exit 1
fi

# Create root game directory
echo "[create_game] Creating game directory at ${GAME_ROOT}..."
mkdir -p "${GAME_ROOT}"

# Initialize a new git repository for this game (if not already one)
cd "${GAME_ROOT}"
if [ ! -d .git ]; then
  echo "[create_game] Initializing new git repository..."
  git init >/dev/null 2>&1
else
  echo "[create_game] Existing git repository detected, reusing it."
fi

# Create game subfolder and base layout inside it
echo "[create_game] Creating base game folder layout..."
mkdir -p game
cd game

mkdir -p assets components lib systems

# Assets subdirs
mkdir -p assets/audio assets/fonts assets/models assets/scenes assets/textures

# Add mementoGE as submodule under game/lib/memento inside this game repo
cd "${GAME_ROOT}"

echo "[create_game] Adding mementoGE as git submodule (game/lib/memento)..."
git submodule add https://github.com/hello-world-was-taken/mementoGE.git "game/lib/memento" >/dev/null 2>&1 || {
  echo "[create_game] Warning: git submodule add failed. Ensure this directory is a git repo and that 'game/lib/memento' is unique." >&2
}

# Ensure mementoGE's own submodules are initialized (engine dependencies)
if [ -d "${GAME_ROOT}/game/lib/memento" ]; then
  echo "[create_game] Initializing mementoGE engine submodules (this may take a while)..."
  cd "${GAME_ROOT}/game/lib/memento"
  git submodule update --init --recursive >/dev/null 2>&1 || {
    echo "[create_game] Warning: failed to recursively initialize mementoGE submodules. You may need to run 'git submodule update --init --recursive' manually inside game/lib/memento." >&2
  }
  cd "${GAME_ROOT}"
fi

# Create minimal starter files directly (no dependency on an existing project layout)
echo "[create_game] Writing main.cpp..."
cat > "${GAME_ROOT}/main.cpp" << 'EOF'
#include "engine/editor/EditorLayer.h"

#include <iostream>

int main()
{
  std::cout << "MementoGE" << std::endl;
  bool editorMode = true;

  if (editorMode)
  {
    EditorLayer editor;
    editor.run();
  }
  else
  {
    std::cout << "TODO: hook up MementoGE and your game here" << std::endl;
  }

  return 0;
}
EOF

echo "[create_game] Writing CMakeLists.txt..."
cat > "${GAME_ROOT}/CMakeLists.txt" << EOF
cmake_minimum_required(VERSION 3.26)

project(${GAME_NAME})

set(CMAKE_CXX_STANDARD 17)

# Global toggle: when ON, both the game and the engine
# are built with editor-only code paths enabled via the
# EDITOR_BUILD macro.
option(EDITOR_BUILD "Build with editor tooling enabled" ON)

set(GAME_ASSETS_DIR "\${CMAKE_CURRENT_SOURCE_DIR}/game/assets/" CACHE INTERNAL "")

add_subdirectory(game/lib/memento)

add_executable(${GAME_NAME}
    main.cpp
    # TODO: add your game source files here
)

target_include_directories(${GAME_NAME}
    PRIVATE .
  PRIVATE game
  PRIVATE game/lib/memento
)

target_link_libraries(${GAME_NAME}
    PRIVATE engine
)

add_custom_target(copy_assets ALL
  COMMAND \${CMAKE_COMMAND} -E copy_directory
    \${CMAKE_SOURCE_DIR}/game/assets
    $<TARGET_FILE_DIR:\${PROJECT_NAME}>/assets
    COMMENT "Copying assets to build directory...")

add_dependencies(\${PROJECT_NAME} copy_assets)

target_compile_definitions(${GAME_NAME} PUBLIC GAME_ASSETS_DIR="\${GAME_ASSETS_DIR}")
if(EDITOR_BUILD)
  target_compile_definitions(\${PROJECT_NAME} PUBLIC EDITOR_BUILD)
endif()
EOF

echo "[create_game] Creating build.sh..."
cat > "${GAME_ROOT}/build.sh" << 'EOF'
#!/usr/bin/env bash
set -euo pipefail

# Ensure build directory exists
BUILD_DIR="out"
if [ ! -d "${BUILD_DIR}" ]; then
  mkdir -p "${BUILD_DIR}"
fi

cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DGLFW_BUILD_DOCS=OFF \
      -S . -B "${BUILD_DIR}"

cmake --build "${BUILD_DIR}" -- -j8
EOF

echo "[create_game] Creating run.sh..."
cat > "${GAME_ROOT}/run.sh" << EOF
#!/usr/bin/env bash
set -euo pipefail

"\${PWD}/build.sh"
cd "\${PWD}/out"
"\${PWD}/${GAME_NAME}"
EOF

chmod +x "${GAME_ROOT}/build.sh" "${GAME_ROOT}/run.sh"

# Create initial build directory for builds
echo "[create_game] Preparing initial out/ build directory..."
mkdir -p "${GAME_ROOT}/out"

echo "[create_game] Writing .clang-format..."
cat > "${GAME_ROOT}/.clang-format" << 'EOF'
BasedOnStyle: LLVM
IndentWidth: 4
UseTab: Never
BreakBeforeBraces: Allman
AllowShortIfStatementsOnASingleLine: false
AllowShortLoopsOnASingleLine: false
AllowShortFunctionsOnASingleLine: None
ColumnLimit: 120

# Keep lambdas compact but break before the body brace
LambdaBodyIndentation: Signature
AllowShortLambdasOnASingleLine: None
AlignAfterOpenBracket: DontAlign
BinPackParameters: false
BinPackArguments: false

# Align access modifiers (public/private/protected) to the left edge
AccessModifierOffset: -4


InsertBraces: true
EOF

echo "[create_game] Creating .vscode configuration..."
mkdir -p "${GAME_ROOT}/.vscode"

cat > "${GAME_ROOT}/.vscode/launch.json" << EOF
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Launch ${GAME_NAME}",
      "type": "cppdbg",
      "request": "launch",
      "program": "\${workspaceFolder}/out/${GAME_NAME}",
      "args": [],
      "cwd": "\${workspaceFolder}",
      "stopAtEntry": false,
      "externalConsole": false,
      "MIMode": "lldb"
    }
  ]
}
EOF

cat > "${GAME_ROOT}/.vscode/tasks.json" << 'EOF'
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Build Game",
      "type": "shell",
      "command": "${workspaceFolder}/build.sh",
      "group": {
        "kind": "build",
        "isDefault": true
      },
      "problemMatcher": []
    }
  ]
}
EOF

echo "[create_game] Game skeleton created at ${GAME_ROOT}"