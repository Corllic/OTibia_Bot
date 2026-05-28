#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <cstdint>

namespace CreatureScanner {

struct Creature {
    uintptr_t address;
    int       x;
    int       y;
    int       z;
    std::string name;
    int       hp_pct;
};

extern std::atomic<int> scan_progress_pct;
extern std::mutex       creatures_mutex;
extern std::vector<Creature> known_creatures;

std::vector<Creature> scan_map(int player_x, int player_y, int player_z);
void                  refresh_known();
void                  reset_regions();

}
