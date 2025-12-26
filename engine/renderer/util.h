#pragma once

#include "renderer/Constants.h"

#include <vector>

/**
 * @brief Generates indices to draw box using GL_LINES.
 *
 * Each box has 4 vertices added in this order:
 *   0: topLeft, 1: bottomLeft, 2: bottomRight, 3: topRight
 *
 * To form the rectangle edges, we add 4 lines:
 *   - topLeft → bottomLeft
 *   - bottomLeft → bottomRight
 *   - bottomRight → topRight
 *   - topRight → topLeft
 *
 * Each box contributes 8 indices (4 lines * 2 vertices).
 */
void inline generateIndexArrayForLines(std::vector<unsigned int> &indices)
{
    indices.resize(BATCH_SIZE * INDICES_PER_LINE_QUAD);
    for (int i = 0; i < 1000; i++)
    {
        // topLeft -> bottomLeft
        indices[i * INDICES_PER_LINE_QUAD] = i * 4;
        indices[i * INDICES_PER_LINE_QUAD + 1] = i * 4 + 1;

        // bottomLeft -> bottomRight
        indices[i * INDICES_PER_LINE_QUAD + 2] = i * 4 + 1;
        indices[i * INDICES_PER_LINE_QUAD + 3] = i * 4 + 2;

        // bottomRight -> topRight
        indices[i * INDICES_PER_LINE_QUAD + 4] = i * 4 + 2;
        indices[i * INDICES_PER_LINE_QUAD + 5] = i * 4 + 3;

        // topRight -> topLeft
        indices[i * INDICES_PER_LINE_QUAD + 6] = i * 4 + 3;
        indices[i * INDICES_PER_LINE_QUAD + 7] = i * 4;
    }
}

/**
 * @brief Generates indices to draw box using GL_TRIANGLES.
 *
 * The pattern of the indices is the same for each
 * quad:
 *
 *   0, 1, 2, 0, 3, 2     // then by adding 4 to each index
 *  +4,+4,+4,+4,+4,+4
 *   4, 5, 6, 4, 7, 6
 *
 * generateIndexArray, generates the indices for each quad
 */
void inline generateIndexArrayForTriangle(std::vector<unsigned int> &indices)
{
    indices.resize(BATCH_SIZE * INDICES_PER_QUAD);
    for (int i = 0; i < BATCH_SIZE; i++)
    {
        indices[i * INDICES_PER_QUAD] = i * 4;
        indices[i * INDICES_PER_QUAD + 1] = i * 4 + 1;
        indices[i * INDICES_PER_QUAD + 2] = i * 4 + 2;
        indices[i * INDICES_PER_QUAD + 3] = i * 4;
        indices[i * INDICES_PER_QUAD + 4] = i * 4 + 3;
        indices[i * INDICES_PER_QUAD + 5] = i * 4 + 2;
    }
}
