//
// Created by geguj on 2025/12/27.
//

#pragma once
#include <cstdint>
#include <cstring>

namespace lmx::runtime {
enum class Opcode : uint8_t {
    /*
         * R = 寄存器 1
         * I = 立即数 8
         * M = 内存偏移 （1字节寄存器 + 1字节偏移）
         * C = 常量池偏移 8
         */
    MOV_RI, MOV_RM, MOV_RR, MOV_RC, //op dst(1), src
    MOV_MI, MOV_MM, MOV_MR, MOV_MC, //op dst(2), src
    ADD, SUB, MUL, DIV, MOD, POW,   //op dst(1), src1(1), src2(1)
    HALT,
    FCALL,  //op mem(8)
    FRET, DEBUG_LOG,
    JMP,
    CMP_GE, CMP_LT, CMP_LE, CMP_GT, CMP_EQ, CMP_NE,
    IF_TRUE,
    IF_FALSE,
    FUNC_CREATE, FUNC_END,
    LOCAL_GET, LOCAL_SET,

    AND, OR,
    VMC,  // vmcall
    DEC,
};

inline uint8_t opcode_len(Opcode op) {
    switch (op) {
        using enum Opcode;
    case MOV_RI:
    case MOV_RC:
    case IF_TRUE:
    case IF_FALSE: return 9;
    case MOV_RM:
    case MOV_MR:
    case CMP_EQ:
    case CMP_GE:
    case CMP_GT:
    case CMP_LE:
    case CMP_LT:
    case CMP_NE:
    case AND:
    case OR: return 3;
    case MOV_RR:
    case VMC: return 2;
    case MOV_MC:
    case MOV_MI: return 10;
    case MOV_MM:
    case ADD:
    case SUB:
    case MUL:
    case DIV:
    case MOD:
    case POW:
    case LOCAL_GET:
    case LOCAL_SET: return 4;
    case HALT:
    case FRET:
    case FUNC_END:
    case FUNC_CREATE: return 0;
    case FCALL: return 9;
    case DEC: return 1;
    case JMP: return 8;
    case DEBUG_LOG: return 9;
    }
    return 0;
}

struct Op {
    Opcode op;
    uint8_t operands[12]{};

    explicit inline Op(const Opcode op, const uint8_t* operand): op(op) {
        memcpy(operands, operand, 12);
    }
    explicit inline Op(const Opcode op): op(op) {}
    explicit inline Op() = default;
};
} // namespace lmx
