# Copyright Cam Mannett 2020
#
# See LICENSE file
#

cmake_minimum_required(VERSION 3.12)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ccache.cmake)

project(malbolge
        VERSION 1.0
        DESCRIPTION "Malbolge virtual machine"
        HOMEPAGE_URL "https://github.com/cmannett85/malbolge")

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/traits.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/math/ipow.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/math/tritset.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/math/ternary.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/virtual_memory.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/cpu_instruction.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/loader.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/virtual_cpu.hpp
)

set(SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/math/ternary.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/virtual_cpu.cpp
)

set(FOR_IDE
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
    ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile
)

enable_testing()
add_subdirectory(test)
