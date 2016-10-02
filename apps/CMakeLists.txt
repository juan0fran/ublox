file (GLOB gps_sources gps_core.c)

set(EXAMPLE_BIN ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/gps_core CACHE STRING "gps_core dir")

add_executable(gps_process ${gps_sources})

target_link_libraries(gps_process ublox_gps pcsensor)
