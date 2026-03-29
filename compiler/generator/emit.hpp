//
// Created by geguj on 2025/12/28.
//

#pragma once
#include <cstdint>
#include <vector>

#include "../../include/opcode.hpp"
#include "../../include/lmx_export.hpp"

namespace lmx {

class LMC_API LMXOpcodeEmitter {

    template<class T>
    static void write_imm(uint8_t *dst, T imm);
    static void write_imm(uint8_t *dst, uint16_t imm);

    template<class... Args>
    static void write_regs(uint8_t* dst, Args... args);

public:
    enum Ret_Type {
        None, Reg, Addr
    };
    static Ret_Type ret_type;
    static void emit_mov_ri(std::vector<lmx::runtime::Op>& ops, uint8_t r1, int64_t imm);
    static void emit_mov_rr(std::vector<lmx::runtime::Op>& ops, uint8_t r1, uint8_t r2);
    static void emit_mov_rm(std::vector<lmx::runtime::Op>& ops, uint8_t r1, uint8_t r2, int8_t offest);
    static void emit_mov_rc(std::vector<lmx::runtime::Op>& ops, uint8_t r1, uint64_t idx);

    static void emit_dec(std::vector<lmx::runtime::Op> &ops, uint8_t r);

    static void emit_mov_mi(std::vector<lmx::runtime::Op>& ops, uint8_t r1, int8_t offest1, int64_t imm);
    static void emit_mov_mr(std::vector<lmx::runtime::Op>& ops, uint8_t r1, int8_t offest1, uint8_t r2);
    static void emit_mov_mm(std::vector<lmx::runtime::Op>& ops, uint8_t r1, int8_t offest1, uint8_t r2, int8_t offest2);
    static void emit_mov_mc(std::vector<lmx::runtime::Op>& ops, uint8_t r1, int8_t offest1, uint64_t idx);

    static void emit_add(std::vector<lmx::runtime::Op>& ops, uint8_t r1, uint8_t r2, uint8_t r3);
    static void emit_sub(std::vector<lmx::runtime::Op>& ops, uint8_t r1, uint8_t r2, uint8_t r3);
    static void emit_mul(std::vector<lmx::runtime::Op>& ops, uint8_t r1, uint8_t r2, uint8_t r3);
    static void emit_div(std::vector<lmx::runtime::Op>& ops, uint8_t r1, uint8_t r2, uint8_t r3);
    static void emit_mod(std::vector<lmx::runtime::Op>& ops, uint8_t r1, uint8_t r2, uint8_t r3);
    static void emit_pow(std::vector<lmx::runtime::Op>& ops, uint8_t r1, uint8_t r2, uint8_t r3);

    static void emit_halt (std::vector<lmx::runtime::Op>& ops);
    static void emit_fcall(std::vector<lmx::runtime::Op> &ops, uint64_t idx, uint8_t ac);
    static void emit_fret (std::vector<lmx::runtime::Op>& ops);

    static void emit_debug_log(std::vector<lmx::runtime::Op> &ops, uint64_t idx);

    static void emit_jmp(std::vector<lmx::runtime::Op> &ops, uint64_t idx);

    static void emit_cmp_gt(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3);
    static void emit_cmp_ge(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3);
    static void emit_cmp_lt(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3);
    static void emit_cmp_le(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3);
    static void emit_cmp_eq(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3);
    static void emit_cmp_ne(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3);

    static void emit_if_true(std::vector<lmx::runtime::Op> &ops, uint8_t r, uint64_t idx);

    static void emit_if_false(std::vector<lmx::runtime::Op> &ops, uint8_t r, uint64_t idx);

    static void emit_func_create(std::vector<lmx::runtime::Op> &ops);
    static void emit_func_end(std::vector<lmx::runtime::Op> &ops);

    static void emit_local_get(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t s, uint16_t idx);

    static void emit_local_set(std::vector<lmx::runtime::Op> &ops, uint8_t s, uint16_t idx, uint8_t r2);

    static void emit_and(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3);

    static void emit_or(std::vector<lmx::runtime::Op> &ops, uint8_t r1, uint8_t r2, uint8_t r3);

    static void emit_vmc(std::vector<lmx::runtime::Op> &ops, uint16_t idx);
};

} // namespace lmx