#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

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

build_test_artifacts(${theseTestsName} ON ${LIBS})

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
	install_managed_dotnet_binding_binaries(${theseTestsName}_exe ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
	install_managed_dotnet_e2etest_module_binaries(${theseTestsName}_exe ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
	install_dotnet_host_module_binaries(${theseTestsName}_exe ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
endif()
