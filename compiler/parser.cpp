//
// Created by geguj on 2025/12/28.
//

#include "parser.hpp"

#include <format>
#include <stack>

#include "../include/opcode.hpp"

namespace lmx {

void Parser::parse_args(std::vector<std::shared_ptr<ASTNode>> &args) {
    if (!match(TokenType::LPAREN)) {
        advance();
        error("expected '(')");
        return;
    }
    advance();
    while (true) {
        if (match(TokenType::RPAREN)) {
            advance();
            break;
        }
        args.push_back(parse_expr());
        if (match(TokenType::COMMA)) advance();
        else if (match(TokenType::RPAREN)) {
            advance();
            break;
        }
        else {
            error("Missing ',' or ')'");
            advance();
            break;
        }
    }
}

void Parser::advance() {
    if (pos < tokens.size()) {
        pos++;
        if (cur().type == TokenType::COMMENT) advance();
        if (cur().type == TokenType::UNKNOWN) error("unknown token: `" + cur().text + "`");
    }
}
#define check_type(op) if (match(TokenType::COLON)) { \
advance();\
op(parse_type());\
}
Token& Parser::cur() const {
    if (pos >= tokens.size()) {
        return tokens.back();
    }
    return tokens[pos];
}

bool Parser::match(TokenType t) const {
    return cur().type == t;
}

bool Parser::is_eof() const {
    return pos >= tokens.size();
}

