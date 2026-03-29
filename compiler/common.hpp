//
// Common definitions for compiler module
//

#pragma once
#include <string>
#include <vector>
#include <filesystem>

#include "ast.hpp"
#include "lexer.hpp"
#include <fstream>
#include <iostream>

#include "parser.hpp"

namespace lmx {
    class Generator;
#define REG_COUNT 255
#define REG_COUNT_INDEX_MAX 254
    inline std::vector<std::filesystem::path> module_path;

    inline std::string read_file(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open file " << path << std::endl;
            return {};
        }
        return std::string(std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{});
    }
    inline std::shared_ptr<ASTNode> compile(const std::string &path) {
        std::string code = read_file(path);
        if (code.empty())  return nullptr;
        Lexer lexer(code);
        auto tks = lexer.tokenize(code);

        Parser parser(tks);
        if (auto node = parser.parse_program(); node && !parser.error()) return node;
        return nullptr;
    }
}
