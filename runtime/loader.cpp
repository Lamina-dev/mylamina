//
// Created by geguj on 2026/2/9.
//

#include "loader.hpp"

#include <filesystem>
#include <iostream>

#include "lmx_export.hpp"
#include "opcode.hpp"

namespace lmx::runtime {
BinaryLoader::BinaryLoader(const std::string &path) noexcept(false) : file(path, std::ios::binary) {
    if (!file.is_open()) {
        file.close();
        throw std::runtime_error("Failed to open file " + path);
    }
}
BinaryLoader::BinaryLoader(std::fstream&& file) noexcept(true) : file(std::move(file)){}
BinaryLoader::~BinaryLoader() {
    if (file.is_open()) file.close();
}


bool BinaryLoader::check_head() {
    file.seekg(0, std::ios::beg);
    static Header h;

    file.read(reinterpret_cast<char*>(&h), sizeof(h));
    return h.magic == LMX_MAGIC_NUM && h.version == LMX_VERSION;
}

void BinaryLoader::load() {
    if (!check_head()) {
        std::cerr << "Loader: binary file format bad" << std::endl;
        exit(-1);
    }
    while (true) {
        Op op;
        //std::cout << (int)op.op << std::endl;
        op.op = static_cast<Opcode>(file.get());
        file.read(reinterpret_cast<char*>(op.operands), opcode_len(op.op));
        ops.push_back(op);
        if (op.op == Opcode::HALT)break;
    }
    data = std::vector(
        std::istreambuf_iterator(file),
        std::istreambuf_iterator<char>()
        );
}

std::vector<char> *BinaryLoader::get_data() {
    return &data;
}
std::vector<Op> *BinaryLoader::get_ops() {
    return &ops;
}

}
