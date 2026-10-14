#include <subway.hpp>
#include <stage.hpp>
#include <terrain.hpp>

static float subway_size = 2.0f;
static float dt = 1.0f / 60.0f;

// models face +z, tip them about their own x axis to follow the grade
static void DrawCar(Model model, Vector3 position, float rotation, float pitch, float scale) {
    model.transform = MatrixRotateX(-pitch * DEG2RAD);
    DrawModelEx(model, position, (Vector3){0.0f, 1.0f, 0.0f}, rotation, (Vector3){scale, scale, scale}, WHITE);
}

void DrawSubway(Subway& subway, bool debug){
    // same sun, shadows and fog as the terrain
    subway.head_model.materials[0].shader = GetTerrainShader();
    subway.car_model.materials[0].shader = GetTerrainShader();
    DrawCar(subway.head_model, subway.head, subway.head_rotation, subway.head_pitch, subway.car_scale);
    //DrawModel(subway.head_model, subway.head, 1.0f, WHITE);
    for(int i = 0; i < subway.trailing_cars.size(); i++){
        if (debug) DrawCubeWires(subway.trailing_cars[i].position, subway_size, subway_size, subway_size, BLUE);
        const SubwayCar& car = subway.trailing_cars[i];
        DrawCar(subway.car_model, car.position, car.rotation, car.pitch, subway.car_scale);
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
        subway.head_model.materials[0].maps[MATERIAL_MAP_METALNESS].texture = GetShadowMap();
        subway.car_model.materials[0].maps[MATERIAL_MAP_METALNESS].texture = GetShadowMap();
        subway.models_loaded = true;
    }

    subway.head = (Vector3){0.0f, 0.0f, 0.0f};
    subway.head_vel = (Vector3){0.0f, 0.0f, 0.0f};
    subway.grid_pos = (GridPosition){0, 0};
    for(int i = 0; i < num_cars; i++){
        AddCar(subway);
    }
}



// Every car rides on two bogies a fixed distance apart on the track. The car sits between
// them and points from the back one to the front one, so it stays on the rails through
// curves and tilts with the grade
static void PlaceOnTrack(const std::vector<StagePiece>& pieces, float s, Vector3& position, float& rotation, float& pitch) {
    const float bogie_offset = 1.8f;
    Vector3 front = TrackPosition(pieces, s + bogie_offset);
    Vector3 back = TrackPosition(pieces, s - bogie_offset);
    Vector3 dir = Vector3Subtract(front, back);
    position = Vector3Lerp(back, front, 0.5f);
    rotation = atan2f(dir.x, dir.z) * RAD2DEG;
    pitch = atan2f(dir.y, sqrtf(dir.x * dir.x + dir.z * dir.z)) * RAD2DEG;
}

void UpdateSubway(Subway& subway, const std::vector<StagePiece>& pieces, float dt) {
    const float acceleration = 1.5f;
    subway.speed = fminf(subway.speed + acceleration * dt, subway.max_speed);
    subway.head_s += subway.speed * dt;
    subway.distance += subway.speed * dt;

    // the whole train moves as one: each car is a fixed distance behind the one in front
    PlaceOnTrack(pieces, subway.head_s, subway.head, subway.head_rotation, subway.head_pitch);
    for (size_t i = 0; i < subway.trailing_cars.size(); i++) {
        SubwayCar& car = subway.trailing_cars[i];
        PlaceOnTrack(pieces, subway.head_s - (i + 1) * subway.car_spacing, car.position, car.rotation, car.pitch);
    }
}
