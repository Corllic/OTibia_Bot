#include "CreatureScanner.h"
#include "../Core/Addresses.h"
#include <windows.h>
#include <cstring>
#include <string>
#include <cwchar>
#include <algorithm>

namespace CreatureScanner {

std::atomic<int> scan_progress_pct{0};

static uintptr_t scan_end() {
    return (Addresses::application_architecture == 64)
        ? (uintptr_t)0x7FFFFFFFFFFull
        : (uintptr_t)0x7FFFFFFFull;
}

static int ptr_size() {
    return (Addresses::application_architecture == 64) ? 8 : 4;
}

static std::string utf16_to_utf8(const wchar_t* src, int len) {
    if (!src || len <= 0) return "";
    int out_len = WideCharToMultiByte(CP_UTF8, 0, src, len, nullptr, 0, nullptr, nullptr);
    if (out_len <= 0) return "";
    std::string out(out_len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, src, len, out.data(), out_len, nullptr, nullptr);
    return out;
}

static bool validate_ascii_name(const char* name, int len) {
    if (len < 3 || len > 64) return false;
    if (name[0] < 'A' || name[0] > 'Z') return false;
    for (int c = 0; c < len; ++c) {
        unsigned char ch = (unsigned char)name[c];
        bool ok = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                  ch == ' ' || ch == '-' || ch == '\'';
        if (!ok) return false;
    }
    return true;
}

static bool validate_wname(const wchar_t* wname, int len) {
    if (len < 3 || len > 32) return false;
    if (wname[0] < L'A' || wname[0] > L'Z') return false;
    for (int c = 0; c < len; ++c) {
        wchar_t ch = wname[c];
        bool ok = (ch >= L'A' && ch <= L'Z') ||
                  (ch >= L'a' && ch <= L'z') ||
                  ch == L' ' || ch == L'-' || ch == L'\'';
        if (!ok) return false;
    }
    return true;
}

struct NameAddressEntry {
    uintptr_t addr;
    bool is_unicode;
};

static std::vector<NameAddressEntry> find_name_in_memory(
    const std::string& hint_name,
    uintptr_t SCAN_START, uintptr_t SCAN_END,
    int progress_base, int progress_range)
{
    std::vector<NameAddressEntry> results;
    constexpr SIZE_T BLOCK = 4 * 1024 * 1024;
    std::vector<uint8_t> buf(BLOCK);

    std::wstring whint(hint_name.begin(), hint_name.end());
    SIZE_T whint_bytes = whint.size() * sizeof(wchar_t);

    uintptr_t total = SCAN_END - SCAN_START;

    for (uintptr_t base = SCAN_START; base < SCAN_END; ) {
        scan_progress_pct = progress_base + static_cast<int>(
            (double)(base - SCAN_START) / (double)total * progress_range);

        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQueryEx(Addresses::process_handle,
                            reinterpret_cast<LPCVOID>(base), &mbi, sizeof(mbi))) {
            base += 4096; continue;
        }
        uintptr_t region_end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
            base = region_end; continue;
        }

        uintptr_t pos = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        while (pos < region_end) {
            SIZE_T to_read = BLOCK;
            if (pos + to_read > region_end) to_read = region_end - pos;
            SIZE_T actually_read = 0;
            if (!ReadProcessMemory(Addresses::process_handle,
                                   reinterpret_cast<LPCVOID>(pos),
                                   buf.data(), to_read, &actually_read) || actually_read == 0) {
                pos += to_read; continue;
            }

            for (SIZE_T i = 0; i + hint_name.size() < actually_read; i++) {
                if (buf[i] == (uint8_t)hint_name[0]) {
                    if (std::memcmp(buf.data() + i, hint_name.c_str(), hint_name.size()) == 0) {
                        unsigned char next = buf[i + hint_name.size()];
                        if (next == 0 || !isalnum(next))
                            results.push_back({pos + i, false});
                    }
                }
                if (i + whint_bytes < actually_read &&
                    buf[i] == (uint8_t)whint[0] && buf[i+1] == 0) {
                    if (std::memcmp(buf.data() + i, whint.c_str(), whint_bytes) == 0) {
                        uint16_t next = 0;
                        if (i + whint_bytes + 1 < actually_read)
                            std::memcpy(&next, buf.data() + i + whint_bytes, 2);
                        if (next == 0 || !isalnum((unsigned char)next))
                            results.push_back({pos + i, true});
                    }
                }
            }
            pos += actually_read;
        }
        base = region_end;
    }
    return results;
}

