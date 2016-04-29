#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for the hello_world module
cmake_minimum_required(VERSION 2.8.11)

set(hello_world_sources
	./src/hello_world.c
)

set(hello_world_headers
	./inc/hello_world.h
)

set(hello_world_static_sources
	${hello_world_sources}
)

set(hello_world_static_headers
	${hello_world_headers}
)

set(hello_world_hl_sources
	./src/hello_world_hl.c
)

set(hello_world_hl_headers
	./inc/hello_world_hl.h
)

set(hello_world_hl_static_sources
	${hello_world_hl_sources}
)

set(hello_world_hl_static_headers
	${hello_world_hl_headers}
)


include_directories(./inc)
include_directories(${GW_INC})

#this builds the hello_world dynamic library
add_library(hello_world MODULE ${hello_world_sources}  ${hello_world_headers})
target_link_libraries(hello_world gateway)

#this builds the hello_world static library
add_library(hello_world_static  ${hello_world_static_sources} ${hello_world_static_headers})
target_compile_definitions(hello_world_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(hello_world_static gateway)

#this builds the hello_world dynamic library (by default it uses hello_world module linked statically)
add_library(hello_world_hl MODULE ${hello_world_hl_sources} ${hello_world_hl_headers})
target_link_libraries(hello_world_hl hello_world_static gateway)

#this builds the hello_world static library (by default it uses hello_world module linked statically)
add_library(hello_world_hl_static ${hello_world_hl_static_sources} ${hello_world_hl_static_headers})
target_compile_definitions(hello_world_hl_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(hello_world_hl_static hello_world_static gateway)

linkSharedUtil(hello_world)
linkSharedUtil(hello_world_static)
linkSharedUtil(hello_world_hl)
linkSharedUtil(hello_world_hl_static)

add_module_to_solution(hello_world)
if(install_executables)
	install(TARGETS hello_world LIBRARY DESTINATION lib) 
    install(TARGETS hello_world_hl LIBRARY DESTINATION lib) 
endif()

