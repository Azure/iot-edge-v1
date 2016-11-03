if(WIN32)
    file(
        INSTALL
            "${CMAKE_CURRENT_LIST_DIR}/../bin/nanomsg.dll"
        DESTINATION
            "${CMAKE_CURRENT_BINARY_DIR}"
    )
endif()

link_directories("${CMAKE_CURRENT_LIST_DIR}/../lib")

include("${CMAKE_CURRENT_LIST_DIR}/azure_iot_gateway_sdkTargets.cmake")