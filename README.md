This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments

# Beta Azure IoT Gateway SDK
Using this SDK, developers write applications that enable gateway-connected devices to communicate with Azure IoT Hub. Such applications use a collection of **modules** to aggregate and transform data, process commands, or perform any number of related tasks. Modules communicate with one another via a message broker. Developers can choose from available modules or write their own to create gateways tailored to any scenario.

Visit http://azure.com/iotdev to learn more about developing applications for Azure IoT.

## SDK Modules
The following modules are available in this repository:
>| Name             | Description                                                             |
>|------------------|-------------------------------------------------------------------------|
>| ble              | Represents a Bluetooth low energy (BLE) device connected to the gateway |
>| hello_world      | Sends a "hello world" message periodically                              |
>| identitymap      | Maps MAC addresses to IoT Hub device IDs/keys                           |
>| iothub           | Sends/receives messages to/from mapped devices and IoT Hub              |
>| logger           | Writes received message content to a file                               |
>| simulated_device | Simulates a gateway-connected BLE device                                | 

## Featured Modules
Other people are creating modules for the gateway SDK too! See the **More information** link for a module to find out how to get it, who supports it, etc.
>| Name         | More information | Targets gateway SDK version |
>|--------------|------------------|-----------------------------|
>| placeholder  |                  |                             |

We'd love to feature your module here! See our [Contribution guidelines](Contributing.md) for more info.

## Operating system compatibility
The SDK is designed to be used with a broad range of operating system platforms. The Beta version of the Azure IoT Gateway SDK has been tested by means of development on the following operating systems:

- Ubuntu 14.04
- Ubuntu 15.10
- Yocto Linux 3.0 on Intel Edison
- Windows 10

## Hardware compatibility
The SDK is designed to be independent from hardware in addition to the operating system. Developers can power their gateways with hardware as constrained as a microcontroller to systems as powerful as a ruggedized server.

## Directory structure

### /doc
This folder contains general documentation for the SDK as well as step by step instructions for building and running the samples:

General documentation

- [Dev box setup](doc/devbox_setup.md) contains instructions for configure your machine to build the Azure IoT Gateway SDK.
- [Getting started](doc/getting_started.md) contains an introduction to the Azure IoT Gateway SDK as well as a detailed walkthrough of the [Hello World sample](doc/sample_hello_world.md).

Samples

- [Hello World sample](doc/sample_hello_world.md) contains step by step instructions for building and running the Hello World sample.
- [Simulated Device](doc/sample_simulated_device_cloud_upload.md) contains step by step instructions for building and running the Simulated Device sample.
- [TI Sensor Tag](doc/sample_ble.md) contains step by step instructions for building and running the TI Sensor Tag sample.


API documentation can be found [here](http://azure.github.io/azure-iot-gateway-sdk/api_reference/c/html).

### /samples
This folder contains all of the samples for the Azure IoT Gateway SDK. Samples are seperated in their own folders. Step by step instructions for building and running each sample can be found in a sample specific .md file located in azure-iot-gateway-sdk/doc.

### /modules
This folder contains all of the modules included with the Azure IoT Gateway SDK. Each module represents a specific piece of functionality that can be composed into an end to end gateway solution. Details on the implementation of each module can be found in each module's devdoc/ folder. 

### /core
This folder contains all of the core infrastructure necessary to create a gateway solution. In general, developers only need to use components in the core folder, not modify them. API documentation for core infrastructure can be found [here](http://azure.github.io/azure-iot-gateway-sdk/api_reference/c/html). Details on the implementation of core components can be found in the [core/devdoc](core/devdoc).

### /build
This is the default folder that cmake will place the output from our build scripts. The developer always has the final say about the destinaiton of build output by creating a folder, navigating to it, and then running cmake from there. Detailed instructions are contained in each sample doc.
