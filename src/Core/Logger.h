#pragma once
#include <string>

namespace Logger {

void log(const std::string& msg);
void open_file(const std::string& path = "logs.txt");
void close_file();

}
