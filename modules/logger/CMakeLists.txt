#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for the logger module
cmake_minimum_required(VERSION 2.8.11)

set(logger_sources
	./src/logger.c
)

set(logger_headers
	./inc/logger.h
)

set(logger_static_sources
	${logger_sources}
)

set(logger_static_headers
	${logger_headers}
)

set(logger_hl_sources
	./src/logger_hl.c
)

set(logger_hl_headers
	./inc/logger_hl.h
)

set(logger_hl_static_sources
	${logger_hl_sources}
)

set(logger_hl_static_headers
	${logger_hl_headers}
)


include_directories(./inc)
include_directories(${GW_INC})

#this builds the Logger dynamic library
add_library(logger MODULE ${logger_sources}  ${logger_headers})
target_link_libraries(logger gateway)

#this builds the Logger static library
add_library(logger_static  ${logger_static_sources} ${logger_static_headers})
target_compile_definitions(logger_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(logger_static gateway)

#this builds the Logger_HL dynamic library (by default it uses Logger linked statically)
add_library(logger_hl MODULE ${logger_hl_sources} ${logger_hl_headers})
target_link_libraries(logger_hl logger_static gateway)

#this builds the Logger_HL static library (by default it uses Logger linked statically)
add_library(logger_hl_static ${logger_hl_static_sources} ${logger_hl_static_headers})
target_compile_definitions(logger_hl_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(logger_hl_static logger_static gateway)

linkSharedUtil(logger)
linkSharedUtil(logger_static)
linkSharedUtil(logger_hl)
linkSharedUtil(logger_hl_static)

add_module_to_solution(logger)

add_subdirectory(tests)



if(install_executables)
	install(TARGETS logger LIBRARY DESTINATION lib) 
    install(TARGETS logger_hl LIBRARY DESTINATION lib) 
endif()

