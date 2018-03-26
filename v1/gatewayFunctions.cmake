#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(WIN32)
    set(cores /m)
else()
    set(cores -j ${build_cores})
endif()

function(updateSubmodule submoduleRootDirectory testFile workingDirectory)
    if(NOT EXISTS "${submoduleRootDirectory}/${testFile}")
        execute_process(
            COMMAND git submodule update --init --recursive ${submoduleRootDirectory}
            WORKING_DIRECTORY ${workingDirectory}
            RESULT_VARIABLE res
        )
        if(${res})
            message(FATAL_ERROR "Error updating submodule: ${res}")
        endif()
    endif()
endfunction()

function(buildSubmodule libraryName submoduleRootDirectory cmakeRootDirectory)
    message(STATUS "Building ${libraryName}...")

    # Update submodule if needed
    updateSubmodule(${cmakeRootDirectory} "CMakeLists.txt" ${PROJECT_SOURCE_DIR})

    # Build clean by deleting/recreating the build folder
    file(REMOVE_RECURSE ${cmakeRootDirectory}/build)
    file(MAKE_DIRECTORY ${cmakeRootDirectory}/build)

    # Generate CMake command
    set(CMD cmake)
    foreach(arg ${ARGN})
        set(CMD ${CMD} ${arg})
    endforeach()

    # Reuse some of the parent's settings to build the child dependency
    if(CMAKE_BUILD_TYPE)
        set(CMD ${CMD} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
    endif()

    if(CMAKE_TOOLCHAIN_FILE)
        set(CMD ${CMD} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
    endif()

    if(${dependency_install_prefix})
        set(CMD ${CMD} -DCMAKE_INSTALL_PREFIX=${dependency_install_prefix})
    endif()

    # Set path to build
    set(CMD ${CMD} ..)

    # Generate the build
    execute_process(
        COMMAND ${CMD}
        WORKING_DIRECTORY ${cmakeRootDirectory}/build
        RESULT_VARIABLE res
    )
    if(${res})
        message(FATAL_ERROR "Error running cmake for ${libraryName}: ${res}")
    endif()

    # Set the config option (Release, Debug) for multi-config environments like Visual Studio
    # (ignored by gcc and clang)
    if(CMAKE_BUILD_TYPE)
        set(BUILD_CONFIGURATION --config ${CMAKE_BUILD_TYPE})
    else()
        set(BUILD_CONFIGURATION "")
    endif()

    # Build and install the library
    message(STATUS "Installing ${libraryName}. Please wait...")
    execute_process(
        COMMAND cmake --build . --target install ${BUILD_CONFIGURATION} -- ${cores}
        WORKING_DIRECTORY ${cmakeRootDirectory}/build
        RESULT_VARIABLE res
        OUTPUT_FILE output.txt
        ERROR_FILE error.txt
    )
    if(${res})
        message(FATAL_ERROR "**ERROR (${res}) installing ${libraryName}. See "
            "${cmakeRootDirectory}/build/error.txt and "
            "${cmakeRootDirectory}/build/output.txt.\n")
    endif()
endfunction()

# Additional arguments to this function will be passed to the library's cmake command
function(findAndInstallNonFindPkg libraryName submoduleRootDirectory cmakeRootDirectory)
    if(NOT EXISTS "${CMAKE_INSTALL_PREFIX}/../${libraryName}" OR ${rebuild_deps})
        message(STATUS "${libraryName} not found...")
        buildSubmodule(${libraryName} ${submoduleRootDirectory} ${cmakeRootDirectory} ${ARGN})
    endif()
endfunction()

# Additional arguments to this function will be passed to the library's cmake command
function(findAndInstall libraryName version submoduleRootDirectory cmakeRootDirectory)
    # The azure-iot-sdk-c repo requires special treatment: its Parson submodule must be initialized
    if(${libraryName} STREQUAL azure_iot_sdks)
        updateSubmodule(deps/iot-sdk-c CMakeLists.txt ${PROJECT_SOURCE_DIR})
        updateSubmodule(deps/parson README.md ${cmakeRootDirectory})
    endif()

    # Don't bother to call find_package() if the caller requested 'rebuild_deps'
    if(NOT ${libraryName}_FOUND AND NOT ${rebuild_deps})
        message(STATUS "${libraryName} not found...")
        find_package(${libraryName} ${version} QUIET CONFIG HINTS ${dependency_install_prefix})
    endif()

    # Find the library, fail the build if it doesn't exist
    if(NOT ${libraryName}_FOUND OR ${rebuild_deps})
        buildSubmodule(${libraryName} ${submoduleRootDirectory} ${cmakeRootDirectory} ${ARGN})
        find_package(${libraryName} ${version} REQUIRED CONFIG HINTS ${dependency_install_prefix})
    endif()
endfunction()