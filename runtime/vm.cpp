//
// Created by geguj on 2025/12/27.
//

#include "vm.hpp"
#include <cmath>
#include <iostream>
#include <ostream>

#include "vmcall.hpp"
#include "../compiler/generator/generator.hpp"

namespace lmx::runtime {

VirtualCore::VirtualCore() : const_pool_top(nullptr), ste() {
    static std::vector<Op> program;
    ste.program = &program;
    ste.pc = 0;
    ste.cur.push_back(std::make_unique<StackFrame>());
    ste.cur.back()->locals.resize(64);
}

/*VirtualCore::VirtualCore(LMXState ste) : const_pool_top(nullptr), ste(std::move(ste)) {}

VirtualCore::VirtualCore(LMXState ste, void* const_pool_top) : 
    const_pool_top(const_pool_top), 
    ste(std::move(ste)) {
}*/

Value *VirtualCore::get_value_from_pool(const size_t offest) const {
    return static_cast<Value*>(const_pool_top) + offest;
}

int VirtualCore::run() {
    RUN_CONTINUE:
    const Opcode& op = ste.program->operator[](ste.pc).op;
    const auto& operands = ste.program->operator[](ste.pc).operands;
    switch (op) {
        using enum Opcode;
    case MOV_RI: {
        ste.regs[operands[0]] = *reinterpret_cast<const int64_t*>(operands + 1);
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case MOV_RM: {
        // todo!
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case MOV_RR: {
        ste.regs[operands[0]] = ste.regs[operands[1]];
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case MOV_RC: {
        ste.regs[operands[0]] = (char*)get_constant() + *(uint64_t*)(operands + 1);
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case MOV_MI: {

        // todo!
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case MOV_MM: {
        // todo!
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case MOV_MR: {
        // todo!
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case MOV_MC: {
        // todo!
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case ADD: {
        ste.regs[operands[0]] = ste.regs[operands[1]].i64 + ste.regs[operands[2]].i64;
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case SUB: {
        ste.regs[operands[0]] = ste.regs[operands[1]].i64 - ste.regs[operands[2]].i64;
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case MUL: {
        ste.regs[operands[0]] = ste.regs[operands[1]].i64 * ste.regs[operands[2]].i64;
        ste.pc++;   
        goto RUN_CONTINUE;
    }
    case DIV: {
        ste.regs[operands[0]] = ste.regs[operands[1]].i64 / ste.regs[operands[2]].i64;
        ste.pc++;   
        goto RUN_CONTINUE;
    }
    case MOD: {
        ste.regs[operands[0]] = ste.regs[operands[1]].i64 % ste.regs[operands[2]].i64;
        ste.pc++;   
        goto RUN_CONTINUE;
    }
    case POW: {
        ste.regs[operands[0]] = std::pow(ste.regs[operands[1]].f64, ste.regs[operands[2]].f64);
        ste.pc++;   
        goto RUN_CONTINUE;
    }
    case FCALL: {
        ste.ret_addr_stack.push_back(ste.pc + 1); // 返回地址
        ste.pc = *reinterpret_cast<const uint64_t*>(operands); // 跳转地址
        const auto args_count = operands[8]; // 传参数量
        ste.cur.push_back(std::make_unique<StackFrame>()); //新建栈帧
        ste.cur.back()->locals.resize(args_count + 1);
        for (uint8_t i = 0; i != args_count; i++) ste.cur.back()->locals[i] = ste.regs[REG_COUNT_INDEX_MAX - i];
        goto RUN_CONTINUE;
    }
    case FRET: {
        ste.pc = ste.ret_addr_stack.back(); //返回地址
        ste.ret_addr_stack.pop_back();
        ste.cur.pop_back(); //  恢复栈帧
        goto RUN_CONTINUE;
    }
    case HALT: {
        return 0;
    }
    case DEBUG_LOG: {
        fprintf(stderr,"[LogInfo]: %s\n", static_cast<char *>(const_pool_top) + *reinterpret_cast<const uint64_t*>(operands));
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case JMP: {
        ste.pc = *reinterpret_cast<const uint64_t*>(operands);
        goto RUN_CONTINUE;
    }
    case CMP_GE: {
        ste.regs[operands[0]].b = ste.regs[operands[1]].i64 >= ste.regs[operands[2]].i64;
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case CMP_LT: {
        ste.regs[operands[0]].b = ste.regs[operands[1]].i64 <  ste.regs[operands[2]].i64;
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case CMP_LE: {
        ste.regs[operands[0]].b = ste.regs[operands[1]].i64 <= ste.regs[operands[2]].i64;
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case CMP_GT: {
        ste.regs[operands[0]].b = ste.regs[operands[1]].i64 >  ste.regs[operands[2]].i64;
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case CMP_EQ: {
        ste.regs[operands[0]].b = ste.regs[operands[1]].i64 == ste.regs[operands[2]].i64;
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case CMP_NE: {
        ste.regs[operands[0]].b = ste.regs[operands[1]].i64 != ste.regs[operands[2]].i64;
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case IF_TRUE: {
        if (ste.regs[operands[0]].b) ste.pc = *reinterpret_cast<const uint64_t*>(operands + 1);
        else ste.pc++;
        goto RUN_CONTINUE;
    }
    case IF_FALSE: {
        if (!ste.regs[operands[0]].b) ste.pc = *reinterpret_cast<const uint64_t*>(operands + 1);
        else ste.pc++;
        goto RUN_CONTINUE;
    }
    case FUNC_CREATE: {
        while (ste.program->operator[](ste.pc).op != FUNC_END) {ste.pc++;}
        ste.pc ++;
        goto RUN_CONTINUE;
    }
    case FUNC_END: {
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case LOCAL_GET: {
        ste.regs[operands[0]] = ste.cur[operands[1]]->locals[*(uint16_t*)(operands + 2)];
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case LOCAL_SET: {
        ste.cur[operands[0]]->locals[*(uint16_t*)(operands + 1)] = ste.regs[operands[3]];
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case AND: {
        ste.regs[operands[0]].b = ste.regs[operands[1]].b && ste.regs[operands[2]].b;
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case OR: {
        ste.regs[operands[0]].b = ste.regs[operands[1]].b || ste.regs[operands[2]].b;
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case VMC: {
        VMCall::vmcall_table[*(uint16_t*)operands](this);
        ste.pc++;
        goto RUN_CONTINUE;
    }
    case DEC: {
        ste.regs[operands[0]].i64--;
        ste.pc++;
        goto RUN_CONTINUE;
    }
    }

    return 1;
}

}
