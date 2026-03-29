//
// Created by geguj on 2025/12/28.
//

#pragma once
#include <string>
#include <vector>

#include "../include/lmx_export.hpp"

namespace lmx {
enum class /*LMC_API*/ TokenType {
    END_OF_FILE,
    OPER_PLUS, OPER_MINUS, OPER_MUL, OPER_DIV, OPER_MOD, OPER_POW,

    ASSIGN, COLON, COL_COLON, COMMA, NOT,

    LPAREN, RPAREN, LBRACK, RBRACK, LBRACE, RBRACE,

    EQ, NE, LT, GT, LE, GE, PIPE, OR, AND,

    NUM_LITERAL, STRING_LITERAL, TRUE_LITERAL, FALSE_LITERAL, IDENTIFIER,

    KW_FUNC, KW_RETURN,
    UNKNOWN, KW_IF, KW_ELSE, KW_LET,
    KW_VMC,
    KW_MODULE,
    KW_USE,
    DOT,
    KW_LOOP,
    KW_BREAK,
    KW_CONTINUE,
    COMMENT,
};

struct LMC_API Token {
    TokenType type;
    std::string text;
    size_t line, col;

    friend std::ostream& operator<<(std::ostream& os, const Token& t);
};

class LMC_API Lexer {
    size_t pos{0}, line{1}, col{1};

    void advance();
    std::string& src;

    Token next();
public:
    explicit Lexer(std::string& src): src(src) {};
    std::vector<Token> tokenize(const std::string &new_src);
};

}