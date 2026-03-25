//
// 错误报告系统
// 提供标准的错误报告接口
//

#pragma once

#include <string>
#include <source_location>

#include "lmx_export.hpp"

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

// 便捷宏
#define LM_ERROR(msg) error_reporter(lmx::ErrorType::ERR, msg)

} // lmx