bool detect_name_offset(int player_x, int player_y, int player_z,
                         const std::string& hint_name) {
    if (!Addresses::process_handle || hint_name.empty()) return false;

    constexpr SIZE_T    BLOCK      = 4 * 1024 * 1024;
    constexpr uintptr_t SCAN_START = 0x00010000;
    const     uintptr_t SCAN_END   = scan_end();
    const     int       PS         = ptr_size();

    const int min_x = player_x - 7;
    const int max_x = player_x + 7;
    const int min_y = player_y - 5;
    const int max_y = player_y + 5;

    const uintptr_t x_off = Addresses::creature_x_off;
    const uintptr_t y_off = Addresses::creature_y_off;
    const uintptr_t z_off = Addresses::creature_z_off;

    auto name_entries = find_name_in_memory(hint_name, SCAN_START, SCAN_END, 0, 50);
    if (name_entries.empty()) return false;

    std::vector<uintptr_t> name_addresses;
    for (auto& e : name_entries) name_addresses.push_back(e.addr);

    std::vector<uint8_t> buf(BLOCK);

    for (uintptr_t name_addr : name_addresses) {
        for (uintptr_t base2 = SCAN_START; base2 < SCAN_END; ) {
            MEMORY_BASIC_INFORMATION mbi{};
            if (!VirtualQueryEx(Addresses::process_handle,
                                reinterpret_cast<LPCVOID>(base2), &mbi, sizeof(mbi))) {
                base2 += 4096; continue;
            }
            uintptr_t region_end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
            if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
                base2 = region_end; continue;
            }

            uintptr_t pos = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
            while (pos < region_end) {
                SIZE_T to_read = BLOCK;
                if (pos + to_read > region_end) to_read = region_end - pos;
                SIZE_T actually_read = 0;
                if (!ReadProcessMemory(Addresses::process_handle,
                                       reinterpret_cast<LPCVOID>(pos),
                                       buf.data(), to_read, &actually_read) || actually_read == 0) {
                    pos += to_read; continue;
                }

                for (SIZE_T i = 0; i + (SIZE_T)PS <= actually_read; i += (SIZE_T)PS) {
                    uintptr_t val = 0;
                    std::memcpy(&val, buf.data() + i, PS);
                    if (val != name_addr) continue;

                    uintptr_t ptr_addr = pos + i;

                    for (intptr_t delta = -0x200; delta <= 0; delta += 4) {
                        uintptr_t candidate = ptr_addr + (uintptr_t)delta;
                        if (candidate < SCAN_START) continue;

                        uint8_t tmp[8] = {};
                        SIZE_T rd = 0;

                        if (!ReadProcessMemory(Addresses::process_handle,
                                               reinterpret_cast<LPCVOID>(candidate + x_off),
                                               tmp, 4, &rd) || rd != 4) continue;
                        int32_t cx; std::memcpy(&cx, tmp, 4);
                        if (cx < min_x || cx > max_x) continue;

                        if (!ReadProcessMemory(Addresses::process_handle,
                                               reinterpret_cast<LPCVOID>(candidate + y_off),
                                               tmp, 4, &rd) || rd != 4) continue;
                        int32_t cy; std::memcpy(&cy, tmp, 4);
                        if (cy < min_y || cy > max_y) continue;

                        if (!ReadProcessMemory(Addresses::process_handle,
                                               reinterpret_cast<LPCVOID>(candidate + z_off),
                                               tmp, 4, &rd) || rd != 4) continue;
                        int32_t cz4; std::memcpy(&cz4, tmp, 4);
                        int16_t cz2; std::memcpy(&cz2, tmp, 2);
                        int32_t cz = (cz4 == player_z) ? cz4 : (int32_t)cz2;
                        if (cz != player_z) continue;

                        intptr_t name_off_signed = (intptr_t)(ptr_addr - candidate);
                        if (name_off_signed < 0 || name_off_signed > 0x400) continue;

                        Addresses::creature_name_off     = (uintptr_t)name_off_signed;
                        Addresses::creature_name_len_off = (name_off_signed >= 4)
                                                           ? (uintptr_t)(name_off_signed - 4) : 0;
                        return true;
                    }
                }
                pos += actually_read;
            }
            base2 = region_end;
        }
    }

    return false;
}

