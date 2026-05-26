#include "HealingThread.h"
#include "../Functions/MemoryFunctions.h"
#include "../Functions/MouseFunctions.h"
#include "../Functions/KeyboardFunctions.h"
#include "../Core/Addresses.h"
#include <QThread>
#include <random>

HealThread::HealThread(std::vector<QVariantMap> s_data,
                       std::vector<QVariantMap> i_data,
                       QObject* parent)
    : QThread(parent), spell_data(std::move(s_data)), item_data(std::move(i_data)) {}

void HealThread::stop() { running = false; }

bool HealThread::eval_condition(const QVariantMap& entry, int hp, int max_hp, int mp, int max_mp) const {
    QString when     = entry.value("When",     "Health Percent").toString();
    QString cmp      = entry.value("Is",       "Below").toString();
    int val          = entry.value("Value",    0).toInt();
    int mana_cost    = entry.value("ManaCost", 0).toInt();

    double actual = 0.0;
    if (when == "Current Mana")        actual = mp;
    else if (when == "Current Health") actual = hp;
    else if (when == "Mana Percent")   actual = max_mp > 0 ? (mp * 100.0 / max_mp) : 0;
    else                               actual = max_hp > 0 ? (hp * 100.0 / max_hp) : 0;

    bool cond = false;
    if (cmp == "Below")       cond = actual < val;
    else if (cmp == "Above")  cond = actual > val;
    else                      cond = (int)actual == val;

    if (!cond) return false;
    if (mp < mana_cost) return false;
    return true;
}

void HealThread::run() {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> poll(100, 200);
    std::uniform_int_distribution<int> act(200, 400);

    while (running) {
        try {
            auto [hp, max_hp, mp, max_mp] = MemoryFunctions::read_my_stats();
            if (max_hp > 0 && max_mp > 0) {
                for (auto& entry : spell_data) {
                    if (!running) break;
                    if (!eval_condition(entry, hp, max_hp, mp, max_mp)) continue;
                    QString key = entry.value("Key", "F1").toString();
                    KeyboardFunctions::press_hotkey(key.mid(1).toInt());
                    QThread::msleep(act(rng));
                    break;
                }
                for (auto& entry : item_data) {
                    if (!running) break;
                    if (!eval_condition(entry, hp, max_hp, mp, max_mp)) continue;
                    QString key_type = entry.value("KeyType", "Hotkey").toString();
                    if (key_type == "Hotkey") {
                        QString key = entry.value("Key", "F1").toString();
                        KeyboardFunctions::press_hotkey(key.mid(1).toInt());
                    } else {
                        int ix = entry.value("X", 0).toInt();
                        int iy = entry.value("Y", 0).toInt();
                        MouseFunctions::mouse_function(ix, iy, 1);
                        MouseFunctions::mouse_function(Addresses::coordinates_x[0], Addresses::coordinates_y[0], 2);
                    }
                    QThread::msleep(act(rng));
                    break;
                }
            }
        } catch (...) {}
        QThread::msleep(poll(rng));
    }
}
