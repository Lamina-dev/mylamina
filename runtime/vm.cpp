//
// Created by geguj on 2025/12/27.
//

#include "vm.hpp"
#include <cmath>
#include <iostream>
#include <ostream>

#include "builtins.hpp"
#include "value/value.hpp"
#include "vmcall.hpp"
#include "../compiler/generator/generator.hpp"
#include "../include/debug.hpp"

namespace lmx::runtime {

VirtualCore::VirtualCore() : const_pool_top(nullptr) {
    ste.program = nullptr;
    ste.pc = 0;
    ste.stack_frames.push_back(std::make_unique<StackFrame>());
    ste.stack_frames.back()->locals.resize(64);
    insert_builtins();
}

Value* VirtualCore::get_value_from_pool(const size_t offset) const {
    return static_cast<Value*>(const_pool_top) + offset;
}

bool VirtualCore::is_valid_register(const uint8_t reg) {
    DEBUG_LOG(ITIS(reg, std::to_string) << ", " << ITIS(REG_COUNT, std::to_string));
    return reg < REG_COUNT;
}

bool VirtualCore::validate_registers(const uint8_t* regs, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (!is_valid_register(regs[i])) {
            return false;
        }
    }
    return true;
}

bool VirtualCore::validate_jump_address(const uint64_t address) const {
    return address < ste.program->size();
}

bool VirtualCore::validate_stack_frame(const size_t frame_index, const size_t local_index) const {
    if (frame_index >= ste.stack_frames.size()) {
        return false;
    }
    return local_index < ste.stack_frames[frame_index]->locals.size();
}

const char* VirtualCore::get_constant_string(const uint64_t offset) const {
    if (const_pool_top == nullptr) {
        return nullptr;
    }
    return static_cast<char*>(const_pool_top) + offset;
}

void VirtualCore::handle_error(const char* error_message) {
    LM_ERROR(error_message);
}

VirtualCore::~VirtualCore() {
    ste.stack_frames.clear();
    ste.ret_addr_stack.clear();
    ste.program = nullptr;
    const_pool_top = nullptr;
    libs.clear();
}

