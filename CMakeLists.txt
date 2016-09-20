cmake_minimum_required(VERSION 2.6)

##project
project(ublox_driver C)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
MARK_AS_ADVANCED(
        LIBRARY_OUTPUT_PATH
        EXECUTABLE_OUTPUT_PATH
)

link_directories(${LIBRARY_OUTPUT_PATH})

add_subdirectory(src)
add_subdirectory(apps)