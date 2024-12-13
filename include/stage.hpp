#pragma once

#include <globals.hpp> 

static int piece_width = 64;
static int piece_height = 64;

struct StagePiece {
    Vector3 position;
    int width = piece_width;
    int height = piece_height;
    float detection_line;
    bool detect_line_crossed = false;
    bool is_active = true;
    int id;
};

void InitStagePiece(std::vector<StagePiece>& pieces, StagePiece& curr, StagePiece prev);
void DrawStagePiece(StagePiece& piece);
void DrawStagePieces(std::vector<StagePiece>& pieces);
void UpdateStagePieces(std::vector<StagePiece>& pieces, Vector3 subway_head);