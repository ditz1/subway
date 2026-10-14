#pragma once

#include <globals.hpp>

struct SubwayCar {
    Vector3 position;
    GridPosition grid_pos;
    int car_id;
    Vector2 dir;
    Vector3 vel;
    float rotation = 180.0f;
    float pitch = 0.0f;
};

struct Subway {
    Vector3 head;
    Vector2 head_dir;
    Vector3 head_vel;
    float head_rotation = 180.0f;
    float head_pitch = 0.0f;
    float head_s = 0.0f;      // distance along the track to the middle of the front car
    float speed = 0.0f;
    float max_speed = 5.0f;
    float car_spacing = 6.0f; // middle to middle, along the track
    GridPosition grid_pos; // head
    float car_scale = 1.5f;
    std::vector<SubwayCar> trailing_cars;
    std::vector<Vector2> current_path;
    Model head_model;
    Model car_model;
    Texture2D head_texture;
    Texture2D car_texture;
    float distance = 0.0f;
    bool models_loaded = false;
    int turn = 0;
};



void DrawSubway(Subway& subway, bool debug = false);
void AddCar(Subway& subway);
void InitSubway(Subway& subway, int num_cars);
void UpdateSubway(Subway& subway, const std::vector<StagePiece>& pieces, float dt);
void AutoMoveSubway(Subway& subway, float& timer, float& hold, float dt);
void AutoMoveSubwayLite(Subway& subway, float& timer, float& hold, float dt);
