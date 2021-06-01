# Copyright Cam Mannett 2020
#
# See LICENSE file
#

set(CURSES_NEED_NCURSES ON)
find_package(Curses REQUIRED)

set(TERMINAL_GUI_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/ui/terminal/colour.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/ui/terminal/core.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/ui/terminal/layout.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/ui/terminal/pane.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/ui/terminal/panes/command_pane.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/ui/terminal/panes/functional_pane.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/ui/terminal/panes/log_pane.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/ui/terminal/panes/output_pane.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/ui/terminal/panes/program_pane.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/ui/terminal/shortcut_key.hpp
)

set(TERMINAL_GUI_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/terminal/colour.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/terminal/core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/terminal/layout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/terminal/pane.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/terminal/panes/command_pane.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/terminal/panes/functional_pane.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/terminal/panes/log_pane.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/terminal/panes/output_pane.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/terminal/panes/program_pane.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/terminal/shortcut_key.cpp
)

add_executable(malbolge ${MAIN_SRCS} ${TERMINAL_GUI_HEADERS} ${TERMINAL_GUI_SRCS})

# The documentation is built so we can add the API version of the README to the
# installation packages, this is needed as the 'base' README.md has external
# (i.e. needs an internet connection) components.  It only adds a few seconds to
# the build
add_dependencies(malbolge malbolge_lib documentation)

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
    PUBLIC Threads::Threads
    PUBLIC malbolge_lib
    PUBLIC ${CURSES_LIBRARIES}
)

install(TARGETS malbolge
        COMPONENT exe)

# CPack package configuration
set(CPACK_PACKAGE_VENDOR "Cam Mannett")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_SOURCE_DIR}/docs/README_API.md)
set(CPACK_GENERATOR
    DEB
    TGZ
    STGZ
)
# .deb specific
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_VENDOR})
set(CPACK_DEBIAN_PACKAGE_SECTION "interpreters")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_DEV_PACKAGE_DEPENDS malbolge-exe)
# .rpm specific.  Don't build by default as we currently can't be bothered
# to test it
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
include(CPack)
