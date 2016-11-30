#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

include("dependencies.cmake")

#Disable Valgrind just for Gateway E2E test, since they are having problems with Open SSL and Valgrind.
set(run_valgrind OFF)

compileAsC99()
set(theseTestsName gateway_e2e)
set(${theseTestsName}_cpp_files
${theseTestsName}.cpp
)

#setting the dynamic_loader file based on OS that it is used
if(WIN32)
    set(modules_c_file ./module_config_windows.c)
elseif(LINUX)
    set(modules_c_file ./module_config_linux.c)
endif()

set(${theseTestsName}_c_files
    ${modules_c_file}
)

include_directories(../../../samples/protocol_encapsulation/inc ${IOTHUB_CLIENT_INC_FOLDER} e2e_module/inc/)
include_directories(../../../deps/iot-sdk-c/testtools/iothub_test/inc)
include_directories(../../../deps/c-utility/inc/azure_c_shared_utility)
include_directories(../../../modules/common)
include_directories(../../../modules/iothub/inc)
include_directories(../../../modules/identitymap/inc)
include_directories(${IOTHUB_TEST_INC_FOLDER})
include_directories(${IOTHUB_SERVICE_CLIENT_INC_FOLDER})
include_directories(${GW_INC})

build_test_artifacts(${theseTestsName} ON)
set_target_properties(
    ${theseTestsName}_exe PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
)

if(WIN32)
    if(TARGET ${theseTestsName}_dll)
        target_link_libraries(${theseTestsName}_dll
            gateway
            iothub_test
            rpcrt4
            iothub_client_http_transport
        )
    endif()
    
    if(TARGET ${theseTestsName}_exe)
        target_link_libraries(${theseTestsName}_exe
            gateway
            iothub_test
            rpcrt4
            iothub_client_http_transport
        )    
        install_broker(${theseTestsName}_exe ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
        copy_gateway_dll(${theseTestsName}_exe ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
        copy_iothub_client_dll(${theseTestsName}_exe ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration))
        copy_iothub_service_dll(${theseTestsName}_exe ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration))
    endif()
else()
    if(TARGET ${theseTestsName}_exe)
        target_link_libraries(${theseTestsName}_exe
            iothub_test
            gateway
            aziotsharedutil
            uuid
            iothub_service_client
            iothub_client_http_transport
        )
        target_link_libraries(${theseTestsName}_exe pthread)
        linkUAMQP(${theseTestsName}_exe)    
        linkHttp(${theseTestsName}_exe)
    endif()

endif()


add_subdirectory(e2e_module)