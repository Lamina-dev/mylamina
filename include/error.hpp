#pragma once

#include <string>

namespace lmx {

// 错误类型枚举
enum class ErrorType {
    ERR
};

// 错误报告函数
// 参数:
//   type: 错误类型
//   message: 错误消息
inline void error_reporter(const ErrorType type, const std::string& message) {
    if (type == ErrorType::ERR) std::cerr << "Error: " << message << std::endl;
}

inline void LM_ERROR(const std::string& msg) {
    error_reporter(ErrorType::ERR, msg);
}

class ParserError final : public std::runtime_error {
public:
    explicit ParserError(
        const std::string& msg
    ): std::runtime_error(msg) {}
};

#define ITIS(x, convert) (std::string(#x " = <") + convert(x) + std::string(">"))
} // lmx
