cmake_minimum_required(VERSION 3.0)

project(CodeWizards)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

add_compile_options(
    -std=c++14
    -Wall -Wtype-limits -Wno-unknown-pragmas
    -fno-optimize-sibling-calls -fno-strict-aliasing
)

add_definitions(
    -DRUNNING_LOCAL
    -D_LINUX
)

# Directory for zip file releases
set(RELEASE_DIR ${CMAKE_BINARY_DIR}/release)

include_directories(
    cpp-cgdk
    mystrategy/include
)


add_subdirectory(mystrategy)

file(GLOB cpp_sources
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    cpp-cgdk/*.cpp cpp-cgdk/model/*.cpp cpp-cgdk/csimplesocket/*.cpp)
add_executable(cw_ai ${cpp_sources})
target_link_libraries(cw_ai mystrategy m)