static uintptr_t find_xyz_with_name(
    int cx, int cy, int cz,
    const std::string& hint_name,
    uintptr_t SCAN_START, uintptr_t SCAN_END, int PS,
    uintptr_t& out_ptr_abs, bool& out_is_unicode)
{
    constexpr SIZE_T BLOCK = 4 * 1024 * 1024;
    std::wstring whint(hint_name.begin(), hint_name.end());
    SIZE_T whint_bytes = whint.size() * sizeof(wchar_t);
    std::vector<uint8_t> buf(BLOCK);

    for (uintptr_t base = SCAN_START; base < SCAN_END; ) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQueryEx(Addresses::process_handle,
                            reinterpret_cast<LPCVOID>(base), &mbi, sizeof(mbi))) {
            base += 4096; continue;
        }
        uintptr_t region_end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
            base = region_end; continue;
        }

        uintptr_t pos = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        while (pos < region_end) {
            SIZE_T to_read = BLOCK;
            if (pos + to_read > region_end) to_read = region_end - pos;
            SIZE_T actually_read = 0;
            if (!ReadProcessMemory(Addresses::process_handle,
                                   reinterpret_cast<LPCVOID>(pos),
                                   buf.data(), to_read, &actually_read) || actually_read == 0) {
                pos += to_read; continue;
            }

            for (SIZE_T i = 0; i + 10 <= actually_read; i += 4) {
                int32_t vx = 0, vy = 0; int16_t vz = 0;
                std::memcpy(&vx, buf.data() + i,     4);
                std::memcpy(&vy, buf.data() + i + 4, 4);
                std::memcpy(&vz, buf.data() + i + 8, 2);
                if (vx != cx || vy != cy || (int32_t)vz != cz) continue;

                uintptr_t xyz_addr = pos + i;

                constexpr SIZE_T BACK  = 0x100;
                constexpr SIZE_T FWD   = 0x200;
                constexpr SIZE_T TOTAL = BACK + FWD;
                uint8_t sregion[TOTAL] = {};
                SIZE_T rd = 0;
                uintptr_t rbase = (xyz_addr > BACK) ? xyz_addr - BACK : SCAN_START;
                if (!ReadProcessMemory(Addresses::process_handle,
                                       reinterpret_cast<LPCVOID>(rbase),
                                       sregion, TOTAL, &rd) || rd < 10) continue;

                for (SIZE_T pi = 0; pi + (SIZE_T)PS <= rd; pi += 4) {
                    uintptr_t ptr_val = 0;
                    std::memcpy(&ptr_val, sregion + pi, (SIZE_T)PS);
                    if (ptr_val < 0x10000) continue;

                    uint8_t pname[128] = {};
                    SIZE_T rd2 = 0;
                    if (!ReadProcessMemory(Addresses::process_handle,
                                           reinterpret_cast<LPCVOID>(ptr_val),
                                           pname, 128, &rd2) || rd2 == 0) continue;

                    bool found = false;
                    if (rd2 >= whint_bytes &&
                        std::memcmp(pname, whint.c_str(), whint_bytes) == 0) {
                        out_is_unicode = true; found = true;
                    } else if (rd2 >= hint_name.size() &&
                               std::memcmp(pname, hint_name.c_str(), hint_name.size()) == 0) {
                        out_is_unicode = false; found = true;
                    }
                    if (!found) continue;
                    out_ptr_abs = rbase + pi;
                    return xyz_addr;
                }
            }
            pos += actually_read;
        }
        base = region_end;
    }
    return 0;
}

