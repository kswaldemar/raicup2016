cmake_minimum_required(VERSION 3.0)

project(mystrategy)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(headers
    include/MyStrategy.h
    include/Logger.h
    include/ScoutingBehaviour.h
    include/Behaviour.h
    include/InfoPack.h
)

set(sources
    src/MyStrategy.cpp
    src/Behaviour.cpp
    src/ScoutingBehaviour.cpp
)

add_library(mystrategy STATIC ${sources} ${headers})
add_dependencies(mystrategy cppcgdk)

# Strategy zip file creating
configure_file(zipgen.sh.in ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/zipgen.sh @ONLY)
file(COPY ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/zipgen.sh
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

add_custom_target(zip
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/zipgen.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Creating strategy release zip file")