cmake_minimum_required(VERSION 3.0)

project(mystrategy)

set(sources

    src/MyStrategy.cpp

    src/Eviscerator.cpp
    src/FieldMap.cpp
    src/FieldsDescription.cpp
    src/MovementHandler.cpp
    src/PathFinder.cpp
    src/VisualDebug.cpp
    src/SkillBuilder.cpp
    src/ExWorld.cpp
    src/Canvas.cpp
    src/PotentialField.cpp

    src/MyLivingUnit.cpp
    src/MyBonus.cpp
    src/MyBuilding.cpp
    )
file(GLOB headers include/*.h)

add_library(mystrategy STATIC ${sources} ${headers})