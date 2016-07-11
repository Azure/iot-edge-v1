#this is CMakeLists.txt to build parson only as a lib
cmake_minimum_required(VERSION 2.8.11)

remove_definitions(-D_CRT_SECURE_NO_WARNINGS)

set(parson_c_sources
    ../../deps/parson/parson.c
)

set(parson_h_sources
    ../../deps/parson/parson.h
)

include_directories(../../deps/parson)

remove_definitions(-D_CRT_SECURE_NO_WARNINGS) 

add_library(parson
    ${parson_c_sources}
    ${parson_h_sources}
)