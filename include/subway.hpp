#pragma once

#include <globals.hpp>

struct SubwayCar {
    Vector3 position;
    GridPosition grid_pos;
    int car_id;
    Vector2 dir;
    Vector3 vel;
    float rotation = 180.0f;
};

struct Subway {
    Vector3 head;
    Vector3 last_pos; // purely for anim
    Vector2 head_dir;
    Vector3 head_vel;
    std::vector<Vector3> path_history;
    int last_idxs[5];
    int last_path_index;
    float head_rotation = 180.0f;
    GridPosition grid_pos; // head
    float car_scale = 1.5f;
    std::vector<SubwayCar> trailing_cars;
    std::vector<Vector2> current_path;
    size_t current_path_index = 0;
    size_t current_stage_piece_index = 0;
    Model head_model;
    Model car_model;
    Texture2D head_texture;
    Texture2D car_texture;
    float distance = 0.0f;
    bool models_loaded = false;
    int turn = 0;
};



void DrawSubway(Subway& subway);
void AddCar(Subway& subway);
void InitSubway(Subway& subway, int num_cars);
void UpdateSubway(Subway& subway);
void AutoMoveSubway(Subway& subway, float& timer, float& hold, float dt);
void AutoMoveSubwayLite(Subway& subway, float& timer, float& hold, float dt);
