#include "MemoryFunctions.h"
#include "../Core/Addresses.h"
#include <iostream>
#include <cstring>

namespace MemoryFunctions {

    static int pointer_size() { return Addresses::application_architecture / 8; }

    static bool rpm(LPCVOID addr, void* buf, SIZE_T sz) {
        SIZE_T rd = 0;
        return ReadProcessMemory(Addresses::process_handle, addr, buf, sz, &rd) && rd > 0;
    }

    std::optional<int64_t> read_memory_address(uintptr_t address_read, uintptr_t offset, int option) {
        try {
            uintptr_t addr = Addresses::base_address + address_read + offset;
            int buf_size = (option < 6) ? pointer_size() : 32;
            std::vector<uint8_t> buffer(buf_size, 0);
            if (!rpm(reinterpret_cast<LPCVOID>(addr), buffer.data(), buf_size)) return std::nullopt;
            switch (option) {
                case 1: return static_cast<int8_t>(buffer[0]);
                case 2: { int16_t v; memcpy(&v, buffer.data(), 2); return v; }
                case 3: { int32_t v; memcpy(&v, buffer.data(), 4); return v; }
                case 4: { int64_t v; memcpy(&v, buffer.data(), 8); return v; }
                case 5: { double v; memcpy(&v, buffer.data(), 8); return static_cast<int64_t>(v); }
                default: return std::nullopt;
            }
        } catch (...) { return std::nullopt; }
    }

    std::string read_string_memory(uintptr_t address_read, uintptr_t offset) {
        uintptr_t addr = Addresses::base_address + address_read + offset;
        char buf[32] = {};
        if (!rpm(reinterpret_cast<LPCVOID>(addr), buf, 31)) return "";
        return std::string(buf);
    }

    std::optional<int64_t> read_pointer_address(uintptr_t address_read, const std::vector<uintptr_t>& offsets, int option) {
        try {
            uintptr_t addr = Addresses::base_address + address_read;
            int buf_size = (option == 6 || option == 7) ? 64 : pointer_size();
            std::vector<uint8_t> buffer(buf_size, 0);

            for (uintptr_t off : offsets) {
                if (!rpm(reinterpret_cast<LPCVOID>(addr), buffer.data(), buf_size)) return std::nullopt;
                if (buf_size == 4) {
                    int32_t v; memcpy(&v, buffer.data(), 4);
                    addr = static_cast<uintptr_t>(v) + off;
                } else {
                    int64_t v; memcpy(&v, buffer.data(), 8);
                    addr = static_cast<uintptr_t>(v) + off;
                }
            }

            if (!rpm(reinterpret_cast<LPCVOID>(addr), buffer.data(), buf_size)) return std::nullopt;
            switch (option) {
                case 1: return static_cast<int8_t>(buffer[0]);
                case 2: { int16_t v; memcpy(&v, buffer.data(), 2); return v; }
                case 3: { int32_t v; memcpy(&v, buffer.data(), 4); return v; }
                case 4: { int64_t v; memcpy(&v, buffer.data(), 8); return v; }
                case 5: { double v; memcpy(&v, buffer.data(), 8); return static_cast<int64_t>(v); }
                default: return std::nullopt;
            }
        } catch (...) { return std::nullopt; }
    }

    std::string read_string_pointer(uintptr_t address_read, const std::vector<uintptr_t>& offsets) {
        try {
            uintptr_t addr = Addresses::base_address + address_read;
            int ps = pointer_size();
            std::vector<uint8_t> buffer(64, 0);
            for (uintptr_t off : offsets) {
                if (!rpm(reinterpret_cast<LPCVOID>(addr), buffer.data(), ps)) return "";
                if (ps == 4) {
                    int32_t v; memcpy(&v, buffer.data(), 4);
                    addr = static_cast<uintptr_t>(v) + off;
                } else {
                    int64_t v; memcpy(&v, buffer.data(), 8);
                    addr = static_cast<uintptr_t>(v) + off;
                }
            }
            if (!rpm(reinterpret_cast<LPCVOID>(addr), buffer.data(), 63)) return "";
            return std::string(reinterpret_cast<char*>(buffer.data()));
        } catch (...) { return ""; }
    }

    int read_targeting_status() {
        if (Addresses::attack_address_offset.size() == 1 && Addresses::attack_address_offset[0] == static_cast<uintptr_t>(-1)) {
            auto v = read_memory_address(Addresses::attack_address, 0, Addresses::my_attack_type);
            if (v && *v > 0) return static_cast<int>(*v);
            return 0;
        }
        auto v = read_pointer_address(Addresses::attack_address, Addresses::attack_address_offset, Addresses::my_attack_type);
        return v ? static_cast<int>(*v) : 0;
    }

