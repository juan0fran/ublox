file (GLOB_RECURSE temp_source *)

add_library(pcsensor  SHARED  ${temp_source})

set_target_properties(pcsensor PROPERTIES
			VERSION 0.1
			SOVERSION 1) 

target_link_libraries(pcsensor usb)