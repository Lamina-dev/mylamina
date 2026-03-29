//
// Created by geguj on 2025/12/28.
//

#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

#include "../include/lmx_export.hpp"

// Forward declarations to avoid circular dependencies
namespace lmx {
struct StringNode;
class Generator;
    enum class TokenType;
    struct Token;
}

namespace lmx {

enum ASTKind {
    Program,
    Binary, Unary, NumLiteral, StringLiteral, BoolLiteral,
    BlockStmt,
    IfStmt,
    VarDecl,
    VarRef,
    FuncDecl,
    FuncCallExpr,
    Return,
    VMCall,
    Module,
    Use,
    ExternFunc,
    Loop,
    Break,
    Continue
};

struct LMC_API ASTNode {
    ASTKind kind;

    virtual ~ASTNode() = default;
    explicit ASTNode(ASTKind kind) : kind(kind) {}
};

struct TypeNode {
    enum class Kind { Il, Co,}; //一般类型和复合类型
    std::string name;

    Kind kind{Kind::Co};
    
    explicit TypeNode(std::string name) : name(std::move(name)), kind(Kind::Co) {}
    virtual ~TypeNode() = default;
    
    TypeNode() = default;
};
struct CompositeTypeNode final : TypeNode {
    std::vector<std::shared_ptr<TypeNode>> others;

    CompositeTypeNode(std::string base, std::vector<std::shared_ptr<TypeNode>> others) :
        TypeNode(std::move(base)), others(std::move(others)) {}
};

struct LMC_API ProgramASTNode final : public ASTNode {
    std::vector<std::shared_ptr<ASTNode>> children;
    
    explicit ProgramASTNode(
        std::vector<std::shared_ptr<ASTNode>> children
    ) : ASTNode(Program),
        children(std::move(children)) {}
};

struct StmtNode : public ASTNode {
    explicit StmtNode(ASTKind kind) : ASTNode(kind) {}
    
    ~StmtNode() override = default;
};

struct ExprNode : public ASTNode {
    explicit ExprNode(ASTKind kind) : ASTNode(kind) {}
    
    ~ExprNode() override = default;
};

struct BlockStmtNode final : public StmtNode {
    std::vector<std::shared_ptr<ASTNode>> children;
    
    explicit BlockStmtNode(std::vector<std::shared_ptr<ASTNode>> children) 
        : StmtNode(ASTKind::BlockStmt), 
          children(std::move(children)) {}
};

struct IfStmtNode final : public StmtNode {
    std::shared_ptr<ASTNode> condition, thenBlock, elseBlock;
    
    explicit IfStmtNode(
        std::shared_ptr<ExprNode> condition,
        std::shared_ptr<BlockStmtNode> thenBlock,
        std::shared_ptr<BlockStmtNode> elseBlock
    ) : StmtNode(ASTKind::IfStmt),
        condition(std::move(condition)),
        thenBlock(std::move(thenBlock)),
        elseBlock(std::move(elseBlock)) {}

};
struct ExternFuncNode final : public ASTNode {
    std::string name;


    std::vector<std::string> args;

    /*
    * 不搞键值对是因为之前就这么写的，更改成本高
    */
    std::vector<std::shared_ptr<TypeNode>> args_type;
    std::shared_ptr<TypeNode> ret_type;

    std::shared_ptr<StringNode> extern_label;

    explicit ExternFuncNode(
        std::string name,
        std::vector<std::string> args,
        std::vector<std::shared_ptr<TypeNode>> args_type,
        std::shared_ptr<StringNode> extern_label,
        std::shared_ptr<TypeNode> ret_type)

           :ASTNode(ExternFunc),
            name(std::move(name)),
            args(std::move(args)),
            args_type(std::move(args_type)),
            ret_type(std::move(ret_type)),
            extern_label(std::move(extern_label)) {}
};
struct FuncDeclNode final : public ASTNode {
    std::string name;
    std::vector<std::string> args;
    std::vector<std::shared_ptr<TypeNode>> args_type;
    std::shared_ptr<TypeNode> ret_type;
    std::shared_ptr<ASTNode> body;
    
    explicit FuncDeclNode(
        std::string name,
        std::vector<std::string> args,
        std::shared_ptr<BlockStmtNode> body
    ) : ASTNode(ASTKind::FuncDecl),
        name(std::move(name)), 
        args(std::move(args)), 
        body(std::move(body)) {}
};

struct ReturnStmtNode final : public StmtNode {
    std::shared_ptr<ASTNode> expr;
    
    explicit ReturnStmtNode(std::shared_ptr<ExprNode> expr) 
        : StmtNode(ASTKind::Return), 
          expr(std::move(expr)) {}
};

struct FuncCallExprNode final : public ExprNode {
    std::string name;
    std::vector<std::shared_ptr<ASTNode>> args;
    
