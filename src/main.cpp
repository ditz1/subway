#include <globals.hpp>
#include <debug_drawing.hpp>
#include <stage.hpp>
#include <subway.hpp>
#include "raylib.h"
#include "raymath.h"
#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION 330
#else
    #define GLSL_VERSION 100
#endif

Vector3 Lerp(Vector3 start, Vector3 end, float amount = 0.5f) {
    return Vector3Add(start, Vector3Scale(Vector3Subtract(end, start), amount));
}

void PollCameraControls(Camera3D &cam) {
    if (IsKeyDown(KEY_UP)) cam.target.y += 0.1f;
    if (IsKeyDown(KEY_DOWN)) cam.target.y -= 0.1f;
    if (IsKeyDown(KEY_LEFT)) cam.target.x -= 0.1f;
    if (IsKeyDown(KEY_RIGHT)) cam.target.x += 0.1f;
    if (IsKeyDown(KEY_COMMA)) cam.target.z += 0.1f;
    if (IsKeyDown(KEY_PERIOD)) cam.target.z -= 0.1f;

    if (IsKeyPressed(KEY_SLASH)) {
        cam.position = (Vector3){19.00f, 17.70f, 7.50f};
        cam.target = (Vector3){0.0f, 0.0f, 0.0f};
    }
}

void UpdateCamera(Camera3D &cam, Vector3 subway_head, Vector3 offset) {
    cam.position = Lerp(cam.position, Vector3Add(subway_head, offset), 0.1f);
    cam.target = Lerp(cam.target, subway_head, 0.1f);
}

int main() {
    const int screenWidth = 800;
    const int screenHeight = 450;
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(screenWidth, screenHeight, "Subway Simulation with Lighting");
    SetTargetFPS(60);

    Camera3D camera = {0};
    camera.fovy = 60.0f;
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.position = (Vector3){4.0f, 4.0f, 4.0f};
    camera.projection = CAMERA_PERSPECTIVE;

    Vector3 cam_start = {19.00f, 17.70f, 7.50f};
    Subway subway;
    subway.head = (Vector3){0.0f, 0.0f, 0.0f};
    camera.position = cam_start;
    Vector3 offset = Vector3Subtract(camera.position, subway.head);

    std::vector<StagePiece> stage_pieces;
    StagePiece initial_piece;
    initial_piece.position = (Vector3){0.0f, 0.0f, 0.0f};
    initial_piece.id = 0;
    initial_piece.detection_line = initial_piece.position.z;
    stage_pieces.push_back(initial_piece);

    int subway_length = 5;
    InitSubway(subway, subway_length);

    Shader shader = LoadShader(TextFormat("../assets/shaders/glsl%i/lighting.vs", GLSL_VERSION),
                               TextFormat("../assets/shaders/glsl%i/lighting.fs", GLSL_VERSION));
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

    int ambientLoc = GetShaderLocation(shader, "ambient");
    SetShaderValue(shader, ambientLoc, (float[4]){0.3f, 0.3f, 0.3f, 1.0f}, SHADER_UNIFORM_VEC3);

    Light subwayLight = CreateLight(LIGHT_POINT, Vector3Add(subway.head, (Vector3){-12.0f, 0.0f, 0.0f}), Vector3Zero(), WHITE, shader);

    float move_timer = 0.0f;
    float hold_timer = 0.0f;
    float dt = 1.0f / 60.0f;

    while (!WindowShouldClose()) {
        AutoMoveSubway(subway, move_timer, hold_timer, dt);
        PollCameraControls(camera);
        UpdateCamera(camera, subway.head, offset);
        UpdateStagePieces(stage_pieces, subway.head);
        UpdateSubway(subway);

        subwayLight.position = Vector3Add(subway.head, (Vector3){7.0f, 2.0f, 8.0f});

        float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};
        SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        UpdateLightValues(shader, subwayLight);

        BeginDrawing();
            ClearBackground(BLUE);
            BeginMode3D(camera);
                DrawGrid(32, 2.0f);
                BeginShaderMode(shader);
                    DrawSubway(subway);
                EndShaderMode();
                DrawStagePieces(stage_pieces);

                //DrawSphere(subwayLight.position, 2.0f, subwayLight.color);

            EndMode3D();
            DrawDebug(camera, subway, stage_pieces);
        EndDrawing();
    }

    UnloadShader(shader);
    CloseWindow();

    return 0;
}
