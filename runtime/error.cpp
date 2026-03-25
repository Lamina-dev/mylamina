//
// 错误报告系统实现
//

#include "../include/error.hpp"
#include <iostream>
#include <string>

namespace lmx {

// 错误报告函数实现
void error_reporter(ErrorType type, const std::string& message, 
                   const std::source_location& loc) {
    // 根据错误类型选择前缀和颜色
    std::string prefix;
    std::string color;
    std::string reset_color = "\033[0m";
    
    switch (type) {
        case ErrorType::ERR:
            prefix = "[ERROR]";
            color = "\033[31m"; // 红色
            break;
        case ErrorType::WARNING:
            prefix = "[WARNING]";
            color = "\033[33m"; // 黄色
            break;
        case ErrorType::DEBUG:
            prefix = "[DEBUG]";
            color = "\033[36m"; // 青色
            break;
        case ErrorType::INFO:
            prefix = "[INFO]";
            color = "\033[32m"; // 绿色
            break;
        default:
            prefix = "[UNKNOWN]";
            color = "\033[37m"; // 白色
            break;
    }
    
    // 输出错误信息
    std::cerr << color << prefix << " " 
              << loc.file_name() << ":" << loc.line() << " - " 
              << message << reset_color << std::endl;
}

} // lmx
