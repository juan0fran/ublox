file (GLOB example_sources example.c)

set(EXAMPLE_BIN ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/example CACHE STRING "example dir")

add_executable(example ${example_sources})

target_link_libraries(example ublox_gps cc_beacon_iface)
