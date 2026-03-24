#pragma once
#include "value/value.hpp"
#include "../include/lmx_export.hpp"

namespace lmx::runtime::builtins {

// 内置常量定义
struct BuiltinConstant {
    const char* name;
    Value value;
};

// 内置常量数量
inline int builtin_start = 64;
extern LMVM_API size_t builtin_constants_count;
extern LMVM_API const BuiltinConstant builtin_constants[];

LMVM_API size_t get_builtin_constant_index(const std::string& name);

} // namespace lmx::runtime::builtins