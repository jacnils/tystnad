#pragma once

#include <string>

#if MACOS
std::string get_executable_path();
std::string get_home_dir();
void write_launch_agent(const std::string& app_path);
void remove_launch_agent();
#endif