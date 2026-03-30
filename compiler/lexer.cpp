//
// Created by geguj on 2025/12/28.
//

#include "lexer.hpp"

#include <unordered_map>
#include <iostream>
#include <regex>

namespace lmx {

std::ostream& operator<<(std::ostream& os, const Token& t) {
    os << "Token(";
    switch (t.type) {
    case TokenType::END_OF_FILE: os << "END_OF_FILE"; break;
    case TokenType::IDENTIFIER: os << "IDENTIFIER"; break;
    case TokenType::NUM_LITERAL: os << "NUM_LITERAL"; break;
    case TokenType::STRING_LITERAL: os << "STRING_LITERAL"; break;
    case TokenType::COMMA: os << "COMMA"; break;
    case TokenType::TRUE_LITERAL: os << "TRUE_LITERAL"; break;
    case TokenType::FALSE_LITERAL: os << "FALSE_LITERAL"; break;
    case TokenType::OPER_PLUS: os << "OPER_PLUS"; break;
    case TokenType::OPER_MINUS: os << "OPER_MINUS"; break;
    case TokenType::OPER_MUL: os << "OPER_MUL"; break;
    case TokenType::OPER_DIV: os << "OPER_DIV"; break;
    case TokenType::OPER_MOD: os << "OPER_MOD"; break;
    case TokenType::EQ: os << "EQ"; break;
    case TokenType::GE: os << "GE"; break;
    case TokenType::GT: os << "GT"; break;
    case TokenType::LE: os << "LE"; break;
    case TokenType::LT: os << "LT"; break;
    case TokenType::COLON: os << "COLON"; break;
    case TokenType::COL_COLON: os << "COL_COLON"; break;
    case TokenType::OPER_POW: os << "OPER_POW"; break;
    case TokenType::ASSIGN: os << "ASSIGN"; break;
    case TokenType::NOT: os << "NOT"; break;
    case TokenType::NE: os << "NE"; break;
    case TokenType::LPAREN: os << "LPAREN"; break;
    case TokenType::RPAREN: os << "RPAREN"; break;
    case TokenType::LBRACK: os << "LBRACK"; break;
    case TokenType::RBRACK: os << "RBRACK"; break;
    case TokenType::LBRACE: os << "LBRACE"; break;
    case TokenType::RBRACE: os << "RBRACE"; break;
    case TokenType::UNKNOWN: os << "UNKNOWN"; break;
    case TokenType::KW_FUNC: os << "KEYWORD_FUNC"; break;
    case TokenType::KW_RETURN: os << "KEYWORD_RETURN"; break;
    case TokenType::KW_IF: os << "KEYWORD_IF"; break;
    case TokenType::KW_ELSE: os << "KEYWORD_ELSE"; break;
    case TokenType::KW_LET: os << "KEYWORD_LET"; break;
    case TokenType::KW_VMC: os << "KEYWORD_VMC"; break;
    case TokenType::KW_MODULE: os << "KEYWORD_MODULE"; break;
    case TokenType::KW_USE: os << "KEYWORD_USE"; break;
    case TokenType::KW_LOOP: os << "KEYWORD_LOOP"; break;
    case TokenType::KW_BREAK: os << "KEYWORD_BREAK"; break;
    case TokenType::KW_CONTINUE: os << "KEYWORD_CONTINUE"; break;
    case TokenType::PIPE: os << "PIPE"; break;
    case TokenType::OR: os << "OR"; break;
    case TokenType::AND: os << "AND"; break;
    case TokenType::DOT: os << "DOT"; break;
    case TokenType::COMMENT: os << "COMMENT"; break;
    default: os << "UNKNOWN";
    }
    os << ", " << t.text << ", " << t.line << ", " << t.col << ')';
    return os;
}

// 定义token类型和对应的正则表达式
struct TokenPattern {
    TokenType type;
    std::regex pattern;
    TokenPattern(TokenType t, const std::string& regex_str) : type(t), pattern(regex_str) {}
};

// 优先级从高到低排序
static const std::vector<TokenPattern> token_patterns = {
    {TokenType::COMMENT, "^#.*?$"},
    {TokenType::STRING_LITERAL, R"(^"(\.|[^\"])*")"},
    {TokenType::EQ, "^=="},
    {TokenType::NE, "^!="},
    {TokenType::GE, "^>="},
    {TokenType::LE, "^<="},
    {TokenType::OR, "^\\|\\|"},
    {TokenType::AND, "^&&"},
    {TokenType::PIPE, "^\\|>"},
    {TokenType::COL_COLON, "^::"},
    {TokenType::OPER_PLUS, "^\\+"},
    {TokenType::OPER_MINUS, "^-"},
    {TokenType::OPER_MUL, "^\\*"},
    {TokenType::OPER_DIV, "^/"},
    {TokenType::OPER_MOD, "^%"},
    {TokenType::OPER_POW, "^\\^"},
    {TokenType::ASSIGN, "^="},
    {TokenType::NOT, "^!"},
    {TokenType::GT, "^>"},
    {TokenType::LT, "^<"},
    {TokenType::COLON, "^:"},
    {TokenType::LPAREN, "^\\("},
    {TokenType::RPAREN, "^\\)"},
    {TokenType::LBRACE, "^\\{"},
    {TokenType::RBRACE, "^\\}"},
    {TokenType::LBRACK, "^\\["},
    {TokenType::RBRACK, "^\\]"},
    {TokenType::COMMA, "^,"},
    {TokenType::DOT, "^\\."},
    {TokenType::NUM_LITERAL, "^\\d[_\\d]*"},
    {TokenType::IDENTIFIER, "^[a-zA-Z_][a-zA-Z0-9_]*"},
};

// 关键字映射
static const std::unordered_map<std::string, TokenType> keywords = {
    {"func", TokenType::KW_FUNC},
    {"return", TokenType::KW_RETURN},
    {"if", TokenType::KW_IF},
    {"else", TokenType::KW_ELSE},
    {"let", TokenType::KW_LET},
    {"__VMC", TokenType::KW_VMC},
    {"module", TokenType::KW_MODULE},
    {"use", TokenType::KW_USE},
    {"loop", TokenType::KW_LOOP},
    {"break", TokenType::KW_BREAK},
    {"continue", TokenType::KW_CONTINUE},
    {"true", TokenType::TRUE_LITERAL},
    {"false", TokenType::FALSE_LITERAL},
};

Token Lexer::next() {
    // 跳过空白字符
    while (pos < src.size() && isspace(src[pos])) {
        if (src[pos] == '\n') {
            line++;
            col = 1;
        } else {
            col++;
        }
        pos++;
    }

    if (pos >= src.size()) {
        return {TokenType::END_OF_FILE, "", line, col};
    }

    // 提取当前位置开始的子串
    const std::string remaining = src.substr(pos);

    // 尝试匹配所有token模式
    for (const auto& pattern : token_patterns) {
        std::smatch match;
        if (std::regex_search(remaining, match, pattern.pattern)) {
            const std::string matched_text = match.str(0);
            const size_t match_length = matched_text.size();
            
            // 保存当前位置信息
            const size_t token_line = line;
            const size_t token_col = col;

            // 更新位置信息
            for (const char c : matched_text) {
                if (c == '\n') {
                    line++;
                    col = 1;
                } else {
                    col++;
                }
            }
            pos += match_length;

            // 处理标识符和关键字
            if (pattern.type == TokenType::IDENTIFIER) {
                auto it = keywords.find(matched_text);
                if (it != keywords.end()) {
                    return {it->second, matched_text, token_line, token_col};
                }
            }

            // 处理字符串字面量（去除引号）
            if (pattern.type == TokenType::STRING_LITERAL) {
                // 去除首尾引号
                const std::string unquoted = matched_text.substr(1, matched_text.size() - 2);
                // 处理转义字符
                std::string processed;
                for (size_t i = 0; i < unquoted.size(); i++) {
                    if (unquoted[i] == '\\' && i + 1 < unquoted.size()) {
                        i++;
                        switch (unquoted[i]) {
                            case 'n': processed += '\n'; break;
                            case 't': processed += '\t'; break;
                            case 'r': processed += '\r'; break;
                            case 'b': processed += '\b'; break;
                            case 'f': processed += '\f'; break;
                            case 'v': processed += '\v'; break;
                            case '0': processed += '\0'; break;
                            default: processed += unquoted[i]; break;
                        }
                    } else {
                        processed += unquoted[i];
                    }
                }
                return {TokenType::STRING_LITERAL, processed, token_line, token_col};
            }

            // 处理数字字面量（去除下划线）
            if (pattern.type == TokenType::NUM_LITERAL) {
                std::string num_without_underscores;
                for (char c : matched_text) {
                    if (c != '_') {
                        num_without_underscores += c;
                    }
                }
                return {TokenType::NUM_LITERAL, num_without_underscores, token_line, token_col};
            }

            // 跳过注释
            if (pattern.type == TokenType::COMMENT) {
                return next();
            }

            return {pattern.type, matched_text, token_line, token_col};
        }
    }

    // 无法识别的字符
    const char unknown_char = src[pos];
    Token token = {TokenType::UNKNOWN, std::string(1, unknown_char), line, col};
    pos++;
    col++;
    return token;
}

std::vector<Token> Lexer::tokenize(const std::string& new_src) {
    src = new_src;
    pos = 0;
    line = 1;
    col = 1;
    std::vector<Token> tokens;
    
    while (pos < src.size()) {
        Token t = next();
        if (t.type != TokenType::COMMENT) {
            tokens.push_back(t);
        }
    }

    tokens.push_back({TokenType::END_OF_FILE, "", line, col});
    
    return tokens;
}

}