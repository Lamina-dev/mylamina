//
// Created by geguj on 2025/12/27.
//
//
// AD-iOS add "case ValueType::NO_ENUM_VALUE: break;" to line 63
//

#include "value.hpp"

#include <cstring>

namespace lmx::runtime {

Value::Value() : null(nullptr), type(ValueType::Ptr) {
}

Value::Value(void* p) : i64(*static_cast<int64_t*>(p)), type(ValueType::Ptr) {
}

Value& Value::operator=(void* new_ptr) {
    this->ptr = new_ptr;
    this->type = ValueType::Ptr;
    return *this;
}
Value& Value::operator=(bool b) {
    this->b = b;
    this->type = ValueType::Bool;
    return *this;
}
Value& Value::operator=(char* n) {
    this->str = n;
    this->type = ValueType::Str;
    return *this;
}
Value& Value::operator=(int64_t n) {
    this->i64 = n;
    this->type = ValueType::Int;
    return *this;
}
Value& Value::operator=(double n) {
    this->f64 = n;
    this->type = ValueType::Float;
    return *this;
}
Value& Value::operator=(uint8_t n) {
    this->u8 = n;
    this->type = ValueType::Int;
    return *this;
}
Value& Value::operator=(uint64_t n) {
    this->u64 = n;
    this->type = ValueType::Int;
    return *this;
}
Value& Value::operator=(const Value& rhs) {
    this->type = rhs.type;
    switch (this->type) {
        case ValueType::Int: this->i64 = rhs.i64; break;
        case ValueType::Float: this->f64 = rhs.f64; break;
        case ValueType::Str: this->str = rhs.str; break;
        case ValueType::Bool: this->b = rhs.b; break;
        case ValueType::Ptr: this->ptr = rhs.ptr; break;
        case ValueType::Null: this->null = nullptr; break;
        case ValueType::NO_ENUM_VALUE: break; // add this line to avoid warning.
    }
    return *this;
}


bool Value::operator==(const Value& rhs) const {
    switch (this->type) {
        case ValueType::Int: return this->i64 == rhs.i64;
        case ValueType::Float: return this->f64 == rhs.f64;
        case ValueType::Str: return strcmp(this->str, rhs.str) == 0;
        case ValueType::Bool: return this->b == rhs.b;
        case ValueType::Ptr: return this->ptr == rhs.ptr;
        case ValueType::Null: return this->null == rhs.null;
        default: return false;
    }
}

bool Value::operator!=(const Value& rhs) const {
    switch (this->type) {
    case ValueType::Int: return this->i64 != rhs.i64;
    case ValueType::Float: return this->f64 != rhs.f64;
    case ValueType::Str: return strcmp(this->str, rhs.str) != 0;
    case ValueType::Bool: return this->b != rhs.b;
    case ValueType::Ptr: return this->ptr != rhs.ptr;
    case ValueType::Null: return this->null != rhs.null;
    default: return true;
    }
}

template<class T>
T &Value::get() {
    return *reinterpret_cast<T*>(this);
}

const char* Value::type_name() const {
    return ValueType::type_name(this->type);
}

std::string Value::to_string() const {
    switch (this->type) {
        case ValueType::Int:    return std::to_string(this->i64);
        case ValueType::Bool:   return this->b ? "true" : "false";
        case ValueType::Str:    return this->str;
        case ValueType::Float:  return std::to_string(this->f64);
        case ValueType::Null:   return "null";
        case ValueType::Ptr:    if (!this->ptr) return "null";
        default: return {};
    }
}

size_t ValueHasher::operator()(const Value& v) const noexcept {
    size_t seed = std::hash<int>{}(static_cast<int>(v.type));

    switch (v.type) {
    case ValueType::Bool:
        return seed ^ std::hash<bool>{}(v.b);

    case ValueType::Int:
        return seed ^ std::hash<uint64_t>{}(v.u64);

    case ValueType::Float:
        return seed ^ std::hash<double>{}(v.f64);

    case ValueType::Null:
        return seed ^ 0xdeadbeef;

    case ValueType::Ptr:
        return seed ^ std::hash<void*>{}(v.ptr);

    case ValueType::Str:
        if (v.str) {
            return seed ^ std::hash<std::string_view>{}(v.str);
        }
        return seed ^ 0xcafebabe;

    default:
        return seed;
    }
}
} // namespace lmx::runtime