void VirtualCore::log_op(const Opcode &op, const uint8_t(&operands)[12]) {
    switch (op) {
            using enum Opcode;
        case MOV_RI: {
            DEBUG_LOG_FMT("MOVRI: r%d, %lld", static_cast<int>(operands[0]), *reinterpret_cast<const int64_t*>(operands + 1));
            break;
        }
        case MOV_RM: {
            DEBUG_LOG_FMT("MOVRM: r%d, 0x%llx", static_cast<int>(operands[0]), static_cast<uint64_t>(operands[1]));
            break;
        }
        case MOV_RR: {
            DEBUG_LOG_FMT("MOVRR: r%d, r%d", static_cast<int>(operands[0]), static_cast<int>(operands[1]));
            break;
        }
        case MOV_RC: {
            DEBUG_LOG_FMT("MOVRC: r%d, const[%llu]", static_cast<int>(operands[0]), *(uint64_t*)(operands + 1));
            break;
        }
        case MOV_MI: {
            DEBUG_LOG_FMT("MOVMI: 0x%llx, %lld", static_cast<uint64_t>(operands[0]), *reinterpret_cast<const int64_t*>(operands + 1));
            break;
        }
        case MOV_MM: {
            DEBUG_LOG_FMT("MOVMM: 0x%llx, 0x%llx", static_cast<uint64_t>(operands[0]), static_cast<uint64_t>(operands[1]));
            break;
        }
        case MOV_MR: {
            DEBUG_LOG_FMT("MOVMR: 0x%llx, r%d", static_cast<uint64_t>(operands[0]), static_cast<int>(operands[1]));
            break;
        }
        case MOV_MC: {
            DEBUG_LOG_FMT("MOVMC: 0x%llx, const[%llu]", static_cast<uint64_t>(operands[0]), static_cast<uint64_t>(operands[1]));
            break;
        }
        case ADD: {
            DEBUG_LOG_FMT("ADD: r%d = r%d + r%d", static_cast<int>(operands[0]), static_cast<int>(operands[1]), static_cast<int>(operands[2]));
            break;
        }
        case SUB: {
            DEBUG_LOG_FMT("SUB: r%d = r%d - r%d", static_cast<int>(operands[0]), static_cast<int>(operands[1]), static_cast<int>(operands[2]));
            break;
        }
        case MUL: {
            DEBUG_LOG_FMT("MUL: r%d = r%d * r%d", static_cast<int>(operands[0]), static_cast<int>(operands[1]), static_cast<int>(operands[2]));
            break;
        }
        case DIV: {
            DEBUG_LOG_FMT("DIV: r%d = r%d / r%d", static_cast<int>(operands[0]), static_cast<int>(operands[1]), static_cast<int>(operands[2]));
            break;
        }
        case MOD: {
            DEBUG_LOG_FMT("MOD: r%d = r%d %% r%d", static_cast<int>(operands[0]), static_cast<int>(operands[1]), static_cast<int>(operands[2]));
            break;
        }
        case POW: {
            DEBUG_LOG_FMT("POW: r%d = pow(r%d, r%d)", static_cast<int>(operands[0]), static_cast<int>(operands[1]), static_cast<int>(operands[2]));
            break;
        }
        case FCALL: {
            DEBUG_LOG_FMT("FCALL: func=0x%llx, args=%d", *reinterpret_cast<const uint64_t*>(operands), static_cast<int>(operands[8]));
            break;
        }
        case FRET: {
            DEBUG_LOG("FRET: Return from function");
            break;
        }
        case HALT: {
            DEBUG_LOG("HALT: Terminate execution");
            break;
        }
        case DEBUG_LOG: {
            DEBUG_LOG_FMT("DEBUG_LOG: const[%llu]", *reinterpret_cast<const uint64_t*>(operands));
            break;
        }
        case JMP: {
            DEBUG_LOG_FMT("JMP: to %llu", *reinterpret_cast<const uint64_t*>(operands));
            break;
        }
        case CMP_GE: {
            DEBUG_LOG_FMT("CMP_GE: r%d = r%d >= r%d", static_cast<int>(operands[0]), static_cast<int>(operands[1]), static_cast<int>(operands[2]));
            break;
        }
        case CMP_LT: {
            DEBUG_LOG_FMT("CMP_LT: r%d = r%d < r%d", static_cast<int>(operands[0]), static_cast<int>(operands[1]), static_cast<int>(operands[2]));
            break;
        }
        case CMP_LE: {
            DEBUG_LOG_FMT("CMP_LE: r%d = r%d <= r%d", static_cast<int>(operands[0]), static_cast<int>(operands[1]), static_cast<int>(operands[2]));
            break;
        }
        case CMP_GT: {
            DEBUG_LOG_FMT("CMP_GT: r%d = r%d > r%d", static_cast<int>(operands[0]), static_cast<int>(operands[1]), static_cast<int>(operands[2]));
            break;
        }
        case CMP_EQ: {
            DEBUG_LOG_FMT("CMP_EQ: r%d = r%d == r%d", static_cast<int>(operands[0]), static_cast<int>(operands[1]), static_cast<int>(operands[2]));
            break;
        }
        case CMP_NE: {
            DEBUG_LOG_FMT("CMP_NE: r%d = r%d != r%d", static_cast<int>(operands[0]), static_cast<int>(operands[1]), static_cast<int>(operands[2]));
            break;
        }
        case IF_TRUE: {
            DEBUG_LOG_FMT("IF_TRUE: r%d ? to %llu", static_cast<int>(operands[0]), *reinterpret_cast<const uint64_t*>(operands + 1));
            break;
        }
        case IF_FALSE: {
            DEBUG_LOG_FMT("IF_FALSE: r%d ? to %llu", static_cast<int>(operands[0]), *reinterpret_cast<const uint64_t*>(operands + 1));
            break;
        }
        case FUNC_CREATE: {
            DEBUG_LOG("FUNC_CREATE: Function definition");
            break;
        }
        case FUNC_END: {
            DEBUG_LOG("FUNC_END: Function end");
            break;
        }
        case LOCAL_GET: {
            DEBUG_LOG_FMT("LOCAL_GET: r%d = frame[%d]->locals[%d]", (int)operands[0], (int)operands[1], *(uint16_t*)(operands + 2));
            break;
        }
        case LOCAL_SET: {
            DEBUG_LOG_FMT("LOCAL_SET: frame[%d]->locals[%d] = r%d", (int)operands[0], *(uint16_t*)(operands + 1), (int)operands[3]);
            break;
        }
        case AND: {
            DEBUG_LOG_FMT("AND: r%d = r%d && r%d", static_cast<int>(operands[0]), static_cast<int>(operands[1]), static_cast<int>(operands[2]));
            break;
        }
        case OR: {
            DEBUG_LOG_FMT("OR: r%d = r%d || r%d", static_cast<int>(operands[0]), static_cast<int>(operands[1]), static_cast<int>(operands[2]));
            break;
        }
        case VMC: {
            DEBUG_LOG_FMT("VMC: Call vmcall[%d]", *(uint16_t*)operands);
            break;
        }
        case DEC: {
            DEBUG_LOG_FMT("DEC: r%d--", static_cast<int>(operands[0]));
            break;
        }
        case PUSH: {
            DEBUG_LOG_FMT("PUSH: r%d", static_cast<int>(operands[0]));
            break;
        }
        case CREATE_VECTOR: {
            DEBUG_LOG_FMT("CREATE_VECTOR: r%d, %d", static_cast<int>(operands[0]), static_cast<int>(operands[1]));
            break;
        }
        default: {
            DEBUG_LOG_FMT("Unknown opcode: %d", static_cast<int>(op));
            break;
        }
    }
}

