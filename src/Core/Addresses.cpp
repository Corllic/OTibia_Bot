#include "Addresses.h"
#include "Logger.h"
#include <windows.h>
#include <psapi.h>
#include <sstream>
#include <QSettings>

namespace Addresses {

    // 0 → No logs
    // 1 → Console
    // 2 → File logs.txt
    // 3 → Console + File
    int log_level = 1;

    std::mutex walker_Lock;
    std::mutex attack_Lock;

    DWORD lParam[8] = {
        0x00480001, 0x00500001, 0x004D0001,
        0x004B0001, 0x00490001, 0x00470001,
        0x00510001, 0x004F0001
    };
    DWORD rParam[8] = {
        0x26, 0x28, 0x27,
        0x25, 0x21, 0x24,
        0x22, 0x23
    };

    uintptr_t my_x_address = 0;
    std::vector<uintptr_t> my_x_address_offset;
    int my_x_type = 3;

    uintptr_t my_y_address = 0;
    std::vector<uintptr_t> my_y_address_offset;
    int my_y_type = 3;

    uintptr_t my_z_address = 0;
    std::vector<uintptr_t> my_z_address_offset;
    int my_z_type = 2;

    uintptr_t my_stats_address = 0;

    std::vector<uintptr_t> my_hp_offset;
    std::vector<uintptr_t> my_hp_max_offset;
    int my_hp_type = 2;

    std::vector<uintptr_t> my_mp_offset;
    std::vector<uintptr_t> my_mp_max_offset;
    int my_mp_type = 2;

    uintptr_t attack_address = 0;
    std::vector<uintptr_t> attack_address_offset;
    int my_attack_type = 3;

    uintptr_t target_x_offset = 0;
    int target_x_type = 3;

    uintptr_t target_y_offset = 0;
    int target_y_type = 3;

    uintptr_t target_z_offset = 0;
    int target_z_type = 2;

    uintptr_t target_hp_offset = 0;
    int target_hp_type = 1;

    uintptr_t target_name_offset = 0;
    int target_name_type = 6;

    uintptr_t creature_base_address = 0;
    std::vector<uintptr_t> creature_base_offset;
    uintptr_t creature_x_off        = 0x34;
    uintptr_t creature_y_off        = 0x38;
    uintptr_t creature_z_off        = 0x3C;
    uintptr_t creature_name_len_off = 0x3C;
    uintptr_t creature_name_off     = 0x40;
    bool      creature_name_is_unicode  = true;
    bool      creature_name_is_ptr      = true;
    uintptr_t creature_found_address    = 0;
    uint32_t  creature_id_prefix        = 0;
    uint32_t  creature_id_prefix_div    = 100000;

    std::string game_name;
    HWND game = nullptr;
    uintptr_t base_address = 0;
    HANDLE process_handle = nullptr;
    DWORD proc_id = 0;
    std::string client_name;
    int square_size = 75;
    int application_architecture = 32;
    double collect_threshold = 0.85;
    int attack_key = 1;
    int walk_mode  = 0;

    int screen_x[1] = {0};
    int screen_y[1] = {0};
    int battle_x[1] = {0};
    int battle_y[1] = {0};
    int screen_width[2] = {0, 0};
    int screen_height[2] = {0, 0};
    int coordinates_x[12] = {};
    int coordinates_y[12] = {};
    int fishing_x[4] = {};
    int fishing_y[4] = {};

    const char* dark_theme = "";

