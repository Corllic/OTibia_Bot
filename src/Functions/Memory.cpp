#include "Memory.h"
#include "../Core/Addresses.h"
#include "../Core/Logger.h"
#include <sstream>
#include <iomanip>

namespace Memory {

static size_t ptr_size() {
    return Addresses::application_architecture == 64 ? 8 : 4;
}

static bool rpm(uintptr_t addr, void* out, size_t size) {
    SIZE_T n = 0;
    return ReadProcessMemory(Addresses::process_handle,
                             reinterpret_cast<LPCVOID>(addr),
                             out, size, &n) && n == size;
}

static std::optional<uintptr_t> deref_ptr(uintptr_t addr) {
    uintptr_t val = 0;
    if (!rpm(addr, &val, ptr_size())) return std::nullopt;
    if (val == 0) return std::nullopt;
    return val;
}

static std::string hex(uintptr_t v) {
    std::ostringstream ss;
    ss << "0x" << std::hex << v;
    return ss.str();
}

static int type_to_bytes(int type) {
    switch (type) {
        case 1: return 1;
        case 2: return 2;
        case 3: return 4;
        default: return 4;
    }
}

std::optional<int64_t> read(uintptr_t offset_from_base,
                             const std::vector<uintptr_t>& offsets,
                             int size_bytes)
{
    size_bytes = type_to_bytes(size_bytes);
    if (!Addresses::process_handle) {
        Logger::log("[Memory] process_handle is null");
        return std::nullopt;
    }
    if (!Addresses::base_address) {
        Logger::log("[Memory] base_address is 0");
        return std::nullopt;
    }

    uintptr_t current = Addresses::base_address + offset_from_base;
    Logger::log("[Memory] arch=" + std::to_string(Addresses::application_architecture)
                + " base=" + hex(Addresses::base_address)
                + " offset=" + hex(offset_from_base)
                + " start=" + hex(current));

    if (offsets.empty()) {
        int64_t v = 0;
        if (!rpm(current, &v, (size_t)size_bytes)) {
            Logger::log("[Memory] rpm failed (no offsets) err=" + std::to_string(GetLastError()));
            return std::nullopt;
        }
        if (size_bytes == 2) v = (int64_t)(int16_t)(v & 0xFFFF);
        else if (size_bytes == 1) v = (int64_t)(int8_t)(v & 0xFF);
        else v = (int64_t)(int32_t)(v & 0xFFFFFFFF);
        Logger::log("[Memory] value=" + std::to_string(v));
        return v;
    }

    auto ptr = deref_ptr(current);
    if (!ptr) {
        Logger::log("[Memory] initial deref failed at " + hex(current) + " err=" + std::to_string(GetLastError()));
        return std::nullopt;
    }
    Logger::log("[Memory] initial deref " + hex(current) + " -> " + hex(*ptr));
    current = *ptr;

    for (size_t i = 0; i < offsets.size() - 1; ++i) {
        uintptr_t ptr_addr = current + offsets[i];
        auto next = deref_ptr(ptr_addr);
        if (!next) {
            Logger::log("[Memory] deref[" + std::to_string(i) + "] failed at " + hex(ptr_addr) + " err=" + std::to_string(GetLastError()));
            return std::nullopt;
        }
        Logger::log("[Memory] deref[" + std::to_string(i) + "] " + hex(ptr_addr) + " -> " + hex(*next));
        current = *next;
    }

    uintptr_t final_addr = current + offsets.back();
    int64_t v = 0;
    if (!rpm(final_addr, &v, (size_t)size_bytes)) {
        Logger::log("[Memory] rpm failed at " + hex(final_addr) + " err=" + std::to_string(GetLastError()));
        return std::nullopt;
    }
    if (size_bytes == 2) v = (int64_t)(int16_t)(v & 0xFFFF);
    else if (size_bytes == 1) v = (int64_t)(int8_t)(v & 0xFF);
    else v = (int64_t)(int32_t)(v & 0xFFFFFFFF);
    Logger::log("[Memory] final " + hex(final_addr) + " value=" + std::to_string(v));
    return v;
}

}
