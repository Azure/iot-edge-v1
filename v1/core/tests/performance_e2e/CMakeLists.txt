#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

include_directories(./inc)
include_directories(${GW_INC})

set(simulator_sources
    ./src/simulator.cpp
)

set(simulator_headers
    ./inc/simulator.h
)


#this builds the simulator module
add_library(simulator MODULE ${simulator_sources}  ${simulator_headers})
target_link_libraries(simulator gateway)

add_library(simulator_static STATIC ${simulator_sources}  ${simulator_headers})
target_compile_definitions(simulator_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(simulator_static gateway)

linkSharedUtil(simulator)
linkSharedUtil(simulator_static)

add_module_to_solution(simulator)

if(install_modules)
    install(TARGETS simulator LIBRARY DESTINATION "${LIB_INSTALL_DIR}/modules") 
endif()


set(metrics_sources
    ./src/metrics.cpp
)

set(metrics_headers
    ./inc/metrics.h
)


#this builds the metrics module
add_library(metrics MODULE ${metrics_sources}  ${metrics_headers})
target_link_libraries(metrics gateway)

add_library(metrics_static STATIC ${metrics_sources}  ${metrics_headers})
target_compile_definitions(metrics_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(metrics_static gateway)

linkSharedUtil(metrics)
linkSharedUtil(metrics_static)

add_module_to_solution(metrics)

if(install_modules)
    install(TARGETS metrics LIBRARY DESTINATION "${LIB_INSTALL_DIR}/modules") 
endif()


# This builds the command line tool.
set(performance_e2e_sources
    ./src/main.cpp
)
if(WIN32)
    set(performance_e2e_sources 
        ${performance_e2e_sources}
        ./src/performance_win.json
    )
    set_source_files_properties(./src/performance_win.json PROPERTIES HEADER_FILE_ONLY ON)
else()
    set(performance_e2e_sources 
        ${performance_e2e_sources}
        ./src/performance_lin.json
    )
    set_source_files_properties(./src/performance_lin.json PROPERTIES HEADER_FILE_ONLY ON)
endif()

add_executable(performance_e2e ${performance_e2e_sources})

add_dependencies(performance_e2e simulator metrics)

target_link_libraries(performance_e2e gateway nanomsg)
linkSharedUtil(performance_e2e)
install_broker(performance_e2e ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
copy_gateway_dll(performance_e2e ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )

set_target_properties(performance_e2e
            PROPERTIES
            FOLDER "tests/E2ETests")

# Run E2E as a test.

set(theseTestsName performance_e2e)

#Disable valgrind just for performance E2E test 
set(run_valgrind OFF)

compileAsC99()

set(${theseTestsName}_test_files
    ./src/${theseTestsName}.c
)

#setting the dynamic_loader file based on OS that it is used
if(WIN32)
    set(modules_c_file ./src/module_config_windows.c)
elseif(UNIX) # LINUX OR APPLE
    set(modules_c_file ./src/module_config_linux.c)
endif()


set(${theseTestsName}_c_files
    ${modules_c_file}
)

set(${theseTestsName}_h_files
    ./inc/module_config_resources.h
)

build_c_test_artifacts(${theseTestsName} ON "tests/E2ETests")
set_target_properties(
    ${theseTestsName}_exe PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
)

linkSharedUtil(${theseTestsName}_exe)

if(WIN32)
    if(TARGET ${theseTestsName}_dll)
        target_link_libraries(${theseTestsName}_dll
            gateway
        )
    endif()
    
    if(TARGET ${theseTestsName}_exe)
        target_link_libraries(${theseTestsName}_exe
            gateway
        )    
        install_broker(${theseTestsName}_exe ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
        copy_gateway_dll(${theseTestsName}_exe ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
    endif()
else()
    if(TARGET ${theseTestsName}_exe)
        target_link_libraries(${theseTestsName}_exe
            gateway
            aziotsharedutil
            pthread
        )
    endif()

endif()
