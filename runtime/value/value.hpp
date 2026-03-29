//
// Created by geguj on 2025/12/27.
//

#pragma once
#include <cstdint>
#include <string>
#include "../../include/lmx_export.hpp"

namespace lmx::runtime {
namespace ValueType { enum ValueType {
    Bool, Int, Float, Str, Ptr, Null,// ptr用于 外来类型
    NO_ENUM_VALUE,
};
inline const char* value_type_name[NO_ENUM_VALUE] = {
    "Bool", "int", "Float", "Str", "Ptr", "Null"
};
inline const char* type_name(const ValueType t) { return value_type_name[t]; }
};
struct LMVM_API Value {
    ValueType::ValueType type;
    union {
        bool       b;
        int8_t     i8;
        uint8_t    u8;
        int16_t    i16;
        uint16_t   u16;
        int32_t    i32;
        uint32_t   u32;
        int64_t    i64;
        uint64_t   u64;
        float      f32;
        double     f64;
        std::nullptr_t null;
        void* ptr;
        char* str;
    };
    explicit Value(void* p);
    Value();
    Value(const Value& other) = default;
    template<class T>
    T& get();

    Value &operator=(void* new_ptr);
    Value &operator=(bool n);
    Value &operator=(uint8_t n);
    Value &operator=(uint64_t n);
    Value &operator=(int64_t n);
    Value &operator=(double n);
    Value &operator=(char* n);

    Value &operator=(const Value &rhs);
    Value &operator=(Value &&rhs) = delete;

    [[nodiscard]] const char* type_name() const;

    [[nodiscard]] std::string to_string() const;

    bool operator==(const Value &rhs) const;
    bool operator!=(const Value &rhs) const;
};

struct ValueHasher {
    size_t operator()(const Value& v) const noexcept;
};

}
