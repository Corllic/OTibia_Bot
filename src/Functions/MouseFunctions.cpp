#include "MouseFunctions.h"
#include "../Core/Addresses.h"
#include <windows.h>
#include <mutex>

namespace MouseFunctions {

    static std::mutex mouse_lock;

    static LPARAM make_long(int x, int y) { return MAKELPARAM(x, y); }

    void mouse_function(int x_source, int y_source, int x_dest, int y_dest, int option) {
        std::lock_guard<std::mutex> lk(mouse_lock);
        HWND g = Addresses::game;
        switch (option) {
            case 1:
                PostMessage(g, WM_MOUSEMOVE,   0, make_long(x_source, y_source));
                PostMessage(g, WM_RBUTTONDOWN,  2, make_long(x_source, y_source));
                PostMessage(g, WM_RBUTTONUP,    0, make_long(x_source, y_source));
                break;
            case 2:
                PostMessage(g, WM_MOUSEMOVE,   0, make_long(x_source, y_source));
                PostMessage(g, WM_LBUTTONDOWN,  1, make_long(x_source, y_source));
                PostMessage(g, WM_LBUTTONUP,    0, make_long(x_source, y_source));
                break;
            case 3:
                PostMessage(g, WM_MOUSEMOVE,   0, make_long(x_source, y_source));
                PostMessage(g, WM_LBUTTONDOWN,  1, make_long(x_source, y_source));
                PostMessage(g, WM_MOUSEMOVE,   0, make_long(x_dest, y_dest));
                PostMessage(g, WM_LBUTTONUP,    0, make_long(x_dest, y_dest));
                PostMessage(g, WM_MOUSEMOVE,   0, make_long(x_dest, y_dest));
                PostMessage(g, WM_RBUTTONDOWN,  2, make_long(x_dest, y_dest));
                PostMessage(g, WM_RBUTTONUP,    0, make_long(x_dest, y_dest));
                break;
            case 4:
                PostMessage(g, WM_MOUSEMOVE,   0, make_long(x_source, y_source));
                PostMessage(g, WM_LBUTTONDOWN,  1, make_long(x_source, y_source));
                PostMessage(g, WM_MOUSEMOVE,   1, make_long(x_dest, y_dest));
                PostMessage(g, WM_LBUTTONUP,    0, make_long(x_dest, y_dest));
                break;
            case 5:
                PostMessage(g, WM_MOUSEMOVE,   0, make_long(x_source, y_source));
                PostMessage(g, WM_RBUTTONDOWN,  2, make_long(x_source, y_source));
                PostMessage(g, WM_RBUTTONUP,    0, make_long(x_source, y_source));
                PostMessage(g, WM_MOUSEMOVE,   0, make_long(x_dest, y_dest));
                PostMessage(g, WM_LBUTTONDOWN,  1, make_long(x_dest, y_dest));
                PostMessage(g, WM_LBUTTONUP,    0, make_long(x_dest, y_dest));
                break;
        }
    }

    void manage_collect(int x, int y, int action) {
        if (action > 0) {
            mouse_function(x, y, Addresses::coordinates_x[action], Addresses::coordinates_y[action], 3);
        } else if (action == 0) {
            mouse_function(x, y, Addresses::coordinates_x[0], Addresses::coordinates_y[0], 4);
        } else if (action == -1) {
            mouse_function(x, y, 0, 0, 1);
        } else if (action == -2) {
            mouse_function(x, y, 0, 0, 2);
            mouse_function(x, y, 0, 0, 2);
        } else if (action == -3) {
            mouse_function(x, y, Addresses::coordinates_x[0], Addresses::coordinates_y[0], 5);
        }
    }
}
