#include <globals.hpp>
#include <stage.hpp>
#include <subway.hpp>
#include <terrain.hpp>
#include <cstdlib>
#include "raylib.h"
#include "raymath.h"

static Vector3 cam_original_pos = {25.00f, 27.70f, 7.50f};

Vector3 Lerp(Vector3 start, Vector3 end, float amount = 0.5f) {
    return Vector3Add(start, Vector3Scale(Vector3Subtract(end, start), amount));
}

void InitGrid(Grid& grid, int slices, float spacing){
   for (int i = -slices/2; i <= slices/2; i++){
       for (int j = -slices/2; j <= slices/2; j++){
           grid.draw_positions.push_back((Vector3){i*spacing, 0.0f, j*spacing});
           grid.cells.push_back((Cell){(Vector2){(float)(i+(slices/2)), (float)(j+(slices/2))}, 0.0f});
       }
   }
}



Vector3 GridToWorld(Grid grid, int p[2]) {
   int grid_width = (int)sqrt(grid.draw_positions.size());
   int index = p[1] * grid_width + p[0];
   if (index < 0 || index >= grid.draw_positions.size()) {
       return (Vector3){0.0f, 0.0f, 0.0f};
   }
   return grid.draw_positions[index];
}

// keep track laid well ahead of the train, and drop pieces once it is well past them
void UpdateStagePieces(std::vector<StagePiece>& pieces, Subway& subway) {
    const float lay_ahead = 100.0f;
    const float keep_behind = 80.0f;

    while (TrackLength(pieces) - subway.head_s < lay_ahead) {
        StagePiece new_piece;
        InitStagePiece(pieces, new_piece, pieces.back());
        pieces.push_back(new_piece);
    }

    while (pieces.size() > 1 && subway.head_s > pieces.front().track_length + keep_behind) {
        // distances are measured from the first piece, so shift the train back with it
        subway.head_s -= pieces.front().track_length;
        UnloadStagePiece(pieces.front());
        pieces.erase(pieces.begin());
    }
}

void PollCameraControls(Camera3D &cam) {
    if (IsKeyDown(KEY_UP)) cam.target.y += 0.1f;
    if (IsKeyDown(KEY_DOWN)) cam.target.y -= 0.1f;
    if (IsKeyDown(KEY_LEFT)) cam.target.x -= 0.1f;
    if (IsKeyDown(KEY_RIGHT)) cam.target.x += 0.1f;
    if (IsKeyDown(KEY_COMMA)) cam.target.z += 0.1f;
    if (IsKeyDown(KEY_PERIOD)) cam.target.z -= 0.1f;
    if (IsKeyDown(KEY_LEFT_BRACKET)) cam.position.x -= 0.1f;
    if (IsKeyDown(KEY_RIGHT_BRACKET)) cam.position.x += 0.1f; 

    if (IsKeyPressed(KEY_SLASH)) {
        cam.position = cam_original_pos;
        cam.target = (Vector3){0.0f, 0.0f, 0.0f};
    }
}

void UpdateCamera(Camera3D &cam, Vector3 subway_head, Vector3 offset) {
    cam.position = Lerp(cam.position, Vector3Add(subway_head, offset), 0.1f);
    cam.target = Lerp(cam.target, subway_head, 0.1f);
}

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 800;
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(screenWidth, screenHeight, "Subway Sim");
    SetTargetFPS(60);

#if !defined(PLATFORM_WEB)
    // assets are found at ../assets from the executable, so work from its folder no matter
    // where the program was started from
    ChangeDirectory(GetApplicationDirectory());
#endif

    // a new world every run, set SUBWAY_SEED to get the same one back
    const char* seed_env = getenv("SUBWAY_SEED");
    int seed = seed_env ? atoi(seed_env) : GetRandomValue(0, 1 << 30);
    InitTerrain(seed);
    std::cout << "terrain seed: " << seed << std::endl;

    Camera3D camera = {0};
    camera.fovy = 60.0f;
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.position = (Vector3){4.0f, 4.0f, 4.0f};
    camera.projection = CAMERA_PERSPECTIVE;

    Vector3 cam_start = cam_original_pos;
    Subway subway;
    subway.head = (Vector3){0.0f, 0.0f, 0.0f};
    camera.position = cam_start;
    Vector3 offset = Vector3Subtract(camera.position, subway.head);

    std::vector<StagePiece> stage_pieces;
    StagePiece initial_piece;
    initial_piece.position = (Vector3){0.0f, 0.0f, 0.0f};
    initial_piece.id = 0;
    initial_piece.width = piece_width;
    initial_piece.height = piece_height;
    initial_piece.detection_line = initial_piece.position.z;
    initial_piece.grid.size = 1.00f;

    InitGrid(initial_piece.grid, 64, initial_piece.grid.size);

    // the train makes a straight run across the first piece to get going, so its track is straight too
    BuildStagePiece(initial_piece, true);

    stage_pieces.push_back(initial_piece);

    int subway_length = 5;
    InitSubway(subway, subway_length);

    // start in the middle of the first piece, with the cars strung out behind
    subway.head_s = initial_piece.track_length / 2.0f;
    UpdateStagePieces(stage_pieces, subway);
    UpdateSubway(subway, stage_pieces, 0.0f);


    bool show_debug = false;
    // SUBWAY_SHOT=<frame> saves a screenshot at that frame and quits
    const char* shot_env = getenv("SUBWAY_SHOT");
    int shot_frame = shot_env ? atoi(shot_env) : 0;
    int frame = 0;

    while (!WindowShouldClose()) {
        // capped so a slow frame (like one that builds a new piece) can't make the train jump
        float dt = fminf(GetFrameTime(), 1.0f / 20.0f);
        UpdateStagePieces(stage_pieces, subway);
        UpdateSubway(subway, stage_pieces, dt);
        PollCameraControls(camera);
        UpdateCamera(camera, subway.head, offset);

        if (IsKeyPressed(KEY_F1)) show_debug = !show_debug;
        UpdateTerrainShaders(camera, (float)GetTime());

        BeginShadowPass(subway.head);
            DrawStagePieces(stage_pieces);
            DrawSubway(subway);
        EndShadowPass();

        BeginDrawing();
            ClearBackground(GetSkyColor());
            BeginMode3D(camera);
                //DrawGrid(40, 1.2f);
                DrawStagePieces(stage_pieces);
                DrawSubway(subway, show_debug);
                if (show_debug) {
                    for (auto& piece : stage_pieces) DrawPathMarkers(piece);
                    DrawSphere(TrackPosition(stage_pieces, subway.head_s), 0.5f, RED);
                }
                DrawStageWater(stage_pieces);
            EndMode3D();
        EndDrawing();

        if (shot_frame > 0 && ++frame == shot_frame) {
            TakeScreenshot("shot.png");
            break;
        }
    }

    for (auto& piece : stage_pieces) UnloadStagePiece(piece);
    UnloadTerrain();
    CloseWindow();

    return 0;
}
