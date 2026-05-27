#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

namespace Addresses {

    constexpr int TITLE_BAR_OFFSET = 35;

    extern int log_level;

    extern std::mutex walker_Lock;
    extern std::mutex attack_Lock;

    extern DWORD lParam[8];
    extern DWORD rParam[8];

    extern uintptr_t my_x_address;
    extern std::vector<uintptr_t> my_x_address_offset;
    extern int my_x_type;

    extern uintptr_t my_y_address;
    extern std::vector<uintptr_t> my_y_address_offset;
    extern int my_y_type;

    extern uintptr_t my_z_address;
    extern std::vector<uintptr_t> my_z_address_offset;
    extern int my_z_type;

    extern uintptr_t my_stats_address;

    extern std::vector<uintptr_t> my_hp_offset;
    extern std::vector<uintptr_t> my_hp_max_offset;
    extern int my_hp_type;

    extern std::vector<uintptr_t> my_mp_offset;
    extern std::vector<uintptr_t> my_mp_max_offset;
    extern int my_mp_type;

    extern uintptr_t attack_address;
    extern std::vector<uintptr_t> attack_address_offset;
    extern int my_attack_type;

    extern uintptr_t target_x_offset;
    extern int target_x_type;

    extern uintptr_t target_y_offset;
    extern int target_y_type;

    extern uintptr_t target_z_offset;
    extern int target_z_type;

    extern uintptr_t target_hp_offset;
    extern int target_hp_type;

    extern uintptr_t target_name_offset;
    extern int target_name_type;

    extern uintptr_t creature_base_address;
    extern std::vector<uintptr_t> creature_base_offset;
    extern uintptr_t creature_x_off;
    extern uintptr_t creature_y_off;
    extern uintptr_t creature_z_off;
    extern uintptr_t creature_name_len_off;
    extern uintptr_t creature_name_off;
    extern bool      creature_name_is_unicode;
    extern bool      creature_name_is_ptr;
    extern uintptr_t creature_found_address;
    extern uint32_t  creature_id_prefix;
    extern uint32_t  creature_id_prefix_div;

    extern std::string game_name;
    extern HWND game;
    extern uintptr_t base_address;
    extern HANDLE process_handle;
    extern DWORD proc_id;
    extern std::string client_name;
    extern int square_size;
    extern int application_architecture;
    extern double collect_threshold;
    extern int attack_key;
    extern int walk_mode;

    extern int screen_x[1];
    extern int screen_y[1];
    extern int battle_x[1];
    extern int battle_y[1];
    extern int screen_width[2];
    extern int screen_height[2];
    extern int coordinates_x[12];
    extern int coordinates_y[12];
    extern int fishing_x[4];
    extern int fishing_y[4];

    extern const char* dark_theme;

    void load_tibia(const std::string& window_title = "", DWORD pid = 0, HWND hwnd = nullptr);
    void load_custom_addresses();
    uintptr_t parse_hex(const std::string& val);
    std::vector<uintptr_t> parse_offsets(const std::string& val);
    bool enable_debug_privilege();
}
