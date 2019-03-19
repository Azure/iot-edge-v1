.NET Core Managed Dynamic module addition sample (Linux only for now) 
=====================================================================

Overview
--------

This sample showcases how one might build an IoT gateway as a .NET Core Application and and 
load .NET Core Modules dynamically. It demonstrates the use of gateway creation, updating of 
module and route details from the json file and the gateway start/destroy APIs using dotnet 
core.

Other resources:
- Hello World [sample](../hello_world/README.md)
- .NET Core Sample [dotnetcoresample] (../dotnet_core_module_sample/README.md)
- [Devbox setup](../../doc/devbox_setup.md)
- [.Net Core Framework Installer](https://www.microsoft.com/net/download/core)

Prerequisites
--------------
1. Setup your development machine. A guide for doing this can be found [here](../../doc/devbox_setup.md).
2. Make sure you have .NET Core Framework installed. Our current version of the binding was tested and loads modules written in .NET **Core** v1.1.11.

Building the sample (Linux)
---------------------------
The sample code and dependancies gets build by running:
```
tools\build.sh --enable-dotnet-core-binding --disable-native-remote-modules
```


The [devbox setup](../../doc/devbox_setup.md) guide has more information on how you can build Azure IoT Edge.


Running the sample (Linux)
--------------------------

1. Create a soft link to the dependant shared libraries - libdotnetcore.so and libgateway.so 
   or copy those files to the binary location.

2. Run the binary with the following as parameters:
```   
dotnet dotnet_core_dynamic_add_module.dll module1_lin.json module2_lin.json links.json
```
   where the module1_lin.json and module2_lin.json contains the details about the sensor and printer module
   respectively and the links.json consists of the link between the two. 

 
Sample output(Linux)
--------------------
```
#dotnet dotnet_core_dynamic_add_module.dll module1_lin.json module2_lin.json links.json
Started gateway. Press return to link the module

.NET Core Gateway is running. Press return to quit.

#dotnet dotnet_core_dynamic_add_module.dll module1_lin.json module2_lin.json links.json
Started gateway. Press return to link the module

.NET Core Gateway is running. Press return to quit.
08/14/2017 21:54:47>ÿPrinterÿmoduleÿreceivedÿmessage:ÿSensorData: 476932477
        Property[0]>ÿKey=sourceÿ:ÿValue=sensor
08/14/2017 21:54:52>ÿPrinterÿmoduleÿreceivedÿmessage:ÿSensorData: 573914640
        Property[0]>ÿKey=sourceÿ:ÿValue=sensor
08/14/2017 21:54:57>ÿPrinterÿmoduleÿreceivedÿmessage:ÿSensorData: 928588399
        Property[0]>ÿKey=sourceÿ:ÿValue=sensor
```
  	 
