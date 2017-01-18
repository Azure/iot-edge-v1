# Prepare your development environment

This document describes how to prepare your development environment to use the *Microsoft Azure IoT Gateway SDK*. It describes preparing a development environment in Windows using Visual Studio and in Linux.

- [Setting up a Windows development environment](#windows)
- [Setting up a Linux development environment](#linux)
- [Setting up cross-compiling for Wind River Linux 7](#windriver)
- [Setting up a development environment for module language bindings](#bindings)

<a name="windows"/>
## Setting up a Windows development environment

This section shows you how to set up a development environment for the Azure IoT Gateway SDK on Windows 10.

1. Install [Visual Studio 2015](https://www.visualstudio.com). You can use the free Community Edition if you meet the licensing requirements.

    > Note: Be sure to include Visual C++ and NuGet Package Manager.

2. Install [git](http://www.git-scm.com) making sure git.exe can be run from a command line.
3. Install [cmake](https://cmake.org/download/) making sure cmake.exe can be run from a command line.

    > Note: Using the .msi is the easiest option when installing on Windows. Add CMake to the PATH for at least the current user when prompted to do so by the installer.

4. Clone the latest version of this repository to your local machine 
```
git clone https://github.com/Azure/azure-iot-gateway-sdk.git
```
Use the **master** branch to ensure you fetch the latest release version.

>Note: Make sure to clone the repo into a directory heirachy with less than 20 characters (if the path is <b>prefix</b>\azure-iot-gateway-sdk, <b>prefix</b> must be less than 20 characters). Windows has limitations on the length of file names and placing the repo in a hierarchy deeper than 20 characters will cause the build to fail. Our `build.cmd` script throws an error if it would eventually hit this failure, but one can hit this when manually building the project with cmake. 

<a name="linux"/>
## Set up a Linux development environment

This section shows you how to set up a development environment for the Azure IoT Gateway SDK on Ubuntu.
 
1. The following packages are needed and they can be installed with the following commands:

 ```
 sudo apt-get update 
 sudo apt-get install curl build-essential libcurl4-openssl-dev git cmake libssl-dev uuid-dev valgrind libglib2.0-dev
 ``` 
 
 * libglib2.0-dev is required for ble module/sample
2. Clone the latest version of this repository to your Ubuntu machine
```
git clone https://github.com/Azure/azure-iot-gateway-sdk.git
```
Use the **master** branch to ensure you fetch the latest release version.

<a name="windriver"/>
## Setting up cross-compiling for Wind River Linux 7

This section shows you how to set up a development environment for the Azure IoT Gateway SDK for cross-compiling to Wind River Linux on Ubuntu.

1. Set up the Ubuntu system as described in the previous section: [Setting up a Linux development environment](#linux)

2. Acquire or build the Wind River Linux toolchain. This is licensed software from Intel, see [Wind River Linux Toolchain and Build System User's Guide](https://knowledge.windriver.com/en-us/000_Products/000/010/000/050/000_Wind_River_Linux_Toolchain_and_Build_System_User's_Guide%2C_7.0) for more information.

3. Run the toolchain installer: 
 ```
 ./wrlinux-7.0.0.13-glibc-x86_64-intel_baytrail_64-wrlinux-image-idp-sdk.sh
 ```
 
4. By default, the toolchain will install to a directory like `/opt/windriver/wrlinux/7.0-intel-baytrail-64`

5. Change directory to the Azure IoT Gateway SDK repository.

6. Run the Wind River Linux build script:
 ```
 tools/windriver_linux_c.sh --toolchain /opt/windriver/wrlinux/7.0-intel-baytrail-64
 ```

<a name="bindings">
## Setting up a development environment for module language bindings

This sections shows you how to set up a development environment for creating modules written in Java, Node.js, and .NET.

- [Java](../samples/java_sample/java_devbox_setup.md)
- [Node.js](../samples/nodejs_simple_sample/README.md)
- [.NET](../samples/dotnet_binding_sample/README.md)
