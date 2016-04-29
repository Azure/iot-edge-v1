#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for the identitymap module

set(identity_map_sources
	./src/identitymap.c
)

set(identity_map_headers
	./inc/identitymap.h
)

set(identity_map_static_sources
	${identity_map_sources}
)

set(identity_map_static_headers
	${identity_map_headers}
)

set(identity_map_hl_sources
	./src/identitymap_hl.c
)

set(identity_map_hl_headers
	./inc/identitymap_hl.h
)

set(identity_map_hl_static_sources
	${identity_map_hl_sources}
)

set(identity_map_hl_static_headers
	${identity_map_hl_headers}
)

include_directories(./inc)
include_directories(${GW_INC})

#this builds the identity_map dynamic library
add_library(identity_map MODULE ${identity_map_sources}  ${identity_map_headers})
target_link_libraries(identity_map gateway)

#this builds the identity_map static library
add_library(identity_map_static  ${identity_map_static_sources} ${identity_map_static_headers})
target_compile_definitions(identity_map_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(identity_map_static gateway)

#this builds the identity_map hl dynamic library (by default it uses identity_map module linked statically)
add_library(identity_map_hl MODULE ${identity_map_hl_sources} ${identity_map_hl_headers})
target_link_libraries(identity_map_hl identity_map_static gateway)

#this builds the identity_map hl static library (by default it uses identity_map module linked statically)
add_library(identity_map_hl_static ${identity_map_hl_static_sources} ${identity_map_hl_static_headers})
target_compile_definitions(identity_map_hl_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(identity_map_hl_static identity_map_static gateway)

linkSharedUtil(identity_map)
linkSharedUtil(identity_map_static)
linkSharedUtil(identity_map_hl)
linkSharedUtil(identity_map_hl_static)

add_module_to_solution(identity_map)

if(install_executables)
	install(TARGETS identity_map LIBRARY DESTINATION lib) 
    install(TARGETS identity_map_hl LIBRARY DESTINATION lib) 
endif()

add_subdirectory(tests)