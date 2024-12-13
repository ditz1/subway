#pragma once

#include <globals.hpp>
#include <subway.hpp>   

void DrawDebug(Camera3D camera, Subway subway, std::vector<StagePiece>& pieces){
    DrawText(TextFormat("Camera position: (%03.2f, %03.2f, %03.2f)", camera.position.x, camera.position.y, camera.position.z), 600, 10, 10, RAYWHITE);
    DrawText(TextFormat("Subway head position: (%03.2f, %03.2f, %03.2f)", subway.head.x, subway.head.y, subway.head.z), 600, 30, 10, RAYWHITE);
    DrawText(TextFormat("Num stage pieces: %d", pieces.size()), 600, 50, 10, RAYWHITE);
    DrawText(TextFormat("Subway Length: %d", subway.trailing_cars.size()), 600, 70, 10, RAYWHITE);
    DrawText(TextFormat("Subway Distance: %03.2f", subway.distance), 20, 20, 15, RAYWHITE);
}