# Copyright Cam Mannett 2020
#
# See LICENSE file
#

option(BUILD_SHARED_LIBS "Build shared Malbolge library")

add_library(malbolge_lib ${HEADERS} ${SRCS} ${FOR_IDE} ${VERSION_FILE})
add_dependencies(malbolge_lib gen_version)

target_compile_features(malbolge_lib PUBLIC cxx_std_20)
set_target_properties(malbolge_lib PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(malbolge_lib PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Werror -Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4>
)

target_include_directories(malbolge_lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(malbolge_lib
    PUBLIC Threads::Threads
)

set(LIB_COMP dev)
if(BUILD_SHARED_LIBS)
    set(LIB_COMP exe)
endif()

install(TARGETS malbolge_lib
        COMPONENT ${LIB_COMP})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge
        DESTINATION include
        COMPONENT dev)
