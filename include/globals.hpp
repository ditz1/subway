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

typedef struct StagePiece StagePiece;
typedef struct Subway Subway;
typedef struct SubwayCar SubwayCar;
