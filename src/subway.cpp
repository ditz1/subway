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

void AutoMoveSubway(Subway& subway, float& timer, float& hold, float dt){
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
    //std::cout << subway.turn << std::endl;
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
    subway.head_vel = (Vector3){0.0f, 0.0f, 0.0f};
    subway.grid_pos = (GridPosition){0, 0};
    for(int i = 0; i < num_cars; i++){
        AddCar(subway);
    }
}

void UpdateSubway(Subway& subway) {
    subway.last_pos = subway.head;
    subway.head = Vector3Add(subway.head, Vector3Scale(subway.head_vel, dt));
    Vector3 previous_position = subway.last_pos;
    float minimum_distance = 2.0f;

    Vector3 h_dir = Vector3Subtract(subway.head, subway.last_pos);
    float h_dist = Vector3Length(h_dir);
    Vector3 h_norm_dir = Vector3Normalize(h_dir);
    subway.head_rotation = atan2f(h_dir.x, h_dir.z) * RAD2DEG;

    for (int i = 0; i < subway.trailing_cars.size(); i++) {
        Vector3 direction = Vector3Subtract(previous_position, subway.trailing_cars[i].position);
        float distance = Vector3Length(direction);
        if (distance > minimum_distance) {
            Vector3 normalized_direction = Vector3Normalize(direction);
            subway.trailing_cars[i].position = Vector3Add(subway.trailing_cars[i].position, 
                Vector3Scale(normalized_direction, ((distance - minimum_distance)) * dt));
        }
        
        subway.trailing_cars[i].rotation = atan2f(direction.x, direction.z) * RAD2DEG;
        previous_position = subway.trailing_cars[i].position;
    }
}


