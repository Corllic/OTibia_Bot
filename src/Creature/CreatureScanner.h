#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <atomic>

namespace CreatureScanner {

extern std::atomic<int> scan_progress_pct;

struct Creature {
    int x, y, z;
    std::string name;
    uintptr_t address;
};

std::vector<Creature> scan(int player_x, int player_y, int player_z,
                            int range_x = 7, int range_y = 5);

bool detect_name_offset(int player_x, int player_y, int player_z,
                         const std::string& hint_name);

bool detect_all_offsets(int cx1, int cy1, int cz1, const std::string& name1,
                         int cx2, int cy2, int cz2, const std::string& name2);

}