bool detect_all_offsets(int cx1, int cy1, int cz1, const std::string& name1,
                         int cx2, int cy2, int cz2, const std::string& name2) {
    if (!Addresses::process_handle || name1.empty() || name2.empty()) return false;

    constexpr uintptr_t SCAN_START = 0x00010000;
    const     uintptr_t SCAN_END   = scan_end();
    const     int       PS         = ptr_size();

    scan_progress_pct = 0;

    uintptr_t ptr_abs_A = 0; bool is_unicode_A = false;
    uintptr_t xyz_addr_A = find_xyz_with_name(cx1, cy1, cz1, name1,
                                               SCAN_START, SCAN_END, PS,
                                               ptr_abs_A, is_unicode_A);
    scan_progress_pct = 50;
    if (xyz_addr_A == 0) { scan_progress_pct = 100; return false; }

    uintptr_t ptr_abs_B = 0; bool is_unicode_B = false;
    uintptr_t xyz_addr_B = find_xyz_with_name(cx2, cy2, cz2, name2,
                                               SCAN_START, SCAN_END, PS,
                                               ptr_abs_B, is_unicode_B);
    scan_progress_pct = 100;
    if (xyz_addr_B == 0) return false;

    for (uintptr_t back = 0; back <= 0x100; back += 4) {
        uintptr_t cand_A = xyz_addr_A - back;
        uintptr_t cand_B = xyz_addr_B - back;
        uint32_t id_A = 0, id_B = 0;
        SIZE_T rd3 = 0, rd4 = 0;
        if (!ReadProcessMemory(Addresses::process_handle,
                               reinterpret_cast<LPCVOID>(cand_A),
                               &id_A, 4, &rd3) || rd3 != 4) continue;
        if (!ReadProcessMemory(Addresses::process_handle,
                               reinterpret_cast<LPCVOID>(cand_B),
                               &id_B, 4, &rd4) || rd4 != 4) continue;
        if (id_A < 100000 || id_B < 100000) continue;
        if (id_A == id_B) continue;

        uint32_t div = 1;
        uint32_t tmp = id_A;
        while (tmp >= 1000) { tmp /= 10; div *= 10; }
        if (div == 0) continue;
        if (id_A / div != id_B / div) continue;

        uintptr_t x_off        = back;
        uintptr_t struct_base_A = xyz_addr_A - x_off;
        uintptr_t name_off     = (ptr_abs_A > struct_base_A) ? ptr_abs_A - struct_base_A : 0;
        uint32_t  prefix       = id_A / div;

        Addresses::creature_id_prefix       = prefix;
        Addresses::creature_id_prefix_div   = div;
        Addresses::creature_x_off           = x_off;
        Addresses::creature_y_off           = x_off + 4;
        Addresses::creature_z_off           = x_off + 8;
        Addresses::creature_name_off        = name_off;
        Addresses::creature_name_len_off    = (name_off >= 4) ? name_off - 4 : 0;
        Addresses::creature_name_is_unicode = is_unicode_A;
        Addresses::creature_name_is_ptr     = true;
        Addresses::creature_found_address   = struct_base_A;
        return true;
    }

    return false;
}

