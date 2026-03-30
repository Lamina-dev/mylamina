#include "repl.hpp"
#include "../compiler/lexer.hpp"
#include "../compiler/parser.hpp"
#include "../compiler/generator/generator.hpp"
#include "../compiler/generator/emit.hpp"
#include "../runtime/vm.hpp"
#include "../compiler/ast.hpp"
#include "../../../include/debug.hpp"

void print_ast(const std::shared_ptr<lmx::ASTNode>& node, const int indent = 0) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) {
        std::cerr << "  ";
    }
    
    switch (node->kind) {
        case lmx::ASTKind::Program:
            std::cerr << "Program" << std::endl;
            for (const auto& child : dynamic_cast<lmx::ProgramASTNode*>(node.get())->children) {
                print_ast(child, indent + 1);
            }
            break;
        case lmx::ASTKind::NumLiteral:
            std::cerr << "NumLiteral(" << dynamic_cast<lmx::NumberNode*>(node.get())->num << ")" << std::endl;
            break;
        case lmx::ASTKind::StringLiteral:
            std::cerr << "StringLiteral(\"" << dynamic_cast<lmx::StringNode*>(node.get())->str << "\")" << std::endl;
            break;
        case lmx::ASTKind::BoolLiteral:
            std::cerr << "BoolLiteral(" << (dynamic_cast<lmx::BoolNode*>(node.get())->b ? "true" : "false") << ")" << std::endl;
            break;
        case lmx::ASTKind::VarDecl:
            {
                auto decl = dynamic_cast<lmx::VarDeclNode*>(node.get());
                std::cerr << "VarDecl(" << decl->name << ", mutable=" << (decl->is_mut ? "true" : "false") << ")" << std::endl;
                if (decl->value) {
                    print_ast(decl->value, indent + 1);
                }
            }
            break;
        case lmx::ASTKind::VarRef:
            std::cerr << "VarRef(" << dynamic_cast<lmx::VarRefNode*>(node.get())->name << ")" << std::endl;
            break;
        case lmx::ASTKind::FuncDecl:
            {
                auto func = dynamic_cast<lmx::FuncDeclNode*>(node.get());
                std::cerr << "FuncDecl(" << func->name << ")" << std::endl;
                if (func->body) {
                    print_ast(func->body, indent + 1);
                }
            }
            break;
        case lmx::ASTKind::FuncCallExpr:
            {
                auto call = dynamic_cast<lmx::FuncCallExprNode*>(node.get());
                std::cerr << "FuncCallExpr(" << call->name << ")" << std::endl;
                for (const auto& arg : call->args) {
                    print_ast(arg, indent + 1);
                }
            }
            break;
        case lmx::ASTKind::Binary:
            {
                auto bin = dynamic_cast<lmx::BinaryNode*>(node.get());
                std::cerr << "Binary(" << bin->op << ")" << std::endl;
                print_ast(bin->left, indent + 1);
                print_ast(bin->right, indent + 1);
            }
            break;
        case lmx::ASTKind::Unary:
            {
                auto unary = dynamic_cast<lmx::UnaryNode*>(node.get());
                std::cerr << "Unary(" << unary->op << ")" << std::endl;
                print_ast(unary->operand, indent + 1);
            }
            break;
        case lmx::ASTKind::BlockStmt:
            {
                std::cerr << "BlockStmt" << std::endl;
                for (const auto& child : dynamic_cast<lmx::BlockStmtNode*>(node.get())->children) {
                    print_ast(child, indent + 1);
                }
            }
            break;
        case lmx::ASTKind::IfStmt:
            {
                auto if_stmt = dynamic_cast<lmx::IfStmtNode*>(node.get());
                std::cerr << "IfStmt" << std::endl;
                std::cerr << "  Condition:" << std::endl;
                print_ast(if_stmt->condition, indent + 2);
                std::cerr << "  Then:" << std::endl;
                print_ast(if_stmt->thenBlock, indent + 2);
                if (if_stmt->elseBlock) {
                    std::cerr << "  Else:" << std::endl;
                    print_ast(if_stmt->elseBlock, indent + 2);
                }
            }
            break;
        case lmx::ASTKind::Return:
            {
                auto ret = dynamic_cast<lmx::ReturnStmtNode*>(node.get());
                std::cerr << "Return" << std::endl;
                if (ret->expr) {
                    print_ast(ret->expr, indent + 1);
                }
            }
            break;
        case lmx::ASTKind::VMCall:
            {
                auto vmcall = dynamic_cast<lmx::VMCallNode*>(node.get());
                std::cerr << "VMCall(" << vmcall->idx << ")" << std::endl;
                for (const auto& arg : vmcall->args) {
                    print_ast(arg, indent + 1);
                }
            }
            break;
        case lmx::ASTKind::Module:
            {
                auto module = dynamic_cast<lmx::ModuleNode*>(node.get());
                std::cerr << "Module(" << module->name << ")" << std::endl;
            }
            break;
        case lmx::ASTKind::Use:
            {
                auto use = dynamic_cast<lmx::UseNode*>(node.get());
                std::cerr << "Use(" << use->path->str << ")" << std::endl;
            }
            break;
        case lmx::ASTKind::Loop:
            {
                auto loop = dynamic_cast<lmx::LoopNode*>(node.get());
                std::cerr << "Loop" << std::endl;
                std::cerr << "  Condition:" << std::endl;
                print_ast(loop->condition, indent + 2);
                std::cerr << "  Body:" << std::endl;
                print_ast(loop->body, indent + 2);
            }
            break;
        case lmx::ASTKind::Break:
            std::cerr << "Break" << std::endl;
            break;
        case lmx::ASTKind::Continue:
            std::cerr << "Continue" << std::endl;
            break;
        default:
            std::cerr << "Unknown AST node type" << std::endl;
            break;
    }
}

