#include <string>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <unistd.h>
#include <pwd.h>
#include <launch_agent.hpp>

#if MACOS
#include <mach-o/dyld.h>

std::string get_executable_path() {
	char pathbuf[PATH_MAX];
	uint32_t bufsize = sizeof(pathbuf);

	if (_NSGetExecutablePath(pathbuf, &bufsize) == 0) {
		return {(pathbuf)};
	} else {
		return {};
	}
}

std::string get_home_dir() {
	if (const char* home = getenv("HOME")) {
		return {(home)};
	}
	struct passwd* pw = getpwuid(getuid());
	return pw ? std::string(pw->pw_dir) : "";
}

void write_launch_agent(const std::string& app_path) {
	std::string home_dir = get_home_dir();
	if (home_dir.empty()) {
		throw std::runtime_error{"Failed to get home directory"};
	}

	std::filesystem::path launch_agent_dir = std::filesystem::path(home_dir) / "Library" / "LaunchAgents";
	std::filesystem::path plist_path = launch_agent_dir / "com.jacobnilsson.tystnad.plist";

	if (std::filesystem::is_regular_file(plist_path)) {
		return;
	}

	std::error_code ec;
	if (!std::filesystem::exists(launch_agent_dir) && !std::filesystem::create_directories(launch_agent_dir, ec)) {
		throw std::runtime_error{"Failed to create directory"};
	}

	std::ofstream plist_file(plist_path);
	if (!plist_file.is_open()) {
		throw std::runtime_error{"Failed to open plist file"};
	}

	plist_file << R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
   "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
  <dict>
    <key>Label</key>
    <string>com.yourdomain.tystnad</string>

    <key>ProgramArguments</key>
    <array>
      <string>)" << app_path << R"(</string>
    </array>

    <key>RunAtLoad</key>
    <true/>

    <key>KeepAlive</key>
    <false/>
  </dict>
</plist>
)";
	plist_file.close();
}

void remove_launch_agent() {
	std::string home_dir = get_home_dir();
	if (home_dir.empty()) {
		throw std::runtime_error{"Failed to get home directory"};
	}

	std::filesystem::path launch_agent_dir = std::filesystem::path(home_dir) / "Library" / "LaunchAgents";
	std::filesystem::path plist_path = launch_agent_dir / "com.jacobnilsson.tystnad.plist";

	std::filesystem::remove(plist_path);
}
#endif
