//
// Created by geguj on 2025/12/28.
//

#pragma once
#include <iostream>
#include <memory>

#include "../include/lmx_export.hpp"
#include "lexer.hpp"
#include "ast.hpp"

namespace lmx {

class LMC_API Parser {
    bool in_module{false};
    bool has_err{false};
    std::vector<Token>& tokens;
    size_t pos{0};

    void parse_args(std::vector<std::shared_ptr<ASTNode>> &args);

    void advance();
    [[nodiscard]] Token& cur() const;
    [[nodiscard]] bool match(TokenType t) const;
    [[nodiscard]] bool is_eof() const;
    std::shared_ptr<ExprNode> expr();
    std::shared_ptr<ExprNode> term();


    std::shared_ptr<ExprNode> factor();

    std::shared_ptr<ExprNode> parse_func_call();

    bool peek_match(TokenType type) const;

    void check_eof();

    void error(const std::string& msg);

    std::shared_ptr<BlockStmtNode> parse_block();

    std::shared_ptr<StringNode> parse_string();

    std::shared_ptr<ASTNode> parse_if();
    std::shared_ptr<ExprNode> parse_expr();

    std::shared_ptr<ExprNode> parse_logical_and();

    std::shared_ptr<ExprNode> parse_relational();

    std::shared_ptr<ExprNode> parse_logical_or();

    std::shared_ptr<ASTNode> parse_funcdecl(bool has_block);

public:
    explicit Parser(std::vector<Token>& tokens): tokens(tokens) {}

    std::shared_ptr<ASTNode> parse();

    std::shared_ptr<ASTNode> parse_module();

    std::shared_ptr<TypeNode> parse_type();


    std::shared_ptr<ProgramASTNode> parse_program();
    [[nodiscard]] bool error() const {return has_err;}
};

} // lmx