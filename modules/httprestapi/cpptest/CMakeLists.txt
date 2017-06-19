cmake_minimum_required(VERSION 3.8.0)
set(sources
    CPPTest.cpp
)
set(headers
    CPPTest.h
)

#link_directories(c:/vcpkg/vcpkg/packages)

# dynamic library
add_library(cpptest ${sources} ${headers})

# static library
add_library(cpptest_static STATIC ${sources} ${headers})

add_module_to_solution(cpptest)
