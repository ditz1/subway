#pragma once

#include <globals.hpp>

// Terrain is a pure function of world position and the seed, so stage pieces
// line up with each other no matter what order they are generated in.
void InitTerrain(int seed); // call after InitWindow, loads the terrain shaders
void UnloadTerrain();
float TerrainHeight(float wx, float wz); // natural height, before the track is cut in

// fills in the piece's heights, path and meshes. piece.position and piece.grid must be set
void BuildStagePiece(StagePiece& piece, bool straight_path);
void UnloadStagePiece(StagePiece& piece);

// world position on top of the track for path cell i
Vector3 PathPoint(const StagePiece& piece, size_t i);

// the depth shader while a shadow pass is open, the lit terrain shader otherwise
Shader GetTerrainShader();
Texture2D GetShadowMap();

// draw anything that should cast a shadow between these, before the main 3D pass
void BeginShadowPass(Vector3 focus);
void EndShadowPass();
void UpdateTerrainShaders(Camera3D camera, float time);
Color GetSkyColor();
