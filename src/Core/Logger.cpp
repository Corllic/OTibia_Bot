#include "Logger.h"
#include "Addresses.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <sstream>

namespace Logger {

static std::ofstream log_file;

static std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
    return std::string(buf);
}

void log(const std::string& msg) {
    if (Addresses::log_level == 0) return;

    std::string line = "[" + timestamp() + "] " + msg;

    if (Addresses::log_level == 1 || Addresses::log_level == 3)
        std::cout << line << "\n";

    if ((Addresses::log_level == 2 || Addresses::log_level == 3) && log_file.is_open())
        log_file << line << "\n" << std::flush;
}

void open_file(const std::string& path) {
    log_file.open(path, std::ios::app);
}

void close_file() {
    if (log_file.is_open()) log_file.close();
}

}
