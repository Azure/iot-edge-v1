#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

# We are disabling running integration tests on Linux
# till we can figure out why Helgrind and DRD
# fail when run under "ctest". Also, Valgrind doesn't
# pass with a garbage collected system like v8.
if(WIN32)
    add_subdirectory(nodejs_binding_integrationtests)
endif()

add_subdirectory(nodejs_binding_hl_unittests)
