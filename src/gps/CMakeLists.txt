file (GLOB_RECURSE gps_source *)

add_library(ublox_gps  SHARED  ${gps_source})

set_target_properties(ublox_gps PROPERTIES
			VERSION 0.1
			SOVERSION 1)

target_link_libraries(ublox_gps pcsensor dbman)
