//
// Created by geguj on 2025/12/28.
//

#include "emit.hpp"
#include <bit>
#include <chrono>
#include <cstring>

namespace lmx {
LMXOpcodeEmitter::Ret_Type LMXOpcodeEmitter::ret_type = None;
template<class T>
void LMXOpcodeEmitter::write_imm(uint8_t *dst, T imm) {
    memcpy(dst, &imm, sizeof(imm));
}

void LMXOpcodeEmitter::write_imm(uint8_t *dst, uint16_t imm) {
    memcpy(dst, &imm, sizeof(imm));
}

template<class... Args>
 void LMXOpcodeEmitter::write_regs(uint8_t *dst, Args... args) {
    ((*dst++ = std::forward<Args>(args)), ...);
}
void LMXOpcodeEmitter::emit_mov_ri(std::vector<lmx::runtime::Op> &ops, uint8_t r1, int64_t imm) {
    lmx::runtime::Op op(lmx::runtime::Opcode::MOV_RI);
    op.operands[0] = r1;
    memcpy(op.operands + 1, &imm, sizeof(imm));
    ops.push_back(op);
    ret_type = Reg;
}
void LMXOpcodeEmitter::emit_mov_rr(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2) {

    if (r1 == r2) return;
    if (ret_type == Reg) {
        // 如果上一条指令的 目标寄存器 与本条指令的 源寄存器 相同
        // 不生成新指令， 修改上条指令目标为本条目标
        if (auto& last = ops.back(); last.operands[0] == r2) {
            last.operands[0] = r1;
            return;
        }
    }
    lmx::runtime::Op op(lmx::runtime::Opcode::MOV_RR);
    op.operands[0] = r1;
    op.operands[1] = r2;
    ops.push_back(op);
    ret_type = Reg;
}
void LMXOpcodeEmitter::emit_div(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3) {
    lmx::runtime::Op op(lmx::runtime::Opcode::DIV);
    write_regs(op.operands, r1, r2, r3);
    ops.push_back(op);
    ret_type = Reg;
}
void LMXOpcodeEmitter::emit_add(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3) {
    lmx::runtime::Op op(lmx::runtime::Opcode::ADD);
    write_regs(op.operands, r1, r2, r3);
    ops.push_back(op);
    ret_type = Reg;
}
void LMXOpcodeEmitter::emit_sub(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3) {
    lmx::runtime::Op op(lmx::runtime::Opcode::SUB);
    write_regs(op.operands, r1, r2, r3);
    ops.push_back(op);
    ret_type = Reg;
}
void LMXOpcodeEmitter::emit_mul(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3) {
    lmx::runtime::Op op(lmx::runtime::Opcode::MUL);
    write_regs(op.operands, r1, r2, r3);
    ops.push_back(op);
    ret_type = Reg;
}
void LMXOpcodeEmitter::emit_mod(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3) {
    lmx::runtime::Op op(lmx::runtime::Opcode::MOD);
    write_regs(op.operands, r1, r2, r3);
    ops.push_back(op);
    ret_type = Reg;
}
void LMXOpcodeEmitter::emit_pow(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3) {
    lmx::runtime::Op op(lmx::runtime::Opcode::POW);
    write_regs(op.operands, r1, r2, r3);
    ops.push_back(op);
    ret_type = Reg;
}
void LMXOpcodeEmitter::emit_fcall(std::vector<lmx::runtime::Op> &ops, uint64_t idx, uint8_t ac) {
    lmx::runtime::Op op(lmx::runtime::Opcode::FCALL);
    write_imm(op.operands, std::bit_cast<int64_t>(idx));
    op.operands[8] = ac;
    ops.push_back(op);
    ret_type = None;
}
void LMXOpcodeEmitter::emit_halt(std::vector<lmx::runtime::Op> &ops) {
    lmx::runtime::Op op(lmx::runtime::Opcode::HALT);
    ops.push_back(op);
    ret_type = None;
}
void LMXOpcodeEmitter::emit_fret(std::vector<lmx::runtime::Op> &ops) {
    if (ops.back().op == lmx::runtime::Opcode::FRET) return;
    lmx::runtime::Op op(lmx::runtime::Opcode::FRET);
    ops.push_back(op);
    ret_type = Reg;
}
void LMXOpcodeEmitter::emit_debug_log(std::vector<lmx::runtime::Op> &ops, uint64_t idx) {
    lmx::runtime::Op op(lmx::runtime::Opcode::DEBUG_LOG);
    write_imm(op.operands, std::bit_cast<int64_t>(idx));
    ops.push_back(op);
    ret_type = None;
}

void LMXOpcodeEmitter::emit_jmp(std::vector<lmx::runtime::Op> &ops, uint64_t idx) {
    lmx::runtime::Op op(lmx::runtime::Opcode::JMP);
    write_imm(op.operands, std::bit_cast<int64_t>(idx));
    ops.push_back(op);
    ret_type = None;
}

void LMXOpcodeEmitter::emit_cmp_gt(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3) {
    lmx::runtime::Op op(lmx::runtime::Opcode::CMP_GT);
    write_regs(op.operands, r1, r2, r3);
    ops.push_back(op);
    ret_type = Reg;
}

void LMXOpcodeEmitter::emit_cmp_ge(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3) {
    lmx::runtime::Op op(lmx::runtime::Opcode::CMP_GE);
    write_regs(op.operands, r1, r2, r3);
    ops.push_back(op);
    ret_type = Reg;
}

void LMXOpcodeEmitter::emit_cmp_lt(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3) {
    lmx::runtime::Op op(lmx::runtime::Opcode::CMP_LT);
    write_regs(op.operands, r1, r2, r3);
    ops.push_back(op);
    ret_type = Reg;
}

void LMXOpcodeEmitter::emit_cmp_le(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3) {
    lmx::runtime::Op op(lmx::runtime::Opcode::CMP_LE);
    write_regs(op.operands, r1, r2, r3);
    ops.push_back(op);
    ret_type = Reg;
}

void LMXOpcodeEmitter::emit_cmp_eq(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3) {
    lmx::runtime::Op op(lmx::runtime::Opcode::CMP_EQ);
    write_regs(op.operands, r1, r2, r3);
    ops.push_back(op);
    ret_type = Reg;
}

void LMXOpcodeEmitter::emit_cmp_ne(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3) {
    lmx::runtime::Op op(lmx::runtime::Opcode::CMP_NE);
    write_regs(op.operands, r1, r2, r3);
    ops.push_back(op);
    ret_type = Reg;
}

void LMXOpcodeEmitter::emit_if_true(std::vector<lmx::runtime::Op> &ops, uint8_t r, uint64_t idx) {
    lmx::runtime::Op op(lmx::runtime::Opcode::IF_TRUE);
    op.operands[0] = r;
    write_imm(op.operands + 1, std::bit_cast<int64_t>(idx));
    ops.push_back(op);
    ret_type = None;
}

void LMXOpcodeEmitter::emit_if_false(std::vector<lmx::runtime::Op> &ops, uint8_t r, uint64_t idx) {
    lmx::runtime::Op op(lmx::runtime::Opcode::IF_FALSE);
    op.operands[0] = r;
    write_imm(op.operands + 1, std::bit_cast<int64_t>(idx));
    ops.push_back(op);
    ret_type = None;
}

void LMXOpcodeEmitter::emit_func_create(std::vector<lmx::runtime::Op> &ops) {
    lmx::runtime::Op op(lmx::runtime::Opcode::FUNC_CREATE);
    ops.push_back(op);
    ret_type = None;
}

void LMXOpcodeEmitter::emit_func_end(std::vector<lmx::runtime::Op> &ops) {
    lmx::runtime::Op op(lmx::runtime::Opcode::FUNC_END);
    ops.push_back(op);
    ret_type = None;
}
#define SHOULD_OPT false
#if SHOULD_OPT
/* 对getter指令进行合并优化
 * getter指令源为地址，目标为寄存器
 * 如果上条指令返回类型为Addr且等于本条指令的Addr，那么合并，生成一次mov_rr操作
 */
#define getter_opt_code \
if (ret_type == Addr) { \
if (const auto& last = ops.back();\
    *reinterpret_cast<const uint16_t *>(last.operands) == idx\
    ) {\
    auto src_reg = last.operands[2], dst_reg = r;\
    ops.pop_back();\
    emit_mov_rr(ops,  dst_reg, src_reg);\
    return;\
    }\
}
#else
#define getter_opt_code
#endif
void LMXOpcodeEmitter::emit_local_get(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t s, uint16_t idx) {
    getter_opt_code
    lmx::runtime::Op op(lmx::runtime::Opcode::LOCAL_GET);
    op.operands[0] = r1;
    op.operands[1] = s;
    write_imm(op.operands + 2, idx);
    ops.push_back(op);
    ret_type = Reg;
}

void LMXOpcodeEmitter::emit_local_set(std::vector<lmx::runtime::Op> &ops, uint8_t s, uint16_t idx, uint8_t r2) {
    lmx::runtime::Op op(lmx::runtime::Opcode::LOCAL_SET);
    op.operands[0] = s;
    write_imm(op.operands + 1, idx);
    op.operands[3] = r2;
    ops.push_back(op);
    ret_type = Addr;
}
void LMXOpcodeEmitter::emit_and(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3) {
    runtime::Op op(runtime::Opcode::AND);
    op.operands[0] = r1;
    op.operands[1] = r2;
    op.operands[2] = r3;
    ops.push_back(op);
    ret_type = Reg;
}
void LMXOpcodeEmitter::emit_or(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3) {
    runtime::Op op(runtime::Opcode::OR);
    op.operands[0] = r1;
    op.operands[1] = r2;
    op.operands[2] = r3;
    ops.push_back(op);
    ret_type = Reg;
}

void LMXOpcodeEmitter::emit_vmc(std::vector<lmx::runtime::Op> &ops, uint16_t idx) {
    runtime::Op op(runtime::Opcode::VMC);
    write_imm(op.operands, idx);
    ops.push_back(op);
    ret_type = None;
}

void LMXOpcodeEmitter::emit_mov_rc(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint64_t idx) {
    runtime::Op op(runtime::Opcode::MOV_RC);
    op.operands[0] = r1;
    write_imm(op.operands + 1, std::bit_cast<int64_t>(idx));
    ops.push_back(op);
}
void LMXOpcodeEmitter::emit_dec(std::vector<lmx::runtime::Op> &ops, uint8_t r) {
    runtime::Op op(runtime::Opcode::DEC);
    op.operands[0] = r;
    ops.push_back(op);
}

#undef getter_opt_code
} // namespace lmx




