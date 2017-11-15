# Prepare your development environment

This document describes how to prepare your development environment to use *Azure IoT Edge*. It describes how to prepare a development environment in Windows using Visual Studio and in Linux.

- [Set up a Windows development environment](#set-up-a-windows-development-environment)
- [Set up a Linux development environment](#set-up-a-linux-development-environment)
- [Set up cross-compiling for Wind River Linux 7](#set-up-cross-compiling-for-wind-river-linux-7)
- [Set up a development environment for module language bindings](#set-up-a-development-environment-for-module-language-bindings)

## Set up a Windows development environment

This section shows you how to set up a development environment for Azure IoT Edge on Windows 10.

1. Install [Visual Studio 2015 or 2017](https://www.visualstudio.com). You can use the free Community Edition if you meet the licensing requirements.

    > Note: Be sure to include Visual C++ and NuGet Package Manager.

1. Install [git](http://www.git-scm.com) making sure git.exe can be run from a command line.

1. Install [CMake](https://cmake.org/download/) making sure cmake.exe can be run from a command line. CMake version 3.7.2 or later is recommended.

    > Note: Using the .msi is the easiest option when installing on Windows. Add CMake to the PATH for at least the current user when prompted to do so by the installer.

1. Install [Python 2.7](https://www.python.org/downloads/release/python-27)

    > Note: Ensure Python is added to your `PATH` environment variable (*Control Panel - Edit environment variables for your account*)

1. Clone the latest version of this repository to your local machine:

    ```
    git clone https://github.com/Azure/iot-edge.git
    ```

    Use the **master** branch to ensure you fetch the latest release version.

> Note: Make sure to clone the repo into a directory heirachy with less than 20 characters (if the path is **prefix**\iot-edge, **prefix** must be less than 20 characters). Windows has limitations on the length of file names and placing the repo in a hierarchy deeper than 20 characters will cause the build to fail. Our `build.cmd` script throws an error if it would eventually hit this failure, but one can hit this when manually building the project with cmake.

## Set up a Linux development environment

This section shows you how to set up a development environment for Azure IoT Edge on Ubuntu.

1. The following packages are needed and they can be installed with the following commands:

    ```
    sudo apt-get update 
    sudo apt-get install curl build-essential libcurl4-openssl-dev git cmake pkg-config libssl-dev uuid-dev valgrind libglib2.0-dev libtool autoconf
    ```

    > Note: libglib2.0-dev is required for ble module/sample.

1. Clone the latest version of this repository to your Ubuntu machine

    ```
    git clone https://github.com/Azure/iot-edge.git
    ```

    Use the **master** branch to ensure you fetch the latest release version.

## Set up cross-compiling for Wind River Linux 7

This section shows you how to set up a development environment for the Azure IoT Edge for cross-compiling to Wind River Linux on Ubuntu.

1. Set up the Ubuntu system as described in the previous section: [Setting up a Linux development environment](#set-up-a-linux-development-environment).

1. Acquire or build the Wind River Linux toolchain. This is licensed software from Intel, see [Wind River Linux Toolchain and Build System User's Guide](https://knowledge.windriver.com/en-us/000_Products/000/010/000/050/000_Wind_River_Linux_Toolchain_and_Build_System_User's_Guide%2C_7.0) for more information.

1. Run the toolchain installer:

    ```
    ./wrlinux-7.0.0.13-glibc-x86_64-intel_baytrail_64-wrlinux-image-idp-sdk.sh
    ```

1. By default, the toolchain will install to a directory like: `/opt/windriver/wrlinux/7.0-intel-baytrail-64`

1. Change directory to the Azure IoT Edge repository.

1. Run the Wind River Linux build script:

    ```
    tools/windriver_linux_c.sh --toolchain /opt/windriver/wrlinux/7.0-intel-baytrail-64
    ```

## Set up a development environment for module language bindings

This sections shows you how to set up a development environment for creating modules written in Java, Node.js, and .NET.

- [Java](../samples/java_sample/java_devbox_setup.md)
- [Node.js](../samples/nodejs_simple_sample/README.md)
- [.NET](../samples/dotnet_binding_sample/README.md)
