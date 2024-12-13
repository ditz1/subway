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
}


void DrawStagePiece(StagePiece& piece) {
    DrawPlane(piece.position, (Vector2){(float)piece.width, (float)piece.height}, GREEN);
    //DrawCube(Vector3{piece.position.x, piece.position.y, piece.detection_line}, 1.0f, 1.0f, 1.0f, YELLOW);
}

void DrawStagePieces(std::vector<StagePiece>& pieces) {
    for (auto& piece : pieces) {
        DrawStagePiece(piece);
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
