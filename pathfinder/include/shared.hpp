#pragma once

#include <raylib.h>
#include <raymath.h>
#include <iostream>
#include <vector>

typedef struct Cell {
   Vector2 grid_pos;
   float perlin_value;
   Color color;
} Cell;

typedef struct Grid {
   std::vector<Vector3> draw_positions;
   std::vector<Cell> cells;
   float size;
   unsigned char permutation[256];
} Grid;