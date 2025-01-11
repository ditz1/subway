#include <subway.hpp>

static float subway_size = 2.0f;
static float dt = 1.0f / 60.0f;

void DrawSubway(Subway& subway){
    Vector3 car_scale = {subway.car_scale, subway.car_scale, subway.car_scale};
    DrawModelEx(subway.head_model, subway.head, (Vector3){0.0f, 1.0f, 0.0f}, subway.head_rotation, car_scale, WHITE);
    //DrawModel(subway.head_model, subway.head, 1.0f, WHITE);
    for(int i = 0; i < subway.trailing_cars.size(); i++){
        DrawCubeWires(subway.trailing_cars[i].position, subway_size, subway_size, subway_size, BLUE);
        DrawModelEx(subway.car_model, subway.trailing_cars[i].position, (Vector3){0.0f, 1.0f, 0.0f}, subway.trailing_cars[i].rotation, car_scale, WHITE);
    }
}

void AddCar(Subway& subway){
    SubwayCar new_car;
    switch(subway.trailing_cars.size()){
        case 0:
            new_car.position = subway.head;
            break;
        default:
            Vector3 new_pos = subway.trailing_cars.back().position;
            new_pos.z += 1.0f;
            new_car.position = new_pos;
            break;
    }
    new_car.vel = (Vector3){0.0f, 0.0f, 0.0f};
    new_car.grid_pos = subway.grid_pos;
    new_car.car_id = subway.trailing_cars.size();
    subway.trailing_cars.push_back(new_car);
}

void AutoMoveSubwayLite(Subway& subway, float& timer, float& hold, float dt) {
    // Only handle the forward (z) movement here
    if (subway.head_vel.z > -3.0f) {
        subway.head_vel.z -= 0.1f;
    }

    float d = subway.head.z * -1;
    subway.distance = d;
    
}

void AutoMoveSubway(Subway& subway, float& timer, float& hold, float dt){
    if (!subway.current_path.empty()){
        std::cout << "Following path" << std::endl;
    }
    
    if (subway.head_vel.z > -3.0f){
        subway.head_vel.z -= 0.1f;
    }
    if (subway.head.x > 27.0f){
        subway.turn = 2;
        return;
    }
    if (subway.head.x < -27.0f){
        subway.turn = 1;
        return;
    }

    timer += dt;

    if (timer > 2.0f){
        float d  = abs(subway.head.z);
        subway.distance = d;
    }

    if (hold > 0.0f){
        hold -= dt;
        switch(subway.turn){
            case 0:
                if (subway.head_vel.x == 0.0f){
                    subway.head_vel.x = 0.0f;                    
                } else if (subway.head_vel.x > 0.0f){
                    subway.head_vel.x -= 0.1f;
                } else if (subway.head_vel.x < 0.0f){
                    subway.head_vel.x += 0.1f;
                } 
                break;
            case 1:
                if (subway.head_vel.x < 2.0f){
                    subway.head_vel.x += 0.1f;
                }
                break;
            case 2:
                if (subway.head_vel.x > -2.0f){
                    subway.head_vel.x -= 0.1f;
                }
                break;
            default:
                break;
        }
        return;
    } else if (hold <= 0.0f){
        hold = 4.0f;        
    }

    int t = int(timer) % 3;
    switch(t){
        case 0:
            subway.turn = 0;
            break;
        case 1:
            subway.turn = 1;
            break;
        case 2:
            subway.turn = 2;
            break;
        default:
            break;
    }

}

void InitSubway(Subway& subway, int num_cars){
    if (!subway.models_loaded){
        subway.head_model = LoadModel("../assets/head.obj");
        subway.head_texture = LoadTexture("../assets/head.png");
        subway.car_model = LoadModel("../assets/car.obj");
        subway.car_texture = LoadTexture("../assets/car.png");
        subway.head_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = subway.head_texture;
        subway.car_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = subway.car_texture;
    }

    subway.head = (Vector3){0.0f, 0.0f, 0.0f};
    subway.path_history.push_back(subway.head);
    subway.head_vel = (Vector3){0.0f, 0.0f, 0.0f};
    subway.grid_pos = (GridPosition){0, 0};
    subway.last_path_index = 0;
    for(int i = 0; i < num_cars; i++){
        AddCar(subway);
    }
}