    bool enable_debug_privilege() {
        HANDLE hToken;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
            return false;
        LUID luid;
        if (!LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &luid)) {
            CloseHandle(hToken);
            return false;
        }
        TOKEN_PRIVILEGES tp;
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
        CloseHandle(hToken);
        return true;
    }

    uintptr_t parse_hex(const std::string& val) {
        std::string s = val;
        while (!s.empty() && s.front() == ' ') s.erase(s.begin());
        while (!s.empty() && s.back() == ' ') s.pop_back();
        if (s.empty()) return 0;
        try {
            return static_cast<uintptr_t>(std::stoull(s, nullptr, 16));
        } catch (...) {
            return 0;
        }
    }

    std::vector<uintptr_t> parse_offsets(const std::string& val) {
        std::vector<uintptr_t> result;
        std::istringstream ss(val);
        std::string token;
        while (std::getline(ss, token, ',')) {
            while (!token.empty() && token.front() == ' ') token.erase(token.begin());
            while (!token.empty() && token.back() == ' ') token.pop_back();
            if (!token.empty()) {
                try {
                    result.push_back(static_cast<uintptr_t>(std::stoull(token, nullptr, 16)));
                } catch (...) {}
            }
        }
        return result;
    }

    void load_custom_addresses() {
        QSettings s("EasyBot", "Addresses");

        s.beginGroup("game_config");
        square_size  = s.value("square_size", 75).toInt();
        attack_key   = s.value("attack_key",   1).toInt();
        walk_mode    = s.value("walk_mode",     0).toInt();
        s.endGroup();

        struct Entry {
            const char*              key;
            uintptr_t*               addr;
            std::vector<uintptr_t>*  offsets;
        };

        Entry entries[] = {
            {"my_x",      &my_x_address,     &my_x_address_offset},
            {"my_y",      &my_y_address,     &my_y_address_offset},
            {"my_z",      &my_z_address,     &my_z_address_offset},
            {"my_hp",     &my_stats_address, &my_hp_offset},
            {"my_hp_max", nullptr,           &my_hp_max_offset},
            {"my_mp",     nullptr,           &my_mp_offset},
            {"my_mp_max", nullptr,           &my_mp_max_offset},
            {"attack",         &attack_address,         &attack_address_offset},
            {"creature_base",  &creature_base_address,  &creature_base_offset},
            {"target_x",       nullptr,                 nullptr},
            {"target_y",       nullptr,                 nullptr},
            {"target_z",       nullptr,                 nullptr},
            {"target_hp",      nullptr,                 nullptr},
            {"target_name",    nullptr,                 nullptr},
        };

        uintptr_t* target_single[] = {
            &target_x_offset, &target_y_offset, &target_z_offset,
            &target_hp_offset, &target_name_offset
        };
        int ti = 0;

        for (auto& e : entries) {
            s.beginGroup(e.key);
            QString addr_str   = s.value("address").toString();
            QString offset_str = s.value("offset").toString();
            s.endGroup();

            if (e.addr && !addr_str.isEmpty()) {
                uintptr_t v = parse_hex(addr_str.toStdString());
                if (v) *e.addr = v;
            }

            bool is_target = (e.addr == nullptr && e.offsets == nullptr);
            if (is_target) {
                if (!offset_str.isEmpty()) {
                    auto offs = parse_offsets(offset_str.toStdString());
                    if (!offs.empty()) *target_single[ti] = offs[0];
                }
                ti++;
            } else if (e.offsets && !offset_str.isEmpty()) {
                *e.offsets = parse_offsets(offset_str.toStdString());
            }
        }

        Logger::log("[Addresses] Loaded addresses from registry");
    }

    void load_tibia(const std::string& window_title, DWORD pid, HWND hwnd) {
        square_size = 75;
        application_architecture = 32;
        collect_threshold = 0.95;

        my_x_address = 0;
        my_stats_address = 0;
        attack_address = 0;
        creature_base_address = 0;

        my_x_address_offset.clear();
        my_y_address_offset.clear();
        my_z_address_offset.clear();
        my_hp_offset.clear();
        my_hp_max_offset.clear();
        my_mp_offset.clear();
        my_mp_max_offset.clear();
        attack_address_offset.clear();
        creature_base_offset.clear();

        load_custom_addresses();

        target_x_offset = 0;
        target_y_offset = 0;
        target_z_offset = 0;
        target_hp_offset = 0;
        target_name_offset = 0;

        if (!window_title.empty() && pid && hwnd) {
            game_name = window_title;
            game = hwnd;
            proc_id = pid;
            size_t sp = window_title.find(' ');
            client_name = (sp != std::string::npos) ? window_title.substr(0, sp) : window_title;
        } else {
            client_name = "";
            game_name = "";
            game = nullptr;
        }

        Logger::log("[Addresses] Connected to: " + game_name);

        process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, proc_id);

        HMODULE mods[1024];
        DWORD needed;
        if (EnumProcessModules(process_handle, mods, sizeof(mods), &needed)) {
            base_address = reinterpret_cast<uintptr_t>(mods[0]);
        }

        BOOL is_wow64 = FALSE;
        if (IsWow64Process(process_handle, &is_wow64)) {
            application_architecture = is_wow64 ? 32 : 64;
        }

        std::ostringstream ss;
        ss << "[Addresses] architecture=" << application_architecture
           << " base=0x" << std::hex << base_address;
        Logger::log(ss.str());
    }
}
