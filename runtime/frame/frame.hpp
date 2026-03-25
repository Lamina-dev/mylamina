//
// Created by geguj on 2026/1/30.
//
#pragma once
#include <vector>
#include "../value/value.hpp"


namespace lmx::runtime {
    struct StackFrame {
        std::vector<Value> locals{};

        StackFrame() { locals.resize(56); }

        void new_var(const uint16_t addr, const Value& value) {
            if (locals.size() <= addr) locals.resize(addr + 1);
            locals[addr] = value;
        }
    };
}