bool VirtualCore::run_op(const Opcode &op, const uint8_t(&operands)[12], int &result) {
    switch (op) {
            using enum Opcode;
        case MOV_RI: {
            if (!is_valid_register(operands[0])) {
                handle_error("Invalid register index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                result = 1;
                return true;
            }
            const uint8_t dst_reg = operands[0];
            const int64_t imm_val = *reinterpret_cast<const int64_t*>(operands + 1);
            ste.regs[dst_reg] = imm_val;
            ste.pc++;
            DEBUG_LOG_FMT("MOVRI: r%d = %lld", static_cast<int>(dst_reg), imm_val);
            break;
        }
        case MOV_RM: {
            if (!is_valid_register(operands[0])) {
                handle_error("Invalid register index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                result = 1;
                return true;
            }
            const uint8_t dst_reg = operands[0];
            const uint64_t mem_addr = operands[1];
            if (mem_addr == 0) {
                handle_error("Null memory address in MOV_RM");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                result = 1;
                return true;
            }
            ste.regs[dst_reg] = *reinterpret_cast<const int64_t*>(mem_addr);
            ste.pc++;
            DEBUG_LOG_FMT("MOVRM: r%d = mem[0x%llx]", static_cast<int>(dst_reg), mem_addr);
            break;
        }
        case MOV_RR: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1])) {
                handle_error("Invalid register index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                result = 1;
                return true;
            }
            const uint8_t dst_reg = operands[0];
            const uint8_t src_reg = operands[1];
            ste.regs[dst_reg] = ste.regs[src_reg];
            ste.pc++;
            DEBUG_LOG_FMT("MOVRR: r%d = r%d", static_cast<int>(dst_reg), static_cast<int>(src_reg));
            break;
        }
        case MOV_RC: {
            if (!is_valid_register(operands[0])) {
                handle_error("Invalid register index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                result = 1;
                return true;
            }
            if (const_pool_top == nullptr) {
                handle_error("Constant pool is null");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                result = 1;
                return true;
            }
            const uint8_t dst_reg = operands[0];
            const uint64_t const_idx = *(uint64_t*)(operands + 1);
            ste.regs[dst_reg] = (char*)get_constant() + const_idx;
            ste.pc++;
            DEBUG_LOG_FMT("MOVRC: r%d = const[%llu]", static_cast<int>(dst_reg), const_idx);
            break;
        }
        case MOV_MI: {
            const uint64_t mem_addr = operands[0];
            const int64_t imm_val = *reinterpret_cast<const int64_t*>(operands + 1);
            ste.heap[mem_addr] = imm_val;
            ste.pc++;
            DEBUG_LOG_FMT("MOVMI: mem[0x%llx] = %lld", mem_addr, imm_val);
            break;
        }
        case MOV_MM: {
            const uint64_t dst_addr = operands[0];
            const uint64_t src_addr = operands[1];
            ste.heap[dst_addr] = ste.heap[src_addr];
            ste.pc++;
            DEBUG_LOG_FMT("MOVMM: mem[0x%llx] = mem[0x%llx]", dst_addr, src_addr);
            break;
        }
        case MOV_MR: {
            if (!is_valid_register(operands[1])) {
                handle_error("Invalid register index");
                result = 1;
                return true;
            }
            const uint64_t mem_addr = operands[0];
            const uint8_t src_reg = operands[1];
            ste.heap[mem_addr] = ste.regs[src_reg];
            ste.pc++;
            DEBUG_LOG_FMT("MOVMR: mem[0x%llx] = r%d", mem_addr, static_cast<int>(src_reg));
            break;
        }
        case MOV_MC: {
            if (const_pool_top == nullptr) {
                handle_error("Constant pool is null");
                result = 1;
                return true;
            }
            const uint64_t mem_addr = operands[0];
            const uint64_t const_idx = operands[1];
            ste.heap[mem_addr] = (char*)get_constant() + const_idx;
            ste.pc++;
            DEBUG_LOG_FMT("MOVMC: mem[0x%llx] = const[%llu]", mem_addr, const_idx);
            break;
        }
        case ADD: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                result = 1;
                return true;
            }
            const uint8_t add_dst_reg = operands[0];
            const uint8_t add_src1_reg = operands[1];
            const uint8_t add_src2_reg = operands[2];

            const ValueType::ValueType type1 = ste.regs[add_src1_reg].type;
            const ValueType::ValueType type2 = ste.regs[add_src2_reg].type;
            DEBUG_LOG("type1: " << type1 << ", type2: " << type2);

            if ((type1 == ValueType::Ptr && type2 == ValueType::Int) ||
                (type1 == ValueType::Int && type2 == ValueType::Ptr)) {
                handle_error("Cannot add vector and number");
                result = 1;
                return true;
            }

            if (type1 != type2) {
                handle_error("Cannot add different types");
                result = 1;
                return true;
            }

            if (type1 == ValueType::Int) {
                const int64_t add_result = ste.regs[add_src1_reg].i64 + ste.regs[add_src2_reg].i64;
                ste.regs[add_dst_reg] = add_result;
                ste.pc++;
                DEBUG_LOG_FMT("ADD: r%d = r%d + r%d = %lld", static_cast<int>(add_dst_reg),
                              static_cast<int>(add_src1_reg), static_cast<int>(add_src2_reg), add_result);
            } else if (type1 == ValueType::Ptr) {
                DEBUG_LOG("Adding ptr...");
                const size_t vec1_addr = ste.regs[add_src1_reg].u64;
                const size_t vec2_addr = ste.regs[add_src2_reg].u64;

                const size_t vec1_len = ste.heap[vec1_addr].i64;
                const size_t vec2_len = ste.heap[vec2_addr].i64;

                if (vec1_len != vec2_len) {
                    handle_error("Vectors must have the same length for addition");
                    result = 1;
                    return true;
                }

                const size_t vec_result_addr = ste.heap.size();
                ste.heap.resize(vec_result_addr + 1 + vec1_len);

                ste.heap[vec_result_addr].type = ValueType::Int;
                ste.heap[vec_result_addr].i64 = vec1_len;

                for (size_t i = 0; i < vec1_len; i++) {
                    const Value& elem1 = ste.heap[vec1_addr + 1 + i];
                    const Value& elem2 = ste.heap[vec2_addr + 1 + i];

                    if (elem1.type != elem2.type) {
                        handle_error("Vector elements must have the same type for addition");
                        result = 1;
                        return true;
                    }

                    Value result_elem;
                    if (elem1.type == ValueType::Int) {
                        result_elem = elem1.i64 + elem2.i64;
                    } else {
                        handle_error("Unsupported vector element type for addition");
                        result = 1;
                        return true;
                    }

                    ste.heap[vec_result_addr + 1 + i] = result_elem;
                }

                ste.regs[add_dst_reg].type = ValueType::Ptr;
                ste.regs[add_dst_reg].u64 = vec_result_addr;
                ste.pc++;
                DEBUG_LOG_FMT("ADD: vector r%d + r%d, result len=%llu",
                              static_cast<int>(add_src1_reg), static_cast<int>(add_src2_reg), vec1_len);
            } else {
                handle_error("Unsupported type for addition");
                result = 1;
                return true;
            }
            break;
        }
        case SUB: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                result = 1;
                return true;
            }
            const uint8_t sub_dst_reg = operands[0];
            const uint8_t sub_src1_reg = operands[1];
            const uint8_t sub_src2_reg = operands[2];
            const int64_t sub_result = ste.regs[sub_src1_reg].i64 - ste.regs[sub_src2_reg].i64;
            ste.regs[sub_dst_reg] = sub_result;
            ste.pc++;
            DEBUG_LOG_FMT("SUB: r%d = r%d - r%d = %lld", static_cast<int>(sub_dst_reg),
                          static_cast<int>(sub_src1_reg), static_cast<int>(sub_src2_reg), sub_result);
            break;
        }
        case MUL: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                result = 1;
                return true;
            }
            const uint8_t mul_dst_reg = operands[0];
            const uint8_t mul_src1_reg = operands[1];
            const uint8_t mul_src2_reg = operands[2];
            const int64_t mul_result = ste.regs[mul_src1_reg].i64 * ste.regs[mul_src2_reg].i64;
            ste.regs[mul_dst_reg] = mul_result;
            ste.pc++;
            DEBUG_LOG_FMT("MUL: r%d = r%d * r%d = %lld", static_cast<int>(mul_dst_reg),
                          static_cast<int>(mul_src1_reg), static_cast<int>(mul_src2_reg), mul_result);
            break;
        }
        case DIV: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                result = 1;
                return true;
            }
            const uint8_t div_dst_reg = operands[0];
            const uint8_t div_src1_reg = operands[1];
            const uint8_t div_src2_reg = operands[2];
            if (ste.regs[div_src2_reg].i64 == 0) {
                handle_error("Division by zero");
                result = 1;
                return true;
            }
            const int64_t div_result = ste.regs[div_src1_reg].i64 / ste.regs[div_src2_reg].i64;
            ste.regs[div_dst_reg] = div_result;
            ste.pc++;
            DEBUG_LOG_FMT("DIV: r%d = r%d / r%d = %lld", static_cast<int>(div_dst_reg),
                          static_cast<int>(div_src1_reg), static_cast<int>(div_src2_reg), div_result);
            break;
        }
        case MOD: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                result = 1;
                return true;
            }
            const uint8_t mod_dst_reg = operands[0];
            const uint8_t mod_src1_reg = operands[1];
            const uint8_t mod_src2_reg = operands[2];
            if (ste.regs[mod_src2_reg].i64 == 0) {
                handle_error("Modulo by zero");
                result = 1;
                return true;
            }
            const int64_t mod_result = ste.regs[mod_src1_reg].i64 % ste.regs[mod_src2_reg].i64;
            ste.regs[mod_dst_reg] = mod_result;
            ste.pc++;
            DEBUG_LOG_FMT("MOD: r%d = r%d %% r%d = %lld", static_cast<int>(mod_dst_reg),
                          static_cast<int>(mod_src1_reg), static_cast<int>(mod_src2_reg), mod_result);
            break;
        }
        case POW: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                result = 1;
                return true;
            }
            const uint8_t pow_dst_reg = operands[0];
            const uint8_t pow_src1_reg = operands[1];
            const uint8_t pow_src2_reg = operands[2];
            const double pow_result = std::pow(ste.regs[pow_src1_reg].f64, ste.regs[pow_src2_reg].f64);
            ste.regs[pow_dst_reg] = pow_result;
            ste.pc++;
            DEBUG_LOG_FMT("POW: r%d = pow(r%d, r%d) = %f", static_cast<int>(pow_dst_reg),
                          static_cast<int>(pow_src1_reg), static_cast<int>(pow_src2_reg), pow_result);
            break;
        }
        case FCALL: {
            uint64_t target_pc = *reinterpret_cast<const uint64_t*>(operands);
            if (target_pc >= ste.program->size()) {
                handle_error("Invalid jump address");
                result = 1;
                return true;
            }
            const auto args_count = operands[8];
            ste.ret_addr_stack.push_back(ste.pc + 1);
            ste.pc = target_pc;
            ste.stack_frames.push_back(std::make_unique<StackFrame>());
            ste.stack_frames.back()->locals.resize(args_count + 1);
            for (uint8_t i = 0; i != args_count; i++) {
                if (REG_COUNT_INDEX_MAX - i >= REG_COUNT) {
                    handle_error("Invalid register index");
                    result = 1;
                    return true;
                }
                ste.stack_frames.back()->locals[i] = ste.regs[REG_COUNT_INDEX_MAX - i];
            }
            DEBUG_LOG_FMT("FCALL: addr=%llu, args=%d", target_pc, static_cast<int>(args_count));
            break;
        }
        case FRET: {
            if (ste.ret_addr_stack.empty() || ste.stack_frames.size() <= 1) {
                handle_error("Invalid return operation");
                result = 1;
                return true;
            }
            const size_t return_addr = ste.ret_addr_stack.back();
            ste.pc = return_addr;
            ste.ret_addr_stack.pop_back();
            ste.stack_frames.pop_back();
            DEBUG_LOG_FMT("FRET: return to %llu", return_addr);
            break;
        }
        case HALT: {
            DEBUG_LOG("HALT");
            DEBUG_SEPARATOR("VM EXECUTION END (SUCCESS)");
            DEBUG_LEAVE_FUNC();
            result = 0;
            return true;
        }
        case DEBUG_LOG: {
            if (const_pool_top == nullptr) {
                handle_error("Constant pool is null");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                result = 1;
                return true;
            }
            DEBUG_LOG_FMT("[LogInfo]: %s", static_cast<char *>(const_pool_top) + *reinterpret_cast<const uint64_t*>(operands));
            ste.pc++;
            break;
        }
        case JMP: {
            uint64_t target_pc = *reinterpret_cast<const uint64_t*>(operands);
            if (target_pc >= ste.program->size()) {
                handle_error("Invalid jump address");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                result = 1;
                return true;
            }
            ste.pc = target_pc;
            DEBUG_LOG_FMT("JMP: %llu", target_pc);
            break;
        }
        case CMP_GE: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                result = 1;
                return true;
            }
            const uint8_t dst_reg = operands[0];
            const uint8_t src1_reg = operands[1];
            const uint8_t src2_reg = operands[2];
            bool result = ste.regs[src1_reg].i64 >= ste.regs[src2_reg].i64;
            ste.regs[dst_reg].b = result;
            ste.pc++;
            DEBUG_LOG_FMT("CMP_GE: r%d = (r%d >= r%d) = %s", static_cast<int>(dst_reg),
                          static_cast<int>(src1_reg), static_cast<int>(src2_reg), result ? "true" : "false");
            break;
        }
        case CMP_LT: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                result = 1;
                return true;
            }
            const uint8_t cmp_dst_reg = operands[0];
            const uint8_t cmp_src1_reg = operands[1];
            const uint8_t cmp_src2_reg = operands[2];
            bool cmp_result = ste.regs[cmp_src1_reg].i64 < ste.regs[cmp_src2_reg].i64;
            ste.regs[cmp_dst_reg].b = cmp_result;
            ste.pc++;
            DEBUG_LOG_FMT("CMP_LT: r%d = (r%d < r%d) = %s", static_cast<int>(cmp_dst_reg),
                          static_cast<int>(cmp_src1_reg), static_cast<int>(cmp_src2_reg), cmp_result ? "true" : "false");
            break;
        }
        case CMP_LE: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                result = 1;
                return true;
            }
            const uint8_t cmp_dst_reg = operands[0];
            const uint8_t cmp_src1_reg = operands[1];
            const uint8_t cmp_src2_reg = operands[2];
            bool cmp_result = ste.regs[cmp_src1_reg].i64 <= ste.regs[cmp_src2_reg].i64;
            ste.regs[cmp_dst_reg].b = cmp_result;
            ste.pc++;
            DEBUG_LOG_FMT("CMP_LE: r%d = (r%d <= r%d) = %s", static_cast<int>(cmp_dst_reg),
                          static_cast<int>(cmp_src1_reg), static_cast<int>(cmp_src2_reg), cmp_result ? "true" : "false");
            break;
        }
        case CMP_GT: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                result = 1;
                return true;
            }
            ste.regs[operands[0]].b = ste.regs[operands[1]].i64 >  ste.regs[operands[2]].i64;
            ste.pc++;
            break;
        }
        case CMP_EQ: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                result = 1;
                return true;
            }
            ste.regs[operands[0]].b = ste.regs[operands[1]].i64 == ste.regs[operands[2]].i64;
            ste.pc++;
            break;
        }
        case CMP_NE: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                result = 1;
                return true;
            }
            ste.regs[operands[0]].b = ste.regs[operands[1]].i64 != ste.regs[operands[2]].i64;
            ste.pc++;
            break;
        }
        case IF_TRUE: {
            if (!is_valid_register(operands[0])) {
                handle_error("Invalid register index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                result = 1;
                return true;
            }
            uint64_t target_pc = *reinterpret_cast<const uint64_t*>(operands + 1);
            if (target_pc >= ste.program->size()) {
                handle_error("Invalid jump address");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                result = 1;
                return true;
            }
            const uint8_t cond_reg = operands[0];
            bool condition = ste.regs[cond_reg].b;
            if (condition) {
                ste.pc = target_pc;
                DEBUG_LOG_FMT("IF_TRUE: r%d is true, jump to %llu", static_cast<int>(cond_reg), target_pc);
            } else {
                ste.pc++;
                DEBUG_LOG_FMT("IF_TRUE: r%d is false, continue", static_cast<int>(cond_reg));
            }
            break;
        }
        case IF_FALSE: {
            if (!is_valid_register(operands[0])) {
                handle_error("Invalid register index");
                result = 1;
                return true;
            }
            uint64_t target_pc = *reinterpret_cast<const uint64_t*>(operands + 1);
            if (target_pc >= ste.program->size()) {
                handle_error("Invalid jump address");
                result = 1;
                return true;
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
                result = 1;
                return true;
            }
            if (operands[1] >= ste.stack_frames.size()) {
                handle_error("Invalid stack frame index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                result = 1;
                return true;
            }
            uint16_t local_index = *(uint16_t*)(operands + 2);
            if (local_index >= ste.stack_frames[operands[1]]->locals.size()) {
                handle_error("Invalid local variable index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                result = 1;
                return true;
            }
            const uint8_t dst_reg = operands[0];
            const uint8_t frame_idx = operands[1];

            ste.regs[dst_reg] = ste.stack_frames[frame_idx]->locals[local_index];
            ste.pc++;

            DEBUG_LOG_FMT("LOCAL_GET: r%d = frame[%d].locals[%d] = %s", static_cast<int>(dst_reg),
                          static_cast<int>(frame_idx), local_index,
                          ste.regs[dst_reg].to_string().c_str());
            break;
        }
        case LOCAL_SET: {
            if (!is_valid_register(operands[3])) {
                handle_error("Invalid register index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                result = 1;
                return true;
            }
            if (operands[0] >= ste.stack_frames.size()) {
                handle_error("Invalid stack frame index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                result = 1;
                return true;
            }
            uint16_t local_index = *(uint16_t*)(operands + 1);
            if (local_index >= ste.stack_frames[operands[0]]->locals.size()) {
                handle_error("Invalid local variable index");
                DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
                result = 1;
                return true;
            }
            const uint8_t frame_idx = operands[0];
            const uint8_t src_reg = operands[3];
            ste.stack_frames[frame_idx]->locals[local_index] = ste.regs[src_reg];
            ste.pc++;
            DEBUG_LOG_FMT("LOCAL_SET: frame[%d].locals[%d] = r%d (%s)", static_cast<int>(frame_idx),
                          local_index, static_cast<int>(src_reg), ste.regs[src_reg].to_string().c_str());
            break;
        }
        case AND: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                result = 1;
                return true;
            }
            ste.regs[operands[0]].b = ste.regs[operands[1]].b && ste.regs[operands[2]].b;
            ste.pc++;
            break;
        }
        case OR: {
            if (!is_valid_register(operands[0]) || !is_valid_register(operands[1]) || !is_valid_register(operands[2])) {
                handle_error("Invalid register index");
                result = 1;
                return true;
            }
            ste.regs[operands[0]].b = ste.regs[operands[1]].b || ste.regs[operands[2]].b;
            ste.pc++;
            break;
        }
        case VMC: {
            uint16_t vmcall_index = *(uint16_t*)operands;
            if (vmcall_index >= VMCall::vmcall_count) {
                handle_error("Invalid VMCall index");
                result = 1;
                return true;
            }
            VMCall::vmcall_table[vmcall_index](this);
            ste.pc++;
            break;
        }
        case DEC: {
            if (!is_valid_register(operands[0])) {
                handle_error("Invalid register index");
                result = 1;
                return true;
            }
            ste.regs[operands[0]].i64--;
            ste.pc++;
            break;
        }
        case PUSH: {
            if (!is_valid_register(operands[0])) {
                handle_error("Invalid register index");
                result = 1;
                return true;
            }
            const uint8_t src_reg = operands[0];
            ste.stack.push_back(ste.regs[src_reg]);
            ste.pc++;
            DEBUG_LOG_FMT("PUSH: r%d, stack size=%llu", static_cast<int>(src_reg), ste.stack.size());
            break;
        }
        case CREATE_VECTOR: {
            if (!is_valid_register(operands[0])) {
                handle_error("Invalid register index");
                result = 1;
                return true;
            }
            const uint8_t dst_reg = operands[0];
            const uint8_t count = operands[1];

            if (ste.stack.size() < count) {
                handle_error("Not enough elements on stack for CREATE_VECTOR");
                result = 1;
                return true;
            }

            size_t vec_addr = ste.heap.size();
            ste.heap.resize(vec_addr + 1 + count);
            ste.heap[vec_addr].type = ValueType::Int;
            ste.heap[vec_addr].i64 = count;

            for (size_t i = 0; i < count; i++) {
                ste.heap[vec_addr + 1 + i] = ste.stack[ste.stack.size() - count + i];
            }

            for (size_t i = 0; i < count; i++) {
                ste.stack.pop_back();
            }

            ste.regs[dst_reg].type = ValueType::Ptr;
            ste.regs[dst_reg].u64 = vec_addr;

            ste.pc++;
            DEBUG_LOG_FMT("CREATE_VECTOR: r%d = vec[%d] at heap[%llu]", static_cast<int>(dst_reg),
                          static_cast<int>(count), vec_addr);
            break;
        }
        default:
            handle_error("unknown opcode");
            ste.pc++;
            break;
    }
    return false;
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
        const Opcode& op = (*ste.program)[ste.pc].op;
        const auto& operands = (*ste.program)[ste.pc].operands;
        
        DEBUG_EXEC_STEP(ste.pc, op, "");

        log_op(op, operands);
        if (int result; run_op(op, operands, result))
            return result;
    }

    DEBUG_LOG("PC out of bounds");
    DEBUG_SEPARATOR("VM EXECUTION END (ERROR)");
    DEBUG_LEAVE_FUNC();
    return 1;
}

void VirtualCore::insert_builtins() {
    DEBUG_LOG("insert builtins...");

    const auto old_program = ste.program;
    const auto old_pc = ste.pc;

    size_t base_index = builtins::builtin_start;
    for (size_t i = 0; i < builtins::builtin_constants_count; i++) {
        const auto& constant = builtins::builtin_constants[i];

        if (ste.stack_frames[0]->locals.size() <= base_index) {
            ste.stack_frames[0]->locals.resize(base_index + 1);
        }

        ste.stack_frames[0]->locals[base_index] = constant.value;

        DEBUG_LOG_FMT("builtin %s = %s at %zu",
            constant.name,
            constant.value.to_string().c_str(),
            base_index);
        base_index++;
    }

    ste.program = old_program;
    ste.pc = old_pc;
}
}
