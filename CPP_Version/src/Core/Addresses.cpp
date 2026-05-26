#include "Addresses.h"
#include <windows.h>
#include <psapi.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace Addresses {

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
        QFile f("Save/Settings/addresses.json");
        if (!f.open(QIODevice::ReadOnly)) return;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        if (doc.isNull()) return;
        QJsonObject data = doc.object();

        if (data.contains("game_config")) {
            QJsonObject cfg = data["game_config"].toObject();
            QString ss = cfg.value("square_size").toString();
            if (!ss.isEmpty()) square_size = ss.toInt();
            QString ct = cfg.value("collect_threshold").toString();
            if (!ct.isEmpty()) collect_threshold = ct.toDouble();
            QString arch = cfg.value("architecture").toString();
            if (!arch.isEmpty()) application_architecture = arch.contains("64") ? 64 : 32;
            if (cfg.contains("attack_key")) attack_key = cfg.value("attack_key").toInt(1);
            if (cfg.contains("walk_mode"))  walk_mode  = cfg.value("walk_mode").toInt(0);
        }

        struct Mapping {
            std::string key;
            uintptr_t* addr_var;
            std::vector<uintptr_t>* offset_var;
            int* type_var;
            bool is_target;
        };

        std::vector<Mapping> mappings = {
            {"my_x",      &my_x_address,     &my_x_address_offset,   &my_x_type,       false},
            {"my_y",      &my_y_address,     &my_y_address_offset,   &my_y_type,       false},
            {"my_z",      &my_z_address,     &my_z_address_offset,   &my_z_type,       false},
            {"attack",    &attack_address,   &attack_address_offset, &my_attack_type,  false},
            {"my_hp",     &my_stats_address, &my_hp_offset,          &my_hp_type,      false},
            {"my_hp_max", nullptr,           &my_hp_max_offset,      nullptr,          false},
            {"my_mp",     nullptr,           &my_mp_offset,          &my_mp_type,      false},
            {"my_mp_max", nullptr,           &my_mp_max_offset,      nullptr,          false},
            {"target_x",  nullptr,           nullptr,                &target_x_type,   true},
            {"target_y",  nullptr,           nullptr,                &target_y_type,   true},
            {"target_z",  nullptr,           nullptr,                &target_z_type,   true},
            {"target_hp", nullptr,           nullptr,                &target_hp_type,  true},
            {"target_name", nullptr,         nullptr,                &target_name_type,true},
        };

        uintptr_t* target_offsets[] = {
            &target_x_offset, &target_y_offset, &target_z_offset,
            &target_hp_offset, &target_name_offset
        };
        int ti = 0;

        for (auto& m : mappings) {
            QString qkey = QString::fromStdString(m.key);
            if (!data.contains(qkey)) continue;
            QJsonObject entry = data[qkey].toObject();

            if (m.addr_var && entry.contains("address")) {
                uintptr_t v = parse_hex(entry["address"].toString().toStdString());
                if (v) *m.addr_var = v;
            }

            if (m.is_target && entry.contains("offset")) {
                auto offsets = parse_offsets(entry["offset"].toString().toStdString());
                if (!offsets.empty()) *target_offsets[ti] = offsets[0];
                ti++;
            } else if (m.offset_var && entry.contains("offset")) {
                *m.offset_var = parse_offsets(entry["offset"].toString().toStdString());
            }

        }

        if (data.contains("coordinates")) {
            QJsonObject co = data["coordinates"].toObject();
            if (co.contains("X")) {
                QJsonArray xv = co["X"].toArray();
                for (int i = 0; i < xv.size() && i < 12; i++) coordinates_x[i] = xv[i].toInt();
            }
            if (co.contains("Y")) {
                QJsonArray yv = co["Y"].toArray();
                for (int i = 0; i < yv.size() && i < 12; i++) coordinates_y[i] = yv[i].toInt();
            }
            if (co.contains("screen_x") && co["screen_x"].toArray().size() > 0)
                screen_x[0] = co["screen_x"].toArray()[0].toInt();
            if (co.contains("screen_y") && co["screen_y"].toArray().size() > 0)
                screen_y[0] = co["screen_y"].toArray()[0].toInt();
            if (co.contains("screen_width")) {
                QJsonArray sw = co["screen_width"].toArray();
                for (int i = 0; i < sw.size() && i < 2; i++) screen_width[i] = sw[i].toInt();
            }
            if (co.contains("screen_height")) {
                QJsonArray sh = co["screen_height"].toArray();
                for (int i = 0; i < sh.size() && i < 2; i++) screen_height[i] = sh[i].toInt();
            }
            if (co.contains("battle_x") && co["battle_x"].toArray().size() > 0)
                battle_x[0] = co["battle_x"].toArray()[0].toInt();
            if (co.contains("battle_y") && co["battle_y"].toArray().size() > 0)
                battle_y[0] = co["battle_y"].toArray()[0].toInt();
        }

        std::cout << "Loaded dynamic addresses\n";
    }

    void load_tibia(const std::string& window_title, DWORD pid, HWND hwnd) {
        square_size = 75;
        application_architecture = 32;
        collect_threshold = 0.95;

        my_x_address = 0;
        my_stats_address = 0;
        attack_address = 0;

        my_x_address_offset.clear();
        my_y_address_offset.clear();
        my_z_address_offset.clear();
        my_hp_offset.clear();
        my_hp_max_offset.clear();
        my_mp_offset.clear();
        my_mp_max_offset.clear();
        attack_address_offset.clear();

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

        std::filesystem::create_directories("Images/" + client_name);
        std::cout << "Connected to: " << game_name << "\n";

        process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, proc_id);
        HMODULE mods[1024];
        DWORD needed;
        if (EnumProcessModules(process_handle, mods, sizeof(mods), &needed)) {
            base_address = reinterpret_cast<uintptr_t>(mods[0]);
        }
    }
}
