#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for the functionshttptrigger module

set(functions_http_trigger_sources
	./src/functionshttptrigger.c
)

set(functions_http_trigger_headers
	./inc/functionshttptrigger.h
)

set(functions_http_trigger_static_sources
	${functions_http_trigger_sources}
)

set(functions_http_trigger_static_headers
	${functions_http_trigger_headers}
)

set(functions_http_trigger_hl_sources
	./src/functionshttptrigger_hl.c
)

set(functions_http_trigger_hl_headers
	./inc/functionshttptrigger_hl.h
)

set(functions_http_trigger_hl_static_sources
	${functions_http_trigger_hl_sources}
)

set(functions_http_trigger_hl_static_headers
	${functions_http_trigger_hl_headers}
)

include_directories(./inc)
include_directories(${GW_INC})

#this builds the functions_http_trigger dynamic library
add_library(functions_http_trigger MODULE ${functions_http_trigger_sources}  ${functions_http_trigger_headers})
target_link_libraries(functions_http_trigger gateway)

#this builds the functions_http_trigger static library
add_library(functions_http_trigger_static STATIC ${functions_http_trigger_static_sources} ${functions_http_trigger_static_headers})
target_compile_definitions(functions_http_trigger_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(functions_http_trigger_static gateway)

#this builds the functions_http_trigger hl dynamic library (by default it uses functions_http_trigger module linked statically)
add_library(functions_http_trigger_hl MODULE ${functions_http_trigger_hl_sources} ${functions_http_trigger_hl_headers})
target_link_libraries(functions_http_trigger_hl functions_http_trigger_static gateway)

#this builds the functions_http_trigger hl static library (by default it uses functions_http_trigger module linked statically)
add_library(functions_http_trigger_hl_static STATIC ${functions_http_trigger_hl_static_sources} ${functions_http_trigger_hl_static_headers})
target_compile_definitions(functions_http_trigger_hl_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(functions_http_trigger_hl_static functions_http_trigger_static gateway)

linkSharedUtil(functions_http_trigger)
linkSharedUtil(functions_http_trigger_static)
linkSharedUtil(functions_http_trigger_hl)
linkSharedUtil(functions_http_trigger_hl_static)

add_module_to_solution(functions_http_trigger)

if(install_executables)
	install(TARGETS functions_http_trigger LIBRARY DESTINATION lib) 
    install(TARGETS functions_http_trigger_hl LIBRARY DESTINATION lib) 
endif()

add_subdirectory(tests)