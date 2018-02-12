Azure IoT Edge V2 - Direct Method Invocation Sample (Linux only for now) 
=====================================================================

Overview
--------

This sample showcases how one can invoke a direct method from one module to another module 
or a downstream device on [Azure IoT Edge V2](https://docs.microsoft.com/en-us/azure/iot-edge/how-iot-edge-works). 

It uses the following simple scenario -
There is an edge deployment consisting of 2 custom modules -
1. An engine simulator module produces data which includes the engine RPM. It also exposes 
a Reset method, which if called, resets the engine to its initial value.
2. An analyzer module analyzes the data sent by the engine simulator module and invokes a 
Reset direct method call on the engine simulator module if the RPM value is too high.

Invoking direct methods
--------------

A module can invoke a direct method on another module or a downstream device (i.e. a device
connected to the same EdgeHub), by using its EdgeHubConnectionString. It cannot invoke direct methods
on modules on other edge devices, or devices not connected to the same EdgeHub (unless it uses an 
IotHub scoped connection string).

Here are the steps to invoke a direct method from one module to another -

- You need to use the ServiceClient to invoke direct methods from a module. The ServiceClient is in the Microsoft.Azure.Devices nuget package which is not added by default, so you need to manually add that. Make sure to add a reference to a preview* version of the nuget package, to get modules support. 

```
dotnet add package Microsoft.Azure.Devices -Version 1.15.0-preview-001
```

- Create a ServiceClient using the EdgeHubConnectionString that is passed into the module as an 
environment variable. 
```
ServiceClient serviceClient = ServiceClient.CreateFromConnectionString(connectionString);
```
> Note: **Do not** call OpenAsync on the ServiceClient when initializating it with the EdgeHubConnectionString.
This will cause it to try to open the AMQP connection which is not supported, causing it to throw an exception. Method invocation happens over HTTP. 

- Invoke direct method on another module like this -
```
var response = await serviceClient.InvokeDeviceMethodAsync(message.ConnectionDeviceId, message.ConnectionModuleId, new CloudToDeviceMethod("Reset"));                    
```
> Note: An incoming message contains the device Id in the property ConnectionDeviceId, and the
module Id in the property ConnectionModuleId (if the message is from another module). You can 
use those values to invoke a method on that device or module.

- Receiving direct method invocations from a module is same as if receiving it from the cloud. 

```
await deviceClient.SetMethodHandlerAsync("Reset", MethodHandler, userContext);
```

Building and running the sample (Linux)
---------------------------

This sample follows the same steps for developing custom Azure IoT Edge modules as 
described [here](https://docs.microsoft.com/en-us/azure/iot-edge/tutorial-csharp-module). 
You can follow the same steps for both the AnalyzerModule and the EngineSimulatorModule. 

### Building 

For each of the modules, do the following -
- Build the module
- Publish the bits
- Build the docker image using the included Dockerfile
- Push the docker image to a registry such as [Azure Container Registry](https://azure.microsoft.com/en-us/services/container-registry/)

### Deploying

Go to the Azure Portal, and follow the steps [here](https://docs.microsoft.com/en-us/azure/iot-edge/quickstart-linux#deploy-a-module) to create
a deployment with the 2 modules. 

Also set the following 2 routes in the deployment - 
```
"routes": {
    "upstreamRoute": "FROM /messages/modules/engineSimulator INTO $upstream",
    "analyzerRoute": "FROM /messages/modules/engineSimulator INTO BrokeredEndpoint                      (\"/modules/engineAnalyzer/inputs/input1\")"			
}
```

The file [deployment.json](deployment.json) shows how the final deployment should look like. 

### Running

Start the IoT Edge runtime as described [here](https://docs.microsoft.com/en-us/azure/iot-edge/quickstart-linux#install-and-start-the-iot-edge-runtime).

The engineSimulator module should generate data and the analyzerModule should receive it. If the value goes above a threshold, the analyzerModule will invoke the Reset method, which will cause the engineSimulator to get reset. 