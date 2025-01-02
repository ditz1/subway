#pragma once

#include <globals.hpp> 

static int piece_width = 64;
static int piece_height = 64;

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



void InitStagePiece(std::vector<StagePiece>& pieces, StagePiece& curr, StagePiece prev);
void DrawStagePiece(StagePiece& piece);
void DrawStagePieces(std::vector<StagePiece>& pieces);
void UpdateStagePieces(std::vector<StagePiece>& pieces, Vector3 subway_head);
void InitGrid(Grid& grid, int slices, float spacing);
void RandomizePermutation(Grid& grid);
void GenerateNoise(Grid& grid);
void GenerateNoiseEx(Grid& grid, Color high, Color mid, Color low, float scale);
void FollowPath(Subway& subway, const std::vector<Vector2>& path, const Grid& grid);

