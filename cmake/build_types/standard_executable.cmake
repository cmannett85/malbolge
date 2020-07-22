# Copyright Cam Mannett 2020
#
# See LICENSE file
#

add_executable(malbolge ${HEADERS} ${SRCS} ${MAIN_SRCS} ${FOR_IDE} ${VERSION_FILE})
add_dependencies(malbolge gen_version)

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
)

install(TARGETS malbolge)

# CPack package configuration
set(CPACK_PACKAGE_VENDOR "Cam Mannett")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
set(CPACK_GENERATOR
    DEB
    TGZ
    STGZ
)
# .deb specific
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_VENDOR})
set(CPACK_DEBIAN_PACKAGE_SECTION "interpreters")
# .rpm specific.  Don't build by default as we currently can't be bothered
# tested it
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
include(CPack)
