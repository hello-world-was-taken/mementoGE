#pragma once

// Window settings
constexpr float WINDOW_WIDTH = 1280;
constexpr float WINDOW_HEIGHT = 720;
constexpr char WINDOW_TITLE[] = "MementoGE";

// Logical game world size units
constexpr float TILE_SIZE = 32.0f;      // 1 tile = 32 units
constexpr int LOGICAL_WIDTH_TILES = 16; // 16 tiles wide
constexpr int LOGICAL_HEIGHT_TILES = 9; // 9 tiles tall

constexpr float LOGICAL_WIDTH = TILE_SIZE * LOGICAL_WIDTH_TILES;   // 512 units
constexpr float LOGICAL_HEIGHT = TILE_SIZE * LOGICAL_HEIGHT_TILES; // 288 units

// OpenGL / Rendering
constexpr int MAX_TEXTURE_UNITS = 16; // Maximum textures per batch
