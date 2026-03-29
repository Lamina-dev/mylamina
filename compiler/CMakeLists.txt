set(CMAKE_CXX_STANDARD 20)
file(GLOB_RECURSE COMPILER_SRC "*.cpp" "*/[!test_]*.cpp")
add_library(lmc SHARED ${COMPILER_SRC})

# Define cmake-build-debug macro for DLL export
if(WIN32)
    target_compile_definitions(lmc PRIVATE LMC_BUILD)
endif()

# Include common headers
target_include_directories(lmc PUBLIC ${CMAKE_SOURCE_DIR}/include)