void Parser::check_eof() {
    while (!is_eof() && match(TokenType::END_OF_FILE)) {
        advance();
    }
    //if (!is_eof()) {
    //    error("Expected end of file");
    //}
}
void Parser::error(const std::string& msg) {
    has_err = true;
    std::cerr << "Error: " << msg << " at " << cur().line << ":" << cur().col << std::endl;
}
std::shared_ptr<BlockStmtNode> Parser::parse_block() {
    if (!match(TokenType::LBRACE)) error("expected '{'");
    advance();
    std::vector<std::shared_ptr<ASTNode>> stmts;
    while (!match(TokenType::RBRACE) && !is_eof()) {
        if (const auto stmt = parse()) stmts.push_back(stmt);
    }
    if (!match(TokenType::RBRACE)) error("expected '}'");
    advance();
    return std::make_shared<BlockStmtNode>(stmts);
}
std::shared_ptr<ExprNode> Parser::parse_expr() {
    std::shared_ptr<ExprNode> node = parse_logical_and();
    std::shared_ptr<TypeNode> type;
    while (match(TokenType::PIPE) ) {
        auto op = cur().text;
        advance();
        auto n2 = parse_logical_and();
        if (n2->kind != FuncCallExpr) {
            error("expected func calling expr");
        }
        node = std::make_shared<BinaryNode>(node, n2, op);
    }
    check_type(type =);
    return node;
}
std::shared_ptr<ExprNode> Parser::parse_logical_and() {
    std::shared_ptr<ExprNode> node = parse_logical_or();
    while (match(TokenType::AND) ) {
        auto op = cur().text;
        advance();
        node = std::make_shared<BinaryNode>(node, parse_logical_or(), op);
    }
    return node;
}
std::shared_ptr<ExprNode> Parser::parse_logical_or() {
    std::shared_ptr<ExprNode> node = parse_relational();
    while (match(TokenType::OR) ) {
        auto op = cur().text;
        advance();
        node = std::make_shared<BinaryNode>(node, parse_relational(), op);
    }
    return node;
}
std::shared_ptr<ExprNode> Parser::parse_relational() {
    std::shared_ptr<ExprNode> node = expr();
    while (match(TokenType::EQ) || match(TokenType::LT) || match(TokenType::GT) ||
        match(TokenType::LE) || match(TokenType::GE) || match(TokenType::NE)    ) {
        auto op = cur().text;
        advance();
        node = std::make_shared<BinaryNode>(node, parse_expr(), op);
    }
    return node;
}
std::shared_ptr<ASTNode> Parser::parse_if() {
    if (!match(TokenType::LPAREN)) error("expected '('");
    advance();
    std::shared_ptr<ExprNode> condition = parse_expr();
    if (!match(TokenType::RPAREN)) error("expected ')'");

    advance();
    std::shared_ptr<BlockStmtNode> then_block = parse_block();
    std::shared_ptr<BlockStmtNode> else_block = nullptr;
    if (match(TokenType::KW_ELSE)) {
        advance();
        if (match(TokenType::KW_IF)) {
            advance();
            else_block = std::make_shared<BlockStmtNode>(std::vector<std::shared_ptr<ASTNode>>{});
            else_block->children.push_back(parse_if());
        } else {
            else_block = parse_block();
        }
    }
    return std::make_shared<IfStmtNode>(condition, then_block, else_block);
}
std::shared_ptr<ASTNode> Parser::parse() {
    static bool in_func = false;
    static bool in_loop = false;
    std::shared_ptr<ASTNode> node;
    re_parse:
    switch (cur().type) {
    case TokenType::COMMENT: advance(); goto re_parse;
    case TokenType::KW_LET: {
        advance();
        if (!match(TokenType::IDENTIFIER)) error("expected identifier");
        auto name = cur().text;
        advance();
        if (!match(TokenType::ASSIGN)) error("expected assignment");
        advance();
        node = std::make_shared<VarDeclNode>(name, parse_expr(), false);
        break;
    }
    case TokenType::KW_FUNC: {
        advance();
        in_func = true;
        node = parse_funcdecl(true);
        in_func = false;
        break;
    }
    case TokenType::KW_RETURN: {
        if (!in_func) error("expected 'return'");

        auto line = cur().line;
        advance();
        if (cur().line > line) {
            node = std::make_shared<ReturnStmtNode>(nullptr);
        } else {
            auto e = parse_expr();

            node = std::make_shared<ReturnStmtNode>(e);
        }
        break;
    }
    case TokenType::KW_LOOP: {
        advance();
        std::shared_ptr<ExprNode> cond = nullptr;
        if (!match(TokenType::LBRACE))
            cond = parse_expr();
        in_loop = true;
        auto block = parse_block();
        node = std::make_shared<LoopNode>(cond, block);
        in_loop = false;
        break;
    }
    case TokenType::KW_BREAK: {
        advance();
        if (!in_loop) error("expected 'break', but not in loop");
        node = std::make_shared<BreakNode>();
        break;
    }
    case TokenType::KW_CONTINUE: {
        advance();
        if (!in_loop) error("expected 'continue', but not in loop");
        node = std::make_shared<ContinueNode>();
        break;
    }
    case TokenType::KW_IF: {
        advance();
        node = parse_if();
        break;
    }
    case TokenType::KW_MODULE: {
        advance();
        node = parse_module();
        break;
    }
    case TokenType::KW_USE: {
        advance();
        auto path = parse_string();
        break;
    }
    default: {
        if (match(TokenType::IDENTIFIER) && peek_match(TokenType::ASSIGN)) {
            auto name = cur().text;
            advance();
            advance();
            node = std::make_shared<VarDeclNode>(name, parse_expr());
        } else node = parse_expr();
        break;
    }
    }
    return node;
}
std::shared_ptr<ASTNode> Parser::parse_module() {
    if (!match(TokenType::IDENTIFIER)) {
        error("expected identifier");
        return nullptr;
    }

    auto name = cur().text;
    ModuleNode::Types type = ModuleNode::Types::ord;
    std::shared_ptr<StringNode> lib = nullptr;

    advance();
    if (match(TokenType::COLON)) {
        advance();
        lib = std::make_shared<StringNode>(cur().text);
        type = ModuleNode::Types::dyn;
        advance();
    }
    in_module = true;
    if (!match(TokenType::LBRACE)) {
        error("expected '{'");
        advance();
        return nullptr;
    }

    advance();

    std::vector<std::shared_ptr<VarDeclNode>> vars;
    std::vector<std::shared_ptr<FuncDeclNode>> ord;
    std::vector<std::shared_ptr<ExternFuncNode>> dyn;
    std::vector<std::shared_ptr<ModuleNode>>   chd;

    while (!match(TokenType::RBRACE)) {
        auto node = parse();
        switch (node->kind) {
        case VarDecl: vars.push_back(static_pointer_cast<VarDeclNode>(node));break;
        case FuncDecl: ord.push_back(static_pointer_cast<FuncDeclNode>(node));break;
        case ExternFunc: dyn.push_back(static_pointer_cast<ExternFuncNode>(node));break;
        case Module: chd.push_back(static_pointer_cast<ModuleNode>(node));break;
        default: {
            error("in module only decl `var, func`");
            return nullptr;
        }
        }
    }
    advance();
    in_module = false;

    return std::make_shared<ModuleNode>(name, type, lib, vars, ord, dyn, chd);
}

std::shared_ptr<TypeNode> Parser::parse_type() {
    if (!match(TokenType::IDENTIFIER)) {
        error("expected identifier");
        return nullptr;
    }
    auto basic = cur().text;
    advance();
    if (match(TokenType::LBRACK)) {
        advance();
        std::vector<std::shared_ptr<TypeNode>> others;
        while (true) {
            others.push_back(parse_type());
            if (match(TokenType::RBRACK)) break;
            if (match(TokenType::COMMA)) {
                advance();
            } else {
                error("expected ','");
                advance();
                return nullptr;
            }
        }
        advance();
        return std::make_shared<CompositeTypeNode>(basic, others);
    }
    return std::make_shared<TypeNode>(basic);
}

