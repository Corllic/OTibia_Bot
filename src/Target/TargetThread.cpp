#include "TargetThread.h"
#include "../Functions/MemoryFunctions.h"
#include "../Functions/KeyboardFunctions.h"
#include "../Functions/MouseFunctions.h"
#include "../Functions/PathfindingFunctions.h"
#include "../Core/Addresses.h"
#include <random>

TargetThread::TargetThread(std::vector<QVariantMap> tgts, int ak,
                            std::set<std::tuple<int,int,int>> bl, QObject* parent)
    : QThread(parent), targets(std::move(tgts)),
      blacklist_tiles(std::move(bl)), attack_key(ak + 1) {}

void TargetThread::stop() { running = false; }

void TargetThread::run() {
    std::mt19937 rng(std::random_device{}());
    auto [mx, my, mz] = MemoryFunctions::read_my_wpt();
    auto prev = std::make_tuple(mx, my, mz);
    int stuck_timer = 0;

    while (running) {
        std::uniform_int_distribution<int> sleep_d(70, 100);
        QThread::msleep(sleep_d(rng));
        try {
            int target_id = MemoryFunctions::read_targeting_status();

            if (target_id == 0) {
                discovered_obstacles.clear(); stuck_timer = 0; last_target_pos = {-1, -1};
                KeyboardFunctions::press_hotkey(attack_key);
                std::uniform_int_distribution<int> d(100, 150);
                QThread::msleep(d(rng));
                target_id = MemoryFunctions::read_targeting_status();
                if (target_id == 0 && Addresses::walker_Lock.try_lock()) Addresses::walker_Lock.unlock();
            } else {
                auto [tx, ty, tz, tname, thp] = MemoryFunctions::read_target_info();

                bool match = false;
                int target_idx = 0;
                for (int i = 0; i < (int)targets.size(); i++) {
                    QString n = targets[i].value("Name", "").toString();
                    if (n.toStdString() == tname || n == "*") { match = true; target_idx = i; break; }
                }

                if (match) {
                    auto& td = targets[target_idx];
                    while (MemoryFunctions::read_targeting_status() != 0 && running) {
                        auto [ttx, tty, ttz, tn, ttp] = MemoryFunctions::read_target_info();
                        if (tz == ttz) { tx = ttx; ty = tty; tz = ttz; }
                        std::uniform_int_distribution<int> sv(40, 80);
                        int sleep_val = sv(rng);
                        auto [x, y, z] = MemoryFunctions::read_my_wpt();
                        int dist_x = abs(x - tx), dist_y = abs(y - ty);
                        int dist = td.value("Dist", 0).toInt();

                        if ((dist >= dist_x && dist >= dist_y) || dist == 0) {
                            bool acquired = Addresses::walker_Lock.try_lock();
                            if (!acquired) Addresses::walker_Lock.lock();

                            if (dist_x > 1 || dist_y > 1) {
                                int stance = td.value("Stance", 0).toInt();
                                if (stance == 1) {
                                    std::set<std::pair<int,int>> bl2d;
                                    for (auto& [bx, by, bz] : blacklist_tiles)
                                        if (bz == z) bl2d.insert({bx, by});
                                    auto all_obs = discovered_obstacles;
                                    for (auto& o : bl2d) all_obs.insert(o);
                                    auto path = PathfindingFunctions::calculate_path_astar(x, y, tx, ty, all_obs);
                                    if (!path.empty()) {
                                        last_target_pos = {x + path[0].dx, y + path[0].dy};
                                        KeyboardFunctions::walk(0, x, y, z, x + path[0].dx, y + path[0].dy, z);
                                        std::uniform_int_distribution<int> wd(100, 200);
                                        QThread::msleep(wd(rng));
                                        auto [nx, ny, nz] = MemoryFunctions::read_my_wpt();
                                        if (std::make_tuple(nx, ny, nz) == prev) stuck_timer += sleep_val;
                                        else { prev = {nx, ny, nz}; stuck_timer = 0; }
                                        if (stuck_timer > 400 && last_target_pos.first != -1) {
                                            discovered_obstacles.insert(last_target_pos);
                                            stuck_timer = 0; last_target_pos = {-1, -1};
                                        }
                                    }
                                }
                            }
                        } else {
                            if (Addresses::walker_Lock.try_lock()) Addresses::walker_Lock.unlock();
                            KeyboardFunctions::press_hotkey(attack_key);
                            std::uniform_int_distribution<int> d(100, 150);
                            QThread::msleep(d(rng));
                        }
                        QThread::msleep(sleep_val);
                    }

                    auto [x, y, z] = MemoryFunctions::read_my_wpt();
                    int cx = Addresses::coordinates_x[0] + (tx - x) * Addresses::square_size;
                    int cy = Addresses::coordinates_y[0] + (ty - y) * Addresses::square_size;

                    int skin = td.value("Skin", 0).toInt();
                    if (skin > 0) {
                        KeyboardFunctions::press_hotkey(skin);
                        std::uniform_int_distribution<int> d1(10, 50);
                        QThread::msleep(d1(rng));
                        MouseFunctions::mouse_function(cx, cy, 0, 0, 2);
                        std::uniform_int_distribution<int> d2(150, 250);
                        QThread::msleep(d2(rng));
                    }
                } else {
                    if (Addresses::walker_Lock.try_lock()) Addresses::walker_Lock.unlock();
                    KeyboardFunctions::press_hotkey(attack_key);
                    std::uniform_int_distribution<int> d(100, 150);
                    QThread::msleep(d(rng));
                }
            }
        } catch (...) {}
    }
}
