cmake_minimum_required(VERSION 3.21)

# Phase 0 only builds the pure-CPU physics library and its Catch2 tests.
# The Vulkan engine/ subdirectory is added in a later phase.
project(kerr_blackhole_sim
        VERSION 0.1.0
        DESCRIPTION "Physically accurate Kerr black hole simulator"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /permissive- /Zc:__cplusplus)
endif()

enable_testing()

add_subdirectory(physics)
