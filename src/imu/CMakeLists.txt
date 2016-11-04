file (GLOB_RECURSE imu_source *)

add_library(imu  SHARED  ${imu_source})

set_target_properties(imu PROPERTIES
			VERSION 0.1
			SOVERSION 1) 

target_link_libraries(imu)
