#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for the gw e2e tests
cmake_minimum_required(VERSION 2.8.11)

compileAsC99()
set(theseTestsName gw_e2etests)
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
include_directories(../../../deps/azure-iot-sdks/c/testtools/iothub_test/inc)
include_directories(../../../deps/azure-c-shared-utility/inc/azure_c_shared_utility)
include_directories(../../../modules/common)
include_directories(../../../modules/iothubhttp/inc)
include_directories(../../../modules/identitymap/inc)
include_directories(${IOTHUB_TEST_INC_FOLDER})
include_directories(${IOTHUB_SERVICE_CLIENT_INC_FOLDER})
include_directories(${GW_INC})

build_test_artifacts(${theseTestsName} ON)

if(WIN32)
	if(TARGET ${theseTestsName}_dll)
		target_link_libraries(${theseTestsName}_dll
			gateway
			iothub_test
			rpcrt4
		)
	endif()
	
	if(TARGET ${theseTestsName}_exe)
		target_link_libraries(${theseTestsName}_exe
			gateway
			iothub_test
			rpcrt4
		)		
	endif()
else()
	if(TARGET ${theseTestsName}_exe)
		target_link_libraries(${theseTestsName}_exe
			iothub_test
			gateway
			aziotsharedutil
			uuid
			iothub_service_client
		)
		target_link_libraries(${theseTestsName}_exe pthread)
		linkUAMQP(${theseTestsName}_exe)	
		linkHttp(${theseTestsName}_exe)
	endif()

endif()

add_subdirectory(e2e_module)