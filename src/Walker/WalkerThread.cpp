#include "WalkerThread.h"
#include "../Functions/MemoryFunctions.h"
#include "../Functions/KeyboardFunctions.h"
#include "../Functions/MouseFunctions.h"
#include "../Functions/PathfindingFunctions.h"
#include "../Core/Addresses.h"
#include <QMutexLocker>
#include <random>

WalkerThread::WalkerThread(std::vector<PathfindingFunctions::Waypoint> wpts, QObject* parent)
    : QThread(parent), waypoints(std::move(wpts)) {}

void WalkerThread::stop() { running = false; }

int WalkerThread::find_wpt() {
    auto [x, y, z] = MemoryFunctions::read_my_wpt();
    for (int i = 0; i < (int)waypoints.size(); i++) {
        auto& w = waypoints[i];
        if (z == w.Z && abs(w.X - x) <= 4 && abs(w.Y - y) <= 4 && w.Direction == 0)
            return i;
    }
    return 0;
}

void WalkerThread::run() {
    if (waypoints.empty()) return;
    std::mt19937 rng(std::random_device{}());
    int current = find_wpt();
    int timer = 0, second_timer_ms = 0;
    auto [mx, my, mz] = MemoryFunctions::read_my_wpt();
    auto prev = std::make_tuple(mx, my, mz);

    while (running) {
        std::uniform_int_distribution<int> sleep_d(10, 50);
        int sl = sleep_d(rng);
        QThread::msleep(sl);

        bool walker_locked = !Addresses::walker_Lock.try_lock();
        if (!walker_locked) Addresses::walker_Lock.unlock();

        if (!walker_locked) {
            timer += sl;
            second_timer_ms += sl;
        }
        if (timer >= 5000) {
            current = find_wpt();
            discovered_obstacles.clear();
            timer = 0; second_timer_ms = 0;
        }

        emit index_update(0, current);
        auto& wpt = waypoints[current];
        auto [x, y, z] = MemoryFunctions::read_my_wpt();

        if (x == wpt.X && y == wpt.Y && z == wpt.Z) {
            current = (current + 1) % (int)waypoints.size();
            discovered_obstacles.clear(); timer = 0; continue;
        }

        while (!Addresses::walker_Lock.try_lock() && wpt.Action != 4 && running) QThread::msleep(200);
        if (!running) break;
        if (wpt.Action != 4) Addresses::walker_Lock.unlock();

        try {
            if (wpt.Action == 0 || wpt.Action == 4) {
                if (wpt.Direction == 0) {
                    auto path = PathfindingFunctions::calculate_path_astar(x, y, wpt.X, wpt.Y, discovered_obstacles);
                    if (!path.empty()) {
                        last_target_pos = {x + path[0].dx, y + path[0].dy};
                        KeyboardFunctions::walk(0, x, y, z, x + path[0].dx, y + path[0].dy, wpt.Z);
                    }
                } else {
                    KeyboardFunctions::walk(wpt.Direction, x, y, z, wpt.X, wpt.Y, wpt.Z);
                }
            } else if (wpt.Action == 1) {
                QThread::msleep(550);
                MouseFunctions::mouse_function(Addresses::coordinates_x[10], Addresses::coordinates_y[10], 0, 0, 1);
                QThread::msleep(150);
                auto [nx, ny, nz] = MemoryFunctions::read_my_wpt();
                MouseFunctions::mouse_function(
                    Addresses::coordinates_x[0] + (wpt.X - nx) * Addresses::square_size,
                    Addresses::coordinates_y[0] + (wpt.Y - ny) * Addresses::square_size, 0, 0, 2);
                current = (current + 1) % (int)waypoints.size();
            } else if (wpt.Action == 2) {
                QThread::msleep(550);
                MouseFunctions::mouse_function(Addresses::coordinates_x[9], Addresses::coordinates_y[9], 0, 0, 1);
                QThread::msleep(150);
                auto [nx, ny, nz] = MemoryFunctions::read_my_wpt();
                MouseFunctions::mouse_function(
                    Addresses::coordinates_x[0] + (wpt.X - nx) * Addresses::square_size,
                    Addresses::coordinates_y[0] + (wpt.Y - ny) * Addresses::square_size, 0, 0, 2);
                current = (current + 1) % (int)waypoints.size();
            } else if (wpt.Action == 3) {
                QThread::msleep(550);
                MouseFunctions::mouse_function(Addresses::coordinates_x[0], Addresses::coordinates_y[0], 0, 0, 1);
                current = (current + 1) % (int)waypoints.size();
            }
        } catch (...) {}

        auto [nx2, ny2, nz2] = MemoryFunctions::read_my_wpt();
        if (std::make_tuple(nx2, ny2, nz2) == prev) {
            second_timer_ms += sl;
        } else {
            prev = {nx2, ny2, nz2};
            second_timer_ms = 0;
        }
        if (second_timer_ms > 3000 && last_target_pos.first != -1) {
            discovered_obstacles.insert(last_target_pos);
            last_target_pos = {-1, -1};
        }
    }
}

RecordThread::RecordThread(int interval, QObject* parent) : QThread(parent), interval(interval) {}
void RecordThread::stop() { running = false; }

void RecordThread::update_snapshot(int action_id, int direction_id, const std::string& direction_text) {
    QMutexLocker lk(&data_lock);
    current_action    = action_id;
    current_direction = direction_id;
    current_dir_text  = direction_text;
}

void RecordThread::run() {
    auto [x, y, z] = MemoryFunctions::read_my_wpt();
    {
        QMutexLocker lk(&data_lock);
        emit wpt_recorded(x, y, z, current_action, current_direction, QString::fromStdString(current_dir_text));
    }
    int lx = x, ly = y, lz = z;
    int ox = x, oy = y, oz = z;

    while (running) {
        try {
            auto [nx, ny, nz] = MemoryFunctions::read_my_wpt();
            if (nz != oz) {
                int nd = 0; std::string dt = "Center";
                if (ny > oy && nx == ox) { nd = 2; dt = "South"; }
                else if (ny < oy && nx == ox) { nd = 1; dt = "North"; }
                else if (ny == oy && nx > ox) { nd = 3; dt = "East"; }
                else if (ny == oy && nx < ox) { nd = 4; dt = "West"; }
                if (nd != 0) emit wpt_recorded(nx, ny, nz, 0, nd, QString::fromStdString(dt));
            }
            if ((nx != ox || ny != oy) && nz == oz) {
                int dist = abs(nx - lx) + abs(ny - ly);
                if (dist >= interval) {
                    QMutexLocker lk(&data_lock);
                    emit wpt_recorded(nx, ny, nz, current_action, current_direction, QString::fromStdString(current_dir_text));
                    lx = nx; ly = ny; lz = nz;
                }
            }
            ox = nx; oy = ny; oz = nz;
        } catch (...) {}
        QThread::msleep(100);
    }
}
