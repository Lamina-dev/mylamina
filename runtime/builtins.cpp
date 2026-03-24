#include "builtins.hpp"

namespace lmx::runtime::builtins {
    const BuiltinConstant builtin_constants[] = {
        {"pi", 3.14159265358979323846},
        {"e", 2.71828182845904523536},
        {"phi", 1.61803398874989484820},
        {"gamma", 0.5772156649015328606065120900824024310421}
    };
    size_t builtin_constants_count = std::size(builtin_constants);
    size_t get_builtin_constant_index(const std::string& name) {
        for (size_t i = 0; i < builtin_constants_count; i++)
            if (builtin_constants[i].name == name)
                return i + builtin_start;
        return -1;
    }
}