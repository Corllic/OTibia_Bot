#include "SpellThread.h"
#include "../Functions/MemoryFunctions.h"
#include "../Functions/KeyboardFunctions.h"
#include "../Functions/MouseFunctions.h"
#include "../Core/Addresses.h"
#include <random>

SpellThread::SpellThread(std::vector<QVariantMap> data, QObject* parent)
    : QThread(parent), spell_data(std::move(data)) {}

void SpellThread::stop() { running = false; }

void SpellThread::run() {
    std::mt19937 rng(std::random_device{}());
    while (running) {
        try {
            if (!Addresses::attack_Lock.try_lock()) { QThread::msleep(50); continue; }
            Addresses::attack_Lock.unlock();

            for (auto& item : spell_data) {
                if (!running) break;
                if (MemoryFunctions::read_targeting_status() == 0) continue;

                auto [tx, ty, tz, tname, thp] = MemoryFunctions::read_target_info();
                auto [hp, max_hp, mp, max_mp]  = MemoryFunctions::read_my_stats();
                if (thp < 0 || thp > 100) thp = 100;
                if (max_hp == 0 || max_mp == 0) continue;

                double hp_pct  = (hp * 100.0) / max_hp;
                QString name   = item.value("Name",  "").toString();
                QString key    = item.value("Key",   "").toString();
                int hp_from    = item.value("HpFrom",  100).toInt();
                int hp_to      = item.value("HpTo",    0).toInt();
                int min_mp     = item.value("MinMp",   0).toInt();
                int min_hp     = item.value("MinHp",   0).toInt();
                int min_dist   = item.value("MinDist", 0).toInt();

                if (min_dist != 0) {
                    auto [mx, my, mz] = MemoryFunctions::read_my_wpt();
                    if (abs(mx - tx) > min_dist || abs(my - ty) > min_dist) continue;
                }

                bool name_match = (name == "*") || (QString::fromStdString(tname).contains(name));
                bool cond = (hp_from >= thp && thp > hp_to) && mp >= min_mp && name_match && hp_pct >= min_hp;

                if (cond) {
                    if (!key.isEmpty() && key[0] == 'F') {
                        KeyboardFunctions::press_hotkey(key.mid(1).toInt());
                        std::uniform_int_distribution<int> d(150, 250);
                        QThread::msleep(d(rng));
                    } else {
                        int cx_idx = (key == "First Rune") ? 6 : 8;
                        MouseFunctions::mouse_function(Addresses::coordinates_x[cx_idx], Addresses::coordinates_y[cx_idx], 0, 0, 1);
                        auto [mx, my, mz] = MemoryFunctions::read_my_wpt();
                        int dx = tx - mx, dy = ty - my;
                        MouseFunctions::mouse_function(
                            Addresses::coordinates_x[0] + dx * Addresses::square_size,
                            Addresses::coordinates_y[0] + dy * Addresses::square_size, 0, 0, 2);
                        std::uniform_int_distribution<int> d(800, 1000);
                        QThread::msleep(d(rng));
                    }
                }
            }
        } catch (...) {}
        std::uniform_int_distribution<int> s(100, 200);
        QThread::msleep(s(rng));
    }
}
