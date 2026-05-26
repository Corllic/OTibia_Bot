#include "PathfindingFunctions.h"
#include <queue>
#include <unordered_map>
#include <cmath>
#include <iostream>

namespace PathfindingFunctions {

    std::vector<std::pair<int,int>> calculate_path_simple(int sx, int sy, int sz, int ex, int ey, int ez) {
        if (sz != ez) return {};
        int dist = std::max(abs(ex - sx), abs(ey - sy));
        if (dist <= 1) return {};

        std::vector<std::pair<int,int>> path;
        int cx = sx, cy = sy;
        while (cx != ex || cy != ey) {
            int dx = (cx == ex) ? 0 : (ex > cx ? 1 : -1);
            int dy = (cy == ey) ? 0 : (ey > cy ? 1 : -1);
            cx += dx; cy += dy;
            path.push_back({cx, cy});
        }
        return path;
    }

    struct Node {
        int f, x, y;
        bool operator>(const Node& o) const { return f > o.f; }
    };

    std::vector<Step> calculate_path_astar(int sx, int sy, int ex, int ey,
                                            const std::set<std::pair<int,int>>& obstacles) {
        using P = std::pair<int,int>;
        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;
        std::unordered_map<int, std::unordered_map<int, P>> came_from;
        std::unordered_map<int, std::unordered_map<int, int>> g_score;

        open.push({abs(ex-sx)+abs(ey-sy), sx, sy});
        g_score[sx][sy] = 0;

        int dirs[4][2] = {{0,-1},{0,1},{1,0},{-1,0}};

        while (!open.empty()) {
            auto [cf, cx, cy] = open.top(); open.pop();

            if (cx == ex && cy == ey) {
                std::vector<Step> path;
                int nx = cx, ny = cy;
                while (came_from.count(nx) && came_from[nx].count(ny)) {
                    auto [px, py] = came_from[nx][ny];
                    path.push_back({nx - px, ny - py});
                    nx = px; ny = py;
                }
                std::reverse(path.begin(), path.end());
                return path;
            }

            for (auto& d : dirs) {
                int nx = cx + d[0], ny = cy + d[1];
                if (obstacles.count({nx, ny})) continue;
                int tg = g_score[cx][cy] + 1;
                if (!g_score.count(nx) || !g_score[nx].count(ny) || tg < g_score[nx][ny]) {
                    came_from[nx][ny] = {cx, cy};
                    g_score[nx][ny] = tg;
                    int h = abs(ex - nx) + abs(ey - ny);
                    open.push({tg + h, nx, ny});
                }
            }
        }
        return {};
    }

    std::vector<Waypoint> expand_waypoints(const std::vector<Waypoint>& waypoints) {
        if (waypoints.empty()) return {};
        std::vector<Waypoint> expanded;

        for (size_t i = 0; i < waypoints.size(); i++) {
            const auto& cur = waypoints[i];
            expanded.push_back(cur);

            if (i + 1 < waypoints.size()) {
                const auto& next = waypoints[i + 1];
                if (cur.Action == 0 && next.Action == 0) {
                    if (next.Direction == 0) {
                        auto path = calculate_path_astar(cur.X, cur.Y, next.X, next.Y);
                        if (path.size() > 1) {
                            int cx = cur.X, cy = cur.Y;
                            for (size_t s = 0; s + 1 < path.size(); s++) {
                                cx += path[s].dx; cy += path[s].dy;
                                expanded.push_back({cx, cy, cur.Z, 0, 0});
                            }
                        }
                    } else {
                        auto path = calculate_path_simple(cur.X, cur.Y, cur.Z, next.X, next.Y, next.Z);
                        if (path.size() > 1) {
                            for (size_t s = 0; s + 1 < path.size(); s++) {
                                expanded.push_back({path[s].first, path[s].second, cur.Z, 0, next.Direction});
                            }
                        }
                    }
                }
            }
        }
        return expanded;
    }
}
