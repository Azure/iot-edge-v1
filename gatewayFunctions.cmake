#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(WIN32)
    set(cores /m)
else()
    set(cores -j ${build_cores})
endif()

# Additional arguments to the specific projects cmake command may be specified after specifying the cmakeRootDirectory
function(findAndInstallNonFindPkg libraryName submoduleRootDirectory cmakeRootDirectory)

    # If we have a build type passed in then we use it when building this
    # project by passing it as the value for the --config argument to the
    # "cmake --build" command.
    if(CMAKE_BUILD_TYPE)
        set(BUILD_CONFIGURATION --config ${CMAKE_BUILD_TYPE})
    else()
        set(BUILD_CONFIGURATION "")
    endif()

    if(${rebuild_deps} OR NOT EXISTS "${CMAKE_INSTALL_PREFIX}/../${libraryName}")
        message(STATUS "${libraryName} not found...")
        message(STATUS "Building ${libraryName}...")

        #If the library directory doesn't exist, pull submodules
        if(NOT EXISTS "${cmakeRootDirectory}/src")
            execute_process(
                COMMAND git submodule update --init ${submoduleRootDirectory}
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                RESULT_VARIABLE res
            )
        endif()
        if(${res})
            message(FATAL_ERROR "Error pull submodules: ${res}")
        endif()

        #Create the build directory to run cmake, and run cmake

        #generate command
        set(CMD cmake)
        foreach(arg ${ARGN})
            set(CMD ${CMD} ${arg})
        endforeach()

        # If we have a build type passed in then we propagate it down the chain
        # when building this dependency.
        if(CMAKE_BUILD_TYPE)
            set(CMD ${CMD} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
        endif()

        if(DEFINED ${dependency_install_prefix})
            set(CMD ${CMD} -DCMAKE_INSTALL_PREFIX=${dependency_install_prefix})
        endif()
        if(CMAKE_TOOLCHAIN_FILE)
           set( CMD ${CMD} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()
        set(CMD ${CMD} ../)

        # Re-create the build folder for making a fresh build
        file(REMOVE_RECURSE ${cmakeRootDirectory}/build)
        file(MAKE_DIRECTORY ${cmakeRootDirectory}/build)

        execute_process(
            COMMAND ${CMD}
            WORKING_DIRECTORY ${cmakeRootDirectory}/build
            RESULT_VARIABLE res
        )
        if(${res})
            message(FATAL_ERROR "Error running cmake for ${libraryName}: ${res}")
        endif()

        # Install library
        message(STATUS "Installing ${libraryName}. Please wait...")
        execute_process(
            COMMAND cmake --build . --target install ${BUILD_CONFIGURATION} -- ${cores}
            WORKING_DIRECTORY ${cmakeRootDirectory}/build
            RESULT_VARIABLE res
            OUTPUT_FILE output.txt
            ERROR_FILE error.txt
        )
        if(${res})
            message(FATAL_ERROR "Error installing ${libraryName}: ${res}")
        endif()
    endif()
endfunction()

#Additional arguments to the specific projects cmake command may be specified after specifying the cmakeRootDirectory
function(findAndInstall libraryName version submoduleRootDirectory cmakeRootDirectory)

    # If we have a build type passed in then we use it when building this
    # project by passing it as the value for the --config argument to the
    # "cmake --build" command.
    if(CMAKE_BUILD_TYPE)
        set(BUILD_CONFIGURATION --config ${CMAKE_BUILD_TYPE})
    else()
        set(BUILD_CONFIGURATION "")
    endif()

    if(${rebuild_deps} OR NOT ${libraryName}_FOUND)
        # We are interested in doing a "find_package" only if we aren't going
        # to rebuild dependencies anyway.
        if(NOT ${rebuild_deps})
            find_package(${libraryName} ${version} QUIET CONFIG HINTS ${dependency_install_prefix})
        endif()
        if(${rebuild_deps} OR NOT ${libraryName}_FOUND)
            if(NOT ${rebuild_deps} )
                message(STATUS "${libraryName} not found...")
            endif()

            message(STATUS "Building ${libraryName}...")

            #If the library directory doesn't exist, pull submodules
            if(NOT EXISTS "${cmakeRootDirectory}/CMakeLists.txt")
                execute_process(
                    COMMAND git submodule update --init --recursive ${submoduleRootDirectory}
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    RESULT_VARIABLE res
                )
            endif()
            if(${res})
                message(FATAL_ERROR "Error pulling submodules: ${res}")
            endif()

            #Create the build directory to run cmake, and run cmake

            #generate comand
            set(CMD cmake)
            foreach(arg ${ARGN})
                set(CMD ${CMD} ${arg})
            endforeach()

            # If we have a build type passed in then we propagate it down the chain
            # when building this dependency.
            if(CMAKE_BUILD_TYPE)
                set(CMD ${CMD} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
            endif()

            if(DEFINED ${dependency_install_prefix})
                set(CMD ${CMD} -DCMAKE_INSTALL_PREFIX=${dependency_install_prefix})
            endif()
            if(CMAKE_TOOLCHAIN_FILE)
                set(CMD ${CMD} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
            endif()
            set(CMD ${CMD} ../)

            # Re-create the build folder for making a fresh build
            file(REMOVE_RECURSE ${cmakeRootDirectory}/build)
            file(MAKE_DIRECTORY ${cmakeRootDirectory}/build)

            execute_process(
                COMMAND ${CMD}
                WORKING_DIRECTORY ${cmakeRootDirectory}/build
                RESULT_VARIABLE res
            )
            if(${res})
                message(FATAL_ERROR "Error running cmake for ${libraryName}: ${res}")
            endif()

            # Install library
            message(STATUS "Installing ${libraryName}. Please wait...")
            execute_process(
                COMMAND cmake --build . --target install ${BUILD_CONFIGURATION} -- ${cores}
                WORKING_DIRECTORY ${cmakeRootDirectory}/build
                RESULT_VARIABLE res
                OUTPUT_FILE output.txt
                ERROR_FILE error.txt
            )
            if(${res})
                message(FATAL_ERROR "Error installing ${libraryName}: ${res}")
            endif()

            #Attempt to find library with the REQUIRED option
            find_package(${libraryName} ${version} REQUIRED CONFIG HINTS ${dependency_install_prefix})
        endif()
    endif()

endfunction()