cmake_minimum_required(VERSION 3.8.0)

#add_definitions(-DVCPKG_TARGET_TRIPLET=x86-windows-static)
if (WIN32)
    set(VCPKG_DIR c:/vcpkg/vcpkg)
    set(CMAKE_TOOLCHAIN_FILE c:/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake)
endif()

set(sources
    httprestserver.cpp
)
set(headers
    httprestserver.h
)

#link_directories(c:/vcpkg/vcpkg/packages)

# dynamic library
add_library(httprestserver ${sources} ${headers})

## static library
add_library(httprestserver_static STATIC ${sources} ${headers})
target_compile_definitions(httprestserver_static PRIVATE BUILD_MODULE_TYPE_STATIC)

add_module_to_solution(httprestserver)
