#include "CreatureScanner.h"
#include "../Core/Addresses.h"
#include "../Core/Logger.h"
#include <windows.h>
#include <algorithm>
#include <optional>
#include <unordered_set>

namespace CreatureScanner {

static constexpr int RANGE_X = 7;
static constexpr int RANGE_Y = 5;

static constexpr uintptr_t OFF_X    = 0x38;
static constexpr uintptr_t OFF_Y    = 0x3C;
static constexpr uintptr_t OFF_Z    = 0x40;
static constexpr uintptr_t OFF_NAME = 0x108;
static constexpr uintptr_t OFF_HP   = 0x148;

static constexpr size_t MIN_REGION_SIZE = 64 * 1024;

std::atomic<int>      scan_progress_pct{0};
std::mutex            creatures_mutex;
std::vector<Creature> known_creatures;

struct Region {
    uintptr_t base;
    size_t    size;
};

static std::mutex           regions_mutex;
static std::vector<Region>  hot_regions;
static bool                 regions_discovered = false;
static int                  scan_counter       = 0;
static constexpr int        FULL_SCAN_EVERY    = 10;

static bool rpm(uintptr_t addr, void* out, size_t sz) {
    SIZE_T n = 0;
    return ReadProcessMemory(Addresses::process_handle,
                             reinterpret_cast<LPCVOID>(addr),
                             out, sz, &n) && n == sz;
}

static std::string read_name(uintptr_t obj) {
    uintptr_t field = 0;
    if (!rpm(obj + OFF_NAME, &field, 8)) return "";
    if (field == 0) return "";

    uint32_t hi = static_cast<uint32_t>(field >> 32);
    if (hi == 0x00007FF6) {
        char buf[64] = {};
        if (!rpm(field, buf, 63)) return "";
        std::string s(buf);
        auto nul = s.find('\0');
        if (nul != std::string::npos) s = s.substr(0, nul);
        if (!s.empty() && std::all_of(s.begin(), s.end(),
            [](unsigned char c){ return c >= 0x20 && c < 0x7F; }))
            return s;
        return "";
    }

    char buf[64] = {};
    memcpy(buf, &field, 8);
    rpm(obj + OFF_NAME + 8, buf + 8, 55);
    std::string s(buf);
    auto nul = s.find('\0');
    if (nul != std::string::npos) s = s.substr(0, nul);
    if (!s.empty() && std::all_of(s.begin(), s.end(),
        [](unsigned char c){ return c >= 0x20 && c < 0x7F; }))
        return s;
    return "";
}

static bool is_valid_id(uintptr_t id) {
    return static_cast<uint32_t>(id >> 32) == 0x00007FF6;
}

static std::vector<Region> enumerate_candidate_regions() {
    std::vector<Region> result;
    MEMORY_BASIC_INFORMATION mbi{};
    uintptr_t addr = 0;
    while (VirtualQueryEx(Addresses::process_handle,
                          reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi))) {
        uintptr_t region_base = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        if (mbi.State == MEM_COMMIT &&
            mbi.Type == MEM_PRIVATE &&
            (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE)) &&
            !(mbi.Protect & PAGE_GUARD) &&
            mbi.RegionSize >= MIN_REGION_SIZE)
        {
            result.push_back({region_base, mbi.RegionSize});
        }
        addr = region_base + mbi.RegionSize;
    }
    return result;
}

