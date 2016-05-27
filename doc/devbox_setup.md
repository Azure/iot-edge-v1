# Prepare your development environment

This document describes how to prepare your development environment to use the *Microsoft Azure IoT Gateway SDK*. It describes preparing a development environment in Windows using Visual Studio and in Linux.

- [Setting up a Windows development environment](#windows)
- [Setting up a Linux development environment](#linux)

<a name="windows"/>
## Setting up a Windows development environment

This section shows you how to set up a development environment for the Azure IoT Gateway SDK on Windows 10.

1. Install [Visual Studio 2015](https://www.visualstudio.com). You can use the free Community Edition if you meet the licensing requirements.
Be sure to include Visual C++ and NuGet Package Manager.
2. Install [git](http://www.git-scm.com) making sure git.exe can be run from a command line.
3. Install [cmake](https://cmake.org/download/) making sure cmake.exe can be run from a command line.

>Note: Using the .msi is the easiest option when installing on Windows. Add CMake to the PATH for at least the current user when prompted to do so by the installer. 
4. Clone the latest version of this repository to your local machine with the recursive parameter
```
git clone --recursive https://github.com/Azure/azure-iot-gateway-sdk.git
```
Use the **master** branch to ensure you fetch the latest release version.

>Note: Make sure to clone the repo into a directory heirachy with less than 20 characters. Windows has limitations on the length of file names and placing the repo in a hierarchy deeper than 20 characters will cause the build to fail. Our `build.cmd` script throws an error if it would eventually hit this failure, but one can hit this when manually building the project with cmake. 

> In the path <b>prefix</b>\azure-iot-gateway-sdk, <b>prefix</b> must be less than 20 characters.

<a name="Linux"/>
## Set up a Linux development environment

This section shows you how to set up a development environment for the Azure IoT Gateway SDK on Ubuntu.
 
1. The following packages are needed and they can be installed with the following commands:
```
sudo apt-get update 
sudo apt-get install curl build-essential libcurl4-openssl-dev git cmake libssl-dev uuid-dev valgrind libglib2.0-dev
``` 
2. Clone the latest version of this repository to your Ubuntu machine with the recursive parameter
```
git clone --recursive https://github.com/Azure/azure-iot-gateway-sdk.git
```
Use the **master** branch to ensure you fetch the latest release version.