std::vector<Creature> scan(int player_x, int player_y, int player_z,
                            int range_x, int range_y) {
    std::vector<Creature> result;

    if (!Addresses::process_handle) return result;

    constexpr SIZE_T    BLOCK     = 4 * 1024 * 1024;
    constexpr uintptr_t SCAN_START = 0x00010000;
    const     uintptr_t SCAN_END   = scan_end();
    const     int       PS         = ptr_size();

    const uintptr_t x_off        = Addresses::creature_x_off;
    const uintptr_t y_off        = Addresses::creature_y_off;
    const uintptr_t z_off        = Addresses::creature_z_off;
    const uintptr_t name_off     = Addresses::creature_name_off;
    const bool      name_unicode = Addresses::creature_name_is_unicode;
    const bool      name_is_ptr  = Addresses::creature_name_is_ptr;

    const SIZE_T min_struct = (name_off > x_off ? name_off : x_off) + 64;

    struct XYPattern {
        uint8_t bytes[8];
        int32_t cx, cy;
    };
    std::vector<XYPattern> patterns;
    patterns.reserve((size_t)(2 * range_x + 1) * (2 * range_y + 1));
    for (int dx = -range_x; dx <= range_x; dx++) {
        for (int dy = -range_y; dy <= range_y; dy++) {
            int32_t cx = player_x + dx;
            int32_t cy = player_y + dy;
            if (cx == player_x && cy == player_y) continue;
            XYPattern p;
            std::memcpy(p.bytes,     &cx, 4);
            std::memcpy(p.bytes + 4, &cy, 4);
            p.cx = cx; p.cy = cy;
            patterns.push_back(p);
        }
    }
    if (patterns.empty()) return result;

    std::vector<uint8_t> buf(BLOCK);
    scan_progress_pct = 0;

    for (uintptr_t base = SCAN_START; base < SCAN_END; ) {
        scan_progress_pct = static_cast<int>(
            (double)(base - SCAN_START) / (double)(SCAN_END - SCAN_START) * 100.0);

        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQueryEx(Addresses::process_handle,
                            reinterpret_cast<LPCVOID>(base), &mbi, sizeof(mbi))) {
            base += 4096; continue;
        }

        uintptr_t region_end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;

        if (mbi.State != MEM_COMMIT ||
            (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) ||
            mbi.Type != MEM_PRIVATE) {
            base = region_end; continue;
        }

        uintptr_t pos = reinterpret_cast<uintptr_t>(mbi.BaseAddress);

        while (pos < region_end) {
            SIZE_T to_read = BLOCK;
            if (pos + to_read > region_end) to_read = region_end - pos;

            SIZE_T actually_read = 0;
            if (!ReadProcessMemory(Addresses::process_handle,
                                   reinterpret_cast<LPCVOID>(pos),
                                   buf.data(), to_read, &actually_read) || actually_read == 0) {
                pos += to_read; continue;
            }

            const uint8_t* data = buf.data();

            const uint32_t id_prefix  = Addresses::creature_id_prefix;
            const uint32_t id_div     = Addresses::creature_id_prefix_div;
            const bool     use_prefix = (id_prefix > 0 && id_div > 0);

            for (SIZE_T i = 0; i + min_struct <= actually_read; i += 4) {
                if (use_prefix) {
                    if (i + 4 > actually_read) continue;
                    uint32_t id = 0;
                    std::memcpy(&id, data + i, 4);
                    if (id / id_div != id_prefix) continue;
                }

                int32_t cx, cy;
                std::memcpy(&cx, data + i + x_off, 4);
                std::memcpy(&cy, data + i + y_off, 4);

                bool matched = false;
                for (auto& p : patterns) {
                    if (p.cx == cx && p.cy == cy) { matched = true; break; }
                }
                if (!matched) continue;

                int32_t cz4 = 0; int16_t cz2 = 0;
                std::memcpy(&cz4, data + i + z_off, 4);
                std::memcpy(&cz2, data + i + z_off, 2);
                int32_t cz = (cz4 == player_z) ? cz4 : static_cast<int32_t>(cz2);
                if (cz != player_z) continue;

                if (i + name_off + 64 > actually_read) continue;

                std::string name;
                const uint8_t* np = data + i + name_off;

                if (name_is_ptr) {
                    uintptr_t str_ptr = 0;
                    std::memcpy(&str_ptr, np, PS);
                    if (str_ptr > 0x10000) {
                        SIZE_T rd2 = 0;
                        if (name_unicode) {
                            wchar_t wname[33] = {};
                            if (ReadProcessMemory(Addresses::process_handle,
                                                  reinterpret_cast<LPCVOID>(str_ptr),
                                                  wname, 64, &rd2) && rd2 > 0) {
                                wname[32] = L'\0';
                                int wlen = 0;
                                while (wlen < 32 && wname[wlen] != L'\0') wlen++;
                                if (wlen > 0 && validate_wname(wname, wlen))
                                    name = utf16_to_utf8(wname, wlen);
                            }
                        } else {
                            char aname[65] = {};
                            if (ReadProcessMemory(Addresses::process_handle,
                                                  reinterpret_cast<LPCVOID>(str_ptr),
                                                  aname, 64, &rd2) && rd2 > 0) {
                                aname[64] = '\0';
                                int alen = 0;
                                while (alen < 64 && aname[alen] != '\0') alen++;
                                if (alen > 0 && validate_ascii_name(aname, alen))
                                    name = std::string(aname, alen);
                            }
                        }
                    }
                } else {
                    if (name_unicode) {
                        wchar_t wname[33] = {};
                        std::memcpy(wname, np, 64);
                        wname[32] = L'\0';
                        int wlen = 0;
                        while (wlen < 32 && wname[wlen] != L'\0') wlen++;
                        if (wlen > 0 && validate_wname(wname, wlen))
                            name = utf16_to_utf8(wname, wlen);
                    } else {
                        char aname[65] = {};
                        std::memcpy(aname, np, 64);
                        aname[64] = '\0';
                        int alen = 0;
                        while (alen < 64 && aname[alen] != '\0') alen++;
                        if (alen > 0 && validate_ascii_name(aname, alen))
                            name = std::string(aname, alen);
                    }
                }

                if (!name.empty()) {
                    bool dupe = false;
                    for (auto& r : result) {
                        if (r.x == cx && r.y == cy && r.z == cz && r.name == name) {
                            dupe = true; break;
                        }
                    }
                    if (!dupe)
                        result.push_back({cx, cy, cz, name, pos + i});
                }

                i += 4;
            }

            pos += actually_read;
        }

        base = region_end;
    }

    return result;
}

}
