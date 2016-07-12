#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for the bindings
if(${enable_dotnet_binding})
    add_subdirectory(dotnet)
endif()

if(${enable_java_binding})
    add_subdirectory(java)
endif()
    
if(${enable_nodejs_binding})
    add_subdirectory(nodejs)
endif()