void UpdateSubway(Subway& subway) {
    float d = subway.head.z * -1;
    subway.distance = d;

    subway.last_pos = subway.head;
    subway.head = Vector3Add(subway.head, Vector3Scale(subway.head_vel, dt));
    //Vector3 previous_position = subway.last_pos;
    float minimum_distance = 0.8f;

    Vector3 h_dir = Vector3Subtract(subway.head, subway.last_pos);
    float h_dist = Vector3Length(h_dir);
    Vector3 h_norm_dir = Vector3Normalize(h_dir);
    subway.head_rotation = atan2f(h_dir.x, h_dir.z) * RAD2DEG;

    for (int i = 0; i < subway.trailing_cars.size(); i++) {
        Vector3 previous_position = subway.path_history[i];
        Vector3 direction = Vector3Subtract(previous_position, subway.trailing_cars[i].position);
        float distance = Vector3Length(direction);
        if (distance > minimum_distance) {
            Vector3 normalized_direction = Vector3Normalize(direction);
            subway.trailing_cars[i].position = Vector3Add(subway.trailing_cars[i].position, 
                Vector3Scale(normalized_direction, ((distance - minimum_distance)) * dt));
        }
        subway.trailing_cars[i].rotation = atan2f(direction.x, direction.z) * RAD2DEG;
    }
  
}


void FollowPath(Subway& subway, const std::vector<StagePiece>& pieces) {
    if (pieces.empty()) return;

    const auto& current_piece = pieces[subway.current_stage_piece_index];
    const auto& prev_piece = pieces[subway.current_stage_piece_index - 1];
    const auto& path = current_piece.path;
    if (path.empty()) return;

    if (subway.last_path_index == 0) {
        subway.last_path_index = subway.current_path_index;
    }

    
    size_t next_index = (subway.current_path_index + 1) % path.size();
    int path_index = (int)(path[next_index].y * sqrt(current_piece.grid.draw_positions.size()) + path[next_index].x);
    Vector3 target_pos = Vector3Add(
        current_piece.grid.draw_positions[path_index], 
        current_piece.position
    );

    std::cout << path_index << std::endl;

    subway.path_history.clear();

    int trailing_car_idxs[subway.trailing_cars.size()];
    
    std::cout << subway.current_path_index << std::endl;
    if (subway.current_path_index <= (path.size() - 10) || subway.current_path_index >= 40){
        for (int i = 0; i < subway.trailing_cars.size(); i++) {
            int spacing = 5;
            size_t last_idx = (subway.current_path_index - (i * spacing)) % path.size();
            subway.last_idxs[i] = last_idx;
            std::cout << i << " | " << last_idx << std::endl;
            int last_path_index = (int)(path[last_idx].y * sqrt(current_piece.grid.draw_positions.size()) + path[last_idx].x);
            trailing_car_idxs[i] = last_path_index;
        }

        for (int i = 0; i < subway.trailing_cars.size(); i++) {
            Vector3 old_target = Vector3Add(current_piece.grid.draw_positions[trailing_car_idxs[i]], current_piece.position);
            subway.path_history.push_back(old_target);
        }
    } else { // crossing into new grid, just advance previous path
        for (int i = 0; i < subway.trailing_cars.size(); i++) {
            int last_path_index = (int)(path[subway.last_idxs[i]].y * sqrt(prev_piece.grid.draw_positions.size()) + path[subway.last_idxs[i]].x);
            trailing_car_idxs[i] = last_path_index;
        }

        for (int i = 0; i < subway.trailing_cars.size(); i++) {
            Vector3 old_target = Vector3Add(prev_piece.grid.draw_positions[trailing_car_idxs[i]], prev_piece.position);
            subway.path_history.push_back(old_target);
        }
    }
    
    // // situation if subway is crossing grid paths
    // if (Vector3Distance(subway.path_history.back(), target_pos) > 2.5f) {
    //     subway.path_history.push_back(target_pos);
    // }
    // if (subway.path_history.size() > subway.trailing_cars.size()){
    //     subway.path_history.erase(subway.path_history.begin());
    // }
    

    Vector3 direction = Vector3Subtract(target_pos, subway.head);
    Vector3 normalized_dir = Vector3Normalize(direction);
    
    float speed = 8.0f;
    if (subway.distance < 19.0f){
        speed = 20.0f;
    }
    
    subway.head_vel = Vector3Scale(normalized_dir, speed);
    
    subway.head_rotation = atan2f(direction.x, direction.z) * RAD2DEG;
    
    if (Vector3Distance(subway.head, target_pos) < 0.5f) {
        subway.current_path_index = next_index;
    }

}
