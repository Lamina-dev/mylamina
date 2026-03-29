#pragma once
#include <cstdint>
#include <fstream>
#include <vector>
#include "lmx_export.hpp"

namespace lmx::runtime {
struct Op;

class LMVM_API BinaryLoader {
    std::vector<Op> ops;
    std::vector<char> data;
    std::remove_reference<std::fstream &>::type file;
    struct Header {
        uint32_t magic, version;
    };
    bool check_head();
public:
    void load();
    std::vector<char>* get_data();
    std::vector<Op>* get_ops();
    explicit BinaryLoader(const std::string& path) noexcept(false);
    explicit BinaryLoader(std::fstream&& file) noexcept(true);

    ~BinaryLoader();
};

}
