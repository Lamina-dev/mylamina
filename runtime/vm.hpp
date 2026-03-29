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

struct LMVM_API LMXState {
    size_t pc{0};
    std::array<Value, REG_COUNT> regs{};

    std::vector<size_t> ret_addr_stack;
    //void* const_pool_top;
    std::vector<Op>* program{};

    std::vector<std::unique_ptr<StackFrame>> cur;
    LMXState() = default;
    // 2. 删除拷贝构造函数和拷贝赋值运算符
    LMXState(const LMXState&) = delete;
    LMXState& operator=(const LMXState&) = delete;

    // 3. 允许移动构造函数和移动赋值运算符
    LMXState(LMXState&&) = delete;
    LMXState& operator=(LMXState&&) = delete;
    LMXState(LMXState&);
    // 4. 析构函数
    ~LMXState() = default;
};
class LMVM_API VirtualCore {
    void* const_pool_top;
    LMXState ste;


    [[nodiscard]] Value *get_value_from_pool(size_t offest) const;
public:
    std::vector<std::unique_ptr<DynLib>> libs;
    VirtualCore();
    VirtualCore(const VirtualCore&) = delete;
    VirtualCore& operator=(const VirtualCore&) = delete;
    VirtualCore(VirtualCore&&) = delete;
    explicit VirtualCore(LMXState ste);
    explicit VirtualCore(LMXState ste, void* const_pool_top);
    int run();

    [[nodiscard]] std::vector<Op> *get_program() const { return ste.program; }
    void set_program(std::vector<Op> *program) { ste.pc = 0;ste.program = program; }
    [[nodiscard]] int64_t look_register(const size_t r) const { return ste.regs[r].i64; }

    [[nodiscard]] Value& get_register(const size_t r) { return ste.regs[r]; }

    [[nodiscard]] void* get_constant() const { return const_pool_top; }
    void set_constant(void* const_pool) { const_pool_top = const_pool; }

    void set_reg_ptr(const size_t idx, void* np) { ste.regs[idx].ptr = np; }
};

}
