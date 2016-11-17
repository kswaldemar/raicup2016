cmake_minimum_required(VERSION 3.0)

project(mystrategy)

file(GLOB sources src/*.cpp)
file(GLOB headers include/*.h)

add_library(mystrategy STATIC ${sources} ${headers})