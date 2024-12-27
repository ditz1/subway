#pragma once

#include <raylib.h>
#include <shared.hpp>
#include <queue>
#include <unordered_map>
#include <cmath>

struct Node {
    int x, y;
    float f_cost;
    float g_cost;
    float h_cost;
    Node* parent;

    Node(int x, int y) : x(x), y(y), f_cost(0), g_cost(0), h_cost(0), parent(nullptr) {}
};

struct CompareNode {
    bool operator()(Node* a, Node* b) {
        return a->f_cost > b->f_cost;
    }
};

class PathFinder {
private:
    static float CalculateTerrainCost(float perlin_value) {
        std::cout << perlin_value << " | ";
        if (perlin_value > 0.45f) return 1.0f; // cost for high, should be easy
        if (perlin_value > 0.25f) return 50.0f; // cost mid, less prio
        if (perlin_value > 0.08f) return 1000.0f; 
        if (perlin_value > 0.01f) return 10000.0f; // further adjustments
        return 100000.0f; // cost low, near impossible
    }

    static float CalculateHeuristic(int x1, int y1, int x2, int y2) {
        return std::sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
    }

public:
    static std::vector<Vector2> FindPath(Grid& grid, int start[2], int end[2]) {
        int grid_width = (int)sqrt(grid.draw_positions.size());
        std::priority_queue<Node*, std::vector<Node*>, CompareNode> open_set;
        std::unordered_map<int, Node*> nodes;
        std::vector<bool> closed_set(grid_width * grid_width, false);

        Node* start_node = new Node(start[0], start[1]);
        open_set.push(start_node);
        nodes[start[1] * grid_width + start[0]] = start_node;

        int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
        int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

        while (!open_set.empty()) {
            Node* current = open_set.top();
            open_set.pop();

            if (current->x == end[0] && current->y == end[1]) {
                std::vector<Vector2> path;
                while (current != nullptr) {
                    path.push_back({(float)current->x, (float)current->y});
                    current = current->parent;
                }
                for (auto& pair : nodes) {
                    delete pair.second;
                }
                std::reverse(path.begin(), path.end());
                return path;
            }

            int current_index = current->y * grid_width + current->x;
            closed_set[current_index] = true;

            for (int i = 0; i < 8; i++) {
                int new_x = current->x + dx[i];
                int new_y = current->y + dy[i];
                int new_index = new_y * grid_width + new_x;

                if (new_x < 0 || new_x >= grid_width || new_y < 0 || new_y >= grid_width ||
                    closed_set[new_index]) {
                    continue;
                }

                float terrain_cost = CalculateTerrainCost(grid.cells[new_index].perlin_value);
                float movement_cost = (dx[i] != 0 && dy[i] != 0) ? 1.4f : 1.0f;
                float new_g_cost = current->g_cost + terrain_cost * movement_cost;

                Node* neighbor;
                if (nodes.find(new_index) == nodes.end()) {
                    neighbor = new Node(new_x, new_y);
                    nodes[new_index] = neighbor;
                } else {
                    neighbor = nodes[new_index];
                }

                if (new_g_cost < neighbor->g_cost || neighbor->g_cost == 0) {
                    neighbor->parent = current;
                    neighbor->g_cost = new_g_cost;
                    neighbor->h_cost = CalculateHeuristic(new_x, new_y, end[0], end[1]);
                    neighbor->f_cost = neighbor->g_cost + neighbor->h_cost;
                    open_set.push(neighbor);
                }
            }
        }

        for (auto& pair : nodes) {
            delete pair.second;
        }
        return std::vector<Vector2>();
    }
};

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
