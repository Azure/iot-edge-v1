#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for the simulated_device module
cmake_minimum_required(VERSION 2.8.11)

set(simulated_device_sources
	./src/simulated_device.c
)

set(simulated_device_headers
	./inc/simulated_device.h
)

set(simulated_device_static_sources
	${simulated_device_sources}
)

set(simulated_device_static_headers
	${simulated_device_headers}
)

set(simulated_device_hl_sources
	./src/simulated_device_hl.c
)

set(simulated_device_hl_headers
	./inc/simulated_device_hl.h
)

set(simulated_device_hl_static_sources
	${simulated_device_hl_sources}
)

set(simulated_device_hl_static_headers
	${simulated_device_hl_headers}
)


include_directories(./inc)
include_directories(${GW_INC})

#this builds the simulated_device dynamic library
add_library(simulated_device MODULE ${simulated_device_sources}  ${simulated_device_headers})
target_link_libraries(simulated_device gateway)

#this builds the simulated_device static library
add_library(simulated_device_static  ${simulated_device_static_sources} ${simulated_device_static_headers})
target_compile_definitions(simulated_device_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(simulated_device_static gateway)

#this builds the simulated_device dynamic library (by default it uses simulated_device module linked statically)
add_library(simulated_device_hl MODULE ${simulated_device_hl_sources} ${simulated_device_hl_headers})
target_link_libraries(simulated_device_hl simulated_device_static gateway)

#this builds the simulated_device static library (by default it uses simulated_device module linked statically)
add_library(simulated_device_hl_static ${simulated_device_hl_static_sources} ${simulated_device_hl_static_headers})
target_compile_definitions(simulated_device_hl_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(simulated_device_hl_static simulated_device_static gateway)

linkSharedUtil(simulated_device)
linkSharedUtil(simulated_device_static)
linkSharedUtil(simulated_device_hl)
linkSharedUtil(simulated_device_hl_static)

add_module_to_solution(simulated_device)

if(install_executables)
	install(TARGETS simulated_device LIBRARY DESTINATION lib) 
    install(TARGETS simulated_device_hl LIBRARY DESTINATION lib) 
endif()

