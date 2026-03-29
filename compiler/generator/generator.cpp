//
// Created by geguj on 2025/12/28.
//
//
// question of AD-iOS fix "printf" format error, in 521 line
//

#include "generator.hpp"
#include <bitset>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "emit.hpp"
#include "opcode.hpp"
#include "vmcall.hpp"
#include "../common.hpp"

namespace lmx {
bool Generator::node_has_error = false;
Allocator::Allocator() {
    bitset.reset();
    bitset.set(0);
}

size_t Allocator::alloc() {
    for (size_t i = 0; i < REG_COUNT; i++) {
        if (!bitset.test(i)) {
            bitset.set(i);
            return i;
        }
    }
    return -1;
}

size_t Allocator::alloc(size_t i) {
    bitset.set(i);
    return i;
}

void Allocator::free(size_t i) {
    if (i > 0 && i <= REG_COUNT_INDEX_MAX && bitset.test(i)) {
        bitset.reset(i);
    }
}

bool Allocator::is_free(size_t i) {
    return bitset.test(i);
}

Generator::Generator() {
    cur.push_back(std::make_unique<CompilingFrame>("global"));
}
void Generator::write(runtime::Op& op) {
    ops.push_back(op);
}


std::vector<runtime::Op> &Generator::get_ops() {
    if (ops.back().op != runtime::Opcode::HALT) {
        ops.emplace_back(runtime::Opcode::HALT);
    }
    return ops;
}

size_t Generator::gen(std::shared_ptr<ASTNode> &n) {
    switch (n->kind) {
        case Program: return gen_program(n);
        case Binary: return gen_binary(n);
        case Unary: return gen_unary(n);
        case Return: return gen_return(n);
        case FuncCallExpr: return gen_func_call(n);
        case VarDecl: return gen_assign(n);
        case VarRef: return gen_var_ref(n);
        case NumLiteral: return gen_num(n);
        case StringLiteral: return gen_string(n);
        case BoolLiteral: return gen_bool(n);
        case BlockStmt: return gen_block(n);
        case IfStmt: return gen_if(n);
        case FuncDecl: return gen_function(n);
        case VMCall: return gen_vmc(n);
        case Module: return gen_module(n);
        case Use: return gen_use(n);
        case Loop: return gen_loop(n);
        case Break: return gen_break(n);
        case Continue: return gen_continue(n);
        default: {
            error("unknown node type");
            return -1;
        }
    }
}

size_t Generator::gen_program(std::shared_ptr<ASTNode> &n) {
    const auto node = std::static_pointer_cast<ProgramASTNode>(std::move(n));
    size_t last_ret = 0;
    for (auto& c: node->children) {
        last_ret = gen(c);
    }
    return last_ret;
}
size_t Generator::gen_loop(const std::shared_ptr<ASTNode> &shared) {
    const auto node = std::static_pointer_cast<LoopNode>(std::move(shared));

    size_t loop_cond = -1;
    if (node->condition) loop_cond = gen(node->condition);
    const size_t loop_start = tagging();

    size_t break_cond = -1;
    if (loop_cond != -1) {
        break_cond = tagging();
        LMXOpcodeEmitter::emit_if_false(ops, loop_cond, 0);
    }
    if (node->body->kind != BlockStmt) {
        error("should not have a loop body");
        return -1;
    }
    gen(node->body);
    const auto continue_point = tagging();
    if (loop_cond != -1)LMXOpcodeEmitter::emit_dec(ops, loop_cond);
    LMXOpcodeEmitter::emit_jmp(ops, loop_start);
    const auto break_point = tagging();

    for (const auto& bp: break_points)
        memcpy(ops[bp].operands, &break_point, sizeof(break_point));
    for (const auto& bp: continue_points)
        memcpy(ops[bp].operands, &continue_point, sizeof(continue_point));
    if (break_cond != -1) memcpy(ops[break_cond].operands + 1, &break_point, sizeof(break_cond));
    break_points.clear();
    continue_points.clear();
    regs.free(loop_cond);
    return -1;
}
size_t Generator::gen_continue(std::shared_ptr<ASTNode> &n) {
    const auto node = std::static_pointer_cast<ContinueNode>(std::move(n));
    continue_points.push_back(tagging());
    LMXOpcodeEmitter::emit_jmp(ops, 0);
    return -1;
}
size_t Generator::gen_break(std::shared_ptr<ASTNode> &n) {
    const auto node = std::static_pointer_cast<BreakNode>(std::move(n));
    break_points.push_back(tagging());
    LMXOpcodeEmitter::emit_jmp(ops, 0);
    return -1;
}

size_t Generator::gen_module(std::shared_ptr<ASTNode> &shared) {
    const auto node = std::static_pointer_cast<ModuleNode>(std::move(shared));
    for (const auto& mn : modules)
        if (mn == node->name) return -1;    //已解析过同名模块，不再解析


    if (node->type == ModuleNode::Types::dyn) {
        auto lib_name = static_pointer_cast<ASTNode>(node->lib);
        auto offest = gen_string(lib_name) - 1;
        LMXOpcodeEmitter::emit_mov_rc(ops, REG_COUNT_INDEX_MAX, offest);
        LMXOpcodeEmitter::emit_vmc(ops, 3);
    }
    std::string tmp;
    if (const auto t = cur_module.str(); t.empty()) {
        tmp = node->name;
        cur_module <<  tmp;
    } else {
        tmp = cur_module.str() + "." + node->name;
        cur_module << (" ." + node->name);
    }
    LMXOpcodeEmitter::emit_local_set(ops, cur.size() - 1, cur.back()->new_var(tmp, false), 0);
    modules.push_back(tmp);
    using_modules.push_back(tmp);


    /*
     * VMC_dyn_set
     * arg1 : lib_ptr(254)
     * arg2 : func_name(253)
     * arg3 : args_types_array_ptr(252)
     * arg4 : args_types_array_len(251)
     * arg5 : ret_type_ptr(250)
     */
    for (const auto& df : node->dyn) {

        // arg1
        auto [s, idx] = find_var(tmp);
        LMXOpcodeEmitter::emit_local_get(ops, REG_COUNT_INDEX_MAX, s, idx.second);

        // arg2
        const auto f_name_ti = constant_pool.size();
        constant_pool.insert(constant_pool.end(), df->extern_label->str.begin(), df->extern_label->str.end() + 1);
        LMXOpcodeEmitter::emit_mov_rc(ops, REG_COUNT_INDEX_MAX - 1, f_name_ti);

        // arg3
        const auto args_ti = constant_pool.size();
        for( const auto& arg : df->args_type)
            constant_pool.push_back(lmtype2ctype(arg->name));
        LMXOpcodeEmitter::emit_mov_rc(ops, REG_COUNT_INDEX_MAX - 2, args_ti);

        // arg4
        LMXOpcodeEmitter::emit_mov_ri(ops, REG_COUNT_INDEX_MAX - 3, df->args.size());


        // arg5
        const auto ret_type_ti = constant_pool.size();
        constant_pool.push_back(lmtype2ctype(df->ret_type->name));
        LMXOpcodeEmitter::emit_mov_rc(ops, REG_COUNT_INDEX_MAX - 4, ret_type_ti);

        LMXOpcodeEmitter::emit_vmc(ops, 4);
        extern_funcs[tmp + '.' + df->name] = {f_name_ti, df->args_type};
    }

    cur_module >> tmp;
    return -1;
}
size_t Generator::gen_use(std::shared_ptr<ASTNode> &shared) {
    const auto node = static_pointer_cast<UseNode>(std::move(shared));
    auto pn = compile(node->path->str);
    gen(pn);
    return -1;
}

size_t Generator::gen_binary(std::shared_ptr<ASTNode>& n) {
    const auto node = std::static_pointer_cast<BinaryNode>(std::move(n));
    volatile size_t tmp = regs.alloc();
    volatile auto lr = gen(node->left);
    LMXOpcodeEmitter::emit_mov_rr(ops, tmp, lr);
    if (expr_release) regs.free(lr);
    lr = tmp;

    volatile size_t expr_ret_reg;
    if (node->op == "|>") {
        LMXOpcodeEmitter::emit_mov_rr(ops, REG_COUNT_INDEX_MAX, lr);
        expr_ret_reg = basic_gen_func_call(node->right, 1);
        expr_release = true;
        return expr_ret_reg;
    }
    volatile size_t rr = gen(node->right);
    tmp = regs.alloc();
    LMXOpcodeEmitter::emit_mov_rr(ops, tmp, rr);
    if (expr_release) regs.free(rr);
    rr = tmp;
    expr_ret_reg = regs.alloc();

    if (node->op == "!=") {
        LMXOpcodeEmitter::emit_cmp_ne(ops, expr_ret_reg, lr, rr);
    } else if (node->op == "<=") {
        LMXOpcodeEmitter::emit_cmp_le(ops, expr_ret_reg, lr, rr);
    } else if (node->op == ">=") {
        LMXOpcodeEmitter::emit_cmp_ge(ops, expr_ret_reg, lr, rr);
    } else if (node->op == "==") {
        LMXOpcodeEmitter::emit_cmp_eq(ops, expr_ret_reg, lr, rr);
    } else if (node->op == "<")  {
        LMXOpcodeEmitter::emit_cmp_lt(ops, expr_ret_reg, lr, rr);
    } else if (node->op == ">")  {
        LMXOpcodeEmitter::emit_cmp_gt(ops, expr_ret_reg, lr, rr);
    } else if (node->op == "+")  {
        LMXOpcodeEmitter::emit_add(ops, expr_ret_reg, lr, rr);
    } else if (node->op == "-")  {
        LMXOpcodeEmitter::emit_sub(ops, expr_ret_reg, lr, rr);
    } else if (node->op == "*")  {
        LMXOpcodeEmitter::emit_mul(ops, expr_ret_reg, lr, rr);
    } else if (node->op == "/")  {
        LMXOpcodeEmitter::emit_div(ops, expr_ret_reg, lr, rr);
    } else if (node->op == "%")  {
        LMXOpcodeEmitter::emit_mod(ops, expr_ret_reg, lr, rr);
    } else if (node->op == "^")  {
        LMXOpcodeEmitter::emit_pow(ops, expr_ret_reg, lr, rr);
    } else if (node->op == "||") {
        LMXOpcodeEmitter::emit_or(ops, expr_ret_reg, lr, rr);
    } else if (node->op == "&&") {
        LMXOpcodeEmitter::emit_and(ops, expr_ret_reg, lr, rr);
    }
    else {
        node_has_error = true;
        error("operator: `" + node->op + "' is not defined.");
        return -1;
    }
    expr_release = true;
    return expr_ret_reg;
}

size_t Generator::gen_num(std::shared_ptr<ASTNode>& n) {
    const auto node = std::static_pointer_cast<NumberNode>(std::move(n));
    const auto expr_ret_reg = regs.alloc();
    expr_release = true;
    LMXOpcodeEmitter::emit_mov_ri(ops, expr_ret_reg, std::stoll(node->num));
    return expr_ret_reg;
}
size_t Generator::gen_assign(std::shared_ptr<ASTNode>& n) {
    const auto node = std::static_pointer_cast<VarDeclNode>(std::move(n));
    size_t expr_ret_reg = 0;
    if (const auto [fst, snd] = find_var(node->name); snd.second != UINT16_MAX) {
        //如果可变
        if (snd.first) {
            expr_ret_reg = gen(node->value);
            LMXOpcodeEmitter::emit_local_set(ops, fst, cur[fst]->locals[node->name].second, expr_ret_reg);
        } else {
            error("The var `" + node->name + "' is not mutable.");
        }
    } else { //如果未定义
        expr_ret_reg = gen(node->value);
        LMXOpcodeEmitter::emit_local_set(ops, cur.size() - 1, cur.back()->new_var(node->name, node->is_mut), expr_ret_reg);
    }
    regs.free(expr_ret_reg);
    return -1;
}
size_t Generator::gen_var_ref(std::shared_ptr<ASTNode>& n) {
    const auto node = std::static_pointer_cast<VarRefNode>(std::move(n));
    const auto [c, t] = find_var(node->name);
    size_t expr_ret_reg = regs.alloc();
    if (t.second != UINT16_MAX) {
        LMXOpcodeEmitter::emit_local_get(ops, expr_ret_reg, c, t.second);
    } else {
        regs.free(expr_ret_reg);
        expr_ret_reg = -1;
        error("The var `" + node->name + "` is not found!");
    }
    expr_release = false;
    return expr_ret_reg;
}

size_t Generator::gen_return(std::shared_ptr<ASTNode> &n) {
    const auto node = std::static_pointer_cast<ReturnStmtNode>(std::move(n));
    const auto expr_ret_reg = gen(node->expr);
    LMXOpcodeEmitter::emit_mov_rr(ops, 0, expr_ret_reg);
    LMXOpcodeEmitter::emit_fret(ops);
    if (expr_release) regs.free(expr_ret_reg);
    return 0;
}

size_t Generator::gen_function(std::shared_ptr<ASTNode> &n) {
    const auto node = std::static_pointer_cast<FuncDeclNode>(std::move(n));
    LMXOpcodeEmitter::emit_func_create(ops);
    new_frame(node->name);
    const auto args_count = node->args.size();
    for (const auto& arg : node->args) {
        if (find_var(arg).first != SIZE_MAX) {
            error("the function args name `" + arg + "` was defined on last scope");
        }
    }
    new_func(node->name, args_count); //函数不做作用域区分，全部全局
    for (size_t i = 0; i < args_count ; i++) {
        LMXOpcodeEmitter::emit_local_set(ops, cur.size() - 1, i,  REG_COUNT_INDEX_MAX - i);
        cur.back()->new_var(node->args[i], true, i);
    }
    if (node->body->kind != BlockStmt) error("expected block statement.");
    basic_gen_block(node->body);

    LMXOpcodeEmitter::emit_fret(ops);
    LMXOpcodeEmitter::emit_func_end(ops);
    free_frame();
    return -1;
}

size_t Generator::gen_func_call(std::shared_ptr<ASTNode> &n) {
    return basic_gen_func_call(n, 0);
}
size_t Generator::basic_gen_func_call(std::shared_ptr<ASTNode> &n, const size_t args_idx) {
    const auto node = std::static_pointer_cast<FuncCallExprNode>(std::move(n));
    if (find_func(node->name)) {
        const auto& [tar_ac, addr] = funcs[node->name];
        const auto args_count = node->args.size();
        if (args_count + args_idx != tar_ac) {
            error(
        "args num mismatch: "
            + std::to_string(args_count)
            + " != "
            + std::to_string(tar_ac) + " at function calling `" + node->name + "`");
            return -1;
        }
        basic_gen_pass_args(node->args, args_idx, args_count);
        LMXOpcodeEmitter::emit_fcall(ops, addr, tar_ac);
    } else if (extern_funcs.contains(node->name)) {
        const auto& [tar_ac, ts] = extern_funcs[node->name];
        const auto args_count = node->args.size();
        if (args_count + args_idx != ts.size()) {
            error(
        "args num mismatch: "
            + std::to_string(args_count)
            + " != "
            + std::to_string(ts.size()) + " at function calling `" + node->name + "`");
            return -1;
        }
        basic_gen_pass_args(node->args, args_idx, args_count);
        auto spilt = node->name.find_last_of('.');

        auto module_name = node->name.substr(0, spilt);
        auto mod = find_var(module_name);
        LMXOpcodeEmitter::emit_local_get(ops, 0, mod.first, mod.second.second);
        LMXOpcodeEmitter::emit_mov_rc(ops, 1, tar_ac);
        LMXOpcodeEmitter::emit_vmc(ops, 5);
    }
    else error("The function `" + node->name + "` is not defined.");
    return 0;
}
void Generator::basic_gen_pass_args(std::vector<std::shared_ptr<ASTNode>> &args, size_t args_idx, size_t args_count) {
    for (size_t i = args_idx; i < args_count ; i++) {
        const auto expr_ret = gen(args[i]);

        LMXOpcodeEmitter::emit_mov_rr(ops , REG_COUNT_INDEX_MAX - i, expr_ret);
        if (expr_release) regs.free(expr_ret);
    }
}

size_t Generator::gen_vmc(std::shared_ptr<ASTNode> &n) {
    const auto node = std::static_pointer_cast<VMCallNode>(std::move(n));
    basic_gen_pass_args(node->args, 0, node->args.size());
    const auto idx = std::stoi(node->idx);
    if (idx > VMCALL_INDEX_MAX || idx < 0) {
        error("exists vmc index: " + node->idx);
        return -1;
    }
    LMXOpcodeEmitter::emit_vmc(ops, idx);
    return 0;
}


size_t Generator::gen_unary(std::shared_ptr<ASTNode> &n) {
    const auto node = std::static_pointer_cast<UnaryNode>(std::move(n));
    const auto expr_ret_reg = gen(node->operand);


    const auto tmp_reg = regs.alloc();
    LMXOpcodeEmitter::emit_mov_ri(ops, tmp_reg, 0);
    regs.free(tmp_reg);

    const auto result = regs.alloc();

    if (node->op == "!") {
        LMXOpcodeEmitter::emit_cmp_eq(ops, result, expr_ret_reg, tmp_reg);
    } else if (node->op == "-") {
        LMXOpcodeEmitter::emit_sub(ops, result, tmp_reg, expr_ret_reg);
    } else {
        error("unary operator: `" + node->op + "' is not defined.");
    }
    if (expr_release) regs.free(expr_ret_reg);
    expr_release = true;
    return result;
}

size_t Generator::gen_string(std::shared_ptr<ASTNode> &n) {
    const auto node = std::static_pointer_cast<StringNode>(std::move(n));
    const auto expr_ret = constant_pool.size();
    constant_pool.insert(constant_pool.end(), node->str.begin(), node->str.end() + 1);
    //constant_pool.push_back('\0');
    const auto expr_ret_reg = regs.alloc();
    LMXOpcodeEmitter::emit_mov_rc(ops, expr_ret_reg, expr_ret);
    return expr_ret_reg;
}

size_t Generator::gen_bool(std::shared_ptr<ASTNode> &n) {
    const auto node = std::static_pointer_cast<BoolNode>(std::move(n));
    const auto expr_ret_reg = regs.alloc();
    LMXOpcodeEmitter::emit_mov_ri(ops, expr_ret_reg, node->b);
    expr_release = true;
    return expr_ret_reg;
}

size_t Generator::gen_block(std::shared_ptr<ASTNode> &n) {

    std::unordered_map save(cur.back()->locals);
    const size_t expr_ret_reg = basic_gen_block(n);
    cur.back()->locals = std::move(save);

    return expr_ret_reg;
}
size_t Generator::basic_gen_block(std::shared_ptr<ASTNode> &n) {
    const auto node = std::static_pointer_cast<BlockStmtNode>(std::move(n));
    size_t expr_ret_reg = 0;

    for (auto& c: node->children) expr_ret_reg = gen(c);

    return expr_ret_reg;
}

size_t Generator::gen_if(std::shared_ptr<ASTNode> &n) {
    const auto node = std::static_pointer_cast<IfStmtNode>(std::move(n));
    const auto cond = gen(node->condition);

    const auto the = tagging();
    LMXOpcodeEmitter::emit_if_true(ops, cond, 0);

    const auto els = tagging();
    LMXOpcodeEmitter::emit_jmp(ops, 0);

    auto tmp = tagging();
    memcpy(ops[the].operands + 1, &tmp, sizeof(tmp));
    gen(node->thenBlock);
    const size_t then_end = tagging();
    LMXOpcodeEmitter::emit_jmp(ops, 0);
    tmp = tagging();
    memcpy(ops[els].operands, &tmp, sizeof(tmp));
    if (node->elseBlock)
        gen(node->elseBlock);

    tmp = tagging();
    memcpy(ops[then_end].operands, &tmp, sizeof(tmp));
    regs.free(cond);
    return -1;
}

void Generator::write_binary_file(const std::string& path) {
    const auto pos = path.find_last_of('.');
    std::string p;
    if (pos != std::string::npos) {
         p = path.substr(0, pos);
    }else p = path;
    p += ".lmc";
    std::ofstream file(p, std::ios::binary);
    file.write((char*)&lmx_magic, sizeof(lmx_magic));
    file.write((char*)&lmx_version, sizeof(lmx_version));
    for (const auto& op: get_ops()) {
        file << static_cast<uint8_t>(op.op);
        file.write((char*)op.operands, runtime::opcode_len(op.op));
    }

    file.write(constant_pool.data(), constant_pool.size());
    file.close();
}
void Generator::print_ops(std::vector<runtime::Op>& ops) {
    size_t i = 0;
    for (auto &op: ops) {
        printf("[0x%zx]\t", i++); // fix [0x%llx] the warning caused by
        switch (op.op) {
            using enum runtime::Opcode;
        case MOV_RI: {
            printf("MOVRI: %d, %lld\n", op.operands[0], *reinterpret_cast<int64_t *>(op.operands + 1));
            break;
        }
        case MOV_RM: {
            printf("MOVRM: %d, 0x%llu\n", op.operands[0], *reinterpret_cast<uint64_t *>(op.operands + 1));
            break;
        }
        case MOV_RR: {
            printf("MOVRR: %u, %u\n", op.operands[0], op.operands[1]);
            break;
        }
        case MOV_RC: {
            printf("MOVRC: %u, 0x%llu\n", op.operands[0], *reinterpret_cast<uint64_t *>(op.operands + 1));
            break;
        }
        case MOV_MI: {

        }
        case MOV_MM: {
        }
        case MOV_MR: {
        }
        case MOV_MC: {
        }
        case ADD: {
            printf("ADD: %u, %u, %u\n", op.operands[0], op.operands[1], op.operands[2]);
            break;
        }
        case SUB: {
            printf("SUB: %u, %u, %u\n", op.operands[0], op.operands[1], op.operands[2]);
            break;
        }
        case MUL: {
            printf("MUL: %u, %u, %u\n", op.operands[0], op.operands[1], op.operands[2]);
            break;
        }
        case DIV: {
            printf("DIV: %u, %u, %u\n", op.operands[0], op.operands[1], op.operands[2]);
            break;
        }
        case MOD: {
            printf("MOD: %u, %u, %u\n", op.operands[0], op.operands[1], op.operands[2]);
            break;
        }
        case POW: {
            printf("POW: %u, %u, %u\n", op.operands[0], op.operands[1], op.operands[2]);
            break;
        }
        case FCALL: {
            printf("FCALL: 0x%llx, %u\n", *reinterpret_cast<uint64_t *>(op.operands), op.operands[8]);
            break;
        }
        case FRET: {
            printf("FRET\n");
            break;
        }
        case HALT: {
            printf("HALT\n");
            break;
        }
        case DEBUG_LOG: {
            printf("DEBUG_LOG\n");
            break;
        }
        case JMP: {
            printf("JMP: 0x%llx\n", *reinterpret_cast<uint64_t *>(op.operands));
            break;
        }
        case CMP_GE: {
            printf("CMP_GE: %u, %u, %u\n", op.operands[0], op.operands[1], op.operands[2]);
            break;
        }
        case CMP_LT: {
            printf("CMP_LT: %u, %u, %u\n", op.operands[0], op.operands[1], op.operands[2]);
            break;
        }
        case CMP_LE: {
            printf("CMP_LE: %u, %u, %u\n", op.operands[0], op.operands[1], op.operands[2]);
            break;
        }
        case CMP_GT: {
            printf("CMP_GT: %u, %u, %u\n", op.operands[0], op.operands[1], op.operands[2]);
            break;
        }
        case CMP_EQ: {
            printf("CMP_EQ: %u, %u, %u\n", op.operands[0], op.operands[1], op.operands[2]);
            break;
        }
        case CMP_NE: {
            printf("CMP_NE: %u, %u, %u\n", op.operands[0], op.operands[1], op.operands[2]);
            break;
        }
        case IF_TRUE: {
            printf("IF_TRUE: %u, 0x%llx\n", op.operands[0], *reinterpret_cast<uint64_t *>(op.operands + 1));
            break;
        }
        case IF_FALSE: {
            printf("IF_FALSE: %u, 0x%llx\n", op.operands[0], *reinterpret_cast<uint64_t *>(op.operands + 1));
            break;
        }
        case LOCAL_GET: {
            printf("LOCAL_GET: %u, [%u, 0x%x]\n", op.operands[0], op.operands[1], *(uint16_t*)(op.operands + 2));
            break;
        }
        case LOCAL_SET: {
            printf("LOCAL_SET: [%u, 0x%x], %u\n", op.operands[0], *(uint16_t*)(op.operands + 1), op.operands[3]);
            break;
        }
        case FUNC_CREATE: {
            printf("FUNC_CREATE\n");
            break;
        }
        case FUNC_END: {
            printf("FUNC_END\n\n");
            break;
        }
        case AND: {
            printf("AND: %u, %u\n", op.operands[0], op.operands[1]);
            break;
        }
        case OR: {
            printf("OR: %u, %u\n", op.operands[0], op.operands[1]);
            break;
        }
        case VMC: {
            printf("VMC: %d\n", *(uint16_t*)op.operands);
            break;
        }
        case DEC: {
            printf("DEC: %u\n", op.operands[0]);
        }
        }
    }
    std::cout << std::flush;
}

void Generator::print_ops() {
    print_ops(ops);
}




} // namespace lmx
