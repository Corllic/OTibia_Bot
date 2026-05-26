#pragma once

namespace KeyboardFunctions {
    void walk(int wpt_direction, int my_x, int my_y, int my_z, int map_x, int map_y, int map_z);
    void stay_diagonal(int my_x, int my_y, int monster_x, int monster_y);
    void chase_monster(int my_x, int my_y, int monster_x, int monster_y);
    void chaseDiagonal_monster(int my_x, int my_y, int monster_x, int monster_y);
    void press_key(char key);
    void press_hotkey(int hotkey);
}
