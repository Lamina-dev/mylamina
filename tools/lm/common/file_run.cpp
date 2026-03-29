#include "file_run.hpp"

#include <string>
#include <fstream>

#include "../runtime/loader.hpp"
#include "../compiler/lexer.hpp"
#include "../compiler/parser.hpp"
#include "../compiler/generator/generator.hpp"
#include "../runtime/vm.hpp"

int binary_run(std::fstream&& file) {
    lmx::runtime::BinaryLoader loader{std::move(file)};
    loader.load();
    lmx::runtime::VirtualCore vm;
    vm.set_constant(loader.get_data()->data());
    vm.set_program(loader.get_ops());
    return vm.run();
}
int file_run(const std::string& file_name) {
    uint32_t magic = 0;
    std::fstream file(file_name);
    if (!file.is_open()) {
        std::cerr << "File not found: " << file_name << std::endl;
        file.close();
        return -1;
    }
    file.read((char*)&magic, sizeof(magic));
    if (magic == LMX_MAGIC_NUM) return binary_run(std::move(file));
    file.seekg(0, std::ios::beg);
    auto src = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    lmx::Lexer lexer(src);
    auto ts = lexer.tokenize(src);
    lmx::Parser parser(ts);
    lmx::Generator gener;
    std::shared_ptr<lmx::ASTNode> node = parser.parse_program();
    if (!node || parser.error()) return -1;
    gener.gen(node);
    if (lmx::Generator::node_has_error)return -1;
    gener.ops.emplace_back(lmx::runtime::Opcode::HALT);
    gener.write_binary_file(file_name);
    //gener.print_ops();
    //return 0;
    lmx::runtime::VirtualCore vm;
    vm.set_program(&gener.ops);
    vm.set_constant(gener.constant_pool.data());

    return vm.run();
}
