#include <stage.hpp>


void InitStagePiece(StagePiece& curr, StagePiece prev) {
    curr.position = prev.position;
    curr.position.z -= prev.height;
    curr.width = piece_width;
    curr.height = piece_height;
    curr.detection_line = curr.position.z + curr.height / 4.0f;
    curr.detect_line_crossed = false;
    curr.is_active = true;
    curr.id = prev.id + 1;
    
    // Initialize grid for this piece
    curr.grid.size = 0.25f;
    InitGrid(curr.grid, 40, curr.grid.size);
    RandomizePermutation(curr.grid);
    GenerateNoiseEx(curr.grid, GREEN, BROWN, BLUE, 0.08f);
    
    // Generate path
    int start[2] = {0, 20};
    int end[2] = {40, 20};
    curr.path = PathFinder::FindPath(curr.grid, start, end);
}

void DrawPathMarkers(const Grid& grid, const std::vector<Vector2>& path) {
    if (path.empty()) return;
    Color debug_path = YELLOW;
    debug_path.a = 125;
    for (size_t i = 0; i < path.size(); i++) {
        int index = (int)(path[i].y * sqrt(grid.draw_positions.size()) + path[i].x);
        Vector3 pos = grid.draw_positions[index];
        DrawCube(pos, grid.size * 1.2f, grid.size * 2.0f, grid.size * 1.2f, debug_path);
    }
}


void DrawStagePiece(StagePiece& piece) {
    DrawPlane(piece.position, (Vector2){(float)piece.width, (float)piece.height}, GREEN);
    // Draw grid cubes with offsete
    float scalar = 4.0f;
    float size = piece.grid.size;
    for (int i = 0; i < piece.grid.draw_positions.size(); i++) {
        Vector3 pos = Vector3Add(piece.grid.draw_positions[i], piece.position);
        DrawCube(pos, size * scalar, size * 1.5f, size * scalar, piece.grid.cells[i].color);
        DrawCubeWires(pos, size * scalar, size * 1.5f, size * scalar, GRAY);
    }
    
    // Draw path markers with offset
    if (!piece.path.empty()) {
        Color debug_path = YELLOW;
        debug_path.a = 125;
        for (size_t i = 0; i < piece.path.size(); i++) {
            int index = (int)(piece.path[i].y * sqrt(piece.grid.draw_positions.size()) + piece.path[i].x);
            Vector3 pos = Vector3Add(piece.grid.draw_positions[index], piece.position);
            DrawCube(pos, piece.grid.size * 1.2f, piece.grid.size * 2.0f, piece.grid.size * 1.2f, debug_path);
        }
    }
}

void DrawStagePieces(std::vector<StagePiece>& pieces) {
    for (auto& piece : pieces) {
        DrawStagePiece(piece);
    }
}

void DrawGridCubes(Grid& grid){
    Color debug_color = Color{235, 235, 235, 40};
    for (int i = 0; i < grid.draw_positions.size(); i++){
        DrawCube(grid.draw_positions[i], grid.size, grid.size, grid.size, grid.cells[i].color);
        DrawCubeWires(grid.draw_positions[i], grid.size, grid.size, grid.size, debug_color);
       
    }
}

void UpdateStagePieces(std::vector<StagePiece>& pieces, Vector3 subway_head) {
    for (size_t i = 0; i < pieces.size(); ++i) {
        auto& piece = pieces[i];

        if (!piece.detect_line_crossed && abs(subway_head.z) > abs(piece.detection_line)) {
            piece.detect_line_crossed = true;

            if (i == pieces.size() - 1) {
                StagePiece newPiece;
                InitStagePiece(newPiece, piece);
                pieces.push_back(newPiece);
            }
        }
    }
}
