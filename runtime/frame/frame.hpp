//
// Created by geguj on 2026/1/30.
//
#pragma once
#include <vector>
#include "../value/value.hpp"


namespace lmx::runtime {
    struct StackFrame {
        // size_t local_pc{};
        std::vector<Value> locals{};
        // size_t ret_addr{};

        StackFrame() { locals.resize(56); }

        void new_var(const uint16_t addr, Value& value) {
            if (locals.size() <= addr) locals.resize(addr + 1);
            locals[addr] = value;
        }
    };
}