    FuncCallExprNode(
        std::string name,
        std::vector<std::shared_ptr<ASTNode>> args
    ) : ExprNode(ASTKind::FuncCallExpr), 
        name(std::move(name)), 
        args(std::move(args)) {}
};

struct VarDeclNode final : public ASTNode {
    std::string name;
    std::shared_ptr<ASTNode> value;
    bool is_mut;
    
    explicit VarDeclNode(
        std::string name,
        std::shared_ptr<ASTNode> value,
        bool is_mut = true
    ) : ASTNode(VarDecl), 
        name(std::move(name)), 
        value(std::move(value)), 
        is_mut(is_mut) {}
};

struct VarRefNode final : public ExprNode {
    std::string name;
    
    explicit VarRefNode(std::string name) 
        : ExprNode(ASTKind::VarRef), 
          name(std::move(name)) {}
};

struct NumberNode final : public ExprNode {
    std::string num;
    
    explicit NumberNode(std::string num) 
        : ExprNode(ASTKind::NumLiteral), 
          num(std::move(num)) {}
    
    ~NumberNode() override = default;
};
struct BoolNode final : public ExprNode {
    bool b;
    explicit BoolNode(bool b)
        : ExprNode(ASTKind::BoolLiteral),
            b(b) {}

    ~BoolNode() override = default;
};
struct StringNode final : public ExprNode {
    std::string str;
    explicit StringNode(std::string str)
        : ExprNode(ASTKind::StringLiteral), str(std::move(str)) {}
    ~StringNode() override = default;
};

struct BinaryNode final : public ExprNode {
    std::shared_ptr<ASTNode> left;
    std::shared_ptr<ASTNode> right;
    std::string op;
    
    BinaryNode(
        std::shared_ptr<ASTNode> left,
        std::shared_ptr<ASTNode> right,
        std::string op
    ) : ExprNode(ASTKind::Binary), 
        left(std::move(left)), 
        right(std::move(right)), 
        op(std::move(op)) {}
};

struct UnaryNode final : public ExprNode {
    std::string op;
    std::shared_ptr<ASTNode> operand;
    
    explicit UnaryNode(std::string op, std::shared_ptr<ASTNode> operand) 
        : ExprNode(ASTKind::Unary), 
          op(std::move(op)), 
          operand(std::move(operand)) {}
};

struct VMCallNode final : public ExprNode {

    std::string idx;
    std::vector<std::shared_ptr<ASTNode>> args;

    explicit VMCallNode(std::string idx, std::vector<std::shared_ptr<ASTNode>> args) :
        ExprNode(VMCall) ,idx(std::move(idx)), args(std::move(args)) {}

};

struct ModuleNode final : public StmtNode {
    std::string name;
    enum class Types { ord, dyn };
    Types type;
    std::shared_ptr<StringNode> lib;    //可能为空
    std::vector<std::shared_ptr<VarDeclNode>> vars;     // 变量定义
    std::vector<std::shared_ptr<FuncDeclNode>> ord;     // 一般函数定义
    std::vector<std::shared_ptr<ExternFuncNode>> dyn;     // 动态函数定义
    std::vector<std::shared_ptr<ModuleNode>> chd;     // 子模块

    explicit ModuleNode(
        std::string name,
        Types type,
        std::shared_ptr<StringNode> lib,
        std::vector<std::shared_ptr<VarDeclNode>> vars,
        std::vector<std::shared_ptr<FuncDeclNode>> ord,
        std::vector<std::shared_ptr<ExternFuncNode>> dyn,
        std::vector<std::shared_ptr<ModuleNode>>   chd) :
            StmtNode(Module),
            name(std::move(name)),
    type(type),
    lib(std::move(lib)),
    vars(std::move(vars)),
    ord(std::move(ord)),
    dyn(std::move(dyn)),
    chd(std::move(chd)) {}
};
struct UseNode final : public StmtNode {
    std::shared_ptr<StringNode> path;

    explicit UseNode(std::shared_ptr<StringNode> path) : StmtNode(Use), path(std::move(path)) {}
};

struct LoopNode final : public StmtNode {
    std::shared_ptr<ASTNode> condition, body;

    explicit LoopNode(std::shared_ptr<ExprNode> condition, std::shared_ptr<BlockStmtNode> body)
        : StmtNode(Loop), condition(std::move(condition)), body(std::move(body)) {}
};
struct BreakNode final : public StmtNode {
    BreakNode(): StmtNode(Break) {}
};
struct ContinueNode final : public StmtNode {
    ContinueNode(): StmtNode(Continue) {}
};

} // namespace lmx
