//
// Created by geguj on 2025/12/27.
//
#pragma once
#include "lmx_export.hpp"
#include "value/value.hpp"
#include "opcode.hpp"
#include "frame/frame.hpp"
#include "../compiler/common.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "libloader.hpp"

namespace lmx::runtime {

// VM执行状态
struct LMVM_API LMXState {
    size_t pc{0};                     // 程序计数器
    std::array<Value, REG_COUNT> regs{}; // 寄存器数组
    std::vector<Value> heap{};
    std::vector<size_t> ret_addr_stack; // 返回地址栈
    std::vector<Op>* program{};        // 程序指令
    std::vector<std::unique_ptr<StackFrame>> stack_frames; // 栈帧
    std::vector<Value> stack;         // 操作数栈

    LMXState() = default;
    LMXState(const LMXState&) = delete;
    LMXState& operator=(const LMXState&) = delete;
    LMXState(LMXState&&) = delete;
    LMXState& operator=(LMXState&&) = delete;
    ~LMXState() = default;
};

// 虚拟机核心类
class LMVM_API VirtualCore {
    void* const_pool_top; // 常量池顶部指针
    LMXState ste;         // VM执行状态

    // 从常量池获取值
    [[nodiscard]] Value* get_value_from_pool(size_t offest) const;
    
    // 检查寄存器索引是否有效
    [[nodiscard]] bool is_valid_register(uint8_t reg) const;
    
    // 统一错误处理函数
    void handle_error(const char* error_message) const;
    
    // 验证多个寄存器索引
    [[nodiscard]] bool validate_registers(const uint8_t* regs, size_t count) const;
    
    // 验证跳转地址
    [[nodiscard]] bool validate_jump_address(uint64_t address) const;
    
    // 验证栈帧和局部变量索引
    [[nodiscard]] bool validate_stack_frame(size_t frame_index, size_t local_index) const;
    
    // 从常量池获取字符串
    [[nodiscard]] const char* get_constant_string(uint64_t offset) const;
public:
    std::vector<std::unique_ptr<DynLib>> libs; // 加载的动态库
    
    VirtualCore();
    ~VirtualCore();
    
    // 禁止拷贝和移动
    VirtualCore(const VirtualCore&) = delete;
    VirtualCore& operator=(const VirtualCore&) = delete;
    VirtualCore(VirtualCore&&) = delete;
    
    // 带状态的构造函数
    explicit VirtualCore(LMXState ste);
    
    // 带状态和常量池的构造函数
    explicit VirtualCore(LMXState ste, void* const_pool_top);
    
    // 运行VM
    int run();

    // 获取程序指令
    [[nodiscard]] std::vector<Op>* get_program() const { return ste.program; }
    
    // 设置程序指令
    void set_program(std::vector<Op>* program) { ste.pc = 0; ste.program = program; }
    
    // 查看寄存器值
    [[nodiscard]] int64_t look_register(const size_t r) const { return ste.regs[r].i64; }

    // 获取寄存器引用
    [[nodiscard]] Value& get_register(const size_t r) { return ste.regs[r]; }

    // 获取常量池指针
    [[nodiscard]] void* get_constant() const { return const_pool_top; }
    
    // 设置常量池指针
    void set_constant(void* const_pool) { const_pool_top = const_pool; }

    // 设置寄存器指针值
    void set_reg_ptr(const size_t idx, void* np) { ste.regs[idx].ptr = np; }
    
    // 堆内存管理方法
    [[nodiscard]] size_t heap_size() const { return ste.heap.size(); }
    Value& heap_at(size_t index) { return ste.heap[index]; }
    [[nodiscard]] const Value& heap_at(size_t index) const { return ste.heap[index]; }
    void heap_push_back(const Value& value) { ste.heap.push_back(value); }

    void insert_builtins();
};

}
