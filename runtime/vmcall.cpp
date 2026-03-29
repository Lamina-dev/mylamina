#include "vmcall.hpp"
#include <cstdlib>
#include "libloader.hpp"

#include "../compiler/generator/generator.hpp"

/*
 * VMC_write_out(0)
 * arg1 : str(254)
 */
VMC_REGISTER(out) {
    fputs(
        self->get_register(REG_COUNT_INDEX_MAX).to_string().c_str(),
        stdout);
}
/*(1)*/
VMC_REGISTER(in) {
    self->get_register(0) = fgets(self->get_register(0).str, 1024, stdin);
}
/*
 * VMC_exit(2)
 * arg1 : exit_code(254)
 */
VMC_REGISTER(exit) {
    exit(self->get_register(REG_COUNT_INDEX_MAX).i32);
}

/*
 * VMC_dyn_load(3)
 * arg1 : lib_name(254)
 *
 * return: lib_ptr
 */
VMC_REGISTER(dyn_load) {
    self->libs.push_back(std::make_unique<DynLib>(self->get_register(REG_COUNT_INDEX_MAX).str));
    self->set_reg_ptr(0, self->libs.back().get());
}

/*
 * VMC_dyn_set(4)
 * arg1 : lib_ptr(254)
 * arg2 : func_name(253)
 * arg3 : args_types_array_ptr(252)
 * arg4 : args_types_array_len(251)
 * arg5 : ret_type_ptr(250)
 */
VMC_REGISTER(dyn_set) {
    const auto args_type_p = static_cast<CBasicTypes *>(self->get_register(REG_COUNT_INDEX_MAX - 2).ptr);
    static_cast<DynLib*>(self->get_register(REG_COUNT_INDEX_MAX).ptr)->set_func(
        self->get_register(REG_COUNT_INDEX_MAX - 1).str,
        std::vector<CBasicTypes>(
            args_type_p,
            args_type_p + self->get_register(REG_COUNT_INDEX_MAX - 3).u64),
            *static_cast<CBasicTypes *>(self->get_register(REG_COUNT_INDEX_MAX - 4).ptr)
        );
}

/*
 * VMC_dyn_call(5)
 * arg1 : lib_ptr(0)
 * arg2 : func_name(1)
 *
 * 函数传参直接遵循虚拟机调用约定，传入虚拟寄存器即可
 */
VMC_REGISTER(dyn_call) {
    static_cast<DynLib*>(self->get_register(0).ptr)->call(
        self->get_register(1).str,
        self
        );
}