static std::vector<Creature> scan_regions(
    const std::vector<Region>& regions,
    int player_x, int player_y, int player_z,
    std::vector<Region>* hit_regions_out)
{
    std::unordered_set<int32_t> valid_x;
    for (int dx = -RANGE_X; dx <= RANGE_X; ++dx)
        valid_x.insert(player_x + dx);

    std::unordered_set<int32_t> valid_y;
    for (int dy = -RANGE_Y; dy <= RANGE_Y; ++dy)
        valid_y.insert(player_y + dy);

    std::vector<Creature> found;
    std::unordered_set<uintptr_t> seen;

    size_t total = regions.size();
    size_t done  = 0;

    for (auto& reg : regions) {
        std::vector<uint8_t> buf(reg.size);
        SIZE_T n = 0;
        if (ReadProcessMemory(Addresses::process_handle,
                              reinterpret_cast<LPCVOID>(reg.base),
                              buf.data(), reg.size, &n) && n >= (OFF_X + 10))
        {
            bool region_hit = false;
            for (size_t off = OFF_X; off + 10 <= n; off += 8) {
                int32_t vx;
                memcpy(&vx, buf.data() + off, 4);
                if (!valid_x.count(vx)) continue;

                int32_t vy;
                memcpy(&vy, buf.data() + off + 4, 4);
                if (!valid_y.count(vy)) continue;

                int16_t vz;
                memcpy(&vz, buf.data() + off + 8, 2);
                if (static_cast<int>(vz) != player_z) continue;

                size_t id_off = off - OFF_X;
                uint64_t id;
                memcpy(&id, buf.data() + id_off, 8);
                if (!is_valid_id(id)) continue;

                uintptr_t obj = reg.base + id_off;
                if (!seen.insert(obj).second) continue;

                std::string name = read_name(obj);
                if (name.empty()) continue;

                uint8_t hp = 0;
                rpm(obj + OFF_HP, &hp, 1);
                if (hp == 0) continue;

                Creature c;
                c.address = obj;
                c.x       = vx;
                c.y       = vy;
                c.z       = static_cast<int>(vz);
                c.name    = name;
                c.hp_pct  = static_cast<int>(hp);
                found.push_back(c);
                region_hit = true;
            }
            if (region_hit && hit_regions_out)
                hit_regions_out->push_back(reg);
        }
        done++;
        scan_progress_pct.store(static_cast<int>(done * 100 / total));
    }

    return found;
}

std::vector<Creature> scan_map(int player_x, int player_y, int player_z) {
    if (!Addresses::process_handle) return {};
    scan_progress_pct.store(0);

    std::vector<Region> regions_to_scan;
    bool need_full = false;

    {
        std::lock_guard<std::mutex> lk(regions_mutex);
        scan_counter++;
        if (!regions_discovered || hot_regions.empty() || scan_counter % FULL_SCAN_EVERY == 0)
            need_full = true;
        else
            regions_to_scan = hot_regions;
    }

    if (need_full)
        regions_to_scan = enumerate_candidate_regions();

    std::vector<Region> hit;
    auto found = scan_regions(regions_to_scan, player_x, player_y, player_z,
                              need_full ? &hit : nullptr);

    if (need_full) {
        std::lock_guard<std::mutex> lk(regions_mutex);
        hot_regions        = hit;
        regions_discovered = true;
    }

    scan_progress_pct.store(100);
    {
        std::lock_guard<std::mutex> lk(creatures_mutex);
        known_creatures = found;
    }
    return found;
}

void reset_regions() {
    std::lock_guard<std::mutex> lk(regions_mutex);
    hot_regions.clear();
    regions_discovered = false;
}

void refresh_known() {
    std::vector<Creature> updated;
    {
        std::lock_guard<std::mutex> lk(creatures_mutex);
        updated = known_creatures;
    }
    for (auto& c : updated) {
        int32_t x = 0, y = 0;
        int16_t z = 0;
        uint8_t hp = 0;
        rpm(c.address + OFF_X,  &x,  4);
        rpm(c.address + OFF_Y,  &y,  4);
        rpm(c.address + OFF_Z,  &z,  2);
        rpm(c.address + OFF_HP, &hp, 1);
        c.x      = x;
        c.y      = y;
        c.z      = static_cast<int>(z);
        c.hp_pct = static_cast<int>(hp);
    }
    {
        std::lock_guard<std::mutex> lk(creatures_mutex);
        known_creatures = updated;
    }
}

}
