cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

file(GLOB_RECURSE RUNTIME_SRC *.cpp */*.cpp)
add_library(lmvm SHARED ${RUNTIME_SRC})

# Define cmake-build-debug macro for DLL export
if(WIN32)
    target_compile_definitions(lmvm PRIVATE LMVM_BUILD)
endif()
# 启用 FetchContent
include(FetchContent)


message(STATUS "downloading dyncall...")
FetchContent_Declare(
        dyncall
        GIT_REPOSITORY https://github.com/ugexe/dyncall.git
        GIT_TAG        "r1.4"
        OVERRIDE_FIND_PACKAGE
        # 禁用更新检查
        UPDATE_DISCONNECTED ON
)
target_include_directories(lmvm PRIVATE ${CMAKE_BINARY_DIR}/_deps/dyncall-src)
FetchContent_MakeAvailable(dyncall)

target_include_directories(lmvm PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(lmvm dyncall_s dyncallback_s dynload_s)
