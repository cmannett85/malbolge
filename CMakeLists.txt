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

set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.73 REQUIRED COMPONENTS
    program_options
    log
)
find_package(Threads REQUIRED)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/traits.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/logging.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/algorithm/remove_from_range.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/exception.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/math/ipow.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/math/tritset.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/math/ternary.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/virtual_memory.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/cpu_instruction.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/loader.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/utility/raii.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/virtual_cpu.hpp
)

set(SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logging.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/exception.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/math/ternary.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/virtual_cpu.cpp
)

# Source files specific to the exe, rather than for the unit tests too
set(MAIN_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

set(FOR_IDE
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
    ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile
)

add_executable(malbolge ${HEADERS} ${SRCS} ${MAIN_SRCS} ${FOR_IDE})

target_compile_features(malbolge PUBLIC cxx_std_20)
set_target_properties(malbolge PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(malbolge PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
          -Werror -Wall -Wextra>
     $<$<CXX_COMPILER_ID:MSVC>:
          /W4>
)

target_include_directories(malbolge
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(malbolge
    PUBLIC Boost::program_options
    PUBLIC Boost::log
    PUBLIC Threads::Threads
)

target_compile_definitions(malbolge
    PUBLIC BOOST_LOG_DYN_LINK
)

enable_testing()
add_subdirectory(test)
