#include <globals.hpp>
#include <debug_drawing.hpp>
#include <stage.hpp>
#include <subway.hpp>

Vector3 Lerp(Vector3 start, Vector3 end, float amount=0.5f){
    Vector3 result = Vector3Add(start, (Vector3Scale((Vector3Subtract(end, start)), amount)));
    return result;
}

void PollCameraControls(Camera3D& cam){
    if (IsKeyDown(KEY_UP)) cam.position.y += 0.1f;
    if (IsKeyDown(KEY_DOWN)) cam.position.y -= 0.1f;
    if (IsKeyDown(KEY_LEFT)) cam.position.x -= 0.1f;
    if (IsKeyDown(KEY_RIGHT)) cam.position.x += 0.1f;
    if (IsKeyDown(KEY_COMMA)) cam.position.z += 0.1f;
    if (IsKeyDown(KEY_PERIOD)) cam.position.z -= 0.1f;

    if (IsKeyPressed(KEY_SLASH)){
        cam.position = (Vector3){ 19.00f, 17.70f, 7.50f };
        cam.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    }
}

void PollSubwayControls(Subway& subway) {
    if (IsKeyDown(KEY_W) && subway.head_vel.z > -3.0f) {
        subway.head_vel.z -= 0.1f;
        return;
    } 
    if (IsKeyDown(KEY_S)) {
        subway.head_vel.z += 0.1f;
        return;
    }
    if (IsKeyDown(KEY_A) && subway.head_vel.x > -2.0f) {
        subway.head_vel.x -= 0.05f;
        return;
    }
    if (IsKeyDown(KEY_D) && subway.head_vel.x < 2.0f) {
        subway.head_vel.x += 0.05f;
        return;
    }
    if (IsKeyUp(KEY_W) && IsKeyUp(KEY_S) && IsKeyUp(KEY_A) && IsKeyUp(KEY_D)){
        subway.head_vel = (Vector3){0.0f, 0.0f, 0.0f};
    }
}




void UpdateCamera(Camera3D& cam, Vector3 subway_head, Vector3 offset) {
    Vector3 new_pos = Vector3Add(subway_head, offset);
    cam.position = Lerp(cam.position, new_pos, 0.1f);
    cam.target = Lerp(cam.target, subway_head, 0.1f);
}


int main() {
    const int screenWidth = 800;
    const int screenHeight = 450;
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

    SetTargetFPS(60);


    Camera3D camera = { 0 };
    camera.fovy = 60.0f;
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.position = (Vector3){ 4.0f, 4.0f, 4.0f };
    camera.projection = CAMERA_PERSPECTIVE;

    Vector3 cam_start = {19.00f, 17.70f, 7.50f};
    Subway subway;
    Vector3 subway_head = {0.0f, 0.0f, 0.0f};
    subway.head = subway_head;
    camera.position = cam_start;
    Vector3 offset = Vector3Subtract(camera.position, subway.head); 

    
    std::vector<StagePiece> stage_pieces;
    StagePiece initial_piece;
    initial_piece.position = (Vector3){ 0.0f, 0.0f, 0.0f };
    initial_piece.id = 0;
    initial_piece.detection_line = initial_piece.position.z;
    stage_pieces.push_back(initial_piece);

    int subway_length = 5;

    InitSubway(subway, subway_length);

    float move_timer = 0.0f;
    float hold_timer = 0.0f;
    float dt = 1.0f / 60.0f;

    while (!WindowShouldClose()) {

        AutoMoveSubway(subway, move_timer, hold_timer, dt);
        //PollSubwayControls(subway); // debug
        PollCameraControls(camera);
        //std::cout << hold_timer << std::endl;

        UpdateCamera(camera, subway.head, offset);
        UpdateStagePieces(stage_pieces, subway.head);
        UpdateSubway(subway);

        BeginDrawing();
            BeginMode3D(camera);
            ClearBackground(BLUE);
                DrawGrid(32, 2.0f);
                DrawSubway(subway);
                DrawStagePieces(stage_pieces);
            EndMode3D();
            DrawDebug(camera, subway, stage_pieces);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}