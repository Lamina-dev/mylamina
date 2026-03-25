#include "builtins.hpp"

namespace lmx::runtime::builtins {
    const BuiltinConstant builtin_constants[] = {
        // 物理常量
        {"EARTH_GRAVITY", 9.80665},
        {"MOON_GRAVITY", 1.625},
        {"MARS_GRAVITY", 3.72076},
        {"WATER_DENSITY", 1000.0},
        {"STANDARD_PRESSURE", 101325.0},
        {"STANDARD_TEMPERATURE", 273.15},
        {"AIR_DENSITY", 1.225},
        {"C", 2.99792458e8},
        {"G", 6.67430e-11},
        {"H", 6.62607015e-34},
        {"KB", 1.380649e-23},
        {"EPSILON_0", 8.8541878128e-12},
        {"MU_0", 1.25663706212e-6},
        // 化学常量
        {"AVOGADRO", 6.02214076e23},
        {"R", 8.314462618},
        {"FARADAY", 9.648533212e4},
        {"AMU", 1.66053906660e-27},
        {"MOLAR_VOLUME_IDEAL", 0.024465},
        {"ROOM_PRESSURE", 1.0e5},
        {"ROOM_TEMPERATURE", 297.15}
    };
    size_t builtin_constants_count = std::size(builtin_constants);
    size_t get_builtin_constant_index(const std::string& name) {
        for (size_t i = 0; i < builtin_constants_count; i++)
            if (builtin_constants[i].name == name)
                return i + builtin_start;
        return SIZE_MAX;
    }
}