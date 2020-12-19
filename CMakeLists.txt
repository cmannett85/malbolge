# Copyright Cam Mannett 2020
#
# See LICENSE file
#

cmake_minimum_required(VERSION 3.12)
include(CMakeDependentOption)

project(malbolge
        VERSION 0.0.3
        DESCRIPTION "Malbolge virtual machine"
        HOMEPAGE_URL "https://github.com/cmannett85/malbolge"
        LANGUAGES CXX)

# Options
option(WASM_BUILD "Enable WASM/Emscripten support")
cmake_dependent_option(DOCS_ONLY "Only build the documentation" OFF
                       "NOT WASM_BUILD" OFF)

# Only enable LTO and find packages if we're not doing a Docs-only build
if(NOT DOCS_ONLY)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ccache.cmake)

    set(BOOST_VERSION 1.67)
    set(Boost_USE_MULTITHREADED ON)
    find_package(Boost ${BOOST_VERSION} REQUIRED)
    find_package(Threads REQUIRED)

    include(CheckIPOSupported)
    check_ipo_supported(RESULT lto_supported OUTPUT error)
    if(lto_supported)
        message(STATUS "LTO enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "LTO not supported")
    endif()
endif()

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/algorithm/remove_from_range.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/algorithm/container_ops.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/algorithm/trim.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/c_interface.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/cpu_instruction.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/debugger/client_control.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/debugger/script_parser.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/debugger/script_runner.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/exception.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/loader.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/log.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/math/ipow.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/math/tritset.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/math/ternary.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/normalise.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/traits.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/utility/argument_parser.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/utility/from_chars.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/utility/gate.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/utility/mutex_wrapper.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/utility/raii.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/utility/stream_lock_guard.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/utility/string_constant.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/utility/string_view_ops.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/utility/tuple_iterator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/utility/unescaper.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/utility/visit.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/virtual_cpu.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/virtual_memory.hpp
)

set(SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/c_interface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cpu_instruction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/debugger/client_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/debugger/script_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/debugger/script_runner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/exception.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/math/ternary.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utility/argument_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utility/from_chars.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/.github/workflows/pr_checker.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/.github/workflows/docs_pusher.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/.github/workflows/release_builder.yml
)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/versioning/version.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_types/documentation.cmake)

if(WASM_BUILD)
    message(STATUS "WASM/Emscripten build")
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_types/library.cmake)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_types/wasm.cmake)
elseif(NOT DOCS_ONLY)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_types/library.cmake)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_types/standard_executable.cmake)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_types/address_sanitizer.cmake)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_types/thread_sanitizer.cmake)

    enable_testing()
    add_subdirectory(test)
endif()
