#pragma once

#include <globals.hpp> 

static int piece_width = 64;
static int piece_height = 64;





void InitStagePiece(std::vector<StagePiece>& pieces, StagePiece& curr, StagePiece prev);
void DrawStagePiece(StagePiece& piece);
void DrawStagePieces(std::vector<StagePiece>& pieces);
void UpdateStagePieces(std::vector<StagePiece>& pieces, Subway& subway);
void InitGrid(Grid& grid, int slices, float spacing);
void DrawStageWater(std::vector<StagePiece>& pieces);
void DrawPathMarkers(const StagePiece& piece);

// Each piece's track starts where the previous one ends, so together they make one line.
// Positions on it are given as distance along the line from the start of pieces[0]
float TrackLength(const std::vector<StagePiece>& pieces);
Vector3 TrackPosition(const std::vector<StagePiece>& pieces, float s);
