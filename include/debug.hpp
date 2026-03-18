//
// 调试输出宏定义
// 定义DEBUG_OUTPUT宏来启用调试输出
// Created by DaLL
//

#pragma once

#include <iostream>
#include <string>
#include <iomanip>

// 启用调试输出宏 - 定义这个宏来启用调试输出
// 注释掉这一行来禁用调试输出
#define DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT

// 控制台颜色代码
#define COLOR_RESET   "\033[0m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_RED     "\033[31m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_WHITE   "\033[37m"

// 带颜色的输出宏
#define DEBUG_COLOR(color, msg) do { \
    std::cerr << color << "[DEBUG] " << __FILE__ << ":" << __LINE__ << " - " << msg << COLOR_RESET << std::endl; \
} while(0)

// 基础调试输出宏（黄色）
#define DEBUG_LOG(msg) do { \
    std::cerr << COLOR_YELLOW << "[DEBUG] " << __FILE__ << ":" << __LINE__ << " - " << msg << COLOR_RESET << std::endl; \
} while(0)

// 带格式的调试输出（黄色）
#define DEBUG_LOG_FMT(fmt, ...) do { \
    std::cerr << COLOR_YELLOW << "[DEBUG] " << __FILE__ << ":" << __LINE__ << " - "; \
    std::fprintf(stderr, fmt, ##__VA_ARGS__); \
    std::cerr << COLOR_RESET << std::endl; \
} while(0)

// 进入函数调试（青色）
#define DEBUG_ENTER_FUNC() do { \
    std::cerr << COLOR_CYAN << "[DEBUG] ENTER: " << __FUNCTION__ << " at " << __FILE__ << ":" << __LINE__ << COLOR_RESET << std::endl; \
} while(0)

// 离开函数调试（绿色）
#define DEBUG_LEAVE_FUNC() do { \
    std::cerr << COLOR_GREEN << "[DEBUG] LEAVE: " << __FUNCTION__ << " at " << __FILE__ << ":" << __LINE__ << COLOR_RESET << std::endl; \
} while(0)

// 值调试（黄色）
#define DEBUG_VAL(name, val) do { \
    std::cerr << COLOR_YELLOW << "[DEBUG] " << __FILE__ << ":" << __LINE__ << " - " << name << " = " << (val) << COLOR_RESET << std::endl; \
} while(0)

// 指针调试（洋红色）
#define DEBUG_PTR(name, ptr) do { \
    std::cerr << COLOR_MAGENTA << "[DEBUG] " << __FILE__ << ":" << __LINE__ << " - " << name << " = " << static_cast<void*>(ptr) << COLOR_RESET << std::endl; \
} while(0)

// 错误调试（红色）
#define DEBUG_ERROR(msg) do { \
    std::cerr << COLOR_RED << "[ERROR] " << __FILE__ << ":" << __LINE__ << " - " << msg << COLOR_RESET << std::endl; \
} while(0)

// Token列表展示（蓝色）
#define DEBUG_TOKEN_LIST(tokens) do { \
    std::cerr << COLOR_BLUE << "\n========== TOKEN LIST ==========" << COLOR_RESET << std::endl; \
    for (size_t i = 0; i < tokens.size(); i++) { \
        std::cerr << COLOR_BLUE << "[" << std::setw(3) << i << "] " << tokens[i] << COLOR_RESET << std::endl; \
    } \
    std::cerr << COLOR_BLUE << "================================" << COLOR_RESET << std::endl; \
} while(0)

// 字节码展示（青色）
#define DEBUG_BYTECODE(ops) do { \
    std::cerr << COLOR_CYAN << "\n========== BYTECODE ==========" << COLOR_RESET << std::endl; \
    Generator::print_ops(const_cast<std::vector<runtime::Op>&>(ops)); \
    std::cerr << COLOR_CYAN << "==============================" << COLOR_RESET << std::endl; \
} while(0)

// 执行步骤展示（黄色）
#define DEBUG_EXEC_STEP(pc, op, desc) do { \
    std::cerr << COLOR_YELLOW << "[EXEC] PC=" << std::setw(4) << pc << " | " << desc << COLOR_RESET << std::endl; \
} while(0)

// 寄存器状态展示（白色）
#define DEBUG_REGS(regs) do { \
    std::cerr << COLOR_WHITE << "  REGS: "; \
    for (size_t i = 0; i < REG_COUNT && i < 16; i++) { \
        if (regs[i].type == ValueType::Int) \
            std::cerr << "r" << i << "=" << regs[i].i64 << " "; \
        else if (regs[i].type == ValueType::Bool) \
            std::cerr << "r" << i << "=" << (regs[i].b ? "T" : "F") << " "; \
        else if (regs[i].type == ValueType::Float) \
            std::cerr << "r" << i << "=" << regs[i].f64 << " "; \
    } \
    std::cerr << COLOR_RESET << std::endl; \
} while(0)

// 分隔线
#define DEBUG_SEPARATOR(title) do { \
    std::cerr << COLOR_YELLOW << "\n========== " << title << " ==========" << COLOR_RESET << std::endl; \
} while(0)

#else

// 禁用时的空宏
#define DEBUG_LOG(msg) do {} while(0)
#define DEBUG_LOG_FMT(fmt, ...) do {} while(0)
#define DEBUG_ENTER_FUNC() do {} while(0)
#define DEBUG_LEAVE_FUNC() do {} while(0)
#define DEBUG_VAL(name, val) do {} while(0)
#define DEBUG_PTR(name, ptr) do {} while(0)
#define DEBUG_COLOR(color, msg) do {} while(0)
#define DEBUG_ERROR(msg) do {} while(0)
#define DEBUG_TOKEN_LIST(tokens) do {} while(0)
#define DEBUG_BYTECODE(ops) do {} while(0)
#define DEBUG_EXEC_STEP(pc, op, desc) do {} while(0)
#define DEBUG_REGS(regs) do {} while(0)
#define DEBUG_SEPARATOR(title) do {} while(0)

#endif
