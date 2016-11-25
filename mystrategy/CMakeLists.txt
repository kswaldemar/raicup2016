cmake_minimum_required(VERSION 3.0)

project(mystrategy)

set(sources

    src/MyStrategy.cpp

    src/Config.cpp
    src/Eviscerator.cpp
    src/FieldMap.cpp
    src/FieldsDescription.cpp
    src/MovementHandler.cpp
    src/PathFinder.cpp
    src/VisualDebug.cpp
)
file(GLOB headers include/*.h)

add_library(mystrategy STATIC ${sources} ${headers})