#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

compileAsC99()
set(theseTestsName dotnet_e2e)




set(${theseTestsName}_cpp_files
    ${theseTestsName}.cpp
)

include_directories(
    ../../inc
    ${GW_SRC}
    ${GW_INC}
)

include_directories(../../../../modules/common)

#setting #defines
if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(sharedutil_include_directories ${MICROMOCK_INC_FOLDER} ${TESTRUNNERSWITCHER_INC_FOLDER} ${CTEST_INC_FOLDER} ${SAL_INC_FOLDER} ${SHARED_UTIL_INC_FOLDER} ${SHARED_UTIL_SRC_FOLDER})

windows_unittests_add_exe(${theseTestsName} ${LIBS})



if(TARGET ${theseTestsName}_dll)
    target_link_libraries(${theseTestsName}_dll
        gateway
    )
endif()
    
if(TARGET ${theseTestsName}_exe)
    target_link_libraries(${theseTestsName}_exe
        gateway
    )    
    add_dependencies(${theseTestsName}_exe dotnet)
        
    install_broker(${theseTestsName}_exe ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
    copy_gateway_dll(${theseTestsName}_exe ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
    install_binaries(${theseTestsName}_exe ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) ${dotnet_managed_binding_dll} )
    install_binaries(${theseTestsName}_exe ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) ${dotnet_e2etest_module_dll} )
    install_binaries(${theseTestsName}_exe ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) ${dotnet_host_binding_dll} )
    

endif()
