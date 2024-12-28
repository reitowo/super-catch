cmake_minimum_required(VERSION 3.29)
project(super_catch LANGUAGES C CXX ASM_MASM)

set(CMAKE_CXX_STANDARD 11)

# Library
add_library(super_catch
        include/super_catch/super_catch.h
        src/super_catch.cpp
)

option(SUPER_CATCH_ENABLE_DEBUG_OUTPUT "Enable debug output to stderr" OFF)

target_include_directories(super_catch PUBLIC include)

if (SUPER_CATCH_ENABLE_DEBUG_OUTPUT)
    target_compile_definitions(super_catch PUBLIC -DSUPER_CATCH_PARAM_DEBUG_OUTPUT)
endif ()

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHa")
endif ()

# Test
add_executable(super_catch_test
        test/main.cpp
)

target_link_libraries(super_catch_test
        super_catch
)