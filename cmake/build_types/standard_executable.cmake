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
    PUBLIC Boost::program_options
    PUBLIC Boost::log
    PUBLIC Threads::Threads
)

target_compile_definitions(malbolge
    PUBLIC BOOST_LOG_DYN_LINK
)

install(TARGETS malbolge)