int run_repl() {
    std::string expr;
    lmx::Lexer l(expr);
    lmx::Generator generator;
    lmx::runtime::VirtualCore core;
    core.set_program(&generator.ops);

    const std::string prompt = std::string(COLOR_MAGENTA) + ">>> " + COLOR_RESET;
    while (true) {
        std::cout << prompt << std::flush;
        if (!std::getline(std::cin, expr)) break;
        if (expr == ":lastret") std::cout << core.look_register(0) << std::endl;
        else if (expr == ":exit") break;
        else if (expr == ":op") generator.print_ops();
        else if (expr == ":vars") generator.print_vars();
        else {
            // Reset error flag before processing each input
            lmx::Generator::node_has_error = false;
            
            // Tokenize and display tokens
            std::vector<lmx::Token> tks = l.tokenize(expr);
            DEBUG_TOKEN_LIST(tks);
            
            // Parse and display AST
            lmx::Parser parser(tks, expr, "<shell#>");
            auto node = parser.parse();
            if (!node || parser.error()) continue;
            
            DEBUG_SEPARATOR("ABSTRACT SYNTAX TREE");
            #ifdef DEBUG_OUTPUT
                std::cerr << COLOR_MAGENTA;
                print_ast(node);
                std::cerr << COLOR_RESET;
            #endif
            DEBUG_SEPARATOR("AST END");
            
            // Generate bytecode and display it
            const auto op = generator.gen(node);
            if (lmx::Generator::node_has_error) continue;
            
            DEBUG_SEPARATOR("GENERATED BYTECODE");
            #ifdef DEBUG_OUTPUT
                std::cerr << COLOR_CYAN;
                generator.print_ops();
                std::cerr << COLOR_RESET;
            #endif
            DEBUG_SEPARATOR("BYTECODE END");
            
            // Execute
            generator.ops.emplace_back(lmx::runtime::Opcode::HALT);
            core.set_constant(generator.constant_pool.data());
            int result = core.run();

            if (op != -1 && result == 0) {
                generator.regs.free(op);
                auto& value = core.get_register(op);
                if (value.type == lmx::runtime::ValueType::Null) {
                } else if (value.type == lmx::runtime::ValueType::Ptr) {
                    std::cout << "<ptr>" << std::endl;
                } else {
                    switch (value.type) {
                        case lmx::runtime::ValueType::Int:
                            std::cout << value.i64 << std::endl;
                            break;
                        case lmx::runtime::ValueType::Float:
                            std::cout << value.f64 << std::endl;
                            break;
                        case lmx::runtime::ValueType::Bool:
                            std::cout << (value.b ? "true" : "false") << std::endl;
                            break;
                        case lmx::runtime::ValueType::Str:
                            std::cout << "\"" << value.str << "\"" << std::endl;
                            break;
                        default:
                            std::cout << value.to_string() << std::endl;
                            break;
                    }
                }
            }
            if (generator.ops.back().op == lmx::runtime::Opcode::HALT) generator.ops.pop_back();
        }
    }
    return 0;

}
