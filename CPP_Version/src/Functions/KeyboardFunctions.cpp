#include "KeyboardFunctions.h"
#include "../Core/Addresses.h"
#include <windows.h>
#include <cstdlib>

namespace KeyboardFunctions {

    static void send_key(int rp, DWORD lp) {
        PostMessage(Addresses::game, WM_KEYDOWN, rp, lp);
        PostMessage(Addresses::game, WM_KEYUP,   rp, lp);
    }

    void walk(int wpt_direction, int my_x, int my_y, int my_z, int map_x, int map_y, int map_z) {
        int x = map_x - my_x;
        int y = map_y - my_y;
        int z = map_z - my_z;

        if (wpt_direction != 0 && wpt_direction < 9) {
            if (wpt_direction == 1 && (-3 <= y && y < 0 || (y == 0 && x == 0)) && abs(z) <= 1)
                { send_key(Addresses::rParam[0], Addresses::lParam[0]); return; }
            if (wpt_direction == 2 && 0 < y && y <= 3 && abs(z) <= 1)
                { send_key(Addresses::rParam[1], Addresses::lParam[1]); return; }
            if (wpt_direction == 3 && 0 < x && x <= 3 && abs(z) <= 1)
                { send_key(Addresses::rParam[2], Addresses::lParam[2]); return; }
            if (wpt_direction == 4 && -3 <= x && x < 0 && abs(z) <= 1)
                { send_key(Addresses::rParam[3], Addresses::lParam[3]); return; }
        } else {
            if (x == 1  && y == 0  && z == 0) { send_key(Addresses::rParam[2], Addresses::lParam[2]); return; }
            if (x == -1 && y == 0  && z == 0) { send_key(Addresses::rParam[3], Addresses::lParam[3]); return; }
            if (x == 0  && y == 1  && z == 0) { send_key(Addresses::rParam[1], Addresses::lParam[1]); return; }
            if (x == 0  && y == -1 && z == 0) { send_key(Addresses::rParam[0], Addresses::lParam[0]); return; }
            if (x == 1  && y == -1 && z == 0) { send_key(Addresses::rParam[4], Addresses::lParam[4]); return; }
            if (x == -1 && y == -1 && z == 0) { send_key(Addresses::rParam[5], Addresses::lParam[5]); return; }
            if (x == 1  && y == 1  && z == 0) { send_key(Addresses::rParam[6], Addresses::lParam[6]); return; }
            if (x == -1 && y == 1  && z == 0) { send_key(Addresses::rParam[7], Addresses::lParam[7]); return; }
        }
    }

    void stay_diagonal(int my_x, int my_y, int monster_x, int monster_y) {
        int x = monster_x - my_x;
        int y = monster_y - my_y;
        if (abs(x) == 1 && abs(y) == 1) return;
        int r = rand() % 2;
        if ((x == 1 || x == -1) && y == 0) {
            send_key(Addresses::rParam[r == 0 ? 0 : 1], Addresses::lParam[r == 0 ? 0 : 1]); return;
        }
        if (x == 0 && (y == 1 || y == -1)) {
            send_key(Addresses::rParam[r == 0 ? 2 : 3], Addresses::lParam[r == 0 ? 2 : 3]); return;
        }
    }

    void chase_monster(int my_x, int my_y, int monster_x, int monster_y) {
        int x = monster_x - my_x;
        int y = monster_y - my_y;
        if (abs(x) == 1 && abs(y) == 1) return;
        int r = rand() % 2;

        if (x > 0 && y == 0) { send_key(Addresses::rParam[2], Addresses::lParam[2]); return; }
        if (x > 0 && y < 0)  { send_key(Addresses::rParam[r == 0 ? 2 : 0], Addresses::lParam[r == 0 ? 2 : 0]); return; }
        if (x > 0 && y > 0)  { send_key(Addresses::rParam[r == 0 ? 2 : 1], Addresses::lParam[r == 0 ? 2 : 1]); return; }
        if (x < 0 && y == 0) { send_key(Addresses::rParam[3], Addresses::lParam[3]); return; }
        if (x < 0 && y < 0)  { send_key(Addresses::rParam[r == 0 ? 3 : 0], Addresses::lParam[r == 0 ? 3 : 0]); return; }
        if (x < 0 && y > 0)  { send_key(Addresses::rParam[r == 0 ? 3 : 1], Addresses::lParam[r == 0 ? 3 : 1]); return; }
        if (x == 0 && y < 0) { send_key(Addresses::rParam[0], Addresses::lParam[0]); return; }
        if (x == 0 && y > 0) { send_key(Addresses::rParam[1], Addresses::lParam[1]); return; }
    }

    void chaseDiagonal_monster(int my_x, int my_y, int monster_x, int monster_y) {
        int xd = monster_x - my_x;
        int yd = monster_y - my_y;
        if (abs(xd) == 1 && abs(yd) == 1) return;
        if ((xd == 0 && abs(yd) == 1) || (yd == 0 && abs(xd) == 1))
            stay_diagonal(my_x, my_y, monster_x, monster_y);
        else
            chase_monster(my_x, my_y, monster_x, monster_y);
    }

    void press_key(char key) {
        SHORT vk = VkKeyScanA(key);
        if (vk != -1) {
            UINT scan = MapVirtualKey(vk & 0xFF, 0);
            DWORD kd_lp = (scan << 16) | 0x0001;
            DWORD ku_lp = kd_lp | (0x3 << 30);
            PostMessage(Addresses::game, WM_KEYDOWN, vk & 0xFF, kd_lp);
            PostMessage(Addresses::game, WM_KEYUP,   vk & 0xFF, ku_lp);
        }
    }

    void press_hotkey(int hotkey) {
        DWORD lp = (((0x003A0001 >> 16) + hotkey) << 16) + 1;
        PostMessage(Addresses::game, WM_KEYDOWN, 0x6F + hotkey, lp);
        PostMessage(Addresses::game, WM_KEYUP,   0x6F + hotkey, lp);
    }
}
