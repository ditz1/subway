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
Vector3 GridToWorld(Grid grid, int p[2]);

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
    std::vector<Vector3> track; // smoothed track bed position for each path cell, local to the piece
    std::vector<float> track_s; // distance along the track to each of those points
    std::vector<bool> track_sleeper; // whether a sleeper sits at each of those points
    float track_length = 0.0f;
    Model terrain;          // ground, trees and track baked into one mesh
    Model water;
    bool has_water = false;
    bool models_loaded = false;
};

typedef struct Subway Subway;
typedef struct SubwayCar SubwayCar;
