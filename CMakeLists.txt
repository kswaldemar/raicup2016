cmake_minimum_required(VERSION 3.0)

project(CodeWizards)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

add_compile_options(
    -std=c++14
    -Wall
)

add_definitions(
    -DRUNNING_LOCAL
)

# Directory for zip file releases
set(RELEASE_DIR ${CMAKE_BINARY_DIR}/release)

include_directories(
    cpp-cgdk
#    cpp-cgdk/model cpp-cgdk/csimplesocket
    mystrategy/include
)

add_subdirectory(cpp-cgdk)
add_subdirectory(mystrategy)


add_executable(cw_ai cpp-cgdk/main.cpp)
target_link_libraries(cw_ai cppcgdk mystrategy)