std::shared_ptr<ASTNode> Parser::parse_funcdecl(const bool has_block = true) {
    if (!match(TokenType::IDENTIFIER)) {
        advance();
        error("expected identifier");
        return nullptr;
    }
    auto name = cur().text;
    advance();
    if (!match(TokenType::LPAREN)) {
        advance();
        error("expected '('");
        return nullptr;
    }
    advance();
    std::vector<std::string> params;
    std::vector<std::shared_ptr<TypeNode>> args_type;
    std::shared_ptr<TypeNode> ret_type = nullptr;
    while (true) {
        if (match(TokenType::IDENTIFIER)) {
            params.push_back(cur().text);
            advance();
            check_type(args_type.push_back)
        } else if (match(TokenType::RPAREN)) {
            advance();
            break;
        } else if (match(TokenType::COMMA)) {
            advance();
        } else {
            advance();
            error("expected identifier, ',' or ')'");
            break;
        }
    }
    check_type(ret_type =)
    if (match(TokenType::LBRACE)) {    // 一般 定义情况
        auto node = std::make_shared<FuncDeclNode>(name, params, parse_block());
        node->args_type = std::move(args_type);
        node->ret_type = std::move(ret_type);
        return node;
    } else if (match(TokenType::ASSIGN)) {  // 外部导入情况
        advance();
        if (!match(TokenType::STRING_LITERAL)) {
            error("expected string literal");
            advance();
            return nullptr;
        }
        auto node = std::make_shared<ExternFuncNode>(name, params, args_type, parse_string(), ret_type);
        check_type(ret_type =)
        node->args_type = std::move(args_type);
        node->ret_type = std::move(ret_type);
        return node;
    } else {    // 仅声明情况
        auto node = std::make_shared<FuncDeclNode>(name, params, nullptr);
        node->args_type = std::move(args_type);
        node->ret_type = std::move(ret_type);
        return node;
    }
}

std::shared_ptr<ExprNode> Parser::expr() {
    auto node = term();
    while (match(TokenType::OPER_PLUS) || match(TokenType::OPER_MINUS)) {
        auto op = cur().text;
        advance();
        node = std::make_shared<BinaryNode>(node, term(), op);
    }
    return node;
}
std::shared_ptr<ExprNode> Parser::term() {
    auto node = factor();
    while (match(TokenType::OPER_MUL) || match(TokenType::OPER_DIV) || match(TokenType::OPER_MOD) || match(TokenType::OPER_POW)) {
        auto op = cur().text;
        advance();
        node = std::make_shared<BinaryNode>(node, factor(), op);
    }
    return node;
}
std::shared_ptr<ProgramASTNode> Parser::parse_program() {
    std::vector<std::shared_ptr<ASTNode>> stmts;
    while (!match(TokenType::RBRACE) && !is_eof()) {
        if (const auto stmt = parse()) stmts.push_back(stmt);
    }
    return std::make_shared<ProgramASTNode>(stmts);
}

std::shared_ptr<ExprNode> Parser::factor() {
    std::shared_ptr<ExprNode> fact = nullptr;
    if (match(TokenType::NUM_LITERAL)) {
        fact = std::make_shared<NumberNode>(cur().text);
        advance();
    } else if (match(TokenType::LPAREN)) {
        advance();
        fact = parse_expr();
        if (match(TokenType::RPAREN)) {
            advance();
        } else {
            error("Missing closing ')'");
        }
    } else if (match(TokenType::OPER_MINUS)) {
        auto op = cur().text;
        advance();
        fact = std::make_shared<UnaryNode>(op, parse_expr());
    } else if (match(TokenType::IDENTIFIER)) {
        auto name = cur().text;
        advance();
        while (match(TokenType::DOT)) {
            name += cur().text;
            advance();
            if (match(TokenType::IDENTIFIER)) {
                name += cur().text;
                advance();
            }
        }
        if (!match(TokenType::LPAREN)) {
            fact = make_shared<VarRefNode>(name);
        }
        else {
            std::vector<std::shared_ptr<ASTNode>> args;
            parse_args(args);
            fact = std::make_shared<FuncCallExprNode>(name, args);
        }
    } else if (match(TokenType::TRUE_LITERAL) || match(TokenType::FALSE_LITERAL)) {
        fact = std::make_shared<BoolNode>(cur().text == "true");
        advance();
    } else if (match(TokenType::END_OF_FILE)) {
        advance();
    } else if (match(TokenType::STRING_LITERAL)) {
        fact = parse_string();
    } else if (match(TokenType::KW_VMC)) {
        advance();
        if (!match(TokenType::NUM_LITERAL)) {
            advance();
            error("expected numeric literal");
            return nullptr;
        }
        auto idx = cur().text;
        advance();
        std::vector<std::shared_ptr<ASTNode>> args;
        parse_args(args);

        fact = std::make_shared<VMCallNode>(idx, std::move(args));
    }
    else {
        error("unknown token: `" + cur().text + "`");
        advance();
    }
    return fact;
}

std::shared_ptr<StringNode> Parser::parse_string() {
    if (!match(TokenType::STRING_LITERAL)) {
        error("expected string literal");
        advance();
        return nullptr;
    }
    auto fact = std::make_shared<StringNode>(cur().text);
    advance();
    return fact;
}

bool Parser::peek_match(TokenType type) const {
    return tokens[pos + 1].type == type;
}
} // lmx