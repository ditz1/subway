#pragma once

#include <raylib.h>
#include <shared.hpp>
#include <queue>
#include <vector>
#include <algorithm>
#include <cmath>

// Lays track along grid rows like minecraft rails: only straight steps, no diagonals. A
// search state is a cell plus the direction of travel and how long the current straight run
// is, so turns can cost extra and only happen once the run is long enough to fit the bend
class PathFinder {
private:
    // cost of stepping onto a neighbouring cell: the track wants gentle grades and dry land
    static float CalculateTerrainCost(float from_height, float to_height) {
        float climb = std::fabs(to_height - from_height);
        float cost = 1.0f + climb * climb * 6.0f;
        if (to_height < SEA_LEVEL) cost += 12.0f; // needs a bridge
        return cost;
    }

public:
    static constexpr int dx[4] = {1, -1, 0, 0};
    static constexpr int dy[4] = {0, 0, 1, -1};

    // start and end are entered and left travelling in direction dir (an index into dx/dy).
    // Returns the cells from start to end inclusive, or nothing if there's no way through
    static std::vector<Vector2> FindPath(Grid& grid, int start[2], int end[2], int dir, int min_run, float turn_cost) {
        int grid_width = (int)sqrt(grid.draw_positions.size());
        auto state_of = [&](int x, int y, int d, int run) { return ((y * grid_width + x) * 4 + d) * min_run + (run - 1); };

        std::vector<float> g(grid_width * grid_width * 4 * min_run, INFINITY);
        std::vector<int> parent(g.size(), -1);
        typedef std::pair<float, int> Entry;
        std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open_set;

        // straight in from the previous stretch of track, so free to turn straight away
        int first = state_of(start[0], start[1], dir, min_run);
        g[first] = 0.0f;
        open_set.push(Entry(0.0f, first));

        while (!open_set.empty()) {
            Entry top = open_set.top();
            open_set.pop();
            int state = top.second;
            int run = state % min_run + 1;
            int d = (state / min_run) % 4;
            int cell = state / min_run / 4;
            int x = cell % grid_width, y = cell / grid_width;
            float heuristic = std::abs(end[0] - x) + std::abs(end[1] - y);
            if (top.first > g[state] + heuristic + 0.001f) continue; // stale entry

            // done once we can carry on out of the piece in the right direction
            if (x == end[0] && y == end[1] && (d == dir || run == min_run)) {
                std::vector<Vector2> path;
                for (int s = state; s != -1; s = parent[s]) {
                    int c = s / min_run / 4;
                    path.push_back((Vector2){(float)(c % grid_width), (float)(c / grid_width)});
                }
                std::reverse(path.begin(), path.end());
                return path;
            }

            for (int nd = 0; nd < 4; nd++) {
                bool turning = nd != d;
                if (turning && (dx[nd] == -dx[d] && dy[nd] == -dy[d])) continue; // no reversing
                if (turning && run < min_run) continue;
                int nx = x + dx[nd], ny = y + dy[nd];
                if (nx < 0 || nx >= grid_width || ny < 0 || ny >= grid_width) continue;

                int next_run = turning ? 1 : std::min(run + 1, min_run);
                int next = state_of(nx, ny, nd, next_run);
                float cost = g[state] + CalculateTerrainCost(grid.cells[cell].height, grid.cells[ny * grid_width + nx].height);
                if (turning) cost += turn_cost;
                if (cost < g[next]) {
                    g[next] = cost;
                    parent[next] = state;
                    open_set.push(Entry(cost + std::abs(end[0] - nx) + std::abs(end[1] - ny), next));
                }
            }
        }
        return std::vector<Vector2>();
    }
};
