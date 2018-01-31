# Migrating from V1 to V2 of Azure IoT Edge

Remote configuration, deployment, and monitoring of IoT Edge devices has been on the road map of Azure IoT Edge. These features are now being released as a public preview. Some internal architecture has changed to support these features in an industry standard way; however all of the important developer concepts are maintained:
* modules remain units compute which can be written in your programming language of choice.
* traditional cloud services and 3rd party business logic can run as a module.
* modules can communicate with each other via declarative message passing
Continuity of concepts allows existing Azure IoT Edge solutions to be easily migrated to the new architecture.

## Overview 
There are six specific changes that must be accounted for when moving a module from V1 to V2:
1.	Module format - Modules are implemented as containers instead of DLLs, SOs, jars, .js files or .NET assemblies. 
2.	Module lifetime - Modules are started by the IoT Edge runtime and run in their own processes instead of running as a thread in the gateway process. 
3.	Module configuration - Modules fetch their configuration data from a module twin instead of having it provided as an argument at startup.
4.	Message passing - Modules use the IoT Edge runtime for sending and receiving messages instead of the broker. 
5.	Declaring routes - The order in which messages are passed between modules is defined by routes on the IoT Edge runtime instead of links.
6.	Talking to the cloud - Communication with IoT Hub is provided by the IoT Edge runtime instead of a separate module.
The following diagram illustrates these changes.

## Specifics
Details on each of the migration steps are below. The [tutorial on writing a module in C#](https://docs.microsoft.com/en-us/azure/iot-edge/tutorial-csharp-module) covers the V2 technical details of each of these.

### Module format
Modules allow for creation of units of compute in different languages. They started off with language specific formats. For example: C modules were either a DLLs or SOs, Java modules were jars, and .NET modules were assemblies.
Module format has changed to the industry standard of containers. This has many benefits, primarily containers...
* are an industry standard.
* include mechanisms for distribution.
* simplify dependency management on the host system since they include all dependencies for code which runs inside the container.
* do not require code changes to code distributed inside of them.

### Module lifetime
Modules used to be started by the gateway process.  It would read a list of modules from the JSON config file and start each one, usually as an internal thread.  This has been changed to a completely out of proc architecture.
The IoT Edge runtime now receives a list of modules to start, downloads them, and starts them in their own processes. The lifetime of that process is controlled by the lifetime of the container in which the process runs. This implies that no special module code is necessary to respond to lifetime events. Specifically, implementing the following interface is no longer necessary.<br>
`static const MODULE_API_1 api =`<br>
`{`<br>
&nbsp;&nbsp;&nbsp;`{MODULE_API_VERSION_1},`<br>
&nbsp;&nbsp;&nbsp;`parse_configuration_from_json,`<br>
&nbsp;&nbsp;&nbsp;`free_configuration,`<br>
&nbsp;&nbsp;&nbsp;`create,`<br>
&nbsp;&nbsp;&nbsp;`destroy,`<br>
&nbsp;&nbsp;&nbsp;`receive,`<br>
&nbsp;&nbsp;&nbsp;`start`<br>
`};`<br>

### Module configuration
Configuration data used to be provided to modules by the gateway. The gateway process parsed configuration arguments out of the JSON config file and passed it to them in the `start` method.
Each module now has an individual module twin which can be used to provide a module with configuration information. The module must first start and then read its twin to receive this data. More information on modules and module twins can be found in [this article](https://docs.microsoft.com/en-us/azure/iot-edge/iot-edge-modules).

### Message passing
Modules used to use a broker inside the gateway process to exchange messages. This functionality is now provided by the IoT Edge runtime.
* Sending message to the runtime - Call `DeviceClient.SendEventAsync` instead of `Broker_Publish`
* Receiving a message from the runtime - Register a delegate for receiving messages with `DeviceClient.SetInputMessageHandlerAsync` instead of setting the `pfModule_Receive` function pointer in the `MODULE_API` structure.
Notice that message passing now uses the same device client code that users are already familiar with when interacting with IoT Hub. The consistency simplifies the number of programing module regardless of whether you're working in the cloud or on the edge. 

### Declaring routes
The original broker used Links specified in the JSON config file to determine how to pass messages between modules. The IoT Edge runtime uses a very similar concept called routes. Use these when declaring your module pipelines. You can find out more about routes in [this article](https://docs.microsoft.com/en-us/azure/iot-edge/module-composition). 

### Talking to the cloud
Communication with IoT Hub was provided by the `iot_hub` module. This functionality has been built into the IoT Edge runtime. It provides the `$UPSTREAM` endpoint, a simple alias that can be used in routes for referring to IoT Hub.