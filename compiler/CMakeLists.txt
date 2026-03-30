set(CMAKE_CXX_STANDARD 20)
file(GLOB_RECURSE COMPILER_SRC "*.cpp" "*/[!test_]*.cpp")
if (NOT TARGET_WASM)
    add_library(lmc SHARED ${COMPILER_SRC})
else ()
    add_library(lmc STATIC ${COMPILER_SRC})
endif ()


# Define cmake-build-debug macro for DLL export
if(WIN32)
    target_compile_definitions(lmc PRIVATE LMC_BUILD)
endif()

# Include common headers
target_include_directories(lmc PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Link against runtime library
target_link_libraries(lmc lmvm)
