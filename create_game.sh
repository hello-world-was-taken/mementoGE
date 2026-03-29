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
mkdir -p "${GAME_ROOT}"

# Initialize a new git repository for this game (if not already one)
cd "${GAME_ROOT}"
if [ ! -d .git ]; then
  git init >/dev/null 2>&1
fi

# Create game subfolder and base layout inside it
mkdir -p game
cd game

mkdir -p assets components lib systems

# Assets subdirs
mkdir -p assets/audio assets/fonts assets/models assets/scenes assets/textures

# Add mementoGE as submodule under game/lib/memento inside this game repo
cd "${GAME_ROOT}"

git submodule add https://github.com/hello-world-was-taken/mementoGE.git "game/lib/memento" >/dev/null 2>&1 || {
  echo "Warning: git submodule add failed. Ensure this directory is a git repo and that 'game/lib/memento' is unique." >&2
}

# Ensure mementoGE's own submodules are initialized (engine dependencies)
if [ -d "${GAME_ROOT}/game/lib/memento" ]; then
  cd "${GAME_ROOT}/game/lib/memento"
  git submodule update --init --recursive >/dev/null 2>&1 || {
    echo "Warning: failed to recursively initialize mementoGE submodules. You may need to run 'git submodule update --init --recursive' manually inside game/lib/memento." >&2
  }
  cd "${GAME_ROOT}"
fi

# Create minimal starter files directly (no dependency on an existing project layout)
cat > "${GAME_ROOT}/main.cpp" << 'EOF'
#include <iostream>

int main()
{
    std::cout << "TODO: hook up MementoGE and your game here" << std::endl;
    return 0;
}
EOF

cat > "${GAME_ROOT}/CMakeLists.txt" << EOF
cmake_minimum_required(VERSION 3.26)

project(${GAME_NAME})

set(CMAKE_CXX_STANDARD 17)

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

target_compile_definitions(${GAME_NAME} PUBLIC GAME_ASSETS_DIR="\${GAME_ASSETS_DIR}")
EOF

cat > "${GAME_ROOT}/build.sh" << 'EOF'
#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="out"

cmake -S . -B "${BUILD_DIR}"
cmake --build "${BUILD_DIR}"
EOF

cat > "${GAME_ROOT}/run.sh" << EOF
#!/usr/bin/env bash
set -euo pipefail

"\${PWD}/out/${GAME_NAME}"
EOF

chmod +x "${GAME_ROOT}/build.sh" "${GAME_ROOT}/run.sh"

# Create initial build directory for builds
mkdir -p "${GAME_ROOT}/out"

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

echo "Game skeleton created at ${GAME_ROOT}"