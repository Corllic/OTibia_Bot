#pragma once
#include <vector>
#include <set>
#include <tuple>

namespace PathfindingFunctions {

    struct Step { int dx; int dy; };
    struct Waypoint { int X; int Y; int Z; int Action; int Direction; };

    std::vector<std::pair<int,int>> calculate_path_simple(int sx, int sy, int sz, int ex, int ey, int ez);
    std::vector<Step> calculate_path_astar(int sx, int sy, int ex, int ey,
                                           const std::set<std::pair<int,int>>& obstacles = {});
    std::vector<Waypoint> expand_waypoints(const std::vector<Waypoint>& waypoints);
}
