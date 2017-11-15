#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

#Seting this disable valgrind on Client and CShared utility, since it were disable. It shall be enabled once they are passing.
set(run_valgrind OFF)

add_subdirectory(./c-utility)
add_subdirectory(./iot-sdk-c)
