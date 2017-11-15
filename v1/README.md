# 11/15/2017
Azure IoT Edge V2 public preview is now available. It implements the cloud configuration, deployment, and monitoring features announced at /Build '17. The documentation can be found [here](https://docs.microsoft.com//azure/iot-edge).

V1 will continue to be supported. V1 documentation which used to live on docs.microsoft.com has been moved to this GitHub repo. All documentation on docs.microsoft.com now refers to Azure IoT Edge V2 as that is the latest evolution of the product. V1 code will continue to live in this repo and bugs can continue to be filed on the issues section of this repo.


# 5/10/2017
The Azure IoT Gateway SDK was our first step to enabling edge analytics in IoT solutions. We’re doubling down on, and expanding, this vision as explained in Satya’s Keynote at the Build conference and Sam George’s [blog post](http://blogs.microsoft.com/iot/?p=23040). As part of this evolution, the SDK is becoming an extensible product you can use instead of a set of code you build. To reflect this, we’re changing the name to Azure IoT Edge.

All the important developer concepts are maintained as we continue to improve Azure IoT Edge. Specifically…
-	modules remain units compute which can be written in your programming language of choice.
-	traditional cloud services and 3rd party business logic can run as a module.
-	modules can communicate with each other via declarative message passing.

This similarity means that existing solutions can evolve with the product! There will be some infrastructural changes. For example: modules will run in Docker containers and the broker used to pass messages between module code will move to a lite version of IoT Hub running locally in a module. The vast majority of this is shielded from both a module developer and gateway developer.

Sign up [here](https://microsoft.qualtrics.com/jfe/form/SV_0V8Xr8y9c4mHUfb) to be kept up to date about the next generation of Azure IoT Edge features. In the meantime, be sure to check out the great features we just added to Azure IoT Edge, like [development packages](https://azure.microsoft.com/en-us/blog/azure-iot-edge-packages/) and an [Azure Stream Analytics module](https://azure.microsoft.com/en-us/blog/announcing-azure-stream-analytics-on-edge-devices-preview/).


# Azure IoT Edge
IoT scenarios vary drastically between industries and even between customers within the same industry. 
Azure IoT Edge lets you build IoT solutions tailored to your exact scenario. Connect new 
or existing devices, regardless of protocol. Process the data in an on-premises gateway using the 
language of your choice (Java, C#, Node.js, or C), before sending it to the cloud. Deploy the solution 
on hardware that meets your performance needs and runs the operating system of your choice.

In the image below an existing device has been connected to the cloud using edge intelligence. The 
on-premises gateway not only performs protocol adaptation which allows the device to send data to the 
cloud, it also filters the data so that only the most important information is uploaded.

![](doc/media/READMEDiagram.png)

Using existing modules from the Azure IoT Edge ecosystem significantly reduces your development 
and maintenance costs.  Running the gateway on-premises opens up all kinds of scenarios like 
communicating between devices in real time, operating your IoT solution with an intermittent cloud 
connection, or enforcing additional security requirements.

Visit [https://azure.microsoft.com/campaigns/iot-edge](https://azure.microsoft.com/campaigns/iot-edge/) to learn more about Azure IoT Edge. 

## Azure IoT Edge Modules
The following modules are available in this repository:
>| Name             | Description                                                             |
>|------------------|-------------------------------------------------------------------------|
>| ble              | Represents a Bluetooth low energy (BLE) device connected to the gateway |
>| hello_world      | Sends a "hello world" message periodically                              |
>| identitymap      | Maps MAC addresses to IoT Hub device IDs/keys                           |
>| iothub           | Sends/receives messages to/from mapped devices and IoT Hub              |
>| logger           | Writes received message content to a file                               |
>| simulated_device | Simulates a gateway-connected BLE device                                | 
>| azure_functions  | Sends message content to an Azure Function                              | 

## Create Modules using Packages
The fastest way to setup your development environment to start writing modules is to leverage our packages for Java, C#, and Node.js. Our [sample apps repo](https://github.com/Azure-Samples/iot-edge-samples) has quick steps on getting started with these packages:
- [Azure IoT Edge Maven](https://mvnrepository.com/artifact/com.microsoft.azure.gateway/gateway-module-base): With this you will be able to run the Azure IoT Edge sample app and start writing Java modules. This package contains the Azure IoT Edge core and links to the module dependencies’ packages for Linux or Windows. Requires the [java binding package](https://mvnrepository.com/artifact/com.microsoft.azure.gateway/gateway-java-binding).  
- [Azure IoT Edge npm](https://www.npmjs.com/package/azure-iot-gateway): With this you will be able to run the Azure IoT Edge sample app and start writing Node.js modules. This package contains the Azure IoT Edge core and auto-installs the module dependencies’ packages for Linux or Windows.
- [Azure IoT Edge NuGet .NET Standard](https://www.nuget.org/packages/Microsoft.Azure.Devices.Gateway.Module.NetStandard/): With this you will be able to run the Azure IoT Edge sample app and write .NET Standard modules on Windows ([IoT Edge Core for Windows](https://www.nuget.org/packages/Microsoft.Azure.Devices.Gateway.Native.Windows.x64/)) or Ubuntu ([IoT Edge Core for Ubuntu](https://www.nuget.org/packages/Microsoft.Azure.Devices.Gateway.Native.Ubuntu.x64/)).
- [Azure IoT Edge NuGet .NET Framework](https://www.nuget.org/packages/Microsoft.Azure.IoT.Gateway.Module): With this you will be able to run the Azure IoT Edge sample app and write .NET Framework modules on Windows, dependent on [IoT Edge Core for Windows](https://www.nuget.org/packages/Microsoft.Azure.Devices.Gateway.Native.Windows.x64/).

## Featured Modules
Other people are creating modules for Azure IoT Edge too! See the **More information** link for 
a module to find out how to get it, who supports it, etc.
>| Name              | More information                                               | Targets IoT Edge version |
>|-------------------|----------------------------------------------------------------|--------------------------|
>| OPC Publisher     | https://github.com/Azure/iot-edge-opc-publisher                | 2017-04-27               |
>| OPC Proxy         | https://github.com/Azure/iot-edge-opc-proxy                    | 2017-04-27               |
>| Modbus            | https://github.com/Azure/iot-gateway-modbus                    | 2017-01-13               |
>| GZip Compression  | https://github.com/Azure/iot-gateway-compression-gzip-nodejs   | 2016-12-16               |
>| Proficy Historian | https://github.com/Azure-Samples/iot-edge-proficy-historian    | 2017-04-27               |
>| SQLite            | https://github.com/Azure/iot-gateway-sqlite                    | 2017-01-13               |
>| Batch/Shred       | https://github.com/Azure/iot-gateway-batch-nodejs              | 2017-01-13               |

We'd love to feature your module here! See our [Contribution guidelines](Contributing.md) for 
more info.

## Operating system compatibility
Azure IoT Edge is designed to be used with a broad range of operating system platforms. It has been tested on the following operating systems:

- Ubuntu 14.04
- Ubuntu 15.10
- Yocto Linux 3.0 on Intel Edison
- Windows 10
- Wind River 7.0

## Hardware compatibility
Azure IoT Edge is designed to be independent from hardware in addition to the operating system. 
Developers can power their gateways with hardware as constrained as a microcontroller to 
systems as powerful as a ruggedized server.

## Directory structure

### /doc
This folder contains general documentation for Azure IoT Edge as well as step by step instructions 
for building and running the samples:

General documentation

- [Dev box setup](doc/devbox_setup.md) contains instructions for configuring your machine to 
build Azure IoT Edge as well as instructions for configuring your machine to build 
modules written in Java, Node.js, and .NET.

- [Ubuntu Snap Package Walk-through](doc/azure_iot_gateway_snap.md) provides the step-by-step
instructions used to create Azure IoT Edge Snap package. The walk-through is a great
starting point for anyone trying to integrate Azure IoT Edge into their own snap
package.

API documentation can be found [here](http://azure.github.io/iot-edge).

### /samples
This folder contains all of the samples for Azure IoT Edge. Samples are separated 
in their own folders. Step by step instructions for building and running each sample can be found 
in the README.md file in the root of each sample's folder.

Samples include:
- [Hello World](samples/hello_world/README.md) - Learn the basic concepts of Azure IoT Edge by creating a simple gateway that logs a hello world message to a file every 5 seconds.
- [Simulated Device](samples/simulated_device_cloud_upload/README.md) Send data to IoTHub from
a gateway using a simulated device instead of using a real device. 
- [Real Device](samples/ble_gateway/README.md) - Send data to IoTHub from a real device that could not
connect to the cloud unless it connected through a gateway. This sample uses a Bluetooth Low Energy 
[Texas Instruments SensorTag](http://www.ti.com/ww/en/wireless_connectivity/sensortag2015/index.html) 
as the end device .

### /modules
This folder contains all of the modules included with Azure IoT Edge. Each module 
represents a specific piece of functionality that can be composed into an end to end gateway 
solution. Details on the implementation of each module can be found in each module's devdoc/ folder. 

### /core
This folder contains all of the core infrastructure necessary to create a gateway solution. 
In general, developers only need to *use* components in the core folder, not modify them. API 
documentation for core infrastructure can be found [here](http://azure.github.io/iot-edge/api_reference/c/html). 
Details on the implementation of core components can be found in [core/devdoc](core/devdoc).

### /build
This is the default folder that cmake will place the output from our build scripts. The developer 
always has the final say about the destinaiton of build output by creating a folder, navigating to 
it, and then running cmake from there. Detailed instructions are contained in each sample doc.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments
