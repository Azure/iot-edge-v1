#this is CMakeLists.txt for the gateway core
cmake_minimum_required(VERSION 2.8.11)

add_subdirectory(deps)

#this sets a global var that stores where the includes for GW are
set(GW_INC ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "Needs to be included for gateway includes" FORCE)
set(GW_SRC ${CMAKE_CURRENT_LIST_DIR}/src CACHE INTERNAL "Needs to be included for gateway sources" FORCE)

#setting the dynamic_loader file based on OS that it is used
if(WIN32)
    include_directories(${GW_INC}/windows )
elseif(LINUX)
    include_directories(${GW_INC}/linux)
endif()

#setting the dynamic_loader file based on OS that it is used
if(WIN32)
    set(dynamic_library_c_file ./adapters/dynamic_library_windows.c ./adapters/gb_library_windows.c)
elseif(LINUX)
    set(dynamic_library_c_file ./adapters/dynamic_library_linux.c ./adapters/gb_library_linux.c )
endif()

#setting specific libraries to be loaded based on OS (for example, Linux needs "-ldl", windows does not)
if(LINUX)
    set(dynamic_loader_library dl)
endif()

function(linkHttp whatExecutableIsBuilding)
    includeHttp()
    if(WIN32)
        if(WINCE)
            target_link_libraries(${whatExecutableIsBuilding} crypt32.lib)
            target_link_libraries(${whatExecutableIsBuilding} ws2.lib)
        else()
            target_link_libraries(${whatExecutableIsBuilding} winhttp.lib)
        endif()
    else()
        target_link_libraries(${whatExecutableIsBuilding} curl)
    endif()
endfunction(linkHttp)

function(includeHttp)
#function body to be similar to includeProton
endfunction(includeHttp)


set(gateway_c_sources
    ./src/message.c
    ./src/module_loader.c
    ./src/gateway_ll.c
    ./src/gateway.c
    ${dynamic_library_c_file}
)


set(gateway_h_sources
    ./inc/message.h
    ./inc/message_bus.h
    ./inc/module.h
    ./inc/gateway_ll.h
    ./inc/gateway.h
    ./inc/module_loader.h
    ./inc/dynamic_library.h
)

if (${default_broker})
    set(gateway_c_sources ${gateway_c_sources} ./src/message_bus.c)
else()
    set(gateway_c_sources ${gateway_c_sources} ./src/message_bus_broadcast.c)
endif()

include_directories(./inc)

add_library(gateway
    ${gateway_c_sources}
    ${gateway_h_sources}
)

if(WIN32)
    target_link_libraries(gateway rpcrt4.lib)
else()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(UIDLINUX REQUIRED uuid)

    target_link_libraries(gateway uuid)
endif()

target_link_libraries(gateway aziotsharedutil parson ${dynamic_loader_library} )
link_broker(gateway)
#this adds the tests to the build process

add_subdirectory(tests)