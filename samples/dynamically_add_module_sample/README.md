# Azure IoT Edge - Dynamically Add Module Sample

This document provides a detailed overview of the Dynamically Add Module sample [code](./src) which uses the fundamental components of Azure IoT Edge architecture to create a gateway without modules, adding modules (and links) from a JSON file after the gateway is created, starting a gateway, removing a module and destroying a gateway.

# Dev box setup

A dev box configured with Azure IoT Edge and necessary libraries is necessary to complete this walkthrough. Please complete the [dev box setup](../../doc/devbox_setup.md) before continuing.


# The sample contains:

1. A hello world Module that publishes data to gateway.
2. A logger module for producing message broker diagnostics.

## How to build the sample
Linux

1. Open a shell
2. Navigate to `iot-edge/tools/`
3. Run `./build.sh`

>Note: `build.sh` does multiple things. It builds the project and places it in the "build" folder in the root of the repo. This folder is deleted and recreated every time `build.sh` is run. Additionally, `build.sh` runs all tests. The project can be build manually by using cmake. To do this:

>create a folder for the build output and navigate to that folder.

>run `sudo cmake <path to repo root>`

>Note: The cmake scripts will check for installed cmake dependencies on your machine before it attempts to fetch, build, and install them for you.

>run `make -j $(nproc)`

>To run the tests:

>run `ctest -j $(nproc) -C Debug --output-on-failure`

Windows

1. Open a Developer Command Prompt for VS2015 as an Administrator
2. Navigate to `iot-edge\tools\`
3. Run `build.cmd`. 

>Note: `build.cmd` does multiple things. It builds a solution ('azure_iot_gateway_sdk.sln') and places it in the "build" folder in the root of the repo. This folder is deleted and recreated every time `build.cmd` is run. Additionally, `build.cmd` runs all tests. The project can be build manually by using cmake. To do this:

>create a folder for the build output and navigate to that folder.

>run `cmake <path to repo root>`

>Note: The cmake scripts will check for installed cmake dependencies on your machine before it attempts to fetch, build, and install them for you.

>run `msbuild /m /p:Configuration="Debug" /p:Platform="Win32" azure_iot_gateway_sdk.sln`

>To run the tests:

>run `ctest -C Debug -V`

## How to run the sample

# Linux

`./build/sample/dynamically_add_module_sample/dynamically_add_module_sample sample/dynamically_add_module_sample/src/modules_lin.json sample/dynamically_add_module_sample/src/links_lin.json`

# Windows

`.\build\sample\dynamically_add_module_sample\dynamically_add_module_sample.exe sample\dynamically_add_module_sample\src\modules_win.json sample\dynamically_add_module_sample\src\links_win.json`

