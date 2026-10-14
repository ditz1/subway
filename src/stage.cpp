#include <stage.hpp>
#include <terrain.hpp>


void InitStagePiece(std::vector<StagePiece>& pieces, StagePiece& curr, StagePiece prev){
    curr.position = prev.position;
    curr.position.z -= prev.height;
    curr.width = piece_width;
    curr.height = piece_height;
    curr.detection_line = curr.position.z + curr.height / 4.0f;
    curr.detect_line_crossed = false;
    curr.is_active = true;
    curr.id = prev.id + 1;
    
    // Initialize grid for this piece
    curr.grid.size = 1.00f;
    InitGrid(curr.grid, 64, curr.grid.size);

    // Generate terrain, path and meshes
    BuildStagePiece(curr, false);
}

float TrackLength(const std::vector<StagePiece>& pieces) {
    float length = 0.0f;
    for (const auto& piece : pieces) length += piece.track_length;
    return length;
}

Vector3 TrackPosition(const std::vector<StagePiece>& pieces, float s) {
    s = fmaxf(s, 0.0f);
    for (size_t p = 0; p < pieces.size(); p++) {
        const StagePiece& piece = pieces[p];
        if (s > piece.track_length && p + 1 < pieces.size()) {
            s -= piece.track_length;
            continue;
        }
        s = fminf(s, piece.track_length);
        // first track point past s, then blend from the one before it
        size_t i = std::upper_bound(piece.track_s.begin(), piece.track_s.end(), s) - piece.track_s.begin();
        i = std::min(std::max(i, (size_t)1), piece.track_s.size() - 1);
        float segment = piece.track_s[i] - piece.track_s[i - 1];
        float t = segment > 0.0f ? (s - piece.track_s[i - 1]) / segment : 0.0f;
        return Vector3Lerp(PathPoint(piece, i - 1), PathPoint(piece, i), t);
    }
    return (Vector3){0.0f, 0.0f, 0.0f};
}

void DrawPathMarkers(const StagePiece& piece) {
    if (piece.path.empty()) return;
    Color debug_path = YELLOW;
    debug_path.a = 125;
    for (size_t i = 0; i < piece.track.size(); i++) {
        DrawCube(PathPoint(piece, i), piece.grid.size * 1.2f, piece.grid.size * 1.2f, piece.grid.size * 1.2f, debug_path);
    }
}

void DrawStagePiece(StagePiece& piece) {
    if (!piece.models_loaded) return;
    piece.terrain.materials[0].shader = GetTerrainShader();
    DrawModel(piece.terrain, piece.position, 1.0f, WHITE);
}

void DrawStagePieces(std::vector<StagePiece>& pieces) {
    for (auto& piece : pieces) {
        DrawStagePiece(piece);
    }
}

// water is see-through so it has to go down after everything solid
void DrawStageWater(std::vector<StagePiece>& pieces) {
    for (auto& piece : pieces) {
        if (piece.has_water) DrawModel(piece.water, piece.position, 1.0f, WHITE);
    }
}

// void UpdateStagePieces(std::vector<StagePiece>& pieces, Vector3 subway_head, Subway& subway) {
//     for (size_t i = 0; i < pieces.size(); ++i) {
//         auto& piece = pieces[i];

//         if (!piece.detect_line_crossed && abs(subway_head.z) > abs(piece.detection_line)) {
//             piece.detect_line_crossed = true;

//             if (i == pieces.size() - 1) {
//                 StagePiece newPiece;
//                 InitStagePiece(newPiece, piece);
//                 pieces.push_back(newPiece);
//             }
//         }
//     }
//     if (pieces.size() > 3) {
//         pieces.erase(pieces.begin());
//     }
// }
