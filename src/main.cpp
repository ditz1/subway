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

#define MAX_CUBES 10

static Vector3 cam_original_pos = {25.00f, 27.70f, 7.50f};

Vector3 Lerp(Vector3 start, Vector3 end, float amount = 0.5f) {
    return Vector3Add(start, Vector3Scale(Vector3Subtract(end, start), amount));
}

void InitGrid(Grid& grid, int slices, float spacing){
   for (int i = -slices/2; i <= slices/2; i++){
       for (int j = -slices/2; j <= slices/2; j++){
           grid.draw_positions.push_back((Vector3){i*spacing, 0.0f, j*spacing});
           grid.cells.push_back((Cell){(Vector2){(float)(i+(slices/2)), (float)(j+(slices/2))}, 0.0f, RED});
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

void RandomizePermutation(Grid& grid) {
   for(int i = 0; i < 256; i++) {
       grid.permutation[i] = i;
   }
   
   for(int i = 255; i > 0; i--) {
       int j = GetRandomValue(0, i);
       unsigned char temp = grid.permutation[i];
       grid.permutation[i] = grid.permutation[j];
       grid.permutation[j] = temp;
   }
}

float perlin2d(const Grid& grid, float x, float y) {
   auto fade = [](float t) { return t * t * t * (t * (t * 6 - 15) + 10); };
   auto lerp = [](float t, float a, float b) { return a + t * (b - a); };
   auto grad = [](int hash, float x, float y) {
       int h = hash & 15;
       float u = h < 8 ? x : y;
       float v = h < 4 ? y : h == 12 || h == 14 ? x : 0;
       return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
   };

   int X = (int)floor(x) & 255;
   int Y = (int)floor(y) & 255;

   x -= floor(x);
   y -= floor(y);

   float u = fade(x);
   float v = fade(y);

   int A = grid.permutation[X] + Y;
   int B = grid.permutation[X + 1] + Y;

   return lerp(v, 
       lerp(u, grad(grid.permutation[A], x, y), grad(grid.permutation[B], x - 1, y)),
       lerp(u, grad(grid.permutation[A + 1], x, y - 1), grad(grid.permutation[B + 1], x - 1, y - 1))
   );
}

void GenerateNoise(Grid& grid) {
   float scale = 0.2f;
   
   for (auto& cell : grid.cells) {
       float nx = cell.grid_pos.x * scale;
       float ny = cell.grid_pos.y * scale;
       cell.perlin_value = perlin2d(grid, nx, ny);
       
       unsigned char c = (unsigned char)((cell.perlin_value + 1) * 127.5f);
       cell.color = (Color){c, 0, 255, 255};
   }
}

Color LerpColor(Color a, Color b, float t) {
  return (Color){
      (unsigned char)((1 - t) * a.r + t * b.r),
      (unsigned char)((1 - t) * a.g + t * b.g),
      (unsigned char)((1 - t) * a.b + t * b.b),
      (unsigned char)((1 - t) * a.a + t * b.a)
  };
}

void UpdateSubwayPath(Subway& subway, const std::vector<StagePiece>& pieces) {
    if (pieces.empty()) {
        return;
    }

    // Get current piece bounds
    const StagePiece& current = pieces[subway.current_stage_piece_index];
    Vector3 piece_end = current.position;
    piece_end.z -= (piece_width / 2.0f) - 5.0f;
    Vector3 piece_start = piece_end;
    piece_start.z += piece_width - 5.0f;

    // Check if subway has moved past current piece
    if (abs(subway.head.z) >= abs(piece_end.z)){
        // Only advance if we have a next piece available
            if (pieces.size() == 2){
                subway.current_stage_piece_index+=1;
            } else if (pieces.size() == 3){
                if (subway.current_stage_piece_index == 2){
                    subway.current_stage_piece_index--;
                } else if (subway.current_stage_piece_index == 1){
                    subway.current_stage_piece_index+=1;
                }
            }
            subway.current_path_index = 0;
    }
}

void UpdateStagePieces(std::vector<StagePiece>& pieces, Vector3 subway_head, Subway& subway) {
    if (pieces.empty()) return;

    // Check the last piece for crossing
    StagePiece& last_piece = pieces.back();
    if (!last_piece.detect_line_crossed && 
        abs(subway_head.z) > abs(last_piece.detection_line)) {
        
        last_piece.detect_line_crossed = true;
        
        // Create new piece
        StagePiece new_piece;
        InitStagePiece(pieces, new_piece, last_piece);
        pieces.push_back(new_piece);
    }

    // Remove oldest piece if we have too many
    if (pieces.size() > 3) {
        pieces.erase(pieces.begin());
        if (subway.current_stage_piece_index == 2) {
            subway.current_stage_piece_index--;
        }
    }
}

void GenerateNoiseEx(Grid& grid, Color high, Color mid, Color low, float scale=0.08f) {
  for (auto& cell : grid.cells) {
      float nx = cell.grid_pos.x * scale;
      float ny = cell.grid_pos.y * scale;
      cell.perlin_value = perlin2d(grid, nx, ny);
      
      float t = (cell.perlin_value + 1.0f) * 0.5f;
      
      if (t < 0.5f) {
          float factor = t * 2.0f;
          cell.color = LerpColor(low, mid, factor);
      } else {
          float factor = (t - 0.5f) * 2.0f;
          cell.color = LerpColor(mid, high, factor);
      }
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
    RandomizePermutation(initial_piece.grid);
    GenerateNoiseEx(initial_piece.grid, GREEN, BROWN, BLUE, 0.08f);

    // initial piece wont really work anyways but we just need to get it started
    int start[2] = {64, 32};
    int end[2] = {0, 32};
    initial_piece.path = PathFinder::FindPath(initial_piece.grid, start, end);

    stage_pieces.push_back(initial_piece);

    int subway_length = 5;
    InitSubway(subway, subway_length);


    float move_timer = 0.0f;
    float hold_timer = 0.0f;
    float dt = 1.0f / 60.0f;

    for (int i = 0; i < initial_piece.grid.cells.size(); i++) {
        std::cout << initial_piece.grid.cells[i].perlin_value << std::endl;
    }


    while (!WindowShouldClose()) {
        UpdateSubwayPath(subway, stage_pieces);
        FollowPath(subway, stage_pieces);
        //AutoMoveSubwayLite(subway, move_timer, hold_timer, dt);
        PollCameraControls(camera);
        UpdateCamera(camera, subway.head, offset);
        UpdateStagePieces(stage_pieces, subway.head, subway);
        UpdateSubway(subway);
        Vector3 x = stage_pieces[subway.current_stage_piece_index].position;
        x.z -= (stage_pieces[0].width / 2.0f) - 5.0f;
        Vector3 z = x;
        z.z += stage_pieces[0].width - 5.0f;
    

        BeginDrawing();
            ClearBackground(BLUE);
            BeginMode3D(camera);
                DrawLine3D(subway.head, subway.last_pos, RED);
                //DrawGrid(40, 1.2f);
                DrawSubway(subway);
                DrawStagePieces(stage_pieces);
                //DrawStagePieces(stage_pieces);
                for (int i = 0; i < subway.path_history.size(); i++) {
                    Color a = (Color){255, 0, 0, 255};
                    a.r -= i * 50;
                    DrawSphere(subway.path_history[i], 1.5f, a);
                }
                DrawSphere(x, 2.5f, RED);
                DrawSphere(z, 2.5f, GREEN);
                //printf("%f %f %f\n", x.x, x.y, x.z);
            EndMode3D();
            DrawDebug(camera, subway, stage_pieces);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
