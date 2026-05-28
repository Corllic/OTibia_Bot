#pragma once
#include <windows.h>
#include <vector>
#include <cstdint>
#include <optional>

namespace Memory {

std::optional<int64_t> read(uintptr_t offset_from_base,
                             const std::vector<uintptr_t>& offsets = {},
                             int type = 3);

}
