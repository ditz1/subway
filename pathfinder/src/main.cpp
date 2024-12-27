#include <shared.hpp>
#include <astar.hpp>


void InitGrid(Grid& grid, int slices, float spacing){
   for (int i = -slices/2; i <= slices/2; i++){
       for (int j = -slices/2; j <= slices/2; j++){
           grid.draw_positions.push_back((Vector3){i*spacing, 0.0f, j*spacing});
           grid.cells.push_back((Cell){(Vector2){(float)(i+(slices/2)), (float)(j+(slices/2))}, 0.0f, RED});
       }
   }
}

Vector3 GridToWorld(Grid& grid, int p[2]) {
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

void DrawGridCubes(Grid& grid){
    Color debug_color = Color{235, 235, 235, 40};
    for (int i = 0; i < grid.draw_positions.size(); i++){
        DrawCube(grid.draw_positions[i], grid.size, grid.size, grid.size, grid.cells[i].color);
        DrawCubeWires(grid.draw_positions[i], grid.size, grid.size, grid.size, debug_color);
       
    }
}


void CameraControls(Camera* camera){
   if (IsKeyDown(KEY_W)) camera->position.z -= 0.1f;
   if (IsKeyDown(KEY_S)) camera->position.z += 0.1f;
   if (IsKeyDown(KEY_A)) camera->position.x -= 0.1f;
   if (IsKeyDown(KEY_D)) camera->position.x += 0.1f;
   
   if (IsKeyDown(KEY_Q)) camera->position.y -= 0.1f;
   if (IsKeyDown(KEY_E)) camera->position.y += 0.1f;
   

   if (IsKeyDown(KEY_UP)) camera->target.z += 0.1f;
   if (IsKeyDown(KEY_DOWN)) camera->target.z -= 0.1f;
   if (IsKeyDown(KEY_LEFT)) camera->target.x -= 0.1f;
   if (IsKeyDown(KEY_RIGHT)) camera->target.x += 0.1f;
}

int main(void) {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "raylib [shaders] example - basic lighting");
    Camera camera = {0};
    camera.position = (Vector3){2.0f, 7.0f, 10.0f};
    camera.target = (Vector3){0.0f, 0.5f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    SetTargetFPS(60);   
    int grid_size = 40;
    float cube_size = 0.25f;
    float scale = 0.08f; 
    Grid grid;
    grid.size = cube_size;
    InitGrid(grid, grid_size, cube_size);
    RandomizePermutation(grid);
    GenerateNoiseEx(grid, GREEN, BROWN, BLUE);  
    int start[2] = {0, grid_size / 2};
    int end[2] = {grid_size, grid_size / 2};    
    std::vector<Vector2> current_path;
    RandomizePermutation(grid);
    current_path = PathFinder::FindPath(grid, start, end);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_G)) { scale += 0.01f; GenerateNoiseEx(grid, GREEN, BROWN, BLUE, scale); }
        if (IsKeyPressed(KEY_H)) { scale -= 0.01f; GenerateNoiseEx(grid, GREEN, BROWN, BLUE, scale); }
        if (IsKeyPressed(KEY_R)) {
            RandomizePermutation(grid);
            GenerateNoiseEx(grid, GREEN, BROWN, BLUE, scale);
            current_path = PathFinder::FindPath(grid, start, end);
        }

        CameraControls(&camera);

        BeginDrawing();
        ClearBackground(GRAY);
            BeginMode3D(camera);
                DrawGridCubes(grid);
                //DrawGrid(10, cube_size);
                DrawCube(GridToWorld(grid, start), cube_size, cube_size, cube_size, GREEN);
                DrawCube(GridToWorld(grid, end), cube_size, cube_size, cube_size, RED);
                DrawPathMarkers(grid, current_path);
            EndMode3D();
            DrawFPS(10, 10);
            DrawText(TextFormat("Scale: %.2f", scale), 10, 30, 20, WHITE);
        EndDrawing();
        }

    CloseWindow();
    return 0;
}