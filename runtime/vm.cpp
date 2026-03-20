//
// Created by geguj on 2025/12/27.
//

#include "vm.hpp"
#include <cmath>
#include <iostream>
#include <ostream>

#include "value/value.hpp"
#include "vmcall.hpp"
#include "../compiler/generator/generator.hpp"
#include "../include/debug.hpp"

namespace lmx::runtime {

VirtualCore::VirtualCore() : const_pool_top(nullptr), ste() {
    ste.program = nullptr;
    ste.pc = 0;
    ste.stack_frames.push_back(std::make_unique<StackFrame>());
    ste.stack_frames.back()->locals.resize(64);
}

/*VirtualCore::VirtualCore(LMXState ste) : const_pool_top(nullptr), ste(std::move(ste)) {}

VirtualCore::VirtualCore(LMXState ste, void* const_pool_top) : 
    const_pool_top(const_pool_top), 
    ste(std::move(ste)) {
}*/

Value* VirtualCore::get_value_from_pool(const size_t offest) const {
    return static_cast<Value*>(const_pool_top) + offest;
}

bool VirtualCore::is_valid_register(uint8_t reg) const {
    return reg < REG_COUNT;
}

bool VirtualCore::validate_registers(const uint8_t* regs, size_t count) const {
    for (size_t i = 0; i < count; i++) {
        if (!is_valid_register(regs[i])) {
            return false;
        }
    }
    return true;
}

bool VirtualCore::validate_jump_address(uint64_t address) const {
    return address < ste.program->size();
}

bool VirtualCore::validate_stack_frame(size_t frame_index, size_t local_index) const {
    if (frame_index >= ste.stack_frames.size()) {
        return false;
    }
    return local_index < ste.stack_frames[frame_index]->locals.size();
}

const char* VirtualCore::get_constant_string(uint64_t offset) const {
    if (const_pool_top == nullptr) {
        return nullptr;
    }
    return static_cast<char*>(const_pool_top) + offset;
}

void VirtualCore::handle_error(const char* error_message) const {
    fprintf(stderr, "[Error]: %s\n", error_message);
}

VirtualCore::~VirtualCore() {
    // 清理栈帧
    ste.stack_frames.clear();
    // 清理返回地址栈
    ste.ret_addr_stack.clear();
    // 程序指针设为nullptr
    ste.program = nullptr;
    // 常量池指针设为nullptr
    const_pool_top = nullptr;
    // 清理加载的库 - 库的析构函数会自动释放动态库句柄
    libs.clear();
}

int VirtualCore::run() {
    DEBUG_ENTER_FUNC();
    DEBUG_SEPARATOR("VM EXECUTION START");
    
    if (ste.program == nullptr) {
        handle_error("Program is null");
        DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
        return 1;
    }
    
    DEBUG_LOG_FMT("Program size: %zu instructions", ste.program->size());
    DEBUG_LOG("Starting VM execution");
    
    while (ste.pc < ste.program->size()) {
        const Opcode& op = ste.program->operator[](ste.pc).op;
        const auto& operands = ste.program->operator[](ste.pc).operands;
        
        // 显示当前执行的字节码行
        std::cerr << COLOR_YELLOW << "[EXEC] " << ste.pc << " | " << std::flush;
        // 根据指令类型显示详细信息
        switch (op) {
            using enum Opcode;
        case MOV_RI: {
            std::cerr << "MOVRI: r" << (int)operands[0] << ", " << *reinterpret_cast<const int64_t*>(operands + 1);
            break;
        }
        case MOV_RM: {
            std::cerr << "MOVRM: r" << (int)operands[0] << ", 0x" << std::hex << (uint64_t)operands[1] << std::dec;
            break;
        }
        case MOV_RR: {
            std::cerr << "MOVRR: r" << (int)operands[0] << ", r" << (int)operands[1];
            break;
        }
        case MOV_RC: {
            std::cerr << "MOVRC: r" << (int)operands[0] << ", const[" << *(uint64_t*)(operands + 1) << "]";
            break;
        }
        case MOV_MI: {
            std::cerr << "MOVMI: 0x" << std::hex << (uint64_t)operands[0] << std::dec << ", " << *reinterpret_cast<const int64_t*>(operands + 1);
            break;
        }
        case MOV_MM: {
            std::cerr << "MOVMM: 0x" << std::hex << (uint64_t)operands[0] << ", 0x" << (uint64_t)operands[1] << std::dec;
            break;
        }
        case MOV_MR: {
            std::cerr << "MOVMR: 0x" << std::hex << (uint64_t)operands[0] << std::dec << ", r" << (int)operands[1];
            break;
        }
        case MOV_MC: {
            std::cerr << "MOVMC: 0x" << std::hex << (uint64_t)operands[0] << std::dec << ", const[" << (uint64_t)operands[1] << "]";
            break;
        }
        case ADD: {
            std::cerr << "ADD: r" << (int)operands[0] << " = r" << (int)operands[1] << " + r" << (int)operands[2];
            break;
        }
        case SUB: {
            std::cerr << "SUB: r" << (int)operands[0] << " = r" << (int)operands[1] << " - r" << (int)operands[2];
            break;
        }
        case MUL: {
            std::cerr << "MUL: r" << (int)operands[0] << " = r" << (int)operands[1] << " * r" << (int)operands[2];
            break;
        }
        case DIV: {
            std::cerr << "DIV: r" << (int)operands[0] << " = r" << (int)operands[1] << " / r" << (int)operands[2];
            break;
        }
        case MOD: {
            std::cerr << "MOD: r" << (int)operands[0] << " = r" << (int)operands[1] << " % r" << (int)operands[2];
            break;
        }
        case POW: {
            std::cerr << "POW: r" << (int)operands[0] << " = pow(r" << (int)operands[1] << ", r" << (int)operands[2] << ")";
            break;
        }
        case FCALL: {
            std::cerr << "FCALL: func=0x" << std::hex << *reinterpret_cast<const uint64_t*>(operands) << std::dec << ", args=" << (int)operands[8];
            break;
        }
        case FRET: {
            std::cerr << "FRET: Return from function";
            break;
        }
        case HALT: {
            std::cerr << "HALT: Terminate execution";
            break;
        }
        case DEBUG_LOG: {
            std::cerr << "DEBUG_LOG: const[" << *reinterpret_cast<const uint64_t*>(operands) << "]";
            break;
        }
        case JMP: {
            std::cerr << "JMP: to " << *reinterpret_cast<const uint64_t*>(operands);
            break;
        }
        case CMP_GE: {
            std::cerr << "CMP_GE: r" << (int)operands[0] << " = r" << (int)operands[1] << " >= r" << (int)operands[2];
            break;
        }
        case CMP_LT: {
            std::cerr << "CMP_LT: r" << (int)operands[0] << " = r" << (int)operands[1] << " < r" << (int)operands[2];
            break;
        }
        case CMP_LE: {
            std::cerr << "CMP_LE: r" << (int)operands[0] << " = r" << (int)operands[1] << " <= r" << (int)operands[2];
            break;
        }
        case CMP_GT: {
            std::cerr << "CMP_GT: r" << (int)operands[0] << " = r" << (int)operands[1] << " > r" << (int)operands[2];
            break;
        }
        case CMP_EQ: {
            std::cerr << "CMP_EQ: r" << (int)operands[0] << " = r" << (int)operands[1] << " == r" << (int)operands[2];
            break;
        }
        case CMP_NE: {
            std::cerr << "CMP_NE: r" << (int)operands[0] << " = r" << (int)operands[1] << " != r" << (int)operands[2];
            break;
        }
        case IF_TRUE: {
            std::cerr << "IF_TRUE: r" << (int)operands[0] << " ? to " << *reinterpret_cast<const uint64_t*>(operands + 1);
            break;
        }
        case IF_FALSE: {
            std::cerr << "IF_FALSE: r" << (int)operands[0] << " ? to " << *reinterpret_cast<const uint64_t*>(operands + 1);
            break;
        }
        case FUNC_CREATE: {
            std::cerr << "FUNC_CREATE: Function definition";
            break;
        }
        case FUNC_END: {
            std::cerr << "FUNC_END: Function end";
            break;
        }
        case LOCAL_GET: {
            std::cerr << "LOCAL_GET: r" << (int)operands[0] << " = frame[" << (int)operands[1] << "]->locals[" << *(uint16_t*)(operands + 2) << "]";
            break;
        }
        case LOCAL_SET: {
            std::cerr << "LOCAL_SET: frame[" << (int)operands[0] << "]->locals[" << *(uint16_t*)(operands + 1) << "] = r" << (int)operands[3];
            break;
        }
        case AND: {
            std::cerr << "AND: r" << (int)operands[0] << " = r" << (int)operands[1] << " && r" << (int)operands[2];
            break;
        }
        case OR: {
            std::cerr << "OR: r" << (int)operands[0] << " = r" << (int)operands[1] << " || r" << (int)operands[2];
            break;
        }
        case VMC: {
            std::cerr << "VMC: Call vmcall[" << *(uint16_t*)operands << "]";
            break;
        }
        case DEC: {
            std::cerr << "DEC: r" << (int)operands[0] << "--";
            break;
        }
        case PUSH: {
            std::cerr << "PUSH: r" << (int)operands[0];
            break;
        }
        case CREATE_VECTOR: {
            std::cerr << "CREATE_VECTOR: r" << (int)operands[0] << ", " << (int)operands[1];
            break;
        }
        default: {
            std::cerr << "Unknown opcode: " << (int)op;
            break;
        }
        }
        std::cerr << COLOR_RESET << std::endl;
        
        // 执行指令
        switch (op) {
            using enum Opcode;
        case MOV_RI: {
            if (!is_valid_register(operands[0])) {
                handle_error("Invalid register index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                return 1;
            }
            const uint8_t dst_reg = operands[0];
            const int64_t imm_val = *reinterpret_cast<const int64_t*>(operands + 1);
            ste.regs[dst_reg] = imm_val;
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Move immediate value " << imm_val << " to register r" << (int)dst_reg << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Register r" << (int)dst_reg << " value set to " << imm_val << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed MOVRI instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case MOV_RM: {
            if (!is_valid_register(operands[0])) {
                handle_error("Invalid register index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                return 1;
            }
            const uint8_t dst_reg = operands[0];
            const uint64_t mem_addr = operands[1];
            if (mem_addr == 0) {
                handle_error("Null memory address in MOV_RM");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                return 1;
            }
            ste.regs[dst_reg] = *reinterpret_cast<const int64_t*>(mem_addr);
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Move value from memory address 0x" << std::hex << mem_addr << std::dec << " to register r" << (int)dst_reg << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Register r" << (int)dst_reg << " value set to " << ste.regs[dst_reg].i64 << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed MOVRM instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case MOV_RR: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1])) {
                handle_error("Invalid register index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                return 1;
            }
            const uint8_t dst_reg = operands[0];
            const uint8_t src_reg = operands[1];
            ste.regs[dst_reg] = ste.regs[src_reg];
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Move value from register r" << (int)src_reg << " to register r" << (int)dst_reg << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Register r" << (int)dst_reg << " value set to " << ste.regs[dst_reg].i64 << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed MOVRR instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case MOV_RC: {
            if (!is_valid_register(operands[0])) {
                handle_error("Invalid register index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                return 1;
            }
            if (const_pool_top == nullptr) {
                handle_error("Constant pool is null");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                return 1;
            }
            const uint8_t dst_reg = operands[0];
            const uint64_t const_idx = *(uint64_t*)(operands + 1);
            ste.regs[dst_reg] = (char*)get_constant() + const_idx;
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Move value from constant pool at index " << const_idx << " to register r" << (int)dst_reg << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Register r" << (int)dst_reg << " value set to constant[" << const_idx << "]" << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed MOVRC instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case MOV_MI: {
            const uint64_t mem_addr = operands[0];
            const int64_t imm_val = *reinterpret_cast<const int64_t*>(operands + 1);
            ste.heap[mem_addr] = imm_val;
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Move immediate value " << imm_val << " to memory address 0x" << std::hex << mem_addr << std::dec << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Memory at 0x" << std::hex << mem_addr << std::dec << " set to " << imm_val << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed MOVMI instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case MOV_MM: {
            const uint64_t dst_addr = operands[0];
            const uint64_t src_addr = operands[1];
            ste.heap[dst_addr] = ste.heap[src_addr];
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Move value from memory address 0x" << std::hex << src_addr << std::dec << " to memory address 0x" << std::hex << dst_addr << std::dec << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Memory at 0x" << std::hex << dst_addr << std::dec << " set to value from 0x" << std::hex << src_addr << std::dec << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed MOVMM instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case MOV_MR: {
            if (!is_valid_register(operands[1])) {
                handle_error("Invalid register index");
                return 1;
            }
            const uint64_t mem_addr = operands[0];
            const uint8_t src_reg = operands[1];
            ste.heap[mem_addr] = ste.regs[src_reg];
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Move value from register r" << (int)src_reg << " to memory address 0x" << std::hex << mem_addr << std::dec << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Memory at 0x" << std::hex << mem_addr << std::dec << " set to value from r" << (int)src_reg << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed MOVMR instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case MOV_MC: {
            if (const_pool_top == nullptr) {
                handle_error("Constant pool is null");
                return 1;
            }
            const uint64_t mem_addr = operands[0];
            const uint64_t const_idx = operands[1];
            ste.heap[mem_addr] = (char*)get_constant() + const_idx;
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Move value from constant pool at index " << const_idx << " to memory address 0x" << std::hex << mem_addr << std::dec << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Memory at 0x" << std::hex << mem_addr << std::dec << " set to constant[" << const_idx << "]" << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed MOVMC instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case ADD: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                return 1;
            }
            const uint8_t add_dst_reg = operands[0];
            const uint8_t add_src1_reg = operands[1];
            const uint8_t add_src2_reg = operands[2];
            
            // 类型检查
            const ValueType::ValueType type1 = ste.regs[add_src1_reg].type;
            const ValueType::ValueType type2 = ste.regs[add_src2_reg].type;
            DEBUG_LOG("type1: " << type1 << ", type2: " << type2);
            
            // 禁止向量与数字相加
            if ((type1 == ValueType::Ptr && type2 == ValueType::Int) || 
                (type1 == ValueType::Int && type2 == ValueType::Ptr)) {
                handle_error("Cannot add vector and number");
                return 1;
            }
            
            // 只允许相同类型的加法
            if (type1 != type2) {
                handle_error("Cannot add different types");
                return 1;
            }
            
            // 处理不同类型的加法
            if (type1 == ValueType::Int) {
                const int64_t add_result = ste.regs[add_src1_reg].i64 + ste.regs[add_src2_reg].i64;
                ste.regs[add_dst_reg] = add_result;
                ste.pc++;
                std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Add registers r" << (int)add_src1_reg << " + r" << (int)add_src2_reg << " = " << add_result << std::endl;
                std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Register r" << (int)add_dst_reg << " value set to " << add_result << std::endl;
            } else if (type1 == ValueType::Ptr) {
                DEBUG_LOG("Adding ptr...");
                // 向量加法：对应元素相加
                const size_t vec1_addr = ste.regs[add_src1_reg].u64;
                const size_t vec2_addr = ste.regs[add_src2_reg].u64;
                
                // 检查向量长度是否相同
                const size_t vec1_len = ste.heap[vec1_addr].i64;
                const size_t vec2_len = ste.heap[vec2_addr].i64;
                
                if (vec1_len != vec2_len) {
                    handle_error("Vectors must have the same length for addition");
                    return 1;
                }
                
                // 分配新向量
                const size_t vec_result_addr = ste.heap.size();
                ste.heap.resize(vec_result_addr + 1 + vec1_len);
                
                // 存储长度
                ste.heap[vec_result_addr].type = ValueType::Int;
                ste.heap[vec_result_addr].i64 = vec1_len;
                
                // 对应元素相加
                for (size_t i = 0; i < vec1_len; i++) {
                    const Value& elem1 = ste.heap[vec1_addr + 1 + i];
                    const Value& elem2 = ste.heap[vec2_addr + 1 + i];
                    
                    // 确保元素类型相同
                    if (elem1.type != elem2.type) {
                        handle_error("Vector elements must have the same type for addition");
                        return 1;
                    }
                    
                    // 执行元素加法
                    Value result_elem;
                    if (elem1.type == ValueType::Int) {
                        result_elem = elem1.i64 + elem2.i64;
                    } else {
                        handle_error("Unsupported vector element type for addition");
                        return 1;
                    }
                    
                    ste.heap[vec_result_addr + 1 + i] = result_elem;
                }
                
                // 设置结果
                ste.regs[add_dst_reg].type = ValueType::Ptr;
                ste.regs[add_dst_reg].u64 = vec_result_addr;
                ste.pc++;
                std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Add vectors r" << (int)add_src1_reg << " + r" << (int)add_src2_reg << std::endl;
                std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Register r" << (int)add_dst_reg << " value set to vector of length " << vec1_len << std::endl;
            } else {
                handle_error("Unsupported type for addition");
                return 1;
            }
            
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed ADD instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case SUB: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                return 1;
            }
            const uint8_t sub_dst_reg = operands[0];
            const uint8_t sub_src1_reg = operands[1];
            const uint8_t sub_src2_reg = operands[2];
            const int64_t sub_result = ste.regs[sub_src1_reg].i64 - ste.regs[sub_src2_reg].i64;
            ste.regs[sub_dst_reg] = sub_result;
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Subtract registers r" << (int)sub_src1_reg << " - r" << (int)sub_src2_reg << " = " << sub_result << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Register r" << (int)sub_dst_reg << " value set to " << sub_result << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed SUB instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case MUL: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                return 1;
            }
            const uint8_t mul_dst_reg = operands[0];
            const uint8_t mul_src1_reg = operands[1];
            const uint8_t mul_src2_reg = operands[2];
            const int64_t mul_result = ste.regs[mul_src1_reg].i64 * ste.regs[mul_src2_reg].i64;
            ste.regs[mul_dst_reg] = mul_result;
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Multiply registers r" << (int)mul_src1_reg << " * r" << (int)mul_src2_reg << " = " << mul_result << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Register r" << (int)mul_dst_reg << " value set to " << mul_result << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed MUL instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case DIV: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                return 1;
            }
            const uint8_t div_dst_reg = operands[0];
            const uint8_t div_src1_reg = operands[1];
            const uint8_t div_src2_reg = operands[2];
            if (ste.regs[div_src2_reg].i64 == 0) {
                handle_error("Division by zero");
                return 1;
            }
            const int64_t div_result = ste.regs[div_src1_reg].i64 / ste.regs[div_src2_reg].i64;
            ste.regs[div_dst_reg] = div_result;
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Divide registers r" << (int)div_src1_reg << " / r" << (int)div_src2_reg << " = " << div_result << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Register r" << (int)div_dst_reg << " value set to " << div_result << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed DIV instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case MOD: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                return 1;
            }
            const uint8_t mod_dst_reg = operands[0];
            const uint8_t mod_src1_reg = operands[1];
            const uint8_t mod_src2_reg = operands[2];
            if (ste.regs[mod_src2_reg].i64 == 0) {
                handle_error("Modulo by zero");
                return 1;
            }
            const int64_t mod_result = ste.regs[mod_src1_reg].i64 % ste.regs[mod_src2_reg].i64;
            ste.regs[mod_dst_reg] = mod_result;
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Modulo registers r" << (int)mod_src1_reg << " % r" << (int)mod_src2_reg << " = " << mod_result << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Register r" << (int)mod_dst_reg << " value set to " << mod_result << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed MOD instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case POW: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                return 1;
            }
            const uint8_t pow_dst_reg = operands[0];
            const uint8_t pow_src1_reg = operands[1];
            const uint8_t pow_src2_reg = operands[2];
            const double pow_result = std::pow(ste.regs[pow_src1_reg].f64, ste.regs[pow_src2_reg].f64);
            ste.regs[pow_dst_reg] = pow_result;
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Power registers r" << (int)pow_src1_reg << " ^ r" << (int)pow_src2_reg << " = " << pow_result << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Register r" << (int)pow_dst_reg << " value set to " << pow_result << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed POW instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case FCALL: {
            uint64_t target_pc = *reinterpret_cast<const uint64_t*>(operands);
            if (target_pc >= ste.program->size()) {
                handle_error("Invalid jump address");
                return 1;
            }
            const auto args_count = operands[8]; // 传参数量
            ste.ret_addr_stack.push_back(ste.pc + 1); // 返回地址
            ste.pc = target_pc; // 跳转地址
            ste.stack_frames.push_back(std::make_unique<StackFrame>()); //新建栈帧
            ste.stack_frames.back()->locals.resize(args_count + 1);
            for (uint8_t i = 0; i != args_count; i++) {
                if (REG_COUNT_INDEX_MAX - i >= REG_COUNT) {
                    handle_error("Invalid register index");
                    return 1;
                }
                ste.stack_frames.back()->locals[i] = ste.regs[REG_COUNT_INDEX_MAX - i];
            }
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Call function at address " << target_pc << " with " << (int)args_count << " arguments" << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: PC set to " << target_pc << ", return address pushed to stack, new stack frame created" << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed FCALL instruction, jumping to function address" << COLOR_RESET << std::endl;
            break;
        }
        case FRET: {
            if (ste.ret_addr_stack.empty() || ste.stack_frames.size() <= 1) {
                handle_error("Invalid return operation");
                return 1;
            }
            const size_t return_addr = ste.ret_addr_stack.back();
            ste.pc = return_addr; //返回地址
            ste.ret_addr_stack.pop_back();
            ste.stack_frames.pop_back(); //  恢复栈帧
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Return from function" << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: PC set to return address " << return_addr << ", stack frame popped" << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed FRET instruction, returning to caller" << COLOR_RESET << std::endl;
            break;
        }
        case HALT: {
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Terminate VM execution" << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: VM execution terminated normally" << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed HALT instruction, VM stopped running" << COLOR_RESET << std::endl;
            DEBUG_SEPARATOR("VM EXECUTION END (SUCCESS)");
            DEBUG_LEAVE_FUNC();
            return 0;
        }
        case DEBUG_LOG: {
            if (const_pool_top == nullptr) {
                handle_error("Constant pool is null");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                return 1;
            }
            fprintf(stderr,"[LogInfo]: %s\n", static_cast<char *>(const_pool_top) + *reinterpret_cast<const uint64_t*>(operands));
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Print debug log" << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Log printed to console" << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed DEBUG_LOG instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case JMP: {
            uint64_t target_pc = *reinterpret_cast<const uint64_t*>(operands);
            if (target_pc >= ste.program->size()) {
                handle_error("Invalid jump address");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                return 1;
            }
            ste.pc = target_pc;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Jump to address " << target_pc << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Program counter set to " << target_pc << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed JMP instruction, PC set to target address, continuing execution" << COLOR_RESET << std::endl;
            break;
        }
        case CMP_GE: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                return 1;
            }
            const uint8_t dst_reg = operands[0];
            const uint8_t src1_reg = operands[1];
            const uint8_t src2_reg = operands[2];
            bool result = ste.regs[src1_reg].i64 >= ste.regs[src2_reg].i64;
            ste.regs[dst_reg].b = result;
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Compare registers r" << (int)src1_reg << " >= r" << (int)src2_reg << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Register r" << (int)dst_reg << " set to " << (result ? "true" : "false") << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed CMP_GE instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case CMP_LT: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                return 1;
            }
            const uint8_t cmp_dst_reg = operands[0];
            const uint8_t cmp_src1_reg = operands[1];
            const uint8_t cmp_src2_reg = operands[2];
            bool cmp_result = ste.regs[cmp_src1_reg].i64 < ste.regs[cmp_src2_reg].i64;
            ste.regs[cmp_dst_reg].b = cmp_result;
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Compare registers r" << (int)cmp_src1_reg << " < r" << (int)cmp_src2_reg << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Register r" << (int)cmp_dst_reg << " set to " << (cmp_result ? "true" : "false") << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed CMP_LT instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case CMP_LE: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                return 1;
            }
            const uint8_t cmp_dst_reg = operands[0];
            const uint8_t cmp_src1_reg = operands[1];
            const uint8_t cmp_src2_reg = operands[2];
            bool cmp_result = ste.regs[cmp_src1_reg].i64 <= ste.regs[cmp_src2_reg].i64;
            ste.regs[cmp_dst_reg].b = cmp_result;
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Compare registers r" << (int)cmp_src1_reg << " <= r" << (int)cmp_src2_reg << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Register r" << (int)cmp_dst_reg << " set to " << (cmp_result ? "true" : "false") << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed CMP_LE instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case CMP_GT: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                return 1;
            }
            ste.regs[operands[0]].b = ste.regs[operands[1]].i64 >  ste.regs[operands[2]].i64;
            ste.pc++;
            break;
        }
        case CMP_EQ: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                return 1;
            }
            ste.regs[operands[0]].b = ste.regs[operands[1]].i64 == ste.regs[operands[2]].i64;
            ste.pc++;
            break;
        }
        case CMP_NE: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                return 1;
            }
            ste.regs[operands[0]].b = ste.regs[operands[1]].i64 != ste.regs[operands[2]].i64;
            ste.pc++;
            break;
        }
        case IF_TRUE: {
            if (!is_valid_register(operands[0])) {
                handle_error("Invalid register index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                return 1;
            }
            uint64_t target_pc = *reinterpret_cast<const uint64_t*>(operands + 1);
            if (target_pc >= ste.program->size()) {
                handle_error("Invalid jump address");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                return 1;
            }
            const uint8_t cond_reg = operands[0];
            bool condition = ste.regs[cond_reg].b;
            if (condition) {
                ste.pc = target_pc;
                std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Jump to address " << target_pc << " if condition is true" << std::endl;
                std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Condition is true, PC set to " << target_pc << std::endl;
            } else {
                ste.pc++;
                std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Jump to address " << target_pc << " if condition is true" << std::endl;
                std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Condition is false, PC increased by 1" << std::endl;
            }
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed IF_TRUE instruction, " << (condition ? "jumping to target address" : "continuing to next instruction") << COLOR_RESET << std::endl;
            break;
        }
        case IF_FALSE: {
            if (!is_valid_register(operands[0])) {
                handle_error("Invalid register index");
                return 1;
            }
            uint64_t target_pc = *reinterpret_cast<const uint64_t*>(operands + 1);
            if (target_pc >= ste.program->size()) {
                handle_error("Invalid jump address");
                return 1;
            }
            if (!ste.regs[operands[0]].b) ste.pc = target_pc;
            else ste.pc++;
            break;
        }
        case FUNC_CREATE: {
            while (ste.pc < ste.program->size() && ste.program->operator[](ste.pc).op != FUNC_END) {
                ste.pc++;
            }
            if (ste.pc < ste.program->size()) {
                ste.pc++;
            }
            break;
        }
        case FUNC_END: {
            ste.pc++;
            break;
        }
        case LOCAL_GET: {
            if (!is_valid_register(operands[0])) {
                handle_error("Invalid register index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                return 1;
            }
            if (operands[1] >= ste.stack_frames.size()) {
                handle_error("Invalid stack frame index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                return 1;
            }
            uint16_t local_index = *(uint16_t*)(operands + 2);
            if (local_index >= ste.stack_frames[operands[1]]->locals.size()) {
                handle_error("Invalid local variable index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                return 1;
            }
            const uint8_t dst_reg = operands[0];
            const uint8_t frame_idx = operands[1];
            ste.regs[dst_reg] = ste.stack_frames[frame_idx]->locals[local_index];
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Load local variable " << local_index << " from frame " << (int)frame_idx << " to register r" << (int)dst_reg << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Register r" << (int)dst_reg << " value set to " << ste.regs[dst_reg].i64 << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed LOCAL_GET instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case LOCAL_SET: {
            if (!is_valid_register(operands[3])) {
                handle_error("Invalid register index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                return 1;
            }
            if (operands[0] >= ste.stack_frames.size()) {
                handle_error("Invalid stack frame index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                return 1;
            }
            uint16_t local_index = *(uint16_t*)(operands + 1);
            if (local_index >= ste.stack_frames[operands[0]]->locals.size()) {
                handle_error("Invalid local variable index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                return 1;
            }
            const uint8_t frame_idx = operands[0];
            const uint8_t src_reg = operands[3];
            ste.stack_frames[frame_idx]->locals[local_index] = ste.regs[src_reg];
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Store register r" << (int)src_reg << " value to local variable " << local_index << " in frame " << (int)frame_idx << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: frame[" << (int)frame_idx << "]->locals[" << local_index << "] = " << ste.regs[src_reg].i64 << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Current state: " << COLOR_RESET;
            DEBUG_REGS(ste.regs);
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Because: Executed LOCAL_SET instruction, PC increased by 1, continuing to next instruction" << COLOR_RESET << std::endl;
            break;
        }
        case AND: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                return 1;
            }
            ste.regs[operands[0]].b = ste.regs[operands[1]].b && ste.regs[operands[2]].b;
            ste.pc++;
            break;
        }
        case OR: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                return 1;
            }
            ste.regs[operands[0]].b = ste.regs[operands[1]].b || ste.regs[operands[2]].b;
            ste.pc++;
            break;
        }
        case VMC: {
            uint16_t vmcall_index = *(uint16_t*)operands;
            if (vmcall_index >= VMCall::vmcall_count) {
                handle_error("Invalid VMCall index");
                return 1;
            }
            VMCall::vmcall_table[vmcall_index](this);
            ste.pc++;
            break;
        }
        case DEC: {
            if (!is_valid_register(operands[0])) {
                handle_error("Invalid register index");
                return 1;
            }
            ste.regs[operands[0]].i64--;
            ste.pc++;
            break;
        }
        case PUSH: {
            if (!is_valid_register(operands[0])) {
                handle_error("Invalid register index");
                return 1;
            }
            const uint8_t src_reg = operands[0];
            ste.stack.push_back(ste.regs[src_reg]);
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Push register r" << (int)src_reg << " to stack" << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Stack size: " << ste.stack.size() << std::endl;
            break;
        }
        case CREATE_VECTOR: {
            if (!is_valid_register(operands[0])) {
                handle_error("Invalid register index");
                return 1;
            }
            const uint8_t dst_reg = operands[0];
            const uint8_t count = operands[1];
            
            // 检查栈中是否有足够的元素
            if (ste.stack.size() < count) {
                handle_error("Not enough elements on stack for CREATE_VECTOR");
                return 1;
            }
            
            // 1. 计算需要的堆空间: 1个长度 + count个元素
            size_t slots = 1 + count;
            
            // 2. 在堆上分配内存
            size_t vec_addr = ste.heap.size();
            for (size_t i = 0; i < slots; i++) {
                Value value;
                value.type = ValueType::Null;
                value.null = nullptr;
                ste.heap.push_back(value);
            }
            
            // 3. 存长度到第一个槽位
            ste.heap[vec_addr].type = ValueType::Int;
            ste.heap[vec_addr].i64 = count;
            
            // 4. 从栈顶取元素（从最后压入的到最先压入的）
            for (size_t i = 0; i < count; i++) {
                // 从栈中取元素（栈顶是最后压入的元素）
                ste.heap[vec_addr + 1 + i] = ste.stack[ste.stack.size() - count + i];
            }
            
            // 5. 调整栈（弹出元素）
            for (size_t i = 0; i < count; i++) {
                ste.stack.pop_back();
            }
            
            // 6. 返回向量地址到目标寄存器
            ste.regs[dst_reg].type = ValueType::Ptr;
            ste.regs[dst_reg].u64 = vec_addr;
            
            ste.pc++;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Operation: Create vector with " << (int)count << " elements" << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Result: Vector at heap[" << vec_addr << "]" << std::endl;
            std::cerr << COLOR_YELLOW << "[EXEC]  |  Stack size after: " << ste.stack.size() << std::endl;
            break;
        }
        default:
            ste.pc++;
            break;
        }
    }

    DEBUG_LOG("PC out of bounds");
    DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
    DEBUG_LEAVE_FUNC();
    return 1;
}

}
