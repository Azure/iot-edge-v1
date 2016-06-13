#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

set(dotnet_sources
	./src/dotnet.cpp
)

set(dotnet_headers
	./inc/dotnet.h
)

set(dotnet_static_sources
	${dotnet_sources}
)

set(dotnet_static_headers
	${dotnet_headers}
)

set(dotnet_hl_sources
	./src/dotnet_hl.c
)

set(dotnet_hl_headers
	./inc/dotnet_hl.h
)

set(dotnet_hl_static_sources
	${dotnet_hl_sources}
)

set(dotnet_hl_static_headers
	${dotnet_hl_headers}
)


include_directories(./inc)
include_directories(${GW_INC})
include_directories(${IOTHUB_CLIENT_INC_FOLDER})

#this builds the dotnet dynamic library
add_library(dotnet MODULE ${dotnet_sources}  ${dotnet_headers})
target_link_libraries(dotnet gateway mscoree)

#this builds the dotnet static library
add_library(dotnet_static  ${dotnet_static_sources} ${dotnet_static_headers})
target_compile_definitions(dotnet_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(dotnet_static gateway mscoree)

#this builds the dotnet dynamic library (by default it uses dotnet module linked statically)
add_library(dotnet_hl MODULE ${dotnet_hl_sources} ${dotnet_hl_headers})
target_link_libraries(dotnet_hl dotnet_static gateway mscoree)

#this builds the dotnet static library (by default it uses dotnet module linked statically)
add_library(dotnet_hl_static ${dotnet_hl_static_sources} ${dotnet_hl_static_headers})
target_compile_definitions(dotnet_hl_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(dotnet_hl_static dotnet_static gateway mscoree)

linkSharedUtil(dotnet)
linkSharedUtil(dotnet_static)
linkSharedUtil(dotnet_hl)
linkSharedUtil(dotnet_hl_static)

add_subdirectory(tests)

add_module_to_solution(dotnet)