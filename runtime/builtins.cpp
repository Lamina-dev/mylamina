#include "builtins.hpp"

namespace lmx::runtime::builtins {
    const BuiltinConstant builtin_constants[] = {
        {"EARTH_GRAVITY", Value(9.80665)},
        {"MOON_GRAVITY", Value(1.625)},
        {"MARS_GRAVITY", Value(3.72076)},
        {"WATER_DENSITY", Value(1000.0)},
        {"STANDARD_PRESSURE", Value(101325.0)},
        {"STANDARD_TEMPERATURE", Value(273.15)},
        {"AIR_DENSITY", Value(1.225)},
        {"C", Value(2.99792458e8)},
        {"G", Value(6.67430e-11)},
        {"H", Value(6.62607015e-34)},
        {"KB", Value(1.380649e-23)},
        {"EPSILON_0", Value(8.8541878128e-12)},
        {"MU_0", Value(1.25663706212e-6)},
        {"AVOGADRO", Value(6.02214076e23)},
        {"R", Value(8.314462618)},
        {"FARADAY", Value(9.648533212e4)},
        {"AMU", Value(1.66053906660e-27)},
        {"MOLAR_VOLUME_IDEAL", Value(0.024465)},
        {"ROOM_PRESSURE", Value(1.0e5)},
        {"ROOM_TEMPERATURE", Value(297.15)}
    };
    size_t builtin_constants_count = std::size(builtin_constants);
    size_t get_builtin_constant_index(const std::string& name) {
        for (size_t i = 0; i < builtin_constants_count; i++)
            if (builtin_constants[i].name == name)
                return i + builtin_start;
        return SIZE_MAX;
    }
}