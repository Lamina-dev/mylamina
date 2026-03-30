cmake_minimum_required(VERSION 3.15)
project(LMX)

set(CMAKE_CXX_STANDARD 23)

# Add MSVC compatibility support
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /utf-8")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -march=native -mtune=native -flto=auto -funroll-loops -finline-functions -finline-small-functions -findirect-inlining -fipa-cp -fipa-cp-clone -fipa-bit-cp -fipa-vrp -fipa-ra -fipa-icf -fdevirtualize-at-ltrans -fvariable-expansion-in-unroller -fipa-sra -fsplit-loops -fsplit-paths -ftree-loop-vectorize -ftree-slp-vectorize -fvect-cost-model=unlimited -ftree-partial-pre -fpeel-loops -fpredictive-commoning -funswitch-loops -fgcse-after-reload -ftree-loop-distribute-patterns -fsee -fipa-pta -fno-semantic-interposition -fno-plt -fmodulo-sched -fmodulo-sched-allow-regmoves -ffunction-sections -fdata-sections -Wl,--gc-sections")
    endif()
endif ()



# 如果你不编译到wasm可以设为OFF
set(TARGET_WASM OFF)
if (TARGET_WASM)
add_definitions(-DTARGET_WASM)
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target wasm32-wasi")
if(EMSCRIPTEN)
    # 1. 设置为纯 WASM 模式
    set(CMAKE_EXECUTABLE_SUFFIX ".wasm")
    
    # 2. 清除所有 CXX 标志
    set(CMAKE_CXX_FLAGS "")
    
    # 3. 核心编译选项（必须按此顺序）
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WASM=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s STANDALONE_WASM=1")
    
    # 4. 指定 WASI 目标（关键！）
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -target wasm32-wasi")
    
    # 5. 设置内存大小
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s INITIAL_MEMORY=16777216")  # 16MB
    
    # 6. 导出函数
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s EXPORTED_FUNCTIONS=_start -s EXPORTED_FUNCTIONS=_main")
    
    # 7. 禁用所有与 JS 相关的功能
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ERROR_ON_UNDEFINED_SYMBOLS=0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s NO_DYNAMIC_EXECUTION=1")

    # ✅ 启用 WASM 异常
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WASM=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WASM_EXCEPTIONS=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fwasm-exceptions")
    
    # ✅ 允许 unwind
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s SUPPORT_LONGJMP=wasm")
    
    # ❌ 不要用 STANDALONE_WASM（与异常不兼容）
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s STANDALONE_WASM=1")
    
    # ✅ 使用 JS 异常处理
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s DISABLE_EXCEPTION_CATCHING=0")
    
    # ✅ 链接时包含异常支持
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s WASM_EXCEPTIONS=1")
    
    message(STATUS "启用 WASM 异常支持")
    
    # ✅ 禁用 unwind 表
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-unwind-tables")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s DISABLE_EXCEPTION_CATCHING=1")

    # ✅ 禁用未定义符号错误
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ERROR_ON_UNDEFINED_SYMBOLS=0")
    
    # ✅ 明确链接 libc
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s DEFAULT_LIBRARY_FUNCS_TO_INCLUDE=[]")
    
    # 8. 设置链接选项
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--export-all")
    
    # 9. 使用最小化运行时
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s MINIMAL_RUNTIME=2")
endif()
endif ()





# Define DLL export macros
if(BUILD_SHARED_LIBS OR WIN32)
    add_definitions(-DLMX_DLL)
endif()
include_directories(include)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(compiler)
add_subdirectory(runtime)

add_subdirectory(tools/lm)
