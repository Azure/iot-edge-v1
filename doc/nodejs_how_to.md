# How-To (Node JS)
This document describes how to prepare your development environment to use the *Microsoft Azure IoT Gateway SDK* for Node JS module development.

- [Node JS](https://nodejs.org/)
- [Google V8](https://developers.google.com/v8/)
- [libuv](http://libuv.org/)

## Prerequisites
- Install [Python 2.7](https://www.python.org/downloads/release/python-2712/)
- Clone https://github.com/Azure/azure-iot-gateway-sdk.git

## Building
### Windows
Open a Developer Command Prompt for VS and: 

- cd <<your azure repo>>\tools
- Run build_nodejs.cmd ==> build_nodejs.cmd downloads and builds Node JS from source as runtime linked modules.
- Set 2 environment variables : `NODE_INCLUDE=<your azure-iot-gateway_root>\build_nodejs\dist\inc` and `NODE_LIB=<your aziore-iot-gateway_root>\%build-root%\build_nodejs\dist\lib`. These envioronment variables are a path to node.dll library and header files used by our Node JS binding. 
- run build.cmd --enable-nodejs-binding


### Linux
Open a terminal window and run the following commands:

- cd <<your azure gateway repo>>\tools
- build_nodejs.sh ==> build_nodejs.sh downloads and builds Node JS from source as runtime linked modules.
- Copy and paste the export message that shows up on screen so you have the NODE_INCLUDE and NODE_LIB environment variables set.
- build.sh --enable-nodejs-binding


## Running simple sample
### Windows
In order to run a gateway with a Node.js module do the following:

- Edit file under <<your azure gateway repo>>\samples\nodejs_simple_sample\src\gateway_sample.json
- Copy the configuration values from the sample given below.
- Copy `node.dll` from `NODE_LIB` to <<your azure gateway repo>>\build\samples\nodejs_simple_sample\Debug.
- On folder <<your azure gateway repo>>\build\samples\nodejs_simple_sample\Debug run the following command: 
- nodejs_simple_sample ..\..\..\..\samples\nodejs_simple_sample\src\gateway_sample.json

```
{
  "modules": [
    {
      "module name": "node_printer",
      "module path": "..\\..\\..\\bindings\\nodejs\\Debug\\nodejs_binding.dll",
      "args": {
        "main_path": "../../../../samples/nodejs_simple_sample/nodejs_modules/printer.js",
        "args": null
      }
    },
    {
      "module name": "node_sensor",
      "module path": "..\\..\\..\\bindings\\nodejs\\Debug\\nodejs_binding.dll",
      "args": {
        "main_path": "../../../../samples/nodejs_simple_sample/nodejs_modules/sensor.js",
        "args": null
      }
    },
    {
      "module name": "Logger",
      "module path": "..\\..\\..\\modules\\logger\\Debug\\logger.dll",
      "args": {
        "filename": "Log.txt"
      }
    }
  ],
  "links" : [
    { "source" : "*", "sink" : "Logger" }, 
    { "source" : "node_sensor", "sink" : "node_printer" }
  ]
}
```


### Linux
On a terminal windows follow these steps:
- Assuming that your azure-iot-gateway-sdk was cloned at ~/azure-iot-gateway-sdk/
- Edit file under ~/azure-iot-gateway-sdk/samples/nodejs_simple_sample/src/gateway_sample.json
- Copy the configuration values from the sample given below.
- On folder ~/azure-iot-gateway-sdk/build/samples/nodejs_simple_sample run the following command: 
- ./nodejs_simple_sample ../../../samples/nodejs_simple_sample/src/gateway_sample.json

Here is a sample of the gateway_sample.json file filled:
```
{
  "modules": [
    {
      "module name": "node_printer",
      "module path": "../../bindings/nodejs/libnodejs_binding.so",
      "args": {
        "main_path": "../../../samples/nodejs_simple_sample/nodejs_modules/printer.js",
        "args": null
      }
    },
    {
      "module name": "node_sensor",
      "module path": "../../bindings/nodejs/libnodejs_binding.so",
      "args": {
        "main_path": "../../../samples/nodejs_simple_sample/nodejs_modules/sensor.js",
        "args": null
      }
    },
    {
      "module name": "Logger",
      "module path": "../../modules/logger/liblogger.so",
      "args": {
        "filename": "log.txt"
      }
    }
  ],
  "links" : [
    { "source" : "*", "sink" : "Logger" },
    { "source" : "node_sensor", "sink" : "node_printer" }
  ]
}
```