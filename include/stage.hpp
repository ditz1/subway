#pragma once

#include <globals.hpp> 

static int piece_width = 64;
static int piece_height = 64;





void InitStagePiece(std::vector<StagePiece>& pieces, StagePiece& curr, StagePiece prev);
void DrawStagePiece(StagePiece& piece);
void DrawStagePieces(std::vector<StagePiece>& pieces);
void UpdateStagePieces(std::vector<StagePiece>& pieces, Vector3 subway_head);
void InitGrid(Grid& grid, int slices, float spacing);
void RandomizePermutation(Grid& grid);
void GenerateNoise(Grid& grid);
void GenerateNoiseEx(Grid& grid, Color high, Color mid, Color low, float scale);
void FollowPath(Subway& subway, const std::vector<StagePiece>& pieces);
