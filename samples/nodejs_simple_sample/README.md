# How-To Enable Node JS Module Development
This document describes how to prepare your development environment to use the *Microsoft Azure IoT Gateway SDK* for Node JS module development.
- [Node JS](https://nodejs.org/)
- [Google V8](https://developers.google.com/v8/)
- [libuv](http://libuv.org/)

## Prerequisites
- Install [Python 2.7](https://www.python.org/downloads/release/python-2712/)
- Ensure Python is added to your `PATH` environment variable (*Control Panel - Edit environment variables for your account*)
- [Prepare your development environment](../../doc/devbox_setup.md)

## Building simple sample
### Windows
From a Visual Studio Developer Command Prompt:
- `cd <azure_iot_gateway_sdk_root>\tools`
- `build_nodejs.cmd`
  - Will download and build Node JS from source as runtime linked modules
- Copy and paste the set message that shows up on screen to set the `NODE_INCLUDE` and `NODE_LIB` environment variables
- `build.cmd --enable-nodejs-binding`


### Linux
From the command line:
- `cd <azure_iot_gateway_sdk_root>/tools`
- `build_nodejs.sh`
  - Will download and build Node JS from source as runtime linked modules
- Copy and paste the export message that shows up on screen to set the `NODE_INCLUDE` and `NODE_LIB` environment variables
- `build.sh --enable-nodejs-binding`


## Running simple sample

### Windows
In order to run a gateway with a Node.js module do the following:
- Update the `iothub_writer` module config JSON (*shown below*) in `<azure_iot_gateway_sdk_root>\samples\nodejs_simple_sample\src\gateway_sample_win.json` by replacing `<IoTHub device connection string>` with your actual IoT Hub device connection string
```
        {
            "name": "iothub_writer",
            "loader": {
                "name": "node",
                "entrypoint": {
                    "main.path": "../../../samples/nodejs_simple_sample/nodejs_modules/iothub_writer.js"
                }
            },
            "args": {
                "connection_string": "<IoTHub device connection string>"
            }
        },
```
- `cd <azure_iot_gateway_sdk_root>\build\samples\nodejs_simple_sample\Debug`
- `nodejs_simple_sample ..\..\..\..\samples\nodejs_simple_sample\src\gateway_sample_win.json`

### Linux
On a terminal windows follow these steps:
- Update the `iothub_writer` module config JSON (*shown below*) in `<azure_iot_gateway_sdk_root>\samples\nodejs_simple_sample\src\gateway_sample_lin.json` by replacing `<IoTHub device connection string>` with your actual IoT Hub device connection string
```
        {
            "name": "iothub_writer",
            "loader": {
                "name": "node",
                "entrypoint": {
                    "main.path": "../../../samples/nodejs_simple_sample/nodejs_modules/iothub_writer.js"
                }
            },
            "args": {
                "connection_string": "<IoTHub device connection string>"
            }
        },
```
- `cd <azure_iot_gateway_sdk_root>/build/samples/nodejs_simple_sample/Debug`
- `nodejs_simple_sample ../../../../samples/nodejs_simple_sample/src/gateway_sample_win.json`


## Simple sample output
On successful run you should see **sample** output like this

```
Gateway is running. Press return to quit.
printer.receive - 47, 33
printer.receive - 11, 12
printer.receive - 39, 30
printer.receive - 30, 21
printer.receive - 12, 0

Gateway is quitting
printer.destroy
sensor.destroy
iothub_writer.destroy
```
