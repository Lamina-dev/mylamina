#include "common/file_run.hpp"
#include "common/repl.hpp"
#include "../compiler/common.hpp"
#include <string>
#include <filesystem>

#include "../../compiler/generator/generator.hpp"
#ifdef _WIN32
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <limits.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <linux/limits.h>
#endif

std::filesystem::path get_executable_path() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (length > 0) {
        return std::filesystem::path(buffer);
    }

#elif defined(__APPLE__)
    char buffer[PATH_MAX];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0) {
        char real_path[PATH_MAX];
        if (realpath(buffer, real_path) != nullptr) {
            return std::filesystem::path(real_path);
        }
    }

#elif defined(__linux__)
    // Linux
    char buffer[PATH_MAX];
    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (length != -1) {
        buffer[length] = '\0';
        return std::filesystem::path(buffer);
    }
#endif

    return {};  // 获取失败
}

std::filesystem::path get_executable_directory() {
    const std::filesystem::path exe_path = get_executable_path();
    if (!exe_path.empty()) {
        return exe_path.parent_path();  // 获取目录部分
    }
    return {};
}

int main(int argc, char* argv[]) {
    lmx::module_path.clear();
    lmx::module_path.push_back(std::filesystem::current_path());
    if (argc == 1) {
        return run_repl();
    }
    const auto exe_dir = get_executable_directory() / "modules";
    if (!std::filesystem::exists(exe_dir))
        lmx::module_path.push_back(exe_dir);
    return file_run(argv[1]);
}
