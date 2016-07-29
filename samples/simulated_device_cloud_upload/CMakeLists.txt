#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for simulated_device_cloud_upload sample

set(simulated_device_cloud_upload_sources
	./src/main.c
)

if(WIN32)
    set(simulated_device_cloud_upload_sources 
        ${simulated_device_cloud_upload_sources}
        ./src/simulated_device_cloud_upload_win.json
    )
    set_source_files_properties(./src/simulated_device_cloud_upload_win.json PROPERTIES HEADER_FILE_ONLY ON)
else()
    set(simulated_device_cloud_upload_sources 
        ${simulated_device_cloud_upload_sources}
        ./src/simulated_device_cloud_upload_lin.json
    )
    set_source_files_properties(./src/simulated_device_cloud_upload_lin.json PROPERTIES HEADER_FILE_ONLY ON)
endif()

if(run_as_a_service)
    set(modules_service_c_file ./src/module_service_config_enabled.c)
else()
    set(modules_service_c_file ./src/module_service_config_disabled.c)
endif()

include_directories(./inc ${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${GW_INC})
include_directories(../../modules/common)

#link modules
linkModule(logger)
linkModule(identitymap)
linkModule(iothubhttp)
linkModule(simulated_device)

add_executable(simulated_device_cloud_upload_sample ${modules_service_c_file} ${simulated_device_cloud_upload_sources})
target_link_libraries(simulated_device_cloud_upload_sample gateway)
linkSharedUtil(simulated_device_cloud_upload_sample)
linkHttp(simulated_device_cloud_upload_sample)
install_broker(simulated_device_cloud_upload_sample ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )


add_sample_to_solution(simulated_device_cloud_upload_sample)

if(install_executables)
	install(TARGETS simulated_device_cloud_upload_sample DESTINATION bin)
endif()