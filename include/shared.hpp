#pragma once

#include <raylib.h>
#include <raymath.h>
#include <iostream>
#include <vector>

// world height of the water surface, terrain below this is flooded
static const int SEA_LEVEL = 0;

typedef struct Cell {
   Vector2 grid_pos;
   float height; // uncarved terrain height of this column, used for pathfinding
} Cell;

typedef struct Grid {
   std::vector<Vector3> draw_positions;
   std::vector<Cell> cells;
   float size;
} Grid;