    std::tuple<int,int,int,int> read_my_stats() {
        auto hp  = read_pointer_address(Addresses::my_stats_address, Addresses::my_hp_offset,     Addresses::my_hp_type);
        auto mhp = read_pointer_address(Addresses::my_stats_address, Addresses::my_hp_max_offset, Addresses::my_hp_type);
        auto mp  = read_pointer_address(Addresses::my_stats_address, Addresses::my_mp_offset,     Addresses::my_mp_type);
        auto mmp = read_pointer_address(Addresses::my_stats_address, Addresses::my_mp_max_offset, Addresses::my_mp_type);
        return {
            hp  ? static_cast<int>(*hp)  : 0,
            mhp ? static_cast<int>(*mhp) : 0,
            mp  ? static_cast<int>(*mp)  : 0,
            mmp ? static_cast<int>(*mmp) : 0
        };
    }

    std::tuple<int,int,int> read_my_wpt() {
        auto x = read_pointer_address(Addresses::my_x_address, Addresses::my_x_address_offset, Addresses::my_x_type);
        auto y = read_pointer_address(Addresses::my_y_address, Addresses::my_y_address_offset, Addresses::my_y_type);
        auto z = read_pointer_address(Addresses::my_z_address, Addresses::my_z_address_offset, Addresses::my_z_type);
        return {
            x ? static_cast<int>(*x) : 0,
            y ? static_cast<int>(*y) : 0,
            z ? static_cast<int>(*z) : 0
        };
    }

    std::tuple<int,int,int,std::string,int> read_target_info() {
        uintptr_t attack_addr;

        if (Addresses::attack_address_offset.size() == 1 && Addresses::attack_address_offset[0] == static_cast<uintptr_t>(-1)) {
            auto id = read_memory_address(Addresses::attack_address, 0, Addresses::my_attack_type);
            if (!id || *id <= 0) return {0, 0, 0, "", 0};
            uintptr_t excl = Addresses::base_address + Addresses::attack_address;
            auto found = scan_memory_for_value(static_cast<uint32_t>(*id), excl);
            if (!found) return {0, 0, 0, "", 0};
            attack_addr = *found - Addresses::base_address;
        } else {
            auto v = read_memory_address(Addresses::attack_address, 0, Addresses::my_attack_type);
            if (!v) return {0, 0, 0, "", 0};
            attack_addr = static_cast<uintptr_t>(*v) - Addresses::base_address;
        }

        auto tx = read_memory_address(attack_addr, Addresses::target_x_offset, Addresses::target_x_type);
        auto ty = read_memory_address(attack_addr, Addresses::target_y_offset, Addresses::target_y_type);
        auto tz = read_memory_address(attack_addr, Addresses::target_z_offset, Addresses::target_z_type);
        auto thp = read_memory_address(attack_addr, Addresses::target_hp_offset, Addresses::target_hp_type);
        std::string name = read_string_memory(attack_addr, Addresses::target_name_offset);

        return {
            tx  ? static_cast<int>(*tx)  : 0,
            ty  ? static_cast<int>(*ty)  : 0,
            tz  ? static_cast<int>(*tz)  : 0,
            name,
            thp ? static_cast<int>(*thp) : 0
        };
    }

    std::optional<uintptr_t> scan_memory_for_value(uint32_t value, uintptr_t exclude_address) {
        try {
            uintptr_t current = 0;
            MEMORY_BASIC_INFORMATION mbi;
            uint8_t vbytes[4];
            memcpy(vbytes, &value, 4);
            uintptr_t max_addr = (Addresses::application_architecture == 32) ? 0x7FFFFFFF : 0x7FFFFFFFFFFFllu;

            while (current < max_addr) {
                if (!VirtualQueryEx(Addresses::process_handle, reinterpret_cast<LPCVOID>(current), &mbi, sizeof(mbi)))
                    break;
                if (mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
                    SIZE_T region = mbi.RegionSize;
                    if (region < 100 * 1024 * 1024) {
                        std::vector<uint8_t> buf(region);
                        SIZE_T read = 0;
                        if (ReadProcessMemory(Addresses::process_handle, mbi.BaseAddress, buf.data(), region, &read)) {
                            for (SIZE_T i = 0; i + 4 <= read; i++) {
                                if (memcmp(buf.data() + i, vbytes, 4) == 0) {
                                    uintptr_t found = current + i;
                                    if (found != exclude_address) return found;
                                }
                            }
                        }
                    }
                }
                current += mbi.RegionSize;
            }
        } catch (...) {}
        return std::nullopt;
    }
}
