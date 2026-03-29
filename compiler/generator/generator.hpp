//
// Created by geguj on 2025/12/28.
//

#pragma once
#include <bitset>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>
#include <memory>
#include <ranges>
#include <sstream>

#include "../ast.hpp"

#include "lmx_export.hpp"
#include "opcode.hpp"
#include "../common.hpp"
#include "../../runtime/libloader.hpp"

namespace lmx {

namespace runtime {
struct Op;
}

class LMC_API Allocator {
    std::bitset<REG_COUNT> bitset;
public:
    Allocator();
    size_t alloc();
    size_t alloc(size_t i);
    void free(size_t i);
    bool is_free(size_t i);
};
class LMC_API Generator {

    /* ===========================
     * expr_release
     * type: bool
     * 标记expr_ret_reg是否可以结束后释放
    =========================== */
    bool expr_release{false};


    size_t gen_program(std::shared_ptr<ASTNode>& n);
    size_t gen_binary(std::shared_ptr<ASTNode>& n);



    size_t gen_function(std::shared_ptr<ASTNode>& n);
    size_t gen_num(std::shared_ptr<ASTNode>& n);
    size_t gen_assign(std::shared_ptr<ASTNode>& n);
    size_t gen_var_ref(std::shared_ptr<ASTNode>& n);

    size_t gen_vmc(std::shared_ptr<ASTNode> &n);

    size_t gen_return(std::shared_ptr<ASTNode>& n);
    size_t gen_func_call(std::shared_ptr<ASTNode>& n);

    size_t basic_gen_func_call(std::shared_ptr<ASTNode> &n, size_t args_idx);

    void basic_gen_pass_args(std::vector<std::shared_ptr<ASTNode>> &args, size_t args_idx, size_t args_count);

    size_t gen_unary(std::shared_ptr<ASTNode>& n);

    size_t gen_string(std::shared_ptr<ASTNode>& n);

    size_t gen_bool(std::shared_ptr<ASTNode>& n);

    size_t gen_block(std::shared_ptr<ASTNode>& n);

    size_t basic_gen_block(std::shared_ptr<ASTNode> &n);

    size_t gen_module(std::shared_ptr<ASTNode> &shared);

    size_t gen_use(std::shared_ptr<ASTNode> &shared);

    [[nodiscard]] auto tagging() const { return ops.size(); }
    void new_frame(const std::string& name) { cur.push_back(std::make_unique<CompilingFrame>(name));}
    void free_frame() { cur.pop_back(); }
    size_t gen_if(std::shared_ptr<ASTNode>& n);



    static void error(const std::string& msg) {
        node_has_error = true;
        std::cerr << "Error: " << msg << std::endl;
    }

    struct CompilingFrame {
        std::string name;
        std::unordered_map<std::string, std::pair<bool, uint16_t>> locals; // name <mutable, here>

        uint16_t local_count{static_cast<uint16_t>(-1)};

        explicit CompilingFrame(std::string name): name(std::move(name))  {}

        CompilingFrame() = default;



        uint16_t new_var(const std::string& n, bool is_mut, uint16_t addr) {
            if (!locals.contains(n)) {
                if (addr > local_count) {
                    locals.reserve(addr + 1);
                    local_count = addr;
                }
                locals[n] = std::make_pair(is_mut, addr);
            } else error("redefined var: `" + n + "`");
            return local_count;
        }
        uint16_t new_var(const std::string& n, bool is_mut) {
            return new_var(n, is_mut, ++local_count);
        }
    };
    std::vector<std::unique_ptr<CompilingFrame>> cur;
    /* ====================================== *
         * function find_var(name)
         * find the var named `name` up to frames
         *
         * @return
         * 一般情况： （frame*, (mutable, idx))
         * 未找到情况： （nullptr, (false, UINT16_MAX))
         * ======================================= */
    auto find_var(const std::string& nme) const -> std::pair<size_t, std::pair<bool, uint16_t>> {
        size_t i = 0;
        for (const auto& c : cur) {
            for (const auto& [n, l] : c->locals)
                if (nme == n) return {i, l};
            i++;
        }
        // 未找到
        return {SIZE_MAX, {false,UINT16_MAX}};
    }
    std::unordered_map<std::string, std::pair<uint8_t, size_t>> funcs;
    void new_func(const std::string & name, uint8_t size) {
        if (!funcs.contains(name)) {
            funcs[name] = std::make_pair(size, tagging());
        } else error("redefined function: `" + name + "`");
    }
    void new_func(const std::string & name, uint8_t size, size_t addr) {
        if (!funcs.contains(name)) {
            funcs[name] = std::make_pair(size, addr);
        } else error("redefined function: `" + name + "`");
    }
    bool find_func(const std::string & name) const {
        return funcs.contains(name);
    }

    /*
     * extern funcs
     * name : args_type
     */
    std::unordered_map<
        std::string,
        std::pair<size_t,
            std::vector<    std::shared_ptr<TypeNode>   >
        >> extern_funcs;
    static inline runtime::CBasicTypes lmtype2ctype(std::string& lmt) {
        if (lmt.empty()) return runtime::Void;
        if (lmt == "bool") return runtime::CBasicTypes::Bool;
        if (lmt == "num") return runtime::CBasicTypes::LongLong;
        if (lmt == "text") return runtime::CBasicTypes::Ptr;
        if (lmt == "null") return runtime::CBasicTypes::Void;
        return runtime::CBasicTypes::NO_ENUM_VALUE;
    }
    std::vector<std::string> modules;
    std::stringstream cur_module;
    std::vector<std::string> using_modules;

    std::vector<size_t> break_points;
    std::vector<size_t> continue_points;
public:
    static bool node_has_error;
    Allocator regs;
    Generator();
    ~Generator() = default;

    std::vector<runtime::Op> ops;
    void write(runtime::Op& op);

    std::vector<runtime::Op> &get_ops();
    std::vector<char> constant_pool;

    size_t gen_loop(const std::shared_ptr<ASTNode> & shared);

    size_t gen_continue(std::shared_ptr<ASTNode> &n);

    size_t gen_break(std::shared_ptr<ASTNode> &n);

    size_t gen(std::shared_ptr<ASTNode> &n);

    [[nodiscard]] size_t togging() const {return ops.size();}

    void print_ops();
    static void print_ops(std::vector<runtime::Op>& ops);
    void write_binary_file(const std::string &path);

    void print_vars() {
        for (auto& v: cur)
            for (auto& [n, a] : v->locals)
                std::cout << n << " at address: " << a.second << " mutable: " << (a.first ? "true" : "false") << std::endl;
    }
};

}
