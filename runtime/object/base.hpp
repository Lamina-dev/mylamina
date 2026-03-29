#pragma once
#include <cstdint>
#include <string>

namespace lmx::runtime {
enum class LMXObjType: uint8_t {
    Null, Table, Complex, Fractional
};
struct BaseObj {
    LMXObjType type;
    constexpr explicit BaseObj(const LMXObjType type) : type(type) {}
};

}
