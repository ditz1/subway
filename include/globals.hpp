#pragma once

#include <raylib.h>
#include <raymath.h>
#include <iostream>
#include <vector>
#include <shared.hpp>
#include <astar.hpp>

typedef struct GridPosition{
    int x;
    int y;
} GridPosition;

Vector3 Lerp(Vector3 start, Vector3 end, float amount);

struct StagePiece {
    Vector3 position;
    int width;
    int height;
    float detection_line;
    bool detect_line_crossed = false;
    bool is_active = true;
    int id;
    Grid grid;              // Add grid for perlin noise
    std::vector<Vector2> path; // Add path for subway to follow
};

typedef struct Subway Subway;
typedef struct SubwayCar SubwayCar;
