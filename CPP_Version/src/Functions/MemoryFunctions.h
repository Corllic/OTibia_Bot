#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <tuple>

namespace MemoryFunctions {

    struct MEMORY_BASIC_INFO_EX {
        PVOID BaseAddress;
        PVOID AllocationBase;
        DWORD AllocationProtect;
        SIZE_T RegionSize;
        DWORD State;
        DWORD Protect;
        DWORD Type;
    };

    std::optional<int64_t> read_memory_address(uintptr_t address_read, uintptr_t offset, int option);
    std::optional<int64_t> read_pointer_address(uintptr_t address_read, const std::vector<uintptr_t>& offsets, int option);
    std::string read_string_memory(uintptr_t address_read, uintptr_t offset);
    std::string read_string_pointer(uintptr_t address_read, const std::vector<uintptr_t>& offsets);

    int read_targeting_status();
    std::tuple<int,int,int,int> read_my_stats();
    std::tuple<int,int,int> read_my_wpt();
    std::tuple<int,int,int,std::string,int> read_target_info();

    std::optional<uintptr_t> scan_memory_for_value(uint32_t value, uintptr_t exclude_address = 0);
}
